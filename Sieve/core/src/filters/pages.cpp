// Sieve — small filters for pages of any alphabet (list Z7; IDEAS 19.232 and 19.235).
//
//   few-values-v1     more than 2^k distinct symbols on the page (Edward, 15 November 2026: "should we
//                     perhaps not adapt the filter to anything with only two values?"): a page of at
//                     most 2^k values reads as k bits a symbol and is the binary line's, as two values
//                     are one bit. An automaton whose state is the set of symbols seen (at most 2^k of
//                     them, then "more"), where it fits; counted exactly past that (every page, less
//                     those of i symbols for i to 2^k: C(P, i) times the onto maps of N places to i).
//   two-symbol-run-v1 no stretch of more than N symbols on at most two (Edward: "knocking out 'welcome to
//                     the sieveaaaabbbb'"): max-run's next step, padding such as "aaaabbbb" or "ababab"
//                     anywhere on the page. N = 12 at first: on 1,304 pages of 1,000 of English prose
//                     (this project's manual, specifications and ideas, as lower27), the longest such
//                     stretch was 3 to 7 on all but 11, and 15 at most (12 passes all but 2). An automaton of the stretch's two
//                     symbols, its length and the run of the last symbol, where it fits; judged past that.
//   golden-word-v1    the golden ratio's word (IDEAS 19.235: "Dan Shechtman's binary interpretation of
//                     the golden ratio as a filter"): a page of two symbols that, read either way round,
//                     is a window of the Fibonacci word, 0100101001001..., of which there are N + 1 at
//                     length N; with slopes = every, any window of any Sturmian word (a balanced word: any
//                     two stretches of one length hold the same count of either symbol, give or take one),
//                     1 + sum over k of (N - k + 1) phi(k) of them over two symbols. Survivors: those over
//                     each pair of the alphabet's symbols, each using both, counted exactly; the golden
//                     word's written out and ranked while there are 65,536 at most.
//   pronounceable-v1  every word could be said aloud (list Z9): a word of 4 letters or more has a vowel
//                     (a e i o u y), and no word runs more than 5 consonants or 5 vowels together; a word is
//                     a run of letters, anything else between. Speech-like letter strings pass, the
//                     "xqzkr" of random pages fail. An automaton of the word so far (its length to 4, a
//                     vowel met, the consonants and vowels running): counted and ranked exactly.
//   letter-case-v1    capitals where writing puts them (list Z9; an alphabet with both cases): a capital
//                     only begins a word or fills it ("Sieve", "NASA", not "sIeve"), and after a sentence's
//                     end (. ! or ?) and its spaces the next letter is a capital. An automaton.
//   brackets-v1       every ( [ { opened is closed by its own kind, in order, nested at most D deep, and
//                     never closed before it is opened (list Z9): prose's asides and code's blocks. An
//                     automaton of the open brackets (3^D stacks at most).
//   word-length-v2    no word (a run of letters) longer than N: 28 at first (5 to 100), so
//                     antidisestablishmentarianism passes (Edward). word-length-v1, retired, had 20 (1 to 64).
//   word-length-v1    no word (a run of letters) longer than N, 20 at first (list Z9): on random lower27
//                     pages a space comes once in 27 letters, so their words run on; of this project's pages
//                     every one of the manual's 206 passes, 333 of the specifications' 335 (their
//                     longest words are 19 and 23 letters). An automaton of the word's length.

#include "sieve/alphabet.hpp"
#include "sieve/dfa.hpp"
#include "sieve/filter.hpp"
#include "sieve/plugin.hpp" // make_dfa_filter

#include <algorithm>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

