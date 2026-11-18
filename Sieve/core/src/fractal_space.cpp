// Sieve — the fractal dimension's whole space (see fractal_space.hpp).
#include "sieve/fractal_space.hpp"

#include "sieve/fractal.hpp"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <optional>
#include <string_view>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace sieve {

namespace {

constexpr uint32_t kAxiomOverhead = 7; // "axiom " and the line feed
constexpr uint32_t kRuleOverhead = 8;  // "rule X " and the line feed
constexpr uint32_t kSubHeader = 30;    // "sieve-fractal-v1\n" and "substitution\n"
static_assert(sizeof("sieve-fractal-v2") == sizeof("sieve-fractal-v1")); // (v2's lines' lengths below count on it)

uint32_t digits_of(uint32_t a) { return a < 10 ? 1 : a < 100 ? 2 : 3; }
// The angles written with d digits: the first of them and how many there are (1-9, 10-99, 100-359).
uint32_t first_angle(uint32_t d) { return d == 1 ? 1 : d == 2 ? 10 : 100; }
uint32_t angles_with(uint32_t d) { return d == 1 ? 9 : d == 2 ? 90 : 260; }
uint32_t ls_header(uint32_t d) { return 26 + d; } // "sieve-fractal-v1\n", "lsystem ", the angle, "\n"

BigUint times(const BigUint& a, const BigUint& b) { return BigUint::mul(a, b); }

// A word's place among the words of its length (its symbols as digits, the first the most significant).
BigUint word_rank(const std::string& w, const std::string& symbols)
{
    BigUint r;
    for (const char c : w)
    {
        r.mul_small(uint32_t(symbols.size()));
        r.add_small(uint32_t(symbols.find(c)));
    }
    return r;
}

std::string word_at(BigUint rank, uint32_t length, const std::string& symbols)
{
    std::string w(length, ' ');
    for (uint32_t i = length; i-- > 0;) w[i] = symbols[rank.divmod_small(uint32_t(symbols.size()))];
    return w;
}

} // namespace

BigUint FractalSpace::Family::rules(size_t j, uint32_t r, bool any) const
{
    BigUint v = ways[j][r];
    if (!any && r == 0) v -= BigUint(1); // (no rule at all: refused)
    return v;
}

BigUint FractalSpace::Family::after_header(uint32_t r) const
{
    BigUint total;
    for (uint32_t n = 1; n <= kFractalWordMost && kAxiomOverhead + n <= r; ++n) total += times(words[n], rules(0, r - kAxiomOverhead - n, false));
    return total;
}

// ---- fractal-space-v3 (list X3): v3's rulesets counted by their parts (fractal_space.hpp) ----

