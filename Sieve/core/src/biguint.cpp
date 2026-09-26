#include "sieve/biguint.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>
#include <stdexcept>

namespace sieve {
namespace {

// ---------------------------------------------------------------- wide arithmetic
//
// A limb is 64 bits, so a product of two limbs is 128 and a division takes a 128-bit numerator.
// Where the compiler offers a 128-bit integer that is used directly; where it does not, the same
// answers are worked out from 32-bit halves with 64-bit intermediates, which every C++ compiler
// has. Both paths are exercised by the conformance vectors (tests/vectors_biguint_v1.tsv), and
// SIEVE_BIGUINT_PORTABLE forces the second one so it can be tested where the first exists.

constexpr uint64_t kLimbBits = 64;
constexpr uint64_t kHalf = uint64_t(1) << 32;
constexpr uint64_t kHalfMask = kHalf - 1;

struct Wide
{
    uint64_t hi, lo;
};

// a * b as 128 bits.
inline Wide mul_wide(uint64_t a, uint64_t b)
{
#if defined(__SIZEOF_INT128__) && !defined(SIEVE_BIGUINT_PORTABLE)
    const __uint128_t p = static_cast<__uint128_t>(a) * b;
    return {static_cast<uint64_t>(p >> 64), static_cast<uint64_t>(p)};
#else
    const uint64_t a0 = a & kHalfMask, a1 = a >> 32, b0 = b & kHalfMask, b1 = b >> 32;
    const uint64_t p00 = a0 * b0, p01 = a0 * b1, p10 = a1 * b0, p11 = a1 * b1;
    const uint64_t mid = (p00 >> 32) + (p01 & kHalfMask) + (p10 & kHalfMask);
    return {p11 + (p01 >> 32) + (p10 >> 32) + (mid >> 32), (mid << 32) | (p00 & kHalfMask)};
#endif
}

// x * y + a + b as 128 bits. It cannot overflow: the largest product is 2^128 - 2^65 + 1, and
// two more limbs add at most 2^65 - 2. This is the shape of every inner loop below, so having
// the carry fall out of the top half rather than being detected with comparisons is most of
// what makes wide multiplication worth doing at all.
inline Wide mul_add2(uint64_t x, uint64_t y, uint64_t a, uint64_t b)
{
#if defined(__SIZEOF_INT128__) && !defined(SIEVE_BIGUINT_PORTABLE)
    const __uint128_t t = static_cast<__uint128_t>(x) * y + a + b;
    return {static_cast<uint64_t>(t >> 64), static_cast<uint64_t>(t)};
#else
    Wide p = mul_wide(x, y);
    p.lo += a;
    if (p.lo < a) ++p.hi;
    p.lo += b;
    if (p.lo < b) ++p.hi;
    return p;
#endif
}

// (hi:lo) / d, with hi < d and d's top bit set. Returns the quotient; the remainder goes in *rem.
// Both preconditions hold everywhere this is used (Knuth's algorithm D normalises the divisor).
inline uint64_t div_wide(uint64_t hi, uint64_t lo, uint64_t d, uint64_t* rem)
{
#if defined(__SIZEOF_INT128__) && !defined(SIEVE_BIGUINT_PORTABLE)
    const __uint128_t n = (static_cast<__uint128_t>(hi) << 64) | lo;
    *rem = static_cast<uint64_t>(n % d);
    return static_cast<uint64_t>(n / d);
#else
    // Knuth 4.3.1 algorithm D on 32-bit halves. The sums below are meant to wrap: unsigned
    // overflow is defined, and the value being computed is known to fit in 64 bits.
    const uint64_t d1 = d >> 32, d0 = d & kHalfMask;
    const uint64_t u1 = lo >> 32, u0 = lo & kHalfMask;
    uint64_t q1 = hi / d1, rhat = hi - q1 * d1;
    while (q1 >= kHalf || q1 * d0 > kHalf * rhat + u1)
    {
        --q1;
        rhat += d1;
        if (rhat >= kHalf) break;
    }
    const uint64_t n21 = hi * kHalf + u1 - q1 * d;
    uint64_t q0 = n21 / d1;
    rhat = n21 - q0 * d1;
    while (q0 >= kHalf || q0 * d0 > kHalf * rhat + u0)
    {
        --q0;
        rhat += d1;
        if (rhat >= kHalf) break;
    }
    *rem = n21 * kHalf + u0 - q0 * d;
    return q1 * kHalf + q0;
#endif
}

// log2(base) if base is a power of two (2, 4, ..., 2^31), else 0.
unsigned pow2_shift(uint32_t base)
{
    if (base < 2 || (base & (base - 1))) return 0;
    unsigned s = 0;
    while ((1u << s) != base) ++s;
    return s;
}

// The largest k with base^k < 2^64, and base^k itself. Only used for bases that are not powers
// of two (those take the direct bit paths), so base^k never lands exactly on 2^64.
std::pair<size_t, uint64_t> chunk_of(uint32_t base)
{
    size_t k = 1;
    uint64_t p = base;
    while (p <= (std::numeric_limits<uint64_t>::max)() / base)
    {
        p *= base;
        ++k;
    }
    return {k, p};
}

} // namespace

BigUint::BigUint(uint64_t v)
{
    if (v) limbs_.push_back(v);
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
        const Wide t = mul_add2(limb, m, carry, 0);
        limb = t.lo;
        carry = t.hi;
    }
    if (carry) limbs_.push_back(carry);
    trim();
}

