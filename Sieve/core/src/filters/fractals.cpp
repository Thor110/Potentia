// The fractal dimension's filters (list X, X1; IDEAS 19.235): rules that set aside the noise of the
// whole space (fractal-space-v2), every ruleset the reader takes. Most of it draws nothing, or never
// changes, or is another ruleset under other names (Edward, 16 November 2026: "it looks like there are a
// lot of fractals that aren't going to be worth looking at and figuring out a way to filter some of the
// noise there would also be useful in the long run").
//
// A unit is a ruleset's text (sieve-fractal-v1 or v2), a byte a symbol; a text the reader refuses fails
// every one of them. Each judges a ruleset by the symbols its axiom reaches, rule after rule, at any
// depth (its "reach"), without growing it, but fractal-dimension (list X2), which draws it and measures. The four
// structural ones (draws, grows, no-idle, canonical) are counted on v1 to v3 by fractal_count.cpp (list F), and
// their survivors numbered there too, so the line has a compact mode (F-B: sieve::FractalCompact); the others judge.
#include "sieve/alphabet.hpp"
#include "sieve/audio.hpp"
#include "sieve/dfa.hpp"
#include "sieve/filter.hpp"
#include "sieve/fractal.hpp"
#include "sieve/image.hpp"
#include "sieve/plugin.hpp" // make_dfa_filter
#include "sieve/sha256.hpp"

#include <algorithm>
#include <map>
#include <memory>
#include <optional>
#include <array>
#include <cmath>
#include <functional>
#include <utility>
#include <set>
#include <stdexcept>
#include <span>
#include <string>
#include <vector>

