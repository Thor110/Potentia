// Sieve — the vault's decoders (vault_decode.hpp), vault-decoders-v1: each takes a unit's whole text
// and gives back the bytes it would be in that encoding, or nothing when it is not that encoding.
//
// The set, in order:
//   text     the text's own bytes: its code points as bytes when all are below 256 (Latin-1, which
//            for ASCII is also its UTF-8), else its UTF-8; as it is, without its trailing
//            whitespace (a unit's padding), and so with one line feed (a file's last line)
//   hex      hexadecimal digits, either case, with 0x or \x prefixes and any separators among
//            space , : ; - _ allowed; an even number of digits
//   base64   the standard or the URL-safe alphabet, padded or not (a data: URL's prefix is dropped)
//   base32   RFC 4648, either case, padded or not
//   ascii85  Adobe's, with or without its <~ ~> marks, 'z' for four zero bytes
//   decimal  byte values 0-255 written in decimal, separated by spaces or commas, in brackets or not
//   nibbles  the sixteen letters a-p, one per four bits (a is 0): hex for an alphabet without digits
//   spelled  hex digits spelled out: zero to nine, ten to fifteen, or the letters a to f, as words
//   binary   any two symbols, one per bit, most significant first, eight to a byte; both ways round
// Whitespace is ignored throughout. Nothing here allocates more than a few times the text's size.

#include "cli/vault_decode.hpp"

#include "cli/locate.hpp"
#include "cli/vault.hpp"
#include "cli/timings.hpp"

#include <algorithm>
#include <array>
#include <optional>
#include <set>

