// The fractal dimension's filters (list X, X1; IDEAS 19.235): rules that set aside the noise of the
// whole space (fractal-space-v2), every ruleset the reader takes. Most of it draws nothing, or never
// changes, or is another ruleset under other names (Edward, 16 November 2026: "it looks like there are a
// lot of fractals that aren't going to be worth looking at and figuring out a way to filter some of the
// noise there would also be useful in the long run").
//
// A unit is a ruleset's text (sieve-fractal-v1 or v2), a byte a symbol; a text the reader refuses fails
// every one of them. Each judges a ruleset by the symbols its axiom reaches, rule after rule, at any
// depth (its "reach"), without growing it, but fractal-dimension (list X2), which draws it and measures. None of them can yet count how many of the space pass, so each
// judges only and the line has no compact mode.
#include "sieve/filter.hpp"
#include "sieve/fractal.hpp"
#include "sieve/image.hpp"
#include "sieve/sha256.hpp"

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

class Drawn : public Filter
{
public:
    Drawn(const FilterLine& l, bool keep, Variants v)
        : keep_(keep), any_(v.colours == Colours::Any)
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
        const uint32_t base = l.base;
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
                survivors_ = keep_ ? total : BigUint::pow(base, l.length);
                if (!keep_) survivors_ -= total;
            }
        }
        if (!any_)
        {
            if (base <= kRankedColoursMost) ranker_ = std::make_unique<DrawnRanker>(units_, base, l.length, keep_);
            else
            {
                survivors_ = keep_ ? BigUint(uint64_t(units_.size())) : BigUint::pow(base, l.length);
                if (!keep_) survivors_ -= BigUint(uint64_t(units_.size()));
            }
        }
        provenance_ = std::string("keep=") + (keep_ ? "include" : "exclude") + " chart=" + Sha256::hex_hash(all).substr(0, 16) + " fractals=" +
                      std::to_string(chart->size()) + " pictures=" + std::to_string(drawn.size());
        if (v.turns || v.colours != Colours::Drawn)
            provenance_ += std::string(" turns=") + (v.turns ? "every" : "drawn") + " colours=" +
                           (v.colours == Colours::Drawn ? "drawn" : v.colours == Colours::Inverted ? "inverted" : "any") + " variants=" +
                           (any_ ? std::to_string(shapes_.size()) + " shapes" : std::to_string(units_.size()));
    }
    bool passes(std::span<const uint32_t> unit) const override
    {
        const bool drawn = any_ ? shapes_.count(shape_of(unit)) > 0 : units_.count(std::vector<uint32_t>(unit.begin(), unit.end())) > 0;
        return drawn == keep_;
    }
    const Ranker* ranker() const override { return ranker_.get(); }
    const BigUint* survivors() const override { return ranker_ ? nullptr : &survivors_; }

private:
    bool keep_, any_;
    std::set<std::vector<uint32_t>> units_, shapes_;
    std::unique_ptr<DrawnRanker> ranker_;
    BigUint survivors_;
};

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
    drawn.counts_as = "own"; // (a trie of its pictures, not merged with other automata)
    drawn.retired = true;
    drawn.replaced_by = "fractal-drawn-v2";
    drawn.make = [own = drawn.name()](const FilterLine& l, const FilterValues& v, const FilterResources&) -> std::unique_ptr<Filter> {
        const FilterSpec& spec = *find_filter(own);
        return std::make_unique<Drawn>(l, param_value(spec, v, "keep") == "include", variants_of(spec, v));
    };
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
        return std::make_unique<Drawn>(l, param_value(spec, v, "keep") == "include", variants_of(spec, v));
    };
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
        return std::make_unique<Drawn>(l, param_value(spec, v, "keep") == "include", variants_of(spec, v));
    };
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
        return std::make_unique<Drawn>(l, param_value(spec, v, "keep") == "include", variants_of(spec, v));
    };
    out.push_back(moving2);
}

} // namespace sieve