BigUint& BigUint::operator+=(const BigUint& other)
{
    if (other.limbs_.size() > limbs_.size()) limbs_.resize(other.limbs_.size(), 0);
    uint64_t carry = 0;
    for (size_t i = 0; i < limbs_.size(); ++i)
    {
        const uint64_t add = i < other.limbs_.size() ? other.limbs_[i] : 0;
        uint64_t t = limbs_[i] + add;
        uint64_t next = t < add ? 1 : 0;
        t += carry;
        if (t < carry) next = 1;
        limbs_[i] = t;
        carry = next;
        if (!carry && i >= other.limbs_.size()) break;
    }
    if (carry) limbs_.push_back(carry);
    return *this;
}

double BigUint::log10_approx() const
{
    if (limbs_.empty()) return -std::numeric_limits<double>::infinity();
    // The top two limbs (up to 128 bits) are ample as a mantissa.
    double m = 0.0;
    const size_t n = limbs_.size();
    const size_t take = std::min<size_t>(2, n);
    for (size_t i = 0; i < take; ++i) m = m * 18446744073709551616.0 + double(limbs_[n - 1 - i]);
    return std::log10(m) + double(n - take) * 64.0 * std::log10(2.0);
}

void BigUint::add_small(uint32_t a)
{
    uint64_t carry = a;
    for (size_t i = 0; carry && i < limbs_.size(); ++i)
    {
        const uint64_t t = limbs_[i] + carry;
        carry = t < carry ? 1 : 0;
        limbs_[i] = t;
    }
    if (carry) limbs_.push_back(carry);
}

uint32_t BigUint::divmod_small(uint32_t d)
{
    if (d == 0) throw std::domain_error("division by zero");
    // The divisor is 32 bits and the running remainder is smaller still, so each limb can be
    // taken in two 64-by-32 steps and no 128-bit division is needed.
    uint64_t rem = 0;
    for (size_t i = limbs_.size(); i-- > 0;)
    {
        const uint64_t hi = (rem << 32) | (limbs_[i] >> 32);
        const uint64_t qhi = hi / d;
        const uint64_t lo = ((hi % d) << 32) | (limbs_[i] & kHalfMask);
        limbs_[i] = (qhi << 32) | (lo / d);
        rem = lo % d;
    }
    trim();
    return static_cast<uint32_t>(rem);
}

BigUint BigUint::from_limbs(std::span<const uint32_t> limbs)
{
    // The interface is 32-bit (SHA-256 gives its digest that way); a limb holds two of them.
    BigUint v;
    v.limbs_.assign((limbs.size() + 1) / 2, 0);
    for (size_t i = 0; i < limbs.size(); ++i) v.limbs_[i / 2] |= uint64_t(limbs[i]) << (32 * (i % 2));
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
        v.limbs_.assign((digits.size() * sh + kLimbBits - 1) / kLimbBits, 0);
        size_t bit = 0;
        for (size_t i = digits.size(); i-- > 0; bit += sh)
        {
            const uint32_t d = digits[i];
            if (d >= base) throw std::invalid_argument("digit out of range for base");
            const size_t w = bit / kLimbBits, o = bit % kLimbBits;
            v.limbs_[w] |= uint64_t(d) << o;
            if (o + sh > kLimbBits) v.limbs_[w + 1] |= uint64_t(d) >> (kLimbBits - o);
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
        v.mul_limb(scale);
        v.add_limb(chunk);
        v.trim();
    }
    return v;
}

