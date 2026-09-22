#include "sieve/biguint.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>
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

namespace {

// log2(base) if base is a power of two (2, 4, ..., 2^31), else 0.
unsigned pow2_shift(uint32_t base)
{
    if (base < 2 || (base & (base - 1))) return 0;
    unsigned s = 0;
    while ((1u << s) != base) ++s;
    return s;
}

// The largest k with base^k <= 2^32, and base^k itself (as uint64_t, may equal 2^32).
std::pair<size_t, uint64_t> chunk_of(uint32_t base)
{
    size_t k = 1;
    uint64_t p = base;
    while (p * base <= (uint64_t(1) << 32)) { p *= base; ++k; }
    return {k, p};
}

} // namespace

BigUint BigUint::from_limbs(std::span<const uint32_t> limbs)
{
    BigUint v;
    v.limbs_.assign(limbs.begin(), limbs.end());
    v.trim();
    return v;
}

BigUint BigUint::from_digits(std::span<const uint32_t> digits, uint32_t base)
{
    if (base < 2) throw std::invalid_argument("base must be at least 2");
    if (const unsigned sh = pow2_shift(base))
    {
        // A power-of-two base: the digits are the bits, placed directly (linear time).
        BigUint v;
        v.limbs_.assign((digits.size() * sh + 31) / 32, 0);
        size_t bit = 0;
        for (size_t i = digits.size(); i-- > 0; bit += sh)
        {
            const uint32_t d = digits[i];
            if (d >= base) throw std::invalid_argument("digit out of range for base");
            const size_t w = bit / 32, o = bit % 32;
            v.limbs_[w] |= d << o;
            if (o + sh > 32) v.limbs_[w + 1] |= d >> (32 - o);
        }
        v.trim();
        return v;
    }
    // Horner's rule, k digits per big multiply: v = v * base^k + (next k digits).
    const size_t k = chunk_of(base).first;
    BigUint v;
    for (size_t i = 0; i < digits.size();)
    {
        const size_t take = std::min(k, digits.size() - i);
        uint64_t chunk = 0, scale = 1;
        for (size_t j = 0; j < take; ++j, ++i)
        {
            if (digits[i] >= base) throw std::invalid_argument("digit out of range for base");
            chunk = chunk * base + digits[i];
            scale *= base;
        }
        if (scale == (uint64_t(1) << 32))
            v.limbs_.insert(v.limbs_.begin(), 0u); // multiply by exactly 2^32
        else
            v.mul_small(static_cast<uint32_t>(scale));
        v.add_small(static_cast<uint32_t>(chunk));
        v.trim();
    }
    return v;
}

std::vector<uint32_t> BigUint::to_digits(uint32_t base, size_t length) const
{
    if (base < 2) throw std::invalid_argument("base must be at least 2");
    std::vector<uint32_t> digits(length, 0);
    if (const unsigned sh = pow2_shift(base))
    {
        // A power-of-two base: read the bits directly (linear time).
        if (bit_length() > uint64_t(length) * sh) throw std::out_of_range("value does not fit in the requested number of digits");
        const uint32_t mask = base - 1;
        size_t bit = 0;
        for (size_t i = length; i-- > 0; bit += sh)
        {
            const size_t w = bit / 32, o = bit % 32;
            if (w >= limbs_.size()) break;
            uint64_t word = limbs_[w];
            if (w + 1 < limbs_.size()) word |= uint64_t(limbs_[w + 1]) << 32;
            digits[i] = uint32_t(word >> o) & mask;
        }
        return digits;
    }
    BigUint v = *this;
    // Peel k digits per big division, using the largest base^k that fits a 32-bit divisor.
    auto [k, full] = chunk_of(base);
    if (full == (uint64_t(1) << 32)) { --k; full /= base; }
    for (size_t i = length; i > 0 && !v.is_zero();)
    {
        uint32_t rem = v.divmod_small(static_cast<uint32_t>(full));
        for (size_t j = 0; j < k && i > 0; ++j)
        {
            digits[--i] = rem % base;
            rem /= base;
        }
        if (rem) throw std::out_of_range("value does not fit in the requested number of digits");
    }
    if (!v.is_zero()) throw std::out_of_range("value does not fit in the requested number of digits");
    return digits;
}