namespace sieve {

namespace {

bool is_fractals(const FilterLine& l) { return l.kind == "fractals"; }

std::optional<Fractal> fractal_of(std::span<const uint32_t> unit)
{
    std::string t;
    t.reserve(unit.size());
    for (const uint32_t s : unit)
    {
        if (s > 127) return std::nullopt;
        t += char(s);
    }
    return read_fractal(t);
}

// The symbols met at any depth: the axiom's, then each met symbol's rule word's, in the order met
// (breadth first: the axiom left to right, then each symbol's word in the order the symbols were met).
std::vector<char> reach(const Fractal& f)
{
    std::map<char, std::vector<const std::string*>> rule; // (v3: several for one symbol, each reached)
    for (const auto& [c, w] : f.rules) rule[c].push_back(&w);
    std::vector<char> met;
    std::set<char> seen;
    auto meet = [&](const std::string& w) {
        for (const char c : w)
            if (seen.insert(c).second) met.push_back(c);
    };
    meet(f.axiom);
    for (size_t k = 0; k < met.size(); ++k)
        if (const auto it = rule.find(met[k]); it != rule.end())
            for (const std::string* w : it->second) meet(*w);
    return met;
}

// A symbol whose name changes nothing but a colour, or nothing at all: every substitution symbol (its
// colour), and an L-system's letters but F, G and f (they draw nothing).
bool renamable(const Fractal& f, char c)
{
    if (f.family == Fractal::Family::Substitution) return true;
    return ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) && c != 'F' && c != 'G' && c != 'f';
}

class Judge : public Filter
{
public:
    using Test = bool (*)(const Fractal&);
    explicit Judge(Test t) : test_(t) {}
    bool passes(std::span<const uint32_t> unit) const override
    {
        const auto f = fractal_of(unit);
        return f && test_(*f);
    }

private:
    Test test_;
};

// Passes an L-system measured at least `least` hundredths (fractal_box_dimension), and every substitution.
class AtLeast : public Filter
{
public:
    explicit AtLeast(int least) : least_(double(least) / 100.0) {}
    bool passes(std::span<const uint32_t> unit) const override
    {
        const auto f = fractal_of(unit);
        if (!f) return false;
        if (f->family == Fractal::Family::Substitution || f->is_formula() || f->family == Fractal::Family::Life || f->family == Fractal::Family::Flow) return true;
        const auto d = fractal_box_dimension(*f);
        return d && d->value >= least_;
    }

private:
    double least_;
};

// A v4 formula mentions a token.
bool mentions(const Fractal& f, const char* t)
{
    for (const std::string& k : f.formula)
        if (k == t) return true;
    return false;
}
// What changes from one point of an escape or converge picture to the next: z (with julia, where z starts
// at the point) or c (without it, where c is the point).
bool varies(const Fractal& f) { return f.julia ? mentions(f, "z") : mentions(f, "c"); }

// An automaton over its first generations (kAutomatonLooked): whether a cell is ever live, and whether
// any generation differs from the one before it (an elementary one's newest row against the last, a
// life-like one's field against the last's, a cell in from each side, and the field beyond).
constexpr uint32_t kAutomatonLooked = 8;
bool automaton_lives(const Fractal& f)
{
    const auto c = fractal_cells(f, std::min(kAutomatonLooked, fractal_deepest_within(f, kFractalMost)));
    if (!c) return false;
    for (const uint8_t v : c->cells)
        if (v) return true;
    return c->beyond != 0;
}
bool automaton_changes(const Fractal& f)
{
    const uint32_t most = std::min(kAutomatonLooked, fractal_deepest_within(f, kFractalMost));
    for (uint32_t g = 1; g <= most; ++g)
    {
        const auto a = fractal_cells(f, g - 1), b = fractal_cells(f, g);
        if (!a || !b) return false;
        if (a->beyond != b->beyond) return true;
        if (f.family == Fractal::Family::Elementary)
        {
            const size_t ra = size_t(a->height - 1) * a->width, rb = size_t(b->height - 1) * b->width;
            for (uint32_t x = 0; x < b->width; ++x)
            {
                const uint8_t before = x >= 1 && x - 1 < a->width ? a->cells[ra + x - 1] : a->beyond;
                if (b->cells[rb + x] != before) return true;
            }
            continue;
        }
        for (uint32_t y = 0; y < b->height; ++y)
            for (uint32_t x = 0; x < b->width; ++x)
            {
                const bool in = x >= 1 && y >= 1 && x - 1 < a->width && y - 1 < a->height;
                const uint8_t before = in ? a->cells[size_t(y - 1) * a->width + x - 1] : a->beyond;
                if (b->cells[size_t(y) * b->width + x] != before) return true;
            }
    }
    return false;
}
// A start written one way: an elementary one's "1" not written (it is the default); a life-like one's with
// no empty row at its top or bottom and no column of 0 at its left (one at the right is the same field).
bool start_trimmed(const Fractal& f)
{
    if (f.family == Fractal::Family::Elementary) return f.start != "1";
    const auto empty = [](const std::string& r) { return r.find('1') == std::string::npos; };
    if (empty(f.start_rows.front()) || empty(f.start_rows.back())) return false;
    for (const std::string& r : f.start_rows)
        if (r[0] == '1') return true;
    return false;
}

// A path's (an attractor's or a flow's) distinct points among its first depth's last 256, rounded to a
// millionth: one that settles on a point or a short cycle has few.
size_t path_distinct(const Fractal& f)
{
    const auto p = fractal_path(f, 0);
    if (!p) return 0;
    std::set<std::array<long long, 3>> seen;
    for (size_t k = p->size() > 256 ? p->size() - 256 : 0; k < p->size(); ++k)
        seen.insert({std::llround((*p)[k][0] * 1e6), std::llround((*p)[k][1] * 1e6), std::llround((*p)[k][2] * 1e6)});
    return seen.size();
}

bool draws(const Fractal& f)
{
    if (f.family == Fractal::Family::Aggregate) return true;
    if (f.is_path()) return path_distinct(f) >= 2;
    if (f.family == Fractal::Family::Ifs) return true;
    if (f.is_automaton()) return automaton_lives(f);
    if (!f.rewrites()) return varies(f);
    const std::vector<char> m = reach(f);
    if (f.family == Fractal::Family::Substitution) return m.size() >= 2;
    for (const char c : m)
        if (c == 'F' || c == 'G') return true;
    return false;
}

bool grows(const Fractal& f)
{
    if (f.family == Fractal::Family::Ifs) return f.maps.size() >= 2;
    if (f.is_automaton()) return automaton_changes(f);
    if (f.family == Fractal::Family::Aggregate) return true;
    if (f.is_path()) return path_distinct(f) >= 16;
    if (!f.rewrites()) return mentions(f, "z");
    const std::vector<char> m = reach(f);
    const std::set<char> met(m.begin(), m.end());
    for (const auto& [c, w] : f.rules)
        if (met.count(c) && w.size() >= 2) return true;
    return false;
}

// An IFS's maps in the order of their texts, each once (a reordering of them draws the same).
bool maps_in_order(const Fractal& f)
{
    std::string last;
    for (size_t k = 0; k < f.maps.size(); ++k)
    {
        std::string t;
        for (const std::string& n : f.maps[k].text) t += n + " ";
        if (k && !(last < t)) return false;
        last = t;
    }
    return true;
}

bool no_idle(const Fractal& f)
{
    if (f.family == Fractal::Family::Ifs)
    {
        // (a map given twice adds no point)
        for (size_t a = 0; a < f.maps.size(); ++a)
            for (size_t b = a + 1; b < f.maps.size(); ++b)
                if (f.maps[a].text == f.maps[b].text) return false;
        return true;
    }
    if (!f.rewrites()) return true;
    const std::vector<char> m = reach(f);
    const std::set<char> met(m.begin(), m.end());
    for (const auto& [c, w] : f.rules)
        if (!met.count(c) || w == std::string(1, c)) return false;
    return true;
}

bool canonical(const Fractal& f)
{
    // Renamable symbols take their names in the order they are met, the first of the names free to
    // them first; and every rule is for a symbol met (a rule for one never met could be named anything).
    if (f.family == Fractal::Family::Ifs) return maps_in_order(f);
    if (f.is_automaton()) return start_trimmed(f);
    if (f.is_path())
    {
        // (its start not written where it is the one it has without it: 0 0, or a flow's 1 1 1)
        const bool flow = f.family == Fractal::Family::Flow;
        return !f.from || !(f.from_text[0] == (flow ? "1" : "0") && f.from_text[1] == (flow ? "1" : "0") && (!flow || f.from_text[2] == "1"));
    }
    if (f.family == Fractal::Family::Aggregate) return true;
    if (!f.rewrites()) return true;
    std::string names;
    if (f.family == Fractal::Family::Substitution) names = "0123456789abcdefghijklmnopqrstuvwxyz";
    else
        for (const char c : std::string("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"))
            if (renamable(f, c)) names += c;
    const std::vector<char> m = reach(f);
    size_t next = 0;
    for (const char c : m)
        if (renamable(f, c) && (next >= names.size() || names[next++] != c)) return false;
    const std::set<char> met(m.begin(), m.end());
    for (const auto& [c, w] : f.rules)
        if (!met.count(c)) return false;
    return true;
}

// ---------------------------------------------------------------- fractal-strange, fractal-lasts (list Z3)
//
// The noise of the families X5 and Y2 brought (Edward, 17 November 2026: "the filters dictate what we get
// and how we find it so the more the better"): most attractors and flows of the space settle on a point or
// a loop, or fly off; most automata die out, freeze or repeat.

// A path's points at a depth long enough for two starts a billionth apart to part on a strange attractor:
// a map's fourth (8,192 points), a flow's sixth (32,768 steps, as its steps are short: Rossler's, 0.02 of
// a time unit, part a billionth to a hundredth in about 330 units), or nothing where it does not stay
// finite that long.
constexpr uint32_t kStrangeMapDepth = 3, kStrangeFlowDepth = 5;
std::optional<std::vector<std::array<double, 3>>> whole_path(const Fractal& f)
{
    const uint32_t want = f.family == Fractal::Family::Flow ? kStrangeFlowDepth : kStrangeMapDepth;
    const uint32_t d = std::min(want, fractal_deepest_within(f, kFractalMost));
    auto p = fractal_path(f, d);
    // (the path a depth holds when every point is finite: its points less the 64 left out)
    if (!p || p->size() + 64 < (size_t(1024) << d)) return std::nullopt;
    return p;
}

// Strange: its path stays finite, and a start a billionth away (x moved) ends further from it than a
// thousandth of the path's extent: sensitive to where it starts, as a chaotic attractor is and a point, a
// loop or a torus is not.
bool strange(const Fractal& f)
{
    if (!f.is_path()) return true;
    const auto a = whole_path(f);
    if (!a || a->empty()) return false;
    Fractal g = f;
    g.from = true;
    g.from_value[0] += 1e-9;
    const auto b = whole_path(g);
    if (!b || b->size() != a->size()) return false;
    double extent = 0;
    for (int k = 0; k < 3; ++k)
    {
        double lo = (*a)[0][size_t(k)], hi = lo;
        for (const auto& q : *a) lo = std::min(lo, q[size_t(k)]), hi = std::max(hi, q[size_t(k)]);
        extent = std::max(extent, hi - lo);
    }
    if (!(extent > 1e-6)) return false;
    double apart = 0;
    for (int k = 0; k < 3; ++k) apart = std::max(apart, std::abs(a->back()[size_t(k)] - b->back()[size_t(k)]));
    return apart > extent * 1e-3;
}

// An automaton's live part at a generation, apart from where it stands: the field with its rows and
// columns of only the field beyond taken off (an elementary one's newest row), and the beyond.
std::vector<uint8_t> live_part(const AutomatonCells& c, bool newest_row)
{
    const uint32_t y0 = newest_row ? c.height - 1 : 0;
    uint32_t lx = c.width, hx = 0, ly = c.height, hy = 0;
    for (uint32_t y = y0; y < c.height; ++y)
        for (uint32_t x = 0; x < c.width; ++x)
            if (c.cells[size_t(y) * c.width + x] != c.beyond) lx = std::min(lx, x), hx = std::max(hx, x), ly = std::min(ly, y), hy = std::max(hy, y);
    std::vector<uint8_t> out{c.beyond};
    if (lx > hx) return out; // (nothing but the beyond: died out, or every cell the same)
    out.push_back(uint8_t(hx - lx + 1));
    for (uint32_t y = ly; y <= hy; ++y)
        for (uint32_t x = lx; x <= hx; ++x) out.push_back(c.cells[size_t(y) * c.width + x]);
    return out;
}

// Lasts: at generation G it is alive (some cell not the beyond), and its live part, wherever it stands,
// is none of the last kLastsLooked generations' (not frozen, not repeating with a short period, not a
// glider moving on). Every other family passes.
constexpr uint32_t kLastsLooked = 8;
bool lasts_at(const Fractal& f, uint32_t generations)
{
    if (!f.is_automaton()) return true;
    const uint32_t g = std::min(generations, fractal_deepest_within(f, kFractalMost));
    if (g < kLastsLooked) return false;
    const bool elementary = f.family == Fractal::Family::Elementary;
    std::vector<std::vector<uint8_t>> parts;
    if (elementary)
    {
        // (one call holds every generation's row: each row's own beyond is the cell at its left, outside
        // the window it had then, but for the newest, whose beyond is the field's)
        const auto c = fractal_cells(f, g);
        if (!c) return false;
        for (uint32_t k = g - kLastsLooked; k <= g; ++k)
        {
            AutomatonCells row;
            row.width = c->width, row.height = 1;
            row.cells.assign(c->cells.begin() + std::ptrdiff_t(size_t(k) * c->width), c->cells.begin() + std::ptrdiff_t(size_t(k + 1) * c->width));
            row.beyond = k == g ? c->beyond : c->cells[size_t(k) * c->width];
            parts.push_back(live_part(row, true));
        }
    }
    else
    {
        // (the last generations together, the field grown once)
        const auto cs = fractal_cells_from(f, g - kLastsLooked, g);
        if (!cs) return false;
        for (const AutomatonCells& c : *cs) parts.push_back(live_part(c, false));
    }
    if (parts.back().size() <= 1) return false;
    for (size_t k = 0; k + 1 < parts.size(); ++k)
        if (parts[k] == parts.back()) return false;
    return true;
}

class Lasts : public Filter
{
public:
    explicit Lasts(uint32_t generations) : generations_(generations) {}
    bool passes(std::span<const uint32_t> unit) const override
    {
        const auto f = fractal_of(unit);
        return f && lasts_at(*f, generations_);
    }

private:
    uint32_t generations_;
};

FilterSpec judge(const char* id, const char* description, Judge::Test t)
{
    FilterSpec s;
    s.id = id;
    s.title = id;
    s.description = description;
    s.applies = is_fractals;
    s.judge_only = true; // (how many of the space pass is not yet counted)
    s.make = [t](const FilterLine&, const FilterValues&, const FilterResources&) -> std::unique_ptr<Filter> { return std::make_unique<Judge>(t); };
    return s;
}

// ---------------------------------------------------------------- fractal-drawn (the image line)
//
// The fractal dimension's first way into another line (list X1c; Edward, 16 November 2026: "a second
// filter or setting ... that turns on or off its cross dimensional filtering from that specific
// dimension", include to find what the fractals make, exclude to knock it out): the image line's units
// that are a charted fractal drawn. Each charted fractal (fractal_chart()) at each depth it grows to
// within kDrawnMost symbols, drawn as the hallway draws it (draw_fractal) on a picture kScale times the
// unit's size a side (at most kSideMost), then fitted to the line as any picture warped in is
// (canon-image-v2). keep = include passes those pictures alone; exclude passes every other.

constexpr size_t kDrawnMost = size_t(1) << 18; // (as the hallway's shelves draw)
constexpr uint32_t kScale = 4, kSideMost = 512;

bool is_image(const FilterLine& l)
{
    return l.kind == "image" && l.width && l.height && l.length == l.width * l.height && l.symbols_id.rfind("image/", 0) == 0;
}

// The video line's units are frames one after another (list Y4): fractal-moving-v1's, a charted fractal's
// depths in turn, depth being time (IDEAS 19.239).
bool is_video(const FilterLine& l)
{
    return l.kind == "video" && l.width && l.height && l.frames >= 2 && l.length == l.width * l.height * l.frames && l.symbols_id.rfind("video/", 0) == 0;
}
// (a video's first depth from 0 to this at most: past it, an elementary automaton's 512 depths would be
// hundreds of videos each, of rows a cell high)
constexpr uint32_t kMovingStartsMost = 32;

// The pictures as a trie, walked a pixel at a time: include, the units in the trie; exclude, every unit
// but those (a state off the trie is free from there on). So both count and rank exactly, and the line
// compacts to the fractals drawn, or closes up without them.
// The memory of one node of DrawnRanker's trie: its map of children, its entry in its parent's, its count.
constexpr double kTrieNodeBytes = 104;

class DrawnRanker : public Ranker
{
public:
    DrawnRanker(const std::set<std::vector<uint32_t>>& units, uint32_t base, uint32_t length, bool keep)
        : base_(base), length_(length), keep_(keep)
    {
        node_.push_back({});
        under_.push_back(0);
        for (const auto& u : units)
        {
            size_t at = 0;
            ++under_[0];
            for (const uint32_t c : u)
            {
                auto it = node_[at].find(c);
                if (it == node_[at].end())
                {
                    it = node_[at].emplace(c, node_.size()).first;
                    node_.push_back({});
                    under_.push_back(0);
                }
                at = it->second;
                ++under_[at];
            }
        }
        free_ = node_.size();
        pow_.assign(length + 1, BigUint(1));
        for (uint32_t r = 1; r <= length; ++r)
        {
            pow_[r] = pow_[r - 1];
            pow_[r].mul_small(base);
        }
        set_count();
    }
    uint32_t length() const override { return length_; }
    uint32_t base() const override { return base_; }
    State start() const override { return 0; }
    State next(State s, uint32_t symbol) const override
    {
        if (s == free_) return free_;
        const auto it = node_[size_t(s)].find(symbol);
        if (it != node_[size_t(s)].end()) return it->second;
        return keep_ ? kDead : free_;
    }
    BigUint completions(State s, uint32_t remaining) const override
    {
        if (s == free_) return pow_[remaining];
        if (keep_) return BigUint(uint64_t(under_[size_t(s)]));
        BigUint all = pow_[remaining];
        all -= BigUint(uint64_t(under_[size_t(s)]));
        return all;
    }

private:
    uint32_t base_, length_;
    bool keep_;
    std::vector<std::map<uint32_t, size_t>> node_;
    std::vector<uint64_t> under_; // pictures under each node
    State free_ = 0;
    std::vector<BigUint> pow_;
};

// (ranked on palettes of 256 colours or fewer: a walk tries each colour in turn)
constexpr uint32_t kRankedColoursMost = 256;

// The variants of a picture drawn (version 2; IDEAS 19.244: "the colouring ruleset can also be inverted,
// flipped or just follow a completely different pattern/procedure which would allow for every possible
// colour/pixel colour variant of each fractal"). turns: the picture as drawn, or also turned and mirrored
// (the eight ways a square has, the four a picture that is not square has: mirrored either way and a half
// turn). colours: as drawn; inverted too (each colour's opposite, 255 less each channel, the palette's
// nearest); or any (every way of colouring the same shape: the pixels that share a colour in a picture
// share one in its variant, any colours, each different).
enum class Colours { Drawn, Inverted, Any };
struct Variants
{
    bool turns = false;
    Colours colours = Colours::Drawn;
};

// A frame's pixels turned: bit 1 mirrors left to right, bit 2 top to bottom, bit 4 swaps the axes (a square only).
std::vector<uint32_t> turned(std::span<const uint32_t> digits, uint32_t w, uint32_t h, uint32_t frames, uint32_t t)
{
    std::vector<uint32_t> out(digits.size());
    const size_t per = size_t(w) * h;
    for (uint32_t f = 0; f < frames; ++f)
        for (uint32_t y = 0; y < h; ++y)
            for (uint32_t x = 0; x < w; ++x)
            {
                uint32_t sx = (t & 1) ? w - 1 - x : x, sy = (t & 2) ? h - 1 - y : y;
                if (t & 4) std::swap(sx, sy);
                out[f * per + size_t(y) * w + x] = digits[f * per + size_t(sy) * w + sx];
            }
    return out;
}

std::vector<uint32_t> inverted(const std::vector<uint32_t>& digits, const Palette& p)
{
    std::map<uint32_t, uint32_t> opposite;
    std::vector<uint32_t> out(digits.size());
    for (size_t i = 0; i < digits.size(); ++i)
    {
        auto it = opposite.find(digits[i]);
        if (it == opposite.end())
        {
            const Rgb c = p.colour(digits[i]);
            it = opposite.emplace(digits[i], p.nearest(Rgb{uint8_t(255 - c.r), uint8_t(255 - c.g), uint8_t(255 - c.b)})).first;
        }
        out[i] = it->second;
    }
    return out;
}

// A picture's shape apart from its colours: each colour renamed by the order it first appears in.
std::vector<uint32_t> shape_of(std::span<const uint32_t> digits, uint32_t* classes = nullptr)
{
    std::map<uint32_t, uint32_t> name;
    std::vector<uint32_t> out(digits.size());
    for (size_t i = 0; i < digits.size(); ++i) out[i] = name.emplace(digits[i], uint32_t(name.size())).first->second;
    if (classes) *classes = uint32_t(name.size());
    return out;
}

// (any colours: written out as pictures, and so ranked, while there are this many at most; past it, counted
// exactly and judged)
constexpr uint64_t kAnyWrittenMost = uint64_t(1) << 16;

// A filter that keeps (include) or sets aside (exclude) a list of units the fractals make in a line, or
// with `any`, every renaming of their symbols (each unit's shape); a trie ranks the list exactly on
// alphabets of 256 symbols or fewer, and any is written out as units while it is small (kAnyWrittenMost).
// The most symbols a line may have for a list to be an automaton (Listed::as_dfa): past it, a table of
// even a few states would not fit the bound every automaton here keeps (2^24 cells).
constexpr uint32_t kListedAutomatonBaseMost = uint32_t(1) << 16;

class Listed : public Filter
{
public:
    bool passes(std::span<const uint32_t> unit) const override
    {
        const bool listed = any_ ? shapes_.count(shape_of(unit)) > 0 : units_.count(std::vector<uint32_t>(unit.begin(), unit.end())) > 0;
        return listed == keep_;
    }
    const Ranker* ranker() const override { return ranker_.get(); }
    const BigUint* survivors() const override { return ranker_ ? nullptr : &survivors_; }

