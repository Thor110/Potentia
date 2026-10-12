// Sieve CLI — `sieve ai` (see ai.hpp).
#include "cli/ai.hpp"

#include "cli/filter_config.hpp"
#include "sieve/compact.hpp"
#include "sieve/training.hpp"

#include "sieve/corridor.hpp"
#include "sieve/llm.hpp"
#include "sieve/llm_tokenizer.hpp"
#include "sieve/sha256.hpp"

#include <cctype>
#include <cmath>
#include <cstdio>
#include <iomanip>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>

namespace sieve::cli {

namespace {

std::filesystem::path path_of(const std::string& s) { return std::filesystem::path(std::u8string(s.begin(), s.end())); }

void write_file(const std::filesystem::path& p, const std::string& bytes)
{
    std::ofstream out(p, std::ios::binary);
    if (!out || !out.write(bytes.data(), std::streamsize(bytes.size()))) throw std::invalid_argument("cannot write " + p.string());
}

std::string read_file(const std::filesystem::path& p)
{
    std::ifstream in(p, std::ios::binary);
    if (!in) throw std::invalid_argument("cannot open " + p.string());
    std::ostringstream s;
    s << in.rdbuf();
    return s.str();
}

// What a model writes after a text, shown with its bytes as JSON writes a string.
std::string babble(const AiSpace& space, const AiSpace::Digits& d, const Args& a)
{
    llm::Model model(space.config(), space.tensors_of(d), 1);
    const llm::Tokenizer tok(json::parse(AiSpace::tokenizer_json()));
    llm::Sampling s;
    s.temperature = a.has("temperature") ? float(std::stod(a.get("temperature"))) : 0.8f;
    s.top_p = 1.0f;
    s.seed = a.has("seed") ? a.get_u32("seed", 1) : 1;
    llm::Sampler sampler(s);
    const size_t n = a.has("max-tokens") ? a.get_positive("max-tokens", 64) : 64;
    return llm::continue_text(model, tok, a.has("prompt") ? a.get("prompt") : std::string("\n"), sampler, n);
}

// Bytes as text: printable ASCII as it is, a newline, tab or quote escaped, any other byte as \xNN.
std::string shown(const std::string& bytes)
{
    static const char* hex = "0123456789abcdef";
    std::string out = "\"";
    for (const char ch : bytes)
    {
        const uint8_t c = uint8_t(ch);
        if (c == '\n') out += "\\n";
        else if (c == '\t') out += "\\t";
        else if (c == '"' || c == '\\') out += std::string("\\") + char(c);
        else if (c >= 0x20 && c < 0x7F) out.push_back(char(c));
        else out += std::string("\\x") + hex[c >> 4] + hex[c & 15];
    }
    return out + "\"";
}

FilterConfig filter_config_of(const Args& a)
{
    if (a.has("filters") && !std::filesystem::exists(path_of(a.get("filters"))))
        throw std::invalid_argument("no filter settings file at " + a.get("filters"));
    return FilterConfig::load(a.has("filters") ? path_of(a.get("filters")) : FilterConfig::default_path());
}

std::vector<uint32_t> unit_of(const AiSpace::Digits& d) { return std::vector<uint32_t>(d.begin(), d.end()); }

// A part of the line as a power of ten of the whole ("10^-143.06").
std::string share(double log2_part, double log2_all)
{
    const double x = (log2_part - log2_all) * std::log10(2.0);
    if (x > -0.005) return "about all";
    std::ostringstream o;
    o << "10^" << std::fixed << std::setprecision(2) << x;
    return o.str();
}

double log2_of(const BigUint& n)
{
    const size_t bits = n.bit_length();
    return bits < 64 ? std::log2(double(std::stoull(n.to_decimal()))) : double(bits) + std::log2(n.ratio_to_power_of_two(bits));
}

std::string shortened(const std::string& hex)
{
    return hex.size() > 64 ? hex.substr(0, 32) + "..." + hex.substr(hex.size() - 32) : hex;
}

} // namespace

AiShape ai_shape_of(const Args& a)
{
    AiShape s;
    if (a.has("ai-layers")) s.layers = a.get_positive("ai-layers", s.layers);
    if (a.has("ai-width")) s.width = a.get_positive("ai-width", s.width);
    if (a.has("ai-heads")) s.heads = a.get_positive("ai-heads", s.heads);
    if (a.has("ai-bits")) s.bits = a.get_positive("ai-bits", s.bits);
    s.check();
    return s;
}

int cmd_ai(const Args& a)
{
    const AiSpace space(ai_shape_of(a), a.get("key", "sieve"));
    const AddressMode mode = address_mode_from_string(a.get("mode", "positional"));
    const AiShape& sh = space.shape();
    // The line's filters (sieve-filters.ini's [ai]): each one's verdict, at the settings there.
    const FilterConfig cfg = filter_config_of(a);
    const FilterLine fl = ai_filter_line(space);
    const FilterStack stack = build_stack(fl, cfg.ai);
    std::unique_ptr<CompactLine> compact;
    if (!stack.empty() && stack.ranker()) compact = std::make_unique<CompactLine>(*stack.ranker(), a.get("key", "sieve"), stack.id());
    auto judge = [&](const AiSpace::Digits& d) {
        const std::vector<uint32_t> u = unit_of(d);
        std::string out;
        for (const FilterSpec* f : filters_for(fl))
        {
            const FilterStack one(fl, {{f, cfg.ai.values_of(f->name())}}, AppResources());
            out += (out.empty() ? "" : ", ") + f->name() + (cfg.ai.is_enabled(f->name()) ? " [x] " : " [ ] ") + (one.passes(u) ? "pass" : "FAIL");
        }
        std::cout << "filters      " << out << "\n";
        // What the AI line's own measures say of it: its language (language-v1's run and bytes, as
        // ticked or at its defaults) and, with near-model-v1 ticked, its layer round the anchor.
        if (cfg.ai.is_enabled("language-v1"))
        {
            const FilterSpec& ls = *find_filter("language-v1");
            const FilterValues& lv = cfg.ai.values_of("language-v1");
            const auto run = model_run(param_value(ls, lv, "run"));
            const size_t n = std::min<size_t>(size_t(param_int(ls, lv, "bytes")), run->text.size());
            const training::Score sc = training::evaluate_model(space, d, {std::span<const uint8_t>(run->text.data(), n)});
            char b[160];
            std::snprintf(b, sizeof b, "%.2f thousandths for the right next byte, on average (%.2f bits a byte), over %zu bytes",
                          double(sc.prob_sum) * 1000.0 / 65536.0 / double(sc.predictions), sc.bits_per_byte(), n);
            std::cout << "speaks       " << run->id << " (" << run->text_name << "): " << b << "\n";
        }
        if (cfg.ai.is_enabled("near-model-v1"))
        {
            const FilterSpec& ns = *find_filter("near-model-v1");
            const auto run = model_run(param_value(ns, cfg.ai.values_of("near-model-v1"), "run"));
            if (run->final_address.empty()) std::cout << "layer        " << run->id << " has not been trained\n";
            else
            {
                const AiSpace::Digits a = space.digits_at(space.parse(run->final_address), AddressMode::Positional);
                uint64_t k = 0;
                for (size_t i = 0; i < d.size(); ++i) k += a[i] != d[i] ? 1 : 0;
                std::cout << "layer        " << k << " (digits from the model " << run->id << " trained)\n";
            }
        }
        if (stack.empty()) return;
        if (!stack.passes(u)) std::cout << "stack        fails at " << stack.filter_name(size_t(stack.first_failure(u))) << "\n";
        else if (compact)
            for (const AddressMode m : {AddressMode::Positional, AddressMode::Scrambled})
            {
                const std::string hex = compact->hex_of(compact->index_of(u, m));
                std::cout << "compact      " << to_string(m) << (m == AddressMode::Positional ? " " : "  ") << shortened(hex) << " (" << hex.size()
                          << " hex digits)\n";
            }
        else std::cout << "stack        passes (it judges; no compact address: " << stack.compact_blocker() << ")\n";
    };
    // --canonical: the model of its class in the canonical form (canonical-model-v1) in its place.
    auto chosen = [&](AiSpace::Digits d) {
        if (!a.has("canonical")) return d;
        AiSpace::Digits c = space.canonical_of(d);
        std::cout << "canonical    " << (c == d ? "already its class's canonical form" : "its class's canonical form, in its place") << "\n";
        return c;
    };
    auto show_model = [&](const BigUint& index, const AiSpace::Digits& d) {
        const std::string hex = space.hex_of(index);
        std::cout << "address      " << (hex.size() > 64 ? hex.substr(0, 32) + "..." + hex.substr(hex.size() - 32) : hex) << " (" << hex.size()
                  << " hex digits; SHA-256 of the address " << Sha256::hex(Sha256::hash(hex)).substr(0, 16) << ")\n";
        std::cout << "says         " << shown(babble(space, d, a)) << "\n";
        judge(d);
        if (a.has("out"))
        {
            // The model's files, to keep or to talk to with `sieve chat --raw`.
            const std::filesystem::path dir = path_of(a.get("out"));
            std::filesystem::create_directories(dir);
            write_file(dir / "config.json", json::write(space.config()) + "\n");
            write_file(dir / "model.safetensors", space.safetensors_of(d));
            write_file(dir / "tokenizer.json", AiSpace::tokenizer_json());
            std::cout << "wrote        " << a.get("out") << " (config.json, model.safetensors, tokenizer.json)\n";
        }
    };

    if (a.has("read") || a.has("address-file") || a.has("bearing"))
    {
        BigUint index;
        if (a.has("read")) index = space.parse(a.get("read"));
        else if (a.has("address-file"))
        {
            // An address too long for a command line (a wide model's runs past 128 KB): from a file.
            std::string hex = read_file(path_of(a.get("address-file")));
            while (!hex.empty() && std::isspace(uint8_t(hex.back()))) hex.pop_back();
            index = space.parse(hex);
        }
        else
        {
            const double deg = std::stod(a.get("bearing"));
            if (!(deg >= 0 && deg < 360) || deg != std::floor(deg)) throw std::invalid_argument("--bearing takes a whole number of degrees, 0 to 359");
            index = unit_at_bearing(BigUint(uint64_t(deg)), space.size(), 0);
            std::cout << "bearing      " << uint64_t(deg) << " degrees, " << to_string(mode) << "\n";
        }
        const AiSpace::Digits d = chosen(space.digits_at(index, mode));
        show_model(space.index_of(d, mode), d);
        if (a.has("address-out")) write_file(path_of(a.get("address-out")), space.hex_of(space.index_of(d, mode)) + "\n");
        return 0;
    }
    if (a.has("warp"))
    {
        // A model's file, or the folder it is in, back to its address on the line.
        std::filesystem::path p = path_of(a.get("warp"));
        if (std::filesystem::is_directory(p)) p /= "model.safetensors";
        auto found = space.digits_of_safetensors(read_file(p));
        if (!found) throw std::invalid_argument(p.string() + " is not a model of this line's shape whose weights are all the line's values");
        const AiSpace::Digits d = chosen(*found);
        for (const AddressMode m : {AddressMode::Positional, AddressMode::Scrambled})
        {
            const std::string hex = space.hex_of(space.index_of(d, m));
            std::cout << to_string(m) << (m == AddressMode::Positional ? "   " : "    ") << shortened(hex) << "  SHA-256 "
                      << Sha256::hex(Sha256::hash(hex)).substr(0, 16) << "\n";
        }
        judge(d);
        if (a.has("address-out")) write_file(path_of(a.get("address-out")), space.hex_of(space.index_of(d, mode)) + "\n");
        if (a.has("out"))
        {
            const std::filesystem::path dir = path_of(a.get("out"));
            std::filesystem::create_directories(dir);
            write_file(dir / "config.json", json::write(space.config()) + "\n");
            write_file(dir / "model.safetensors", space.safetensors_of(d));
            write_file(dir / "tokenizer.json", AiSpace::tokenizer_json());
            std::cout << "wrote        " << a.get("out") << " (config.json, model.safetensors, tokenizer.json)\n";
        }
        return 0;
    }
    if (a.has("browse"))
    {
        std::mt19937_64 rng(a.has("seed") ? a.get_u32("seed", 1) : 1);
        for (uint64_t i = 0, n = a.get_positive("browse", 3); i < n; ++i)
        {
            AiSpace::Digits d(size_t(space.weights()));
            for (auto& x : d) x = uint8_t(rng() & ((1u << sh.bits) - 1));
            d = chosen(d);
            show_model(space.index_of(d, mode), d);
        }
        return 0;
    }
    char b[64];
    std::snprintf(b, sizeof b, "%.1f", double(space.bits()) * std::log10(2.0));
    std::cout << "line         ai  (" << space.id() << ")\n";
    std::cout << "shape        " << sh.layers << (sh.layers == 1 ? " layer" : " layers") << ", width " << sh.width << ", " << sh.heads << " heads of "
              << sh.width / sh.heads << ", feed-forward " << sh.ffn() << ", a vocabulary of the 256 bytes\n";
    std::cout << "weights      " << space.weights() << ", " << sh.bits << (sh.bits == 1 ? " bit" : " bits") << " each\n";
    std::cout << "models       2^" << space.bits() << " (~10^" << b << ")\n";
    std::cout << "address      " << space.hex_width() << " hex digits\n";
    return 0;
}

int cmd_filters_ai(const Args& a)
{
    const AiSpace space(ai_shape_of(a), a.get("key", "sieve"));
    const FilterConfig cfg = filter_config_of(a);
    const LineFilters& lf = cfg.ai;
    const FilterLine fl = ai_filter_line(space);
    const double all = double(space.bits());
    std::cout << "line         ai  (" << space.id() << ")\n"
              << "settings     " << (a.has("filters") ? a.get("filters") : FilterConfig::default_path().string()) << "\n"
              << "mode         " << to_string(lf.mode) << "\n\n";
    for (const FilterSpec* f : filters_for(fl))
    {
        std::cout << (lf.is_enabled(f->name()) ? "[x] " : "[ ] ") << f->name() << (f->category.empty() ? "" : "   (" + f->category + ")") << "\n";
        std::istringstream desc(f->description);
        for (std::string l; std::getline(desc, l);) std::cout << "      " << l << "\n";
        for (const auto& p : f->params) std::cout << "      " << p.key << " = " << param_value(*f, lf.values_of(f->name()), p.key) << "   " << p.description << "\n";
        // What it keeps alone, at these settings: exactly where it counts.
        const FilterStack one(fl, {{f, lf.values_of(f->name())}}, AppResources());
        if (one.ranker())
        {
            BigUint out = space.size();
            out -= one.ranker()->count();
            std::cout << "      keeps " << share(log2_of(one.ranker()->count()), all) << " of the line and sets aside "
                      << (out.is_zero() ? std::string("none") : share(log2_of(out), all)) << " (exact; compact mode available)\n";
        }
        else if (f->id == "canonical-model")
        {
            // Its count, exact up to a line of 2^20 bits; past that, from its log.
            if (space.bits() <= (uint64_t(1) << 20)) std::cout << "      keeps " << share(log2_of(space.canonical_count()), all) << " of the line (exact; it judges)\n";
            else std::cout << "      keeps " << share(space.canonical_log2_count(), all) << " of the line (it judges)\n";
            std::cout << "      sets aside all but 1 in 2^" << std::fixed << std::setprecision(1) << all - space.canonical_log2_count()
                      << std::defaultfloat << " (each class's other models)\n";
        }
    }
    const FilterStack st = build_stack(fl, lf);
    if (st.empty()) return 0;
    std::cout << "\nstack        " << st.id().substr(0, 16) << "...  " << st.provenance() << "\n";
    if (st.ranker())
    {
        const BigUint& n = st.ranker()->count();
        std::cout << "survivors    2^" << std::fixed << std::setprecision(2) << log2_of(n) << std::defaultfloat << " (exact; compact mode available)";
        if (a.has("exact")) std::cout << ": " << n.to_decimal();
        BigUint out = space.size();
        out -= n;
        std::cout << "\nshare        the stack keeps " << share(log2_of(n), all) << " of the line and sets aside "
                  << (out.is_zero() ? std::string("none") : share(log2_of(out), all)) << "\n";
    }
    else std::cout << "compact      unavailable: " << st.compact_blocker() << "\n";
    return 0;
}

} // namespace sieve::cli