namespace v3parts {

using Poly = std::vector<BigUint>; // [length]: how many texts of that length

// A part of a v3 ruleset's text: how many texts of each length it has, the one at a place, how far a
// text of it reaches from the start of a string, and a text's place among those of its length.
struct Part
{
    Poly n;
    virtual ~Part() = default;
    virtual void write(uint32_t len, BigUint i, std::string& out) const = 0;
    virtual std::optional<size_t> take(std::string_view s) const = 0;
    virtual BigUint rank(std::string_view s) const = 0;
};
using P = std::shared_ptr<const Part>;

thread_local uint32_t g_longest = 0; // (set while one space's parts are built: every Poly has longest + 1 places)

Poly zeros() { return Poly(size_t(g_longest) + 1); }

struct Lit : Part
{
    std::string s;
    explicit Lit(std::string t) : s(std::move(t))
    {
        n = zeros();
        if (s.size() <= g_longest) n[s.size()] = BigUint(1);
    }
    void write(uint32_t, BigUint, std::string& out) const override { out += s; }
    std::optional<size_t> take(std::string_view t) const override { return t.substr(0, s.size()) == s ? std::optional<size_t>(s.size()) : std::nullopt; }
    BigUint rank(std::string_view) const override { return BigUint(); }
};

// One of a set of symbols.
struct Chars : Part
{
    std::string cs; // in order of code
    explicit Chars(std::string c) : cs(std::move(c))
    {
        n = zeros();
        if (g_longest >= 1) n[1] = BigUint(uint64_t(cs.size()));
    }
    void write(uint32_t, BigUint i, std::string& out) const override { out += cs[i.mod_small(1u << 30)]; }
    std::optional<size_t> take(std::string_view t) const override { return !t.empty() && cs.find(t[0]) != std::string::npos ? std::optional<size_t>(1) : std::nullopt; }
    BigUint rank(std::string_view t) const override { return BigUint(uint64_t(cs.find(t[0]))); }
};

uint64_t pow10(uint32_t k)
{
    uint64_t v = 1;
    while (k--) v *= 10;
    return v;
}
uint32_t digits_in(uint64_t v) { return v < 10 ? 1 : 1 + digits_in(v / 10); }

// A decimal as v3 writes them: a sign or none, a whole part of no leading zero (at most max_whole), up to
// 6 decimals, the last not 0; more than 0; with only_zero, its whole part 0; with not_one, not 1. With
// zero_ok (v4's), 0 itself too (unsigned).
struct Num : Part
{
    bool signed_ = false, only_zero = false, not_one = false, zero_ok = false;
    uint32_t max_whole = 0;
    Num(bool sgn, uint32_t mw, bool oz, bool n1, bool z0 = false) : signed_(sgn), only_zero(oz), not_one(n1), zero_ok(z0), max_whole(mw)
    {
        n = zeros();
        for (int s = 0; s <= (signed_ ? 1 : 0); ++s)
            for (uint32_t d = 0; d <= 6; ++d)
                for (uint32_t w = 0; w <= max_whole; ++w)
                    if (valid(s, w, d))
                    {
                        const uint32_t len = uint32_t(s) + digits_in(w) + (d ? d + 1 : 0);
                        if (len <= g_longest) n[len] += BigUint(fracs(d));
                    }
    }
    bool valid(int s, uint32_t w, uint32_t d) const
    {
        return !(only_zero && w != 0) && !(w == 0 && d == 0 && !(zero_ok && s == 0)) && !(not_one && w == 1 && d == 0);
    }
    static uint64_t fracs(uint32_t d) { return d == 0 ? 1 : 9 * pow10(d - 1); }
    void write(uint32_t len, BigUint i, std::string& out) const override
    {
        uint64_t k = std::stoull(i.to_decimal());
        for (int s = 0; s <= (signed_ ? 1 : 0); ++s)
            for (uint32_t d = 0; d <= 6; ++d)
                for (uint32_t w = 0; w <= max_whole; ++w)
                {
                    if (!valid(s, w, d) || uint32_t(s) + digits_in(w) + (d ? d + 1 : 0) != len) continue;
                    if (k >= fracs(d))
                    {
                        k -= fracs(d);
                        continue;
                    }
                    out += (s ? "-" : "") + std::to_string(w);
                    if (d)
                    {
                        const uint64_t f = (k / 9) * 10 + k % 9 + 1; // (the k-th of d digits not ending in 0)
                        out += "." + std::to_string(pow10(d) + f).substr(1);
                    }
                    return;
                }
        throw std::logic_error("fractal-space-v3: a number's place past its length's");
    }
    std::optional<size_t> take(std::string_view t) const override
    {
        size_t k = 0;
        if (signed_ && !t.empty() && t[0] == '-') ++k;
        const size_t w0 = k;
        while (k < t.size() && t[k] >= '0' && t[k] <= '9') ++k;
        if (k == w0 || k - w0 > 3 || (k - w0 > 1 && t[w0] == '0')) return std::nullopt;
        const uint32_t w = uint32_t(std::stoul(std::string(t.substr(w0, k - w0))));
        uint32_t d = 0;
        if (k < t.size() && t[k] == '.')
        {
            size_t e = k + 1;
            while (e < t.size() && t[e] >= '0' && t[e] <= '9') ++e;
            d = uint32_t(e - k - 1);
            if (d == 0 || d > 6 || t[e - 1] == '0') return std::nullopt;
            k = e;
        }
        if (w > max_whole || !valid(signed_ && t[0] == '-' ? 1 : 0, w, d)) return std::nullopt;
        return k;
    }
    BigUint rank(std::string_view t) const override
    {
        const int sign = signed_ && t[0] == '-' ? 1 : 0;
        const std::string_view body = t.substr(size_t(sign));
        const size_t dot = body.find('.');
        const uint32_t w = uint32_t(std::stoul(std::string(body.substr(0, dot)))), d = dot == std::string_view::npos ? 0 : uint32_t(body.size() - dot - 1);
        const uint32_t len = uint32_t(t.size());
        uint64_t k = 0;
        for (int s = 0; s <= (signed_ ? 1 : 0); ++s)
            for (uint32_t dd = 0; dd <= 6; ++dd)
                for (uint32_t ww = 0; ww <= max_whole; ++ww)
                {
                    if (!valid(s, ww, dd) || uint32_t(s) + digits_in(ww) + (dd ? dd + 1 : 0) != len) continue;
                    if (s == sign && dd == d && ww == w)
                    {
                        if (d)
                        {
                            const uint64_t f = std::stoull(std::string(body.substr(dot + 1)));
                            k += (f / 10) * 9 + f % 10 - 1;
                        }
                        return BigUint(k);
                    }
                    k += fracs(dd);
                }
        throw std::logic_error("fractal-space-v3: a number not of its part");
    }
};

// A whole number lo to hi, no leading zero, in order.
struct Whole : Part
{
    uint64_t lo, hi;
    Whole(uint64_t l, uint64_t h) : lo(l), hi(h)
    {
        n = zeros();
        for (uint32_t k = 1; k <= 10; ++k)
        {
            const uint64_t a = std::max(lo, k == 1 ? 0 : pow10(k - 1)), b = std::min(hi, pow10(k) - 1);
            if (a <= b && k <= g_longest) n[k] = BigUint(b - a + 1);
        }
    }
    uint64_t first(uint32_t k) const { return std::max(lo, k == 1 ? 0 : pow10(k - 1)); }
    void write(uint32_t len, BigUint i, std::string& out) const override { out += std::to_string(first(len) + std::stoull(i.to_decimal())); }
    std::optional<size_t> take(std::string_view t) const override
    {
        size_t k = 0;
        while (k < t.size() && t[k] >= '0' && t[k] <= '9') ++k;
        if (k == 0 || k > 10 || (k > 1 && t[0] == '0')) return std::nullopt;
        const uint64_t v = std::stoull(std::string(t.substr(0, k)));
        return v >= lo && v <= hi ? std::optional<size_t>(k) : std::nullopt;
    }
    BigUint rank(std::string_view t) const override { return BigUint(std::stoull(std::string(t)) - first(uint32_t(t.size()))); }
};

// A word of 1 to `most` symbols of an alphabet (in order of code).
struct Word : Part
{
    std::string a;
    uint32_t most;
    Word(std::string alphabet, uint32_t m) : a(std::move(alphabet)), most(m)
    {
        n = zeros();
        BigUint v(1);
        for (uint32_t k = 1; k <= std::min(most, g_longest); ++k) v.mul_small(uint32_t(a.size())), n[k] = v;
    }
    void write(uint32_t len, BigUint i, std::string& out) const override { out += word_at(i, len, a); }
    std::optional<size_t> take(std::string_view t) const override
    {
        size_t k = 0;
        while (k < t.size() && a.find(t[k]) != std::string::npos) ++k;
        return k >= 1 && k <= most ? std::optional<size_t>(k) : std::nullopt;
    }
    BigUint rank(std::string_view t) const override { return word_rank(std::string(t), a); }
};

// One or more symbols of an alphabet, in order of code, each once: the k-symbol choices in order.
struct Choice : Part
{
    std::string a;
    std::vector<std::vector<BigUint>> c; // c[m][k]: ways to choose k of m
    explicit Choice(std::string alphabet) : a(std::move(alphabet))
    {
        const size_t m = a.size();
        c.assign(m + 1, std::vector<BigUint>(m + 1));
        for (size_t i = 0; i <= m; ++i)
        {
            c[i][0] = BigUint(1);
            for (size_t k = 1; k <= i; ++k) c[i][k] = c[i - 1][k - 1], c[i][k] += c[i - 1][k];
        }
        n = zeros();
        for (size_t k = 1; k <= m && k <= g_longest; ++k) n[k] = c[m][k];
    }
    void write(uint32_t len, BigUint i, std::string& out) const override
    {
        size_t from = 0;
        for (uint32_t k = len; k > 0; --k)
            for (size_t j = from;; ++j)
            {
                const BigUint& block = c[a.size() - j - 1][k - 1]; // (choices with symbol j first)
                if (i >= block)
                {
                    i -= block;
                    continue;
                }
                out += a[j], from = j + 1;
                break;
            }
    }
    std::optional<size_t> take(std::string_view t) const override
    {
        size_t k = 0;
        while (k < t.size() && a.find(t[k]) != std::string::npos && (k == 0 || t[k - 1] < t[k])) ++k;
        return k ? std::optional<size_t>(k) : std::nullopt;
    }
    BigUint rank(std::string_view t) const override
    {
        BigUint r;
        size_t from = 0;
        for (size_t p = 0; p < t.size(); ++p)
        {
            const size_t j = a.find(t[p]);
            for (size_t q = from; q < j; ++q) r += c[a.size() - q - 1][t.size() - p - 1];
            from = j + 1;
        }
        return r;
    }
};

struct Seq : Part
{
    P a, b;
    std::vector<uint32_t> nz; // (a's lengths with texts)
    Seq(P x, P y) : a(std::move(x)), b(std::move(y))
    {
        n = zeros();
        for (uint32_t i = 0; i <= g_longest; ++i)
            if (!a->n[i].is_zero()) nz.push_back(i);
        std::vector<uint32_t> nzb;
        for (uint32_t i = 0; i <= g_longest; ++i)
            if (!b->n[i].is_zero()) nzb.push_back(i);
        for (const uint32_t i : nz)
            for (const uint32_t j : nzb)
            {
                if (i + j > g_longest) break;
                n[i + j] += BigUint::mul(a->n[i], b->n[j]);
            }
    }
    void write(uint32_t len, BigUint i, std::string& out) const override
    {
        for (const uint32_t la : nz)
        {
            if (la > len) break;
            const BigUint& nb = b->n[len - la];
            if (nb.is_zero()) continue;
            const BigUint block = BigUint::mul(a->n[la], nb);
            if (i >= block)
            {
                i -= block;
                continue;
            }
            BigUint q, r;
            BigUint::divmod(i, nb, q, r);
            a->write(la, q, out);
            b->write(len - la, r, out);
            return;
        }
        throw std::logic_error("fractal-space-v3: a place past its length's");
    }
    std::optional<size_t> take(std::string_view t) const override
    {
        const auto x = a->take(t);
        if (!x) return std::nullopt;
        const auto y = b->take(t.substr(*x));
        if (!y) return std::nullopt;
        return *x + *y;
    }
    BigUint rank(std::string_view t) const override
    {
        const size_t la = *a->take(t);
        const uint32_t len = uint32_t(t.size());
        BigUint r;
        for (const uint32_t l : nz)
        {
            if (l >= la) break;
            r += BigUint::mul(a->n[l], b->n[len - l]);
        }
        r += BigUint::mul(a->rank(t.substr(0, la)), b->n[len - la]);
        r += b->rank(t.substr(la));
        return r;
    }
};

P seq(std::vector<P> parts)
{
    P p = parts.back();
    for (size_t k = parts.size() - 1; k-- > 0;) p = std::make_shared<const Seq>(parts[k], p);
    return p;
}
P lit(std::string s) { return std::make_shared<const Lit>(std::move(s)); }

// Absent, or one of its part's.
struct Opt : Part
{
    P p;
    explicit Opt(P x) : p(std::move(x))
    {
        n = p->n;
        n[0] += BigUint(1);
    }
    void write(uint32_t len, BigUint i, std::string& out) const override
    {
        if (len == 0)
        {
            if (i.is_zero()) return;
            i -= BigUint(1);
        }
        p->write(len, i, out);
    }
    std::optional<size_t> take(std::string_view t) const override { return p->take(t).value_or(0); }
    BigUint rank(std::string_view t) const override { return t.empty() ? BigUint() : p->rank(t); }
};

// One of several, the first's texts of a length first.
struct Alt : Part
{
    std::vector<P> ps;
    explicit Alt(std::vector<P> x) : ps(std::move(x))
    {
        n = zeros();
        for (const P& p : ps)
            for (uint32_t i = 0; i <= g_longest; ++i) n[i] += p->n[i];
    }
    void write(uint32_t len, BigUint i, std::string& out) const override
    {
        for (const P& p : ps)
        {
            if (i >= p->n[len])
            {
                i -= p->n[len];
                continue;
            }
            p->write(len, i, out);
            return;
        }
        throw std::logic_error("fractal-space-v3: a choice's place past its length's");
    }
    std::optional<size_t> take(std::string_view t) const override
    {
        for (const P& p : ps)
            if (const auto k = p->take(t)) return k;
        return std::nullopt;
    }
    BigUint rank(std::string_view t) const override
    {
        BigUint r;
        for (const P& p : ps)
        {
            if (p->take(t) == t.size())
            {
                r += p->rank(t);
                return r;
            }
            r += p->n[t.size()];
        }
        throw std::logic_error("fractal-space-v3: a text none of whose choices it is");
    }
};

// None or more of a part's, one after another (its counts shared where the part's are another's).
struct Star : Part
{
    P p;
    Star(P x, const Poly* same) : p(std::move(x))
    {
        if (same)
        {
            n = *same;
            return;
        }
        n = zeros();
        n[0] = BigUint(1);
        for (uint32_t l = 1; l <= g_longest; ++l)
            for (uint32_t a = 1; a <= l; ++a)
                if (!p->n[a].is_zero() && !n[l - a].is_zero()) n[l] += BigUint::mul(p->n[a], n[l - a]);
    }
    void write(uint32_t len, BigUint i, std::string& out) const override
    {
        while (len > 0)
        {
            for (uint32_t a = 1;; ++a)
            {
                if (a > len) throw std::logic_error("fractal-space-v3: a repetition's place past its length's");
                if (p->n[a].is_zero() || n[len - a].is_zero()) continue;
                const BigUint block = BigUint::mul(p->n[a], n[len - a]);
                if (i >= block)
                {
                    i -= block;
                    continue;
                }
                BigUint q, r;
                BigUint::divmod(i, n[len - a], q, r);
                p->write(a, q, out);
                i = r, len -= a;
                break;
            }
        }
    }
    std::optional<size_t> take(std::string_view t) const override
    {
        size_t k = 0;
        while (const auto x = p->take(t.substr(k)))
        {
            if (*x == 0) break;
            k += *x;
        }
        return k;
    }
    BigUint rank(std::string_view t) const override
    {
        BigUint r;
        while (!t.empty())
        {
            const uint32_t len = uint32_t(t.size()), la = uint32_t(*p->take(t));
            for (uint32_t a = 1; a < la; ++a)
                if (!p->n[a].is_zero()) r += BigUint::mul(p->n[a], n[len - a]);
            r += BigUint::mul(p->rank(t.substr(0, la)), n[len - la]);
            t.remove_prefix(la);
        }
        return r;
    }
};

// Its part without its empty text (one rule at least).
struct NonEmpty : Part
{
    P p;
    explicit NonEmpty(P x) : p(std::move(x))
    {
        n = p->n;
        n[0] = BigUint();
    }
    void write(uint32_t len, BigUint i, std::string& out) const override { p->write(len, i, out); }
    std::optional<size_t> take(std::string_view t) const override
    {
        const auto k = p->take(t);
        return k && *k ? k : std::nullopt;
    }
    BigUint rank(std::string_view t) const override { return p->rank(t); }
};

// A v4 formula (sieve/fractal.hpp): tokens one space apart, in prefix, never more than
// kFractalAwaitedMost values awaited. g[p][len]: the texts of len characters that end what p awaited.
// Its texts of one length in this order: by the first token's kind (+ - * / ^ in the order of their
// codes, then abs conj cos exp log sin, then c i z, then a number), that token's length, its place among
// those, then the rest alike.
struct Formula : Part
{
    struct Tok
    {
        P part;
        uint32_t takes;
    };
    std::vector<Tok> toks;
    std::vector<Poly> g;
    // (v4's: abs conj cos exp log sin, and c i z; v6's (list X5) its own, each list in order of code)
    explicit Formula(P number, std::vector<const char*> unaries = {"abs", "conj", "cos", "exp", "log", "sin"}, std::vector<const char*> leaves = {"c", "i", "z"})
    {
        for (const char* b : {"*", "+", "-", "/", "^"}) toks.push_back({std::make_shared<const Lit>(b), 2});
        for (const char* u : unaries) toks.push_back({std::make_shared<const Lit>(u), 1});
        for (const char* l : leaves) toks.push_back({std::make_shared<const Lit>(l), 0});
        toks.push_back({std::move(number), 0});
        g.assign(kFractalAwaitedMost + 1, zeros());
        for (uint32_t len = 1; len <= g_longest; ++len)
            for (uint32_t p = 1; p <= kFractalAwaitedMost; ++p)
                for (const Tok& t : toks)
                {
                    const uint32_t np = p - 1 + t.takes;
                    if (np > kFractalAwaitedMost) continue;
                    for (uint32_t lt = 1; lt <= len; ++lt)
                    {
                        if (t.part->n[lt].is_zero()) continue;
                        if (np == 0)
                        {
                            if (lt == len) g[p][len] += t.part->n[lt];
                        }
                        else if (len >= lt + 2 && !g[np][len - lt - 1].is_zero()) g[p][len] += BigUint::mul(t.part->n[lt], g[np][len - lt - 1]);
                    }
                }
        n = g[1];
    }
    BigUint rest(uint32_t np, uint32_t len, uint32_t lt) const
    {
        if (np == 0) return lt == len ? BigUint(1) : BigUint();
        return len >= lt + 2 ? g[np][len - lt - 1] : BigUint();
    }
    void write_from(uint32_t p, uint32_t len, BigUint i, std::string& out) const
    {
        for (const Tok& t : toks)
        {
            const uint32_t np = p - 1 + t.takes;
            if (np > kFractalAwaitedMost) continue;
            for (uint32_t lt = 1; lt <= len; ++lt)
            {
                if (t.part->n[lt].is_zero()) continue;
                const BigUint r = rest(np, len, lt);
                if (r.is_zero()) continue;
                const BigUint block = BigUint::mul(t.part->n[lt], r);
                if (i >= block)
                {
                    i -= block;
                    continue;
                }
                BigUint q, rem;
                BigUint::divmod(i, r, q, rem);
                t.part->write(lt, q, out);
                if (np == 0) return;
                out += ' ';
                write_from(np, len - lt - 1, rem, out);
                return;
            }
        }
        throw std::logic_error("fractal-space-v4: a formula's place past its length's");
    }
    void write(uint32_t len, BigUint i, std::string& out) const override { write_from(1, len, std::move(i), out); }
    // The kind a token is (its whole text one of a kind's).
    std::optional<size_t> kind_of(std::string_view tk) const
    {
        for (size_t k = 0; k < toks.size(); ++k)
            if (toks[k].part->take(tk) == tk.size()) return k;
        return std::nullopt;
    }
    std::optional<size_t> take(std::string_view t) const override
    {
        uint32_t awaited = 1;
        size_t pos = 0;
        for (;;)
        {
            size_t end = pos;
            while (end < t.size() && t[end] != ' ' && t[end] != '\n') ++end;
            const auto k = kind_of(t.substr(pos, end - pos));
            if (end == pos || !k) return std::nullopt;
            awaited = awaited - 1 + toks[*k].takes;
            if (awaited == 0) return end;
            if (awaited > kFractalAwaitedMost || end >= t.size() || t[end] != ' ') return std::nullopt;
            pos = end + 1;
        }
    }
    BigUint rank(std::string_view t) const override
    {
        BigUint r;
        uint32_t p = 1;
        for (;;)
        {
            const uint32_t len = uint32_t(t.size());
            const size_t sp = t.find(' ');
            const std::string_view tk = t.substr(0, sp);
            const size_t kind = *kind_of(tk);
            const uint32_t ltk = uint32_t(tk.size());
            for (size_t k = 0; k <= kind; ++k)
            {
                const Tok& o = toks[k];
                const uint32_t np = p - 1 + o.takes;
                if (np > kFractalAwaitedMost) continue;
                for (uint32_t lt = 1; lt <= len && (k < kind || lt < ltk); ++lt)
                    if (!o.part->n[lt].is_zero()) r += BigUint::mul(o.part->n[lt], rest(np, len, lt));
            }
            const uint32_t np = p - 1 + toks[kind].takes;
            r += BigUint::mul(toks[kind].part->rank(tk), rest(np, len, ltk));
            if (np == 0) return r;
            t.remove_prefix(sp + 1), p = np;
        }
    }
};

} // namespace v3parts

