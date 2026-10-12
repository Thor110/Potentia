// Sieve CLI — diffusion runs (see diffusion_run.hpp).
#include "cli/diffusion_run.hpp"

#include "sieve/audio.hpp"
#include "sieve/image.hpp"
#include "sieve/sha256.hpp"

#include <algorithm>
#include <fstream>
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

AiSpace::Digits start_digits(const AiSpace& space, uint64_t seed)
{
    diffusion::SplitMix g(seed);
    AiSpace::Digits d(space.weights());
    const uint64_t n = uint64_t(1) << space.shape().bits;
    for (auto& x : d) x = uint8_t(g.next() % n);
    return d;
}

std::string digest(const AiSpace::Digits& d) { return Sha256::hex(Sha256::hash(std::string_view(reinterpret_cast<const char*>(d.data()), d.size()))); }

} // namespace

std::string DiffusionRecipe::text() const
{
    std::ostringstream t;
    t << "; sieve diffusion run: " << diffusion::kDiffusionVersion << " on "
      << (train.version == 1 ? training::kTrainingVersion : training::kTrainingVersion2) << "\n"
      << "version = " << train.version << "\n"
      << "loss_scale_bits = " << train.loss_scale_bits << "\n"
      << "layers = " << train.shape.layers << "\nwidth = " << train.shape.width << "\nheads = " << train.shape.heads
      << "\nbits = " << train.shape.bits << "\n"
      << (audio() ? "line = audio\nsymbols = " + symbols + "\nlength = " + std::to_string(setup.width) + "\n"
                  : "picture = " + std::to_string(setup.width) + "x" + std::to_string(setup.height) + "\npalette = " + palette + "\n")
      << "levels = " << setup.levels << "\n"
      << "learning_rate = " << train.learning_rate << "\nmomentum = " << train.momentum << "\n"
      << "batch = " << batch << "\nsteps = " << steps << "\ncheckpoint_every = " << checkpoint_every << "\n"
      << "seed = " << seed << "\nstart_seed = " << start_seed << "\n"
      << "pictures_sha256 = " << pictures_sha256 << "\n";
    return t.str();
}

DiffusionRecipe DiffusionRecipe::parse(const std::string& text)
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
        else if (k == "line")
        {
            if (v != "image" && v != "audio") bad("diffusion.ini: line is image or audio");
            r.line = v;
        }
        else if (k == "symbols") r.symbols = v;
        else if (k == "length") r.setup.width = uint32_t(num(v, k)), r.setup.height = 1;
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
    r.setup.colours = r.audio() ? note_set_of(r.symbols).base() : palette_by_id(r.palette).size();
    r.check();
    return r;
}

void DiffusionRecipe::check() const
{
    train.shape.check();
    setup.check();
    if (audio())
    {
        if (!is_note_symbols(symbols)) bad("an audio run's symbols are a note set: notes104 or notes2 (" + symbols + ")");
        if (setup.height != 1 || setup.colours != note_set_of(symbols).base()) bad("a melody is one row of its note set's symbols");
    }
    else if (setup.colours != palette_by_id(palette).size()) bad("a picture's colours are its palette's");
    if (batch < 1 || batch > 32) bad("from 1 to 32 examples a step");
    if (checkpoint_every < 1) bad("a checkpoint at least every step");
    training::Recipe t = train;
    t.batch = batch;
    t.check();
}

std::string pictures_text(const std::vector<std::vector<uint32_t>>& pics, const diffusion::Setup& s)
{
    std::string t;
    for (const auto& p : pics)
    {
        for (size_t i = 0; i < p.size(); ++i)
        {
            if (s.colours <= 36) t.push_back(kDigits[p[i]]);
            else t += (i ? " " : "") + std::to_string(p[i]);
        }
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
        std::vector<uint32_t> p;
        if (s.colours <= 36)
            for (const char ch : line)
            {
                const char* at = std::char_traits<char>::find(kDigits, s.colours, ch);
                if (!at) bad("pictures.txt: a cell beyond the palette");
                p.push_back(uint32_t(at - kDigits));
            }
        else
        {
            std::istringstream cells(line);
            std::string c;
            while (std::getline(cells, c, ' '))
            {
                if (c.empty() || c.size() > 3 || c.find_first_not_of("0123456789") != std::string::npos || std::stoul(c) >= s.colours)
                    bad("pictures.txt: a cell that is not a symbol of the line");
                p.push_back(uint32_t(std::stoul(c)));
            }
        }
        if (p.size() != s.cells()) bad("pictures.txt: a unit of " + std::to_string(s.cells()) + " cells");
        out.push_back(std::move(p));
    }
    if (out.empty()) bad("pictures.txt: no pictures");
    return out;
}


bool is_diffusion_run(const fs::path& dir) { return fs::is_regular_file(dir / "diffusion.ini"); }

DiffusionRun load_diffusion_run(const fs::path& dir)
{
    if (!is_diffusion_run(dir)) bad(dir.string() + " is not a diffusion run (it has no diffusion.ini)");
    DiffusionRun run;
    run.path = dir;
    const std::u8string n = dir.filename().u8string();
    run.name = std::string(n.begin(), n.end());
    run.recipe = DiffusionRecipe::parse(read_text(dir / "diffusion.ini"));
    const std::string pt = read_text(dir / "pictures.txt");
    if (Sha256::hex(Sha256::hash(pt)) != run.recipe.pictures_sha256) bad("pictures.txt is not the file the recipe names (its SHA-256 differs)");
    run.pictures = parse_pictures(pt, run.recipe.setup);
    if (fs::exists(dir / "result.txt"))
    {
        std::string r = read_text(dir / "result.txt");
        r.erase(std::remove(r.begin(), r.end(), '\r'), r.end());
        run.result = r;
    }
    return run;
}

