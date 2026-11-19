// Sieve — filters for pictures that count on every palette, rgb24 included (image and video):
//   palette-size-v1  at most `colours` distinct colours in the unit, or, with `scope = frame` on a
//                    video, in each frame on its own. Pixel art, icons and diagrams use a handful of
//                    colours; noise in a large palette uses nearly as many as it has pixels.
//   row-runs-v1      along each row of pixels, at most `changes` places where a pixel differs from
//                    the one to its left. Flat areas and edges pass; static changes almost every
//                    pixel. The rows of a video are its frames' rows, one after another.
//   border-v1        (list Q) every pixel of the picture's edge one colour: a kind of picture, ticked by hand.
//   specks-v1        (list Q) no lone specks: every pixel shares its colour with a neighbour. An automaton of the
//                    row above and which of its pixels are matched, where it fits.
//   gradient-v1      (list Q) colours never falling (or never rising) along the rows, or down the columns.
//   rows-alike-v1    (list N) each row differs from the row above in at most `changes` pixels: row-runs'
//                    other half (row-runs is blind up and down). An automaton of the row above, where it fits.
// Both rank exactly, with completions that depend on a few numbers rather than on which colours
// were used, so neither steps through the palette symbol by symbol: rank and unrank are worked out
// arithmetically at each pixel, which keeps 2^24 colours as cheap as two.
//
// palette-size: with m colours used so far in the current scope and r pixels left in it, the ways
// to finish are comp(m, r) = m comp(m, r - 1) + (B - m) comp(m + 1, r - 1), with comp(m, 0) = 1
// and comp(k + 1, .) = 0; a video's later frames multiply by comp(0, W H) each. A pixel's block of
// smaller symbols holds the used colours below it, each worth comp(m, r - 1), and the rest,
// each worth comp(m + 1, r - 1).
//
// row-runs: with t changes so far in the current row and r pixels left in the unit, the next
// pixel's place in its row is fixed by r, so G(t, r) = B G(0, r - 1) at the start of a row and
// G(t, r - 1) + (B - 1) G(t + 1, r - 1) inside it (G(changes + 1, .) = 0, G(t, 0) = 1). Below a
// pixel inside a row stand the same colour as its left neighbour (if smaller) and the others.
//
// On a small palette each is also an automaton over the pixels (row-runs: the column, the changes
// so far in the row and the last colour; palette-size: the colours used so far, and on a video
// counted per frame the place in the frame), built when it is small enough (as max-run-v1's is):
// then it combines with every other automaton (not-packed-v1, the other one, plugins) and the
// stack counts as one. On a large palette (rgb24) the automaton would be too large, and each counts
// with its own ranker above ("own" in docs/FILTERS-CONFLICTS.md); counts_as_on says which a line
// gets, judged at the largest settings it allows.

#include "sieve/filter.hpp"
#include "sieve/plugin.hpp"

#include "changes_ranker.hpp"

#include <algorithm>
#include <map>
#include <memory>
#include <string>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <tuple>
#include <unordered_map>
#include <vector>

