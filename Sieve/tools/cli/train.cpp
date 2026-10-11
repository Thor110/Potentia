// Sieve CLI — training runs and `sieve ai-train` (see train.hpp).
#include "cli/train.hpp"

#include "cli/locate.hpp"
#include "cli/pack.hpp"
#include "sieve/sha256.hpp"

#include <algorithm>
#include <chrono>
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

TrainingOutcome train_run(const TrainingRun& run, unsigned threads, const TrainingCallback& progress, const std::atomic<bool>* cancel)
{
    const training::Recipe& rc = run.recipe;
    const AiSpace space(rc.shape, rc.key);
    const AiSpace::Digits start = space.digits_at(space.parse(rc.start), rc.start_mode);
    training::Trainer trainer(space, start, rc, threads);
    TrainingOutcome out;
    auto address = [&]() { return space.hex_of(space.index_of(trainer.digits(), AddressMode::Positional)); };
    const uint64_t steps = run.steps();
    out.result.version = rc.result_version();
    out.result.steps = steps;
    // A checkpoint: its hash (v2), and its address where the result keeps them.
    auto checkpoint = [&](uint64_t step, const training::Score& score) {
        training::Checkpoint c{step, score, {}, {}};
        const AiSpace::Digits& d = trainer.digits();
        if (out.result.version >= 2) c.digest = Sha256::hex(Sha256::hash(std::string_view(reinterpret_cast<const char*>(d.data()), d.size())));
        if (out.result.version == 1 || rc.checkpoint_addresses) c.address = address();
        out.result.checkpoints.push_back(std::move(c));
    };
    checkpoint(0, {});
    training::Score recent;
    for (uint64_t k = 0; k < steps; ++k)
    {
        if (cancel && cancel->load())
        {
            out.cancelled = true;
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
        }
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
    // The 1-2-5 steps from 0.00002 up, as decimals written exactly.
    static const char* kSteps[] = {"0.00002", "0.00005", "0.0001", "0.0002", "0.0005", "0.001", "0.002", "0.005", "0.01", "0.02", "0.05", "0.1"};
    std::string best = kSteps[0];
    for (const char* step : kSteps)
        if (std::stod(step) <= x * (1 + 1e-9)) best = step;
    return best;
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
    e.learning_rate_at_floor = rc.learning_rate_q16() <= 4;
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

} // namespace sieve::cli
