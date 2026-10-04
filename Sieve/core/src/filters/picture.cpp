// Sieve — filters for pictures that count on every palette, rgb24 included (image and video):
//   palette-size-v1  at most `colours` distinct colours in the unit, or, with `scope = frame` on a
//                    video, in each frame on its own. Pixel art, icons and diagrams use a handful of
//                    colours; noise in a large palette uses nearly as many as it has pixels.
//   row-runs-v1      along each row of pixels, at most `changes` places where a pixel differs from
//                    the one to its left. Flat areas and edges pass; static changes almost every
//                    pixel. The rows of a video are its frames' rows, one after another.
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

#include <algorithm>
#include <map>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <tuple>

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

} // namespace

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
    p.make = [](const FilterLine& l, const FilterValues& v, const FilterResources&) -> std::unique_ptr<Filter> {
        const FilterSpec& spec = *find_filter("palette-size-v1");
        const int64_t k = param_int(spec, v, "colours");
        const std::string scope = param_value(spec, v, "scope");
        const std::string prov = "colours=" + std::to_string(k) + (l.kind == "video" ? " scope=" + scope : "");
        const uint32_t sc = scope == "frame" && l.frames > 1 ? l.width * l.height : l.length;
        if (auto dfa = palette_dfa(l.base, l.length, sc, uint32_t(k))) return make_dfa_filter(std::move(*dfa), l.length, prov);
        return std::make_unique<PaletteSize>(l, uint32_t(k), scope == "frame", prov);
    };
    // An automaton where every setting's would be small enough on this line: all its colours, and on
    // a video counted per frame.
    p.counts_as_on = [](const FilterLine& l) -> std::string {
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
    r.make = [](const FilterLine& l, const FilterValues& v, const FilterResources&) -> std::unique_ptr<Filter> {
        const int64_t c = param_int(*find_filter("row-runs-v1"), v, "changes");
        if (auto dfa = row_runs_dfa(l.base, l.width, uint32_t(c))) return make_dfa_filter(std::move(*dfa), l.length, "changes=" + std::to_string(c));
        return std::make_unique<RowRuns>(l, uint32_t(c), "changes=" + std::to_string(c));
    };
    // An automaton where every setting's would be small enough on this line (a row's worth of changes).
    r.counts_as_on = [](const FilterLine& l) -> std::string {
        return cells_fit(row_runs_states(l.base, l.width, l.width > 0 ? l.width - 1 : 0), l.base) ? "automaton" : "own";
    };
    out.push_back(r);
}

} // namespace sieve
