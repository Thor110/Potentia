// Exact unsigned big integers, the ground everything addressed stands on (SPECIFICATIONS §4.3).
//
// An address is a single integer that can run to hundreds of thousands of bits, and every line
// turns it into symbols and back through the conversions at the bottom of this file. Those
// conversions are the most expensive thing the engine does, so the arithmetic under them is
// chosen for long operands: Karatsuba multiplication above a few dozen limbs, and division by
// multiplying with a precomputed reciprocal once the divisor is long enough to repay working
// the reciprocal out. The conversions then split a number in half by a power of the base and
// recurse, so each level of the recursion is a handful of those fast multiplications instead of
// one slow pass per limb.
//
// Every fast path here is exact. The reciprocal and the quotient it gives are estimates, and each
// is followed by a correction that compares against the true value and steps until it is right.
// The mathematics allows each correction only a step or two, so the steps are capped at
// kMaxCorrection: going past it can only mean the arithmetic underneath is broken, and it throws
// rather than returning a wrong digit or searching for ever. The schoolbook paths are kept
// underneath for short operands, where they are faster, and they are what the conformance vectors
// in tests/ were first checked against.

#include "sieve/biguint.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>
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

// Where the fast algorithms take over, in limbs. They were measured on x86-64 with GCC at -O2 and
// are not critical: a threshold a little off either way costs a few percent, never correctness.
//
// Below kKaratsuba, schoolbook multiplication wins on its lower overhead. A reciprocal costs about
// two multiplications of the divisor's length to work out and one more per use, so whether it
// pays depends on how often it is used. The conversions keep theirs, and use one for any power of
// the base from kLadderBarrett limbs up. A single division works one out only for a divisor of
// kBarrett limbs or more with a quotient at least twice as long, or of kBarrettOnce limbs or more
// with a quotient at least as long; anything shorter is faster by Knuth's algorithm D. A
// reciprocal of kReciprocalBase limbs or fewer is itself found by one long division rather than
// by Newton's method. A conversion peels one limb of digits at a time once the number is
// kConvertLeaf limbs or shorter.
constexpr size_t kKaratsuba = 32;
constexpr size_t kLadderBarrett = 64;
constexpr size_t kBarrett = 160;
constexpr size_t kBarrettOnce = 1000;
constexpr size_t kReciprocalBase = 64;
constexpr size_t kConvertLeaf = 40;

// Barrett's quotient is at most two short and the Newton step leaves the reciprocal within a few
// units, so no correction should ever take more than a handful of steps (see the introduction).
constexpr int kMaxCorrection = 16;

void correction_step(int& steps)
{
    if (++steps > kMaxCorrection) throw std::logic_error("BigUint: a division estimate would not converge");
}

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

