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
#include <memory>
#include <mutex>
#include <unordered_map>
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
    // States: 0 start, 1 SPACE before any letter, 2 after a letter, 3 SPACE after a word,
    // 4 padding (two or more trailing SPACEs; version 2 only). Only the "after a letter" column
    // is stored; the others follow from it with one small multiply (a quarter of the memory):
    //   c2(r) = c3(r-1) + 26 c2(r-1),  c1(r) = 26 c2(r-1),  c3(r) = 26 c2(r-1) + [padding]
    //   c0(r) = c1(r-1) + c1(r),       c4(r) = [padding],   with c2(0) = c3(0) = 1.
    CleanRanker(uint32_t L, bool padding) : L_(L), padding_(padding), c2_(L + 1)
    {
        c2_[0] = BigUint(1);
        for (uint32_t r = 1; r <= L; ++r)
        {
            c2_[r] = c3(r - 1);
            c2_[r] += c1(r);
        }
        set_count();
    }
    uint32_t length() const override { return L_; }
    uint32_t base() const override { return 27; }
    State start() const override { return 0; }
    State next(State s, uint32_t c) const override
    {
        if (c > 26) return kDead;
        if (c == 0) return s == 0 ? 1 : s == 2 ? 3 : (padding_ && (s == 3 || s == 4)) ? 4 : kDead;
        return s == 4 ? kDead : 2;
    }
    BigUint completions(State s, uint32_t r) const override
    {
        switch (s)
        {
        case 0:
        {
            if (r == 0) return BigUint();
            BigUint v = c1(r - 1);
            v += c1(r);
            return v;
        }
        case 1: return c1(r);
        case 2: return c2_[r];
        case 3: return c3(r);
        default: return BigUint(padding_ ? 1 : 0);
        }
    }
    bool alive(State s, uint32_t r) const override
    {
        switch (s)
        {
        case 0:
        case 1: return r > 0;
        case 2:
        case 3: return true;
        default: return padding_;
        }
    }

private:
    BigUint c1(uint32_t r) const
    {
        if (r == 0) return BigUint();
        BigUint v = c2_[r - 1];
        v.mul_small(26);
        return v;
    }
    BigUint c3(uint32_t r) const
    {
        if (r == 0) return BigUint(1);
        BigUint v = c1(r);
        if (padding_) v += BigUint(1);
        return v;
    }
    uint32_t L_;
    bool padding_;
    std::vector<BigUint> c2_;
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
        set_count();
    }

    uint32_t length() const override { return L_; }
    uint32_t base() const override { return 27; }
    State start() const override { return pack(kStart, 0); }
    State next(State st, uint32_t c) const override
    {
        if (c > 26) return kDead;
        const Kind kind = kind_of(st);
        const uint32_t node = node_of(st);
        if (c == 0)
        {
            if (kind == kStart) return pack(kSpaceNoLetter, 0);
            if (kind == kWord && nodes_[node].word) return pack(kSpaceAfterWord, 0);
            if (padding_ && (kind == kSpaceAfterWord || kind == kPadding)) return pack(kPadding, 0);
            return kDead;
        }
        if (kind == kPadding) return kDead;
        const uint32_t child = find(kind == kWord ? node : 0, c);
        return child == kNone ? kDead : pack(kWord, child);
    }
    BigUint completions(State st, uint32_t r) const override
    {
        switch (kind_of(st))
        {
        case kStart:
        {
            BigUint t = F(0, r);
            if (r >= 1) t += F(0, r - 1); // a leading SPACE
            return t;
        }
        case kSpaceNoLetter: return F(0, r);
        case kSpaceAfterWord: return A_[r];
        case kPadding: return BigUint(1);
        case kWord: return F(node_of(st), r);
        }
        return BigUint();
    }
    // F(n, r) > 0 without big-number arithmetic.
    bool alive(State st, uint32_t r) const override
    {
        switch (kind_of(st))
        {
        case kStart: return live_F(0, r) || (r >= 1 && live_F(0, r - 1));
        case kSpaceNoLetter: return live_F(0, r);
        case kSpaceAfterWord: return !A_[r].is_zero();
        case kPadding: return true;
        case kWord: return live_F(node_of(st), r);
        }
        return false;
    }

