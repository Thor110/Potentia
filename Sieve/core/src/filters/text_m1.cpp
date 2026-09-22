// M1 filters for lower27 text (clean, window, words), with rankers for clean and words.
//
// Symbols: 0 = SPACE, 1..26 = a..z.
//   clean   no two SPACEs in a row, and at least one letter
//   window  clean, and could be cut from running English (see sieve/sieve.hpp)
//   words   clean, and every token is a dictionary word
// Survivors are ranked in address (positional, i.e. dictionary) order.
//
// Version 2 of clean and words also accepts trailing padding: a unit that ends in two or more
// SPACEs passes if what comes before them passes version 1 (as the last unit of warped text
// does). Everything else is exactly version 1.

#include "sieve/filter.hpp"

#include <algorithm>
#include <stdexcept>

namespace sieve {

namespace {

constexpr char kLower27[] = " abcdefghijklmnopqrstuvwxyz";

// Above this unit length the rankers' count tables would take too much memory on today's
// machines (they grow with the square of the length). Describes the hardware, not the design.
constexpr uint32_t kMaxRankLength = 20000;

bool is_lower27(const FilterLine& line) { return line.kind == "text" && line.symbols_id == "lower27"; }

std::string to_text(std::span<const uint32_t> unit)
{
    std::string s(unit.size(), ' ');
    for (size_t i = 0; i < unit.size(); ++i)
    {
        if (unit[i] > 26) throw std::invalid_argument("not a lower27 unit");
        s[i] = kLower27[unit[i]];
    }
    return s;
}

// ---------------------------------------------------------------- clean

// States: 0 start, 1 SPACE before any letter, 2 last was a letter, 3 SPACE after a letter,
// 4 padding (v2: a second SPACE after a letter; only SPACEs follow).
class CleanRanker : public Ranker
{
public:
    CleanRanker(uint32_t L, bool padding) : L_(L), padding_(padding), cnt_(5, std::vector<BigUint>(L + 1))
    {
        cnt_[2][0] = BigUint(1);
        cnt_[3][0] = BigUint(1);
        if (padding) cnt_[4][0] = BigUint(1);
        for (uint32_t r = 1; r <= L; ++r)
        {
            BigUint letters = cnt_[2][r - 1];
            letters.mul_small(26);
            cnt_[0][r] = cnt_[1][r - 1];
            cnt_[0][r] += letters;
            cnt_[1][r] = letters;
            cnt_[2][r] = cnt_[3][r - 1];
            cnt_[2][r] += letters;
            cnt_[3][r] = letters;
            if (padding)
            {
                cnt_[3][r] += cnt_[4][r - 1];
                cnt_[4][r] = BigUint(1);
            }
        }
    }
    const BigUint& count() const override { return cnt_[0][L_]; }

    std::vector<uint32_t> unrank(const BigUint& k0) const override
    {
        if (k0 >= count()) throw std::out_of_range("rank beyond the survivors");
        BigUint k = k0;
        std::vector<uint32_t> u(L_);
        int s = 0;
        for (uint32_t i = 0; i < L_; ++i)
        {
            const uint32_t r = L_ - i;
            for (uint32_t c = 0; c <= 26; ++c)
            {
                const int t = next(s, c);
                if (t < 0) continue;
                const BigUint& n = cnt_[size_t(t)][r - 1];
                if (k < n) { u[i] = c; s = t; break; }
                k -= n;
            }
        }
        return u;
    }