    // The list as an automaton (list W3: so that it merges with the line's other automata and counts with
    // them), where it is one (not `any`) and fits: the smallest automaton of the sorted list, built a unit
    // at a time (Daciuk, Mihov, Watson and Watson, "Incremental construction of minimal acyclic finite-state
    // automata", 2000); for exclude, with one state more that every unit leaving the list goes to and stays
    // in, accepting. It passes exactly what this filter passes, and ranks them in the same order.
    std::optional<Dfa> as_dfa(uint32_t base, uint32_t length) const
    {
        if (any_ || units_.empty()) return std::nullopt;
        // A unit of the line's length it holds: then the automaton has a state at each depth at least (they accept
        // suffixes of different lengths), length + 1 in all, and where so many cannot fit the states' bound or the
        // filter memory, the automaton built would be refused (here, or by as_automaton): so it is not built. A
        // picture of 96 by 96 had its trie take 12 GB and overflow the stack before it was refused (review 4).
        const auto fits_line = [&](const std::vector<uint32_t>& u) {
            return u.size() == length && std::none_of(u.begin(), u.end(), [base](uint32_t c) { return c >= base; });
        };
        const uint64_t most_states = (uint64_t(1) << 24) / base;
        if (std::any_of(units_.begin(), units_.end(), fits_line) &&
            (uint64_t(length) + 1 > most_states || DfaRanker::table_bytes(size_t(length) + 1, base, length) > filter_memory()))
            return std::nullopt;
        struct Node
        {
            std::vector<std::pair<uint32_t, int32_t>> next; // (symbol, node), symbols increasing
            bool final = false;
        };
        std::vector<Node> nodes(1);
        std::map<std::pair<bool, std::vector<std::pair<uint32_t, int32_t>>>, int32_t> registry;
        // (a node's last child, made minimal: an equal one already registered takes its place; first that child's
        // own last child, and so on down, the deepest first, walked as a path rather than recursed into, since a
        // unit's path is as deep as the unit is long)
        std::vector<int32_t> path;
        const auto settle = [&](int32_t s) {
            path.assign(1, s);
            for (int32_t c = nodes[size_t(s)].next.back().second; !nodes[size_t(c)].next.empty(); c = nodes[size_t(c)].next.back().second)
                path.push_back(c);
            for (size_t k = path.size(); k-- > 0;)
            {
                Node& at = nodes[size_t(path[k])];
                const int32_t child = at.next.back().second;
                const auto key = std::make_pair(nodes[size_t(child)].final, nodes[size_t(child)].next);
                const auto [it, added] = registry.emplace(key, child);
                if (!added) at.next.back().second = it->second;
            }
        };
        for (const auto& u : units_) // (in order: a std::set)
        {
            // Only the units the line can hold: one of another length, or with a symbol past the
            // line's, is never met, and in the automaton would leave a path of the line's length
            // unfinished (exclude would then set aside units the list does not hold).
            if (!fits_line(u)) continue;
            int32_t s = 0;
            size_t i = 0;
            while (i < u.size() && !nodes[size_t(s)].next.empty() && nodes[size_t(s)].next.back().first == u[i])
                s = nodes[size_t(s)].next.back().second, ++i;
            if (!nodes[size_t(s)].next.empty()) settle(s);
            // The registered states are distinct and all stay in the automaton: past the bound, it would be refused.
            if (registry.size() > most_states) return std::nullopt;
            for (; i < u.size(); ++i)
            {
                nodes.push_back({});
                const int32_t n = int32_t(nodes.size() - 1);
                nodes[size_t(s)].next.push_back({u[i], n});
                s = n;
            }
            nodes[size_t(s)].final = true;
        }
        if (!nodes[0].next.empty()) settle(0);
        // Numbered from the root as they are reached, the orphans of the merging left out.
        std::vector<int32_t> number(nodes.size(), -1), order{0};
        number[0] = 0;
        for (size_t k = 0; k < order.size(); ++k)
            for (const auto& [c, n] : nodes[size_t(order[k])].next)
                if (number[size_t(n)] < 0) number[size_t(n)] = int32_t(order.size()), order.push_back(n);
        const size_t states = order.size() + (keep_ ? 0 : 1);
        if (states > most_states) return std::nullopt; // (as the other filters bound their automata)
        Dfa d;
        d.base = base;
        d.start = 0;
        const int32_t free = keep_ ? Dfa::kDead : int32_t(order.size());
        d.next.assign(states * base, free);
        d.accept.assign(states, 0);
        for (size_t k = 0; k < order.size(); ++k)
        {
            const Node& node = nodes[size_t(order[k])];
            for (const auto& [c, n] : node.next)
                if (c < base) d.next[k * base + c] = number[size_t(n)];
            d.accept[k] = keep_ ? node.final : 0;
        }
        if (!keep_)
        {
            for (uint32_t c = 0; c < base; ++c) d.next[size_t(free) * base + c] = free;
            d.accept[size_t(free)] = 1;
        }
        return minimise(d);
    }

protected:
    Listed(bool keep, bool any) : keep_(keep), any_(any) {}
    // Once units_ holds the list: counted, and ranked where it can be.
    void settle(uint32_t base, uint32_t length)
    {
        if (any_)
        {
            // Every colouring of each shape: k colours on a shape of k, base x (base - 1) x ... ways.
            std::map<std::vector<uint32_t>, uint32_t> shapes;
            for (const auto& u : units_)
            {
                uint32_t k = 0;
                auto s = shape_of(u, &k);
                shapes.emplace(std::move(s), k);
            }
            BigUint total;
            for (const auto& [s, k] : shapes)
            {
                BigUint ways(1);
                for (uint32_t i = 0; i < k; ++i) ways.mul_small(base > i ? base - i : 0);
                total += ways;
            }
            if (total <= BigUint(kAnyWrittenMost) && base <= kRankedColoursMost)
            {
                // Written out: each shape's colourings, in turn (the colours for its k classes, each different).
                units_.clear();
                for (const auto& [s, k] : shapes)
                {
                    std::vector<uint32_t> pick(k, 0), unit(s.size());
                    std::vector<bool> used(base, false);
                    // (a walk through the k-permutations of the palette, one class at a time)
                    std::function<void(uint32_t)> place = [&](uint32_t i) {
                        if (i == k)
                        {
                            for (size_t p = 0; p < s.size(); ++p) unit[p] = pick[s[p]];
                            units_.insert(unit);
                            return;
                        }
                        for (uint32_t c = 0; c < base; ++c)
                            if (!used[c])
                            {
                                used[c] = true, pick[i] = c;
                                place(i + 1);
                                used[c] = false;
                            }
                    };
                    place(0);
                }
                any_ = false; // (the pictures themselves now: judged and ranked as the others are)
            }
            else
            {
                for (const auto& [s, k] : shapes) shapes_.insert(s);
                survivors_ = keep_ ? total : BigUint::pow(base, length);
                if (!keep_) survivors_ -= total;
            }
        }
        if (!any_)
        {
            // The ranker's trie: a node for each distinct prefix of the sorted list (one more than the symbols
            // each unit does not share with the one before it), each about kTrieNodeBytes. Past 2^24 nodes, the bound
            // the filters' automata keep to, the list is counted and judged rather than ranked (a 96 by 96 picture
            // line, some 575 million nodes, ran out of memory; review 4).
            double nodes = 1;
            const std::vector<uint32_t>* before = nullptr;
            for (const auto& u : units_)
            {
                size_t same = 0;
                if (before)
                    while (same < u.size() && same < before->size() && u[same] == (*before)[same]) ++same;
                nodes += double(u.size() - same);
                before = &u;
            }
            const double bytes = nodes * kTrieNodeBytes;
            if (base <= kRankedColoursMost && nodes <= double(uint64_t(1) << 24)) ranker_ = std::make_unique<DrawnRanker>(units_, base, length, keep_);
            else
            {
                if (base <= kRankedColoursMost) set_table_bytes(bytes);
                survivors_ = keep_ ? BigUint(uint64_t(units_.size())) : BigUint::pow(base, length);
                if (!keep_) survivors_ -= BigUint(uint64_t(units_.size()));
            }
        }
    }
    std::string variants() const { return any_ ? std::to_string(shapes_.size()) + " shapes" : std::to_string(units_.size()); }

