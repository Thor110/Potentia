// Sieve CLI — `sieve diffuse`: train a diffusion-v1 model on pictures of the image line, check a run
// again, and make pictures with a model (sieve/diffusion.hpp; IDEAS §19.2). The run folder and its
// training are in cli/diffusion_run.hpp, shared with the hallway's AI Training Harness.
#include "cli/args.hpp"
#include "cli/diffusion_run.hpp"
#include "cli/filter_config.hpp"
#include "cli/lines.hpp"
#include "cli/train.hpp"
#include "sieve/audio.hpp"
#include "sieve/diffusion.hpp"
#include "sieve/image.hpp"
#include "sieve/sha256.hpp"

#include <algorithm>
#include <array>
#include <optional>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace fs = std::filesystem;

namespace sieve::cli {

namespace {

[[noreturn]] void bad(const std::string& why) { throw std::invalid_argument(why); }

std::string read_text(const fs::path& p)
{
    std::ifstream in(p, std::ios::binary);
    if (!in) bad("cannot read " + p.string());
    std::ostringstream s;
    s << in.rdbuf();
    return s.str();
}

void write_text(const fs::path& p, const std::string& t)
{
    std::ofstream out(p, std::ios::binary);
    if (!out || !out.write(t.data(), std::streamsize(t.size()))) bad("cannot write " + p.string());
}

const char kDigits[] = "0123456789abcdefghijklmnopqrstuvwxyz";

// Trains the run, saying how it goes at each checkpoint, and returns result.txt.
std::string train_run(const DiffusionRun& run, unsigned threads, bool quiet)
{
    const auto t0 = std::chrono::steady_clock::now();
    training::Score since;
    const uint32_t every = run.recipe.checkpoint_every;
    const DiffusionOutcome out = train_diffusion_run(run, threads, [&](uint64_t k, uint64_t n, const training::Score& s) {
        since.predictions += s.predictions, since.bits += s.bits;
        if (k % every == 0 || k == n)
        {
            if (!quiet)
                std::cout << "step " << k << " of " << n << ": " << since.bits / double(std::max<uint64_t>(since.predictions, 1))
                          << " bits a cell, " << std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count() << " s"
                          << std::endl;
            since = {};
        }
    });
    return out.text;
}

void show_picture(const std::vector<uint32_t>& p, const diffusion::Setup& s, uint32_t background)
{
    for (uint32_t y = 0; y < s.height; ++y)
    {
        std::string row;
        for (uint32_t x = 0; x < s.width; ++x)
        {
            const uint32_t c = p[y * s.width + x];
            row += c == background ? ". " : s.colours <= 2 ? "##" : std::string(2, kDigits[c]);
        }
        std::cout << "  " << row << "\n";
    }
}

// The line a run's units are of, built as the tools build it (for its filters).
Line line_of_recipe(const DiffusionRecipe& r)
{
    Args la;
    if (r.audio())
    {
        const NoteSet set = note_set_of(r.symbols);
        la.opts["line"] = "audio";
        la.opts["length"] = std::to_string(r.setup.width / set.voices);
        if (!set.legacy)
        {
            la.opts["note-set"] = "notes2";
            la.opts["low"] = note_name(set.low), la.opts["high"] = note_name(set.high);
            la.opts["durations"] = set.durations, la.opts["voices"] = std::to_string(set.voices);
        }
    }
    else
    {
        la.opts["line"] = "image";
        la.opts["image-width"] = std::to_string(r.setup.width), la.opts["image-height"] = std::to_string(r.setup.height);
        la.opts["image-palette"] = r.palette;
    }
    return make_line(la);
}

} // namespace

int cmd_diffuse(const Args& a)
{
    const unsigned threads = a.has("threads") ? a.get_positive("threads", 1) : 0;
    const bool quiet = a.has("quiet");
    // Making pictures with a run's model.
    if (a.has("make"))
    {
        const fs::path dir(a.get("make"));
        const DiffusionRecipe r = DiffusionRecipe::parse(read_text(dir / "diffusion.ini"));
        const AiSpace space(r.train.shape, "sieve");
        std::string final_hex;
        {
            std::istringstream in(read_text(dir / "result.txt"));
            std::string w;
            while (in >> w)
                if (w == "final") in >> final_hex;
        }
        if (final_hex.empty()) bad("the run has not been trained (no final model in result.txt)");
        const AiSpace::Digits model = space.digits_at(BigUint::from_hex(final_hex), AddressMode::Positional);
        const Space line = unit_space_of(r);
        const uint32_t count = a.get_positive("count", 1);
        const uint64_t seed = a.has("seed") ? std::stoull(a.get("seed")) : 1;
        const uint32_t background = r.audio() ? 0 : palette_by_id(r.palette).nearest(Rgb{255, 255, 255});
        // A melody is shown as its notation; a picture as its cells.
        auto show = [&](const std::vector<uint32_t>& u) {
            if (r.audio()) std::cout << "  " << notes_to_notation(note_set_of(r.symbols), u) << "\n";
            else show_picture(u, r.setup, background);
        };
        // --judge FILTERS: how many of the units made, and of the noises they were made from, each
        // filter the file ticks on the line keeps, and the whole stack (instead of each unit).
        std::optional<Line> judge_line;
        std::vector<std::pair<std::string, FilterStack>> judges;
        std::vector<std::array<uint32_t, 2>> kept; // made, noise
        if (a.has("judge"))
        {
            judge_line = line_of_recipe(r);
            const LineFilters lf = FilterConfig::load(fs::path(a.get("judge"))).of(judge_line->kind);
            if (lf.enabled.empty()) bad("--judge: the file ticks no filters on the " + std::string(r.audio() ? "audio" : "image") + " line");
            for (const std::string& name : lf.enabled)
            {
                LineFilters one = lf;
                one.enabled = {name};
                judges.emplace_back(name, build_stack(*judge_line, one));
            }
            judges.emplace_back("the stack", build_stack(*judge_line, lf));
            kept.assign(judges.size(), {0, 0});
        }
        diffusion::SplitMix noise_g(a.has("noise-seed") ? std::stoull(a.get("noise-seed")) : 1);
        for (uint32_t i = 0; i < count; ++i)
        {
            std::vector<uint32_t> noise;
            if (a.has("noise") && i == 0)
            {
                noise = line.unit_at(a.get("noise"), AddressMode::Positional);
            }
            else
            {
                noise.resize(r.setup.cells());
                for (auto& c : noise) c = uint32_t(noise_g.next() % r.setup.colours);
            }
            std::vector<std::vector<uint32_t>> levels;
            const std::vector<uint32_t> pic = diffusion::make_picture(space, model, r.setup, noise, seed + i, a.has("show-levels") ? &levels : nullptr);
            if (judge_line)
            {
                for (size_t j = 0; j < judges.size(); ++j)
                {
                    kept[j][0] += judges[j].second.passes(pic) ? 1 : 0;
                    kept[j][1] += judges[j].second.passes(noise) ? 1 : 0;
                }
                continue;
            }
            std::cout << "noise    " << line.address_of(noise, AddressMode::Positional) << (r.audio() ? "  (positional, the audio line)\n" : "  (positional, the image line)\n");
            for (size_t l = 0; l < levels.size(); ++l)
            {
                std::cout << "level " << (r.setup.levels - l - 1) << "\n";
                show(levels[l]);
            }
            std::cout << (r.audio() ? "melody   " : "picture  ") << line.address_of(pic, AddressMode::Positional) << "  (seed " << seed + i << ")\n";
            show(pic);
            std::cout << "\n";
        }
        if (judge_line)
        {
            std::cout << "judged " << count << (r.audio() ? " melodies" : " pictures") << " made from noise, and the noises themselves:\n";
            for (size_t j = 0; j < judges.size(); ++j)
                std::cout << "  " << judges[j].first << std::string(std::max<size_t>(1, 22 - judges[j].first.size()), ' ') << "made " << kept[j][0] << " of " << count
                          << ", noise " << kept[j][1] << " of " << count << "\n";
        }
        return 0;
    }
    // Recording a run: the recipe and the pictures, then training it.
    if (a.has("record"))
    {
        const fs::path dir(a.get("record"));
        if (dir.empty()) bad("--record DIR: the run's folder, to make");
        DiffusionRecipe r;
        r.train.version = 2;
        r.train.loss_scale_bits = a.get_u32("loss-scale-bits", 4);
        r.train.shape.layers = a.get_positive("ai-layers", 1);
        r.train.shape.width = a.get_positive("ai-width", 16);
        r.train.shape.heads = a.get_positive("ai-heads", 2);
        r.train.shape.bits = a.get_positive("ai-bits", 8);
        r.train.learning_rate = a.get("learning-rate", "0.0001");
        r.train.momentum = a.get("momentum", "0.9");
        r.batch = a.get_positive("batch", 8);
        r.steps = a.get_positive("steps", 4000);
        r.checkpoint_every = a.get_positive("checkpoint-every", std::max<uint32_t>(1, r.steps / 10));
        r.seed = a.has("seed") ? std::stoull(a.get("seed")) : 1;
        r.start_seed = a.has("start-seed") ? std::stoull(a.get("start-seed")) : 7;
        r.setup.levels = a.get_positive("levels", 16);
        // The units: pictures of the image line (the default), or melodies of the audio line
        // (--line audio, with its note set's options); a file of them, or survivors of the line's
        // stack drawn at random.
        Args la = a;
        la.opts["line"] = a.get("line", "image");
        la.opts.erase("levels"); // diffusion's levels of noise, not notes3's levels of loudness
        const Line line = make_line(la);
        if (line.kind == LineKind::Audio)
        {
            if (!is_note_symbols(line.space.symbols_id())) bad("a melody model is of notes104 or notes2 (--note-set)");
            r.line = "audio";
            r.symbols = line.space.symbols_id();
            r.setup.width = line.space.unit_length(), r.setup.height = 1, r.setup.colours = line.space.base();
        }
        else if (line.kind == LineKind::Image)
        {
            r.palette = line.image.palette->id();
            r.setup.width = line.image.width, r.setup.height = line.image.height, r.setup.colours = line.image.palette->size();
        }
        else bad("a diffusion run is of the image line (pictures) or the audio line (melodies)");
        std::vector<std::vector<uint32_t>> pics;
        if (a.has("pictures")) pics = parse_pictures(read_text(fs::path(a.get("pictures"))), r.setup);
        else
        {
            const fs::path cfg_path = a.has("filters") ? fs::path(a.get("filters")) : FilterConfig::default_path();
            const FilterConfig cfg = FilterConfig::load(cfg_path);
            const FilterStack stack = build_stack(line, cfg.of(line.kind));
            const std::string which = r.audio() ? "audio" : "image";
            if (stack.empty()) bad("the " + which + " line's stack in " + cfg_path.string() + " is empty: tick its filters (figure-v1 or key-data-v2, say) or give --pictures FILE");
            const Ranker* rk = stack.ranker();
            if (!rk) bad("the " + which + " line's stack cannot be counted, so its survivors cannot be drawn at random");
            const uint32_t n = a.get_positive("figures", 4000);
            pics = draw_pictures(*rk, n, a.has("figure-seed") ? std::stoull(a.get("figure-seed")) : 42);
            if (!quiet) std::cout << n << (r.audio() ? " training melodies" : " training pictures") << " drawn from the stack " << stack.id() << "\n";
        }
        const DiffusionRun run = record_diffusion_run(dir, r, std::move(pics));
        const std::string res = train_run(run, threads, quiet);
        write_text(dir / "result.txt", res);
        if (!quiet) std::cout << "trained: " << (dir / "result.txt").string() << "\n";
        return 0;
    }
    // Checking a run again.
    if (a.positional.empty()) bad("sieve diffuse --record DIR ... | sieve diffuse DIR | sieve diffuse --make DIR");
    const fs::path dir(a.positional[0]);
    const DiffusionRun run = load_diffusion_run(dir);
    const std::string res = train_run(run, threads, quiet);
    const fs::path rp = dir / "result.txt";
    if (!fs::exists(rp))
    {
        write_text(rp, res);
        std::cout << "trained: " << rp.string() << "\n";
        return 0;
    }
    std::string want = read_text(rp);
    want.erase(std::remove(want.begin(), want.end(), '\r'), want.end());
    if (want == res)
    {
        std::cout << "reproduced exactly: every checkpoint and the final model are the run's result.txt, byte for byte\n";
        return 0;
    }
    std::cout << "NOT reproduced: training again gave a different result.txt\n";
    return 1;
}

} // namespace sieve::cli
