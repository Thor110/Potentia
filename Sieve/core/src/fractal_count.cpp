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

using Poly = std::vector<BigUint>; // [r]: how many of exactly r characters

enum class Rule { None, Any, NoSelf, Short, ShortNoSelf };

uint64_t choose(uint32_t n, uint32_t k)
{
    if (k > n) return 0;
    uint64_t v = 1;
    for (uint32_t i = 1; i <= k; ++i) v = v * (n - k + i) / i; // (exact at each step; n <= 64 here)
    return v;
}

// A symbol's rules over an alphabet of a symbols, multiplied in. v1 and v2: none or one, "rule X " and a word and
// a line feed, p + q p. v3 (many): any number, each also with a context letter or none either side ("L<" and
// ">R", of `context` letters each), p / (1 - q), so q' = p + q q'. Its words: a^n of n symbols, n to
// word_most (Short: one symbol only; NoSelf: not the symbol itself). Both in one pass, as the words' counts
// are a geometric series: h[j] = a (h[j-1] + src[j-1]) less a^(W+1) src[j-1-W], h[j] being the sum over n of
// a^n src[j-n], src the input (one rule) or the output so far (many: h[j] needs it only below j).
struct RuleMul
{
    uint32_t a, word_most, context;
    bool many;
    Rule mode;
    BigUint a_top; // a^(word_most + 1)
    RuleMul(uint32_t alphabet, const FractalBodyKinds& kd, Rule m)
        : a(alphabet), word_most(kd.word_most), context(kd.context_letters), many(kd.many_rules), mode(m), a_top(BigUint::pow(alphabet, kd.word_most + 1))
    {
    }
    void apply(Poly& p) const
    {
        if (mode == Rule::None) return;
        const size_t top = p.size() - 1;
        const bool shorts = mode == Rule::Short || mode == Rule::ShortNoSelf;
        const bool noself = mode == Rule::NoSelf || mode == Rule::ShortNoSelf;
        Poly out(top + 1), hf(top + 1);
        const Poly& src = many ? out : p;
        // h(j): the words of a rule ending j characters after its "rule X ", ahead of src (j >= 1)
        auto h = [&](size_t j) {
            BigUint v = hf[j];
            if (noself && !shorts) v -= src[j - 1]; // (the n = 1 term is a src[j-1]; one of the a is the symbol itself)
            return v;
        };
        // (context is a letter count, at most 52: its square fits a small multiplier)
        const uint32_t both = uint32_t(context) * uint32_t(context), either = uint32_t(context) * 2;
        for (size_t k = 0; k <= top; ++k)
        {
            if (k > kRuleOverhead)
            {
                const size_t j = k - kRuleOverhead; // (src below j is final by now)
                if (shorts)
                {
                    hf[j] = src[j - 1];
                    hf[j].mul_small(noself ? a - 1 : a);
                }
                else
                {
                    BigUint v = hf[j - 1];
                    v += src[j - 1];
                    v.mul_small(a);
                    if (j >= word_most + 1 && !src[j - 1 - word_most].is_zero()) v -= BigUint::mul(a_top, src[j - 1 - word_most]);
                    hf[j] = std::move(v);
                }
            }
            BigUint t = p[k];
            if (k > kRuleOverhead) t += h(k - kRuleOverhead);
            if (context && k > kRuleOverhead + 2) t.add_mul_small(h(k - kRuleOverhead - 2), either);
            if (context && k > kRuleOverhead + 4) t.add_mul_small(h(k - kRuleOverhead - 4), both);
            out[k] = std::move(t);
        }
        p = std::move(out);
    }
};

Poly axiom_poly(uint32_t a, uint32_t word_most, uint32_t top)
{
    Poly p(top + 1);
    BigUint w(1);
    for (uint32_t n = 1; n <= word_most && kAxiomOverhead + n <= top; ++n)
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
        const RuleMul rule(I + s, kd, in);
        Poly all = axiom_poly(I + s, kd.word_most, top);
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
    const RuleMul outside(kd.alphabet(), kd, out);
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
    const Poly none = bodies(kinds, top, f, Rule::None, Rule::None); // (no rule at all: the reader refuses it)
    auto with_rules = [&](Rule in) {
        Poly v = bodies(kinds, top, f, in, out);
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

FractalBodyKinds fractal_substitution_kinds() { return {0, 0, 0, 36, true}; } // 0-9 a-z
FractalBodyKinds fractal_lsystem_kinds() { return {5, 2, 1, 49, false}; }     // + - [ ] |, F G, f, the other letters
FractalBodyKinds fractal_scaled_kinds() { return {7, 2, 1, 49, false}; }      // and " '
FractalBodyKinds fractal_v3_kinds()
{
    // The 74 symbols of a v3 word: the 52 letters (rules), and the digits and + - [ ] | ! & ^ / " ' \ (none)
    FractalBodyKinds k{22, 2, 1, 49, false};
    k.word_most = uint32_t(kFractalWordMostV3), k.context_letters = 52, k.many_rules = true;
    return k;
}

} // namespace sieve