BigUint BigUint::pow(uint32_t base, uint32_t exponent)
{
    if (base < 2) return BigUint(base == 0 && exponent > 0 ? 0 : 1); // 0^0 = 1
    BigUint v(1);
    if (const unsigned sh = pow2_shift(base))
    {
        v <<= size_t(exponent) * sh; // 2^(sh * exponent)
        return v;
    }
    const auto [k, full] = chunk_of(base);
    uint32_t e = exponent;
    if (full < (uint64_t(1) << 32))
        for (; e >= k; e -= static_cast<uint32_t>(k)) v.mul_small(static_cast<uint32_t>(full));
    for (; e > 0; --e) v.mul_small(base);
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
    // Eight hex digits per limb, from the least significant end (linear time).
    BigUint v;
    v.limbs_.assign((hex.size() + 7) / 8, 0);
    for (size_t i = 0; i < hex.size(); ++i)
    {
        const char c = hex[hex.size() - 1 - i];
        uint32_t d;
        if (c >= '0' && c <= '9') d = static_cast<uint32_t>(c - '0');
        else if (c >= 'a' && c <= 'f') d = static_cast<uint32_t>(c - 'a' + 10);
        else if (c >= 'A' && c <= 'F') d = static_cast<uint32_t>(c - 'A' + 10);
        else throw std::invalid_argument(std::string("invalid hex digit '") + c + "'");
        v.limbs_[i / 8] |= d << (4 * (i % 8));
    }
    v.trim();
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

bool BigUint::bit(size_t i) const
{
    const size_t limb = i / 32;
    return limb < limbs_.size() && ((limbs_[limb] >> (i % 32)) & 1u) != 0;
}

uint32_t BigUint::low_bits(unsigned n) const
{
    if (n > 32) throw std::invalid_argument("low_bits takes at most 32 bits");
    const uint32_t v = limbs_.empty() ? 0 : limbs_[0];
    return n == 32 ? v : v & ((1u << n) - 1);
}

BigUint BigUint::from_decimal(std::string_view dec)
{
    if (dec.empty()) throw std::invalid_argument("empty decimal number");
    BigUint v;
    for (char c : dec)
    {
        if (c < '0' || c > '9') throw std::invalid_argument("invalid decimal digit '" + std::string(1, c) + "'");
        v.mul_small(10);
        v.add_small(uint32_t(c - '0'));
    }
    return v;
}

BigUint BigUint::mod(const BigUint& a, const BigUint& m)
{
    if (m.is_zero()) throw std::domain_error("modulo by zero");
    if (a < m) return a;
    if (m.is_power_of_two())
    {
        const size_t bits = m.bit_length() - 1;
        BigUint r = a;
        r.limbs_.resize(std::min(r.limbs_.size(), (bits + 31) / 32));
        if (bits % 32 && !r.limbs_.empty() && r.limbs_.size() == (bits + 31) / 32) r.limbs_.back() &= (1u << (bits % 32)) - 1;
        r.trim();
        return r;
    }
    // a < 2m (the common case: a sum of two residues, a position one loop past the end).
    {
        BigUint once = a;
        once -= m;
        if (once < m) return once;
    }
    BigUint q, r;
    divmod(a, m, q, r);
    return r;
}

BigUint BigUint::mul(const BigUint& a, const BigUint& b)
{
    if (a.is_zero() || b.is_zero()) return BigUint();
    BigUint p;
    p.limbs_.assign(a.limbs_.size() + b.limbs_.size(), 0);
    for (size_t i = 0; i < a.limbs_.size(); ++i)
    {
        uint64_t carry = 0;
        const uint64_t x = a.limbs_[i];
        for (size_t j = 0; j < b.limbs_.size(); ++j)
        {
            const uint64_t t = x * b.limbs_[j] + p.limbs_[i + j] + carry;
            p.limbs_[i + j] = static_cast<uint32_t>(t);
            carry = t >> 32;
        }
        size_t k = i + b.limbs_.size();
        while (carry)
        {
            const uint64_t t = uint64_t(p.limbs_[k]) + carry;
            p.limbs_[k++] = static_cast<uint32_t>(t);
            carry = t >> 32;
        }
    }
    p.trim();
    return p;
}

void BigUint::divmod(const BigUint& a, const BigUint& b, BigUint& q, BigUint& r)
{
    if (&q == &a || &q == &b || &r == &a || &r == &b || &q == &r)
    {
        // Outputs sharing storage with the inputs: work on copies.
        BigUint qq, rr;
        divmod(BigUint(a), BigUint(b), qq, rr);
        q = std::move(qq);
        r = std::move(rr);
        return;
    }
    if (b.is_zero()) throw std::domain_error("division by zero");
    if (a < b)
    {
        q = BigUint();
        r = a;
        return;
    }
    if (b.limbs_.size() == 1)
    {
        q = a;
        r = BigUint(q.divmod_small(b.limbs_[0]));
        return;
    }
    // Knuth, TAOCP vol. 2, 4.3.1, algorithm D, with 32-bit digits.
    const size_t n = b.limbs_.size(), m = a.limbs_.size() - n;
    unsigned s = 0;
    for (uint32_t top = b.limbs_.back(); !(top & 0x80000000u); top <<= 1) ++s;
    // Normalise: shift so the divisor's top bit is set.
    std::vector<uint32_t> v(n), u(a.limbs_.size() + 1);
    for (size_t i = n; i-- > 0;)
        v[i] = (b.limbs_[i] << s) | (s && i ? b.limbs_[i - 1] >> (32 - s) : 0);
    u[a.limbs_.size()] = s ? a.limbs_.back() >> (32 - s) : 0;
    for (size_t i = a.limbs_.size(); i-- > 0;)
        u[i] = (a.limbs_[i] << s) | (s && i ? a.limbs_[i - 1] >> (32 - s) : 0);
    std::vector<uint32_t> qd(m + 1, 0);
    const uint64_t B = uint64_t(1) << 32;
    for (size_t j = m + 1; j-- > 0;)
    {
        // Estimate the quotient digit from the top two digits, then correct it.
        const uint64_t num = (uint64_t(u[j + n]) << 32) | u[j + n - 1];
        uint64_t qhat = num / v[n - 1], rhat = num % v[n - 1];
        while (qhat >= B || qhat * v[n - 2] > ((rhat << 32) | u[j + n - 2]))
        {
            --qhat;
            rhat += v[n - 1];
            if (rhat >= B) break;
        }
        // Multiply and subtract.
        int64_t borrow = 0;
        uint64_t carry = 0;
        for (size_t i = 0; i < n; ++i)
        {
            const uint64_t p = qhat * v[i] + carry;
            carry = p >> 32;
            const int64_t t = int64_t(u[i + j]) - int64_t(p & 0xFFFFFFFFu) + borrow;
            u[i + j] = static_cast<uint32_t>(t);
            borrow = t >> 32;
        }
        const int64_t t = int64_t(u[j + n]) - int64_t(carry) + borrow;
        u[j + n] = static_cast<uint32_t>(t);
        if (t < 0)
        {
            // qhat was one too large: add the divisor back.
            --qhat;
            uint64_t c = 0;
            for (size_t i = 0; i < n; ++i)
            {
                const uint64_t sum = uint64_t(u[i + j]) + v[i] + c;
                u[i + j] = static_cast<uint32_t>(sum);
                c = sum >> 32;
            }
            u[j + n] = static_cast<uint32_t>(uint64_t(u[j + n]) + c);
        }
        qd[j] = static_cast<uint32_t>(qhat);
    }
    q.limbs_ = std::move(qd);
    q.trim();
    // Unnormalise the remainder.
    r.limbs_.assign(n, 0);
    for (size_t i = 0; i < n; ++i) r.limbs_[i] = (u[i] >> s) | (s && i + 1 < u.size() ? u[i + 1] << (32 - s) : 0);
    r.trim();
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

BigUint& BigUint::operator-=(const BigUint& other)
{
    if (compare(*this, other) < 0) throw std::underflow_error("BigUint subtraction would go below zero");
    int64_t borrow = 0;
    for (size_t i = 0; i < limbs_.size(); ++i)
    {
        int64_t t = int64_t(limbs_[i]) - borrow - (i < other.limbs_.size() ? int64_t(other.limbs_[i]) : 0);
        borrow = t < 0 ? 1 : 0;
        if (t < 0) t += int64_t(1) << 32;
        limbs_[i] = static_cast<uint32_t>(t);
        if (!borrow && i >= other.limbs_.size()) break;
    }
    trim();
    return *this;
}

BigUint& BigUint::operator<<=(size_t bits)
{
    if (limbs_.empty() || bits == 0) return *this;
    const size_t whole = bits / 32, part = bits % 32;
    if (part)
    {
        uint32_t carry = 0;
        for (auto& limb : limbs_)
        {
            const uint32_t next = limb >> (32 - part);
            limb = (limb << part) | carry;
            carry = next;
        }
        if (carry) limbs_.push_back(carry);
    }
    limbs_.insert(limbs_.begin(), whole, 0u);
    return *this;
}

BigUint& BigUint::operator^=(const BigUint& other)
{
    if (limbs_.size() < other.limbs_.size()) limbs_.resize(other.limbs_.size(), 0);
    for (size_t i = 0; i < other.limbs_.size(); ++i) limbs_[i] ^= other.limbs_[i];
    trim();
    return *this;
}

BigUint& BigUint::operator>>=(size_t bits)
{
    const size_t whole = bits / 32, part = bits % 32;
    if (whole >= limbs_.size()) { limbs_.clear(); return *this; }
    limbs_.erase(limbs_.begin(), limbs_.begin() + static_cast<std::ptrdiff_t>(whole));
    if (part)
    {
        for (size_t i = 0; i < limbs_.size(); ++i)
        {
            const uint32_t hi = i + 1 < limbs_.size() ? limbs_[i + 1] : 0;
            limbs_[i] = (limbs_[i] >> part) | (hi << (32 - part));
        }
    }
    trim();
    return *this;
}

double BigUint::ratio_to_power_of_two(size_t bits) const
{
    if (limbs_.empty()) return 0.0;
    // Top 64 bits as the mantissa is ample for a double.
    const size_t len = bit_length();
    BigUint top = *this;
    size_t dropped = 0;
    if (len > 64) { dropped = len - 64; top >>= dropped; }
    double m = 0.0;
    for (size_t i = top.limbs_.size(); i-- > 0;) m = m * 4294967296.0 + top.limbs_[i];
    return std::ldexp(m, static_cast<int>(dropped) - static_cast<int>(bits));
}

int compare(const BigUint& a, const BigUint& b)
{
    if (a.limbs_.size() != b.limbs_.size()) return a.limbs_.size() < b.limbs_.size() ? -1 : 1;
    for (size_t i = a.limbs_.size(); i-- > 0;)
        if (a.limbs_[i] != b.limbs_[i]) return a.limbs_[i] < b.limbs_[i] ? -1 : 1;
    return 0;
}

} // namespace sieve
