#include "sieve/cipher.hpp"

#include "sieve/space.hpp"

#include <algorithm>
#include <array>
#include <stdexcept>

namespace sieve::cipher {

namespace {

[[noreturn]] void bad(const std::string& what) { throw std::invalid_argument("cipher-v1: " + what); }

std::string trim(std::string_view s)
{
    size_t a = 0, b = s.size();
    while (a < b && (s[a] == ' ' || s[a] == '\t' || s[a] == '\r')) ++a;
    while (b > a && (s[b - 1] == ' ' || s[b - 1] == '\t' || s[b - 1] == '\r')) --b;
    return std::string(s.substr(a, b - a));
}

int hex_value(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

Layer parse_layer(const std::string& line)
{
    const size_t sp = line.find(' ');
    const std::string word = line.substr(0, sp), arg = sp == std::string::npos ? std::string() : line.substr(sp + 1);
    Layer l;
    if (word == "reverse")
    {
        if (!arg.empty()) bad("reverse takes no key");
        l.kind = Layer::Kind::Reverse;
    }
    else if (word == "caesar")
    {
        if (arg.empty() || arg.size() > 3 || arg.find_first_not_of("0123456789") != std::string::npos || std::stoi(arg) > 255)
            bad("caesar needs a shift from 0 to 255, not \"" + arg + "\"");
        l.kind = Layer::Kind::Caesar;
        l.key = {uint8_t(std::stoi(arg))};
    }
    else if (word == "xor")
    {
        if (arg.empty() || arg.size() % 2) bad("xor needs a key of whole bytes in hex, not \"" + arg + "\"");
        l.kind = Layer::Kind::Xor;
        for (size_t i = 0; i < arg.size(); i += 2)
        {
            const int hi = hex_value(arg[i]), lo = hex_value(arg[i + 1]);
            if (hi < 0 || lo < 0) bad("xor needs a key in hex, not \"" + arg + "\"");
            l.key.push_back(uint8_t(hi * 16 + lo));
        }
    }
    else if (word == "vigenere-hex" || word == "feistel-hex")
    {
        if (arg.empty() || arg.size() % 2) bad(word + " needs a key of whole bytes in hex, not \"" + arg + "\"");
        l.kind = word == "vigenere-hex" ? Layer::Kind::Vigenere : Layer::Kind::Feistel;
        for (size_t i = 0; i < arg.size(); i += 2)
        {
            const int hi = hex_value(arg[i]), lo = hex_value(arg[i + 1]);
            if (hi < 0 || lo < 0) bad(word + " needs a key in hex, not \"" + arg + "\"");
            l.key.push_back(uint8_t(hi * 16 + lo));
        }
    }
    else if (word == "vigenere" || word == "feistel")
    {
        if (arg.empty()) bad(word + " needs a key");
        l.kind = word == "vigenere" ? Layer::Kind::Vigenere : Layer::Kind::Feistel;
        l.key.assign(arg.begin(), arg.end());
    }
    else bad("not a layer: \"" + line + "\"");
    return l;
}

// The binary line of n bytes with key K: its scramble is the feistel layer.
Space feistel_space(const Layer& l, size_t n)
{
    return Space("bytes256", 256, uint32_t(n), std::string(l.key.begin(), l.key.end()));
}

void apply(const Layer& l, std::vector<uint8_t>& b, bool forward)
{
    switch (l.kind)
    {
    case Layer::Kind::Reverse: std::reverse(b.begin(), b.end()); break;
    case Layer::Kind::Caesar:
        for (auto& c : b) c = uint8_t(forward ? c + l.key[0] : c - l.key[0]);
        break;
    case Layer::Kind::Xor:
        for (size_t i = 0; i < b.size(); ++i) b[i] ^= l.key[i % l.key.size()];
        break;
    case Layer::Kind::Vigenere:
        for (size_t i = 0; i < b.size(); ++i) b[i] = uint8_t(forward ? b[i] + l.key[i % l.key.size()] : b[i] - l.key[i % l.key.size()]);
        break;
    case Layer::Kind::Feistel:
    {
        if (b.empty()) break;
        if (b.size() > 0xFFFFFFFFu) bad("a file of more than 2^32 - 1 bytes");
        const Space s = feistel_space(l, b.size());
        Space::Digits d(b.begin(), b.end());
        d = forward ? s.scramble(std::move(d)) : s.unscramble(std::move(d));
        for (size_t i = 0; i < b.size(); ++i) b[i] = uint8_t(d[i]);
        break;
    }
    }
}

// English letter frequencies in hundredths of a percent (a 817 means 8.17%), for a to z.
constexpr std::array<int64_t, 26> kLetters = {817, 149, 278, 425, 1270, 223, 202, 609, 697, 15, 77, 403, 241,
                                              675, 751, 193, 10,  599, 633, 906,  276, 98,  236, 15,  197, 7};

int64_t weight(uint8_t c)
{
    if (c == ' ') return 1500;
    if (c >= 'a' && c <= 'z') return kLetters[size_t(c - 'a')];
    if (c >= 'A' && c <= 'Z') return kLetters[size_t(c - 'A')];
    if ((c >= 0x21 && c <= 0x7E) || c == '\n' || c == '\r' || c == '\t') return 100;
    return -1000;
}

} // namespace

std::string Layer::text() const
{
    static const char kHex[] = "0123456789abcdef";
    std::string h;
    for (const uint8_t c : key) h += {kHex[c >> 4], kHex[c & 15]};
    // A text key is written as text when it reads back the same: printable ASCII, no ';' or '#',
    // and no space at either end (which a recipe line would lose).
    bool plain = !key.empty() && key.front() != ' ' && key.back() != ' ';
    for (const uint8_t c : key)
        if (c < 0x20 || c > 0x7E || c == ';' || c == '#') plain = false;
    switch (kind)
    {
    case Kind::Reverse: return "reverse";
    case Kind::Caesar: return "caesar " + std::to_string(key[0]);
    case Kind::Xor: return "xor " + h;
    case Kind::Vigenere: return plain ? "vigenere " + std::string(key.begin(), key.end()) : "vigenere-hex " + h;
    case Kind::Feistel: return plain ? "feistel " + std::string(key.begin(), key.end()) : "feistel-hex " + h;
    }
    return "reverse";
}

Recipe Recipe::parse(std::string_view text)
{
    Recipe r;
    std::string part;
    bool first = true;
    auto flush = [&] {
        const std::string line = trim(part);
        part.clear();
        if (line.empty() || line[0] == '#') return;
        if (first && line == "sieve-cipher-v1")
        {
            first = false;
            return;
        }
        first = false;
        r.layers.push_back(parse_layer(line));
    };
    for (const char c : text)
        if (c == '\n' || c == ';') flush();
        else part.push_back(c);
    flush();
    if (r.layers.empty()) bad("a recipe needs at least one layer");
    return r;
}

std::string Recipe::text() const
{
    std::string t = "sieve-cipher-v1\n";
    for (const Layer& l : layers) t += l.text() + "\n";
    return t;
}

std::vector<uint8_t> encrypt(const Recipe& r, std::vector<uint8_t> bytes)
{
    for (const Layer& l : r.layers) apply(l, bytes, true);
    return bytes;
}

std::vector<uint8_t> decrypt(const Recipe& r, std::vector<uint8_t> bytes)
{
    for (auto it = r.layers.rbegin(); it != r.layers.rend(); ++it) apply(*it, bytes, false);
    return bytes;
}

int64_t english_score(const std::vector<uint8_t>& bytes)
{
    int64_t s = 0;
    for (const uint8_t c : bytes) s += weight(c);
    return s;
}

std::vector<Guess> break_weak(const std::vector<uint8_t>& cipher, size_t max_key, size_t top)
{
    std::vector<Guess> all;
    auto add = [&](Layer l) {
        Guess g;
        const int64_t extra = int64_t(l.key.size()) - 1;
        g.recipe.layers.push_back(std::move(l));
        g.plain = decrypt(g.recipe, cipher);
        g.score = english_score(g.plain);
        g.rank = g.score - kKeyByteCost * extra;
        all.push_back(std::move(g));
    };
    // A key whose bytes repeat with a shorter period is that shorter key, already tried.
    auto repeats = [](const std::vector<uint8_t>& k) {
        for (size_t p = 1; p < k.size(); ++p)
            if (k.size() % p == 0 && std::equal(k.begin() + std::ptrdiff_t(p), k.end(), k.begin())) return true;
        return false;
    };
    for (int k = 0; k < 256; ++k) add(Layer{Layer::Kind::Caesar, {uint8_t(k)}});
    for (int k = 0; k < 256; ++k) add(Layer{Layer::Kind::Xor, {uint8_t(k)}});
    // Repeating keys, column by column: each column's byte is the one whose decryption of that
    // column scores best (the lowest byte on a tie).
    for (size_t m = 2; m <= max_key && cipher.size() >= 8 * m; ++m)
        for (const Layer::Kind kind : {Layer::Kind::Xor, Layer::Kind::Vigenere})
        {
            Layer l{kind, std::vector<uint8_t>(m, 0)};
            for (size_t col = 0; col < m; ++col)
            {
                int64_t best = 0;
                bool any = false;
                for (int k = 0; k < 256; ++k)
                {
                    int64_t s = 0;
                    for (size_t i = col; i < cipher.size(); i += m)
                        s += weight(kind == Layer::Kind::Xor ? uint8_t(cipher[i] ^ k) : uint8_t(cipher[i] - k));
                    if (!any || s > best)
                    {
                        best = s;
                        l.key[col] = uint8_t(k);
                        any = true;
                    }
                }
            }
            if (!repeats(l.key)) add(std::move(l));
        }
    std::stable_sort(all.begin(), all.end(), [](const Guess& a, const Guess& b) { return a.rank > b.rank; });
    if (all.size() > top) all.resize(top);
    return all;
}

} // namespace sieve::cipher
