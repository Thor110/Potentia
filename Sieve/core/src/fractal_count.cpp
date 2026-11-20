// Sieve — the fractal dimension's structural filters counted (see fractal_count.hpp).
#include "sieve/fractal_count.hpp"

#include "sieve/fractal.hpp" // kFractalWordMost

#include <algorithm>
#include <array>
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <tuple>
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

// inner[s]: the bodies on a set of s symbols reaching every one of them, the reached symbols' rules as `in`.
std::vector<Poly> inner_polys(const FractalBodyKinds& kd, uint32_t top, Rule in)
{
    const uint32_t R = kd.ruled(), I = kd.inert;
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
    return inner;
}

// The bodies passing, by length, from inner_polys, with the symbols never reached having rules as `out`.
Poly bodies_from(const FractalBodyKinds& kd, uint32_t top, const FractalStructureFilters& f, const std::vector<Poly>& inner, Rule out)
{
    const uint32_t R = kd.ruled();
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

Rule in_rule(const FractalStructureFilters& f) { return f.no_idle ? Rule::NoSelf : Rule::Any; }
Rule flat_rule(const FractalStructureFilters& f) { return f.no_idle ? Rule::ShortNoSelf : Rule::Short; }
Rule out_rule(const FractalStructureFilters& f) { return (f.no_idle || f.canonical) ? Rule::None : Rule::Any; }

// The figure fractal_bodies_passing gives, from the three regimes' inner polys (flat empty unless grows).
Poly passing_from(const FractalBodyKinds& kd, uint32_t top, const FractalStructureFilters& f, const std::vector<Poly>& in, const std::vector<Poly>& flat,
                  const std::vector<Poly>& none_inner)
{
    const Poly none = bodies_from(kd, top, f, none_inner, Rule::None); // (no rule at all: the reader refuses it)
    Poly v = bodies_from(kd, top, f, in, out_rule(f));
    if (f.grows)
    {
        // (the bodies with no rule are among the flat ones too, so they go with them)
        const Poly fl = bodies_from(kd, top, f, flat, out_rule(f));
        for (size_t k = 0; k <= top; ++k) v[k] -= fl[k];
    }
    else
        for (size_t k = 0; k <= top; ++k) v[k] -= none[k];
    return v;
}

} // namespace