struct FractalSpaceV3
{
    v3parts::P all;
};

namespace v3parts {

std::shared_ptr<const FractalSpaceV3> make_v3(uint32_t longest)
{
    g_longest = longest;
    const std::string letters = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    std::string symbols; // v3's, in order of code
    for (int c = 33; c < 127; ++c)
    {
        const char ch = char(c);
        if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9') || std::string("+-[]|!&^/\"'\\").find(ch) != std::string::npos) symbols += ch;
    }
    const P angle = std::make_shared<const Alt>(std::vector<P>{std::make_shared<const Num>(false, 359, false, false), seq({lit("360/"), std::make_shared<const Whole>(2, 9999)})});
    const P scale = std::make_shared<const Opt>(seq({lit("scale "), std::make_shared<const Num>(false, 0, true, false), lit("\n")}));
    const P seed = std::make_shared<const Opt>(seq({lit("seed "), std::make_shared<const Whole>(1, 4294967295ull), lit("\n")}));
    const P ignore = std::make_shared<const Opt>(seq({lit("ignore "), std::make_shared<const Choice>(symbols), lit("\n")}));
    const P turn = std::make_shared<const Num>(true, 359, false, false), factor = std::make_shared<const Num>(false, 999, false, true);
    std::vector<P> defines;
    for (char d = '0'; d <= '9'; ++d)
        defines.push_back(std::make_shared<const Opt>(std::make_shared<const Alt>(std::vector<P>{seq({lit(std::string("define ") + d + " turn "), turn, lit("\n")}),
                                                                                            seq({lit(std::string("define ") + d + " scale "), factor, lit("\n")})})));
    const P word = std::make_shared<const Word>(symbols, uint32_t(kFractalWordMostV3));
    const P letter = std::make_shared<const Chars>(letters);
    const P left = std::make_shared<const Opt>(seq({letter, lit("<")})), right = std::make_shared<const Opt>(seq({lit(">"), letter}));
    std::vector<P> each;
    const Poly* same = nullptr;
    for (const char s : letters)
    {
        const P rule = seq({lit("rule "), left, lit(std::string(1, s)), right, lit(" "), word, lit("\n")});
        each.push_back(std::make_shared<const Star>(rule, same));
        same = &each.back()->n; // (every symbol's rules counted alike)
    }
    const P rules = std::make_shared<const NonEmpty>(seq(each));
    std::vector<P> parts{lit(std::string(kFractalVersion3) + "\nlsystem "), angle, lit("\n"), scale, seed, ignore};
    parts.insert(parts.end(), defines.begin(), defines.end());
    parts.insert(parts.end(), {lit("axiom "), word, lit("\n"), rules});
    auto v = std::make_shared<FractalSpaceV3>();
    v->all = seq(parts);
    return v;
}

// fractal-space-v4's v4 rulesets (list X4): an IFS's, then the escape and converge formulas'.
std::shared_ptr<const FractalSpaceV3> make_v4(uint32_t longest)
{
    g_longest = longest;
    const P coef = std::make_shared<const Num>(true, 999, false, false, true);
    std::vector<P> map{lit("map ")};
    for (int k = 0; k < 6; ++k)
    {
        if (k) map.push_back(lit(" "));
        map.push_back(coef);
    }
    map.push_back(lit("\n"));
    const P ifs = seq({lit(std::string(kFractalVersion4) + "\nifs\n"), std::make_shared<const NonEmpty>(std::make_shared<const Star>(seq(map), nullptr))});
    const P formula = std::make_shared<const Formula>(std::make_shared<const Num>(true, 999, false, false));
    const P span = std::make_shared<const Num>(false, 999, false, false);
    const P julia = std::make_shared<const Opt>(seq({lit("julia "), coef, lit(" "), coef, lit("\n")}));
    const P view = std::make_shared<const Opt>(seq({lit("view "), coef, lit(" "), coef, lit(" "), span, lit("\n")}));
    const P kind = std::make_shared<const Alt>(std::vector<P>{lit("escape "), lit("converge ")});
    const P esc = seq({lit(std::string(kFractalVersion4) + "\n"), kind, formula, lit("\n"), julia, view});
    auto v = std::make_shared<FractalSpaceV3>();
    v->all = std::make_shared<const Alt>(std::vector<P>{ifs, esc});
    return v;
}

// fractal-space-v5's v5 rulesets (list Y2): the elementary automata's, then the life-like ones'.
std::shared_ptr<const FractalSpaceV3> make_v5(uint32_t longest)
{
    g_longest = longest;
    const P row = std::make_shared<const Word>("01", uint32_t(kFractalWordMost));
    const P head = lit(std::string(kFractalVersion5) + "\n");
    const P elementary = seq({head, lit("elementary "), std::make_shared<const Whole>(0, 255), lit("\n"), std::make_shared<const Opt>(seq({lit("start "), row, lit("\n")}))});
    const P counts = std::make_shared<const Opt>(std::make_shared<const Choice>("012345678"));
    const P rows = seq({row, std::make_shared<const Star>(seq({lit("/"), row}), nullptr)});
    const P life = seq({head, lit("life B"), counts, lit("/S"), counts, lit("\nstart "), rows, lit("\n")});
    auto v = std::make_shared<FractalSpaceV3>();
    v->all = std::make_shared<const Alt>(std::vector<P>{elementary, life});
    return v;
}

// fractal-space-v6's v6 rulesets (list X5): the attractors', then the flows', then the aggregates'.
std::shared_ptr<const FractalSpaceV3> make_v6(uint32_t longest)
{
    g_longest = longest;
    const P number = std::make_shared<const Num>(true, 999, false, false);
    const P coef = std::make_shared<const Num>(true, 999, false, false, true);
    const std::vector<const char*> unaries = {"abs", "cos", "exp", "log", "sin"};
    const P f2 = std::make_shared<const Formula>(number, unaries, std::vector<const char*>{"x", "y"});
    const P f3 = std::make_shared<const Formula>(number, unaries, std::vector<const char*>{"x", "y", "z"});
    const P head = lit(std::string(kFractalVersion6) + "\n");
    const P attractor = seq({head, lit("attractor\nx "), f2, lit("\ny "), f2, lit("\n"), std::make_shared<const Opt>(seq({lit("from "), coef, lit(" "), coef, lit("\n")}))});
    const P step = std::make_shared<const Num>(false, 999, false, false);
    const P flow = seq({head, lit("flow "), step, lit("\nx "), f3, lit("\ny "), f3, lit("\nz "), f3, lit("\n"),
                        std::make_shared<const Opt>(seq({lit("from "), coef, lit(" "), coef, lit(" "), coef, lit("\n")}))});
    const P aggregate = seq({head, lit("aggregate "), std::make_shared<const Whole>(1, 4294967295u), lit("\n")});
    auto v = std::make_shared<FractalSpaceV3>();
    v->all = std::make_shared<const Alt>(std::vector<P>{attractor, flow, aggregate});
    return v;
}

} // namespace v3parts