// floor(log2(base)) for base >= 2. Used only to bound how many digits a value can need, where a
// bound that is one or two digits generous costs nothing.
unsigned floor_log2(uint32_t base)
{
    unsigned s = 0;
    while (base >>= 1) ++s;
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

// ---------------------------------------------------------------- limb vectors
//
// Multiplication works on bare runs of limbs rather than on BigUint values, because Karatsuba
// recurses on halves of its operands and copying each half into a value of its own would cost
// more than the multiplication it saves.

// r[0, n) += a[0, an) with an <= n. Returns the carry out of the top limb.
uint64_t add_to(uint64_t* r, size_t n, const uint64_t* a, size_t an)
{
    uint64_t carry = 0;
    size_t i = 0;
    for (; i < an; ++i)
    {
        // A sum that wraps is at most 2^64 - 2, so adding the carry to it cannot wrap again and
        // the two carries can be combined with an or.
        const uint64_t s = r[i] + a[i];
        const uint64_t t = s + carry;
        carry = uint64_t(s < a[i]) | uint64_t(t < s);
        r[i] = t;
    }
    for (; carry && i < n; ++i) carry = ++r[i] == 0;
    return carry;
}

// r[0, n) -= a[0, an) with an <= n. Returns the borrow out of the top limb.
uint64_t sub_from(uint64_t* r, size_t n, const uint64_t* a, size_t an)
{
    uint64_t borrow = 0;
    size_t i = 0;
    for (; i < an; ++i)
    {
        // The same argument as add_to: a difference that wraps is at least 1, so taking the
        // borrow from it cannot wrap a second time.
        const uint64_t d = r[i] - a[i];
        const uint64_t t = d - borrow;
        borrow = uint64_t(r[i] < a[i]) | uint64_t(d < borrow);
        r[i] = t;
    }
    for (; borrow && i < n; ++i) borrow = r[i]-- == 0;
    return borrow;
}

// out[0, an + bn) = a * b, schoolbook. out must not overlap either operand.
void mul_basecase(uint64_t* out, const uint64_t* a, size_t an, const uint64_t* b, size_t bn)
{
    std::fill(out, out + an + bn, uint64_t(0));
    for (size_t i = 0; i < an; ++i)
    {
        const uint64_t x = a[i];
        if (!x) continue;
        uint64_t carry = 0;
        for (size_t j = 0; j < bn; ++j)
        {
            const Wide t = mul_add2(x, b[j], out[i + j], carry);
            out[i + j] = t.lo;
            carry = t.hi;
        }
        // Nothing at or above i + bn has been written yet, so the carry is stored, not added.
        out[i + bn] = carry;
    }
}

// out[0, 2n) = a * b for two operands of n limbs each, by Karatsuba: three half-size products
// instead of four, recursively. With a = a1·B^h + a0 and b likewise, the middle term
// a1·b0 + a0·b1 is (a0 + a1)(b0 + b1) - a0·b0 - a1·b1, and the two outer products are needed
// anyway. The middle product is never negative, so no signs are carried.
void mul_karatsuba(uint64_t* out, const uint64_t* a, const uint64_t* b, size_t n)
{
    if (n < kKaratsuba)
    {
        mul_basecase(out, a, n, b, n);
        return;
    }
    const size_t h = n / 2, t = n - h; // low halves of h limbs, high halves of t >= h
    mul_karatsuba(out, a, b, h);                 // a0·b0 into out[0, 2h)
    mul_karatsuba(out + 2 * h, a + h, b + h, t); // a1·b1 into out[2h, 2n)
    std::vector<uint64_t> sa(t + 1, 0), sb(t + 1, 0), mid(2 * (t + 1));
    std::copy(a + h, a + n, sa.begin());
    sa[t] = add_to(sa.data(), t, a, h);
    std::copy(b + h, b + n, sb.begin());
    sb[t] = add_to(sb.data(), t, b, h);
    mul_karatsuba(mid.data(), sa.data(), sb.data(), t + 1);
    sub_from(mid.data(), mid.size(), out, 2 * h);
    sub_from(mid.data(), mid.size(), out + 2 * h, 2 * t);
    // The middle term is below 2^(64(n + 1)), and h >= 2 here, so it fits in the 2n - h limbs
    // above its offset and adding it in cannot carry out of the product.
    add_to(out + h, 2 * n - h, mid.data(), mid.size());
}

// out[0, an + bn) = a * b for operands of any length. out must not overlap either operand.
void mul_limbs(uint64_t* out, const uint64_t* a, size_t an, const uint64_t* b, size_t bn)
{
    if (an < bn)
    {
        std::swap(a, b);
        std::swap(an, bn);
    }
    if (bn < kKaratsuba)
    {
        mul_basecase(out, a, an, b, bn);
        return;
    }
    if (an == bn)
    {
        mul_karatsuba(out, a, b, an);
        return;
    }
    // Lopsided: the long operand is cut into pieces the length of the short one, so that every
    // product is square and Karatsuba applies to all of them; the products are added in at their
    // offsets.
    std::fill(out, out + an + bn, uint64_t(0));
    std::vector<uint64_t> part(2 * bn);
    for (size_t i = 0; i < an; i += bn)
    {
        const size_t w = std::min(bn, an - i);
        mul_limbs(part.data(), a + i, w, b, bn);
        add_to(out + i, an + bn - i, part.data(), w + bn);
    }
}

} // namespace

// ---------------------------------------------------------------- division by a reciprocal
//
// Knuth's algorithm D costs the product of the divisor's and the quotient's lengths in 128-bit
// divisions and multiply-subtracts, which is quadratic however fast multiplication becomes. For
// a long divisor d of n limbs, normalised so its top bit is set, the reciprocal
// R = floor(B^(2n) / d), where B = 2^64, turns division into multiplication: for any x < B^(2n),
// q = floor(floor(x / B^(n-1)) · R / B^(n+1)) is never more than the true quotient and at most
// two below it (Barrett, as given in the Handbook of Applied Cryptography, 14.42). The
// remainder x - q·d is then corrected by subtracting d until it is below d. A longer x is taken n
// limbs at a time from the top, like long division with digits of n limbs.
//
// BigUintOps is a friend of BigUint, so these can reach the limbs without widening the public
// interface.