namespace sieve {

namespace {

// States a walk reaches, interned: the rankers below need more than 64 bits of state (a set of
// colours, a pixel's place), but only the states a walk actually reaches are ever asked about.
template <class Key>
class Interned
{
public:
    Ranker::State id(const Key& k) const
    {
        std::lock_guard<std::mutex> lock(mx_);
        const auto [it, added] = ids_.emplace(k, Ranker::State(keys_.size()));
        if (added) keys_.push_back(k);
        return it->second;
    }
    Key key(Ranker::State s) const
    {
        std::lock_guard<std::mutex> lock(mx_);
        return keys_[size_t(s)];
    }

private:
    mutable std::mutex mx_;
    mutable std::map<Key, Ranker::State> ids_;
    mutable std::vector<Key> keys_;
};

// ---------------------------------------------------------------- palette-size

class PaletteRanker : public Ranker
{
public:
    // A unit of `length` pixels in scopes of `scope` pixels each (the whole unit, or one frame).
    PaletteRanker(uint32_t base, uint32_t length, uint32_t scope, uint32_t colours)
        : b_(base), l_(length), s_(scope), k_(std::min(colours, base))
    {
        comp_.assign(k_ + 2, std::vector<BigUint>(s_ + 1));
        for (uint32_t m = 0; m <= k_; ++m) comp_[m][0] = BigUint(1);
        for (uint32_t r = 1; r <= s_; ++r)
            for (uint32_t m = 0; m <= k_; ++m)
            {
                BigUint& c = comp_[m][r];
                c.add_mul_small(comp_[m][r - 1], m);
                c.add_mul_small(comp_[m + 1][r - 1], b_ - m);
            }
        whole_.push_back(BigUint(1));
        for (uint32_t f = 0; f < l_ / s_; ++f) whole_.push_back(BigUint::mul(whole_.back(), comp_[0][s_]));
        start_ = states_.id({0, {}});
        set_count();
    }
    uint32_t length() const override { return l_; }
    uint32_t base() const override { return b_; }
    State start() const override { return start_; }
    State next(State s, uint32_t symbol) const override
    {
        if (s == kDead || symbol >= b_) return kDead;
        auto [pos, used] = states_.key(s);
        if (pos >= l_) return kDead;
        if (!std::binary_search(used.begin(), used.end(), symbol))
        {
            if (used.size() >= k_) return kDead;
            used.insert(std::upper_bound(used.begin(), used.end(), symbol), symbol);
        }
        ++pos;
        if (pos % s_ == 0) used.clear();
        return states_.id({pos, used});
    }
    BigUint completions(State s, uint32_t remaining) const override
    {
        if (s == kDead) return {};
        const auto [pos, used] = states_.key(s);
        if (uint64_t(pos) + remaining != l_) return {};
        return ways(pos, uint32_t(used.size()));
    }
    BigUint rank(std::span<const uint32_t> unit) const override
    {
        if (unit.size() != l_) throw std::invalid_argument("unit has the wrong length");
        BigUint r;
        std::vector<uint32_t> used;
        for (uint32_t pos = 0; pos < l_; ++pos)
        {
            const uint32_t d = unit[pos], m = uint32_t(used.size());
            const uint32_t below_used = uint32_t(std::lower_bound(used.begin(), used.end(), d) - used.begin());
            const bool known = std::binary_search(used.begin(), used.end(), d);
            if (!known && m >= k_) throw std::invalid_argument("the unit is not a survivor");
            BigUint a = ways(pos + 1, m), n = m < k_ ? ways(pos + 1, m + 1) : BigUint();
            if ((pos + 1) % s_ == 0)
            {
                a = ways(pos + 1, 0);
                n = m < k_ ? a : BigUint();
            }
            a.mul_small(below_used);
            n.mul_small(d - below_used);
            r += a;
            r += n;
            if (!known) used.insert(used.begin() + below_used, d);
            if ((pos + 1) % s_ == 0) used.clear();
        }
        return r;
    }
    std::vector<uint32_t> unrank(const BigUint& k) const override
    {
        if (!(k < count())) throw std::out_of_range("survivor number beyond the survivors");
        BigUint rest = k;
        std::vector<uint32_t> used, out;
        for (uint32_t pos = 0; pos < l_; ++pos)
        {
            const uint32_t m = uint32_t(used.size());
            BigUint a = ways(pos + 1, m), n = m < k_ ? ways(pos + 1, m + 1) : BigUint();
            if ((pos + 1) % s_ == 0)
            {
                a = ways(pos + 1, 0);
                n = m < k_ ? a : BigUint();
            }
            // The symbols below x are worth below(x) = u(x) a + (x - u(x)) n, u(x) the used ones
            // below x; the pixel is the largest x with below(x) <= rest.
            auto below = [&](uint32_t x) {
                const uint32_t u = uint32_t(std::lower_bound(used.begin(), used.end(), x) - used.begin());
                BigUint t = a;
                t.mul_small(u);
                BigUint w = n;
                w.mul_small(x - u);
                return t += w;
            };
            uint32_t lo = 0, hi = b_ - 1;
            while (lo < hi)
            {
                const uint32_t mid = lo + (hi - lo + 1) / 2;
                if (below(mid) <= rest) lo = mid;
                else hi = mid - 1;
            }
            // The largest x with below(x) <= rest may be a symbol the walk cannot take (a new
            // colour when the palette is full): step back to the last one it can.
            while (!std::binary_search(used.begin(), used.end(), lo) && m >= k_) --lo;
            rest -= below(lo);
            out.push_back(lo);
            if (!std::binary_search(used.begin(), used.end(), lo)) used.insert(std::upper_bound(used.begin(), used.end(), lo), lo);
            if ((pos + 1) % s_ == 0) used.clear();
        }
        return out;
    }

private:
    uint32_t b_, l_, s_, k_;
    std::vector<std::vector<BigUint>> comp_;
    std::vector<BigUint> whole_; // comp(0, scope)^frames
    State start_ = 0;
    Interned<std::pair<uint32_t, std::vector<uint32_t>>> states_;

