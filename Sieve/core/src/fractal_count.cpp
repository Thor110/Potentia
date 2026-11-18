// Sieve — the fractal dimension's structural filters counted (see fractal_count.hpp).
#include "sieve/fractal_count.hpp"

#include "sieve/fractal.hpp" // kFractalWordMost

#include <cstdint>
#include <utility>
#include <vector>

namespace sieve {

namespace {

constexpr uint32_t kAxiomOverhead = 7; // "axiom " and the line feed (as fractal_space.cpp writes them)
constexpr uint32_t kRuleOverhead = 8;  // "rule X " and the line feed
constexpr uint32_t kWordMost = uint32_t(kFractalWordMost);

using Poly = std::vector<BigUint>; // [r]: how many of exactly r characters

enum class Rule { None, Any, NoSelf, Short, ShortNoSelf };

uint64_t choose(uint32_t n, uint32_t k)
{
    if (k > n) return 0;
    uint64_t v = 1;
    for (uint32_t i = 1; i <= k; ++i) v = v * (n - k + i) / i; // (exact at each step; n <= 64 here)
    return v;
}

// One optional rule over an alphabet of a symbols, multiplied in: p times (no rule, or "rule X " and a word
// and a line feed). Its words: a^n of n symbols, n to kWordMost (Short: one symbol only; NoSelf: not the
// symbol itself). In one pass, as the words' counts are a geometric series: h[k] = a (h[k-1] + p[k-1]) less
// a^(W+1) p[k-1-W], where h[k] = sum over n of a^n p[k-n].
struct RuleMul
{
    uint32_t a;
    Rule mode;
    BigUint a_top; // a^(kWordMost + 1)
    RuleMul(uint32_t alphabet, Rule m) : a(alphabet), mode(m), a_top(BigUint::pow(alphabet, kWordMost + 1)) {}
    void apply(Poly& p) const
    {
        if (mode == Rule::None) return;
        const size_t top = p.size() - 1;
        Poly h(top + 1);
        const bool shorts = mode == Rule::Short || mode == Rule::ShortNoSelf;
        const bool noself = mode == Rule::NoSelf || mode == Rule::ShortNoSelf;
        for (size_t k = 1; k <= top; ++k)
        {
            if (shorts)
            {
                h[k] = p[k - 1];
                h[k].mul_small(noself ? a - 1 : a);
                continue;
            }
            BigUint v = h[k - 1];
            v += p[k - 1];
            v.mul_small(a);
            if (k >= kWordMost + 1 && !p[k - 1 - kWordMost].is_zero()) v -= BigUint::mul(a_top, p[k - 1 - kWordMost]);
            h[k] = std::move(v);
        }
        if (noself && !shorts)
            for (size_t k = top; k >= 1; --k) h[k] -= p[k - 1]; // (h's n = 1 term is a p[k-1]; one of the a is the symbol)
        for (size_t k = top; k >= kRuleOverhead; --k) p[k] += h[k - kRuleOverhead];
    }
};

Poly axiom_poly(uint32_t a, uint32_t top)
{
    Poly p(top + 1);
    BigUint w(1);
    for (uint32_t n = 1; n <= kWordMost && kAxiomOverhead + n <= top; ++n)
    {
        w.mul_small(a);
        p[kAxiomOverhead + n] = w;
    }
    return p;
}

void add_scaled(Poly& acc, const Poly& p, const BigUint& c)
{
    if (c.is_zero()) return;
    for (size_t k = 0; k < acc.size(); ++k)
        if (!p[k].is_zero()) acc[k] += BigUint::mul(p[k], c);
}

// The bodies passing, by length, with the reached symbols' rules as `in` and the others' as `out`.
Poly bodies(const FractalBodyKinds& kd, uint32_t top, const FractalStructureFilters& f, Rule in, Rule out)
{
    const uint32_t R = kd.ruled(), I = kd.inert;
    // inner[s]: the bodies on a set of s symbols reaching every one of them.
    std::vector<Poly> inner;
    for (uint32_t s = 0; s <= R; ++s)
    {
        const RuleMul rule(I + s, in);
        Poly all = axiom_poly(I + s, top);
        for (uint32_t k = 0; k < s; ++k) rule.apply(all);
        // sum over u < s of C(s, u) inner[u] rule^(s - u), by Horner's way
        Poly acc(top + 1);
        for (uint32_t u = 0; u < s; ++u)
        {
            rule.apply(acc);
            add_scaled(acc, inner[u], BigUint(choose(s, u)));
        }
        rule.apply(acc);
        for (size_t k = 0; k <= top; ++k) all[k] -= acc[k];
        inner.push_back(std::move(all));
    }
    // Each reach of s symbols: how many sets it stands for (and, canonical, the share of them named in order);
    // then the unreached symbols' rules, by Horner's way again over s (x_s times out^(R - s)).
    const RuleMul outside(kd.alphabet(), out);
    Poly total(top + 1);
    for (uint32_t s = 0; s <= R; ++s)
    {
        outside.apply(total);
        Poly x(top + 1);
        if (f.canonical)
        {
            for (uint32_t sd = 0; sd <= kd.drawing && sd <= s; ++sd)
                for (uint32_t sf = 0; sf <= kd.fixed && sd + sf <= s; ++sf)
                {
                    const uint32_t sm = s - sd - sf;
                    if (sm > kd.renamable) continue;
                    if (f.draws && (kd.substitution ? s < 2 : sd == 0)) continue;
                    BigUint fact(1);
                    for (uint32_t i = 2; i <= sm; ++i) fact.mul_small(i);
                    const BigUint c(choose(kd.drawing, sd) * choose(kd.fixed, sf));
                    for (size_t k = 0; k <= top; ++k)
                    {
                        if (inner[s][k].is_zero()) continue;
                        BigUint q, rem;
                        BigUint::divmod(inner[s][k], fact, q, rem); // (exact: a renaming of the reached is a different body)
                        x[k] += BigUint::mul(q, c);
                    }
                }
        }
        else
        {
            uint64_t c = choose(R, s);
            if (f.draws) c = kd.substitution ? (s >= 2 ? c : 0) : c - choose(R - kd.drawing, s);
            add_scaled(x, inner[s], BigUint(c));
        }
        for (size_t k = 0; k <= top; ++k) total[k] += x[k];
    }
    return total;
}

} // namespace

std::vector<BigUint> fractal_bodies_passing(const FractalBodyKinds& kinds, uint32_t top, const FractalStructureFilters& f)
{
    const Rule out = (f.no_idle || f.canonical) ? Rule::None : Rule::Any;
    auto with_rules = [&](Rule in) {
        Poly v = bodies(kinds, top, f, in, out);
        const Poly none = bodies(kinds, top, f, Rule::None, Rule::None); // (no rule at all: the reader refuses it)
        for (size_t k = 0; k <= top; ++k) v[k] -= none[k];
        return v;
    };
    Poly v = with_rules(f.no_idle ? Rule::NoSelf : Rule::Any);
    if (f.grows)
    {
        const Poly flat = with_rules(f.no_idle ? Rule::ShortNoSelf : Rule::Short);
        for (size_t k = 0; k <= top; ++k) v[k] -= flat[k];
    }
    return v;
}

FractalBodyKinds fractal_substitution_kinds() { return {0, 0, 0, 36, true}; }        // 0-9 a-z
FractalBodyKinds fractal_lsystem_kinds() { return {5, 2, 1, 49, false}; }            // + - [ ] |, F G, f, the other letters
FractalBodyKinds fractal_scaled_kinds() { return {7, 2, 1, 49, false}; }             // and " '

} // namespace sieve
