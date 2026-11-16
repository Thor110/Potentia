// Sieve — the fractal dimension's whole space (see fractal_space.hpp).
#include "sieve/fractal_space.hpp"

#include "sieve/fractal.hpp"

#include <algorithm>
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
    total_ = total_v1_;
    for (uint32_t l = kFractalSpaceShortest; l <= longest; ++l) total_ += by_length_v2_[l];
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

BigUint FractalSpace::count_at(uint32_t length) const
{
    if (length >= by_length_.size()) return BigUint();
    BigUint n = by_length_[length];
    n += by_length_v2_[length];
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
    if (!f || text.size() > longest_ || text.size() < kFractalSpaceShortest) return std::nullopt;
    const uint32_t l = uint32_t(text.size());
    BigUint index;
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
