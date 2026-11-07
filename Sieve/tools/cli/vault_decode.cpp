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
#include "sieve/written.hpp"

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

} // namespace

const std::vector<std::string>& decoder_names()
{
    static const std::vector<std::string> names = {"text", "hex", "base64", "base32", "ascii85", "decimal", "nibbles", "spelled", "binary"};
    return names;
}

std::vector<Bytes> decodings(const std::string& utf8)
{
    // (The readings are not-written-v1's own, sieve/written.hpp: one copy of each decoder. Until 7
    // November 2026 the vault had its own copies, which had come to differ on text that is not
    // UTF-8 and on a NUL among binary's two symbols.)
    std::vector<Bytes> all = sieve::written_readings(utf8);
    if (auto b = ascii85(utf8)) all.push_back(std::move(*b));
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
