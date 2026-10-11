// Sieve CLI — `sieve ai-train` (see train.hpp).
#include "cli/ai.hpp"
#include "cli/locate.hpp"
#include "cli/train.hpp"
#include "sieve/llm.hpp"
#include "sieve/llm_tokenizer.hpp"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace fs = std::filesystem;

namespace sieve::cli {

namespace {

fs::path path_of(const std::string& s) { return fs::path(std::u8string(s.begin(), s.end())); }

void write_text(const fs::path& p, const std::string& t)
{
    std::ofstream out(p, std::ios::binary);
    if (!out || !out.write(t.data(), std::streamsize(t.size()))) throw std::invalid_argument("cannot write " + p.string());
}

// Bytes as text: printable ASCII and whole UTF-8 characters as they are, a newline as \n, any
// other byte a middle dot.
std::string shown(const std::string& bytes)
{
    std::string out;
    for (size_t i = 0; i < bytes.size();)
    {
        const uint8_t c = uint8_t(bytes[i]);
        const size_t n = c >= 0xC2 && c <= 0xDF ? 2 : c >= 0xE0 && c <= 0xEF ? 3 : c >= 0xF0 && c <= 0xF4 ? 4 : 1;
        bool whole = n > 1 && i + n <= bytes.size();
        for (size_t k = 1; whole && k < n; ++k) whole = (uint8_t(bytes[i + k]) & 0xC0) == 0x80;
        if (whole) out += bytes.substr(i, n), i += n;
        else
        {
            if (c == '\n') out += "\\n";
            else if (c >= 0x20 && c < 0x7F) out.push_back(char(c));
            else out += "\xC2\xB7";
            ++i;
        }
    }
    return out;
}

} // namespace

// ---------------------------------------------------------------- sieve ai-train

int cmd_ai_train(const Args& a)
{
    const unsigned threads = a.has("threads") ? a.get_positive("threads", 1) : 0;
    TrainingRun run;
    fs::path folder; // where result.txt goes, when the run is a folder
    if (a.has("record"))
    {
        if (!a.has("out")) throw std::invalid_argument("--record needs --out FOLDER, the new run's folder");
        RecordOptions o;
        o.recipe.shape = ai_shape_of(a);
        if (a.has("key")) o.recipe.key = a.get("key");
        if (a.has("start")) o.recipe.start = a.get("start");
        if (a.has("mode")) o.recipe.start_mode = address_mode_from_string(a.get("mode"));
        if (a.has("learning-rate")) o.recipe.learning_rate = a.get("learning-rate");
        if (a.has("momentum")) o.recipe.momentum = a.get("momentum");
        if (a.has("batch")) o.recipe.batch = a.get_positive("batch", 4);
        if (a.has("checkpoint-every")) o.recipe.checkpoint_every = a.get_positive("checkpoint-every", 100);
        if (a.has("window")) o.recipe.window = a.get_positive("window", 65);
        if (a.has("epochs")) o.recipe.epochs = a.get_positive("epochs", 30);
        if (a.has("seed")) o.recipe.seed = a.get_u32("seed", 1);
        if (a.has("reading")) o.recipe.reading = training::reading_from_string(a.get("reading"));
        o.recipe.checkpoint_addresses = a.has("checkpoint-addresses");
        o.write_order = a.has("write-order");
        folder = path_of(a.get("out"));
        std::vector<TrainingFile> files = corpus_files(path_of(a.get("record")));
        if (a.has("file-order"))
        {
            // The files named first, in the order named (a line each, as the run names them,
            // without corpus/), then the rest in path order.
            std::ifstream in(path_of(a.get("file-order")), std::ios::binary);
            if (!in) throw std::invalid_argument("cannot open " + a.get("file-order"));
            std::vector<TrainingFile> named;
            std::string line;
            while (std::getline(in, line))
            {
                while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) line.pop_back();
                if (line.empty()) continue;
                const auto it = std::find_if(files.begin(), files.end(), [&](const TrainingFile& f) { return f.path == "corpus/" + line; });
                if (it == files.end()) throw std::invalid_argument("--file-order names " + line + ", which is not in the corpus");
                named.push_back(*it);
                files.erase(it);
            }
            named.insert(named.end(), files.begin(), files.end());
            files = std::move(named);
        }
        run = record_training_run(files, folder, o);
        std::cout << "recorded " << run.name << ": " << run.files.size() - 2 << " corpus file" << (run.files.size() == 3 ? "" : "s") << ", "
                  << run.order.size() << " windows, " << run.steps() << " steps\n";
    }
    else
    {
        if (a.positional.empty()) throw std::invalid_argument("name a run: a folder or a .sieve (or --record CORPUS --out FOLDER)");
        const fs::path p = path_of(a.positional[0]);
        run = load_training_run(p);
        if (fs::is_directory(p)) folder = p;
        std::cout << "run " << run.name << ": " << run.order.size() << " windows, " << run.steps() << " steps"
                  << (run.result ? ", trained before: training it again to check" : ", not trained yet") << "\n";
    }
    const AiShape& s = run.recipe.shape;
    std::cout << "model: " << s.layers << (s.layers == 1 ? " layer" : " layers") << ", width " << s.width << ", " << s.heads << " heads, "
              << s.bits << " bits a weight; learning rate " << run.recipe.learning_rate << ", momentum " << run.recipe.momentum << ", "
              << run.recipe.batch << " windows a step\n";