BigUint FractalSpace::count_v3() const { return total_v3_; }

FractalSpace::FractalSpace(uint32_t longest) : longest_(longest)
{
    if (longest < kFractalSpaceShortest || longest > kFractalSpaceLongestMost)
        throw std::invalid_argument("the longest ruleset is " + std::to_string(kFractalSpaceShortest) + " to " + std::to_string(kFractalSpaceLongestMost) + " characters");
    auto make = [&](Family& f, std::string symbols, std::string rule_symbols) {
        f.symbols = std::move(symbols), f.rule_symbols = std::move(rule_symbols);
        f.words.assign(kFractalWordMost + 1, BigUint(1));
        for (size_t n = 1; n <= kFractalWordMost; ++n)
        {
            f.words[n] = f.words[n - 1];
            f.words[n].mul_small(uint32_t(f.symbols.size()));
        }
        const size_t m = f.rule_symbols.size();
        f.ways.assign(m + 1, std::vector<BigUint>(longest + 1));
        f.ways[m][0] = BigUint(1);
        for (size_t j = m; j-- > 0;)
            for (uint32_t r = 0; r <= longest; ++r)
            {
                BigUint v = f.ways[j + 1][r];
                for (uint32_t n = 1; n <= kFractalWordMost && kRuleOverhead + n <= r; ++n)
                    if (!f.ways[j + 1][r - kRuleOverhead - n].is_zero()) v += times(f.words[n], f.ways[j + 1][r - kRuleOverhead - n]);
                f.ways[j][r] = std::move(v);
            }
    };
    // (each in the order of its character codes)
    std::string digits_letters = "0123456789abcdefghijklmnopqrstuvwxyz";
    make(sub_, digits_letters, digits_letters);
    const std::string letters = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    make(ls_, "+-ABCDEFGHIJKLMNOPQRSTUVWXYZ[]abcdefghijklmnopqrstuvwxyz|", letters);
    make(scaled_, "\"'+-ABCDEFGHIJKLMNOPQRSTUVWXYZ[]abcdefghijklmnopqrstuvwxyz|", letters);
    // v2's angles and scales by the width they are written in, each in order of value.
    for (uint32_t t = 1; t <= 3599; ++t)
    {
        const size_t w = fractal_angle_text(t).size();
        if (t % 10) tenths_[w].push_back(t), angles_[w].push_back(t);
        else angles_[w].push_back(t);
    }
    for (uint32_t s = 1; s <= 999; ++s) scales_[fractal_scale_text(s).size()].push_back(s);
    for (size_t w = 1; w < 6; ++w)
        std::sort(angles_[w].begin(), angles_[w].end()), std::sort(tenths_[w].begin(), tenths_[w].end()), std::sort(scales_[w].begin(), scales_[w].end());
    // "sieve-fractal-v2\n" and "lsystem " and the angle, " scale " and the ratio, "\n"
    for (uint32_t w = 1; w < 6; ++w)
        if (!tenths_[w].empty()) groups_.push_back({&ls_, 26 + w, &tenths_[w], nullptr});
    for (uint32_t w = 1; w < 6; ++w)
        for (uint32_t m = 1; m < 6; ++m)
            if (!angles_[w].empty() && !scales_[m].empty()) groups_.push_back({&scaled_, 33 + w + m, &angles_[w], &scales_[m]});
    by_length_.assign(longest + 1, BigUint());
    by_length_v2_.assign(longest + 1, BigUint());
    for (uint32_t l = kFractalSpaceShortest; l <= longest; ++l)
    {
        by_length_[l] = at_length(l, nullptr);
        total_v1_ += by_length_[l];
        by_length_v2_[l] = at_length_v2(l);
    }
    for (uint32_t l = kFractalSpaceShortest; l <= longest; ++l) total_v2_ += by_length_v2_[l];
    v3_ = v3parts::make_v3(longest);
    for (uint32_t l = kFractalSpaceShortest; l <= longest; ++l) total_v3_ += v3_->all->n[l];
    v4_ = v3parts::make_v4(longest);
    for (uint32_t l = 0; l <= longest; ++l) total_v4_ += v4_->all->n[l];
    v5_ = v3parts::make_v5(longest);
    for (uint32_t l = 0; l <= longest; ++l) total_v5_ += v5_->all->n[l];
    v6_ = v3parts::make_v6(longest);
    for (uint32_t l = 0; l <= longest; ++l) total_v6_ += v6_->all->n[l];
    total_ = total_v1_;
    total_ += total_v2_;
    total_ += total_v3_;
    total_ += total_v4_;
    total_ += total_v5_;
    total_ += total_v6_;
}

