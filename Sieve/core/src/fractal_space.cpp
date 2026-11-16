// Sieve — the fractal dimension's whole space (see fractal_space.hpp).
#include "sieve/fractal_space.hpp"

#include "sieve/fractal.hpp"

#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace sieve {

namespace {

constexpr uint32_t kAxiomOverhead = 7; // "axiom " and the line feed
constexpr uint32_t kRuleOverhead = 8;  // "rule X " and the line feed
constexpr uint32_t kSubHeader = 30;    // "sieve-fractal-v1\n" and "substitution\n"

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
    by_length_.assign(longest + 1, BigUint());
    for (uint32_t l = kFractalSpaceShortest; l <= longest; ++l)
    {
        by_length_[l] = at_length(l, nullptr);
        total_ += by_length_[l];
    }
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

BigUint FractalSpace::count_at(uint32_t length) const { return length < by_length_.size() ? by_length_[length] : BigUint(); }

std::string FractalSpace::at(const BigUint& index) const
{
    if (index >= total_) throw std::invalid_argument("past the last ruleset of " + std::to_string(longest_) + " characters or fewer");
    BigUint i = index;
    uint32_t l = kFractalSpaceShortest;
    while (i >= by_length_[l]) i -= by_length_[l++];
    std::vector<BigUint> blocks;
    at_length(l, &blocks);
    const Family* f = nullptr;
    std::string text = std::string(kFractalVersion) + "\n";
    uint32_t r = 0;
    if (i < blocks[0])
    {
        f = &sub_, text += "substitution\n", r = l - kSubHeader;
    }
    else
    {
        i -= blocks[0];
        for (uint32_t d = 1; d <= 3 && !f; ++d)
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
            i = rem;
            f = &ls_, text += "lsystem " + std::to_string(first_angle(d) + q.mod_small(1u << 31)) + "\n", r = l - ls_header(d);
        }
    }
    // The axiom: the shorter first, then its word.
    for (uint32_t n = 1; n <= kFractalWordMost && kAxiomOverhead + n <= r; ++n)
    {
        const BigUint rest = f->rules(0, r - kAxiomOverhead - n, false);
        const BigUint block = times(f->words[n], rest);
        if (i >= block)
        {
            i -= block;
            continue;
        }
        BigUint q, rem;
        BigUint::divmod(i, rest, q, rem);
        text += "axiom " + word_at(q, n, f->symbols) + "\n";
        i = rem, r -= kAxiomOverhead + n;
        break;
    }
    // Its rules, symbol by symbol: a rule for it first (the shorter word first), then none.
    bool any = false;
    for (size_t j = 0; j < f->rule_symbols.size(); ++j)
    {
        for (uint32_t n = 1; n <= kFractalWordMost && kRuleOverhead + n <= r; ++n)
        {
            const BigUint rest = f->rules(j + 1, r - kRuleOverhead - n, true);
            const BigUint block = times(f->words[n], rest);
            if (i >= block)
            {
                i -= block;
                continue;
            }
            BigUint q, rem;
            BigUint::divmod(i, rest, q, rem);
            text += std::string("rule ") + f->rule_symbols[j] + " " + word_at(q, n, f->symbols) + "\n";
            i = rem, r -= kRuleOverhead + n, any = true;
            break;
        }
    }
    return text;
}

std::optional<BigUint> FractalSpace::index_of(const std::string& text) const
{
    const auto f = read_fractal(text);
    if (!f || text.size() > longest_ || text.size() < kFractalSpaceShortest) return std::nullopt;
    const uint32_t l = uint32_t(text.size());
    BigUint index;
    for (uint32_t k = kFractalSpaceShortest; k < l; ++k) index += by_length_[k];
    std::vector<BigUint> blocks;
    at_length(l, &blocks);
    const bool sub = f->family == Fractal::Family::Substitution;
    const Family& fam = sub ? sub_ : ls_;
    uint32_t r = 0;
    if (sub) r = l - kSubHeader;
    else
    {
        index += blocks[0];
        const uint32_t d = digits_of(f->angle);
        for (uint32_t e = 1; e < d; ++e)
        {
            BigUint all = blocks[e];
            all.mul_small(angles_with(e));
            index += all;
        }
        BigUint before = blocks[d];
        before.mul_small(f->angle - first_angle(d));
        index += before;
        r = l - ls_header(d);
    }
    const uint32_t an = uint32_t(f->axiom.size());
    for (uint32_t n = 1; n < an; ++n) index += times(fam.words[n], fam.rules(0, r - kAxiomOverhead - n, false));
    index += times(word_rank(f->axiom, fam.symbols), fam.rules(0, r - kAxiomOverhead - an, false));
    r -= kAxiomOverhead + an;
    bool any = false;
    size_t next = 0; // the next rule of the ruleset's
    for (size_t j = 0; j < fam.rule_symbols.size() && next < f->rules.size(); ++j)
    {
        if (f->rules[next].first != fam.rule_symbols[j])
        {
            // (no rule for it: after every ruleset with one)
            for (uint32_t n = 1; n <= kFractalWordMost && kRuleOverhead + n <= r; ++n) index += times(fam.words[n], fam.rules(j + 1, r - kRuleOverhead - n, true));
            continue;
        }
        const std::string& w = f->rules[next].second;
        const uint32_t wn = uint32_t(w.size());
        for (uint32_t n = 1; n < wn; ++n) index += times(fam.words[n], fam.rules(j + 1, r - kRuleOverhead - n, true));
        index += times(word_rank(w, fam.symbols), fam.rules(j + 1, r - kRuleOverhead - wn, true));
        r -= kRuleOverhead + wn, any = true, ++next;
    }
    return index;
}

} // namespace sieve
