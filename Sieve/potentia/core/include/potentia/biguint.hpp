// Potentia — minimal exact unsigned big integer for the address path (SPECIFICATIONS §4.3).
//
// Only the operations addressing needs: conversion to/from base-N digit vectors,
// hexadecimal and decimal text, comparison and bit length. No floating point.
#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace potentia {

class BigUint
{
public:
    BigUint() = default;
    explicit BigUint(uint64_t v);

    // Digits are most-significant first, each < base.
    static BigUint from_digits(std::span<const uint32_t> digits, uint32_t base);
    // Returns exactly `length` digits, most-significant first. Throws if the value does not fit.
    std::vector<uint32_t> to_digits(uint32_t base, size_t length) const;

    static BigUint pow(uint32_t base, uint32_t exponent);

    // Lowercase hex, zero-padded to `width` characters. Throws if it does not fit.
    std::string to_hex(size_t width = 0) const;
    // Accepts upper or lower case hex digits; ignores nothing else. Throws on invalid input.
    static BigUint from_hex(std::string_view hex);

    std::string to_decimal() const;

    size_t bit_length() const;
    bool is_zero() const { return limbs_.empty(); }
    bool is_power_of_two() const;

    // log10 of the value (-inf for zero). DISPLAY ONLY: approximate, never used for addressing.
    double log10_approx() const;

    BigUint& operator+=(const BigUint& other);
    void mul_small(uint32_t m);
    void add_small(uint32_t a);
    uint32_t divmod_small(uint32_t d); // divides in place, returns remainder

    friend int compare(const BigUint& a, const BigUint& b);
    friend bool operator<(const BigUint& a, const BigUint& b) { return compare(a, b) < 0; }
    friend bool operator>=(const BigUint& a, const BigUint& b) { return compare(a, b) >= 0; }
    friend bool operator==(const BigUint& a, const BigUint& b) { return compare(a, b) == 0; }

private:
    void trim();
    std::vector<uint32_t> limbs_; // little-endian base 2^32, no trailing zero limbs
};

} // namespace potentia
