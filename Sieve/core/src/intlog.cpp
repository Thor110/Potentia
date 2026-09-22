#include "sieve/intlog.hpp"

#include <stdexcept>

namespace sieve {

int64_t log2_q16(uint64_t x)
{
    if (x == 0) throw std::domain_error("log2 of zero");
    int n = 63;
    while (!((x >> n) & 1)) --n;
    // Mantissa in Q62: x / 2^n, in [1, 2).
    constexpr int kQ = 62;
    uint64_t m = n <= kQ ? x << (kQ - n) : x >> (n - kQ);
    int64_t result = int64_t(n) << kLogFractionBits;
    for (unsigned i = 0; i < kLogFractionBits; ++i)
    {
        // m^2 in Q62, rounded down: (m * m) >> 62, via 32-bit halves.
        const uint64_t hi = m >> 31, lo = m & 0x7FFFFFFFu; // m = hi * 2^31 + lo
        const uint64_t hh = hi * hi, hl = hi * lo, ll = lo * lo;
        // m^2 = hh*2^62 + 2*hl*2^31 + ll; divide by 2^62.
        uint64_t sq = hh + ((2 * hl) >> 31) + (((((2 * hl) & 0x7FFFFFFFu) << 31) + ll) >> 62);
        if (sq >= (uint64_t(2) << kQ)) // >= 2.0
        {
            sq >>= 1;
            result += int64_t(1) << (kLogFractionBits - 1 - i);
        }
        m = sq;
    }
    return result;
}

} // namespace sieve