struct BigUintOps
{
    static std::vector<uint64_t>& limbs(BigUint& v) { return v.limbs_; }
    static const std::vector<uint64_t>& limbs(const BigUint& v) { return v.limbs_; }
    static BigUint make(std::vector<uint64_t> l)
    {
        BigUint v;
        v.limbs_ = std::move(l);
        v.trim();
        return v;
    }
    static void knuth(const BigUint& a, const BigUint& b, BigUint& q, BigUint& r) { BigUint::divmod_knuth(a, b, q, r); }
    static uint64_t divmod_limb(BigUint& v, uint64_t d) { return v.divmod_limb(d); }
    static void mul_limb(BigUint& v, uint64_t m) { v.mul_limb(m); }
    static void add_limb(BigUint& v, uint64_t a) { v.add_limb(a); }
};

namespace {

BigUint power_of_two(size_t bits)
{
    BigUint v(1);
    v <<= bits;
    return v;
}

// floor(B^(2n) / d) for a d of exactly n limbs whose top bit is set.
//
// Newton's iteration for 1/d doubles the number of correct digits at every step, so the
// reciprocal of the top half of d, scaled up, is already right to about half its limbs, and one
// step x' = x + x·(B^(2n) - d·x) / B^(2n) brings it to within a few units of the answer. Because
// only about half of each product in that step is significant, the step is worked on the half
// reciprocal and on the top limbs of the error, which keeps every multiplication at half length.
// The exact correction afterwards moves the result its last few units, so what comes back is
// exactly the floor whatever the rounding on the way.
BigUint reciprocal(const BigUint& d, size_t n)
{
    const BigUint top = power_of_two(128 * n);
    if (n <= kReciprocalBase)
    {
        BigUint x, r;
        BigUintOps::knuth(top, d, x, r);
        return x;
    }
    const size_t h = (n + 1) / 2, s = n - h;
    BigUint dh = d;
    dh >>= 64 * s;
    const BigUint rh = reciprocal(dh, h); // x = rh·B^s is right to about h limbs

    // p = d·x, and the error e = |B^(2n) - p|, whose sign is not known in advance.
    BigUint p = BigUint::mul(d, rh);
    p <<= 64 * s;
    const bool under = p <= top;
    BigUint e = under ? top : p;
    e -= under ? p : top;

    // The step is x·e / B^(2n) = rh·e / B^(n+h). The limbs of e below B^(n-2) change that by
    // less than one part in B, so they are dropped, and the step is taken from what is left.
    e >>= 64 * (n - 2);
    BigUint c = BigUint::mul(rh, e);
    c >>= 64 * (h + 2);

    // Apply it to x, and the same step times d to p, so that p stays d·x without another
    // full-length product. x is far larger than the step, so neither subtraction can go below zero.
    BigUint x = rh;
    x <<= 64 * s;
    const BigUint dc = BigUint::mul(d, c);
    if (under)
    {
        x += c;
        p += dc;
    }
    else
    {
        x -= c;
        p -= dc;
    }

    int steps = 0;
    while (p > top)
    {
        correction_step(steps);
        x -= BigUint(1);
        p -= d;
    }
    BigUint rem = top;
    rem -= p;
    while (rem >= d)
    {
        correction_step(steps);
        x.add_small(1);
        rem -= d;
    }
    return x;
}

// A divisor made ready for repeated division: normalised, with its reciprocal worked out once.
// The conversions divide by the same power of the base at every node of one level of their
// recursion, so building this once per power is what makes them fast.
class Divisor
{
public:
    explicit Divisor(const BigUint& b)
    {
        const uint64_t hi = BigUintOps::limbs(b).back();
        for (uint64_t t = hi; !(t & (uint64_t(1) << 63)); t <<= 1) ++shift_;
        d_ = b;
        d_ <<= shift_;
        n_ = BigUintOps::limbs(d_).size();
        r_ = reciprocal(d_, n_);
    }