    const auto t0 = std::chrono::steady_clock::now();
    const bool quiet = a.has("quiet");
    const TrainingOutcome out = train_run(run, threads, [&](uint64_t k, uint64_t n, const training::Score& recent) {
        if (!quiet && (k % run.recipe.checkpoint_every == 0 || k == n))
        {
            char line[160];
            std::snprintf(line, sizeof line, "step %llu of %llu: %.3f bits a byte, the right byte given %.1f%%\n", (unsigned long long)k,
                          (unsigned long long)n, recent.bits_per_byte(), 100.0 * recent.mean_probability());
            std::cout << line << std::flush;
        }
    });
    const double secs = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    char took[64];
    std::snprintf(took, sizeof took, "%.1f s", secs);
    std::cout << "trained in " << took << "; model " << out.result.model_sha256 << "\n";

    const AiSpace space(run.recipe.shape, run.recipe.key);
    {
        llm::Model model(space.config(), space.tensors_of(out.digits), 1);
        const llm::Tokenizer tok(json::parse(AiSpace::tokenizer_json()));
        llm::Sampling sm;
        sm.temperature = 0.5f;
        sm.top_p = 1.0f;
        sm.seed = 1;
        llm::Sampler sampler(sm);
        std::cout << "it says: " << shown(llm::continue_text(model, tok, a.has("prompt") ? a.get("prompt") : std::string("\n"), sampler, 80)) << "\n";
    }
    if (a.has("address-out"))
    {
        write_text(path_of(a.get("address-out")), out.result.final_address + "\n");
        std::cout << "its address (positional) written to " << a.get("address-out") << "\n";
    }
    if (a.has("model-out"))
    {
        const fs::path m = path_of(a.get("model-out"));
        fs::create_directories(m);
        write_text(m / "config.json", json::write(space.config()) + "\n");
        write_text(m / "model.safetensors", space.safetensors_of(out.digits));
        write_text(m / "tokenizer.json", AiSpace::tokenizer_json());
        std::cout << "its files written to " << a.get("model-out") << " (sieve chat --model " << a.get("model-out") << " --raw)\n";
    }
    if (a.has("result-out")) write_text(path_of(a.get("result-out")), out.text);

    int code = 0;
    if (run.result)
    {
        if (const auto diff = result_difference(*run.result, out.text))
        {
            std::cout << "NOT reproduced: the result differs from the run's result.txt at " << *diff << "\n";
            code = 1;
        }
        else std::cout << "reproduced exactly: every checkpoint and the final model are the run's result.txt, byte for byte\n";
    }
    else if (!folder.empty())
    {
        write_text(folder / "result.txt", out.text);
        std::cout << "result written to " << (folder / "result.txt").string() << "\n";
    }
    if (a.has("sieve"))
    {
        if (folder.empty()) throw std::invalid_argument("--sieve packs a run folder");
        pack_training_run(folder, path_of(a.get("sieve")));
        std::cout << "packed as " << a.get("sieve") << " (" << fs::file_size(path_of(a.get("sieve"))) << " bytes)\n";
    }
    return code;
}

} // namespace sieve::cli