// The two whole-limb helpers the base conversions need. They are not part of the public
// interface: outside this file a small operand is a uint32_t, as it always was.
void BigUint::mul_limb(uint64_t m)
{
    if (m == 0) { limbs_.clear(); return; }
    uint64_t carry = 0;
    for (auto& limb : limbs_)
    {
        const Wide t = mul_add2(limb, m, carry, 0);
        limb = t.lo;
        carry = t.hi;
    }
    if (carry) limbs_.push_back(carry);
    trim();
}

void BigUint::add_limb(uint64_t a)
{
    uint64_t carry = a;
    for (size_t i = 0; carry && i < limbs_.size(); ++i)
    {
        const uint64_t t = limbs_[i] + carry;
        carry = t < carry ? 1 : 0;
        limbs_[i] = t;
    }
    if (carry) limbs_.push_back(carry);
}

// Divide in place by a whole limb, returning the remainder. div_wide wants a normalised
// divisor, so where the divisor's top bit is clear the value is shifted up to meet it and the
// remainder shifted back down at the end.
uint64_t BigUint::divmod_limb(uint64_t d)
{
    if (d == 0) throw std::domain_error("division by zero");
    unsigned s = 0;
    for (uint64_t top = d; !(top & (uint64_t(1) << 63)); top <<= 1) ++s;
    if (s == 0)
    {
        uint64_t rem = 0;
        for (size_t i = limbs_.size(); i-- > 0;) limbs_[i] = div_wide(rem, limbs_[i], d, &rem);
        trim();
        return rem;
    }
    std::vector<uint64_t> u(limbs_.size() + 1, 0);
    for (size_t i = 0; i < limbs_.size(); ++i)
    {
        u[i] |= limbs_[i] << s;
        u[i + 1] = limbs_[i] >> (kLimbBits - s);
    }
    std::vector<uint64_t> q(u.size(), 0);
    uint64_t rem = 0;
    for (size_t i = u.size(); i-- > 0;) q[i] = div_wide(rem, u[i], d << s, &rem);
    limbs_ = std::move(q);
    trim();
    return rem >> s;
}

