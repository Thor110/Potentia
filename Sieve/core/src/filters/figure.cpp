// Sieve — figure-v1: pictures whose ink forms a set number of connected shapes (image line).
//
// Ink is every pixel that is not the background colour (the palette's nearest to white, or to
// black). The rule keeps a picture whose ink makes exactly `shapes` connected shapes, joined
// side to side (four) or corners too (eight), and, as a setting, where the shapes may stand:
//   any       anywhere, touching the border or not;
//   clear     clear of all four edges: no ink on the border;
//   standing  clear of the top and the sides, touching the bottom row (a figure on the ground).
// A figure, a creature, a letter or an icon is one or a few shapes; noise is many.
//
// It counts exactly, as an automaton read pixel by pixel along the rows (a frontier, as
// polyominoes are counted). The state is:
//   - the column;
//   - the shape each of the last W pixels belongs to (0 for background), numbered in the order
//     they first appear, so two frontiers that join the same way are one state;
//   - with eight-way joins, the pixel up and to the left as it was before this row overwrote it;
//   - how many shapes are finished (no longer touching the frontier), at most `shapes`;
//   - for the border: whether this is the first row, and whether this row and the last one held
//     ink. The last row is never known while reading, so the bottom edge is judged at the end:
//     the walk accepts at a row's end only if the row just read was clear (clear) or not
//     (standing).
// Nothing counts the rows, so one automaton serves every height. Its size grows with the width:
// at 10 wide it is 3,012 states (eight-way, clear) to 60,494 (four-way, anywhere), minimised.
//
// On a small palette the two-symbol automaton (background, ink) is widened to every colour and
// combines with every other automaton. On a large one (rgb24) it counts with its own ranker: each
// ink step weighs B - 1, worked out arithmetically, so 2^24 colours cost what two do.

#include "sieve/dfa.hpp"
#include "sieve/filter.hpp"
#include "sieve/image.hpp"
#include "sieve/packed.hpp"
#include "sieve/plugin.hpp"

#include <algorithm>
#include <map>
#include <tuple>
#include <cmath>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace sieve {

