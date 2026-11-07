#pragma once
// The one reader of the numbers Sieve writes in its own codes and streams: seven bits a byte, the
// lowest first, the top bit set on every byte but the last (LEB128, unsigned). Every writer writes
// a number in as few bytes as it fits, so only that form is read: a tenth byte with more than the
// one bit left for it, or a last byte of 0 after others (a number written longer than it is), is
// refused rather than read as something else. (Review of 4 November 2026: of the copies this
// replaces, most took a tenth byte's surplus bits silently.) The formats of others (xz, RAR) keep
// their own readers, as their rules are theirs.
#include <cstddef>
#include <cstdint>
#include <span>

namespace sieve {

enum class VarintFault
{
    none,
    ends,         // the bytes end inside the number
    too_long,     // more bytes than allowed, or a tenth with more than bit 63
    not_canonical // written longer than it is
};

// Reads one number at `at` into `v`, at most `most_bytes` bytes (10 holds any 64-bit number);
// `at` moves past what was read. Where there is a fault, `v` is not to be used.
inline VarintFault read_varint(std::span<const uint8_t> in, size_t& at, uint64_t& v, int most_bytes = 10)
{
    v = 0;
    for (int k = 0; k < most_bytes && k < 10; ++k)
    {
        if (at >= in.size()) return VarintFault::ends;
        const uint8_t b = in[at++];
        if (k == 9 && b > 1) return VarintFault::too_long; // (bit 63 alone is left for the tenth)
        v |= uint64_t(b & 0x7F) << (7 * k);
        if (!(b & 0x80)) return b == 0 && k > 0 ? VarintFault::not_canonical : VarintFault::none;
    }
    return VarintFault::too_long;
}

inline const char* varint_fault_text(VarintFault f)
{
    switch (f)
    {
    case VarintFault::ends: return "ends inside a number";
    case VarintFault::too_long: return "has a number too long";
    case VarintFault::not_canonical: return "has a number not written as Sieve writes it";
    default: return "";
    }
}

} // namespace sieve
