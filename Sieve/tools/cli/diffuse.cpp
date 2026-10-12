// Sieve CLI — `sieve diffuse`: train a diffusion-v1 model on pictures of the image line, check a run
// again, and make pictures with a model (sieve/diffusion.hpp; IDEAS §19.2).
//
// A run is a folder:
//   diffusion.ini   the recipe (below)
//   pictures.txt    the training pictures, one a line, each cell a digit in base 36 (a palette of
//                   up to 36 colours), row after row
//   result.txt      what training gave: a checkpoint a line, then the final model's address
// so that anyone can train it again and get the same model, digit for digit.
#include "cli/args.hpp"
#include "cli/filter_config.hpp"
#include "cli/lines.hpp"
#include "cli/train.hpp"
#include "sieve/diffusion.hpp"
#include "sieve/image.hpp"
#include "sieve/sha256.hpp"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
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

// The recipe: diffusion.ini.
struct DiffusionRecipe
{
    training::Recipe train; // shape, learning rate, momentum, version and loss scale
    diffusion::Setup setup;
    std::string palette = "mono";
    uint32_t batch = 8, steps = 4000, checkpoint_every = 500;
    uint64_t seed = 1, start_seed = 7;
    std::string pictures_sha256;

    std::string text() const
    {
        std::ostringstream t;
        t << "; sieve diffusion run: " << diffusion::kDiffusionVersion << " on "
          << (train.version == 1 ? training::kTrainingVersion : training::kTrainingVersion2) << "\n"
          << "version = " << train.version << "\n"
          << "loss_scale_bits = " << train.loss_scale_bits << "\n"
          << "layers = " << train.shape.layers << "\nwidth = " << train.shape.width << "\nheads = " << train.shape.heads
          << "\nbits = " << train.shape.bits << "\n"
          << "picture = " << setup.width << "x" << setup.height << "\npalette = " << palette << "\nlevels = " << setup.levels << "\n"
          << "learning_rate = " << train.learning_rate << "\nmomentum = " << train.momentum << "\n"
          << "batch = " << batch << "\nsteps = " << steps << "\ncheckpoint_every = " << checkpoint_every << "\n"
          << "seed = " << seed << "\nstart_seed = " << start_seed << "\n"
          << "pictures_sha256 = " << pictures_sha256 << "\n";
        return t.str();
    }

    static DiffusionRecipe parse(const std::string& text)
    {
        DiffusionRecipe r;
        std::istringstream in(text);
        std::string line;
        auto num = [](const std::string& v, const std::string& k) {
            try
            {
                size_t used = 0;
                const unsigned long long n = std::stoull(v, &used);
                if (used != v.size()) throw std::invalid_argument("");
                return uint64_t(n);
            }
            catch (const std::exception&)
            {
                bad("diffusion.ini: " + k + " must be a whole number");
            }
        };
        while (std::getline(in, line))
        {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (line.empty() || line[0] == ';' || line[0] == '#') continue;
            const size_t eq = line.find('=');
            if (eq == std::string::npos) bad("diffusion.ini: a line without '='");
            auto trim = [](std::string s) {
                while (!s.empty() && s.back() == ' ') s.pop_back();
                while (!s.empty() && s.front() == ' ') s.erase(s.begin());
                return s;
            };
            const std::string k = trim(line.substr(0, eq)), v = trim(line.substr(eq + 1));
            if (k == "version") r.train.version = int(num(v, k));
            else if (k == "loss_scale_bits") r.train.loss_scale_bits = uint32_t(num(v, k));
            else if (k == "layers") r.train.shape.layers = uint32_t(num(v, k));
            else if (k == "width") r.train.shape.width = uint32_t(num(v, k));
            else if (k == "heads") r.train.shape.heads = uint32_t(num(v, k));
            else if (k == "bits") r.train.shape.bits = uint32_t(num(v, k));
            else if (k == "picture")
            {
                const size_t x = v.find('x');
                if (x == std::string::npos) bad("diffusion.ini: picture is WxH");
                r.setup.width = uint32_t(num(v.substr(0, x), k));
                r.setup.height = uint32_t(num(v.substr(x + 1), k));
            }
            else if (k == "palette") r.palette = v;
            else if (k == "levels") r.setup.levels = uint32_t(num(v, k));
            else if (k == "learning_rate") r.train.learning_rate = v;
            else if (k == "momentum") r.train.momentum = v;
            else if (k == "batch") r.batch = uint32_t(num(v, k));
            else if (k == "steps") r.steps = uint32_t(num(v, k));
            else if (k == "checkpoint_every") r.checkpoint_every = uint32_t(num(v, k));
            else if (k == "seed") r.seed = num(v, k);
            else if (k == "start_seed") r.start_seed = num(v, k);
            else if (k == "pictures_sha256") r.pictures_sha256 = v;
            else bad("diffusion.ini: unknown key " + k);
        }
        r.setup.colours = palette_by_id(r.palette).size();
        r.check();
        return r;
    }

