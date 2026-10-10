// Sieve CLI — `sieve ai` (see ai.hpp).
#include "cli/ai.hpp"

#include "sieve/corridor.hpp"
#include "sieve/llm.hpp"
#include "sieve/llm_tokenizer.hpp"
#include "sieve/sha256.hpp"

#include <cmath>
#include <cstdio>
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
    auto show_model = [&](const BigUint& index, const AiSpace::Digits& d) {
        const std::string hex = space.hex_of(index);
        std::cout << "address      " << (hex.size() > 64 ? hex.substr(0, 32) + "..." + hex.substr(hex.size() - 32) : hex) << " (" << hex.size()
                  << " hex digits; SHA-256 of the address " << Sha256::hex(Sha256::hash(hex)).substr(0, 16) << ")\n";
        std::cout << "says         " << shown(babble(space, d, a)) << "\n";
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

    if (a.has("read") || a.has("bearing"))
    {
        BigUint index;
        if (a.has("read")) index = space.parse(a.get("read"));
        else
        {
            const double deg = std::stod(a.get("bearing"));
            if (!(deg >= 0 && deg < 360) || deg != std::floor(deg)) throw std::invalid_argument("--bearing takes a whole number of degrees, 0 to 359");
            index = unit_at_bearing(BigUint(uint64_t(deg)), space.size(), 0);
            std::cout << "bearing      " << uint64_t(deg) << " degrees, " << to_string(mode) << "\n";
        }
        show_model(index, space.digits_at(index, mode));
        return 0;
    }
    if (a.has("warp"))
    {
        // A model's file, or the folder it is in, back to its address on the line.
        std::filesystem::path p = path_of(a.get("warp"));
        if (std::filesystem::is_directory(p)) p /= "model.safetensors";
        const auto d = space.digits_of_safetensors(read_file(p));
        if (!d) throw std::invalid_argument(p.string() + " is not a model of this line's shape whose weights are all the line's values");
        for (const AddressMode m : {AddressMode::Positional, AddressMode::Scrambled})
        {
            const std::string hex = space.hex_of(space.index_of(*d, m));
            std::cout << to_string(m) << (m == AddressMode::Positional ? "   " : "    ") << (hex.size() > 64 ? hex.substr(0, 32) + "..." + hex.substr(hex.size() - 32) : hex)
                      << "  SHA-256 " << Sha256::hex(Sha256::hash(hex)).substr(0, 16) << "\n";
        }
        if (a.has("address-out")) write_file(path_of(a.get("address-out")), space.hex_of(space.index_of(*d, mode)) + "\n");
        return 0;
    }
    if (a.has("browse"))
    {
        std::mt19937_64 rng(a.has("seed") ? a.get_u32("seed", 1) : 1);
        for (uint64_t i = 0, n = a.get_positive("browse", 3); i < n; ++i)
        {
            AiSpace::Digits d(size_t(space.weights()));
            for (auto& x : d) x = uint8_t(rng() & ((1u << sh.bits) - 1));
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

} // namespace sieve::cli
