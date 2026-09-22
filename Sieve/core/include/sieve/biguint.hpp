// Sieve — minimal exact unsigned big integer for the address path (SPECIFICATIONS §4.3).
//
// Only the operations addressing needs: conversion to/from base-N digit vectors,
// hexadecimal and decimal text, comparison and bit length. No floating point.
#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace sieve {

class BigUint
{
public:
    BigUint() = default;
    explicit BigUint(uint64_t v);

    // Digits are most-significant first, each < base.
    static BigUint from_digits(std::span<const uint32_t> digits, uint32_t base);
    // The value's own base 2^32 limbs, least significant first: exact and O(n), where building
    // the same value with repeated shift-and-add would be O(n^2).
    static BigUint from_limbs(std::span<const uint32_t> limbs);
    // Returns exactly `length` digits, most-significant first. Throws if the value does not fit.
    std::vector<uint32_t> to_digits(uint32_t base, size_t length) const;

    static BigUint pow(uint32_t base, uint32_t exponent);

    // Lowercase hex, zero-padded to `width` characters. Throws if it does not fit.
    std::string to_hex(size_t width = 0) const;
    // Accepts upper or lower case hex digits; ignores nothing else. Throws on invalid input.
    static BigUint from_hex(std::string_view hex);

    std::string to_decimal() const;
    // Decimal digits only. Throws on anything else.
    static BigUint from_decimal(std::string_view dec);

    size_t bit_length() const;
    bool bit(size_t i) const; // bit i (0 = least significant)
    uint32_t low_bits(unsigned n) const; // value mod 2^n, n <= 32
    bool is_zero() const { return limbs_.empty(); }
    bool is_power_of_two() const;

    // log10 of the value (-inf for zero). DISPLAY ONLY: approximate, never used for addressing.
    double log10_approx() const;

    BigUint& operator+=(const BigUint& other);
    BigUint& operator-=(const BigUint& other); // throws std::underflow_error if other > *this
    BigUint& operator<<=(size_t bits);
    BigUint& operator>>=(size_t bits);          // floor division by 2^bits
    BigUint& operator^=(const BigUint& other);  // bitwise exclusive or
    // value / 2^bits as a double. DISPLAY ONLY (approximate), never used for addressing.
    double ratio_to_power_of_two(size_t bits) const;
    void mul_small(uint32_t m);
    void add_small(uint32_t a);
    uint32_t divmod_small(uint32_t d); // divides in place, returns remainder
    // a mod m for any m >= 1 (a mask when m is a power of two, else long division).
    static BigUint mod(const BigUint& a, const BigUint& m);
    // a * b (schoolbook), and a = q * b + r with 0 <= r < b (Knuth's algorithm D). b >= 1.
    // (q or r may be the same object as a or b.)
    static BigUint mul(const BigUint& a, const BigUint& b);
    static void divmod(const BigUint& a, const BigUint& b, BigUint& q, BigUint& r);

    friend int compare(const BigUint& a, const BigUint& b);
    friend bool operator<(const BigUint& a, const BigUint& b) { return compare(a, b) < 0; }
    friend bool operator>=(const BigUint& a, const BigUint& b) { return compare(a, b) >= 0; }
    friend bool operator<=(const BigUint& a, const BigUint& b) { return compare(a, b) <= 0; }
    friend bool operator>(const BigUint& a, const BigUint& b) { return compare(a, b) > 0; }
    friend bool operator==(const BigUint& a, const BigUint& b) { return compare(a, b) == 0; }

private:
    void trim();
    std::vector<uint32_t> limbs_; // little-endian base 2^32, no trailing zero limbs
};

} // namespace sieve