    BigUint rank(std::span<const uint32_t> unit) const override
    {
        BigUint k;
        int s = 0;
        for (uint32_t i = 0; i < L_; ++i)
        {
            const uint32_t r = L_ - i;
            for (uint32_t c = 0; c < unit[i]; ++c)
                if (const int t = next(s, c); t >= 0) k += cnt_[size_t(t)][r - 1];
            s = next(s, unit[i]);
            if (s < 0) throw std::invalid_argument("unit does not pass clean");
        }
        return k;
    }

private:
    int next(int s, uint32_t c) const
    {
        if (c == 0) return s == 0 ? 1 : s == 2 ? 3 : (padding_ && (s == 3 || s == 4)) ? 4 : -1;
        return s == 4 ? -1 : 2;
    }
    uint32_t L_;
    bool padding_;
    std::vector<std::vector<BigUint>> cnt_;
};

class M1Filter : public Filter
{
public:
    M1Filter(SieveFilter kind, bool padding, std::shared_ptr<const Dictionary> dict, std::unique_ptr<Ranker> ranker)
        : kind_(kind), padding_(padding), dict_(std::move(dict)), ranker_(std::move(ranker))
    {
    }
    bool passes(std::span<const uint32_t> unit) const override
    {
        const std::string t = to_text(unit);
        if (padding_)
        {
            // Two or more trailing SPACEs are padding: judge what comes before them.
            size_t end = t.size();
            while (end > 0 && t[end - 1] == ' ') --end;
            if (t.size() - end >= 2) return end > 0 && unit_passes(std::string_view(t).substr(0, end), kind_, *dict_);
        }
        return unit_passes(t, kind_, *dict_);
    }
    const Ranker* ranker() const override { return ranker_.get(); }
    void set_provenance(std::string p) { provenance_ = std::move(p); }

private:
    SieveFilter kind_;
    bool padding_;
    std::shared_ptr<const Dictionary> dict_;
    std::unique_ptr<Ranker> ranker_;
};

// ---------------------------------------------------------------- words

// A trie of the dictionary. For each node, a histogram of how many words end m letters below it.
// Then, with A(r) the number of ways to fill r symbols right after a SPACE that follows a word:
//   F(n, r) = sum over words w below n, m = |w| - depth(n):  [r = m] + [r > m] A(r - m - 1)
//   A(0) = 1,  A(r) = F(root, r)
// and every count needed for ranking is one of these.
class WordsRanker : public Ranker
{
public:
    WordsRanker(const Dictionary& dict, uint32_t L, bool padding) : L_(L), padding_(padding)
    {
        nodes_.push_back({});
        for (const std::string& w : dict.words())
        {
            uint32_t n = 0;
            for (char ch : w)
            {
                const uint32_t c = uint32_t(ch - 'a' + 1);
                uint32_t child = find(n, c);
                if (child == kNone)
                {
                    child = uint32_t(nodes_.size());
                    nodes_[n].children.emplace_back(uint8_t(c), child);
                    nodes_.push_back({});
                }
                n = child;
            }
            nodes_[n].word = true;
        }
        for (auto& nd : nodes_) std::sort(nd.children.begin(), nd.children.end());
        // Histograms, deepest nodes first (children always have larger indices than parents).
        hist_.resize(nodes_.size());
        for (size_t i = nodes_.size(); i-- > 0;)
        {
            std::vector<uint32_t> h;
            if (nodes_[i].word) h.push_back(1);
            for (const auto& [c, child] : nodes_[i].children)
            {
                const auto& hc = hist_[child];
                if (h.size() < hc.size() + 1) h.resize(hc.size() + 1, 0);
                for (size_t m = 0; m < hc.size(); ++m) h[m + 1] += hc[m];
            }
            hist_[i] = std::move(h);
        }
        A_.resize(L + 1);
        A_[0] = BigUint(1);
        for (uint32_t r = 1; r <= L; ++r)
        {
            A_[r] = F(0, r);
            if (padding) A_[r].add_small(1); // v2: the rest is padding
        }
        total_ = F(0, L);
        if (L >= 1) total_ += F(0, L - 1); // a leading SPACE
    }

    const BigUint& count() const override { return total_; }

    std::vector<uint32_t> unrank(const BigUint& k0) const override
    {
        if (k0 >= total_) throw std::out_of_range("rank beyond the survivors");
        BigUint k = k0;
        std::vector<uint32_t> u(L_);
        State s{kStart, 0};
        for (uint32_t i = 0; i < L_; ++i)
        {
            const uint32_t r = L_ - i;
            for (uint32_t c = 0; c <= 26; ++c)
            {
                State t;
                if (!next(s, c, t)) continue;
                const BigUint n = after(t, r - 1);
                if (k < n) { u[i] = c; s = t; break; }
                k -= n;
            }
        }
        return u;
    }

    BigUint rank(std::span<const uint32_t> unit) const override
    {
        BigUint k;
        State s{kStart, 0};
        for (uint32_t i = 0; i < L_; ++i)
        {
            const uint32_t r = L_ - i;
            for (uint32_t c = 0; c < unit[i]; ++c)
            {
                State t;
                if (next(s, c, t)) k += after(t, r - 1);
            }
            State t;
            if (!next(s, unit[i], t)) throw std::invalid_argument("unit does not pass words");
            s = t;
        }
        return k;
    }

private:
    static constexpr uint32_t kNone = 0xFFFFFFFFu;
    enum Kind { kStart, kSpaceNoLetter, kWord, kSpaceAfterWord, kPadding };
    struct State
    {
        Kind kind;
        uint32_t node;
    };
    struct Node
    {
        std::vector<std::pair<uint8_t, uint32_t>> children;
        bool word = false;
    };

    uint32_t find(uint32_t n, uint32_t c) const
    {
        for (const auto& [cc, child] : nodes_[n].children)
            if (cc == c) return child;
        return kNone;
    }

    BigUint F(uint32_t n, uint32_t r) const
    {
        const auto& h = hist_[n];
        BigUint total;
        if (r < h.size() && h[r]) total = BigUint(h[r]);
        for (uint32_t m = 0; m < h.size() && m + 1 <= r; ++m)
            if (h[m])
            {
                BigUint t = A_[r - m - 1];
                t.mul_small(h[m]);
                total += t;
            }
        return total;
    }