BigUint FractalSpace::at_length_v2(uint32_t l) const
{
    BigUint total;
    for (const Group& g : groups_)
    {
        if (l < g.header) continue;
        BigUint one = g.family->after_header(l - g.header);
        if (one.is_zero()) continue;
        total += times(one, BigUint(g.lines()));
    }
    return total;
}

// The rulesets of one length: the substitutions, then the L-systems, angle by angle (the angles of
// one width alike: their blocks in per_angle_block[d], the substitutions' in [0]).
BigUint FractalSpace::at_length(uint32_t l, std::vector<BigUint>* per_angle_block) const
{
    BigUint total;
    const BigUint sub = l >= kSubHeader ? sub_.after_header(l - kSubHeader) : BigUint();
    total += sub;
    if (per_angle_block) per_angle_block->assign(4, BigUint()), (*per_angle_block)[0] = sub;
    for (uint32_t d = 1; d <= 3; ++d)
    {
        if (l < ls_header(d)) continue;
        const BigUint one = ls_.after_header(l - ls_header(d));
        BigUint all = one;
        all.mul_small(angles_with(d));
        total += all;
        if (per_angle_block) (*per_angle_block)[d] = one;
    }
    return total;
}

BigUint FractalSpace::count_v1_passing(const FractalStructureFilters& filters) const
{
    const std::vector<BigUint> sub = fractal_bodies_passing(fractal_substitution_kinds(), longest_, filters);
    const std::vector<BigUint> ls = fractal_bodies_passing(fractal_lsystem_kinds(), longest_, filters);
    BigUint total;
    for (uint32_t l = kFractalSpaceShortest; l <= longest_; ++l)
    {
        if (l >= kSubHeader) total += sub[l - kSubHeader];
        for (uint32_t d = 1; d <= 3; ++d)
            if (l >= ls_header(d))
            {
                BigUint one = ls[l - ls_header(d)];
                one.mul_small(angles_with(d));
                total += one;
            }
    }
    return total;
}