    void check() const
    {
        train.shape.check();
        setup.check();
        if (palette_by_id(palette).size() > 36) bad("a palette of at most 36 colours (pictures.txt writes a cell as one digit)");
        if (batch < 1 || batch > 32) bad("from 1 to 32 examples a step");
        if (checkpoint_every < 1) bad("a checkpoint at least every step");
        training::Recipe t = train;
        t.batch = batch;
        t.check();
    }
};

std::string pictures_text(const std::vector<std::vector<uint32_t>>& pics)
{
    std::string t;
    for (const auto& p : pics)
    {
        for (const uint32_t c : p) t.push_back(kDigits[c]);
        t.push_back('\n');
    }
    return t;
}

std::vector<std::vector<uint32_t>> parse_pictures(const std::string& text, const diffusion::Setup& s)
{
    std::vector<std::vector<uint32_t>> out;
    std::istringstream in(text);
    std::string line;
    while (std::getline(in, line))
    {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue;
        if (line.size() != s.cells()) bad("pictures.txt: a picture of " + std::to_string(s.cells()) + " cells");
        std::vector<uint32_t> p;
        for (const char ch : line)
        {
            const char* at = std::char_traits<char>::find(kDigits, s.colours, ch);
            if (!at) bad("pictures.txt: a cell beyond the palette");
            p.push_back(uint32_t(at - kDigits));
        }
        out.push_back(std::move(p));
    }
    if (out.empty()) bad("pictures.txt: no pictures");
    return out;
}

AiSpace::Digits start_digits(const AiSpace& space, uint64_t seed)
{
    diffusion::SplitMix g(seed);
    AiSpace::Digits d(space.weights());
    const uint64_t n = uint64_t(1) << space.shape().bits;
    for (auto& x : d) x = uint8_t(g.next() % n);
    return d;
}

std::string digest(const AiSpace::Digits& d) { return Sha256::hex(Sha256::hash(std::string_view(reinterpret_cast<const char*>(d.data()), d.size()))); }