namespace {

enum class Border { Any, Clear, Standing };

struct FigureRule
{
    uint32_t width = 0;
    bool eight = false;
    uint32_t shapes = 1;
    Border border = Border::Any;
};

Border border_of(const std::string& s)
{
    if (s == "any") return Border::Any;
    if (s == "clear") return Border::Clear;
    if (s == "standing") return Border::Standing;
    throw std::invalid_argument("figure-v1: border must be any, clear or standing");
}

// Where an automaton is built: states times symbols under this (max-run-v1's bound, as the other
// picture filters use).
constexpr uint64_t kMaxDfaCells = uint64_t(1) << 24;

// ---------------------------------------------------------------- the frontier

// A state, as bytes: col, ul, done, first, row_ink, last_ink, then the W labels.
constexpr size_t kHead = 6;

// The labels renumbered by first appearance along the frontier, then the up-left pixel.
void canonical(std::string& s, uint32_t W)
{
    uint8_t map[256] = {};
    uint8_t n = 0;
    auto f = [&](char& c) {
        const uint8_t x = uint8_t(c);
        if (!x) return;
        if (!map[x]) map[x] = ++n;
        c = char(map[x]);
    };
    for (uint32_t i = 0; i < W; ++i) f(s[kHead + i]);
    f(s[1]);
}

// The labels still on the frontier (the up-left pixel counts while the row still needs it).
uint32_t live_count(const std::string& s, uint32_t W)
{
    bool seen[256] = {};
    uint32_t n = 0;
    auto add = [&](uint8_t x) {
        if (x && !seen[x]) seen[x] = true, ++n;
    };
    for (uint32_t i = 0; i < W; ++i) add(uint8_t(s[kHead + i]));
    add(uint8_t(s[1]));
    return n;
}

// One pixel: the next state, or nothing when the picture can no longer pass.
std::optional<std::string> step(const FigureRule& r, const std::string& st, bool ink)
{
    const uint32_t W = r.width;
    const uint32_t c = uint8_t(st[0]);
    const bool first = st[3] != 0;
    if (ink && r.border != Border::Any && (first || c == 0 || c + 1 == W)) return std::nullopt;
    std::string s = st;
    char* prof = s.data() + kHead;
    bool live0[256] = {};
    for (uint32_t i = 0; i < W; ++i) live0[uint8_t(prof[i])] = true;
    live0[uint8_t(s[1])] = true;
    live0[0] = false;
    const uint8_t old = uint8_t(prof[c]);
    // The neighbours already placed: left, up, and with eight-way joins up-left and up-right.
    uint8_t nb[4];
    int k = 0;
    if (c > 0) nb[k++] = uint8_t(prof[c - 1]);
    nb[k++] = old;
    if (r.eight)
    {
        if (c > 0) nb[k++] = uint8_t(s[1]);
        if (c + 1 < W) nb[k++] = uint8_t(prof[c + 1]);
    }
    uint8_t merge[256];
    for (int i = 0; i < 256; ++i) merge[i] = uint8_t(i);
    uint8_t label = 0;
    if (ink)
    {
        uint8_t low = 0;
        for (int i = 0; i < k; ++i)
            if (nb[i] && (!low || nb[i] < low)) low = nb[i];
        if (low)
        {
            for (int i = 0; i < k; ++i)
                if (nb[i]) merge[nb[i]] = low;
            label = low;
        }
        else
        {
            uint8_t most = 0;
            for (int i = 1; i < 256; ++i)
                if (live0[i]) most = uint8_t(i);
            label = uint8_t(most + 1);
        }
    }
    for (uint32_t i = 0; i < W; ++i) prof[i] = char(merge[uint8_t(prof[i])]);
    s[1] = char(r.eight && c + 1 < W ? merge[old] : 0);
    prof[c] = char(label);
    // Shapes that left the frontier are finished.
    bool live1[256] = {}, gone[256] = {};
    for (uint32_t i = 0; i < W; ++i) live1[uint8_t(prof[i])] = true;
    live1[uint8_t(s[1])] = true;
    uint32_t done = uint8_t(s[2]);
    for (int i = 1; i < 256; ++i)
        if (live0[i])
        {
            const uint8_t m = merge[i];
            if (!live1[m] && !gone[m]) gone[m] = true, ++done;
        }
    if (done > r.shapes) return std::nullopt;
    s[2] = char(done);
    if (ink) s[4] = 1;
    if (c + 1 == W)
    {
        s[0] = 0;
        s[3] = 0;
        s[5] = r.border == Border::Any ? 0 : s[4];
        s[4] = 0;
    }
    else s[0] = char(c + 1);
    canonical(s, W);
    return s;
}

bool accepting(const FigureRule& r, const std::string& s)
{
    if (s[0] != 0 || s[3] != 0) return false; // only at a row's end, after the first row
    if (uint8_t(s[2]) + live_count(s, r.width) != r.shapes) return false;
    if (r.border == Border::Clear) return s[5] == 0;
    if (r.border == Border::Standing) return s[5] != 0;
    return true;
}

// The rule as an automaton over two symbols (0 background, 1 ink), minimised; nothing when it
// would have more than `most` states before minimising (too wide a picture to count this way).
std::optional<Dfa> figure_dfa2(const FigureRule& r, size_t most)
{
    if (r.width == 0 || r.width > 250) return std::nullopt;
    std::string start(kHead + r.width, '\0');
    start[3] = r.border == Border::Any ? 0 : 1;
    std::unordered_map<std::string, int32_t> id;
    std::vector<std::string> keys;
    auto get = [&](const std::string& k) {
        const auto [it, added] = id.emplace(k, int32_t(keys.size()));
        if (added) keys.push_back(k);
        return it->second;
    };
    Dfa d;
    d.base = 2;
    d.start = get(start);
    for (size_t i = 0; i < keys.size(); ++i)
    {
        if (keys.size() > most) return std::nullopt;
        const std::string s = keys[i];
        d.accept.push_back(accepting(r, s) ? 1 : 0);
        for (bool ink : {false, true})
        {
            const auto t = step(r, s, ink);
            d.next.push_back(t ? get(*t) : Dfa::kDead);
        }
    }
    return minimise(d);
}

// figure_dfa2, kept for the process: the menus ask how a filter counts far more often than a rule
// changes, and building one takes up to a second. Null when it is too large.
std::shared_ptr<const Dfa> figure_dfa2_cached(const FigureRule& r)
{
    static std::mutex mx;
    static std::map<std::tuple<uint32_t, bool, uint32_t, int>, std::shared_ptr<const Dfa>> cache;
    const auto key = std::make_tuple(r.width, r.eight, r.shapes, int(r.border));
    {
        std::lock_guard<std::mutex> lock(mx);
        if (const auto it = cache.find(key); it != cache.end()) return it->second;
    }
    std::optional<Dfa> d = figure_dfa2(r, kMaxDfaCells / 2);
    std::shared_ptr<const Dfa> p = d ? std::make_shared<const Dfa>(std::move(*d)) : nullptr;
    std::lock_guard<std::mutex> lock(mx);
    return cache.emplace(key, p).first->second;
}

// The two-symbol automaton read over every colour: the background is 0, every other colour 1.
Dfa widen(const Dfa& two, uint32_t base, uint32_t background)
{
    Dfa d;
    d.base = base;
    d.start = two.start;
    d.accept = two.accept;
    d.next.resize(two.states() * base);
    for (size_t s = 0; s < two.states(); ++s)
        for (uint32_t c = 0; c < base; ++c) d.next[s * base + c] = two.next[s * 2 + (c == background ? 0 : 1)];
    return minimise(d);
}

// ---------------------------------------------------------------- counting on a large palette

// Counts the two-symbol automaton with the background weighing 1 and ink B - 1, and ranks over
// the colours arithmetically: below a colour d stand the ink colours below it, each worth the ink
// step's completions, and the background once if it is below d.
class InkRanker : public Ranker
{
public:
    InkRanker(std::shared_ptr<const Dfa> two, uint32_t base, uint32_t background, uint32_t length)
        : d_(std::move(two)), b_(base), bg_(background), l_(length)
    {
        const size_t n = d_->states();
        rows_.reserve(size_t(l_) + 1);
        rows_.add_row(n, [&](size_t s, BigUint& v) {
            if (d_->accept[s]) v.add_small(1);
        });
        for (uint32_t r = 1; r <= l_; ++r)
            rows_.add_row(n, [&](size_t s, BigUint& v) {
                const int32_t t0 = d_->next[s * 2], t1 = d_->next[s * 2 + 1];
                if (t0 >= 0 && !rows_.is_zero(r - 1, size_t(t0))) v.add_mul_small(rows_.limbs(r - 1, size_t(t0)), 1);
                if (t1 >= 0 && !rows_.is_zero(r - 1, size_t(t1))) v.add_mul_small(rows_.limbs(r - 1, size_t(t1)), b_ - 1);
            });
        rows_.finish();
        set_count();
    }
    static double table_bytes(size_t states, uint32_t base, uint32_t length) { return DfaRanker::table_bytes(states, base, length); }
    uint32_t length() const override { return l_; }
    uint32_t base() const override { return b_; }
    State start() const override { return d_->start < 0 ? kDead : State(d_->start); }
    State next(State s, uint32_t symbol) const override
    {
        if (s == kDead || symbol >= b_) return kDead;
        const int32_t t = d_->next[size_t(s) * 2 + (symbol == bg_ ? 0 : 1)];
        return t < 0 ? kDead : State(t);
    }
    BigUint completions(State s, uint32_t remaining) const override
    {
        if (s == kDead || remaining > l_) return {};
        return rows_.value(remaining, size_t(s));
    }
    BigUint rank(std::span<const uint32_t> unit) const override
    {
        if (unit.size() != l_) throw std::invalid_argument("unit has the wrong length");
        BigUint r;
        State s = start();
        for (uint32_t pos = 0; pos < l_; ++pos)
        {
            const uint32_t d = unit[pos], rem = l_ - pos - 1;
            const bool bg_below = bg_ < d;
            BigUint ink = completions(next(s, bg_ == 0 ? 1 : 0), rem);
            ink.mul_small(d - (bg_below ? 1 : 0));
            r += ink;
            if (bg_below) r += completions(next(s, bg_), rem);
            s = next(s, d);
            if (s == kDead) throw std::invalid_argument("the unit is not a survivor");
        }
        if (completions(s, 0).is_zero()) throw std::invalid_argument("the unit is not a survivor");
        return r;
    }
    std::vector<uint32_t> unrank(const BigUint& k) const override
    {
        if (!(k < count())) throw std::out_of_range("survivor number beyond the survivors");
        BigUint rest = k;
        std::vector<uint32_t> out;
        State s = start();
        const uint32_t an_ink = bg_ == 0 ? 1 : 0;
        for (uint32_t pos = 0; pos < l_; ++pos)
        {
            const uint32_t rem = l_ - pos - 1;
            const BigUint ink = completions(next(s, an_ink), rem), back = completions(next(s, bg_), rem);
            // The ink colours below the background, then the background, then the rest.
            auto take_ink = [&](uint32_t offset) {
                BigUint q, rr;
                BigUint::divmod(rest, ink, q, rr);
                rest = rr;
                return offset + uint32_t(std::stoul(q.to_decimal()));
            };
            BigUint below_bg = ink;
            below_bg.mul_small(bg_);
            uint32_t d;
            if (!ink.is_zero() && rest < below_bg) d = take_ink(0);
            else
            {
                if (!ink.is_zero()) rest -= below_bg;
                if (rest < back) d = bg_;
                else
                {
                    rest -= back;
                    d = take_ink(bg_ + 1);
                }
            }
            out.push_back(d);
            s = next(s, d);
        }
        return out;
    }

private:
    std::shared_ptr<const Dfa> d_;
    uint32_t b_, bg_, l_;
    PackedRows rows_;
};

// The rule judged directly (shapes found by a flood fill), for a palette too large for the widened
// automaton: the same rule, written another way.
bool figure_passes(const FigureRule& r, uint32_t height, uint32_t background, std::span<const uint32_t> unit)
{
    const uint32_t W = r.width, H = height;
    std::vector<uint8_t> seen(unit.size());
    uint32_t shapes = 0;
    bool bottom = false;
    std::vector<uint32_t> todo;
    for (uint32_t i = 0; i < unit.size(); ++i)
    {
        if (unit[i] == background) continue;
        const uint32_t x = i % W, y = i / W;
        if (r.border != Border::Any && (y == 0 || x == 0 || x + 1 == W)) return false;
        if (y + 1 == H)
        {
            if (r.border == Border::Clear) return false;
            bottom = true;
        }
        if (seen[i]) continue;
        if (++shapes > r.shapes) return false;
        seen[i] = 1;
        todo.assign(1, i);
        while (!todo.empty())
        {
            const uint32_t p = todo.back();
            todo.pop_back();
            const int px = int(p % W), py = int(p / W);
            for (int dy = -1; dy <= 1; ++dy)
                for (int dx = -1; dx <= 1; ++dx)
                {
                    if ((dx == 0 && dy == 0) || (!r.eight && dx != 0 && dy != 0)) continue;
                    const int nx = px + dx, ny = py + dy;
                    if (nx < 0 || ny < 0 || nx >= int(W) || ny >= int(H)) continue;
                    const uint32_t q = uint32_t(ny) * W + uint32_t(nx);
                    if (!seen[q] && unit[q] != background) seen[q] = 1, todo.push_back(q);
                }
        }
    }
    if (r.border == Border::Standing && !bottom) return false;
    return shapes == r.shapes;
}

class Figure : public Filter
{
public:
    Figure(FigureRule r, uint32_t height, uint32_t base, uint32_t background, std::shared_ptr<const Dfa> two, std::string prov)
        : r_(r), h_(height), b_(base), bg_(background), two_(std::move(two))
    {
        provenance_ = std::move(prov);
        if (two_) set_table_bytes(InkRanker::table_bytes(two_->states(), b_, r_.width * h_));
    }
    bool passes(std::span<const uint32_t> unit) const override { return figure_passes(r_, h_, bg_, unit); }
    bool can_rank() const override { return two_ && table_bytes() <= filter_memory(); }
    const Ranker* ranker() const override
    {
        if (!can_rank()) return nullptr;
        std::call_once(once_, [&] { ranker_ = std::make_unique<InkRanker>(two_, b_, bg_, r_.width * h_); });
        return ranker_.get();
    }

private:
    FigureRule r_;
    uint32_t h_, b_, bg_;
    std::shared_ptr<const Dfa> two_;
    mutable std::once_flag once_;
    mutable std::unique_ptr<InkRanker> ranker_;
};

bool figure_line(const FilterLine& l)
{
    return l.kind == "image" && l.width >= 1 && l.height >= 1 && l.base >= 2 && l.length == l.width * l.height;
}

// The palette's colour nearest white (background = lightest) or black (darkest), from the line's
// symbols ("image/mono/10x10").
uint32_t background_of(const FilterLine& l, const std::string& which)
{
    const size_t a = l.symbols_id.find('/'), b = a == std::string::npos ? a : l.symbols_id.find('/', a + 1);
    if (b == std::string::npos) throw std::invalid_argument("figure-v1: the line names no palette");
    const Palette& p = palette_by_id(l.symbols_id.substr(a + 1, b - a - 1));
    const uint8_t v = which == "darkest" ? 0 : 255;
    return p.nearest(Rgb{v, v, v});
}

} // namespace