BigUint FractalSpace::count_v2_passing(const FractalStructureFilters& filters) const
{
    const std::vector<BigUint> ls = fractal_bodies_passing(fractal_lsystem_kinds(), longest_, filters);
    const std::vector<BigUint> scaled = fractal_bodies_passing(fractal_scaled_kinds(), longest_, filters);
    BigUint total;
    for (uint32_t l = kFractalSpaceShortest; l <= longest_; ++l)
        for (const Group& g : groups_)
            if (l >= g.header)
                total += times((g.family == &scaled_ ? scaled : ls)[l - g.header], BigUint(g.lines()));
    return total;
}

BigUint FractalSpace::count_at(uint32_t length) const
{
    if (length >= by_length_.size()) return BigUint();
    BigUint n = by_length_[length];
    n += by_length_v2_[length];
    n += v3_->all->n[length];
    n += v4_->all->n[length];
    n += v5_->all->n[length];
    n += v6_->all->n[length];
    return n;
}

std::string FractalSpace::body_at(const Family& f, uint32_t r, BigUint i) const
{
    std::string text;
    // The axiom: the shorter first, then its word.
    for (uint32_t n = 1; n <= kFractalWordMost && kAxiomOverhead + n <= r; ++n)
    {
        const BigUint rest = f.rules(0, r - kAxiomOverhead - n, false);
        const BigUint block = times(f.words[n], rest);
        if (i >= block)
        {
            i -= block;
            continue;
        }
        BigUint q, rem;
        BigUint::divmod(i, rest, q, rem);
        text += "axiom " + word_at(q, n, f.symbols) + "\n";
        i = rem, r -= kAxiomOverhead + n;
        break;
    }
    // Its rules, symbol by symbol: a rule for it first (the shorter word first), then none.
    for (size_t j = 0; j < f.rule_symbols.size(); ++j)
    {
        for (uint32_t n = 1; n <= kFractalWordMost && kRuleOverhead + n <= r; ++n)
        {
            const BigUint rest = f.rules(j + 1, r - kRuleOverhead - n, true);
            const BigUint block = times(f.words[n], rest);
            if (i >= block)
            {
                i -= block;
                continue;
            }
            BigUint q, rem;
            BigUint::divmod(i, rest, q, rem);
            text += std::string("rule ") + f.rule_symbols[j] + " " + word_at(q, n, f.symbols) + "\n";
            i = rem, r -= kRuleOverhead + n;
            break;
        }
    }
    return text;
}