DiffusionRun record_diffusion_run(const fs::path& dir, DiffusionRecipe recipe, std::vector<std::vector<uint32_t>> pictures)
{
    const std::string pt = pictures_text(pictures, recipe.setup);
    recipe.pictures_sha256 = Sha256::hex(Sha256::hash(pt));
    recipe.check();
    fs::create_directories(dir);
    write_text(dir / "diffusion.ini", recipe.text());
    write_text(dir / "pictures.txt", pt);
    return load_diffusion_run(dir);
}

std::vector<std::vector<uint32_t>> draw_pictures(const Ranker& stack, uint32_t n, uint64_t seed)
{
    if (stack.count().is_zero()) bad("the stack keeps no pictures to draw");
    diffusion::SplitMix g(seed);
    std::vector<std::vector<uint32_t>> out;
    for (uint32_t i = 0; i < n; ++i)
    {
        BigUint k = BigUint(g.next());
        for (int s = 0; s < 4; ++s) k.mul_small(uint32_t(1) << 16);
        k += BigUint(g.next());
        BigUint q, rest;
        BigUint::divmod(k, stack.count(), q, rest);
        out.push_back(stack.unrank(rest));
    }
    return out;
}

DiffusionOutcome train_diffusion_run(const DiffusionRun& run, unsigned threads,
                                     const std::function<void(uint64_t, uint64_t, const training::Score&)>& progress,
                                     const std::atomic<bool>* cancel)
{
    const DiffusionRecipe& r = run.recipe;
    const AiSpace space(r.train.shape, "sieve");
    training::Trainer tr(space, start_digits(space, r.start_seed), r.train, threads);
    diffusion::SplitMix g(r.seed);
    std::ostringstream res;
    res << "sieve-diffusion-result-v1\nversion " << diffusion::kDiffusionVersion << " "
        << (r.train.version == 1 ? training::kTrainingVersion : training::kTrainingVersion2) << "\nsteps " << r.steps << "\n";
    res << "checkpoint 0 0 0 " << digest(tr.digits()) << "\n";
    training::Score since;
    DiffusionOutcome out;
    for (uint32_t k = 1; k <= r.steps; ++k)
    {
        if (cancel && *cancel)
        {
            out.cancelled = true;
            out.stopped_at = k - 1;
            return out;
        }
        const training::Score s = diffusion::step(tr, r.setup, run.pictures, g, r.batch);
        since.prob_sum += s.prob_sum, since.predictions += s.predictions, since.bits += s.bits;
        if (progress) progress(k, r.steps, s);
        if (k % r.checkpoint_every == 0 || k == r.steps)
        {
            res << "checkpoint " << k << " " << since.prob_sum << " " << since.predictions << " " << digest(tr.digits()) << "\n";
            since = {};
        }
    }
    res << "final " << space.hex_of(space.index_of(tr.digits(), AddressMode::Positional)) << "\n";
    out.text = res.str();
    out.digits = tr.digits();
    return out;
}

std::optional<std::string> final_address_of(const std::string& result_text)
{
    std::istringstream in(result_text);
    std::string w, hex;
    while (in >> w)
        if (w == "final") in >> hex;
    if (hex.empty()) return std::nullopt;
    return hex;
}

Space unit_space_of(const DiffusionRecipe& r)
{
    if (r.audio()) return Space(r.symbols, r.setup.colours, r.setup.width);
    ImageFormat fmt;
    fmt.width = r.setup.width, fmt.height = r.setup.height, fmt.palette = &palette_by_id(r.palette);
    return Space(fmt.symbols_id(), fmt.palette->size(), fmt.unit_length());
}

std::vector<uint32_t> nth_picture(const AiSpace& space, const AiSpace::Digits& digits, const diffusion::Setup& s, uint64_t k)
{
    if (k < 1) bad("pictures are numbered from 1");
    diffusion::SplitMix g(1);
    std::vector<uint32_t> noise(s.cells());
    for (uint64_t i = 0; i < k; ++i)
        for (auto& c : noise) c = uint32_t(g.next() % s.colours);
    return diffusion::make_picture(space, digits, s, noise, k);
}

PictureModel load_picture_model(const fs::path& dir)
{
    if (!is_diffusion_run(dir)) bad(dir.string() + " is not a diffusion run (it has no diffusion.ini)");
    PictureModel m;
    m.path = dir;
    const std::u8string n = dir.filename().u8string();
    m.name = std::string(n.begin(), n.end());
    m.recipe = DiffusionRecipe::parse(read_text(dir / "diffusion.ini"));
    const std::optional<std::string> hex = fs::exists(dir / "result.txt") ? final_address_of(read_text(dir / "result.txt")) : std::nullopt;
    if (!hex) bad(m.name + " has not been trained (no final model in result.txt)");
    m.space = std::make_shared<const AiSpace>(m.recipe.train.shape, "sieve");
    m.digits = m.space->digits_at(m.space->parse(*hex), AddressMode::Positional);
    return m;
}

std::vector<fs::path> picture_models_in(const std::vector<fs::path>& dirs)
{
    std::map<std::string, fs::path> by_name;
    for (const fs::path& d : dirs)
    {
        std::error_code ec;
        if (!fs::is_directory(d, ec)) continue;
        for (const auto& e : fs::directory_iterator(d, ec))
        {
            const fs::path p = e.path();
            const std::u8string n = p.filename().u8string();
            const std::string name(n.begin(), n.end());
            if (by_name.count(name) || !is_diffusion_run(p) || !fs::exists(p / "result.txt")) continue;
            by_name[name] = p;
        }
    }
    std::vector<fs::path> out;
    for (auto& [name, p] : by_name) out.push_back(p);
    return out;
}

} // namespace sieve::cli