    // The ways to finish from pixel `pos` with m colours used in its scope.
    BigUint ways(uint32_t pos, uint32_t m) const
    {
        if (m > k_) return {};
        const uint32_t in_scope = (s_ - pos % s_) % s_; // pixels left in this scope (0 at a boundary)
        const uint32_t frames_after = (l_ - pos - in_scope) / s_;
        if (in_scope == 0) return whole_[frames_after];
        return BigUint::mul(comp_[m][in_scope], whole_[frames_after]);
    }
};

class PaletteSize : public Filter
{
public:
    PaletteSize(const FilterLine& l, uint32_t colours, bool per_frame, std::string prov)
        : l_(l), k_(colours), scope_(per_frame && l.frames > 1 ? l.width * l.height : l.length)
    {
        provenance_ = std::move(prov);
    }
    bool passes(std::span<const uint32_t> unit) const override
    {
        for (size_t at = 0; at < unit.size(); at += scope_)
        {
            std::vector<uint32_t> part(unit.begin() + at, unit.begin() + std::min(unit.size(), at + scope_));
            std::sort(part.begin(), part.end());
            if (uint32_t(std::unique(part.begin(), part.end()) - part.begin()) > k_) return false;
        }
        return true;
    }
    const Ranker* ranker() const override
    {
        std::call_once(once_, [&] { ranker_ = std::make_unique<PaletteRanker>(l_.base, l_.length, scope_, k_); });
        return ranker_.get();
    }
    bool can_rank() const override { return true; }

private:
    FilterLine l_;
    uint32_t k_, scope_;
    mutable std::once_flag once_;
    mutable std::unique_ptr<PaletteRanker> ranker_;
};

// ---------------------------------------------------------------- row-runs

class RowRunsRanker : public Ranker
{
public:
    RowRunsRanker(uint32_t base, uint32_t width, uint32_t length, uint32_t changes) : b_(base), w_(width), l_(length), c_(changes)
    {
        g_.assign(c_ + 2, std::vector<BigUint>(l_ + 1));
        for (uint32_t t = 0; t <= c_; ++t) g_[t][0] = BigUint(1);
        for (uint32_t r = 1; r <= l_; ++r)
        {
            const bool row_start = (l_ - r) % w_ == 0;
            for (uint32_t t = 0; t <= c_; ++t)
            {
                if (row_start)
                {
                    BigUint x = g_[0][r - 1];
                    x.mul_small(b_);
                    g_[t][r] = x;
                    continue;
                }
                g_[t][r] = g_[t][r - 1];
                g_[t][r].add_mul_small(g_[t + 1][r - 1], b_ - 1);
            }
        }
        start_ = states_.id({0, 0, 0});
        set_count();
    }
    uint32_t length() const override { return l_; }
    uint32_t base() const override { return b_; }
    State start() const override { return start_; }
    State next(State s, uint32_t symbol) const override
    {
        if (s == kDead || symbol >= b_) return kDead;
        auto [pos, t, last] = states_.key(s);
        if (pos >= l_) return kDead;
        if (pos % w_ == 0) t = 0;
        else if (symbol != last && ++t > c_) return kDead;
        return states_.id({pos + 1, t, symbol});
    }
    BigUint completions(State s, uint32_t remaining) const override
    {
        if (s == kDead) return {};
        const auto [pos, t, last] = states_.key(s);
        if (uint64_t(pos) + remaining != l_) return {};
        return g_[(pos % w_ == 0) ? 0 : t][remaining];
    }
    BigUint rank(std::span<const uint32_t> unit) const override
    {
        if (unit.size() != l_) throw std::invalid_argument("unit has the wrong length");
        BigUint r;
        uint32_t t = 0;
        for (uint32_t pos = 0; pos < l_; ++pos)
        {
            const uint32_t d = unit[pos], rem = l_ - pos - 1;
            if (pos % w_ == 0)
            {
                BigUint x = g_[0][rem];
                x.mul_small(d);
                r += x;
                t = 0;
                continue;
            }
            const uint32_t last = unit[pos - 1];
            const bool same_below = last < d;
            BigUint other = g_[t + 1][rem];
            other.mul_small(d - (same_below ? 1 : 0));
            r += other;
            if (same_below) r += g_[t][rem];
            if (d != last && ++t > c_) throw std::invalid_argument("the unit is not a survivor");
        }
        return r;
    }
    std::vector<uint32_t> unrank(const BigUint& k) const override
    {
        if (!(k < count())) throw std::out_of_range("survivor number beyond the survivors");
        BigUint rest = k;
        std::vector<uint32_t> out;
        uint32_t t = 0;
        for (uint32_t pos = 0; pos < l_; ++pos)
        {
            const uint32_t rem = l_ - pos - 1;
            if (pos % w_ == 0)
            {
                BigUint q, rr;
                BigUint::divmod(rest, g_[0][rem], q, rr);
                out.push_back(uint32_t(std::stoul(q.to_decimal())));
                rest = rr;
                t = 0;
                continue;
            }
            const uint32_t last = out.back();
            const BigUint& same = g_[t][rem];
            const BigUint& other = g_[t + 1][rem];
            // Below a symbol x: (x - [last < x]) others and, when last < x, the same colour once.
            BigUint before_last = other;
            before_last.mul_small(last); // the symbols below last, all of them others
            uint32_t d;
            if (rest < before_last)
            {
                BigUint q, rr;
                BigUint::divmod(rest, other, q, rr);
                d = uint32_t(std::stoul(q.to_decimal()));
                rest = rr;
            }
            else
            {
                rest -= before_last;
                if (rest < same) d = last;
                else
                {
                    rest -= same;
                    BigUint q, rr;
                    BigUint::divmod(rest, other, q, rr);
                    d = last + 1 + uint32_t(std::stoul(q.to_decimal()));
                    rest = rr;
                }
            }
            if (d != last) ++t;
            out.push_back(d);
        }
        return out;
    }

private:
    uint32_t b_, w_, l_, c_;
    std::vector<std::vector<BigUint>> g_;
    State start_ = 0;
    Interned<std::tuple<uint32_t, uint32_t, uint32_t>> states_; // (pixel, changes in its row, last colour)
};

class RowRuns : public Filter
{
public:
    RowRuns(const FilterLine& l, uint32_t changes, std::string prov) : l_(l), c_(changes)
    {
        provenance_ = std::move(prov);
    }
    bool passes(std::span<const uint32_t> unit) const override
    {
        for (size_t row = 0; row < unit.size(); row += l_.width)
        {
            uint32_t t = 0;
            for (size_t x = 1; x < l_.width && row + x < unit.size(); ++x)
                if (unit[row + x] != unit[row + x - 1] && ++t > c_) return false;
        }
        return true;
    }
    const Ranker* ranker() const override
    {
        std::call_once(once_, [&] { ranker_ = std::make_unique<RowRunsRanker>(l_.base, l_.width, l_.length, c_); });
        return ranker_.get();
    }
    bool can_rank() const override { return true; }

private:
    FilterLine l_;
    uint32_t c_;
    mutable std::once_flag once_;
    mutable std::unique_ptr<RowRunsRanker> ranker_;
};

bool picture(const FilterLine& l)
{
    return (l.kind == "image" || l.kind == "video") && l.width && l.height && l.base >= 2 && l.length % l.width == 0;
}

// An automaton is built where its states times its symbols stay under this (max-run-v1's bound).
constexpr uint64_t kMaxDfaCells = uint64_t(1) << 24;
// Whether `states` states over `base` symbols are small enough (worked out without overflow).
bool cells_fit(uint64_t states, uint64_t base) { return base > 0 && states <= kMaxDfaCells / base; }

// row-runs' states: the row's start, then (column 1..W-1, changes 0..c, last colour), or past the
// bound, the bound and one.
uint64_t row_runs_states(uint64_t B, uint64_t W, uint64_t c)
{
    if (W <= 1) return 1;
    if (B > kMaxDfaCells || (c + 1) > kMaxDfaCells / B || (W - 1) > kMaxDfaCells / ((c + 1) * B)) return kMaxDfaCells + 1;
    return 1 + (W - 1) * (c + 1) * B;
}

std::optional<Dfa> row_runs_dfa(uint32_t B, uint32_t W, uint32_t changes)
{
    const uint32_t c = std::min(changes, W > 0 ? W - 1 : 0); // more changes than a row has places allows anything
    const uint64_t n = row_runs_states(B, W, c);
    if (!cells_fit(n, B)) return std::nullopt;
    Dfa d;
    d.base = B;
    d.start = 0;
    d.accept.assign(size_t(n), 1);
    d.next.assign(size_t(n) * B, Dfa::kDead);
    auto at = [&](uint32_t x, uint32_t t, uint32_t last) { return int32_t(1 + (uint64_t(x - 1) * (c + 1) + t) * B + last); };
    for (uint32_t col = 0; col < B; ++col) d.next[col] = W == 1 ? 0 : at(1, 0, col);
    for (uint32_t x = 1; x < W; ++x)
        for (uint32_t t = 0; t <= c; ++t)
            for (uint32_t last = 0; last < B; ++last)
            {
                const size_t s = size_t(at(x, t, last));
                for (uint32_t col = 0; col < B; ++col)
                {
                    const uint32_t tt = t + (col != last ? 1 : 0);
                    if (tt > c) continue;
                    d.next[s * B + col] = x + 1 == W ? 0 : at(x + 1, tt, col);
                }
            }
    return minimise(d);
}

// palette-size's states: the sets of at most k colours, and per frame the place in the frame.
uint64_t palette_states(uint64_t B, uint64_t k, uint64_t scope, bool per_frame)
{
    uint64_t sets = 0, c = 1; // sum of C(B, i), i <= k, stopping once it is too many to build
    for (uint64_t i = 0; i <= std::min(k, B) && sets <= kMaxDfaCells; ++i)
    {
        sets += c;
        c = c * (B - i) / (i + 1);
    }
    if (sets > kMaxDfaCells) return kMaxDfaCells + 1;
    return per_frame ? (scope > kMaxDfaCells / std::max<uint64_t>(sets, 1) ? kMaxDfaCells + 1 : sets * scope) : sets;
}

std::optional<Dfa> palette_dfa(uint32_t B, uint32_t length, uint32_t scope, uint32_t colours)
{
    const uint32_t k = std::min({colours, B, scope});
    const bool per_frame = scope < length;
    if (!cells_fit(palette_states(B, k, scope, per_frame), B)) return std::nullopt;
    // Breadth first from the empty set (at the frame's first pixel): only the states reached.
    std::map<std::pair<uint32_t, std::vector<uint32_t>>, int32_t> id;
    std::vector<std::pair<uint32_t, std::vector<uint32_t>>> keys;
    auto get = [&](uint32_t pos, const std::vector<uint32_t>& used) {
        const auto [it, added] = id.emplace(std::make_pair(pos, used), int32_t(keys.size()));
        if (added) keys.emplace_back(pos, used);
        return it->second;
    };
    Dfa d;
    d.base = B;
    d.start = get(0, {});
    for (size_t i = 0; i < keys.size(); ++i)
    {
        const auto [pos, used] = keys[i];
        d.accept.push_back(1);
        for (uint32_t col = 0; col < B; ++col)
        {
            std::vector<uint32_t> u = used;
            if (!std::binary_search(u.begin(), u.end(), col))
            {
                if (u.size() >= k)
                {
                    d.next.push_back(Dfa::kDead);
                    continue;
                }
                u.insert(std::upper_bound(u.begin(), u.end(), col), col);
            }
            uint32_t p = per_frame ? pos + 1 : 0;
            if (per_frame && p == scope)
            {
                p = 0;
                u.clear();
            }
            d.next.push_back(get(p, u));
        }
    }
    return minimise(d);
}


// ---------------------------------------------------------------- border, specks, gradient (list Q)

// Each built once a line's shape and kept (the filters' lists ask whether one applies often): the automaton, or
// nothing where it does not fit, so a filter is offered only where it counts.
template <class Make>
std::shared_ptr<const std::optional<Dfa>> kept_dfa(const std::string& key, Make make)
{
    static std::mutex m;
    static std::map<std::string, std::shared_ptr<const std::optional<Dfa>>> kept;
    {
        std::lock_guard<std::mutex> lock(m);
        if (const auto it = kept.find(key); it != kept.end()) return it->second;
    }
    auto d = std::make_shared<const std::optional<Dfa>>(make());
    std::lock_guard<std::mutex> lock(m);
    return kept.emplace(key, std::move(d)).first->second;
}

bool still_picture(const FilterLine& l) { return l.kind == "image" && l.width && l.height && l.base >= 2 && l.length == l.width * l.height; }

// The pictures framed in one colour: every pixel of the top and bottom rows and the left and right columns the
// same colour. States: the pixel's place, and the frame's colour once the first pixel has set it.
std::optional<Dfa> border_dfa(uint32_t B, uint32_t W, uint32_t H)
{
    const uint64_t n = uint64_t(W) * H * B + 1;
    if (!cells_fit(n, B)) return std::nullopt;
    Dfa d;
    d.base = B;
    d.start = 0;
    d.accept.assign(size_t(n), 0);
    d.next.assign(size_t(n) * B, Dfa::kDead);
    // state 1 + (i * B + c): pixel i (0..W H - 1, the next to place) with the frame colour c; i = W H: done
    auto at = [&](uint64_t i, uint32_t c) { return int32_t(1 + (i % (uint64_t(W) * H)) * B + c); };
    const uint64_t P = uint64_t(W) * H;
    auto edge = [&](uint64_t i) {
        const uint64_t x = i % W, y = i / W;
        return x == 0 || y == 0 || x + 1 == W || y + 1 == H;
    };
    for (uint32_t c = 0; c < B; ++c) d.next[c] = P == 1 ? at(0, c) : at(1, c); // (the first pixel is the frame's)
    for (uint64_t i = 1; i <= P; ++i)
        for (uint32_t c = 0; c < B; ++c)
        {
            if (i == P)
            {
                d.accept[size_t(at(i, c))] = 1; // (at(P, c) is at(0, c): the picture done)
                continue;
            }
            const size_t s = size_t(at(i, c));
            for (uint32_t x = 0; x < B; ++x)
                if (!edge(i) || x == c) d.next[s * B + x] = at(i + 1, c);
        }
    if (P == 1) for (uint32_t c = 0; c < B; ++c) d.accept[size_t(at(0, c))] = 1;
    return minimise(d);
}

// No lone specks: every pixel shares its colour with one of its four neighbours at least. Pixels are placed row by
// row; a state holds the last W pixels (the row above the next one) with, for each, whether a neighbour has matched
// it yet, the next pixel's column, and whether the first row is done. A pixel leaves when the one below it is
// placed, its last chance; the picture is done when every pixel left matched. Built where its states fit.
std::optional<Dfa> specks_dfa(uint32_t B, uint32_t W)
{
    uint32_t bits = 1;
    while ((1u << bits) < B) ++bits;
    if (uint64_t(W) * (bits + 1) > 46) return std::nullopt; // (the window packed in a key beside its column)
    // key: column (8 bits) | first row done (1) | colours (W * bits) | flags (W), the window oldest first
    struct Win
    {
        uint32_t x = 0;
        bool below = false; // the first row done
        std::vector<uint32_t> col;
        std::vector<uint8_t> ok;
    };
    auto key = [&](const Win& w) {
        uint64_t k = uint64_t(w.x) | (uint64_t(w.below) << 8) | (uint64_t(w.col.size()) << 9);
        uint64_t v = 0;
        for (size_t i = 0; i < w.col.size(); ++i) v = (v << (bits + 1)) | (uint64_t(w.col[i]) << 1) | w.ok[i];
        return k | (v << 16);
    };
    std::unordered_map<uint64_t, int32_t> id;
    std::vector<Win> states{Win{}};
    id[key(states[0])] = 0;
    std::vector<int32_t> next;
    for (size_t k = 0; k < states.size(); ++k)
    {
        next.resize((k + 1) * B, Dfa::kDead);
        for (uint32_t c = 0; c < B; ++c)
        {
            Win w = states[k];
            bool me = false;
            if (w.x > 0 && w.col.back() == c) me = true, w.ok.back() = 1;
            if (w.below)
            {
                if (w.col.front() == c) me = true, w.ok.front() = 1;
                if (!w.ok.front()) continue; // (the pixel above leaves unmatched)
                w.col.erase(w.col.begin()), w.ok.erase(w.ok.begin());
            }
            w.col.push_back(c), w.ok.push_back(me ? 1 : 0);
            if (++w.x == W) w.x = 0, w.below = true;
            const uint64_t kk = key(w);
            const auto [it, added] = id.emplace(kk, int32_t(states.size()));
            if (added)
            {
                states.push_back(std::move(w));
                if (!cells_fit(states.size(), B)) return std::nullopt;
            }
            next[k * B + c] = it->second;
        }
    }
    Dfa d;
    d.base = B;
    d.start = 0;
    d.accept.assign(states.size(), 0);
    for (size_t k = 0; k < states.size(); ++k)
        d.accept[k] = states[k].x == 0 && states[k].below && std::all_of(states[k].ok.begin(), states[k].ok.end(), [](uint8_t v) { return v != 0; });
    d.next = std::move(next);
    return minimise(d);
}

// A gradient one way: along every row (across) or down every column (down) the colours (in the palette's order)
// never fall, or never rise, the same way for the whole picture. States: the place in the row (or the row above,
// down), the last colour (or the colours above), and which way, once a change has said.
std::optional<Dfa> gradient_dfa(uint32_t B, uint32_t W, bool down)
{
    const uint32_t L = down ? W : 1; // colours remembered
    uint64_t combos = 1;
    for (uint32_t k = 0; k < L; ++k)
    {
        combos *= B;
        if (combos > kMaxDfaCells) return std::nullopt;
    }
    std::map<std::tuple<uint32_t, bool, int, std::vector<uint32_t>>, int32_t> id; // (column, first row done, way, remembered)
    std::vector<std::tuple<uint32_t, bool, int, std::vector<uint32_t>>> states{{0, false, 0, {}}};
    id[states[0]] = 0;
    std::vector<int32_t> next;
    for (size_t k = 0; k < states.size(); ++k)
    {
        next.resize((k + 1) * B, Dfa::kDead);
        for (uint32_t c = 0; c < B; ++c)
        {
            auto [x, below, way, mem] = states[k];
            // the colour this pixel follows: the one left of it (across, not at a row's start), or above it (down)
            std::optional<uint32_t> before;
            if (!down && x > 0) before = mem.back();
            if (down && below) before = mem.front();
            if (before && c != *before)
            {
                const int w = c > *before ? 1 : -1;
                if (way && way != w) continue;
                way = w;
            }
            if (down)
            {
                if (below) mem.erase(mem.begin());
                mem.push_back(c);
            }
            else mem = {c};
            if (++x == W) x = 0, below = true;
            const auto st = std::make_tuple(x, below, way, mem);
            const auto [it, added] = id.emplace(st, int32_t(states.size()));
            if (added)
            {
                states.push_back(st);
                if (!cells_fit(states.size(), B)) return std::nullopt;
            }
            next[k * B + c] = it->second;
        }
    }
    Dfa d;
    d.base = B;
    d.start = 0;
    d.accept.assign(states.size(), 1);
    d.next = std::move(next);
    return minimise(d);
}

} // namespace

// Each row differs from the row above in at most k pixels (rows-alike-v1, list N). Pixels are placed row by row; a
// state holds the last W pixels (the row above the next pixel, then the new row so far), the column, the changes in
// this row, and the row's place: on a picture whether the first row is done, on a video the row within its frame (a
// frame's first row has no row above in it, and starts again). Built where its states fit.
std::optional<Dfa> rows_alike_dfa(uint32_t B, uint32_t W, uint32_t rows_a_frame, uint32_t k)
{
    uint64_t combos = 1;
    for (uint32_t i = 0; i < W; ++i)
    {
        combos *= B;
        if (combos > kMaxDfaCells) return std::nullopt;
    }
    using Key = std::tuple<uint32_t, uint32_t, uint32_t, std::vector<uint32_t>>; // (column, row, changes, window)
    std::map<Key, int32_t> id;
    std::vector<Key> states{{0, 0, 0, {}}};
    id[states[0]] = 0;
    std::vector<int32_t> next;
    for (size_t at = 0; at < states.size(); ++at)
    {
        next.resize((at + 1) * B, Dfa::kDead);
        for (uint32_t c = 0; c < B; ++c)
        {
            auto [x, y, ch, win] = states[at];
            if (y > 0 && win.front() != c && ++ch > k) continue; // (win.front(): the pixel above)
            win.push_back(c);
            if (win.size() > W) win.erase(win.begin());
            if (++x == W)
            {
                x = 0, ch = 0;
                y = rows_a_frame ? (y + 1) % rows_a_frame : 1;
            }
            Key key{x, y, ch, std::move(win)};
            const auto [it, added] = id.emplace(key, int32_t(states.size()));
            if (added)
            {
                states.push_back(std::move(key));
                if (!cells_fit(states.size(), B)) return std::nullopt;
            }
            next[at * B + c] = it->second;
        }
    }
    Dfa d;
    d.base = B;
    d.start = 0;
    d.accept.assign(states.size(), 0);
    for (size_t i = 0; i < states.size(); ++i) d.accept[i] = std::get<0>(states[i]) == 0; // (whole rows; the length is the line's)
    d.next = std::move(next);
    return minimise(d);
}

// rows-alike-v1 past where its automaton fits (large palettes): judged by its rule, counted and ranked by a
// ChangesRanker (changes_ranker.hpp) whose parts are the rows, each picture's (each frame's) first row free.
class RowsAlike : public Filter
{
public:
    RowsAlike(const FilterLine& l, uint32_t k) : w_(l.width), h_(l.height), k_(k)
    {
        const size_t rows = l.length / w_;
        std::vector<char> free(rows, 0);
        for (size_t r = 0; r < rows; r += h_) free[r] = 1;
        if (ChangesRanker::table_bytes(l.base, w_, k_) <= filter_memory()) // (a row's table: small but for the widest pictures)
            ranker_ = std::make_unique<ChangesRanker>(l.base, w_, std::move(free), k_, false);
        else set_table_bytes(ChangesRanker::table_bytes(l.base, w_, k_));
        provenance_ = "changes=" + std::to_string(k);
    }
    bool passes(std::span<const uint32_t> unit) const override
    {
        for (size_t at = 0; at < unit.size(); at += size_t(w_) * h_)
            for (uint32_t y = 1; y < h_; ++y)
            {
                uint32_t ch = 0;
                for (uint32_t x = 0; x < w_; ++x)
                    if (unit[at + size_t(y) * w_ + x] != unit[at + size_t(y - 1) * w_ + x] && ++ch > k_) return false;
            }
        return true;
    }
    const Ranker* ranker() const override { return ranker_.get(); }

private:
    uint32_t w_, h_, k_;
    std::unique_ptr<Ranker> ranker_;
};

void add_picture_filters(std::vector<FilterSpec>& out)
{
    FilterSpec p;
    p.id = "palette-size";
    p.title = "palette-size";
    p.description = "At most so many distinct colours in a picture (or, with scope frame, in each frame of a video). Pixel art, "
                    "icons and diagrams use a handful; noise in a large palette uses nearly one a pixel. Exact on every palette, "
                    "rgb24 included: at most 16 colours keeps about 10^-500 of the 10x10 rgb24 line.";
    p.params = {{"colours", "the most distinct colours allowed", FilterParam::Kind::Integer, "16", 1, 4096, 1, {}},
                {"scope", "video: count the colours over the whole film, or in each frame on its own", FilterParam::Kind::Text,
                 "film", 0, 0, 1, {"film", "frame"}}};
    p.applies = picture;
    p.counts_as = "own";
    p.make = [own = p.name()](const FilterLine& l, const FilterValues& v, const FilterResources&) -> std::unique_ptr<Filter> {
        const FilterSpec& spec = *find_filter(own);
        const int64_t k = param_int(spec, v, "colours");
        const std::string scope = param_value(spec, v, "scope");
        const std::string prov = "colours=" + std::to_string(k) + (l.kind == "video" ? " scope=" + scope : "");
        const uint32_t sc = scope == "frame" && l.frames > 1 ? l.width * l.height : l.length;
        if (auto dfa = palette_dfa(l.base, l.length, sc, uint32_t(k))) return make_dfa_filter(std::move(*dfa), l.length, prov);
        return std::make_unique<PaletteSize>(l, uint32_t(k), scope == "frame", prov);
    };
    // An automaton where every setting's would be small enough on this line: all its colours, and on
    // a video counted per frame.
    p.counts_as_on = [](const FilterLine& l, const FilterValues*) -> std::string {
        const bool per_frame = l.frames > 1;
        const uint64_t scope = per_frame ? uint64_t(l.width) * l.height : l.length;
        return cells_fit(palette_states(l.base, std::min<uint64_t>(l.base, scope), scope, per_frame), l.base) ? "automaton" : "own";
    };
    out.push_back(p);

    FilterSpec r;
    r.id = "row-runs";
    r.title = "row-runs";
    r.description = "Along each row of pixels, at most so many places where a pixel differs from the one to its left: flat "
                    "areas and edges pass, static fails. Exact on every palette and at any width (it remembers one row, "
                    "not a whole one above), but blind to up and down: vertical stripes pass.";
    r.params = {{"changes", "the most colour changes allowed along one row", FilterParam::Kind::Integer, "3", 0, 4096, 1, {}}};
    r.applies = picture;
    r.counts_as = "own";
    r.make = [own = r.name()](const FilterLine& l, const FilterValues& v, const FilterResources&) -> std::unique_ptr<Filter> {
        const int64_t c = param_int(*find_filter(own), v, "changes");
        if (auto dfa = row_runs_dfa(l.base, l.width, uint32_t(c))) return make_dfa_filter(std::move(*dfa), l.length, "changes=" + std::to_string(c));
        return std::make_unique<RowRuns>(l, uint32_t(c), "changes=" + std::to_string(c));
    };
    // An automaton where every setting's would be small enough on this line (a row's worth of changes).
    r.counts_as_on = [](const FilterLine& l, const FilterValues*) -> std::string {
        return cells_fit(row_runs_states(l.base, l.width, l.width > 0 ? l.width - 1 : 0), l.base) ? "automaton" : "own";
    };
    out.push_back(r);

    // List Q (Edward, 18 November 2026: "go ahead with the picture filters next"; IDEAS 19.245's next round).
    FilterSpec bo;
    bo.id = "border";
    bo.title = "border";
    bo.description = "Pictures framed in one colour: every pixel of the top and bottom rows and the left and right columns the same "
                     "colour, as icons, diagrams and pictures on a plain ground are. Random black-and-white pictures of 10 x 10 almost all fail (it keeps 10^-10.54; on the 16-colour palette 10^-42.14); of ten real pictures fitted to that line, 4 pass. It picks a kind of picture, so ticking every filter at once leaves it. An automaton: counted and ranked "
                     "exactly where it fits (the place in the picture times the colours).";
    bo.applies = [](const FilterLine& l) { return still_picture(l) && cells_fit(uint64_t(l.width) * l.height * l.base + 1, l.base); }; // (its states known without building)
    bo.counts_as = "automaton";
    bo.picks_a_kind = true; // (4 of 10 real pictures at 10 x 10 pass: a kind, not noise)
    bo.make = [](const FilterLine& l, const FilterValues&, const FilterResources&) -> std::unique_ptr<Filter> {
        auto d = border_dfa(l.base, l.width, l.height);
        if (!d) throw std::invalid_argument("border-v1: past what its automaton holds on this picture");
        return make_dfa_filter(std::move(*d), l.length, "");
    };
    out.push_back(bo);

    FilterSpec sp;
    sp.id = "specks";
    sp.title = "specks";
    sp.description = "Pictures with no lone specks: every pixel shares its colour with one of its four neighbours at least, so "
                     "every patch of colour is two pixels or more. Random black-and-white pictures of 10 x 10: 10^-3.35 kept (random pixels often match a neighbour); of ten real pictures fitted to that line, 7 pass. An automaton of the row above and which of its "
                     "pixels have been matched: counted and ranked exactly where it fits (black and white to about 10 pixels wide; "
                     "a few colours narrower).";
    auto specks_of = [](const FilterLine& l) {
        return kept_dfa("specks/" + std::to_string(l.base) + "/" + std::to_string(l.width), [&] { return specks_dfa(l.base, l.width); });
    };
    sp.applies = [specks_of](const FilterLine& l) { return still_picture(l) && specks_of(l)->has_value(); };
    sp.counts_as = "automaton";
    sp.make = [specks_of](const FilterLine& l, const FilterValues&, const FilterResources&) -> std::unique_ptr<Filter> {
        const auto d = specks_of(l);
        if (!d->has_value()) throw std::invalid_argument("specks-v1: past what its automaton holds at this width and palette");
        return make_dfa_filter(**d, l.length, "");
    };
    out.push_back(sp);

    FilterSpec gr;
    gr.id = "gradient";
    gr.title = "gradient";
    gr.description = "Pictures that are a gradient one way: along every row (across) or down every column (down), the colours, "
                     "in the palette's order, never fall, or never rise, the same way for the whole picture. Random black-and-white pictures of 10 x 10: 10^-19.39 kept either way (16 colours: 10^-54.97 across); of ten real pictures, 2 pass. It picks a kind of picture, so ticking every filter at once leaves it. An "
                     "automaton: counted and ranked exactly where it fits (down remembers a row, so on small palettes and widths).";
    gr.params = {{"way", "across: along the rows; down: down the columns", FilterParam::Kind::Text, "across", 0, 0, 1, {"across", "down"}}};
    auto gradient_of = [](const FilterLine& l, bool down) {
        return kept_dfa("gradient/" + std::to_string(l.base) + "/" + std::to_string(l.width) + (down ? "/down" : "/across"),
                        [&] { return gradient_dfa(l.base, l.width, down); });
    };
    gr.applies = [gradient_of](const FilterLine& l) { return still_picture(l) && gradient_of(l, false)->has_value(); };
    gr.counts_as = "automaton";
    gr.picks_a_kind = true; // (2 of 10 real pictures pass)
    gr.make = [own = gr.name(), gradient_of](const FilterLine& l, const FilterValues& v, const FilterResources&) -> std::unique_ptr<Filter> {
        const bool down = param_value(*find_filter(own), v, "way") == "down";
        const auto d = gradient_of(l, down);
        if (!d->has_value()) throw std::invalid_argument("gradient-v1: down remembers a row, past what its automaton holds here");
        return make_dfa_filter(**d, l.length, std::string("way=") + (down ? "down" : "across"));
    };
    out.push_back(gr);

    FilterSpec ra;
    ra.id = "rows-alike";
    ra.title = "rows-alike";
    ra.description = "Pictures and video whose rows each differ from the row above in at most `changes` pixels: row-runs' other "
                     "half, as row-runs is blind up and down. Noise changes nearly every pixel between rows (all but one in P); a "
                     "drawing or a photograph changes few. On a video each frame's first row is free. Black-and-white pictures "
                     "of 10 x 10: at 5 (the default), 10^-1.85 kept, and 14 of 20 real pictures cut to that size pass (at 3, "
                     "10^-6.88 and 3 of 20: so small a picture changes much between rows). An automaton of the row "
                     "above where it fits (black and white to about 20 pixels wide, a few colours narrower), counted, ranked and "
                     "merging; past that, on any palette, counted and ranked by how many pixels each row has changed so far.";
    ra.params = {{"changes", "the most pixels a row may differ from the row above", FilterParam::Kind::Integer, "5", 0, 64, 1, {}}};
    auto rows_of = [](const FilterLine& l, uint32_t k) {
        const uint32_t per = l.kind == "video" ? l.height : 0;
        return kept_dfa("rows-alike/" + std::to_string(l.base) + "/" + std::to_string(l.width) + "/" + std::to_string(per) + "/" + std::to_string(k),
                        [&] { return rows_alike_dfa(l.base, l.width, per, k); });
    };
    auto moving_picture = [](const FilterLine& l) {
        return (l.kind == "image" || l.kind == "video") && l.width && l.height && l.base >= 2 && l.length % (uint64_t(l.width) * l.height) == 0;
    };
    ra.applies = [moving_picture](const FilterLine& l) { return moving_picture(l) && l.height >= 2; };
    ra.counts_as = "automaton";
    ra.counts_as_on = [own = ra.name(), rows_of](const FilterLine& l, const FilterValues* v) -> std::string {
        const int64_t k = v ? param_int(*find_filter(own), *v, "changes") : int64_t(l.width);
        return rows_of(l, uint32_t(std::min<int64_t>(k, l.width)))->has_value() ? "automaton" : "own";
    };
    ra.make = [own = ra.name(), rows_of](const FilterLine& l, const FilterValues& v, const FilterResources&) -> std::unique_ptr<Filter> {
        const int64_t k = param_int(*find_filter(own), v, "changes");
        const auto d = rows_of(l, uint32_t(std::min<int64_t>(k, l.width)));
        if (!d->has_value()) return std::make_unique<RowsAlike>(l, uint32_t(std::min<int64_t>(k, l.width))); // (counted and ranked by its changes)
        return make_dfa_filter(**d, l.length, "changes=" + std::to_string(k));
    };
    out.push_back(ra);
}

} // namespace sieve