std::string FractalSpace::at(const BigUint& index) const
{
    if (index >= total_) throw std::invalid_argument("past the last ruleset of " + std::to_string(longest_) + " characters or fewer");
    BigUint i = index;
    BigUint v1v2 = total_v1_;
    v1v2 += total_v2_;
    BigUint v1v3 = v1v2;
    v1v3 += total_v3_;
    BigUint v1v4 = v1v3;
    v1v4 += total_v4_;
    BigUint v1v5 = v1v4;
    v1v5 += total_v5_;
    if (i >= v1v5)
    {
        // v6's: as v4's.
        i -= v1v5;
        uint32_t l = 0;
        while (i >= v6_->all->n[l]) i -= v6_->all->n[l++];
        std::string text;
        v6_->all->write(l, i, text);
        return text;
    }
    if (i >= v1v4)
    {
        // v5's: as v4's.
        i -= v1v4;
        uint32_t l = 0;
        while (i >= v5_->all->n[l]) i -= v5_->all->n[l++];
        std::string text;
        v5_->all->write(l, i, text);
        return text;
    }
    if (i >= v1v3)
    {
        // v4's: its length (from the shortest it has), then its parts' places.
        i -= v1v3;
        uint32_t l = 0;
        while (i >= v4_->all->n[l]) i -= v4_->all->n[l++];
        std::string text;
        v4_->all->write(l, i, text);
        return text;
    }
    if (i >= v1v2)
    {
        // v3's: its length, then its parts' places.
        i -= v1v2;
        uint32_t l = kFractalSpaceShortest;
        while (i >= v3_->all->n[l]) i -= v3_->all->n[l++];
        std::string text;
        v3_->all->write(l, i, text);
        return text;
    }
    if (i >= total_v1_)
    {
        // v2's: its length, then its family line's group, the line in it, and its body.
        i -= total_v1_;
        uint32_t l = kFractalSpaceShortest;
        while (i >= by_length_v2_[l]) i -= by_length_v2_[l++];
        for (const Group& g : groups_)
        {
            if (l < g.header) continue;
            const BigUint one = g.family->after_header(l - g.header);
            const BigUint all = times(one, BigUint(g.lines()));
            if (i >= all)
            {
                i -= all;
                continue;
            }
            BigUint q, rem;
            BigUint::divmod(i, one, q, rem);
            const uint64_t line = q.mod_small(1u << 31); // (below 2^22: 3,599 angles by 999 scales at most)
            const size_t per = g.scales ? g.scales->size() : 1;
            std::string text = std::string(kFractalVersion2) + "\nlsystem " + fractal_angle_text((*g.angles)[size_t(line / per)]);
            if (g.scales) text += " scale " + fractal_scale_text((*g.scales)[size_t(line % per)]);
            return text + "\n" + body_at(*g.family, l - g.header, rem);
        }
        throw std::logic_error("fractal-space-v2: an address past its groups");
    }
    uint32_t l = kFractalSpaceShortest;
    while (i >= by_length_[l]) i -= by_length_[l++];
    std::vector<BigUint> blocks;
    at_length(l, &blocks);
    std::string text = std::string(kFractalVersion) + "\n";
    if (i < blocks[0]) return text + "substitution\n" + body_at(sub_, l - kSubHeader, i);
    i -= blocks[0];
    for (uint32_t d = 1; d <= 3; ++d)
    {
        if (blocks[d].is_zero()) continue;
        BigUint all = blocks[d];
        all.mul_small(angles_with(d));
        if (i >= all)
        {
            i -= all;
            continue;
        }
        BigUint q, rem;
        BigUint::divmod(i, blocks[d], q, rem);
        text += "lsystem " + std::to_string(first_angle(d) + q.mod_small(1u << 31)) + "\n";
        return text + body_at(ls_, l - ls_header(d), rem);
    }
    throw std::logic_error("fractal-space-v1: an address past its angles");
}