    bool next(State s, uint32_t c, State& t) const
    {
        if (c == 0)
        {
            if (s.kind == kStart) { t = {kSpaceNoLetter, 0}; return true; }
            if (s.kind == kWord && nodes_[s.node].word) { t = {kSpaceAfterWord, 0}; return true; }
            if (padding_ && (s.kind == kSpaceAfterWord || s.kind == kPadding)) { t = {kPadding, 0}; return true; }
            return false;
        }
        if (s.kind == kPadding) return false;
        const uint32_t from = s.kind == kWord ? s.node : 0;
        const uint32_t child = find(from, c);
        if (child == kNone) return false;
        t = {kWord, child};
        return true;
    }

    BigUint after(State t, uint32_t r) const
    {
        switch (t.kind)
        {
        case kSpaceNoLetter: return F(0, r);
        case kSpaceAfterWord: return A_[r];
        case kPadding: return BigUint(1);
        case kWord: return F(t.node, r);
        default: return BigUint();
        }
    }

    uint32_t L_;
    bool padding_;
    std::vector<Node> nodes_;
    std::vector<std::vector<uint32_t>> hist_;
    std::vector<BigUint> A_;
    BigUint total_;
};

std::unique_ptr<Filter> make_m1(SieveFilter kind, bool padding, const FilterLine& line, const FilterValues& values, const FilterResources& res,
                                const FilterSpec& spec)
{
    // clean needs no words.
    auto dict = kind == SieveFilter::Clean ? std::make_shared<const Dictionary>(Dictionary::from_words({}))
                                           : res.dictionary(param_value(spec, values, "dictionary"));
    std::unique_ptr<Ranker> ranker;
    if (line.length <= kMaxRankLength)
    {
        if (kind == SieveFilter::Clean) ranker = std::make_unique<CleanRanker>(line.length, padding);
        if (kind == SieveFilter::Words) ranker = std::make_unique<WordsRanker>(*dict, line.length, padding);
    }
    auto f = std::make_unique<M1Filter>(kind, padding, dict, std::move(ranker));
    if (kind != SieveFilter::Clean)
    {
        const std::string id = param_value(spec, values, "dictionary");
        f->set_provenance("dictionary=" + (id.empty() ? std::string("default") : id) + " sha256=" + dict->sha256());
    }
    return f;
}

// Version 2 and later of clean and words allow trailing SPACE padding.
std::unique_ptr<Filter> make_m1(SieveFilter kind, const FilterLine& line, const FilterValues& values, const FilterResources& res,
                                const FilterSpec& spec)
{
    const bool padding = spec.version >= 2;
    return make_m1(kind, padding, line, values, res, spec);
}

} // namespace

void add_text_m1_filters(std::vector<FilterSpec>& out)
{
    const FilterParam dict{"dictionary", "registered dictionary id (empty: the default)", FilterParam::Kind::Text, "", 0, 0, 1};
    FilterSpec clean;
    clean.id = "clean";
    clean.title = "clean";
    clean.description = "No two SPACEs in a row, and at least one letter. Can rank (compact).";
    clean.applies = is_lower27;
    clean.make = [](const FilterLine& l, const FilterValues& v, const FilterResources& r) {
        return make_m1(SieveFilter::Clean, l, v, r, *find_filter("clean-v1"));
    };
    out.push_back(clean);

    FilterSpec window;
    window.id = "window";
    window.title = "window";
    window.description = "Could be cut from running English: whole words inside, a word ending at the left edge, "
                         "a word beginning at the right edge.";
    window.params = {dict};
    window.implies = {"clean-v1"};
    window.applies = is_lower27;
    window.make = [](const FilterLine& l, const FilterValues& v, const FilterResources& r) {
        return make_m1(SieveFilter::Window, l, v, r, *find_filter("window-v1"));
    };
    out.push_back(window);

    FilterSpec words;
    words.id = "words";
    words.title = "words";
    words.description = "Every token is a complete dictionary word. Can rank (compact).";
    words.params = {dict};
    words.implies = {"clean-v1", "window-v1"};
    words.applies = is_lower27;
    words.make = [](const FilterLine& l, const FilterValues& v, const FilterResources& r) {
        return make_m1(SieveFilter::Words, l, v, r, *find_filter("words-v1"));
    };
    out.push_back(words);

    // Version 2: trailing SPACE padding allowed (the last unit of warped text).
    FilterSpec clean2 = clean;
    clean2.version = 2;
    clean2.description = "As clean-v1, but a unit may end in SPACE padding (the last unit of warped text). Can rank (compact).";
    clean2.make = [](const FilterLine& l, const FilterValues& v, const FilterResources& r) {
        return make_m1(SieveFilter::Clean, l, v, r, *find_filter("clean-v2"));
    };
    out.push_back(clean2);
    FilterSpec words2 = words;
    words2.version = 2;
    words2.description = "As words-v1, but a unit may end in SPACE padding (the last unit of warped text). Can rank (compact).";
    words2.implies = {"clean-v2"};
    words2.make = [](const FilterLine& l, const FilterValues& v, const FilterResources& r) {
        return make_m1(SieveFilter::Words, l, v, r, *find_filter("words-v2"));
    };
    out.push_back(words2);
}

} // namespace sieve
