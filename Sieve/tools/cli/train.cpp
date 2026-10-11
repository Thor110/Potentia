// Sieve CLI — training runs and `sieve ai-train` (see train.hpp).
#include "cli/train.hpp"

#include "cli/locate.hpp"
#include "cli/pack.hpp"
#include "sieve/sha256.hpp"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <stdexcept>

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
    if (ini == files.end() || order == files.end()) throw std::invalid_argument("a training run holds training.ini and order.txt");
    r.recipe = training::Recipe::parse(text_of(ini->second));
    r.order = training::parse_order(text_of(order->second));
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
    out.result.steps = steps;
    out.result.checkpoints.push_back({0, {}, address()});
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
            out.result.checkpoints.push_back({k + 1, recent, address()});
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

TrainingRun record_training_run(const fs::path& corpus, const fs::path& out, const RecordOptions& o)
{
    o.recipe.check();
    if (!fs::exists(corpus)) throw std::invalid_argument("no corpus at " + corpus.string());
    if (fs::exists(out) && !fs::is_empty(out)) throw std::invalid_argument(out.string() + " is not empty: a run is recorded into a new folder");
    std::vector<std::pair<fs::path, std::string>> sources; // the file, and its path in the run
    if (fs::is_directory(corpus))
    {
        for (const fs::directory_entry& e : fs::recursive_directory_iterator(corpus))
            if (e.is_regular_file()) sources.push_back({e.path(), "corpus/" + manifest_path(e.path(), corpus)});
    }
    else
    {
        const std::u8string n = corpus.filename().u8string();
        sources.push_back({corpus, "corpus/" + std::string(n.begin(), n.end())});
    }
    std::sort(sources.begin(), sources.end(), [](const auto& a, const auto& b) { return a.second < b.second; });
    if (sources.empty()) throw std::invalid_argument("the corpus has no files");
    std::map<std::string, std::vector<uint8_t>> files;
    std::vector<std::pair<std::string, uint64_t>> sizes;
    for (const auto& [from, to] : sources)
    {
        files[to] = read_file_bytes(from);
        sizes.push_back({to, files[to].size()});
    }
    const std::vector<training::Window> order = training::make_order(sizes, o.window, o.epochs, o.seed);
    if (order.empty()) throw std::invalid_argument("the corpus has nothing to learn from: no file of two bytes or more");
    files["training.ini"] = [&] { const std::string t = o.recipe.text(); return std::vector<uint8_t>(t.begin(), t.end()); }();
    files["order.txt"] = [&] { const std::string t = training::order_text(order); return std::vector<uint8_t>(t.begin(), t.end()); }();
    for (const auto& [path, bytes] : files)
    {
        const fs::path dest = out / path_of(path);
        fs::create_directories(dest.parent_path());
        write_text(dest, text_of(bytes));
    }
    const std::u8string n = fs::absolute(out).lexically_normal().filename().u8string();
    return run_of_files(std::string(n.begin(), n.end()), std::move(files));
}

void pack_training_run(const fs::path& folder, const fs::path& sieve_file)
{
    Manifest m = walk_folder(folder);
    add_packed(m, folder);
    write_address_file(sieve_file, binary_address(m.file()), false);
}

} // namespace sieve::cli
