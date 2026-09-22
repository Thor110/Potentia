// Sieve — exact fixed-point base-2 logarithms, for filters that must agree bit for bit on every
// machine (no floating point in any pass/fail decision).
//
// log2_q16(x) = floor(log2(x) * 2^16) for x >= 1, computed with integers only:
//   n = floor(log2 x); m = x / 2^n in fixed point (Q62, in [1, 2)); then 16 rounds of
//   m = m^2 (Q62, rounded down); if m >= 2: m /= 2 and emit bit 1, else emit bit 0.
// The reference oracle implements the same steps.
#pragma once

#include <cstdint>

namespace sieve {

inline constexpr unsigned kLogFractionBits = 16;
inline constexpr int64_t kLogOne = int64_t(1) << kLogFractionBits; // 1.0 in Q16

int64_t log2_q16(uint64_t x); // x >= 1

} // namespace sieve