    bool keep_, any_;
    std::set<std::vector<uint32_t>> units_, shapes_;
    std::unique_ptr<DrawnRanker> ranker_;
    BigUint survivors_;
};

class Drawn : public Listed
{
public:
    Drawn(const FilterLine& l, bool keep, Variants v)
        : Listed(keep, v.colours == Colours::Any)
    {
        // (the palette from the line's symbols: "image/PALETTE/WxH")
        const std::string& id = l.symbols_id;
        const size_t a = id.find('/'), b = id.find('/', a + 1);
        ImageFormat fmt;
        const bool video = l.kind == "video";
        fmt.width = l.width, fmt.height = l.height, fmt.frames = video ? l.frames : 1;
        fmt.palette = &palette_by_id(id.substr(a + 1, b - a - 1));
        const auto chart = fractal_chart();
        std::string all;
        std::set<std::vector<uint32_t>> drawn;
        for (const auto& c : *chart)
        {
            all += c.id + "\n" + c.text;
            const uint32_t deepest = fractal_deepest_within(c.fractal, kDrawnMost);
            const uint32_t w = std::min(l.width * kScale, kSideMost), h = std::min(l.height * kScale, kSideMost);
            if (!video)
            {
                for (uint32_t d = 0; d <= deepest; ++d)
                    if (const auto img = draw_fractal(c.fractal, d, w, h)) drawn.insert(canonicalise_frame(*img, fmt));
                continue;
            }
            // (a video: the frames from depth s on, one a depth, for each s that leaves room for them all;
            // each depth drawn once)
            if (deepest + 1 < fmt.frames) continue;
            std::vector<std::optional<RgbaImage>> pictures(deepest + 1);
            for (uint32_t d = 0; d <= deepest && d < kMovingStartsMost + fmt.frames; ++d) pictures[d] = draw_fractal(c.fractal, d, w, h);
            for (uint32_t s0 = 0; s0 + fmt.frames - 1 <= deepest && s0 <= kMovingStartsMost; ++s0)
            {
                std::vector<RgbaImage> frames;
                for (uint32_t d = s0; d < s0 + fmt.frames; ++d)
                    if (pictures[d]) frames.push_back(*pictures[d]);
                if (frames.size() == fmt.frames) drawn.insert(canonicalise_image(frames, fmt));
            }
        }
        // The variants: turned, then (inverted, any) coloured.
        const uint32_t turns = !v.turns ? 1 : l.width == l.height ? 8 : 4;
        for (const auto& u : drawn)
            for (uint32_t t = 0; t < turns; ++t)
            {
                auto turned_unit = turned(u, l.width, l.height, fmt.frames, t);
                if (v.colours != Colours::Drawn) units_.insert(inverted(turned_unit, *fmt.palette));
                units_.insert(std::move(turned_unit));
            }
        settle(l.base, l.length);
        provenance_ = std::string("keep=") + (keep_ ? "include" : "exclude") + " chart=" + Sha256::hex_hash(all).substr(0, 16) + " fractals=" +
                      std::to_string(chart->size()) + " pictures=" + std::to_string(drawn.size());
        if (v.turns || v.colours != Colours::Drawn)
            provenance_ += std::string(" turns=") + (v.turns ? "every" : "drawn") + " colours=" +
                           (v.colours == Colours::Drawn ? "drawn" : v.colours == Colours::Inverted ? "inverted" : "any") + " variants=" +
                           variants();
    }
};

// ---------------------------------------------------------------- fractal-written, fractal-scored (list Z4)
//
// The fractal dimension in the pages and audio lines (list X1's "the other lines as further ways"). A
// ruleset's word at a depth is text already: on the pages line, a page that is a charted fractal's word,
// its first L characters (or the whole word and spaces after it), each character as the line's alphabet
// writes it (an upper-case letter as its lower case where the alphabet has none); with symbols = any,
// every renaming of its characters into the alphabet. On the audio line (notes), a melody that is a
// charted fractal's, as Prusinkiewicz made scores of L-systems (Score generation with L-systems, 1986):
// the turtle's lines across, left to right, each a note, its height the step of the major scale from the
// line's lowest note, its length the note's (a step an eighth; the nearest the line has), lines one after
// another at one height one note; a substitution's word as one note a symbol, each symbol (in the order
// met) a step of the scale, a quarter each. With transpose = every, at every pitch the line holds it.

bool is_text(const FilterLine& l) { return l.kind == "text" && l.alphabet && !l.up_to_length && l.length >= 1; }
bool is_notes(const FilterLine& l) { return l.kind == "audio" && is_note_symbols(l.symbols_id) && l.length >= 1; }

class Written : public Listed
{
public:
    Written(const FilterLine& l, bool keep, bool any) : Listed(keep, any)
    {
        const Alphabet& a = *l.alphabet;
        const auto space = a.digit_of(U' ');
        const auto chart = fractal_chart();
        std::string all;
        for (const auto& c : *chart)
        {
            all += c.id + "\n" + c.text;
            if (!c.fractal.rewrites()) continue;
            const uint32_t deepest = fractal_deepest_within(c.fractal, kDrawnMost);
            for (uint32_t d = 0; d <= deepest; ++d)
            {
                const auto w = c.fractal.grow(d);
                if (!w) break;
                std::vector<uint32_t> unit;
                bool fits = true;
                for (size_t i = 0; i < w->size() && unit.size() < l.length; ++i)
                {
                    char ch = (*w)[i];
                    if (!a.has_uppercase() && ch >= 'A' && ch <= 'Z') ch = char(ch - 'A' + 'a');
                    const auto digit = a.digit_of(char32_t(uint8_t(ch)));
                    if (!digit)
                    {
                        fits = false;
                        break;
                    }
                    unit.push_back(*digit);
                }
                if (!fits) continue;
                if (unit.size() < l.length)
                {
                    if (!space) continue;
                    unit.resize(l.length, *space);
                }
                units_.insert(std::move(unit));
            }
        }
        settle(l.base, l.length);
        provenance_ = std::string("keep=") + (keep_ ? "include" : "exclude") + " symbols=" + (any ? "any" : "written") + " chart=" +
                      Sha256::hex_hash(all).substr(0, 16) + " fractals=" + std::to_string(chart->size()) + " pages=" + variants();
    }
};

// A melody as (MIDI number, 0 a rest; sixteenths) events.
using Melody = std::vector<std::pair<uint32_t, uint32_t>>;
constexpr uint32_t kMajorSteps[7] = {0, 2, 4, 5, 7, 9, 11};
uint32_t scale_midi(uint32_t tonic, uint32_t step) { return tonic + 12 * (step / 7) + kMajorSteps[step % 7]; }

std::optional<Melody> score_of(const Fractal& f, uint32_t depth, uint32_t tonic)
{
    const auto w = f.grow(depth);
    if (!w) return std::nullopt;
    Melody out;
    if (f.family == Fractal::Family::Substitution)
    {
        std::map<char, uint32_t> step;
        for (const char ch : *w)
        {
            const uint32_t k = step.emplace(ch, uint32_t(step.size())).first->second;
            out.push_back({scale_midi(tonic, k), 4});
        }
        return out;
    }
    // The lines across (left to right, level to a millionth of a step), joined where one carries on another.
    struct Across
    {
        double x0, x1, y;
    };
    std::vector<Across> notes;
    for (const auto& ln : fractal_lines(f, *w))
    {
        const double dx = ln[2] - ln[0], dy = ln[3] - ln[1];
        if (!(dx > 1e-6) || std::abs(dy) > 1e-6 * dx) continue;
        if (!notes.empty() && std::abs(notes.back().x1 - ln[0]) < 1e-6 && std::abs(notes.back().y - ln[1]) < 1e-6) notes.back().x1 = ln[2];
        else notes.push_back({ln[0], ln[2], ln[1]});
    }
    if (notes.empty()) return out;
    double low = notes[0].y;
    for (const auto& n : notes) low = std::min(low, n.y);
    for (const auto& n : notes)
    {
        const double step = std::round(n.y - low), length = std::round(n.x1 - n.x0);
        if (step > 64 || length < 1) return std::nullopt; // (heights or lengths that are not whole steps: not a score)
        out.push_back({scale_midi(tonic, uint32_t(step)), uint32_t(length) * 2});
    }
    return out;
}

class Scored : public Listed
{
public:
    Scored(const FilterLine& l, bool keep, bool every_pitch) : Listed(keep, false)
    {
        const NoteSet set = note_set_of(l.symbols_id);
        // (the line's first C at or above its lowest note: the tonic)
        const uint32_t tonic = set.low + (12 - set.low % 12) % 12;
        std::vector<uint32_t> lengths; // each of the set's durations, in sixteenths
        for (const char code : set.durations)
        {
            static const uint32_t kSixteenths[] = {1, 2, 3, 4, 6, 8, 12, 16};
            lengths.push_back(kSixteenths[std::string_view(kNoteDurationCodes).find(code)]);
        }
        auto nearest = [&](uint32_t sixteenths) {
            uint32_t best = 0;
            for (uint32_t i = 1; i < lengths.size(); ++i)
            {
                const uint32_t d = uint32_t(std::abs(int(lengths[i]) - int(sixteenths))), b = uint32_t(std::abs(int(lengths[best]) - int(sixteenths)));
                if (d < b || (d == b && lengths[i] > lengths[best])) best = i; // (a tie to the longer, as canon-notes-v2)
            }
            return best;
        };
        const auto chart = fractal_chart();
        std::string all;
        for (const auto& c : *chart)
        {
            all += c.id + "\n" + c.text;
            if (!c.fractal.rewrites() || c.fractal.version == 3) continue;
            const uint32_t deepest = fractal_deepest_within(c.fractal, kDrawnMost);
            for (uint32_t d = 0; d <= deepest; ++d)
            {
                const auto m = score_of(c.fractal, d, tonic);
                if (!m) break;
                if (m->empty()) continue;
                // Its first notes, as digits (pitch index, then duration), voice 1; the rest rests.
                std::vector<int> pitch;
                std::vector<uint32_t> dur;
                for (size_t i = 0; i < m->size() && pitch.size() < l.length / set.voices; ++i)
                    pitch.push_back((*m)[i].first ? int((*m)[i].first) - int(set.low) + 1 : 0), dur.push_back(nearest((*m)[i].second));
                int lo = 1 << 30, hi = -1;
                for (const int p : pitch)
                    if (p) lo = std::min(lo, p), hi = std::max(hi, p);
                for (int shift = every_pitch ? 1 - lo : 0; shift <= (every_pitch ? int(set.pitches()) - hi : 0); ++shift)
                {
                    std::vector<uint32_t> unit(l.length, 0);
                    bool fits = true;
                    for (size_t i = 0; i < pitch.size(); ++i)
                    {
                        const int p = pitch[i] ? pitch[i] + shift : 0;
                        if (p < 0 || p > int(set.pitches())) fits = false;
                        unit[i] = uint32_t(p) * set.duration_count() + dur[i];
                    }
                    if (fits) units_.insert(std::move(unit));
                }
            }
        }
        settle(l.base, l.length);
        provenance_ = std::string("keep=") + (keep_ ? "include" : "exclude") + " transpose=" + (every_pitch ? "every" : "drawn") + " chart=" +
                      Sha256::hex_hash(all).substr(0, 16) + " fractals=" + std::to_string(chart->size()) + " melodies=" + variants();
    }
};

// A list filter as an automaton where it is one (Listed::as_dfa) and its counting table fits the filter
// memory, else as it is: a long list's automaton has about a state a symbol of each unit, and its table
// a row of counts for each, where the list's own trie counts and ranks in far less.
std::unique_ptr<Filter> as_automaton(std::unique_ptr<Listed> f, const FilterLine& l)
{
    if (auto d = f->as_dfa(l.base, l.length); d && DfaRanker::table_bytes(d->states(), d->base, l.length) <= filter_memory())
        return make_dfa_filter(std::move(*d), l.length, f->provenance());
    return f;
}

Variants variants_of(const FilterSpec& spec, const FilterValues& v)
{
    Variants out;
    if (spec.version < 2) return out;
    out.turns = param_value(spec, v, "turns") == "every";
    const std::string c = param_value(spec, v, "colours");
    out.colours = c == "any" ? Colours::Any : c == "inverted" ? Colours::Inverted : Colours::Drawn;
    return out;
}

} // namespace