std::vector<BigUint> fractal_bodies_passing(const FractalBodyKinds& kinds, uint32_t top, const FractalStructureFilters& f)
{
    const std::vector<Poly> in = inner_polys(kinds, top, in_rule(f));
    const std::vector<Poly> flat = f.grows ? inner_polys(kinds, top, flat_rule(f)) : std::vector<Poly>{};
    return passing_from(kinds, top, f, in, flat, inner_polys(kinds, top, Rule::None));
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

// ---- The survivors numbered (fractal_count.hpp, list F, F-B) ----

namespace {

// acc += x c, for a 64-bit c (a binomial of up to 52 things).
void add_mul64(BigUint& acc, const BigUint& x, uint64_t c)
{
    if (c == 0 || x.is_zero()) return;
    if (c <= 0xffffffffull) acc.add_mul_small(x, uint32_t(c));
    else acc += BigUint::mul(x, BigUint(c));
}

// A signed count, as its positive and negative parts (one of them 0 once norm() has run): the kernel below is a
// sum with signs, and only the counts it gives in the end are known to be positive.
struct SInt
{
    BigUint p, n;
};
void norm(SInt& x)
{
    if (x.n.is_zero()) return;
    if (x.p >= x.n)
    {
        x.p -= x.n;
        x.n.set_zero();
    }
    else
    {
        BigUint t = std::move(x.n);
        t -= x.p;
        x.n = std::move(t);
        x.p.set_zero();
    }
}
struct SPoly
{
    Poly p, n;
};

// A word's symbols as digits of its alphabet, the first the most significant, and back.
BigUint word_rank_of(const std::string& w, const std::string& symbols)
{
    BigUint r;
    for (const char c : w)
    {
        r.mul_small(uint32_t(symbols.size()));
        r.add_small(uint32_t(symbols.find(c)));
    }
    return r;
}
std::string word_of(BigUint rank, uint32_t length, const std::string& symbols)
{
    std::string w(length, ' ');
    for (uint32_t i = length; i-- > 0;) w[i] = symbols[rank.divmod_small(uint32_t(symbols.size()))];
    return w;
}

// The k-symbol subsets of `from` (in order of code) in lexicographic order: the one at a place, and a subset's place.
std::string subset_at(const std::string& from, uint32_t k, uint64_t i)
{
    std::string out;
    size_t j = 0;
    for (uint32_t left = k; left > 0; --left)
        for (;; ++j)
        {
            const uint64_t block = choose(uint32_t(from.size() - j - 1), left - 1);
            if (i >= block)
            {
                i -= block;
                continue;
            }
            out += from[j++];
            break;
        }
    return out;
}
uint64_t subset_rank(const std::string& from, const std::string& sub)
{
    uint64_t r = 0;
    size_t j = 0;
    for (size_t p = 0; p < sub.size(); ++p)
    {
        const size_t at = from.find(sub[p]);
        for (; j < at; ++j) r += choose(uint32_t(from.size() - j - 1), uint32_t(sub.size() - p - 1));
        j = at + 1;
    }
    return r;
}

BigUint quotient(const BigUint& a, const BigUint& b)
{
    BigUint q, r;
    BigUint::divmod(a, b, q, r);
    return q;
}
uint64_t small_of(const BigUint& v) { return v.is_zero() ? 0 : v.limbs()[0]; } // (a value known to fit a limb)

} // namespace

uint32_t FractalBody::length() const
{
    uint32_t n = kAxiomOverhead + uint32_t(axiom.size());
    for (const Rule& r : rules) n += kRuleOverhead + uint32_t(r.word.size()) + (r.left ? 2 : 0) + (r.right ? 2 : 0);
    return n;
}

std::string FractalBody::text() const
{
    std::string t = "axiom " + axiom + "\n";
    for (const Rule& r : rules)
    {
        t += "rule ";
        if (r.left) t += std::string(1, r.left) + "<";
        t += r.symbol;
        if (r.right) t += ">" + std::string(1, r.right);
        t += " " + r.word + "\n";
    }
    return t;
}

FractalBodyKinds FractalBodyAlphabet::kinds() const
{
    FractalBodyKinds k;
    k.inert = uint32_t(symbols.size() - ruled.size());
    k.drawing = uint32_t(drawing.size()), k.fixed = uint32_t(fixed.size());
    k.renamable = uint32_t(ruled.size() - drawing.size() - fixed.size());
    k.substitution = substitution, k.word_most = word_most, k.context_letters = uint32_t(context.size()), k.many_rules = many_rules;
    return k;
}

FractalBodyAlphabet fractal_substitution_alphabet()
{
    FractalBodyAlphabet a;
    a.symbols = a.ruled = "0123456789abcdefghijklmnopqrstuvwxyz";
    a.substitution = true;
    a.word_most = uint32_t(kFractalWordMost);
    return a;
}
FractalBodyAlphabet fractal_lsystem_alphabet()
{
    FractalBodyAlphabet a;
    a.symbols = "+-ABCDEFGHIJKLMNOPQRSTUVWXYZ[]abcdefghijklmnopqrstuvwxyz|";
    a.ruled = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    a.drawing = "FG", a.fixed = "f";
    a.word_most = uint32_t(kFractalWordMost);
    return a;
}
FractalBodyAlphabet fractal_scaled_alphabet()
{
    FractalBodyAlphabet a = fractal_lsystem_alphabet();
    a.symbols = "\"'" + a.symbols;
    return a;
}
FractalBodyAlphabet fractal_v3_alphabet()
{
    FractalBodyAlphabet a;
    for (int c = 33; c < 127; ++c)
    {
        const char ch = char(c);
        if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9') || std::string("+-[]|!&^/\"'\\").find(ch) != std::string::npos) a.symbols += ch;
    }
    a.ruled = a.context = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    a.drawing = "FG", a.fixed = "f";
    a.word_most = uint32_t(kFractalWordMostV3);
    a.many_rules = true;
    return a;
}

struct FractalBodyNumbering::Impl
{
    FractalBodyAlphabet a;
    FractalBodyKinds kd;
    FractalStructureFilters f;
    uint32_t top = 0, I = 0, R = 0, W = 0, C = 0, Afull = 0;
    std::array<int, 256> kind{}; // -1: no rule; 0 drawing, 1 fixed, 2 renamable
    std::array<int, 256> sym{};  // its place among the symbols, -1 for none
    std::array<int, 256> ctx{};  // its place among the context letters, -1 for none
    std::string ren;             // the renamable symbols in order of code (canonical's names, in the order it gives them)
    std::vector<Poly> inner_p;   // [s]: the bodies on a set of s symbols reaching all of it, passing grows
    std::vector<Poly> inner_n;   // [s]: those of them with no rule at all
    std::vector<Poly> outpow;    // [j]: the rules of j symbols never reached
    std::vector<BigUint> fact;   // [n]: n!
    std::vector<BigUint> powA;   // [n]: Afull^n
    std::vector<BigUint> counts; // [r]
    Rule rin = Rule::Any, rflat = Rule::Short, rout = Rule::Any;

    // A reached set's table: g[regime][w][k] = rho_{I+w}^k times the kernel K(s, w), w from 0 to s and k from 0
    // to w, regime 0 the reached symbols' rules as filtered, 1 those of one symbol at most (grows' flat ones).
    // K(s, s) = 1 and K(s, w) = -sum over v > w of C(s - w, v - w) rho_{I+v}^(v-w) K(s, v): the counting by
    // sets' recursion (inner(t) = all(t) less its smaller reaches), solved once for s whatever the roots, so that
    // the bodies on S of D met symbols, roots to finish (a partial word and Qp symbols waiting), reaching all of
    // the rest of S, are sum over u of C(s - D, u) [partial * rho_{I+D+u}^(Qp+u) * K(s, D + u)]_r.
    struct Tables
    {
        uint32_t s = 0;
        std::vector<std::vector<SPoly>> g[2];
    };
    mutable std::mutex mx;
    mutable std::vector<std::shared_ptr<const Tables>> kept;                       // the latest last, two at most
    mutable std::map<uint32_t, std::shared_ptr<const std::vector<BigUint>>> conv; // [r] -> [s]: a set's bodies of r

    Impl(FractalBodyAlphabet alph, uint32_t t, const FractalStructureFilters& fl) : a(std::move(alph)), f(fl), top(t)
    {
        kd = a.kinds();
        I = kd.inert, R = kd.ruled(), W = kd.word_most, C = kd.context_letters, Afull = kd.alphabet();
        kind.fill(-1), sym.fill(-1), ctx.fill(-1);
        for (size_t i = 0; i < a.symbols.size(); ++i) sym[uint8_t(a.symbols[i])] = int(i);
        for (size_t i = 0; i < a.context.size(); ++i) ctx[uint8_t(a.context[i])] = int(i);
        for (const char c : a.ruled)
        {
            const int k = a.drawing.find(c) != std::string::npos ? 0 : a.fixed.find(c) != std::string::npos ? 1 : 2;
            kind[uint8_t(c)] = k;
            if (k == 2) ren += c;
        }
        rin = in_rule(f), rflat = flat_rule(f), rout = out_rule(f);
        std::vector<Poly> in = inner_polys(kd, top, rin);
        const std::vector<Poly> flat = f.grows ? inner_polys(kd, top, rflat) : std::vector<Poly>{};
        inner_n = inner_polys(kd, top, Rule::None);
        counts = passing_from(kd, top, f, in, flat, inner_n);
        if (f.grows)
            for (uint32_t s = 0; s <= R; ++s)
                for (uint32_t k = 0; k <= top; ++k) in[s][k] -= flat[s][k];
        inner_p = std::move(in);
        const RuleMul out(Afull, kd, rout);
        Poly unit(top + 1);
        unit[0] = BigUint(1);
        outpow.push_back(unit);
        for (uint32_t j = 1; j <= R; ++j)
        {
            Poly p = outpow.back();
            out.apply(p);
            outpow.push_back(std::move(p));
        }
        fact.assign(R + 1, BigUint(1));
        for (uint32_t n = 2; n <= R; ++n) fact[n] = fact[n - 1], fact[n].mul_small(n);
        powA.assign(top + 1, BigUint(1));
        for (uint32_t n = 1; n <= top; ++n) powA[n] = powA[n - 1], powA[n].mul_small(Afull);
    }

    bool draws_ok(uint32_t s, uint32_t sd) const { return !f.draws || (a.substitution ? s >= 2 : sd >= 1); }

    std::shared_ptr<const Tables> tables(uint32_t s) const
    {
        std::lock_guard<std::mutex> lock(mx);
        for (const auto& t : kept)
            if (t->s == s) return t;
        auto t = std::make_shared<Tables>();
        t->s = s;
        for (int reg = 0; reg < (f.grows ? 2 : 1); ++reg)
        {
            const Rule mode = reg ? rflat : rin;
            auto& g = t->g[reg];
            g.resize(s + 1);
            for (uint32_t w = s + 1; w-- > 0;)
            {
                SPoly k{Poly(top + 1), Poly(top + 1)};
                if (w == s) k.p[0] = BigUint(1);
                else
                    for (uint32_t v = w + 1; v <= s; ++v)
                    {
                        const uint64_t c = choose(s - w, v - w);
                        const SPoly& h = g[v][v - w];
                        for (uint32_t x = 0; x <= top; ++x)
                        {
                            add_mul64(k.n[x], h.p[x], c); // (the kernel's sum is taken away)
                            add_mul64(k.p[x], h.n[x], c);
                        }
                    }
                for (uint32_t x = 0; x <= top; ++x)
                {
                    SInt v{std::move(k.p[x]), std::move(k.n[x])};
                    norm(v);
                    k.p[x] = std::move(v.p), k.n[x] = std::move(v.n);
                }
                g[w].resize(w + 1);
                g[w][0] = std::move(k);
                const RuleMul rho(I + w, kd, mode);
                for (uint32_t j = 1; j <= w; ++j)
                {
                    SPoly next = g[w][j - 1];
                    rho.apply(next.p);
                    rho.apply(next.n);
                    for (uint32_t x = 0; x <= top; ++x)
                    {
                        SInt v{std::move(next.p[x]), std::move(next.n[x])};
                        norm(v);
                        next.p[x] = std::move(v.p), next.n[x] = std::move(v.n);
                    }
                    g[w][j] = std::move(next);
                }
            }
        }
        kept.push_back(t);
        if (kept.size() > 2) kept.erase(kept.begin());
        return t;
    }

    // [s]: one labelled set's bodies of r characters (its reached part's and the rest's), before canonical divides
    // them by the renamable ones' orders.
    std::shared_ptr<const std::vector<BigUint>> per_set(uint32_t r) const
    {
        {
            std::lock_guard<std::mutex> lock(mx);
            if (const auto it = conv.find(r); it != conv.end()) return it->second;
        }
        auto v = std::make_shared<std::vector<BigUint>>(R + 1);
        for (uint32_t s = 0; s <= R; ++s)
        {
            BigUint t;
            for (uint32_t x = 0; x <= r; ++x)
                if (!inner_p[s][x].is_zero() && !outpow[R - s][r - x].is_zero()) t += BigUint::mul(inner_p[s][x], outpow[R - s][r - x]);
            if (!f.grows) t -= inner_n[s][r];
            (*v)[s] = std::move(t);
        }
        std::lock_guard<std::mutex> lock(mx);
        if (conv.size() >= 64) conv.clear();
        conv.emplace(r, v);
        return v;
    }

    // The reached set's own bodies of r_in characters (and, below r, the rest's of r - r_in), for a set of s with
    // sm renamable ones.
    BigUint reached_part(uint32_t s, uint32_t sm, uint32_t x, uint32_t r) const
    {
        BigUint c = BigUint::mul(inner_p[s][x], outpow[R - s][r - x]);
        if (x == r && !f.grows) c -= inner_n[s][r];
        return f.canonical ? quotient(c, fact[sm]) : c;
    }

    // The rules of the symbols never reached: one symbol's list of exactly l characters at a place, and back.
    std::vector<FractalBody::Rule> list_at(char y, uint32_t l, BigUint q) const;
    BigUint list_rank(const std::vector<FractalBody::Rule>& rules) const;
    BigUint tail(uint32_t o, uint32_t l) const; // the first rule of a list of l, its contexts taking o
};

// A list of rules of a symbol never reached, of exactly l characters: no rule; or (v1) one; or (v3) a first rule
// by its contexts (none first, then each letter, the left one first), its word's length and its word, then the
// rest of the list.
BigUint FractalBodyNumbering::Impl::tail(uint32_t o, uint32_t l) const
{
    BigUint t;
    for (uint32_t n = 1; n <= W && o + n <= l; ++n)
        if (!outpow[1][l - o - n].is_zero()) t += BigUint::mul(powA[n], outpow[1][l - o - n]);
    return t;
}

std::vector<FractalBody::Rule> FractalBodyNumbering::Impl::list_at(char y, uint32_t l, BigUint q) const
{
    std::vector<FractalBody::Rule> out;
    while (l > 0)
    {
        FractalBody::Rule rule;
        rule.symbol = y;
        uint32_t o = kRuleOverhead;
        if (C)
        {
            const BigUint c8 = a.many_rules ? tail(8, l) : BigUint(), c10 = tail(10, l), c12 = tail(12, l);
            BigUint none = c8;
            add_mul64(none, c10, C);
            if (q < none)
            {
                if (q >= c8)
                {
                    q -= c8;
                    BigUint k, rem;
                    BigUint::divmod(q, c10, k, rem);
                    rule.right = a.context[size_t(small_of(k))], q = std::move(rem), o = 10;
                }
            }
            else
            {
                q -= none;
                BigUint left = c10;
                add_mul64(left, c12, C);
                BigUint k, rem;
                BigUint::divmod(q, left, k, rem);
                rule.left = a.context[size_t(small_of(k))], q = std::move(rem), o = 10;
                if (q >= c10)
                {
                    q -= c10;
                    BigUint::divmod(q, c12, k, rem);
                    rule.right = a.context[size_t(small_of(k))], q = std::move(rem), o = 12;
                }
            }
        }
        for (uint32_t n = 1;; ++n)
        {
            if (n > W || o + n > l) throw std::logic_error("fractal numbering: a list's place past its length's");
            const BigUint& rest = outpow[1][l - o - n];
            if (rest.is_zero()) continue;
            const BigUint block = BigUint::mul(powA[n], rest);
            if (q >= block)
            {
                q -= block;
                continue;
            }
            BigUint w, rem;
            BigUint::divmod(q, rest, w, rem);
            rule.word = word_of(w, n, a.symbols);
            q = std::move(rem), l -= o + n;
            break;
        }
        out.push_back(std::move(rule));
        if (!a.many_rules && l) throw std::logic_error("fractal numbering: a second rule for a v1 or v2 symbol");
    }
    return out;
}

BigUint FractalBodyNumbering::Impl::list_rank(const std::vector<FractalBody::Rule>& rules) const
{
    uint32_t l = 0;
    for (const auto& r : rules) l += kRuleOverhead + uint32_t(r.word.size()) + (r.left ? 2 : 0) + (r.right ? 2 : 0);
    BigUint q;
    for (const auto& rule : rules)
    {
        const uint32_t o = kRuleOverhead + (rule.left ? 2 : 0) + (rule.right ? 2 : 0);
        if (C)
        {
            const BigUint c8 = a.many_rules ? tail(8, l) : BigUint(), c10 = tail(10, l), c12 = tail(12, l);
            if (rule.left)
            {
                BigUint none = c8;
                add_mul64(none, c10, C);
                q += none;
                BigUint left = c10;
                add_mul64(left, c12, C);
                add_mul64(q, left, uint64_t(ctx[uint8_t(rule.left)]));
                if (rule.right)
                {
                    q += c10;
                    add_mul64(q, c12, uint64_t(ctx[uint8_t(rule.right)]));
                }
            }
            else if (rule.right)
            {
                q += c8;
                add_mul64(q, c10, uint64_t(ctx[uint8_t(rule.right)]));
            }
        }
        const uint32_t n = uint32_t(rule.word.size());
        for (uint32_t m = 1; m < n; ++m)
            if (o + m <= l && !outpow[1][l - o - m].is_zero()) q += BigUint::mul(powA[m], outpow[1][l - o - m]);
        q += BigUint::mul(word_rank_of(rule.word, a.symbols), outpow[1][l - o - n]);
        l -= o + n;
    }
    return q;
}

namespace {

// The reached part of a body, walked in the order the axiom reaches it (fractal_count.hpp): to find the body at a
// place among a reached set's (unrank) or a body's place (rank), choice by choice, each choice's count being what
// can follow it.
class ReachWalk
{
public:
    using Impl = FractalBodyNumbering::Impl;
    ReachWalk(const Impl& m, const Impl::Tables& t, const std::string& set, bool need_rule) : m_(m), t_(t), s_(uint32_t(set.size())), need_rule_(need_rule)
    {
        in_.fill(false), met_.fill(false);
        for (const char c : set)
        {
            in_[uint8_t(c)] = true;
            if (m.kind[uint8_t(c)] == 2) set_ren_ += c;
        }
        mr_ = uint32_t(set_ren_.size());
    }

    // The body of the reached part at place k (axiom and its rules) in exactly r characters.
    FractalBody at(uint32_t r, BigUint k)
    {
        FractalBody b;
        go(r, &k, nullptr, b);
        if (!k.is_zero()) throw std::logic_error("fractal numbering: a place past its reached part's");
        return b;
    }
    // The place of a body's reached part (its axiom and the rules of the symbols it reaches).
    BigUint rank(const FractalBody& given)
    {
        BigUint k;
        FractalBody b;
        go(given.length(), &k, &given, b);
        return k;
    }

private:
    enum class Part { None, Axiom, At, Rule };
    struct State
    {
        Part p;
        uint32_t D, Qp, n;
        bool selfish, g, any_rule;
        uint32_t mr, r;
    };

    const SPoly& G(int reg, uint32_t w, uint32_t k) const { return t_.g[reg][w][k]; }

    // T[r'] = sum over j of A^j G[r' - j], j to W - (c - r'): what a word of c - r' symbols so far can end with,
    // its symbols' characters and what follows them taking r' in all (kept for a word, as c stays the same along it).
    const std::vector<SInt>& T(int reg, uint32_t w, uint32_t k, uint32_t c)
    {
        const auto key = std::make_tuple(reg, w, k, c);
        if (const auto it = memo_.find(key); it != memo_.end()) return it->second;
        const SPoly& g = G(reg, w, k);
        const uint32_t A = m_.I + w;
        std::vector<SInt> v(c + 1);
        for (uint32_t rp = 0; rp <= c; ++rp)
        {
            const uint32_t n = c - rp;
            if (n > m_.W) continue;
            SInt x{g.p[rp], g.n[rp]};
            if (n < m_.W && rp >= 1)
            {
                x.p.add_mul_small(v[rp - 1].p, A);
                x.n.add_mul_small(v[rp - 1].n, A);
                norm(x);
            }
            v[rp] = std::move(x);
        }
        return memo_.emplace(key, std::move(v)).first->second;
    }

    // One term of the sum: u of S's unmet symbols allowed (w = D + u), in a regime.
    SInt value(int reg, const State& q, uint32_t u)
    {
        const uint32_t w = q.D + u, A = m_.I + w, base = q.Qp + u;
        auto at = [&](const SPoly& g, uint32_t x) { return SInt{g.p[x], g.n[x]}; };
        auto times = [&](SInt v, uint32_t a) {
            v.p.mul_small(a), v.n.mul_small(a);
            return v;
        };
        switch (q.p)
        {
        case Part::None: return at(G(reg, w, base), q.r);
        case Part::At: return at(G(reg, w, base + 1), q.r);
        case Part::Axiom:
            if (q.n == 0) return q.r == 0 ? SInt{} : times(T(reg, w, base, q.r)[q.r - 1], A);
            return T(reg, w, base, q.n + q.r)[q.r];
        case Part::Rule:
        {
            const uint32_t k = base + (m_.a.many_rules ? 1 : 0);
            const SPoly& g = G(reg, w, k);
            const bool no_self = m_.f.no_idle;
            if (reg == 1)
            {
                // (a word of one symbol at most)
                if (q.n == 0) return q.r == 0 ? SInt{} : times(at(g, q.r - 1), A - (no_self ? 1 : 0));
                if (q.n == 1) return q.selfish && no_self ? SInt{} : at(g, q.r);
                return {};
            }
            if (q.n == 0)
            {
                if (q.r == 0) return {};
                SInt v = times(T(reg, w, k, q.r)[q.r - 1], A);
                if (no_self) // (the word that is the symbol itself)
                {
                    v.p += g.n[q.r - 1];
                    v.n += g.p[q.r - 1];
                    norm(v);
                }
                return v;
            }
            if (q.n == 1 && q.selfish && no_self) return q.r == 0 ? SInt{} : times(T(reg, w, k, q.r + 1)[q.r - 1], A);
            return T(reg, w, k, q.n + q.r)[q.r];
        }
        }
        return {};
    }

    BigUint sum(int reg, const State& q)
    {
        const uint32_t mm = s_ - q.D;
        SInt total;
        for (uint32_t u = 0; u <= mm; ++u)
        {
            const SInt v = value(reg, q, u);
            const uint64_t c = choose(mm, u);
            add_mul64(total.p, v.p, c);
            add_mul64(total.n, v.n, c);
        }
        norm(total);
        if (!total.n.is_zero()) throw std::logic_error("fractal numbering: a count below 0");
        return std::move(total.p);
    }

    // The bodies with no rule at all from here: the axiom's remaining symbols meeting the rest of S.
    BigUint sum_none(const State& q) const
    {
        const uint32_t mm = s_ - q.D;
        SInt total;
        for (uint32_t u = 0; u <= mm; ++u)
        {
            BigUint v;
            if (q.p == Part::Axiom) v = q.r >= (q.n == 0 ? 1u : 0u) && q.n + q.r <= m_.W ? BigUint::pow(m_.I + q.D + u, q.r) : BigUint();
            else if (q.p != Part::Rule && q.r == 0) v = BigUint(1);
            add_mul64((mm - u) % 2 ? total.n : total.p, v, choose(mm, u));
        }
        norm(total);
        return std::move(total.p);
    }

    // What can follow a state: every way to finish the reached part, passing the filters.
    BigUint comp(const State& q)
    {
        BigUint t = sum(0, q);
        if (m_.f.grows && !q.g) t -= sum(1, q);
        else if (need_rule_ && !q.any_rule) t -= sum_none(q);
        if (m_.f.canonical && q.mr > 1)
        {
            BigUint d, rem;
            BigUint::divmod(t, m_.fact[q.mr], d, rem);
            if (!rem.is_zero()) throw std::logic_error("fractal numbering: canonical's share not whole");
            t = std::move(d);
        }
        return t;
    }

    bool old(char c) const { return m_.kind[uint8_t(c)] < 0 || met_[uint8_t(c)]; }
    bool may_meet(char c) const
    {
        const int k = m_.kind[uint8_t(c)];
        if (k < 0 || met_[uint8_t(c)] || !in_[uint8_t(c)]) return false;
        return !m_.f.canonical || k != 2 || (ren_used_ < set_ren_.size() && set_ren_[ren_used_] == c);
    }
    void meet(char c)
    {
        met_[uint8_t(c)] = true;
        order_.push_back(c);
        ++D_;
        if (m_.kind[uint8_t(c)] == 2) --mr_, ++ren_used_;
    }

    // A word (the axiom, or a rule's of x): symbol by symbol, a word ended first, then each symbol in order of code.
    void word(Part p, char x, uint32_t P, uint32_t& r, bool& g, bool any_rule, BigUint* k, const std::string* given, std::string& out)
    {
        uint32_t n = 0;
        bool selfish = false;
        for (;;)
        {
            auto qp = [&]() { return p == Part::Axiom ? D_ : D_ - P - 1; };
            const bool can_end = n >= 1 && !(p == Part::Rule && m_.f.no_idle && selfish && n == 1);
            BigUint end;
            if (can_end)
            {
                const Part next = p == Part::Axiom ? Part::None : (m_.a.many_rules ? Part::At : Part::None);
                const bool g2 = p == Part::Rule ? (g || n >= 2) : g;
                end = comp({next, D_, qp(), 0, false, g2, any_rule, mr_, r});
                if (given ? out.size() == given->size() : *k < end)
                {
                    g = g2;
                    return;
                }
                if (!given) *k -= end;
                else *k += end;
            }
            if (n >= m_.W || r == 0) throw std::logic_error("fractal numbering: a word past its place");
            const bool g2 = p == Part::Rule ? (g || n + 1 >= 2) : g;
            const BigUint c_old = comp({p, D_, qp(), n + 1, false, g2, any_rule, mr_, r - 1});
            const BigUint c_self = p == Part::Rule && n == 0 ? comp({p, D_, qp(), 1, true, g2, any_rule, mr_, r - 1}) : BigUint();
            std::optional<BigUint> c_ren, c_other; // (meeting a new symbol, worked out when one may be met)
            auto count = [&](char c) -> BigUint {
                if (old(c)) return p == Part::Rule && n == 0 && c == x ? c_self : c_old;
                if (!may_meet(c)) return {};
                const bool r2 = m_.kind[uint8_t(c)] == 2;
                auto& slot = r2 ? c_ren : c_other;
                if (!slot) slot = comp({p, D_ + 1, qp() + 1, n + 1, false, g2, any_rule, mr_ - (r2 ? 1 : 0), r - 1});
                return *slot;
            };
            char chosen = 0;
            const char want = given ? (*given)[out.size()] : 0;
            for (const char c : m_.a.symbols)
            {
                if (given && c == want)
                {
                    if (count(c).is_zero()) throw std::invalid_argument("fractal numbering: a symbol the walk cannot take");
                    chosen = c;
                    break;
                }
                const BigUint cnt = count(c);
                if (cnt.is_zero()) continue;
                if (given) *k += cnt;
                else if (*k < cnt)
                {
                    chosen = c;
                    break;
                }
                else *k -= cnt;
            }
            if (!chosen) throw std::logic_error("fractal numbering: no symbol at a word's place");
            if (!old(chosen)) meet(chosen);
            selfish = p == Part::Rule && n == 0 && chosen == x;
            out += chosen, ++n, --r;
        }
    }

    void go(uint32_t r, BigUint* k, const FractalBody* given, FractalBody& b)
    {
        if (r < kAxiomOverhead + 1) throw std::logic_error("fractal numbering: a body too short");
        r -= kAxiomOverhead;
        bool g = false, any_rule = false;
        word(Part::Axiom, 0, 0, r, g, any_rule, k, given ? &given->axiom : nullptr, b.axiom);
        std::vector<std::vector<const FractalBody::Rule*>> given_rules(256);
        if (given)
            for (const auto& rule : given->rules) given_rules[uint8_t(rule.symbol)].push_back(&rule);
        for (uint32_t P = 0; P < D_; ++P)
        {
            const char x = order_[P];
            const auto& mine = given_rules[uint8_t(x)];
            for (size_t j = 0;; ++j)
            {
                const uint32_t Qp = D_ - P - 1;
                const BigUint stop = comp({Part::None, D_, Qp, 0, false, g, any_rule, mr_, r});
                const bool stops = given ? j == mine.size() : *k < stop;
                if (stops) break;
                if (given) *k += stop;
                else *k -= stop;
                // Its contexts: none first, then each letter, the left one first.
                auto rc = [&](uint32_t o) { return r >= o ? comp({Part::Rule, D_, Qp, 0, false, g, true, mr_, r - o}) : BigUint(); };
                FractalBody::Rule rule;
                rule.symbol = x;
                uint32_t o = kRuleOverhead;
                if (m_.C)
                {
                    const BigUint c8 = rc(8), c10 = rc(10), c12 = rc(12);
                    BigUint none = c8;
                    add_mul64(none, c10, m_.C);
                    BigUint left = c10;
                    add_mul64(left, c12, m_.C);
                    if (given)
                    {
                        const auto& gr = *mine[j];
                        rule.left = gr.left, rule.right = gr.right;
                        if (gr.left)
                        {
                            *k += none;
                            add_mul64(*k, left, uint64_t(m_.ctx[uint8_t(gr.left)]));
                            if (gr.right) *k += c10, add_mul64(*k, c12, uint64_t(m_.ctx[uint8_t(gr.right)]));
                        }
                        else if (gr.right) *k += c8, add_mul64(*k, c10, uint64_t(m_.ctx[uint8_t(gr.right)]));
                    }
                    else if (*k < none)
                    {
                        if (*k >= c8)
                        {
                            *k -= c8;
                            BigUint q, rem;
                            BigUint::divmod(*k, c10, q, rem);
                            rule.right = m_.a.context[size_t(small_of(q))], *k = std::move(rem);
                        }
                    }
                    else
                    {
                        *k -= none;
                        BigUint q, rem;
                        BigUint::divmod(*k, left, q, rem);
                        rule.left = m_.a.context[size_t(small_of(q))], *k = std::move(rem);
                        if (*k >= c10)
                        {
                            *k -= c10;
                            BigUint::divmod(*k, c12, q, rem);
                            rule.right = m_.a.context[size_t(small_of(q))], *k = std::move(rem);
                        }
                    }
                    o += (rule.left ? 2 : 0) + (rule.right ? 2 : 0);
                }
                else if (!given && *k >= rc(8)) throw std::logic_error("fractal numbering: a rule's place past its count");
                r -= o, any_rule = true;
                word(Part::Rule, x, P, r, g, true, k, given ? &mine[j]->word : nullptr, rule.word);
                b.rules.push_back(std::move(rule));
                if (!m_.a.many_rules) break;
            }
        }
        if (r != 0 || D_ != s_) throw std::logic_error("fractal numbering: a walk that did not reach its set");
    }

    const Impl& m_;
    const Impl::Tables& t_;
    uint32_t s_;
    bool need_rule_;
    std::array<bool, 256> in_{}, met_{};
    std::string set_ren_, order_;
    uint32_t D_ = 0, mr_ = 0, ren_used_ = 0;
    std::map<std::tuple<int, uint32_t, uint32_t, uint32_t>, std::vector<SInt>> memo_;
};

// The symbols the axiom reaches, in the order met (filters/fractals.cpp's reach, on a body).
std::string reach_of(const FractalBody& b, const std::array<int, 256>& kind)
{
    std::vector<std::vector<const std::string*>> rule(256);
    for (const auto& r : b.rules) rule[uint8_t(r.symbol)].push_back(&r.word);
    std::string met;
    std::array<bool, 256> seen{};
    auto meet = [&](const std::string& w) {
        for (const char c : w)
            if (!seen[uint8_t(c)])
            {
                seen[uint8_t(c)] = true;
                if (kind[uint8_t(c)] >= 0) met += c;
            }
    };
    meet(b.axiom);
    for (size_t k = 0; k < met.size(); ++k)
        for (const std::string* w : rule[uint8_t(met[k])]) meet(*w);
    return met;
}

} // namespace

FractalBodyNumbering::FractalBodyNumbering(FractalBodyAlphabet alphabet, uint32_t top, const FractalStructureFilters& filters)
    : impl_(std::make_unique<Impl>(std::move(alphabet), top, filters))
{
}
FractalBodyNumbering::~FractalBodyNumbering() = default;
uint32_t FractalBodyNumbering::top() const { return impl_->top; }
const FractalBodyAlphabet& FractalBodyNumbering::alphabet() const { return impl_->a; }
const BigUint& FractalBodyNumbering::count(uint32_t r) const
{
    static const BigUint zero;
    return r <= impl_->top ? impl_->counts[r] : zero;
}

FractalBody FractalBodyNumbering::at(uint32_t r, BigUint k) const
{
    const Impl& m = *impl_;
    if (r > m.top || k >= m.counts[r]) throw std::invalid_argument("past the last body of " + std::to_string(r) + " characters");
    const auto cv = m.per_set(r);
    const uint32_t Rd = m.kd.drawing, Rf = m.kd.fixed, Rm = m.kd.renamable;
    for (uint32_t s = 0; s <= m.R; ++s)
    {
        if ((*cv)[s].is_zero()) continue;
        for (uint32_t sd = 0; sd <= std::min(Rd, s); ++sd)
            for (uint32_t sf = 0; sf <= std::min(Rf, s - sd); ++sf)
            {
                const uint32_t sm = s - sd - sf;
                if (sm > Rm || !m.draws_ok(s, sd)) continue;
                const BigUint per = m.f.canonical ? quotient((*cv)[s], m.fact[sm]) : (*cv)[s];
                const uint64_t cm = m.f.canonical ? 1 : choose(Rm, sm), cf = choose(Rf, sf), labels = choose(Rd, sd) * cf * cm;
                BigUint block;
                add_mul64(block, per, labels);
                if (k >= block)
                {
                    k -= block;
                    continue;
                }
                // Which symbols: the drawing ones', then f's, then the renamable ones' choices.
                BigUint li, rest;
                BigUint::divmod(k, per, li, rest);
                uint64_t l = small_of(li);
                const uint64_t mi = l % cm, fi = (l / cm) % cf, di = l / cm / cf;
                std::string set = subset_at(m.a.drawing, sd, di) + subset_at(m.a.fixed, sf, fi) + (m.f.canonical ? m.ren.substr(0, sm) : subset_at(m.ren, sm, mi));
                std::sort(set.begin(), set.end());
                // The reached part's characters, then it, then the rest.
                for (uint32_t x = 0; x <= r; ++x)
                {
                    const BigUint c = m.reached_part(s, sm, x, r);
                    if (rest >= c)
                    {
                        rest -= c;
                        continue;
                    }
                    BigUint inner = rest, outer;
                    if (x < r) BigUint::divmod(rest, m.outpow[m.R - s][r - x], inner, outer);
                    const auto t = m.tables(s);
                    ReachWalk walk(m, *t, set, x == r && !m.f.grows);
                    FractalBody b = walk.at(x, inner);
                    // The symbols never reached, in order of code, the fewer characters first.
                    std::string never;
                    for (const char c : m.a.ruled)
                        if (set.find(c) == std::string::npos) never += c;
                    uint32_t left = r - x;
                    for (size_t j = 0; j < never.size(); ++j)
                    {
                        const auto& after = m.outpow[never.size() - j - 1];
                        for (uint32_t ll = 0;; ++ll)
                        {
                            if (ll > left) throw std::logic_error("fractal numbering: the rest's place past its count");
                            if (m.outpow[1][ll].is_zero() || after[left - ll].is_zero()) continue;
                            const BigUint blk = BigUint::mul(m.outpow[1][ll], after[left - ll]);
                            if (outer >= blk)
                            {
                                outer -= blk;
                                continue;
                            }
                            BigUint q, rem;
                            BigUint::divmod(outer, after[left - ll], q, rem);
                            for (auto& rule : m.list_at(never[j], ll, q)) b.rules.push_back(std::move(rule));
                            outer = std::move(rem), left -= ll;
                            break;
                        }
                    }
                    std::stable_sort(b.rules.begin(), b.rules.end(), [](const FractalBody::Rule& p, const FractalBody::Rule& q) { return uint8_t(p.symbol) < uint8_t(q.symbol); });
                    return b;
                }
                throw std::logic_error("fractal numbering: a set's place past its count");
            }
    }
    throw std::logic_error("fractal numbering: a place past its length's");
}

std::optional<BigUint> FractalBodyNumbering::index(const FractalBody& b) const
{
    const Impl& m = *impl_;
    // Of this alphabet, as the reader takes it.
    auto word_ok = [&](const std::string& w) {
        if (w.empty() || w.size() > m.W) return false;
        for (const char c : w)
            if (m.sym[uint8_t(c)] < 0) return false;
        return true;
    };
    if (!word_ok(b.axiom) || b.rules.empty()) return std::nullopt;
    for (size_t j = 0; j < b.rules.size(); ++j)
    {
        const auto& r = b.rules[j];
        if (m.kind[uint8_t(r.symbol)] < 0 || !word_ok(r.word)) return std::nullopt;
        if ((r.left && m.ctx[uint8_t(r.left)] < 0) || (r.right && m.ctx[uint8_t(r.right)] < 0)) return std::nullopt;
        if (j && uint8_t(b.rules[j - 1].symbol) > uint8_t(r.symbol)) return std::nullopt;
        if (j && !m.a.many_rules && b.rules[j - 1].symbol == r.symbol) return std::nullopt;
    }
    const uint32_t r = b.length();
    if (r > m.top) return std::nullopt;
    // Its reach, and the filters on it.
    const std::string met = reach_of(b, m.kind);
    std::array<bool, 256> in{};
    for (const char c : met) in[uint8_t(c)] = true;
    uint32_t sd = 0, sf = 0, sm = 0;
    for (const char c : met) ++(m.kind[uint8_t(c)] == 0 ? sd : m.kind[uint8_t(c)] == 1 ? sf : sm);
    const uint32_t s = uint32_t(met.size());
    if (!m.draws_ok(s, sd)) return std::nullopt;
    bool grows = false;
    for (const auto& rule : b.rules)
    {
        const bool reached = in[uint8_t(rule.symbol)];
        if (reached && rule.word.size() >= 2) grows = true;
        if (m.f.no_idle && (!reached || rule.word == std::string(1, rule.symbol))) return std::nullopt;
        if (m.f.canonical && !reached) return std::nullopt;
    }
    if (m.f.grows && !grows) return std::nullopt;
    if (m.f.canonical)
    {
        size_t next = 0;
        for (const char c : met)
            if (m.kind[uint8_t(c)] == 2 && (next >= m.ren.size() || m.ren[next++] != c)) return std::nullopt;
    }
    // The sets before its own.
    const auto cv = m.per_set(r);
    const uint32_t Rd = m.kd.drawing, Rf = m.kd.fixed, Rm = m.kd.renamable;
    BigUint index;
    for (uint32_t t = 0; t <= s; ++t)
        for (uint32_t td = 0; td <= std::min(Rd, t); ++td)
            for (uint32_t tf = 0; tf <= std::min(Rf, t - td); ++tf)
            {
                const uint32_t tm = t - td - tf;
                if (tm > Rm || !m.draws_ok(t, td)) continue;
                if (t == s && (td > sd || (td == sd && tf >= sf))) continue;
                const BigUint per = m.f.canonical ? quotient((*cv)[t], m.fact[tm]) : (*cv)[t];
                add_mul64(index, per, choose(Rd, td) * choose(Rf, tf) * (m.f.canonical ? 1 : choose(Rm, tm)));
            }
    std::string set = met, sd_s, sf_s, sm_s;
    std::sort(set.begin(), set.end());
    for (const char c : set) (m.kind[uint8_t(c)] == 0 ? sd_s : m.kind[uint8_t(c)] == 1 ? sf_s : sm_s) += c;
    const uint64_t cm = m.f.canonical ? 1 : choose(Rm, sm), cf = choose(Rf, sf);
    const uint64_t li = (subset_rank(m.a.drawing, sd_s) * cf + subset_rank(m.a.fixed, sf_s)) * cm + (m.f.canonical ? 0 : subset_rank(m.ren, sm_s));
    const BigUint per = m.f.canonical ? quotient((*cv)[s], m.fact[sm]) : (*cv)[s];
    add_mul64(index, per, li);
    // The reached part's characters, its place, and the rest's.
    FractalBody mine, rest;
    mine.axiom = b.axiom;
    for (const auto& rule : b.rules) (in[uint8_t(rule.symbol)] ? mine : rest).rules.push_back(rule);
    const uint32_t x = mine.length();
    for (uint32_t y = 0; y < x; ++y) index += m.reached_part(s, sm, y, r);
    const auto t = m.tables(s);
    ReachWalk walk(m, *t, set, x == r && !m.f.grows);
    const BigUint inner = walk.rank(mine);
    if (x == r)
    {
        index += inner;
        return index;
    }
    index += BigUint::mul(inner, m.outpow[m.R - s][r - x]);
    std::string never;
    for (const char c : m.a.ruled)
        if (!in[uint8_t(c)]) never += c;
    uint32_t left = r - x;
    BigUint outer;
    for (size_t j = 0; j < never.size(); ++j)
    {
        std::vector<FractalBody::Rule> its;
        for (const auto& rule : rest.rules)
            if (rule.symbol == never[j]) its.push_back(rule);
        uint32_t ll = 0;
        for (const auto& rule : its) ll += kRuleOverhead + uint32_t(rule.word.size()) + (rule.left ? 2 : 0) + (rule.right ? 2 : 0);
        const auto& after = m.outpow[never.size() - j - 1];
        for (uint32_t l2 = 0; l2 < ll; ++l2)
            if (!m.outpow[1][l2].is_zero() && !after[left - l2].is_zero()) outer += BigUint::mul(m.outpow[1][l2], after[left - l2]);
        outer += BigUint::mul(m.list_rank(its), after[left - ll]);
        left -= ll;
    }
    index += outer;
    return index;
}
} // namespace sieve