namespace sieve {

namespace {

constexpr uint64_t kMaxDfaCells = uint64_t(1) << 24; // (as the other filters bound their automata)
bool cells_fit(uint64_t states, uint64_t base) { return base > 0 && states <= kMaxDfaCells / base; }

bool page_line(const FilterLine& l) { return l.kind == "text" && l.alphabet && !l.up_to_length && l.base >= 2 && l.length >= 1; }

// ---------------------------------------------------------------- few-values

// The pages of at most m distinct symbols: sum over i of C(P, i) x onto(N, i), onto(N, i) = sum over j of
// (-1)^j C(i, j) (i - j)^N.
BigUint few_count(uint32_t base, uint32_t length, uint32_t most)
{
    BigUint total;
    BigUint choose(1); // C(P, i)
    for (uint32_t i = 1; i <= std::min(most, base); ++i)
    {
        choose.mul_small(base - i + 1);
        (void)choose.divmod_small(i);
        // onto(N, i) by inclusion and exclusion: the positive and negative terms summed apart.
        BigUint plus, minus, cij(1);
        for (uint32_t j = 0; j <= i; ++j)
        {
            if (j)
            {
                cij.mul_small(i - j + 1);
                (void)cij.divmod_small(j);
            }
            const BigUint term = BigUint::mul(cij, BigUint::pow(i - j, length));
            (j % 2 ? minus : plus) += term;
        }
        plus -= minus;
        total += BigUint::mul(choose, plus);
    }
    return total;
}

std::optional<Dfa> few_dfa(uint32_t base, uint32_t most)
{
    // States: the sets of symbols seen, sorted, of at most `most`; then one state for "more" (accepting).
    std::map<std::vector<uint32_t>, int32_t> id;
    std::vector<std::vector<uint32_t>> sets{{}};
    id[{}] = 0;
    for (size_t k = 0; k < sets.size(); ++k)
    {
        if (sets[k].size() == most) continue;
        for (uint32_t c = 0; c < base; ++c)
        {
            if (std::binary_search(sets[k].begin(), sets[k].end(), c)) continue;
            auto next = sets[k];
            next.insert(std::upper_bound(next.begin(), next.end(), c), c);
            if (id.emplace(next, int32_t(sets.size())).second) sets.push_back(std::move(next));
            if (!cells_fit(sets.size() + 1, base)) return std::nullopt;
        }
    }
    const int32_t more = int32_t(sets.size());
    Dfa d;
    d.base = base;
    d.start = 0;
    d.accept.assign(sets.size() + 1, 0);
    d.accept[size_t(more)] = 1;
    d.next.assign((sets.size() + 1) * base, Dfa::kDead);
    for (size_t k = 0; k < sets.size(); ++k)
        for (uint32_t c = 0; c < base; ++c)
        {
            if (std::binary_search(sets[k].begin(), sets[k].end(), c))
            {
                d.next[k * base + c] = int32_t(k);
                continue;
            }
            if (sets[k].size() == most)
            {
                d.next[k * base + c] = more;
                continue;
            }
            auto next = sets[k];
            next.insert(std::upper_bound(next.begin(), next.end(), c), c);
            d.next[k * base + c] = id.at(next);
        }
    for (uint32_t c = 0; c < base; ++c) d.next[size_t(more) * base + c] = more;
    return minimise(d);
}

class FewFilter : public Filter
{
public:
    FewFilter(const FilterLine& l, uint32_t most, std::string prov) : most_(most)
    {
        provenance_ = std::move(prov);
        survivors_ = BigUint::pow(l.base, l.length);
        survivors_ -= few_count(l.base, l.length, most);
    }
    bool passes(std::span<const uint32_t> unit) const override
    {
        std::set<uint32_t> seen;
        for (const uint32_t d : unit)
            if (seen.insert(d).second && seen.size() > most_) return true;
        return false;
    }
    const BigUint* survivors() const override { return &survivors_; }

private:
    uint32_t most_;
    BigUint survivors_;
};

// ---------------------------------------------------------------- two-symbol-run

// The stretch so far on at most two symbols: the other one (base: none yet), the last, the run of the last,
// and the stretch's length.
struct Stretch
{
    uint32_t other, last, run, length;
    bool operator<(const Stretch& o) const { return std::tie(other, last, run, length) < std::tie(o.other, o.last, o.run, o.length); }
};

Stretch step_stretch(const Stretch& s, uint32_t c)
{
    if (s.length == 0) return {s.other, c, 1, 1};
    if (c == s.last) return {s.other, c, s.run + 1, s.length + 1};
    if (c == s.other) return {s.last, c, 1, s.length + 1};
    return {s.last, c, 1, s.run + 1}; // (a third symbol: the stretch starts again from the last's run)
}

std::optional<Dfa> run_dfa(uint32_t base, uint32_t most)
{
    std::map<Stretch, int32_t> id;
    std::vector<Stretch> states{{base, base, 0, 0}};
    id[states[0]] = 0;
    std::vector<int32_t> next;
    for (size_t k = 0; k < states.size(); ++k)
    {
        next.resize((k + 1) * base, Dfa::kDead);
        for (uint32_t c = 0; c < base; ++c)
        {
            const Stretch n = step_stretch(states[k], c);
            if (n.length > most) continue;
            const auto [it, added] = id.emplace(n, int32_t(states.size()));
            if (added)
            {
                states.push_back(n);
                if (!cells_fit(states.size(), base)) return std::nullopt;
            }
            next[k * base + c] = it->second;
        }
    }
    Dfa d;
    d.base = base;
    d.start = 0;
    d.accept.assign(states.size(), 1);
    d.next = std::move(next);
    return minimise(d);
}

class RunFilter : public Filter
{
public:
    RunFilter(uint32_t base, uint32_t most, std::string prov) : base_(base), most_(most) { provenance_ = std::move(prov); }
    bool passes(std::span<const uint32_t> unit) const override
    {
        Stretch s{base_, base_, 0, 0};
        for (const uint32_t c : unit)
            if ((s = step_stretch(s, c)).length > most_) return false;
        return true;
    }

private:
    uint32_t base_, most_;
};

// ---------------------------------------------------------------- golden-word

// The Fibonacci word's windows of length n (0 and 1), each once: n + 1 of them.
std::set<std::vector<uint32_t>> fibonacci_windows(uint32_t n)
{
    std::vector<uint32_t> w{0};
    while (w.size() < size_t(4) * n + 16)
    {
        std::vector<uint32_t> next;
        for (const uint32_t c : w)
        {
            next.push_back(0);
            if (c == 0) next.push_back(1);
        }
        w = std::move(next);
    }
    std::set<std::vector<uint32_t>> out;
    for (size_t i = 0; i + n <= w.size() && out.size() < size_t(n) + 1; ++i) out.insert(std::vector<uint32_t>(w.begin() + std::ptrdiff_t(i), w.begin() + std::ptrdiff_t(i + n)));
    return out;
}

// Balanced: for each length, the stretches' counts of 1 differ by at most one.
bool balanced(std::span<const uint32_t> bits)
{
    const size_t n = bits.size();
    std::vector<uint32_t> ones(n + 1, 0);
    for (size_t i = 0; i < n; ++i) ones[i + 1] = ones[i] + bits[i];
    for (size_t len = 1; len < n; ++len)
    {
        uint32_t lo = UINT32_MAX, hi = 0;
        for (size_t i = 0; i + len <= n; ++i)
        {
            const uint32_t k = ones[i + len] - ones[i];
            lo = std::min(lo, k), hi = std::max(hi, k);
            if (hi - lo > 1) return false;
        }
    }
    return true;
}

// The balanced words of length n over two symbols, both and either: 1 + sum over k of (n - k + 1) phi(k).
BigUint balanced_count(uint32_t n)
{
    BigUint total(1);
    for (uint32_t k = 1; k <= n; ++k)
    {
        uint32_t phi = k, m = k;
        for (uint32_t p = 2; p * p <= m; ++p)
            if (m % p == 0)
            {
                while (m % p == 0) m /= p;
                phi -= phi / p;
            }
        if (m > 1) phi -= phi / m;
        BigUint t(uint64_t(n - k + 1));
        t.mul_small(phi);
        total += t;
    }
    return total;
}

// A page of exactly two symbols as bits, the lower symbol 0, or nothing.
std::optional<std::vector<uint32_t>> two_valued(std::span<const uint32_t> unit)
{
    if (unit.empty()) return std::nullopt;
    uint32_t a = unit[0], b = unit[0];
    for (const uint32_t c : unit)
    {
        if (c == a || c == b) continue;
        if (a != b) return std::nullopt;
        b = c;
    }
    if (a == b) return std::nullopt;
    const uint32_t lo = std::min(a, b);
    std::vector<uint32_t> bits;
    bits.reserve(unit.size());
    for (const uint32_t c : unit) bits.push_back(c == lo ? 0u : 1u);
    return bits;
}

class GoldenFilter : public Filter
{
public:
    GoldenFilter(const FilterLine& l, bool keep, bool every) : keep_(keep), every_(every)
    {
        const uint32_t n = l.length;
        BigUint each; // the words over one pair of symbols, as 0 and 1, using both
        if (every)
        {
            each = balanced_count(n);
            each -= BigUint(2); // (all 0, all 1)
        }
        else
        {
            for (const auto& w : fibonacci_windows(n))
            {
                // (read either way round: the window and its complement, each using both symbols)
                if (std::count(w.begin(), w.end(), 0u) == 0 || std::count(w.begin(), w.end(), 1u) == 0) continue;
                windows_.insert(w);
                std::vector<uint32_t> flip(w.size());
                for (size_t i = 0; i < w.size(); ++i) flip[i] = 1 - w[i];
                windows_.insert(std::move(flip));
            }
            each = BigUint(uint64_t(windows_.size()));
        }
        // Over each unordered pair of the alphabet's symbols, the lower read as 0.
        BigUint pairs(uint64_t(l.base));
        pairs.mul_small(l.base - 1);
        (void)pairs.divmod_small(2);
        const BigUint kept = BigUint::mul(pairs, each);
        survivors_ = keep ? kept : BigUint::pow(l.base, n);
        if (!keep) survivors_ -= kept;
        provenance_ = std::string("keep=") + (keep ? "include" : "exclude") + " slopes=" + (every ? "every" : "golden") + " per-pair=" + each.to_decimal();
    }
    bool passes(std::span<const uint32_t> unit) const override
    {
        const auto bits = two_valued(unit);
        const bool in = bits && (every_ ? balanced(*bits) : windows_.count(*bits) > 0);
        return in == keep_;
    }
    const BigUint* survivors() const override { return &survivors_; }

private:
    bool keep_, every_;
    std::set<std::vector<uint32_t>> windows_; // (words of 32-bit symbols: compared without memcmp)
    BigUint survivors_;
};

// ---------------------------------------------------------------- pronounceable

// Each symbol: 0 not a letter, 1 a vowel, 2 a consonant.
std::vector<uint8_t> letter_kinds(const Alphabet& a)
{
    std::vector<uint8_t> k(a.size(), 0);
    for (uint32_t d = 0; d < a.size(); ++d)
    {
        char32_t c = a.symbol(d);
        if (c >= U'A' && c <= U'Z') c = c - U'A' + U'a';
        if (c >= U'a' && c <= U'z') k[d] = std::u32string_view(U"aeiouy").find(c) != std::u32string_view::npos ? 1 : 2;
    }
    return k;
}

struct Spoken
{
    uint32_t length, vowel, consonants, vowels; // (length counted to `needs`)
    bool operator<(const Spoken& o) const { return std::tie(length, vowel, consonants, vowels) < std::tie(o.length, o.vowel, o.consonants, o.vowels); }
};

std::optional<Dfa> spoken_dfa(const Alphabet& a, uint32_t needs, uint32_t most_c, uint32_t most_v)
{
    const auto kind = letter_kinds(a);
    auto word_ok = [&](const Spoken& s) { return s.length < needs || s.vowel; };
    std::map<Spoken, int32_t> id;
    std::vector<Spoken> states{{0, 0, 0, 0}};
    id[states[0]] = 0;
    std::vector<int32_t> next;
    for (size_t k = 0; k < states.size(); ++k)
    {
        next.resize((k + 1) * a.size(), Dfa::kDead);
        const Spoken s = states[k];
        for (uint32_t c = 0; c < a.size(); ++c)
        {
            Spoken n;
            if (kind[c] == 0)
            {
                if (!word_ok(s)) continue; // (a word ends without a vowel)
                n = {0, 0, 0, 0};
            }
            else if (kind[c] == 1)
            {
                n = {std::min(s.length + 1, needs), 1, 0, s.vowels + 1};
                if (n.vowels > most_v) continue;
            }
            else
            {
                n = {std::min(s.length + 1, needs), s.vowel, s.consonants + 1, 0};
                if (n.consonants > most_c) continue;
            }
            const auto [it, added] = id.emplace(n, int32_t(states.size()));
            if (added) states.push_back(n);
            next[k * a.size() + c] = it->second;
        }
    }
    Dfa d;
    d.base = a.size();
    d.start = 0;
    d.accept.resize(states.size());
    for (size_t k = 0; k < states.size(); ++k) d.accept[k] = word_ok(states[k]) ? 1 : 0;
    d.next = std::move(next);
    return minimise(d);
}

// ---------------------------------------------------------------- letter-case

// Each symbol: 0 other, 1 lower-case letter, 2 capital, 3 a sentence's end (. ! ?), 4 a space.
std::vector<uint8_t> case_kinds(const Alphabet& a)
{
    std::vector<uint8_t> k(a.size(), 0);
    for (uint32_t d = 0; d < a.size(); ++d)
    {
        const char32_t c = a.symbol(d);
        k[d] = (c >= U'a' && c <= U'z') ? 1 : (c >= U'A' && c <= U'Z') ? 2 : (c == U'.' || c == U'!' || c == U'?') ? 3 : c == U' ' ? 4 : 0;
    }
    return k;
}

Dfa case_dfa(const Alphabet& a)
{
    // States: 0 between words; 1 in a word of lower case (or a capital and lower case after it); 2 a word's
    // first letter a capital; 3 a word of capitals (two or more); 4 a sentence ended (its spaces after it);
    // 5 a sentence ended and a space met: the next letter must be a capital.
    const auto kind = case_kinds(a);
    const uint32_t B = a.size();
    Dfa d;
    d.base = B;
    d.start = 0;
    d.accept.assign(6, 1);
    d.next.assign(6 * size_t(B), Dfa::kDead);
    for (uint32_t c = 0; c < B; ++c)
        for (int32_t s = 0; s < 6; ++s)
        {
            int32_t n = Dfa::kDead;
            switch (kind[c])
            {
            case 1: n = (s == 0 || s == 1 || s == 2 || s == 4) ? 1 : Dfa::kDead; break; // (lower case: not after two capitals, not to begin a sentence)
            case 2: n = (s == 0 || s == 4 || s == 5) ? 2 : (s == 2 || s == 3) ? 3 : Dfa::kDead; break;
            case 3: n = 4; break;
            case 4: n = (s == 4 || s == 5) ? 5 : 0; break;
            default: n = s == 5 ? 5 : 0; break; // (a quote or a bracket before the capital keeps the sentence waiting)
            }
            d.next[size_t(s) * B + c] = n;
        }
    return minimise(d);
}

// ---------------------------------------------------------------- brackets

std::optional<Dfa> bracket_dfa(const Alphabet& a, uint32_t deepest)
{
    // A state is the stack of open brackets, each 0 ( 1 [ 2 {, as a number in base 3 with a 1 in front.
    const uint32_t B = a.size();
    std::vector<int> open(B, -1), close(B, -1);
    for (uint32_t d = 0; d < B; ++d)
    {
        const char32_t c = a.symbol(d);
        const char32_t* o = U"([{";
        const char32_t* x = U")]}";
        for (int k = 0; k < 3; ++k)
        {
            if (c == o[k]) open[d] = k;
            if (c == x[k]) close[d] = k;
        }
    }
    std::map<std::vector<int>, int32_t> id;
    std::vector<std::vector<int>> stacks{{}};
    id[{}] = 0;
    std::vector<int32_t> next;
    for (size_t k = 0; k < stacks.size(); ++k)
    {
        next.resize((k + 1) * B, Dfa::kDead);
        for (uint32_t c = 0; c < B; ++c)
        {
            std::vector<int> n = stacks[k];
            if (open[c] >= 0)
            {
                if (n.size() == deepest) continue;
                n.push_back(open[c]);
            }
            else if (close[c] >= 0)
            {
                if (n.empty() || n.back() != close[c]) continue;
                n.pop_back();
            }
            const auto [it, added] = id.emplace(n, int32_t(stacks.size()));
            if (added)
            {
                stacks.push_back(std::move(n));
                if (!cells_fit(stacks.size(), B)) return std::nullopt;
            }
            next[k * B + c] = it->second;
        }
    }
    Dfa d;
    d.base = B;
    d.start = 0;
    d.accept.assign(stacks.size(), 0);
    d.accept[0] = 1; // (every bracket closed)
    d.next = std::move(next);
    return minimise(d);
}

// ---------------------------------------------------------------- word-length

Dfa word_length_dfa(const Alphabet& a, uint32_t most)
{
    const auto kind = letter_kinds(a);
    Dfa d;
    d.base = a.size();
    d.start = 0;
    d.accept.assign(size_t(most) + 1, 1);
    d.next.assign((size_t(most) + 1) * a.size(), Dfa::kDead);
    for (uint32_t n = 0; n <= most; ++n)
        for (uint32_t c = 0; c < a.size(); ++c)
            d.next[size_t(n) * a.size() + c] = kind[c] == 0 ? 0 : n + 1 <= most ? int32_t(n + 1) : Dfa::kDead;
    return minimise(d);
}

} // namespace

void add_page_filters(std::vector<FilterSpec>& out)
{
    FilterSpec f;
    f.id = "few-values";
    f.title = "few-values";
    f.description = "Pages: more than 2^k distinct symbols. A page of at most 2^k values reads as k bits a symbol, so it is the binary "
                    "line's (k = 1: pages of two values, one bit a symbol; 2: four, the DNA alphabet's two bits). In positional "
                    "order the start of every line is such pages (\"aaaa\", \"aaab\"). Counted as an automaton (the symbols seen) "
                    "while it fits; counted exactly and judged past that.";
    f.params = {{"bits", "k: pages of at most 2^k distinct symbols are set aside", FilterParam::Kind::Integer, "1", 1, 4, 1, {}}};
    f.applies = page_line;
    f.counts_as = "automaton";
    f.make = [own = f.name()](const FilterLine& l, const FilterValues& v, const FilterResources&) -> std::unique_ptr<Filter> {
        const uint32_t most = uint32_t(1) << param_int(*find_filter(own), v, "bits");
        const std::string prov = "values=" + std::to_string(most);
        if (auto dfa = few_dfa(l.base, most)) return make_dfa_filter(std::move(*dfa), l.length, prov);
        return std::make_unique<FewFilter>(l, most, prov);
    };
    f.counts_as_on = [own = f.name()](const FilterLine& l, const FilterValues* v) -> std::string {
        uint32_t most = 16;
        if (v)
            try { most = uint32_t(1) << param_int(*find_filter(own), *v, "bits"); }
            catch (const std::exception&) {} // a malformed setting: as without one
        return few_dfa(l.base, most) ? "automaton" : "";
    };
    out.push_back(f);

    FilterSpec r;
    r.id = "two-symbol-run";
    r.title = "two-symbol-run";
    r.description = "Pages: no stretch of more than N symbols on at most two, anywhere on the page: padding such as \"aaaabbbb\" or "
                    "\"ababab\" (max-run's next step, which caps one symbol alone). Real words keep bringing new letters: on 1,304 "
                    "pages of English prose the longest such stretch was 3 to 7 on all but 11, and 15 at most, so 12 at first (all but 2 pass). "
                    "Counted as an automaton (the stretch's symbols, its length, the last symbol's run) while it fits; judged "
                    "past that.";
    r.params = {{"longest", "the longest stretch on at most two symbols allowed", FilterParam::Kind::Integer, "12", 2, 64, 1, {}}};
    r.applies = page_line;
    r.counts_as = "automaton";
    r.make = [own = r.name()](const FilterLine& l, const FilterValues& v, const FilterResources&) -> std::unique_ptr<Filter> {
        const uint32_t most = uint32_t(param_int(*find_filter(own), v, "longest"));
        const std::string prov = "longest=" + std::to_string(most);
        if (auto dfa = run_dfa(l.base, most)) return make_dfa_filter(std::move(*dfa), l.length, prov);
        return std::make_unique<RunFilter>(l.base, most, prov);
    };
    r.counts_as_on = [own = r.name()](const FilterLine& l, const FilterValues* v) -> std::string {
        uint32_t most = 64;
        if (v)
            try { most = uint32_t(param_int(*find_filter(own), *v, "longest")); }
            catch (const std::exception&) {} // a malformed setting: as without one
        // (the stretch's states: the other symbol or none, the last, and run <= length <= most)
        return cells_fit(uint64_t(l.base + 1) * l.base * (uint64_t(most) * (most + 1) / 2) + 1, l.base) ? "automaton" : "";
    };
    out.push_back(r);

    FilterSpec g;
    g.id = "golden-word";
    g.title = "golden-word";
    g.description = "Pages: a page of two symbols that, the lower read as 0, is a window of the golden ratio's word, the Fibonacci "
                    "word 0100101001001... (put 01 for each 0 and 0 for each 1, again and again; the one-dimensional model of Dan "
                    "Shechtman's quasicrystals), or of its complement: N + 1 windows at length N, over each pair of the "
                    "alphabet's symbols. With slopes = every, any window of any Sturmian word (balanced: any two stretches of one "
                    "length hold the same count of 1, give or take one), 1 + the sum over k of (N - k + 1) phi(k) at length N. "
                    "keep = include passes those pages alone; exclude every other. Counted exactly; judged.";
    g.params = {{"keep", "include: only those pages; exclude: everything but them", FilterParam::Kind::Text, "include", 0, 0, 1, {"include", "exclude"}},
                {"slopes", "golden: the golden ratio's word; every: every Sturmian word", FilterParam::Kind::Text, "golden", 0, 0, 1,
                 {"golden", "every"}}};
    g.applies = page_line;
    g.make = [own = g.name()](const FilterLine& l, const FilterValues& v, const FilterResources&) -> std::unique_ptr<Filter> {
        const FilterSpec& spec = *find_filter(own);
        return std::make_unique<GoldenFilter>(l, param_value(spec, v, "keep") == "include", param_value(spec, v, "slopes") == "every");
    };
    out.push_back(g);

    FilterSpec sp;
    sp.id = "pronounceable";
    sp.title = "pronounceable";
    sp.description = "Pages: every word could be said aloud. A word (a run of letters) of 4 letters or more has a vowel (a, e, i, o, "
                     "u or y), and no word runs more than 5 consonants or 5 vowels together. English passes (\"strengths\" runs 5 "
                     "consonants; \"queueing\" 5 vowels), random letters mostly fail. On 206 pages of this project's manual 150 "
                     "pass (\"html\", \"png\" and the like fail); on its speeches, all. An automaton: counted and ranked exactly.";
    sp.params = {{"vowel_from", "words of this many letters or more need a vowel", FilterParam::Kind::Integer, "4", 1, 16, 1, {}},
                 {"consonants", "the most consonants running together", FilterParam::Kind::Integer, "5", 1, 16, 1, {}},
                 {"vowels", "the most vowels running together", FilterParam::Kind::Integer, "5", 1, 16, 1, {}}};
    sp.applies = page_line;
    sp.counts_as = "automaton";
    sp.make = [own = sp.name()](const FilterLine& l, const FilterValues& v, const FilterResources&) -> std::unique_ptr<Filter> {
        const FilterSpec& spec = *find_filter(own);
        const uint32_t needs = uint32_t(param_int(spec, v, "vowel_from")), c = uint32_t(param_int(spec, v, "consonants")),
                       vw = uint32_t(param_int(spec, v, "vowels"));
        const std::string prov = "vowel_from=" + std::to_string(needs) + " consonants=" + std::to_string(c) + " vowels=" + std::to_string(vw);
        return make_dfa_filter(std::move(*spoken_dfa(*l.alphabet, needs, c, vw)), l.length, prov);
    };
    out.push_back(sp);

    FilterSpec lc;
    lc.id = "letter-case";
    lc.title = "letter-case";
    lc.description = "Pages with both cases: capitals where writing puts them. A capital only begins a word or fills it (Sieve, "
                     "NASA; not sIeve), and after a sentence's end (. ! or ?) and its spaces the next letter is a capital. "
                     "Random pages of ascii96 almost all fail (it keeps 10^-42.46 of pages of 1,000). Strict: names written in mixed "
                     "case (macOS, GitHub, GIFs) fail it, so of 227 pages of this project's manual 126 pass, of its speeches 4 of 5. "
                     "An automaton: counted and ranked exactly.";
    lc.applies = [](const FilterLine& l) { return page_line(l) && l.alphabet->has_uppercase(); };
    lc.counts_as = "automaton";
    lc.make = [](const FilterLine& l, const FilterValues&, const FilterResources&) -> std::unique_ptr<Filter> {
        return make_dfa_filter(case_dfa(*l.alphabet), l.length, "");
    };
    out.push_back(lc);

    FilterSpec br;
    br.id = "brackets";
    br.title = "brackets";
    br.description = "Pages: every ( [ or { opened is closed by its own kind, in order, nested at most `deepest` deep, and none is "
                     "closed before it is opened: prose's asides and code's blocks. An automaton of the brackets open: counted and "
                     "ranked exactly while it fits.";
    br.params = {{"deepest", "the most brackets open at once", FilterParam::Kind::Integer, "4", 1, 12, 1, {}}};
    br.applies = [](const FilterLine& l) {
        if (!page_line(l)) return false;
        for (const char32_t c : std::u32string_view(U"()[]{}"))
            if (l.alphabet->contains(c)) return true;
        return false;
    };
    br.counts_as = "automaton";
    br.make = [own = br.name()](const FilterLine& l, const FilterValues& v, const FilterResources&) -> std::unique_ptr<Filter> {
        const uint32_t deepest = uint32_t(param_int(*find_filter(own), v, "deepest"));
        auto d = bracket_dfa(*l.alphabet, deepest);
        if (!d) throw std::invalid_argument("brackets-v1: " + std::to_string(deepest) + " deep is past what its automaton holds on this alphabet");
        return make_dfa_filter(std::move(*d), l.length, "deepest=" + std::to_string(deepest));
    };
    out.push_back(br);

    FilterSpec wl;
    wl.id = "word-length";
    wl.title = "word-length";
    wl.description = "Pages: no word (a run of letters) longer than `longest` letters. On random lower27 pages a space comes once in "
                     "27 letters, so their words run on; of this project's pages every one of the manual's 206 passes at 20, and "
                     "333 of the specifications' 335. An automaton: counted and ranked exactly.";
    wl.params = {{"longest", "the longest word allowed, in letters", FilterParam::Kind::Integer, "20", 1, 64, 1, {}}};
    wl.applies = page_line;
    wl.counts_as = "automaton";
    wl.make = [own = wl.name()](const FilterLine& l, const FilterValues& v, const FilterResources&) -> std::unique_ptr<Filter> {
        const uint32_t most = uint32_t(param_int(*find_filter(own), v, "longest"));
        return make_dfa_filter(word_length_dfa(*l.alphabet, most), l.length, "longest=" + std::to_string(most));
    };
    wl.retired = true;
    wl.replaced_by = "word-length-v2";
    out.push_back(wl);

    // Version 2 (Edward, 17 November 2026: "perhaps the cap should be an adjustable setting for that filter
    // instead, default 28, maximum 100, minimum maybe 5"): 28 lets antidisestablishmentarianism through.
    FilterSpec wl2 = wl;
    wl2.version = 2;
    wl2.retired = false;
    wl2.replaced_by.clear();
    wl2.description = "Pages: no word (a run of letters) longer than `longest` letters, 28 at first, so antidisestablishmentarianism "
                      "passes (5 to 100; the dictionary's longest, pneumonoultramicroscopicsilicovolcanoconiosis, is 45). On "
                      "random lower27 pages a space comes once in 27 letters, so their words run on: on pages of 1,000 a cap of "
                      "28 keeps 10^-12.92 of the line, 20 keeps 10^-24.23, 45 keeps 10^-4.53. An automaton: counted and ranked "
                      "exactly.";
    wl2.params = {{"longest", "the longest word allowed, in letters", FilterParam::Kind::Integer, "28", 5, 100, 1, {}}};
    wl2.make = [own = wl2.name()](const FilterLine& l, const FilterValues& v, const FilterResources&) -> std::unique_ptr<Filter> {
        const uint32_t most = uint32_t(param_int(*find_filter(own), v, "longest"));
        return make_dfa_filter(word_length_dfa(*l.alphabet, most), l.length, "longest=" + std::to_string(most));
    };
    out.push_back(wl2);
}

} // namespace sieve
