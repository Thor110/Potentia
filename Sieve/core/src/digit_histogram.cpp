// Sieve — digits numbered by their histograms (see digit_histogram.hpp).
#include "sieve/digit_histogram.hpp"

#include <stdexcept>
#include <string>

namespace sieve {

BigUint small_binomial(uint64_t a, uint32_t b)
{
    if (b > a) return BigUint(0);
    if (a > 0xFFFFFFFFull) throw std::invalid_argument("digit-histogram-v1: a segment too long to count");
    BigUint c(1);
    for (uint32_t i = 1; i <= b; ++i)
    {
        c.mul_small(uint32_t(a - b + i));
        c.divmod_small(i); // exact: C(a - b + i, i) at each step
    }
    return c;
}

namespace {

uint64_t small_value(const BigUint& v)
{
    const auto l = v.limbs();
    if (l.size() > 1) throw std::logic_error("digit-histogram-v1: a quotient past 64 bits");
    return l.empty() ? 0 : l[0];
}

void check(std::span<const uint64_t> segments, size_t digits, uint32_t K)
{
    if (K < 2 || K > 256) throw std::invalid_argument("digit-histogram-v1: digits of 2 to 256 values");
    uint64_t sum = 0;
    for (const uint64_t n : segments)
    {
        if (n > 0xFFFFFFFFull - K) throw std::invalid_argument("digit-histogram-v1: a segment too long to count");
        sum += n;
    }
    if (sum != digits) throw std::invalid_argument("digit-histogram-v1: the segments are not the digits' length");
}

// The histograms of n digits below K whose first digit's count is below v, given the counts before
// it: with p = K - j - 2 counts after this one less one, the sum over u < v of C(rem - u + p, p),
// which is C(rem + p + 1, p + 1) - C(rem - v + p + 1, p + 1).
BigUint below(uint64_t rem, uint64_t v, uint32_t p)
{
    BigUint all = small_binomial(rem + p + 1, p + 1);
    all -= small_binomial(rem - v + p + 1, p + 1);
    return all;
}

// n! / (c_0! ... c_{K-1}!), built one digit at a time.
BigUint arrangements(const std::vector<uint64_t>& counts)
{
    BigUint m(1);
    uint64_t total = 0;
    for (const uint64_t c : counts)
        for (uint64_t k = 1; k <= c; ++k)
        {
            ++total;
            m.mul_small(uint32_t(total));
            m.divmod_small(uint32_t(k));
        }
    return m;
}

} // namespace

BigUint digit_histogram_number(std::span<const uint8_t> digits, std::span<const uint64_t> segments, uint32_t K)
{
    check(segments, digits.size(), K);
    BigUint A, B, C(1), M(1);
    size_t at = 0;
    for (const uint64_t n : segments)
    {
        const std::span<const uint8_t> x = digits.subspan(at, size_t(n));
        at += size_t(n);
        std::vector<uint64_t> c(K, 0);
        for (const uint8_t d : x)
        {
            if (d >= K) throw std::invalid_argument("digit-histogram-v1: a digit out of range");
            ++c[d];
        }
        // Its histogram's rank.
        BigUint h;
        uint64_t rem = n;
        for (uint32_t j = 0; j + 1 < K; ++j)
        {
            h += below(rem, c[j], K - j - 2);
            rem -= c[j];
        }
        // Its digits' rank among the rows with that histogram.
        const BigUint m = arrangements(c);
        BigUint r, left = m;
        std::vector<uint64_t> cnt = c;
        uint64_t nrem = n;
        for (const uint8_t d : x)
        {
            uint64_t less = 0;
            for (uint32_t s = 0; s < d; ++s) less += cnt[s];
            if (less)
            {
                BigUint t = left;
                t.mul_small(uint32_t(less));
                t.divmod_small(uint32_t(nrem));
                r += t;
            }
            left.mul_small(uint32_t(cnt[d]));
            left.divmod_small(uint32_t(nrem));
            --cnt[d];
            --nrem;
        }
        const BigUint hc = small_binomial(n + K - 1, K - 1);
        A = BigUint::mul(A, hc);
        A += h;
        C = BigUint::mul(C, hc);
        B = BigUint::mul(B, m);
        B += r;
        M = BigUint::mul(M, m);
    }
    BigUint z = BigUint::mul(B, C);
    z += A;
    return z;
}

std::vector<uint8_t> digit_histogram_digits(const BigUint& z, std::span<const uint64_t> segments, uint32_t K)
{
    uint64_t total = 0;
    for (const uint64_t n : segments) total += n;
    check(segments, size_t(total), K);
    // A and B, then each segment's histogram from A (the last segment least significant).
    std::vector<BigUint> hc;
    BigUint C(1);
    for (const uint64_t n : segments)
    {
        hc.push_back(small_binomial(n + K - 1, K - 1));
        C = BigUint::mul(C, hc.back());
    }
    BigUint A, B;
    BigUint::divmod(z, C, B, A);
    std::vector<BigUint> h(segments.size());
    for (size_t t = segments.size(); t-- > 0;)
        BigUint::divmod(A, hc[t], A, h[t]);
    std::vector<std::vector<uint64_t>> counts(segments.size());
    std::vector<BigUint> ms;
    BigUint M(1);
    for (size_t t = 0; t < segments.size(); ++t)
    {
        std::vector<uint64_t>& c = counts[t];
        c.assign(K, 0);
        uint64_t rem = segments[t];
        BigUint r = h[t];
        for (uint32_t j = 0; j + 1 < K; ++j)
        {
            // The largest v with below(rem, v) <= r.
            uint64_t lo = 0, hi = rem;
            while (lo < hi)
            {
                const uint64_t mid = lo + (hi - lo + 1) / 2;
                if (below(rem, mid, K - j - 2) <= r) lo = mid;
                else hi = mid - 1;
            }
            r -= below(rem, lo, K - j - 2);
            c[j] = lo;
            rem -= lo;
        }
        c[K - 1] = rem;
        ms.push_back(arrangements(c));
        M = BigUint::mul(M, ms.back());
    }
    if (B >= M) throw std::invalid_argument("digit-histogram-v1: a number past its histograms' rows (no item has it)");
    std::vector<BigUint> rank(segments.size());
    for (size_t t = segments.size(); t-- > 0;)
        BigUint::divmod(B, ms[t], B, rank[t]);
    std::vector<uint8_t> out;
    out.reserve(size_t(total));
    for (size_t t = 0; t < segments.size(); ++t)
    {
        std::vector<uint64_t> cnt = counts[t];
        BigUint r = rank[t], left = ms[t];
        for (uint64_t nrem = segments[t]; nrem > 0; --nrem)
        {
            // The digit d with (left * below_d) / nrem <= r < (left * upto_d) / nrem: from
            // q = floor(r * nrem / left), below_d <= q < upto_d.
            BigUint t2 = r, q, rr;
            t2.mul_small(uint32_t(nrem));
            BigUint::divmod(t2, left, q, rr);
            const uint64_t qq = small_value(q);
            uint32_t d = 0;
            uint64_t less = 0;
            while (d < K && less + cnt[d] <= qq) less += cnt[d++];
            if (d >= K) throw std::logic_error("digit-histogram-v1: no digit for a rank");
            if (less)
            {
                BigUint sub = left;
                sub.mul_small(uint32_t(less));
                sub.divmod_small(uint32_t(nrem));
                r -= sub;
            }
            left.mul_small(uint32_t(cnt[d]));
            left.divmod_small(uint32_t(nrem));
            --cnt[d];
            out.push_back(uint8_t(d));
        }
    }
    return out;
}

} // namespace sieve