namespace sieve::cli::vault {

namespace {

using Bytes = std::vector<uint8_t>;

bool space(char c) { return c == ' ' || c == '\n' || c == '\r' || c == '\t' || c == '\f' || c == '\v'; }

std::string strip(const std::string& s, bool (*drop)(char))
{
    std::string out;
    out.reserve(s.size());
    for (char c : s)
        if (!drop(c)) out += c;
    return out;
}

std::string lower(std::string s)
{
    for (char& c : s)
        if (c >= 'A' && c <= 'Z') c = char(c - 'A' + 'a');
    return s;
}

int hex_value(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

std::optional<Bytes> from_nibbles(const std::vector<int>& n)
{
    if (n.empty() || n.size() % 2 != 0) return std::nullopt;
    Bytes b(n.size() / 2);
    for (size_t i = 0; i < b.size(); ++i) b[i] = uint8_t(n[2 * i] << 4 | n[2 * i + 1]);
    return b;
}

// text: Latin-1 when every code point fits in a byte, else the UTF-8 as it is.
std::optional<Bytes> text_bytes(const std::string& s)
{
    Bytes out;
    out.reserve(s.size());
    for (size_t i = 0; i < s.size();)
    {
        const auto c = static_cast<unsigned char>(s[i]);
        if (c < 0x80) { out.push_back(c); ++i; continue; }
        if ((c & 0xE0) == 0xC0 && i + 1 < s.size())
        {
            const uint32_t cp = uint32_t(c & 0x1F) << 6 | uint32_t(static_cast<unsigned char>(s[i + 1]) & 0x3F);
            if (cp < 0x100) { out.push_back(uint8_t(cp)); i += 2; continue; }
        }
        return Bytes(s.begin(), s.end()); // a code point past U+00FF: the UTF-8 is the only reading
    }
    return out;
}

std::optional<Bytes> hex(const std::string& s)
{
    // Prefixes first (0x.., \x..), then separators.
    std::string u;
    for (size_t i = 0; i < s.size(); ++i)
    {
        if ((s[i] == '0' || s[i] == '\\') && i + 1 < s.size() && (s[i + 1] == 'x' || s[i + 1] == 'X'))
        {
            ++i;
            continue;
        }
        u += s[i];
    }
    std::vector<int> n;
    for (char c : u)
    {
        if (space(c) || c == ',' || c == ':' || c == ';' || c == '-' || c == '_') continue;
        const int v = hex_value(c);
        if (v < 0) return std::nullopt;
        n.push_back(v);
    }
    return from_nibbles(n);
}

std::optional<Bytes> base64(const std::string& s)
{
    std::string t = strip(s, space);
    const size_t data = t.find("base64,");
    if (t.rfind("data:", 0) == 0 && data != std::string::npos) t = t.substr(data + 7);
    while (!t.empty() && t.back() == '=') t.pop_back();
    if (t.empty() || t.size() % 4 == 1) return std::nullopt;
    Bytes out;
    uint32_t acc = 0;
    int bits = 0;
    bool std_alpha = false, url_alpha = false;
    for (char c : t)
    {
        int v;
        if (c >= 'A' && c <= 'Z') v = c - 'A';
        else if (c >= 'a' && c <= 'z') v = c - 'a' + 26;
        else if (c >= '0' && c <= '9') v = c - '0' + 52;
        else if (c == '+' || c == '/') { v = c == '+' ? 62 : 63; std_alpha = true; }
        else if (c == '-' || c == '_') { v = c == '-' ? 62 : 63; url_alpha = true; }
        else return std::nullopt;
        if (std_alpha && url_alpha) return std::nullopt;
        acc = acc << 6 | uint32_t(v);
        bits += 6;
        if (bits >= 8)
        {
            bits -= 8;
            out.push_back(uint8_t(acc >> bits));
            acc &= (1u << bits) - 1;
        }
    }
    return out;
}

std::optional<Bytes> base32(const std::string& s)
{
    std::string t = strip(s, space);
    while (!t.empty() && t.back() == '=') t.pop_back();
    if (t.empty()) return std::nullopt;
    Bytes out;
    uint32_t acc = 0;
    int bits = 0;
    for (char c : t)
    {
        int v;
        if (c >= 'A' && c <= 'Z') v = c - 'A';
        else if (c >= 'a' && c <= 'z') v = c - 'a';
        else if (c >= '2' && c <= '7') v = c - '2' + 26;
        else return std::nullopt;
        acc = acc << 5 | uint32_t(v);
        bits += 5;
        if (bits >= 8)
        {
            bits -= 8;
            out.push_back(uint8_t(acc >> bits));
            acc &= (1u << bits) - 1;
        }
    }
    return out;
}

std::optional<Bytes> ascii85(const std::string& s)
{
    std::string t = strip(s, space);
    if (t.rfind("<~", 0) == 0) t = t.substr(2);
    if (t.size() >= 2 && t.compare(t.size() - 2, 2, "~>") == 0) t.resize(t.size() - 2);
    if (t.empty()) return std::nullopt;
    Bytes out;
    std::array<uint32_t, 5> g{};
    size_t k = 0;
    for (char c : t)
    {
        if (c == 'z' && k == 0)
        {
            out.insert(out.end(), size_t(4), uint8_t(0));
            continue;
        }
        if (c < '!' || c > 'u') return std::nullopt;
        g[k++] = uint32_t(c - '!');
        if (k == 5)
        {
            uint64_t v = 0;
            for (uint32_t d : g) v = v * 85 + d;
            if (v > 0xFFFFFFFFull) return std::nullopt;
            for (int i = 3; i >= 0; --i) out.push_back(uint8_t(v >> (8 * i)));
            k = 0;
        }
    }
    if (k == 1) return std::nullopt;
    if (k > 1)
    {
        for (size_t i = k; i < 5; ++i) g[i] = 84;
        uint64_t v = 0;
        for (uint32_t d : g) v = v * 85 + d;
        if (v > 0xFFFFFFFFull) return std::nullopt;
        for (size_t i = 0; i + 1 < k; ++i) out.push_back(uint8_t(v >> (8 * (3 - i))));
    }
    return out;
}

std::optional<Bytes> decimal(const std::string& s)
{
    Bytes out;
    uint32_t v = 0;
    int digits = 0;
    auto end = [&]() -> bool {
        if (digits == 0) return true;
        if (v > 255) return false;
        out.push_back(uint8_t(v));
        v = 0;
        digits = 0;
        return true;
    };
    for (char c : s)
    {
        if (c >= '0' && c <= '9')
        {
            if (++digits > 3) return std::nullopt;
            v = v * 10 + uint32_t(c - '0');
            continue;
        }
        if (!(space(c) || c == ',' || c == ';' || c == '[' || c == ']' || c == '{' || c == '}' || c == '(' || c == ')'))
            return std::nullopt;
        if (!end()) return std::nullopt;
    }
    if (!end() || out.empty()) return std::nullopt;
    return out;
}

std::optional<Bytes> nibbles(const std::string& s)
{
    std::vector<int> n;
    for (char c : lower(s))
    {
        if (space(c)) continue;
        if (c < 'a' || c > 'p') return std::nullopt;
        n.push_back(c - 'a');
    }
    return from_nibbles(n);
}

std::optional<Bytes> spelled(const std::string& s)
{
    static const char* const words[] = {"zero", "one", "two", "three", "four", "five", "six", "seven",
                                        "eight", "nine", "ten", "eleven", "twelve", "thirteen", "fourteen", "fifteen"};
    std::vector<int> n;
    std::string w;
    auto end = [&]() -> bool {
        if (w.empty()) return true;
        int v = -1;
        if (w.size() == 1 && w[0] >= 'a' && w[0] <= 'f') v = w[0] - 'a' + 10;
        for (int i = 0; i < 16 && v < 0; ++i)
            if (w == words[i]) v = i;
        w.clear();
        if (v < 0) return false;
        n.push_back(v);
        return true;
    };
    for (char c : lower(s))
    {
        if (c >= 'a' && c <= 'z') { w += c; continue; }
        if (!(space(c) || c == ',' || c == '.' || c == '-')) return std::nullopt;
        if (!end()) return std::nullopt;
    }
    if (!end()) return std::nullopt;
    return from_nibbles(n);
}

void binary(const std::string& s, std::vector<Bytes>& out)
{
    const std::string t = strip(s, space);
    if (t.empty() || t.size() % 8 != 0) return;
    const char a = t[0];
    char b = 0;
    for (char c : t)
    {
        if (c == a) continue;
        if (b == 0) b = c;
        else if (c != b) return;
    }
    if (b == 0) return; // one symbol only: not two ways to read it, and all zeros or all ones
    for (int way = 0; way < 2; ++way)
    {
        Bytes bytes(t.size() / 8);
        for (size_t i = 0; i < t.size(); ++i)
        {
            const bool one = (t[i] == b) != (way == 1);
            if (one) bytes[i / 8] = uint8_t(bytes[i / 8] | (0x80u >> (i % 8)));
        }
        out.push_back(std::move(bytes));
    }
}

} // namespace

const std::vector<std::string>& decoder_names()
{
    static const std::vector<std::string> names = {"text", "hex", "base64", "base32", "ascii85", "decimal", "nibbles", "spelled", "binary"};
    return names;
}

std::vector<Bytes> decodings(const std::string& utf8)
{
    std::vector<Bytes> all;
    if (auto t = text_bytes(utf8))
    {
        Bytes trimmed = *t;
        while (!trimmed.empty() && space(char(trimmed.back()))) trimmed.pop_back();
        all.push_back(*t);
        all.push_back(trimmed);
        trimmed.push_back('\n');
        all.push_back(std::move(trimmed));
    }
    for (auto f : {hex, base64, base32, ascii85, decimal, nibbles, spelled})
        if (auto b = f(utf8); b && !b->empty()) all.push_back(std::move(*b));
    binary(utf8, all);
    std::set<Bytes> seen;
    std::vector<Bytes> out;
    for (Bytes& b : all)
        if (!b.empty() && seen.insert(b).second) out.push_back(std::move(b));
    return out;
}

bool withheld_written(const std::string& utf8)
{
    timings::Scope t("vault.decoders");
    if (status().failed_closed) return true;
    for (const Bytes& b : decodings(utf8))
        if (withheld_bytes(b)) return true;
    return false;
}

} // namespace sieve::cli::vault
