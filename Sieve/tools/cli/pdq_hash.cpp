// Sieve — PDQ (pdq_hash.hpp): the picture's luminance handed to Meta's reference code, and its
// hashes read back in PDQ's own hex order. Compiled with the vendored code, outside the core's
// strict warning flags, since it includes PDQ's headers.

#include "pdq_hash.hpp"

#include <pdq/cpp/common/pdqhashtypes.h>
#include <pdq/cpp/hashing/pdqhashing.h>

#include <bit>
#include <vector>

namespace sieve::cli::pdq {

namespace {

namespace fp = facebook::pdq::hashing;

Hash from_pdq(const fp::Hash256& h)
{
    // PDQ writes w[15] first, each word as four hex digits, most significant first.
    Hash out{};
    for (int i = 0; i < 16; ++i)
    {
        const uint16_t w = h.w[15 - i];
        out[size_t(2 * i)] = uint8_t(w >> 8);
        out[size_t(2 * i + 1)] = uint8_t(w & 0xFF);
    }
    return out;
}

} // namespace

std::optional<Hashes> hash_rgba(const uint8_t* rgba, uint32_t width, uint32_t height)
{
    if (width < 5 || height < 5 || width > 0x7FFF'FFFFu / 4 || height > 0x7FFF'FFFFu / (width * 4)) return std::nullopt;
    const int rows = int(height), cols = int(width);
    std::vector<float> a(size_t(rows) * size_t(cols)), b(a.size());
    uint8_t* base = const_cast<uint8_t*>(rgba); // PDQ's signature takes non-const; it only reads
    fp::fillFloatLumaFromRGB(base, base + 1, base + 2, rows, cols, cols * 4, 4, a.data());
    static thread_local float b64[64][64], b16x64[16][64], b16[16][16], b16aux[16][16];
    fp::Hash256 h[8];
    int quality = 0;
    fp::pdqDihedralHash256esFromFloatLuma(a.data(), b.data(), rows, cols, b64, b16x64, b16, b16aux, &h[0], &h[1], &h[2], &h[3],
                                          &h[4], &h[5], &h[6], &h[7], quality);
    Hashes out;
    for (size_t i = 0; i < 8; ++i) out.orientations[i] = from_pdq(h[i]);
    out.quality = quality;
    return out;
}

int distance(const Hash& a, const Hash& b)
{
    int d = 0;
    for (size_t i = 0; i < a.size(); ++i) d += std::popcount(unsigned(a[i] ^ b[i]));
    return d;
}

std::string to_hex(const Hash& h)
{
    static const char* const digits = "0123456789abcdef";
    std::string s;
    for (uint8_t v : h)
    {
        s += digits[v >> 4];
        s += digits[v & 15];
    }
    return s;
}

std::optional<Hash> from_hex(const std::string& hex)
{
    if (hex.size() != 64) return std::nullopt;
    Hash h{};
    for (size_t i = 0; i < 64; ++i)
    {
        const char c = hex[i];
        int v;
        if (c >= '0' && c <= '9') v = c - '0';
        else if (c >= 'a' && c <= 'f') v = c - 'a' + 10;
        else return std::nullopt;
        h[i / 2] = uint8_t(h[i / 2] | (i % 2 ? v : v << 4));
    }
    return h;
}

} // namespace sieve::cli::pdq