    // a = q·b + r with 0 <= r < b, for any a.
    void divmod(const BigUint& a, BigUint& q, BigUint& r) const
    {
        BigUint x = a;
        x <<= shift_;
        const auto& xl = BigUintOps::limbs(x);
        const size_t m = xl.size();
        if (m <= 2 * n_)
        {
            step(x, q, r);
        }
        else
        {
            // Long division with digits of n limbs. The running remainder is below d, so each
            // partial dividend is below d·B^n <= B^(2n), which is what step() needs, and each
            // partial quotient fits in n limbs.
            const size_t blocks = (m + n_ - 1) / n_;
            std::vector<uint64_t> ql(blocks * n_, 0);
            BigUint rem;
            for (size_t k = blocks; k-- > 0;)
            {
                const size_t lo = k * n_, hi = std::min(m, lo + n_);
                const auto& rl = BigUintOps::limbs(rem);
                std::vector<uint64_t> cur(n_ + rl.size(), 0);
                std::copy(xl.begin() + std::ptrdiff_t(lo), xl.begin() + std::ptrdiff_t(hi), cur.begin());
                std::copy(rl.begin(), rl.end(), cur.begin() + std::ptrdiff_t(n_));
                BigUint qb;
                step(BigUintOps::make(std::move(cur)), qb, rem);
                const auto& qbl = BigUintOps::limbs(qb);
                std::copy(qbl.begin(), qbl.end(), ql.begin() + std::ptrdiff_t(lo));
            }
            q = BigUintOps::make(std::move(ql));
            r = std::move(rem);
        }
        r >>= shift_;
    }

private:
    // One Barrett step, for a normalised x < B^(2n).
    void step(const BigUint& x, BigUint& q, BigUint& r) const
    {
        BigUint t = x;
        t >>= 64 * (n_ - 1);
        q = BigUint::mul(t, r_);
        q >>= 64 * (n_ + 1);
        r = x;
        // q never exceeds the true quotient, so this cannot go below zero unless the arithmetic
        // is broken, in which case the subtraction throws.
        r -= BigUint::mul(q, d_);
        int steps = 0;
        while (r >= d_)
        {
            correction_step(steps);
            r -= d_;
            q.add_small(1);
        }
    }

    BigUint d_, r_;
    unsigned shift_ = 0;
    size_t n_ = 0;
};

// ---------------------------------------------------------------- the conversion ladder
//
// A conversion between an integer and its digits in a base that is not a power of two splits the
// digits in two at a power of the base, converts each half, and recurses. The split points are
// the powers base^(k·2^j), where k is the number of digits that fit in one limb, so that every
// split is at a whole number of limb-sized chunks and each level of the recursion uses one power
// of the base. Those powers and their reciprocals cost as much to build as a conversion does, and
// the same few bases are converted over and over (a line's alphabet, and ten for display), so
// each thread keeps the ladders of the last few bases it used. They are pure functions of the
// base: keeping them changes nothing but the time.

struct Ladder
{
    explicit Ladder(uint32_t b) : base(b)
    {
        const auto c = chunk_of(b);
        k = c.first;
        full = c.second;
    }

    // Makes sure powers 0..j exist. Called before a conversion starts, so that nothing is added
    // to `pow` while references into it are held.
    void reach(size_t j)
    {
        if (pow.empty()) pow.emplace_back(full);
        while (pow.size() <= j) pow.push_back(BigUint::mul(pow.back(), pow.back()));
        if (div.size() < pow.size()) div.resize(pow.size());
    }

    // The largest j with k·2^j < length, for a length > k. The low part of the split then has
    // k·2^j digits and the high part the rest, which is at most as many.
    size_t split_level(size_t length) const
    {
        size_t j = 0;
        while ((k << (j + 1)) < length) ++j;
        return j;
    }

    void divmod(const BigUint& a, size_t j, BigUint& q, BigUint& r)
    {
        const BigUint& p = pow[j];
        if (BigUintOps::limbs(p).size() < kLadderBarrett)
        {
            BigUint::divmod(a, p, q, r);
            return;
        }
        if (!div[j]) div[j] = std::make_unique<Divisor>(p);
        div[j]->divmod(a, q, r);
    }

    uint32_t base;
    size_t k = 0;
    uint64_t full = 0;
    std::vector<BigUint> pow;                   // pow[j] = base^(k·2^j)
    std::vector<std::unique_ptr<Divisor>> div; // built on first use
};

Ladder& ladder_for(uint32_t base)
{
    constexpr size_t kKept = 4;
    thread_local std::vector<std::unique_ptr<Ladder>> kept;
    for (auto& l : kept)
        if (l->base == base) return *l;
    if (kept.size() >= kKept) kept.erase(kept.begin());
    kept.push_back(std::make_unique<Ladder>(base));
    return *kept.back();
}

} // namespace