BigUint FractalSpace::body_index(const Family& fam, uint32_t r, const Fractal& f) const
{
    BigUint index;
    const uint32_t an = uint32_t(f.axiom.size());
    for (uint32_t n = 1; n < an; ++n) index += times(fam.words[n], fam.rules(0, r - kAxiomOverhead - n, false));
    index += times(word_rank(f.axiom, fam.symbols), fam.rules(0, r - kAxiomOverhead - an, false));
    r -= kAxiomOverhead + an;
    size_t next = 0; // the next rule of the ruleset's
    for (size_t j = 0; j < fam.rule_symbols.size() && next < f.rules.size(); ++j)
    {
        if (f.rules[next].first != fam.rule_symbols[j])
        {
            // (no rule for it: after every ruleset with one)
            for (uint32_t n = 1; n <= kFractalWordMost && kRuleOverhead + n <= r; ++n) index += times(fam.words[n], fam.rules(j + 1, r - kRuleOverhead - n, true));
            continue;
        }
        const std::string& w = f.rules[next].second;
        const uint32_t wn = uint32_t(w.size());
        for (uint32_t n = 1; n < wn; ++n) index += times(fam.words[n], fam.rules(j + 1, r - kRuleOverhead - n, true));
        index += times(word_rank(w, fam.symbols), fam.rules(j + 1, r - kRuleOverhead - wn, true));
        r -= kRuleOverhead + wn, ++next;
    }
    return index;
}

std::optional<BigUint> FractalSpace::index_of(const std::string& text) const
{
    const auto f = read_fractal(text);
    if (!f || text.size() > longest_) return std::nullopt;
    const uint32_t l = uint32_t(text.size());
    BigUint index;
    if (f->version == 6)
    {
        index = total_v1_;
        index += total_v2_;
        index += total_v3_;
        index += total_v4_;
        index += total_v5_;
        for (uint32_t k = 0; k < l; ++k) index += v6_->all->n[k];
        index += v6_->all->rank(text);
        return index;
    }
    if (f->version == 5)
    {
        index = total_v1_;
        index += total_v2_;
        index += total_v3_;
        index += total_v4_;
        for (uint32_t k = 0; k < l; ++k) index += v5_->all->n[k];
        index += v5_->all->rank(text);
        return index;
    }
    if (f->version == 4)
    {
        index = total_v1_;
        index += total_v2_;
        index += total_v3_;
        for (uint32_t k = 0; k < l; ++k) index += v4_->all->n[k];
        index += v4_->all->rank(text);
        return index;
    }
    if (text.size() < kFractalSpaceShortest) return std::nullopt;
    if (f->version == 3)
    {
        index = total_v1_;
        index += total_v2_;
        for (uint32_t k = kFractalSpaceShortest; k < l; ++k) index += v3_->all->n[k];
        index += v3_->all->rank(text);
        return index;
    }
    if (f->version == 2)
    {
        index = total_v1_;
        for (uint32_t k = kFractalSpaceShortest; k < l; ++k) index += by_length_v2_[k];
        const uint32_t aw = uint32_t(fractal_angle_text(f->angle_tenths).size()), sw = f->scale ? uint32_t(fractal_scale_text(f->scale).size()) : 0;
        for (const Group& g : groups_)
        {
            if (l < g.header) continue;
            const BigUint one = g.family->after_header(l - g.header);
            const bool mine = (g.scales != nullptr) == (f->scale != 0) && fractal_angle_text(g.angles->front()).size() == aw &&
                              (!g.scales || fractal_scale_text(g.scales->front()).size() == sw);
            if (!mine)
            {
                index += times(one, BigUint(g.lines()));
                continue;
            }
            const auto a = std::lower_bound(g.angles->begin(), g.angles->end(), f->angle_tenths);
            uint64_t line = uint64_t(a - g.angles->begin());
            if (g.scales) line = line * g.scales->size() + uint64_t(std::lower_bound(g.scales->begin(), g.scales->end(), f->scale) - g.scales->begin());
            index += times(one, BigUint(line));
            index += body_index(*g.family, l - g.header, *f);
            return index;
        }
        return std::nullopt;
    }
    for (uint32_t k = kFractalSpaceShortest; k < l; ++k) index += by_length_[k];
    std::vector<BigUint> blocks;
    at_length(l, &blocks);
    const bool sub = f->family == Fractal::Family::Substitution;
    uint32_t r = 0;
    if (sub) r = l - kSubHeader;
    else
    {
        index += blocks[0];
        const uint32_t angle = f->angle_tenths / 10, d = digits_of(angle);
        for (uint32_t e = 1; e < d; ++e)
        {
            BigUint all = blocks[e];
            all.mul_small(angles_with(e));
            index += all;
        }
        BigUint before = blocks[d];
        before.mul_small(angle - first_angle(d));
        index += before;
        r = l - ls_header(d);
    }
    index += body_index(sub ? sub_ : ls_, r, *f);
    return index;
}

} // namespace sieve