// Trains the run and returns result.txt.
std::string train_run(const DiffusionRecipe& r, const std::vector<std::vector<uint32_t>>& pics, unsigned threads, bool quiet,
                      AiSpace::Digits* final_digits)
{
    const AiSpace space(r.train.shape, "sieve");
    training::Trainer tr(space, start_digits(space, r.start_seed), r.train, threads);
    diffusion::SplitMix g(r.seed);
    std::ostringstream res;
    res << "sieve-diffusion-result-v1\nversion " << diffusion::kDiffusionVersion << " "
        << (r.train.version == 1 ? training::kTrainingVersion : training::kTrainingVersion2) << "\nsteps " << r.steps << "\n";
    res << "checkpoint 0 0 0 " << digest(tr.digits()) << "\n";
    training::Score since;
    const auto t0 = std::chrono::steady_clock::now();
    for (uint32_t k = 1; k <= r.steps; ++k)
    {
        const training::Score s = diffusion::step(tr, r.setup, pics, g, r.batch);
        since.prob_sum += s.prob_sum, since.predictions += s.predictions, since.bits += s.bits;
        if (k % r.checkpoint_every == 0 || k == r.steps)
        {
            res << "checkpoint " << k << " " << since.prob_sum << " " << since.predictions << " " << digest(tr.digits()) << "\n";
            if (!quiet)
                std::cout << "step " << k << " of " << r.steps << ": " << since.bits / double(std::max<uint64_t>(since.predictions, 1))
                          << " bits a cell, " << std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count() << " s"
                          << std::endl;
            since = {};
        }
    }
    res << "final " << space.hex_of(space.index_of(tr.digits(), AddressMode::Positional)) << "\n";
    if (final_digits) *final_digits = tr.digits();
    return res.str();
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
        ImageFormat fmt;
        fmt.width = r.setup.width, fmt.height = r.setup.height, fmt.palette = &palette_by_id(r.palette);
        const Space line(fmt.symbols_id(), fmt.palette->size(), fmt.unit_length());
        const uint32_t count = a.get_positive("count", 1);
        const uint64_t seed = a.has("seed") ? std::stoull(a.get("seed")) : 1;
        const uint32_t background = fmt.palette->nearest(Rgb{255, 255, 255});
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
            std::cout << "noise    " << line.address_of(noise, AddressMode::Positional) << "  (positional, the image line)\n";
            for (size_t l = 0; l < levels.size(); ++l)
            {
                std::cout << "level " << (r.setup.levels - l - 1) << "\n";
                show_picture(levels[l], r.setup, background);
            }
            std::cout << "picture  " << line.address_of(pic, AddressMode::Positional) << "  (seed " << seed + i << ")\n";
            show_picture(pic, r.setup, background);
            std::cout << "\n";
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
        // The pictures: a file of them, or survivors of the image line's stack drawn at random.
        Args la = a;
        la.opts["line"] = "image";
        const Line line = make_line(la);
        r.palette = line.image.palette->id();
        r.setup.width = line.image.width, r.setup.height = line.image.height, r.setup.colours = line.image.palette->size();
        std::vector<std::vector<uint32_t>> pics;
        if (a.has("pictures")) pics = parse_pictures(read_text(fs::path(a.get("pictures"))), r.setup);
        else
        {
            const fs::path cfg_path = a.has("filters") ? fs::path(a.get("filters")) : FilterConfig::default_path();
            const FilterConfig cfg = FilterConfig::load(cfg_path);
            const FilterStack stack = build_stack(line, cfg.of(LineKind::Image));
            if (stack.empty()) bad("the image line's stack in " + cfg_path.string() + " is empty: tick its filters (figure-v1, say) or give --pictures FILE");
            const Ranker* rk = stack.ranker();
            if (!rk) bad("the image line's stack cannot be counted, so its survivors cannot be drawn at random");
            const uint32_t n = a.get_positive("figures", 4000);
            diffusion::SplitMix g(a.has("figure-seed") ? std::stoull(a.get("figure-seed")) : 42);
            for (uint32_t i = 0; i < n; ++i)
            {
                // Two draws make a 128-bit number, taken modulo the survivors.
                BigUint k = BigUint(g.next());
                k.mul_small(uint32_t(1) << 16), k.mul_small(uint32_t(1) << 16), k.mul_small(uint32_t(1) << 16), k.mul_small(uint32_t(1) << 16);
                k += BigUint(g.next());
                BigUint q, rest;
                BigUint::divmod(k, rk->count(), q, rest);
                pics.push_back(rk->unrank(rest));
            }
            if (!quiet) std::cout << n << " training pictures drawn from the stack " << stack.id() << "\n";
        }
        const std::string pt = pictures_text(pics);
        r.pictures_sha256 = Sha256::hex(Sha256::hash(pt));
        r.check();
        fs::create_directories(dir);
        write_text(dir / "diffusion.ini", r.text());
        write_text(dir / "pictures.txt", pt);
        const std::string res = train_run(r, pics, threads, quiet, nullptr);
        write_text(dir / "result.txt", res);
        if (!quiet) std::cout << "trained: " << (dir / "result.txt").string() << "\n";
        return 0;
    }
    // Checking a run again.
    if (a.positional.empty()) bad("sieve diffuse --record DIR ... | sieve diffuse DIR | sieve diffuse --make DIR");
    const fs::path dir(a.positional[0]);
    const DiffusionRecipe r = DiffusionRecipe::parse(read_text(dir / "diffusion.ini"));
    const std::string pt = read_text(dir / "pictures.txt");
    if (Sha256::hex(Sha256::hash(pt)) != r.pictures_sha256) bad("pictures.txt is not the file the recipe names (its SHA-256 differs)");
    const auto pics = parse_pictures(pt, r.setup);
    const std::string res = train_run(r, pics, threads, quiet, nullptr);
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