void add_fractal_filters(std::vector<FilterSpec>& out)
{
    out.push_back(judge("fractal-draws",
                        "Draws something: an L-system whose axiom reaches F or G (a line drawn) at some depth; a "
                        "substitution whose axiom reaches two symbols or more (two colours); every IFS; an escape or "
                        "converge formula that mentions what differs from point to point (c, or z for a Julia set); an "
                        "automaton with a live cell in its first 8 generations; an attractor or a flow whose path visits two "
                        "points or more; every aggregate. A "
                        "ruleset that draws nothing is a blank picture at every depth.",
                        draws));
    out.push_back(judge("fractal-grows",
                        "Grows: a symbol the axiom reaches has a rule of two symbols or more, so its word is longer at "
                        "every depth. One that does not is the same length at every depth, a picture that never "
                        "changes size. An IFS grows with two maps or more; an escape or converge formula, where it "
                        "mentions z; an automaton, where a generation of its first 8 differs from the one before; an attractor or "
                        "a flow whose path does not settle (16 points or more among its first depth's last 256); every aggregate.",
                        grows));
    out.push_back(judge("fractal-no-idle",
                        "No idle rules: no rule for a symbol the axiom never reaches, and no rule that rewrites a "
                        "symbol to itself. Either draws what the ruleset without that rule draws, which is at an "
                        "address of its own. An IFS: no map given twice. Every escape or converge formula, automaton, "
                        "attractor, flow and aggregate passes.",
                        no_idle));
    out.push_back(judge("fractal-canonical",
                        "One ruleset of each renaming: the symbols whose names change nothing drawn (an L-system's "
                        "letters but F, G and f) or only a colour (every substitution symbol) take the first names "
                        "free in the order the axiom reaches them, and every rule is for a symbol reached. Of the "
                        "charted fractals, the dragon curve (X and Y) is a renaming of A and B. An IFS: its maps in the "
                        "order of their texts, each once (another order draws the same). An automaton: its start written "
                        "one way (an elementary one's \"1\" left unwritten, a life-like one's field with no empty row "
                        "at the top or bottom and a live cell in its left column). An attractor or a flow: its start not "
                        "written where it is the one it has without it. Every escape or converge formula and every "
                        "aggregate passes.",
                        canonical));
    // The four above count (list F, IDEAS 19.250): by the symbols the axiom reaches, exactly, on v1 to v3, nearly
    // all of the line (sieve/fractal_count.hpp); v4 to v6, about 10^-200 of it at the default, are judged.
    for (size_t k = out.size() - 4; k < out.size(); ++k)
    {
        out[k].judge_only = false;
        out[k].description += " Counted exactly on v1 to v3 by the symbols the axiom reaches; v4 to v6 judged.";
    }
    out.push_back(judge("fractal-strange",
                        "Strange: an attractor or a flow whose path stays finite through its first 8,192 points (a flow's "
                        "32,768 steps) and on "
                        "which a start a billionth away (x moved) ends further from it than a thousandth of the path's "
                        "extent: sensitive to where it starts, as a chaotic attractor is, where one that settles on a "
                        "point, a loop or a torus, or flies off, is not. Every other family passes.",
                        strange));
    {
        FilterSpec lasts;
        lasts.id = "fractal-lasts";
        lasts.title = "fractal-lasts";
        lasts.description = "Lasts: an automaton alive at the generation set (some cell not the field beyond) whose live part, "
                            "wherever it stands, is none of the 8 generations' before: one that dies out, freezes, repeats "
                            "with a short period, or is a glider moving on, does not pass. An elementary automaton is "
                            "judged by its newest row. Every other family passes.";
        lasts.params = {{"generations", "the generation it is judged at", FilterParam::Kind::Integer, "32", 8, 256, 8, {}}};
        lasts.applies = is_fractals;
        lasts.judge_only = true;
        lasts.make = [own = lasts.name()](const FilterLine&, const FilterValues& v, const FilterResources&) -> std::unique_ptr<Filter> {
            return std::make_unique<Lasts>(uint32_t(std::stoi(param_value(*find_filter(own), v, "generations"))));
        };
        out.push_back(lasts);
    }

    // How fractal a drawing is (list X2; IDEAS 19.235: "a measure of how fractal a drawing is (its
    // box-counting dimension, worked out from the picture) could rank what is left").
    FilterSpec dim;
    dim.id = "fractal-dimension";
    dim.title = "fractal-dimension";
    dim.description = "An L-system whose drawing's box-counting dimension, measured (sieve fractal show), is at least "
                      "the least set, in hundredths: 110 passes those measured at 1.10 or more, setting aside the "
                      "straight lines (1), the curves that are hardly bent and those too small to measure (fewer "
                      "than 16 lines, or under three grids). The Koch curve measures 1.29, the Sierpinski triangle "
                      "1.58, the Hilbert curve 2.00. A substitution, whose rows are a sequence and not a figure, "
                      "passes, and so does an escape or converge picture, a life-like automaton or a flow (not measured); "
                      "an IFS or an attractor is measured on its points, an elementary automaton on its live cells over "
                      "its history, an aggregate on its cells.";
    dim.params = {{"least", "the least box-counting dimension passed, in hundredths (110: 1.10)", FilterParam::Kind::Integer, "110", 0, 200, 5, {}}};
    dim.applies = is_fractals;
    dim.judge_only = true;
    dim.make = [own = dim.name()](const FilterLine&, const FilterValues& v, const FilterResources&) -> std::unique_ptr<Filter> {
        return std::make_unique<AtLeast>(std::stoi(param_value(*find_filter(own), v, "least")));
    };
    out.push_back(dim);

    FilterSpec drawn;
    drawn.id = "fractal-drawn";
    drawn.title = "fractal-drawn";
    drawn.description = "The fractal dimension in the image line: a picture that is a charted fractal drawn, at any depth it "
                        "grows to (within 2^18 symbols), on a picture four times the unit's size a side (at most 512) "
                        "and fitted to the line as a picture warped in is (canon-image-v2). keep = include passes those "
                        "pictures alone (find what the fractals make); exclude passes every other (knock them out). "
                        "Exact: counts and ranks (compact) on palettes of 256 colours or fewer; on rgb24, counted exactly "
                        "and judged. Kept so earlier stacks reproduce: fractal-drawn-v2 adds each picture turned and "
                        "mirrored, and coloured other ways.";
    drawn.params = {{"keep", "include: only the fractals drawn; exclude: everything but them", FilterParam::Kind::Text, "include", 0, 0, 1,
                     {"include", "exclude"}}};
    drawn.applies = is_image;
    // Its own trie of the pictures: as an automaton (Listed::as_dfa) the 62,356 pictures of fractal-drawn-v2 at
    // 10 x 10 in black and white make 1,031,932 states, whose counting table (1.9 GB) is past the filter
    // memory, and fractal-moving-v2's 23,830 videos 5,903,640 (87.6 GB); the trie counts and ranks them in far
    // less. (Where the filter memory is set high enough to hold the table, it is made the automaton, and merges.)
    drawn.counts_as = "own";
    drawn.retired = true;
    drawn.replaced_by = "fractal-drawn-v2";
    drawn.make = [own = drawn.name()](const FilterLine& l, const FilterValues& v, const FilterResources&) -> std::unique_ptr<Filter> {
        const FilterSpec& spec = *find_filter(own);
        return as_automaton(std::make_unique<Drawn>(l, param_value(spec, v, "keep") == "include", variants_of(spec, v)), l);
    };
    drawn.set_elsewhere = true;
    out.push_back(drawn);

    // Version 2 (IDEAS 19.244): the same pictures turned, mirrored and coloured other ways.
    const FilterParam turns{"turns", "drawn: each picture as drawn; every: also turned and mirrored (eight ways on a square "
                                     "picture, four otherwise)",
                            FilterParam::Kind::Text, "every", 0, 0, 1, {"drawn", "every"}};
    const FilterParam colours{"colours", "drawn: as drawn; inverted: also each colour's opposite; any: every colouring of the "
                                         "same shape (the pixels that share a colour share one, any colours, each different)",
                              FilterParam::Kind::Text, "inverted", 0, 0, 1, {"drawn", "inverted", "any"}};
    FilterSpec drawn2 = drawn;
    drawn2.version = 2;
    drawn2.retired = false;
    drawn2.replaced_by.clear();
    drawn2.description = "The fractal dimension in the image line: a picture that is a charted fractal drawn, at any depth it "
                         "grows to (within 2^18 symbols), on a picture four times the unit's size a side (at most 512) and "
                         "fitted to the line as a picture warped in is (canon-image-v2); with turns = every, also turned and "
                         "mirrored; with colours = inverted, also in each colour's opposite; with colours = any, in every "
                         "colouring of its shape. keep = include passes those pictures alone (find what the fractals make); "
                         "exclude passes every other (knock them out). Set from the FRACTALS rows of the setup page. Exact: "
                         "counts and ranks (compact) on palettes of 256 colours or fewer; on rgb24, and with colours = any "
                         "past 65,536 pictures, counted exactly and judged.";
    drawn2.params.push_back(turns);
    drawn2.params.push_back(colours);
    drawn2.make = [own = drawn2.name()](const FilterLine& l, const FilterValues& v, const FilterResources&) -> std::unique_ptr<Filter> {
        const FilterSpec& spec = *find_filter(own);
        return as_automaton(std::make_unique<Drawn>(l, param_value(spec, v, "keep") == "include", variants_of(spec, v)), l);
    };
    drawn2.set_elsewhere = true;
    out.push_back(drawn2);

    // In the video line (list Y4; IDEAS 19.239: "every frame of cellular automata simulations could become
    // a texture / image as well", depth being time for every family): a video that is a charted fractal
    // growing, its frames the depths one after another.
    FilterSpec moving = drawn;
    moving.id = "fractal-moving";
    moving.title = "fractal-moving";
    moving.description = "The fractal dimension in the video line: a video that is a charted fractal growing, its frames "
                         "its depths in turn (depth is time: an automaton's generations, a fern's levels, a Mandelbrot "
                         "set's steps), from depth 0 to 32 on, each depth drawn as fractal-drawn-v1 draws it and the "
                         "frames fitted as a video warped in is. keep = include passes those videos alone; exclude "
                         "passes every other. Kept so earlier stacks reproduce: fractal-moving-v2 adds each video turned "
                         "and mirrored, and coloured other ways.";
    moving.applies = is_video;
    moving.replaced_by = "fractal-moving-v2";
    moving.make = [own = moving.name()](const FilterLine& l, const FilterValues& v, const FilterResources&) -> std::unique_ptr<Filter> {
        const FilterSpec& spec = *find_filter(own);
        return as_automaton(std::make_unique<Drawn>(l, param_value(spec, v, "keep") == "include", variants_of(spec, v)), l);
    };
    moving.set_elsewhere = true;
    out.push_back(moving);

    FilterSpec moving2 = moving;
    moving2.version = 2;
    moving2.retired = false;
    moving2.replaced_by.clear();
    moving2.description = "The fractal dimension in the video line: a video that is a charted fractal growing, its frames its "
                          "depths in turn (depth is time: an automaton's generations, a fern's levels, a Mandelbrot set's "
                          "steps), from depth 0 to 32 on, each depth drawn as fractal-drawn-v2 draws it and the frames fitted "
                          "as a video warped in is; turned, mirrored and coloured as fractal-drawn-v2's pictures are, every "
                          "frame alike. keep = include passes those videos alone; exclude passes every other. Set from the "
                          "FRACTALS rows of the setup page, with fractal-drawn-v2.";
    moving2.params.push_back(turns);
    moving2.params.push_back(colours);
    moving2.make = [own = moving2.name()](const FilterLine& l, const FilterValues& v, const FilterResources&) -> std::unique_ptr<Filter> {
        const FilterSpec& spec = *find_filter(own);
        return as_automaton(std::make_unique<Drawn>(l, param_value(spec, v, "keep") == "include", variants_of(spec, v)), l);
    };
    moving2.set_elsewhere = true;
    out.push_back(moving2);

    // In the pages and audio lines (list Z4).
    // As an automaton the fractals' words make 7,496 states on the pages line (ascii96, 32 characters) and
    // their melodies 18,829 on the audio line (notes104, 16 notes), tables of a few megabytes; on a line of more symbols than an
    // automaton's table takes (kListedAutomatonBaseMost), the list's own trie or a count.
    const auto listed_counts_as = [](const FilterLine& l, const FilterValues*) {
        return std::string(l.base > kListedAutomatonBaseMost ? "own" : "automaton");
    };
    FilterSpec written;
    written.id = "fractal-written";
    written.title = "fractal-written";
    written.description = "The fractal dimension in the pages line: a page that is a charted fractal's word at a depth (within "
                          "2^18 symbols), its first characters to the page's length or the whole word and spaces after it, each "
                          "character as the alphabet writes it (an upper-case letter as its lower case where the alphabet has "
                          "none; a word with a character the alphabet lacks is not on the line); with symbols = any, every "
                          "renaming of its characters into the alphabet. keep = include passes those pages alone; exclude "
                          "every other. Set from the FRACTALS rows of the setup page. Exact: counts and ranks (compact) on "
                          "alphabets of 256 symbols or fewer; symbols = any past 65,536 pages is counted exactly and judged.";
    written.params = {{"keep", "include: only the fractals' words; exclude: everything but them", FilterParam::Kind::Text, "include", 0, 0, 1,
                       {"include", "exclude"}},
                      {"symbols", "written: as the word writes them; any: every renaming of its characters", FilterParam::Kind::Text, "written", 0,
                       0, 1, {"written", "any"}}};
    written.applies = is_text;
    written.counts_as = "automaton"; // (Listed::as_dfa: so it merges with the line's other automata)
    written.counts_as_on = listed_counts_as;
    written.make = [own = written.name()](const FilterLine& l, const FilterValues& v, const FilterResources&) -> std::unique_ptr<Filter> {
        const FilterSpec& spec = *find_filter(own);
        return as_automaton(std::make_unique<Written>(l, param_value(spec, v, "keep") == "include", param_value(spec, v, "symbols") == "any"), l);
    };
    written.set_elsewhere = true;
    out.push_back(written);

    FilterSpec scored;
    scored.id = "fractal-scored";
    scored.title = "fractal-scored";
    scored.description = "The fractal dimension in the audio line (notes): a melody that is a charted fractal's at a depth, as "
                         "Prusinkiewicz made scores of L-systems (1986): the turtle's lines across, left to right, each a note, "
                         "its height the step of the major scale up from the line's first C, its length the note's (a step an "
                         "eighth, the nearest length the line has), lines carrying on at one height one note; a substitution's "
                         "word one quarter note a symbol, each symbol in the order met a step of the scale. Its first notes to "
                         "the line's length in the first voice, rests after; with transpose = every (the default), at every "
                         "pitch the line holds it. keep = include passes those melodies alone; exclude every other. Exact: "
                         "counts and ranks (compact) on sets of 256 symbols or fewer (notes104); counted exactly and judged "
                         "past that.";
    scored.params = {{"keep", "include: only the fractals' melodies; exclude: everything but them", FilterParam::Kind::Text, "include", 0, 0, 1,
                      {"include", "exclude"}},
                     {"transpose", "drawn: from the line's first C; every: at every pitch it fits", FilterParam::Kind::Text, "every", 0, 0, 1,
                      {"drawn", "every"}}};
    scored.applies = is_notes;
    scored.counts_as = "automaton"; // (Listed::as_dfa: so it merges with the line's other automata)
    scored.counts_as_on = listed_counts_as;
    scored.make = [own = scored.name()](const FilterLine& l, const FilterValues& v, const FilterResources&) -> std::unique_ptr<Filter> {
        const FilterSpec& spec = *find_filter(own);
        return as_automaton(std::make_unique<Scored>(l, param_value(spec, v, "keep") == "include", param_value(spec, v, "transpose") == "every"), l);
    };
    scored.set_elsewhere = true;
    out.push_back(scored);
}

} // namespace sieve
