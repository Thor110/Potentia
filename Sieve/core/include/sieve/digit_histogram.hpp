// Sieve — digits numbered by their histograms (`digit-histogram-v1`, SPECIFICATIONS §12.2): an
// ordering of a line whose items are rows of digits (the AI line's models), under which the items
// whose digits are unevenly spread come first and have the shorter numbers.
//
// The digits are cut into segments (the AI line's tensors, in address order), each of n digits
// below K. A segment is named by two numbers:
//   its histogram, c_0 .. c_{K-1} (how many of each digit), as its rank among the C(n + K - 1, K - 1)
//     histograms of n digits, in lexicographic order of (c_0, c_1, ...);
//   its digits, as their rank among the n! / (c_0! ... c_{K-1}!) rows with that histogram, in
//     lexicographic order of the digits, the first most significant.
// The histograms' ranks, the first segment's most significant, make one number A below the product
// of every segment's count of histograms, C; the digits' ranks likewise make B below the product of
// their counts, M (which the histograms fix). An item's number is Z = B * C + A.
//
// Every item has one number, and a number names at most one item (none when its B is past the
// rows its A's histograms allow): the histograms are numbered on their own rather than by how many
// rows come before them, which no machine could add up, and that costs C's bits. It is not shorter for
// every item: a row of digits spread as evenly as chance spreads them costs a little more than its
// place (C's bits), and one trained into an uneven spread less. A listing uses it for a line only
// where it is smaller (sieve-manifest-v7). Exact: integers throughout, the same on every machine.
#pragma once

#include "sieve/biguint.hpp"

#include <cstdint>
#include <span>
#include <vector>

namespace sieve {

inline constexpr const char* kDigitHistogramVersion = "digit-histogram-v1";

// Z for `digits` cut into `segments` (their lengths, summing to digits.size()), each digit below K
// (2 to 256). Throws std::invalid_argument on a digit out of range or lengths that do not add up.
BigUint digit_histogram_number(std::span<const uint8_t> digits, std::span<const uint64_t> segments, uint32_t K);

// The digits Z names. Throws std::invalid_argument if Z names none (it is too large for its
// histograms).
std::vector<uint8_t> digit_histogram_digits(const BigUint& z, std::span<const uint64_t> segments, uint32_t K);

// C(a, b) exactly, for b small (b <= a, else 0).
BigUint small_binomial(uint64_t a, uint32_t b);

} // namespace sieve