void add_figure_filters(std::vector<FilterSpec>& out)
{
    FilterSpec f;
    f.id = "figure";
    f.title = "figure";
    f.description = "The ink (every colour but the background) makes exactly so many connected shapes: a figure, a "
                    "creature, a letter or an icon is one or a few, noise is many. The border setting keeps them clear of "
                    "the edges, or standing on the bottom row. Counts exactly, at any height; its automaton grows with the "
                    "width (10 wide: 3,012 to 60,494 states).";
    f.params = {{"shapes", "how many connected shapes the ink makes", FilterParam::Kind::Integer, "1", 1, 16, 1, {}},
                {"connect", "four: shapes join side to side; eight: corners join too", FilterParam::Kind::Text, "eight", 0, 0, 1,
                 {"four", "eight"}},
                {"border", "any: anywhere; clear: off every edge; standing: off the top and sides, on the bottom row",
                 FilterParam::Kind::Text, "clear", 0, 0, 1, {"any", "clear", "standing"}},
                {"background", "the background colour: the palette's nearest to white, or to black", FilterParam::Kind::Text,
                 "lightest", 0, 0, 1, {"lightest", "darkest"}}};
    f.applies = figure_line;
    f.counts_as = "automaton";
    auto rule_of = [](const FilterLine& l, const FilterValues& v, std::string& prov) {
        const FilterSpec& spec = *find_filter("figure-v1");
        FigureRule r;
        r.width = l.width;
        r.shapes = uint32_t(param_int(spec, v, "shapes"));
        r.eight = param_value(spec, v, "connect") == "eight";
        r.border = border_of(param_value(spec, v, "border"));
        prov = "shapes=" + std::to_string(r.shapes) + " connect=" + param_value(spec, v, "connect") +
               " border=" + param_value(spec, v, "border") + " background=" + param_value(spec, v, "background");
        return r;
    };
    f.make = [rule_of](const FilterLine& l, const FilterValues& v, const FilterResources&) -> std::unique_ptr<Filter> {
        std::string prov;
        const FigureRule r = rule_of(l, v, prov);
        const uint32_t bg = background_of(l, param_value(*find_filter("figure-v1"), v, "background"));
        std::shared_ptr<const Dfa> two = figure_dfa2_cached(r);
        if (two && two->states() <= kMaxDfaCells / l.base) return make_dfa_filter(widen(*two, l.base, bg), l.length, prov);
        return std::make_unique<Figure>(r, l.height, l.base, bg, std::move(two), prov);
    };
    // An automaton where the widened one fits; on a large palette its own ranker; past the width
    // where the frontier fits, it judges only.
    f.counts_as_on = [rule_of](const FilterLine& l, const FilterValues* v) -> std::string {
        std::string prov;
        FilterValues widest{{"connect", "four"}, {"border", "any"}, {"shapes", "16"}};
        const FigureRule r = rule_of(l, v ? *v : widest, prov);
        const std::shared_ptr<const Dfa> two = figure_dfa2_cached(r);
        if (!two) return "";
        return two->states() <= kMaxDfaCells / l.base ? "automaton" : "own";
    };
    out.push_back(f);
}

} // namespace sieve