std::vector<uint32_t> BigUint::to_digits(uint32_t base, size_t length) const
{
    if (base < 2) throw std::invalid_argument("base must be at least 2");
    std::vector<uint32_t> digits(length, 0);
    if (const unsigned sh = pow2_shift(base))
    {
        // A power-of-two base: read the bits directly (linear time).
        if (bit_length() > uint64_t(length) * sh)
            throw std::out_of_range("value does not fit in the requested number of digits");
        const uint32_t mask = base - 1;
        size_t bit = 0;
        for (size_t i = length; i-- > 0; bit += sh)
        {
            const size_t w = bit / kLimbBits, o = bit % kLimbBits;
            if (w >= limbs_.size()) break;
            uint64_t word = limbs_[w] >> o;
            if (o && w + 1 < limbs_.size()) word |= limbs_[w + 1] << (kLimbBits - o);
            digits[i] = uint32_t(word) & mask;
        }
        return digits;
    }
    BigUint v = *this;
    // Peel k digits per big division, using the largest base^k that fits a limb.
    const auto [k, full] = chunk_of(base);
    for (size_t i = length; i > 0 && !v.is_zero();)
    {
        uint64_t rem = v.divmod_limb(full);
        for (size_t j = 0; j < k && i > 0; ++j)
        {
            digits[--i] = uint32_t(rem % base);
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
    for (; e >= k; e -= static_cast<uint32_t>(k)) v.mul_limb(full);
    for (; e > 0; --e) v.mul_small(base);
    return v;
}

std::string BigUint::to_hex(size_t width) const
{
    static const char* digits = "0123456789abcdef";
    std::string s;
    for (uint64_t limb : limbs_)
        for (int k = 0; k < 16; ++k)
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
    // Sixteen hex digits per limb, from the least significant end (linear time).
    BigUint v;
    v.limbs_.assign((hex.size() + 15) / 16, 0);
    for (size_t i = 0; i < hex.size(); ++i)
    {
        const char c = hex[hex.size() - 1 - i];
        uint64_t d;
        if (c >= '0' && c <= '9') d = uint64_t(c - '0');
        else if (c >= 'a' && c <= 'f') d = uint64_t(c - 'a' + 10);
        else if (c >= 'A' && c <= 'F') d = uint64_t(c - 'A' + 10);
        else throw std::invalid_argument(std::string("invalid hex digit '") + c + "'");
        v.limbs_[i / 16] |= d << (4 * (i % 16));
    }
    v.trim();
    return v;
}

std::string BigUint::to_decimal() const
{
    if (is_zero()) return "0";
    BigUint v = *this;
    // Base 10^19, the largest power of ten that fits a limb: one big division per 19 digits.
    constexpr uint64_t kChunk = 10000000000000000000ull;
    std::vector<uint64_t> chunks; // least significant first
    while (!v.is_zero()) chunks.push_back(v.divmod_limb(kChunk));
    std::string s = std::to_string(chunks.back());
    for (size_t i = chunks.size() - 1; i-- > 0;)
    {
        std::string part = std::to_string(chunks[i]);
        s += std::string(19 - part.size(), '0') + part;
    }
    return s;
}

bool BigUint::bit(size_t i) const
{
    const size_t limb = i / kLimbBits;
    return limb < limbs_.size() && ((limbs_[limb] >> (i % kLimbBits)) & 1u) != 0;
}

uint32_t BigUint::low_bits(unsigned n) const
{
    if (n > 32) throw std::invalid_argument("low_bits takes at most 32 bits");
    const uint32_t v = limbs_.empty() ? 0 : uint32_t(limbs_[0] & kHalfMask);
    return n == 32 ? v : v & ((1u << n) - 1);
}

BigUint BigUint::from_decimal(std::string_view dec)
{
    if (dec.empty()) throw std::invalid_argument("empty decimal number");
    BigUint v;
    // Nineteen digits per big multiply, for the same reason to_decimal takes nineteen at a time.
    size_t i = 0;
    while (i < dec.size())
    {
        const size_t take = std::min<size_t>(19, dec.size() - i);
        uint64_t chunk = 0, scale = 1;
        for (size_t j = 0; j < take; ++j, ++i)
        {
            const char c = dec[i];
            if (c < '0' || c > '9') throw std::invalid_argument("invalid decimal digit '" + std::string(1, c) + "'");
            chunk = chunk * 10 + uint64_t(c - '0');
            scale *= 10;
        }
        v.mul_limb(scale);
        v.add_limb(chunk);
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
        r.limbs_.resize(std::min(r.limbs_.size(), (bits + kLimbBits - 1) / kLimbBits));
        if (bits % kLimbBits && !r.limbs_.empty() && r.limbs_.size() == (bits + kLimbBits - 1) / kLimbBits)
            r.limbs_.back() &= (uint64_t(1) << (bits % kLimbBits)) - 1;
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
            const Wide t = mul_add2(x, b.limbs_[j], p.limbs_[i + j], carry);
            p.limbs_[i + j] = t.lo;
            carry = t.hi;
        }
        size_t k = i + b.limbs_.size();
        while (carry)
        {
            const uint64_t t = p.limbs_[k] + carry;
            carry = t < carry ? 1 : 0;
            p.limbs_[k++] = t;
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
        r = BigUint(q.divmod_limb(b.limbs_[0]));
        return;
    }
    // Knuth, TAOCP vol. 2, 4.3.1, algorithm D, with 64-bit digits.
    const size_t n = b.limbs_.size(), m = a.limbs_.size() - n;
    unsigned s = 0;
    for (uint64_t top = b.limbs_.back(); !(top & (uint64_t(1) << 63)); top <<= 1) ++s;
    // Normalise: shift so the divisor's top bit is set.
    std::vector<uint64_t> v(n), u(a.limbs_.size() + 1);
    for (size_t i = n; i-- > 0;)
        v[i] = (b.limbs_[i] << s) | (s && i ? b.limbs_[i - 1] >> (kLimbBits - s) : 0);
    u[a.limbs_.size()] = s ? a.limbs_.back() >> (kLimbBits - s) : 0;
    for (size_t i = a.limbs_.size(); i-- > 0;)
        u[i] = (a.limbs_[i] << s) | (s && i ? a.limbs_[i - 1] >> (kLimbBits - s) : 0);
    std::vector<uint64_t> qd(m + 1, 0);
    for (size_t j = m + 1; j-- > 0;)
    {
        // Estimate the quotient digit from the top two digits, then correct it.
        uint64_t qhat, rhat;
        if (u[j + n] >= v[n - 1])
        {
            // The estimate would not fit a limb: it is at most B - 1, and the correction below
            // brings it down to the true digit.
            qhat = (std::numeric_limits<uint64_t>::max)();
            rhat = u[j + n - 1] + v[n - 1];
            if (rhat < v[n - 1]) goto multiply_and_subtract; // rhat overflowed: estimate stands
        }
        else
        {
            qhat = div_wide(u[j + n], u[j + n - 1], v[n - 1], &rhat);
        }
        while (true)
        {
            const Wide p = mul_wide(qhat, v[n - 2]);
            if (p.hi < rhat || (p.hi == rhat && p.lo <= u[j + n - 2])) break;
            --qhat;
            rhat += v[n - 1];
            if (rhat < v[n - 1]) break; // overflowed past B: the estimate is good enough
        }
    multiply_and_subtract:
    {
        uint64_t borrow = 0, carry = 0;
        for (size_t i = 0; i < n; ++i)
        {
            const Wide t = mul_add2(qhat, v[i], carry, 0);
            const uint64_t plo = t.lo;
            carry = t.hi;
            uint64_t diff = u[i + j] - plo;
            const uint64_t next = (u[i + j] < plo ? 1 : 0);
            const uint64_t diff2 = diff - borrow;
            borrow = next + (diff < borrow ? 1 : 0);
            u[i + j] = diff2;
        }
        const uint64_t top = u[j + n];
        uint64_t diff = top - carry;
        uint64_t nb = (top < carry ? 1 : 0);
        const uint64_t diff2 = diff - borrow;
        nb += (diff < borrow ? 1 : 0);
        u[j + n] = diff2;
        if (nb)
        {
            // qhat was one too large: add the divisor back.
            --qhat;
            uint64_t c = 0;
            for (size_t i = 0; i < n; ++i)
            {
                uint64_t sum = u[i + j] + v[i];
                uint64_t cn = sum < v[i] ? 1 : 0;
                sum += c;
                if (sum < c) cn = 1;
                u[i + j] = sum;
                c = cn;
            }
            u[j + n] += c;
        }
        qd[j] = qhat;
    }
    }
    q.limbs_ = std::move(qd);
    q.trim();
    // Unnormalise the remainder.
    r.limbs_.assign(n, 0);
    for (size_t i = 0; i < n; ++i)
        r.limbs_[i] = s ? (u[i] >> s) | (i + 1 < u.size() ? u[i + 1] << (kLimbBits - s) : 0) : u[i];
    r.trim();
}

size_t BigUint::bit_length() const
{
    if (limbs_.empty()) return 0;
    uint64_t top = limbs_.back();
    size_t bits = 0;
    while (top) { ++bits; top >>= 1; }
    return (limbs_.size() - 1) * kLimbBits + bits;
}

bool BigUint::is_power_of_two() const
{
    if (limbs_.empty()) return false;
    for (size_t i = 0; i + 1 < limbs_.size(); ++i)
        if (limbs_[i]) return false;
    const uint64_t top = limbs_.back();
    return (top & (top - 1)) == 0;
}

BigUint& BigUint::operator-=(const BigUint& other)
{
    if (compare(*this, other) < 0) throw std::underflow_error("BigUint subtraction would go below zero");
    uint64_t borrow = 0;
    for (size_t i = 0; i < limbs_.size(); ++i)
    {
        const uint64_t sub = i < other.limbs_.size() ? other.limbs_[i] : 0;
        const uint64_t a = limbs_[i];
        uint64_t t = a - sub;
        uint64_t next = a < sub ? 1 : 0;
        if (t < borrow) next = 1;
        t -= borrow;
        limbs_[i] = t;
        borrow = next;
        if (!borrow && i >= other.limbs_.size()) break;
    }
    trim();
    return *this;
}

BigUint& BigUint::operator<<=(size_t bits)
{
    if (limbs_.empty() || bits == 0) return *this;
    const size_t whole = bits / kLimbBits, part = bits % kLimbBits;
    if (part)
    {
        uint64_t carry = 0;
        for (auto& limb : limbs_)
        {
            const uint64_t next = limb >> (kLimbBits - part);
            limb = (limb << part) | carry;
            carry = next;
        }
        if (carry) limbs_.push_back(carry);
    }
    limbs_.insert(limbs_.begin(), whole, uint64_t(0));
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
    const size_t whole = bits / kLimbBits, part = bits % kLimbBits;
    if (whole >= limbs_.size()) { limbs_.clear(); return *this; }
    limbs_.erase(limbs_.begin(), limbs_.begin() + static_cast<std::ptrdiff_t>(whole));
    if (part)
    {
        for (size_t i = 0; i < limbs_.size(); ++i)
        {
            const uint64_t hi = i + 1 < limbs_.size() ? limbs_[i + 1] : 0;
            limbs_[i] = (limbs_[i] >> part) | (hi << (kLimbBits - part));
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
    for (size_t i = top.limbs_.size(); i-- > 0;) m = m * 18446744073709551616.0 + double(top.limbs_[i]);
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
