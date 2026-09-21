#include "sieve/biguint.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace sieve {

BigUint::BigUint(uint64_t v)
{
    while (v)
    {
        limbs_.push_back(static_cast<uint32_t>(v));
        v >>= 32;
    }
}

void BigUint::trim()
{
    while (!limbs_.empty() && limbs_.back() == 0) limbs_.pop_back();
}

void BigUint::mul_small(uint32_t m)
{
    uint64_t carry = 0;
    for (auto& limb : limbs_)
    {
        const uint64_t t = static_cast<uint64_t>(limb) * m + carry;
        limb = static_cast<uint32_t>(t);
        carry = t >> 32;
    }
    if (carry) limbs_.push_back(static_cast<uint32_t>(carry));
    trim();
}

BigUint& BigUint::operator+=(const BigUint& other)
{
    if (other.limbs_.size() > limbs_.size()) limbs_.resize(other.limbs_.size(), 0);
    uint64_t carry = 0;
    for (size_t i = 0; i < limbs_.size(); ++i)
    {
        const uint64_t t = static_cast<uint64_t>(limbs_[i]) + (i < other.limbs_.size() ? other.limbs_[i] : 0) + carry;
        limbs_[i] = static_cast<uint32_t>(t);
        carry = t >> 32;
        if (!carry && i >= other.limbs_.size()) break;
    }
    if (carry) limbs_.push_back(static_cast<uint32_t>(carry));
    return *this;
}

double BigUint::log10_approx() const
{
    if (limbs_.empty()) return -std::numeric_limits<double>::infinity();
    // Use the top three limbs (up to 96 bits) as the mantissa.
    double m = 0.0;
    const size_t n = limbs_.size();
    const size_t take = std::min<size_t>(3, n);
    for (size_t i = 0; i < take; ++i) m = m * 4294967296.0 + limbs_[n - 1 - i];
    return std::log10(m) + static_cast<double>(n - take) * 32.0 * std::log10(2.0);
}

void BigUint::add_small(uint32_t a)
{
    uint64_t carry = a;
    for (size_t i = 0; carry && i < limbs_.size(); ++i)
    {
        const uint64_t t = static_cast<uint64_t>(limbs_[i]) + carry;
        limbs_[i] = static_cast<uint32_t>(t);
        carry = t >> 32;
    }
    if (carry) limbs_.push_back(static_cast<uint32_t>(carry));
}

uint32_t BigUint::divmod_small(uint32_t d)
{
    if (d == 0) throw std::domain_error("division by zero");
    uint64_t rem = 0;
    for (size_t i = limbs_.size(); i-- > 0;)
    {
        const uint64_t cur = (rem << 32) | limbs_[i];
        limbs_[i] = static_cast<uint32_t>(cur / d);
        rem = cur % d;
    }
    trim();
    return static_cast<uint32_t>(rem);
}

BigUint BigUint::from_digits(std::span<const uint32_t> digits, uint32_t base)
{
    if (base < 2) throw std::invalid_argument("base must be at least 2");
    BigUint v;
    for (uint32_t d : digits)
    {
        if (d >= base) throw std::invalid_argument("digit out of range for base");
        v.mul_small(base);
        v.add_small(d);
    }
    return v;
}

std::vector<uint32_t> BigUint::to_digits(uint32_t base, size_t length) const
{
    if (base < 2) throw std::invalid_argument("base must be at least 2");
    std::vector<uint32_t> digits(length, 0);
    BigUint v = *this;
    for (size_t i = length; i-- > 0;)
    {
        if (v.is_zero()) break;
        digits[i] = v.divmod_small(base);
    }
    if (!v.is_zero()) throw std::out_of_range("value does not fit in the requested number of digits");
    return digits;
}

BigUint BigUint::pow(uint32_t base, uint32_t exponent)
{
    BigUint v(1);
    for (uint32_t i = 0; i < exponent; ++i) v.mul_small(base);
    return v;
}

std::string BigUint::to_hex(size_t width) const
{
    static const char* digits = "0123456789abcdef";
    std::string s;
    for (uint32_t limb : limbs_)
        for (int k = 0; k < 8; ++k)
        {
            s.push_back(digits[limb & 0xF]);
            limb >>= 4;
        }
    while (!s.empty() && s.back() == '0') s.pop_back();
    if (width && s.size() > width) throw std::out_of_range("value does not fit in hex width");
    while (s.size() < std::max<size_t>(width, 1)) s.push_back('0');
    std::reverse(s.begin(), s.end());
    return s;
}

BigUint BigUint::from_hex(std::string_view hex)
{
    if (hex.empty()) throw std::invalid_argument("empty hex string");
    BigUint v;
    for (char c : hex)
    {
        uint32_t d;
        if (c >= '0' && c <= '9') d = static_cast<uint32_t>(c - '0');
        else if (c >= 'a' && c <= 'f') d = static_cast<uint32_t>(c - 'a' + 10);
        else if (c >= 'A' && c <= 'F') d = static_cast<uint32_t>(c - 'A' + 10);
        else throw std::invalid_argument(std::string("invalid hex digit '") + c + "'");
        v.mul_small(16);
        v.add_small(d);
    }
    return v;
}

std::string BigUint::to_decimal() const
{
    if (is_zero()) return "0";
    BigUint v = *this;
    std::vector<uint32_t> chunks; // base 1e9, least significant first
    while (!v.is_zero()) chunks.push_back(v.divmod_small(1000000000u));
    std::string s = std::to_string(chunks.back());
    for (size_t i = chunks.size() - 1; i-- > 0;)
    {
        std::string part = std::to_string(chunks[i]);
        s += std::string(9 - part.size(), '0') + part;
    }
    return s;
}

size_t BigUint::bit_length() const
{
    if (limbs_.empty()) return 0;
    uint32_t top = limbs_.back();
    size_t bits = 0;
    while (top) { ++bits; top >>= 1; }
    return (limbs_.size() - 1) * 32 + bits;
}

bool BigUint::is_power_of_two() const
{
    if (limbs_.empty()) return false;
    for (size_t i = 0; i + 1 < limbs_.size(); ++i)
        if (limbs_[i]) return false;
    const uint32_t top = limbs_.back();
    return (top & (top - 1)) == 0;
}

int compare(const BigUint& a, const BigUint& b)
{
    if (a.limbs_.size() != b.limbs_.size()) return a.limbs_.size() < b.limbs_.size() ? -1 : 1;
    for (size_t i = a.limbs_.size(); i-- > 0;)
        if (a.limbs_[i] != b.limbs_[i]) return a.limbs_[i] < b.limbs_[i] ? -1 : 1;
    return 0;
}

} // namespace sieve