namespace {

const char* const kTooLong = "value does not fit in the requested number of digits";

// v as exactly `length` digits at out, most significant first, one limb's worth of digits per
// division by a single limb. Quadratic, so it is the leaf of the recursion below and nothing
// more. out must already be zero, since a value shorter than `length` leaves the top untouched.
void peel_digits(BigUint v, const Ladder& lad, uint32_t* out, size_t length)
{
    for (size_t i = length; i > 0 && !v.is_zero();)
    {
        uint64_t rem = BigUintOps::divmod_limb(v, lad.full);
        for (size_t j = 0; j < lad.k && i > 0; ++j)
        {
            out[--i] = uint32_t(rem % lad.base);
            rem /= lad.base;
        }
        if (rem) throw std::out_of_range(kTooLong);
    }
    if (!v.is_zero()) throw std::out_of_range(kTooLong);
}

// v as exactly `length` digits at out (already zero), by splitting at base^(k·2^j) and recursing
// on the quotient and the remainder. A value too big for `length` leaves a quotient too big for
// the top half, and so on down, until the topmost leaf finds it has digits left over and throws.
void split_digits(const BigUint& v, Ladder& lad, uint32_t* out, size_t length)
{
    if (v.is_zero()) return;
    if (length <= lad.k || BigUintOps::limbs(v).size() <= kConvertLeaf)
    {
        peel_digits(v, lad, out, length);
        return;
    }
    const size_t j = lad.split_level(length), low = lad.k << j;
    if (v < lad.pow[j])
    {
        split_digits(v, lad, out + (length - low), low); // the top part is all zeros
        return;
    }
    BigUint q, r;
    lad.divmod(v, j, q, r);
    split_digits(q, lad, out, length - low);
    split_digits(r, lad, out + (length - low), low);
}

// The integer whose digits are d[0, length), most significant first: the two halves joined as
// high·base^(k·2^j) + low, with Horner's rule a limb of digits at a time at the leaves.
BigUint join_digits(const uint32_t* d, size_t length, Ladder& lad)
{
    if (length <= lad.k * kConvertLeaf)
    {
        BigUint v;
        for (size_t i = 0; i < length;)
        {
            const size_t take = std::min(lad.k, length - i);
            uint64_t chunk = 0, scale = 1;
            for (size_t t = 0; t < take; ++t, ++i)
            {
                if (d[i] >= lad.base) throw std::invalid_argument("digit out of range for base");
                chunk = chunk * lad.base + d[i];
                scale *= lad.base;
            }
            BigUintOps::mul_limb(v, scale);
            BigUintOps::add_limb(v, chunk);
        }
        return v;
    }
    const size_t j = lad.split_level(length), low = lad.k << j;
    BigUint v = join_digits(d, length - low, lad);
    BigUint lo = join_digits(d + (length - low), low, lad);
    if (v.is_zero()) return lo;
    v = BigUint::mul(v, lad.pow[j]);
    v += lo;
    return v;
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

void BigUint::add_mul_small(const BigUint& x, uint32_t m)
{
    if (&x == this)
    {
        const BigUint copy = x; // (its limbs would change under the loop)
        add_mul_small(copy, m);
        return;
    }
    add_mul_small(std::span<const uint64_t>(x.limbs_), m);
}

void BigUint::add_mul_small(std::span<const uint64_t> x, uint32_t m)
{
    if (m == 0 || x.empty()) return;
    if (limbs_.size() < x.size()) limbs_.resize(x.size(), 0);
    uint64_t carry = 0;
    size_t i = 0;
    for (; i < x.size(); ++i)
    {
        const Wide t = mul_add2(x[i], m, limbs_[i], carry);
        limbs_[i] = t.lo;
        carry = t.hi;
    }
    for (; carry && i < limbs_.size(); ++i)
    {
        limbs_[i] += carry;
        carry = limbs_[i] < carry ? 1 : 0;
    }
    if (carry) limbs_.push_back(carry);
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

uint32_t BigUint::mod_small(uint32_t d) const
{
    if (d == 0) throw std::domain_error("division by zero");
    uint64_t rem = 0;
    for (size_t i = limbs_.size(); i-- > 0;)
    {
        rem = ((rem << 32) | (limbs_[i] >> 32)) % d;
        rem = ((rem << 32) | (limbs_[i] & kHalfMask)) % d;
    }
    return static_cast<uint32_t>(rem);
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

BigUint BigUint::from_limbs64(std::span<const uint64_t> limbs)
{
    BigUint v;
    v.limbs_.assign(limbs.begin(), limbs.end());
    v.trim();
    return v;
}

BigUint BigUint::from_bytes(std::span<const uint8_t> bytes)
{
    BigUint v;
    v.limbs_.assign((bytes.size() + 7) / 8, 0);
    for (size_t i = 0; i < bytes.size(); ++i)
    {
        const size_t from_end = bytes.size() - 1 - i; // place of this byte, least significant first
        v.limbs_[from_end / 8] |= uint64_t(bytes[i]) << (8 * (from_end % 8));
    }
    v.trim();
    return v;
}

std::vector<uint8_t> BigUint::to_bytes(size_t length) const
{
    if (limbs_.size() > (length + 7) / 8 || (bit_length() + 7) / 8 > length) throw std::out_of_range("value does not fit in that many bytes");
    std::vector<uint8_t> out(length, 0);
    for (size_t k = 0; k < length; ++k)
    {
        const size_t limb = k / 8;
        if (limb >= limbs_.size()) break;
        out[length - 1 - k] = uint8_t(limbs_[limb] >> (8 * (k % 8)));
    }
    return out;
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
    Ladder& lad = ladder_for(base);
    if (digits.size() > lad.k) lad.reach(lad.split_level(digits.size()));
    return join_digits(digits.data(), digits.size(), lad);
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
        if (bit_length() > uint64_t(length) * sh) throw std::out_of_range(kTooLong);
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
    // No value needs more digits than this, so any beyond it are leading zeros, and the
    // recursion is not asked to build powers of the base for them. If the value is too big for
    // `length`, `used` is the whole length and the recursion finds the overflow in its top leaf.
    Ladder& lad = ladder_for(base);
    const size_t most = bit_length() / floor_log2(base) + 1;
    const size_t used = std::min(length, most);
    if (used > lad.k) lad.reach(lad.split_level(used));
    split_digits(*this, lad, digits.data() + (length - used), used);
    return digits;
}

BigUint BigUint::pow(uint32_t base, uint64_t exponent)
{
    if (base < 2) return BigUint(base == 0 && exponent > 0 ? 0 : 1); // 0^0 = 1
    BigUint v(1);
    if (const unsigned sh = pow2_shift(base))
    {
        v <<= size_t(exponent) * sh; // 2^(sh * exponent)
        return v;
    }
    // Square and multiply over base^k, the largest power of the base that fits a limb, so a
    // power with a million digits is a score of squarings rather than a million small products.
    const auto [k, full] = chunk_of(base);
    BigUint sq(full);
    for (uint64_t e = exponent / k; e; e >>= 1)
    {
        if (e & 1) v = mul(v, sq);
        if (e > 1) sq = mul(sq, sq);
    }
    uint64_t rest = 1;
    for (uint64_t e = exponent % k; e; --e) rest *= base;
    v.mul_limb(rest);
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
    // Ten is just another base: the same recursion as to_digits, over at least as many digits
    // as the value can need (a decimal digit carries more than three bits), then the leading
    // zeros dropped.
    const std::vector<uint32_t> d = to_digits(10, bit_length() / 3 + 1);
    size_t i = 0;
    while (d[i] == 0) ++i;
    std::string s;
    s.reserve(d.size() - i);
    for (; i < d.size(); ++i) s.push_back(char('0' + d[i]));
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
    std::vector<uint32_t> d(dec.size());
    for (size_t i = 0; i < dec.size(); ++i)
    {
        const char c = dec[i];
        if (c < '0' || c > '9') throw std::invalid_argument("invalid decimal digit '" + std::string(1, c) + "'");
        d[i] = uint32_t(c - '0');
    }
    return from_digits(d, 10);
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
    std::vector<uint64_t> p(a.limbs_.size() + b.limbs_.size());
    mul_limbs(p.data(), a.limbs_.data(), a.limbs_.size(), b.limbs_.data(), b.limbs_.size());
    return BigUintOps::make(std::move(p));
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
    // Whether a reciprocal repays working it out: see kBarrett and kBarrettOnce.
    const size_t n = b.limbs_.size(), qn = a.limbs_.size() >= n ? a.limbs_.size() - n : 0;
    if ((n >= kBarrett && qn >= 2 * n) || (n >= kBarrettOnce && qn >= n))
    {
        Divisor(b).divmod(a, q, r);
        return;
    }
    divmod_knuth(a, b, q, r);
}

// Knuth, TAOCP vol. 2, 4.3.1, algorithm D. Quadratic, and the fastest way there is for a short
// divisor or a short quotient. The outputs must not share storage with the inputs.
void BigUint::divmod_knuth(const BigUint& a, const BigUint& b, BigUint& q, BigUint& r)
{
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
