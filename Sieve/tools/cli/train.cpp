// Sieve CLI — training runs and `sieve ai-train` (see train.hpp).
#include "cli/train.hpp"

#include "cli/locate.hpp"
#include "cli/pack.hpp"
#include "sieve/sha256.hpp"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <thread>

namespace fs = std::filesystem;

namespace sieve::cli {

namespace {

fs::path path_of(const std::string& s) { return fs::path(std::u8string(s.begin(), s.end())); }

std::string text_of(const std::vector<uint8_t>& b) { return std::string(b.begin(), b.end()); }

void write_text(const fs::path& p, const std::string& t)
{
    std::ofstream out(p, std::ios::binary);
    if (!out || !out.write(t.data(), std::streamsize(t.size()))) throw std::invalid_argument("cannot write " + p.string());
}

TrainingRun run_of_files(std::string name, std::map<std::string, std::vector<uint8_t>> files)
{
    TrainingRun r;
    r.name = std::move(name);
    const auto ini = files.find("training.ini"), order = files.find("order.txt");
    if (ini == files.end()) throw std::invalid_argument("a training run holds training.ini");
    r.recipe = training::Recipe::parse(text_of(ini->second));
    if (r.recipe.order == "order-v1")
    {
        // The order made from the recipe's lines, not written out.
        if (order != files.end()) throw std::invalid_argument("the run's order is order-v1, so it holds no order.txt");
        std::vector<std::pair<std::string, uint64_t>> sizes;
        for (const std::string& f : r.recipe.files)
        {
            const auto it = files.find(f);
            if (it == files.end()) throw std::invalid_argument("training.ini reads " + f + ", which the run does not hold");
            sizes.push_back({f, it->second.size()});
        }
        r.order = training::make_order(sizes, r.recipe.window, r.recipe.epochs, r.recipe.seed, r.recipe.reading);
    }
    else
    {
        if (order == files.end()) throw std::invalid_argument("the run's order is written out, and it holds no order.txt");
        r.order = training::parse_order(text_of(order->second));
    }
    for (const training::Window& w : r.order)
    {
        const auto f = files.find(w.path);
        if (f == files.end()) throw std::invalid_argument("the order names " + w.path + ", which the run does not hold");
        if (w.offset + w.length > f->second.size())
            throw std::invalid_argument("the order reads past the end of " + w.path + " (" + std::to_string(f->second.size()) + " bytes)");
    }
    if (const auto res = files.find("result.txt"); res != files.end()) r.result = text_of(res->second);
    r.files = std::move(files);
    return r;
}

} // namespace

TrainingRun load_training_run(const fs::path& p)
{
    std::map<std::string, std::vector<uint8_t>> files;
    if (fs::is_directory(p))
    {
        for (const fs::directory_entry& e : fs::recursive_directory_iterator(p))
            if (e.is_regular_file()) files[manifest_path(e.path(), p)] = read_file_bytes(e.path());
        const std::u8string n = fs::absolute(p).lexically_normal().filename().u8string();
        return run_of_files(std::string(n.begin(), n.end()), std::move(files));
    }
    const Manifest m = installable_manifest(p, false);
    uint64_t at = 0;
    for (const ManifestEntry& e : m.entries)
    {
        if (e.dir) continue;
        files[e.path] = std::vector<uint8_t>(m.contents.begin() + std::ptrdiff_t(at), m.contents.begin() + std::ptrdiff_t(at + e.size));
        at += e.size;
    }
    return run_of_files(m.root, std::move(files));
}

namespace {

// A run's saved state (sieve-training-state-v1): text lines, then the masters and the momenta,
// each 8 bytes little-endian, a weight at a time.
//     sieve-training-state-v1
//     recipe <SHA-256 of training.ini>
//     order <SHA-256 of the order, as order.txt would write it>
//     step <steps done>
//     recent <score sum> <predictions> <bits>      (since the last checkpoint)
//     weights <W>
//     result <bytes>                               then the result so far, as result.txt writes it
//     end
struct SavedState
{
    uint64_t step = 0;
    training::Score recent;
    training::Result result;
    std::vector<int64_t> masters, velocities;
};

std::string run_identity(const TrainingRun& run, const char* what)
{
    if (std::string(what) == "recipe") return Sha256::hex(Sha256::hash(run.recipe.text()));
    return Sha256::hex(Sha256::hash(training::order_text(run.order)));
}

void put_i64s(std::string& out, const std::vector<int64_t>& v)
{
    for (const int64_t x : v)
    {
        const uint64_t u = uint64_t(x);
        for (int k = 0; k < 8; ++k) out.push_back(char((u >> (8 * k)) & 0xFF));
    }
}

void save_state(const fs::path& file, const TrainingRun& run, const SavedState& st)
{
    std::string head = "sieve-training-state-v1\nrecipe " + run_identity(run, "recipe") + "\norder " + run_identity(run, "order") + "\nstep " +
                       std::to_string(st.step) + "\n";
    char recent[96];
    std::snprintf(recent, sizeof recent, "recent %llu %llu %.17g\n", (unsigned long long)st.recent.prob_sum,
                  (unsigned long long)st.recent.predictions, st.recent.bits);
    const std::string result = st.result.text();
    head += recent;
    head += "weights " + std::to_string(st.masters.size()) + "\nresult " + std::to_string(result.size()) + "\n" + result + "end\n";
    std::string body = head;
    body.reserve(head.size() + 16 * st.masters.size());
    put_i64s(body, st.masters);
    put_i64s(body, st.velocities);
    // Written whole, then put in place, so a state is never half written.
    const fs::path tmp = fs::path(file).concat(".tmp");
    write_text(tmp, body);
    std::error_code ec;
    fs::rename(tmp, file, ec);
    if (ec)
    {
        fs::remove(file, ec);
        fs::rename(tmp, file);
    }
}

std::optional<SavedState> load_state(const fs::path& file, const TrainingRun& run, uint64_t weights)
{
    if (!fs::exists(file)) return std::nullopt;
    const std::vector<uint8_t> bytes = read_file_bytes(file);
    const std::string all(bytes.begin(), bytes.end());
    size_t at = 0;
    auto line = [&]() {
        const size_t nl = all.find('\n', at);
        if (nl == std::string::npos) throw std::invalid_argument(file.string() + " is not a training state");
        std::string l = all.substr(at, nl - at);
        at = nl + 1;
        return l;
    };
    auto field = [&](const std::string& key) {
        const std::string l = line();
        if (l.rfind(key + " ", 0) != 0) throw std::invalid_argument(file.string() + " is not a training state");
        return l.substr(key.size() + 1);
    };
    if (line() != "sieve-training-state-v1") throw std::invalid_argument(file.string() + " is not a training state");
    if (field("recipe") != run_identity(run, "recipe") || field("order") != run_identity(run, "order"))
        throw std::invalid_argument(file.string() + " holds another run's state: move it away to train this run from its start");
    SavedState st;
    st.step = std::stoull(field("step"));
    {
        std::istringstream r(field("recent"));
        r >> st.recent.prob_sum >> st.recent.predictions >> st.recent.bits;
    }
    if (std::stoull(field("weights")) != weights) throw std::invalid_argument(file.string() + " holds a state of another shape");
    const size_t rn = size_t(std::stoull(field("result")));
    st.result = training::Result::parse(all.substr(at, rn));
    at += rn;
    if (line() != "end" || all.size() - at != 16 * weights) throw std::invalid_argument(file.string() + " is cut short");
    auto get = [&](std::vector<int64_t>& v) {
        v.resize(size_t(weights));
        for (int64_t& x : v)
        {
            uint64_t u = 0;
            for (int k = 0; k < 8; ++k) u |= uint64_t(uint8_t(all[at + size_t(k)])) << (8 * k);
            x = int64_t(u);
            at += 8;
        }
    };
    get(st.masters);
    get(st.velocities);
    return st;
}

} // namespace

fs::path state_file_of(const fs::path& run)
{
    fs::path p = fs::absolute(run).lexically_normal();
    if (!p.has_filename()) p = p.parent_path();
    return p.concat(".resume");
}

std::optional<uint64_t> saved_step(const TrainingRun& run, const fs::path& state)
{
    try
    {
        const AiShape& s = run.recipe.shape;
        if (auto st = load_state(state, run, 256ull * s.width + 16ull * s.layers * s.width * s.width)) return st->step;
    }
    catch (const std::exception&)
    {
    }
    return std::nullopt;
}

TrainingOutcome train_run(const TrainingRun& run, unsigned threads, const TrainingCallback& progress, const std::atomic<bool>* cancel,
                          const fs::path& state, uint64_t stop_after)
{
    const training::Recipe& rc = run.recipe;
    const AiSpace space(rc.shape, rc.key);
    const AiSpace::Digits start = space.digits_at(space.parse(rc.start), rc.start_mode);
    training::Trainer trainer(space, start, rc, threads);
    TrainingOutcome out;
    auto address = [&]() { return space.hex_of(space.index_of(trainer.digits(), AddressMode::Positional)); };
    const uint64_t steps = run.steps();
    out.result.version = rc.result_version();
    out.result.trainer = rc.version == 1 ? training::kTrainingVersion : training::kTrainingVersion2;
    out.result.steps = steps;
    // A checkpoint: its hash (v2), and its address where the result keeps them.
    auto checkpoint = [&](uint64_t step, const training::Score& score) {
        training::Checkpoint c{step, score, {}, {}};
        const AiSpace::Digits& d = trainer.digits();
        if (out.result.version >= 2) c.digest = Sha256::hex(Sha256::hash(std::string_view(reinterpret_cast<const char*>(d.data()), d.size())));
        if (out.result.version == 1 || rc.checkpoint_addresses) c.address = address();
        out.result.checkpoints.push_back(std::move(c));
    };
    training::Score recent;
    uint64_t first = 0;
    std::optional<SavedState> saved;
    if (!state.empty()) saved = load_state(state, run, space.weights());
    if (saved)
    {
        // Carry on from the saved state, as if training had not stopped.
        if (saved->result.version != out.result.version || saved->step > steps) throw std::invalid_argument("the saved state does not fit this run");
        trainer.restore(saved->masters, saved->velocities);
        out.result.checkpoints = std::move(saved->result.checkpoints);
        recent = saved->recent;
        first = out.resumed_from = saved->step;
    }
    else checkpoint(0, {});
    auto keep_state = [&](uint64_t done) {
        if (state.empty()) return;
        SavedState st;
        st.step = done;
        st.recent = recent;
        st.result.version = out.result.version;
        st.result.steps = steps;
        st.result.checkpoints = out.result.checkpoints;
        st.masters = trainer.masters();
        st.velocities = trainer.velocities();
        save_state(state, run, st);
    };
    for (uint64_t k = first; k < steps; ++k)
    {
        if ((cancel && cancel->load()) || (stop_after && k >= stop_after))
        {
            keep_state(k);
            out.cancelled = true;
            out.stopped_at = k;
            return out;
        }
        std::vector<std::span<const uint8_t>> batch;
        for (uint64_t i = k * rc.batch; i < std::min<uint64_t>(run.order.size(), (k + 1) * rc.batch); ++i)
        {
            const training::Window& w = run.order[size_t(i)];
            batch.push_back(std::span<const uint8_t>(run.files.at(w.path).data() + w.offset, w.length));
        }
        const training::Score s = trainer.step(batch);
        recent.prob_sum += s.prob_sum, recent.predictions += s.predictions, recent.bits += s.bits;
        if (progress) progress(k + 1, steps, recent);
        if ((k + 1) % rc.checkpoint_every == 0 || k + 1 == steps)
        {
            checkpoint(k + 1, recent);
            recent = {};
            if (k + 1 < steps) keep_state(k + 1);
        }
    }
    if (!state.empty())
    {
        std::error_code ec;
        fs::remove(state, ec); // done: nothing to carry on from
    }
    out.digits = trainer.digits();
    out.result.final_address = address();
    out.result.model_sha256 = Sha256::hex(Sha256::hash(space.safetensors_of(out.digits)));
    out.text = out.result.text();
    return out;
}

std::optional<std::string> result_difference(const std::string& expected, const std::string& got)
{
    if (expected == got) return std::nullopt;
    std::istringstream a(expected), b(got);
    std::string la, lb;
    for (size_t n = 1;; ++n)
    {
        const bool ha = bool(std::getline(a, la)), hb = bool(std::getline(b, lb));
        if (!ha && !hb) return "line endings";
        if (ha != hb || la != lb)
        {
            auto cut = [](const std::string& s) { return s.size() > 60 ? s.substr(0, 60) + "..." : s; };
            return "line " + std::to_string(n) + ": expected \"" + (ha ? cut(la) : std::string("(nothing)")) + "\", got \"" +
                   (hb ? cut(lb) : std::string("(nothing)")) + "\"";
        }
    }
}

std::vector<TrainingFile> corpus_files(const fs::path& corpus)
{
    if (!fs::exists(corpus)) throw std::invalid_argument("no corpus at " + corpus.string());
    if (!fs::is_directory(corpus)) throw std::invalid_argument("a corpus is a folder: " + corpus.string() + " is a file");
    std::vector<TrainingFile> out;
    for (const fs::directory_entry& e : fs::recursive_directory_iterator(corpus))
        if (e.is_regular_file()) out.push_back({e.path(), "corpus/" + manifest_path(e.path(), corpus), e.file_size()});
    std::sort(out.begin(), out.end(), [](const TrainingFile& a, const TrainingFile& b) { return a.path < b.path; });
    if (out.empty()) throw std::invalid_argument("the corpus has no files");
    return out;
}

TrainingRun record_training_run(const std::vector<TrainingFile>& sources, const fs::path& out, const RecordOptions& o)
{
    training::Recipe rc = o.recipe;
    rc.check();
    if (fs::exists(out) && !fs::is_empty(out)) throw std::invalid_argument(out.string() + " is not empty: a run is recorded into a new folder");
    std::map<std::string, std::vector<uint8_t>> files;
    std::vector<std::pair<std::string, uint64_t>> sizes;
    rc.files.clear();
    for (const TrainingFile& f : sources)
    {
        if (files.count(f.path)) throw std::invalid_argument(f.path + " is in the corpus twice");
        files[f.path] = read_file_bytes(f.source);
        sizes.push_back({f.path, files[f.path].size()});
        rc.files.push_back(f.path);
    }
    const std::vector<training::Window> order = training::make_order(sizes, rc.window, rc.epochs, rc.seed, rc.reading);
    if (order.empty()) throw std::invalid_argument("the corpus has nothing to learn from: no file of two bytes or more");
    if (o.write_order)
    {
        // The order written out (order.txt) rather than made from the recipe: the files are then
        // only listed by the order.
        rc.order = "order.txt";
        files["order.txt"] = [&] { const std::string t = training::order_text(order); return std::vector<uint8_t>(t.begin(), t.end()); }();
    }
    else rc.order = "order-v1";
    files["training.ini"] = [&] { const std::string t = rc.text(); return std::vector<uint8_t>(t.begin(), t.end()); }();
    for (const auto& [path, bytes] : files)
    {
        const fs::path dest = out / path_of(path);
        fs::create_directories(dest.parent_path());
        write_text(dest, text_of(bytes));
    }
    const std::u8string n = fs::absolute(out).lexically_normal().filename().u8string();
    return run_of_files(std::string(n.begin(), n.end()), std::move(files));
}

TrainingRun record_training_run(const fs::path& corpus, const fs::path& out, const RecordOptions& o)
{
    return record_training_run(corpus_files(corpus), out, o);
}

std::string suggested_learning_rate(const training::Recipe& rc)
{
    const double predictions = double(rc.batch) * double(std::max(1u, rc.window - 1));
    const double x = 0.008 / double(std::max(1u, rc.shape.width)) * 256.0 / predictions;
    // The 1-2-5 steps, as decimals written exactly, down to the smallest the recipe's arithmetic
    // holds to 8 parts or more (2^-13 under training-v1's Q16; 2^-20 with a loss scale of 7 bits).
    static const char* kSteps[] = {"0.0000005", "0.000001", "0.000002", "0.000005", "0.00001", "0.00002", "0.00005", "0.0001",
                                   "0.0002",    "0.0005",   "0.001",    "0.002",    "0.005",   "0.01",    "0.02",    "0.05", "0.1"};
    const double floor = 8.0 / double(uint64_t(1) << (16 + (rc.version >= 2 ? rc.loss_scale_bits : 0)));
    std::string best;
    for (const char* step : kSteps)
        if (std::stod(step) >= floor * (1 - 1e-9) && (best.empty() || std::stod(step) <= x * (1 + 1e-9))) best = step;
    return best;
}

uint32_t suggested_loss_scale(const training::Recipe& rc)
{
    return rc.version >= 2 && (rc.shape.width >= 64 || rc.shape.layers >= 4) ? 6 : 0;
}

TrainingEstimate estimate_training(const training::Recipe& rc, const std::vector<TrainingFile>& files, unsigned threads)
{
    TrainingEstimate e;
    const AiShape& s = rc.shape;
    e.weights = 256ull * s.width + 16ull * s.layers * s.width * s.width;
    const uint64_t step = std::max(1u, rc.window - 1);
    for (const TrainingFile& f : files)
        if (f.size >= 2) e.windows += (f.size - 2) / step + 1;
    e.windows *= rc.epochs;
    e.steps = (e.windows + rc.batch - 1) / rc.batch;
    const unsigned hw = threads ? threads : std::max(1u, std::thread::hardware_concurrency());
    e.threads = std::max(1u, std::min(hw, rc.batch));
    // What a window keeps for its way back (training.cpp's run_window): in each layer some 37 values
    // a position a unit of width, and each head's attention, position by position; and each thread's
    // gradient, a value a weight. The trainer itself keeps three values a weight, and the digits.
    const uint64_t T = step, H = s.width, A = s.heads;
    e.bytes_per_thread = 8 * (uint64_t(s.layers) * (37 * T * H + A * T * T) + 4 * T * H + 1024) + 8 * e.weights;
    e.bytes = uint64_t(e.threads) * e.bytes_per_thread + 25 * e.weights;
    // result.txt: the final address in hex, and a line of about 100 bytes a checkpoint (with the
    // address, when they are kept).
    const uint64_t address = (e.weights * s.bits + 3) / 4;
    const uint64_t checkpoints = e.steps / std::max(1u, rc.checkpoint_every) + 2;
    e.result_bytes = address + 200 + checkpoints * (100 + (rc.checkpoint_addresses ? address : 0));
    e.learning_rate_at_floor = rc.learning_rate_q() <= 8;
    return e;
}

double measure_training_step(const training::Recipe& rc, const std::vector<TrainingFile>& files, unsigned threads)
{
    // The first windows of the files, end to end: a step as the run's steps are, give or take
    // which windows.
    std::vector<uint8_t> bytes;
    const size_t want = size_t(rc.batch) * rc.window;
    for (const TrainingFile& f : files)
    {
        if (bytes.size() >= want) break;
        std::ifstream in(f.source, std::ios::binary);
        std::vector<char> buf(std::min<size_t>(want - bytes.size(), size_t(f.size)));
        in.read(buf.data(), std::streamsize(buf.size()));
        bytes.insert(bytes.end(), buf.begin(), buf.begin() + in.gcount());
    }
    if (bytes.size() < 2) return 0;
    std::vector<std::span<const uint8_t>> batch;
    for (size_t at = 0; at + 1 < bytes.size() && batch.size() < rc.batch; at += rc.window - 1)
        batch.push_back(std::span<const uint8_t>(bytes.data() + at, std::min<size_t>(rc.window, bytes.size() - at)));
    const AiSpace space(rc.shape, rc.key);
    training::Trainer trainer(space, space.digits_at(space.parse(rc.start), rc.start_mode), rc, threads);
    const auto t0 = std::chrono::steady_clock::now();
    trainer.step(batch);
    return std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
}

void pack_training_run(const fs::path& folder, const fs::path& sieve_file)
{
    Manifest m = walk_folder(folder);
    add_packed(m, folder);
    write_address_file(sieve_file, binary_address(m.file()), false);
}

ModelRun model_run_of(const std::string& id, const std::vector<std::filesystem::path>& dirs)
{
    namespace fs = std::filesystem;
    const fs::path given(std::u8string(id.begin(), id.end()));
    fs::path where;
    if (fs::exists(given)) where = given;
    for (const fs::path& d : dirs)
    {
        if (!where.empty()) break;
        for (const fs::path& p : {d / given, d / fs::path(given).concat(".sieve")})
            if (fs::exists(p))
            {
                where = p;
                break;
            }
    }
    if (where.empty()) throw std::invalid_argument("no training run named '" + id + "' (looked beside the programs and in data/training)");
    const TrainingRun run = load_training_run(where);
    ModelRun m;
    m.id = id;
    m.layers = run.recipe.shape.layers, m.width = run.recipe.shape.width, m.heads = run.recipe.shape.heads, m.bits = run.recipe.shape.bits;
    if (run.result) m.final_address = training::Result::parse(*run.result).final_address;
    // Its text: the first file it reads (order-v1's first file; a written order's first window's file).
    const std::string first = !run.recipe.files.empty() ? run.recipe.files.front() : !run.order.empty() ? run.order.front().path : std::string();
    if (auto it = run.files.find(first); it != run.files.end()) m.text = it->second, m.text_name = first;
    Sha256 h;
    h.update(std::to_string(m.layers) + "/" + std::to_string(m.width) + "/" + std::to_string(m.heads) + "/" + std::to_string(m.bits) + "\n" +
             m.final_address + "\n" + m.text_name + "\n");
    h.update(std::span<const uint8_t>(m.text));
    m.sha256 = Sha256::hex(h.finish());
    return m;
}

void use_training_runs(const std::vector<std::filesystem::path>& dirs)
{
    set_model_run_loader([dirs](const std::string& id) { return model_run_of(id, dirs); });
}

} // namespace sieve::cli