private:
    static constexpr uint32_t kNone = 0xFFFFFFFFu;
    enum Kind { kStart, kSpaceNoLetter, kWord, kSpaceAfterWord, kPadding };
    static State pack(Kind k, uint32_t node) { return (State(node) << 3) | State(k); }
    static Kind kind_of(State s) { return Kind(s & 7); }
    static uint32_t node_of(State s) { return uint32_t(s >> 3); }
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

    bool live_F(uint32_t n, uint32_t r) const
    {
        const auto& h = hist_[n];
        if (r < h.size() && h[r]) return true;
        for (uint32_t m = 0; m < h.size() && m + 1 <= r; ++m)
            if (h[m] && !A_[r - m - 1].is_zero()) return true;
        return false;
    }

    uint32_t L_;
    bool padding_;
    std::vector<Node> nodes_;
    std::vector<std::vector<uint32_t>> hist_;
    std::vector<BigUint> A_;
};

// ---------------------------------------------------------------- window

// Survivors of window (v1), and of window-v2 with trailing padding. A window unit is clean text
// whose inner tokens are whole words; its first token, if it touches the left edge, may be any
// suffix of a word (a word cut by the start of the unit), and its last token, if it touches the
// right edge, any prefix of a word; a single token touching both edges may be any substring.
//
// States: start; a SPACE before any letter; inside the edge token (identified by the range of
// word suffixes that begin with it); inside an inner token (a node of the word trie); a SPACE
// after a word; (v2) a SPACE after an unfinished token, which must be padding; padding.
// With B(r) the ways to finish r symbols right after a SPACE that follows a word:
//   G(n, r) = D_n[r] + sum_m W_n[m] B(r-m-1) + pad * sum_{m <= r-2} (D_n[m] - W_n[m])
//   E(p, r) = Sub_p[r] + sum_m Suf_p[m] B(r-m-1) + pad * sum_{m <= r-2} (Sub_p[m] - Suf_p[m])
// where D_n[m] / W_n[m] count the trie nodes / word ends m letters below n, and Sub_p[m] /
// Suf_p[m] the distinct substrings / suffixes of words that are p followed by m letters.
class WindowRanker : public Ranker
{
public:
    WindowRanker(const Dictionary& dict, uint32_t L, bool padding) : L_(L), pad_(padding), suf_(&dict.suffixes())
    {
        // The word trie, with word-end (W) and node (D) depth histograms, deepest nodes first.
        nodes_.push_back({});
        for (const std::string& w : dict.words())
        {
            uint32_t n = 0;
            for (char ch : w)
            {
                const uint32_t c = uint32_t(ch - 'a' + 1);
                uint32_t child = child_of(n, c);
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
        W_.resize(nodes_.size());
        D_.resize(nodes_.size());
        for (size_t i = nodes_.size(); i-- > 0;)
        {
            std::vector<uint32_t> w(1, nodes_[i].word ? 1 : 0), d(1, 1);
            for (const auto& [c, child] : nodes_[i].children)
            {
                if (w.size() < W_[child].size() + 1) w.resize(W_[child].size() + 1, 0);
                if (d.size() < D_[child].size() + 1) d.resize(D_[child].size() + 1, 0);
                for (size_t m = 0; m < W_[child].size(); ++m) w[m + 1] += W_[child][m];
                for (size_t m = 0; m < D_[child].size(); ++m) d[m + 1] += D_[child][m];
            }
            W_[i] = std::move(w);
            D_[i] = std::move(d);
        }
        // Common prefix lengths of neighbouring suffixes (for the edge token's substrings).
        lcp_.assign(suf_->size(), 0);
        for (size_t i = 1; i < suf_->size(); ++i)
        {
            const std::string &a = (*suf_)[i - 1], &b = (*suf_)[i];
            uint8_t k = 0;
            while (k < a.size() && k < b.size() && a[k] == b[k] && k < 255) ++k;
            lcp_[i] = k;
        }
        // B(r), in order: G at the root needs B below r - 1 only.
        B_.resize(L + 1);
        B_[0] = BigUint(1);
        for (uint32_t r = 1; r <= L; ++r)
        {
            B_[r] = G(0, r, true);
            if (pad_) B_[r].add_small(1);
        }
        set_count();
    }

    uint32_t length() const override { return L_; }
    uint32_t base() const override { return 27; }
    State start() const override { return pack(kStart, 0, 0); }
    State next(State st, uint32_t c) const override
    {
        if (c > 26) return kDead;
        const Kind k = kind_of(st);
        switch (k)
        {
        case kStart:
            if (c == 0) return pack(kSpaceNoLetter, 0, 0);
            return edge_child(0, 0, c);
        case kSpaceNoLetter:
        case kSpaceAfterWord:
            if (c == 0) return k == kSpaceAfterWord && pad_ ? pack(kPadding, 0, 0) : kDead;
            return word_child(0, c);
        case kEdge:
        {
            const uint32_t lo = lo_of(st), d = depth_of(st);
            if (c != 0) return edge_child(lo, d, c);
            if ((*suf_)[lo].size() == d) return pack(kSpaceAfterWord, 0, 0); // the edge token is a whole suffix
            return pad_ ? pack(kPendingPad, 0, 0) : kDead;
        }
        case kWord:
        {
            const uint32_t n = node_of(st);
            if (c != 0) return word_child(n, c);
            if (nodes_[n].word) return pack(kSpaceAfterWord, 0, 0);
            return pad_ ? pack(kPendingPad, 0, 0) : kDead;
        }
        case kPendingPad:
        case kPadding: return c == 0 ? pack(kPadding, 0, 0) : kDead;
        }
        return kDead;
    }
    BigUint completions(State st, uint32_t r) const override
    {
        switch (kind_of(st))
        {
        case kStart:
        {
            if (r == 0) return BigUint();
            BigUint t = r - 1 >= 1 ? G(0, r - 1, true) : BigUint(); // a leading SPACE, then an inner token
            for (uint32_t c = 1; c <= 26; ++c)
                if (const State e = edge_child(0, 0, c); e != kDead) t += completions(e, r - 1);
            return t;
        }
        case kSpaceNoLetter: return r >= 1 ? G(0, r, true) : BigUint();
        case kSpaceAfterWord: return B_[r];
        case kPendingPad: return BigUint(r >= 1 ? 1 : 0);
        case kPadding: return BigUint(1);
        case kWord: return G(node_of(st), r, false);
        case kEdge:
        {
            const auto h = edge_hist(lo_of(st), depth_of(st));
            return combine(h->sub, h->suf, r, 0);
        }
        }
        return BigUint();
    }

private:
    static constexpr uint32_t kNone = 0xFFFFFFFFu;
    enum Kind { kStart, kSpaceNoLetter, kEdge, kWord, kSpaceAfterWord, kPendingPad, kPadding };
    struct Node
    {
        std::vector<std::pair<uint8_t, uint32_t>> children;
        bool word = false;
    };
    struct Hist
    {
        std::vector<uint32_t> sub, suf;
    };
    // kind: 3 bits; edge: depth 8 bits, suffix index 32 bits; word: node 32 bits.
    static State pack(Kind k, uint32_t a, uint32_t b) { return State(k) | (State(a) << 3) | (State(b) << 11); }
    static Kind kind_of(State s) { return Kind(s & 7); }
    static uint32_t depth_of(State s) { return uint32_t((s >> 3) & 0xFF); }
    static uint32_t lo_of(State s) { return uint32_t(s >> 11); }
    static uint32_t node_of(State s) { return uint32_t(s >> 11); }

    uint32_t child_of(uint32_t n, uint32_t c) const
    {
        for (const auto& [cc, child] : nodes_[n].children)
            if (cc == c) return child;
        return kNone;
    }
    State word_child(uint32_t n, uint32_t c) const
    {
        const uint32_t child = child_of(n, c);
        return child == kNone ? kDead : pack(kWord, 0, child);
    }
    // The suffixes starting with (the first d letters of suffix lo) + letter c.
    State edge_child(uint32_t lo, uint32_t d, uint32_t c) const
    {
        const auto& S = *suf_;
        if (d >= 255) return kDead;
        std::string p = d ? S[lo].substr(0, d) : std::string();
        p.push_back(char('a' + c - 1));
        const auto first = S.begin() + lo;
        const auto it = std::lower_bound(first, S.end(), p);
        if (it == S.end() || it->compare(0, p.size(), p) != 0) return kDead;
        return pack(kEdge, d + 1, uint32_t(it - S.begin()));
    }

    // Sub[m], Suf[m] for the suffixes beginning with the first d letters of suffix lo.
    // (Shared pointers: another thread may clear the cache while this one still reads its entry.)
    std::shared_ptr<const Hist> edge_hist(uint32_t lo, uint32_t d) const
    {
        const uint64_t key = (uint64_t(lo) << 8) | d;
        std::lock_guard<std::mutex> lock(mu_);
        if (auto it = cache_.find(key); it != cache_.end()) return it->second;
        if (cache_.size() > 200000) cache_.clear();
        const auto& S = *suf_;
        auto made = std::make_shared<Hist>();
        Hist& h = *made;
        h.sub.assign(1, 1); // the token itself
        for (size_t i = lo; i < S.size() && (i == lo || lcp_[i] >= d); ++i)
        {
            const size_t len = S[i].size();
            if (h.suf.size() < len - d + 1) h.suf.resize(len - d + 1, 0);
            ++h.suf[len - d];
            // New distinct prefixes of this suffix: lengths past what it shares with the previous one.
            const size_t from = i == lo ? d : std::max<size_t>(lcp_[i], d);
            if (h.sub.size() < len - d + 1) h.sub.resize(len - d + 1, 0);
            for (size_t k = from + 1; k <= len; ++k) ++h.sub[k - d];
        }
        h.suf.resize(h.sub.size(), 0);
        return cache_.emplace(key, std::move(made)).first->second;
    }

    // D[r] + sum_{m >= m0} W[m] B(r-m-1) + pad * sum_{m0 <= m <= r-2} (D[m] - W[m]).
    BigUint combine(const std::vector<uint32_t>& D, const std::vector<uint32_t>& W, uint32_t r, uint32_t m0) const
    {
        BigUint t;
        if (r < D.size() && D[r]) t = BigUint(D[r]);
        for (uint32_t m = m0; m < W.size() && m + 1 <= r; ++m)
            if (W[m])
            {
                BigUint x = B_[r - m - 1];
                x.mul_small(W[m]);
                t += x;
            }
        if (pad_)
        {
            uint64_t k = 0;
            for (uint32_t m = m0; m < D.size() && m + 2 <= r; ++m) k += D[m] - (m < W.size() ? W[m] : 0);
            if (k) t += BigUint(k);
        }
        return t;
    }
    // Inside an inner token at trie node n (or, at_root, just before one: the token's letters are
    // then counted from depth 1).
    BigUint G(uint32_t n, uint32_t r, bool at_root) const { return combine(D_[n], W_[n], r, at_root ? 1 : 0); }

    uint32_t L_;
    bool pad_;
    const std::vector<std::string>* suf_;
    std::vector<Node> nodes_;
    std::vector<std::vector<uint32_t>> W_, D_;
    std::vector<uint8_t> lcp_;
    std::vector<BigUint> B_;
    mutable std::mutex mu_;
    mutable std::unordered_map<uint64_t, std::shared_ptr<const Hist>> cache_;
};

// ---------------------------------------------------------------- title

// A title: words (as words-v2) in the first N = min(max_length, L) symbols, then only SPACEs.
// A unit passes exactly when its first N symbols pass words-v2 as a unit of length N and the
// rest are SPACEs (whatever the padding, the words before it are then judged the same way), so
// its survivors are words-v2's at length N, each followed by SPACEs: the ranker walks words-v2
// for N symbols and then allows only SPACE.
class TitleRanker : public Ranker
{
public:
    TitleRanker(std::unique_ptr<Ranker> inner, uint32_t L) : inner_(std::move(inner)), L_(L), N_(inner_->length()) { set_count(); }
    uint32_t length() const override { return L_; }
    uint32_t base() const override { return 27; }
    State start() const override { return pack(0, inner_->start()); }
    State next(State s, uint32_t c) const override
    {
        const uint32_t p = pos_of(s);
        if (p >= L_) return kDead;
        if (p >= N_) return c == 0 ? pack(p + 1, 0) : kDead;
        const State t = inner_->next(inner_of(s), c);
        if (t == kDead || t > 0xFFFFFFFFull) return kDead;
        if (p + 1 == N_ && !inner_->alive(t, 0)) return kDead;
        return pack(p + 1, t);
    }
    BigUint completions(State s, uint32_t) const override
    {
        const uint32_t p = pos_of(s);
        if (N_ == 0) return BigUint(); // a title needs at least one letter: nothing passes at length 0
        if (p >= N_) return BigUint(1);
        return inner_->completions(inner_of(s), N_ - p);
    }
    bool alive(State s, uint32_t) const override
    {
        const uint32_t p = pos_of(s);
        if (N_ == 0) return false;
        return p >= N_ || inner_->alive(inner_of(s), N_ - p);
    }

private:
    static State pack(uint32_t pos, State inner) { return (State(pos) << 32) | inner; }
    static uint32_t pos_of(State s) { return uint32_t(s >> 32); }
    static State inner_of(State s) { return s & 0xFFFFFFFFull; }
    std::unique_ptr<Ranker> inner_;
    uint32_t L_, N_;
};

class TitleFilter : public Filter
{
public:
    TitleFilter(std::shared_ptr<const Dictionary> dict, uint32_t L, uint32_t max_length, std::string provenance)
        : words_(SieveFilter::Words, true, std::move(dict), nullptr), N_(std::min(L, max_length))
    {
        provenance_ = std::move(provenance);
    }
    bool passes(std::span<const uint32_t> unit) const override
    {
        for (size_t i = N_; i < unit.size(); ++i)
            if (unit[i] != 0) return false;
        return words_.passes(unit.first(std::min<size_t>(N_, unit.size())));
    }
    const Ranker* ranker() const override { return ranker_.get(); }
    void set_ranker(std::unique_ptr<Ranker> r) { ranker_ = std::move(r); }

private:
    M1Filter words_;
    uint32_t N_;
    std::unique_ptr<Ranker> ranker_;
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
        if (kind == SieveFilter::Window) ranker = std::make_unique<WindowRanker>(*dict, line.length, padding);
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
    const FilterParam dict{"dictionary", "registered dictionary id (empty: the default)", FilterParam::Kind::Text, "", 0, 0, 1, {}};
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
                         "a word beginning at the right edge. Can rank (compact).";
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
    words2.implies = {"clean-v2", "window-v2"};
    words2.make = [](const FilterLine& l, const FilterValues& v, const FilterResources& r) {
        return make_m1(SieveFilter::Words, l, v, r, *find_filter("words-v2"));
    };
    out.push_back(words2);
    // Window with padding: real pages are cut from running text, so a page may begin and end in
    // the middle of a word; the last page of a text ends in padding.
    FilterSpec window2 = window;
    window2.version = 2;
    window2.description = "As window-v1, but a unit may end in SPACE padding (the last page of a text). Pages cut from "
                          "real books pass: words cut by the page edges are allowed. Can rank (compact).";
    window2.implies = {"clean-v2"};
    window2.make = [](const FilterLine& l, const FilterValues& v, const FilterResources& r) {
        return make_m1(SieveFilter::Window, l, v, r, *find_filter("window-v2"));
    };
    out.push_back(window2);

    FilterSpec title;
    title.id = "title";
    title.title = "title";
    title.description = "A title: whole dictionary words within the first max_length characters, then only SPACEs "
                        "(for a book's title page). Can rank (compact).";
    title.params = {dict, {"max_length", "longest title, in characters", FilterParam::Kind::Integer, "64", 1, 1000000, 8, {}}};
    title.implies = {"clean-v2", "window-v2", "words-v2"};
    title.applies = is_lower27;
    title.make = [](const FilterLine& l, const FilterValues& v, const FilterResources& r) -> std::unique_ptr<Filter> {
        const FilterSpec& spec = *find_filter("title-v1");
        const std::string id = param_value(spec, v, "dictionary");
        auto dict = r.dictionary(id);
        const uint32_t max_length = uint32_t(param_int(spec, v, "max_length"));
        const uint32_t n = std::min(l.length, max_length);
        auto f = std::make_unique<TitleFilter>(dict, l.length, max_length,
                                               "dictionary=" + (id.empty() ? std::string("default") : id) + " sha256=" + dict->sha256() +
                                                   " max_length=" + std::to_string(max_length));
        if (n <= kMaxRankLength) f->set_ranker(std::make_unique<TitleRanker>(std::make_unique<WordsRanker>(*dict, n, true), l.length));
        return f;
    };
    out.push_back(title);
}

} // namespace sieve
