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
        if (f->family == Fractal::Family::Substitution || f->family == Fractal::Family::Escape || f->family == Fractal::Family::Converge) return true;
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

bool draws(const Fractal& f)
{
    if (f.family == Fractal::Family::Ifs) return true;
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

class Drawn : public Filter
{
public:
    Drawn(const FilterLine& l, bool keep)
        : keep_(keep)
    {
        // (the palette from the line's symbols: "image/PALETTE/WxH")
        const std::string& id = l.symbols_id;
        const size_t a = id.find('/'), b = id.find('/', a + 1);
        ImageFormat fmt;
        fmt.width = l.width, fmt.height = l.height, fmt.frames = 1;
        fmt.palette = &palette_by_id(id.substr(a + 1, b - a - 1));
        const auto chart = fractal_chart();
        std::string all;
        for (const auto& c : *chart)
        {
            all += c.id + "\n" + c.text;
            const uint32_t deepest = fractal_deepest_within(c.fractal, kDrawnMost);
            for (uint32_t d = 0; d <= deepest; ++d)
                if (const auto img = draw_fractal(c.fractal, d, std::min(l.width * kScale, kSideMost), std::min(l.height * kScale, kSideMost)))
                    units_.insert(canonicalise_frame(*img, fmt));
        }
        if (l.base <= kRankedColoursMost) ranker_ = std::make_unique<DrawnRanker>(units_, l.base, l.length, keep_);
        else
        {
            survivors_ = keep_ ? BigUint(uint64_t(units_.size())) : BigUint::pow(l.base, l.length);
            if (!keep_) survivors_ -= BigUint(uint64_t(units_.size()));
        }
        provenance_ = std::string("keep=") + (keep_ ? "include" : "exclude") + " chart=" + Sha256::hex_hash(all).substr(0, 16) + " fractals=" +
                      std::to_string(chart->size()) + " pictures=" + std::to_string(units_.size());
    }
    bool passes(std::span<const uint32_t> unit) const override
    {
        const bool drawn = units_.count(std::vector<uint32_t>(unit.begin(), unit.end())) > 0;
        return drawn == keep_;
    }
    const Ranker* ranker() const override { return ranker_.get(); }
    const BigUint* survivors() const override { return ranker_ ? nullptr : &survivors_; }

private:
    bool keep_;
    std::set<std::vector<uint32_t>> units_;
    std::unique_ptr<DrawnRanker> ranker_;
    BigUint survivors_;
};

} // namespace

void add_fractal_filters(std::vector<FilterSpec>& out)
{
    out.push_back(judge("fractal-draws",
                        "Draws something: an L-system whose axiom reaches F or G (a line drawn) at some depth; a "
                        "substitution whose axiom reaches two symbols or more (two colours); every IFS; an escape or "
                        "converge formula that mentions what differs from point to point (c, or z for a Julia set). A "
                        "ruleset that draws nothing is a blank picture at every depth.",
                        draws));
    out.push_back(judge("fractal-grows",
                        "Grows: a symbol the axiom reaches has a rule of two symbols or more, so its word is longer at "
                        "every depth. One that does not is the same length at every depth, a picture that never "
                        "changes size. An IFS grows with two maps or more; an escape or converge formula, where it "
                        "mentions z.",
                        grows));
    out.push_back(judge("fractal-no-idle",
                        "No idle rules: no rule for a symbol the axiom never reaches, and no rule that rewrites a "
                        "symbol to itself. Either draws what the ruleset without that rule draws, which is at an "
                        "address of its own. An IFS: no map given twice. Every escape or converge formula passes.",
                        no_idle));
    out.push_back(judge("fractal-canonical",
                        "One ruleset of each renaming: the symbols whose names change nothing drawn (an L-system's "
                        "letters but F, G and f) or only a colour (every substitution symbol) take the first names "
                        "free in the order the axiom reaches them, and every rule is for a symbol reached. Of the "
                        "charted fractals, the dragon curve (X and Y) is a renaming of A and B. An IFS: its maps in the "
                        "order of their texts, each once (another order draws the same). Every escape or converge "
                        "formula passes.",
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
                      "passes, and so does an escape or converge picture (not measured); an IFS is measured on its "
                      "points.";
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
                        "pictures alone (find what the fractals make); exclude passes every other (knock them out). Set "
                        "from the FRACTALS rows of the setup page. Exact: counts and ranks (compact) on palettes of 256 "
                        "colours or fewer; on rgb24, counted exactly and judged.";
    drawn.params = {{"keep", "include: only the fractals drawn; exclude: everything but them", FilterParam::Kind::Text, "include", 0, 0, 1,
                     {"include", "exclude"}}};
    drawn.applies = is_image;
    drawn.counts_as = "own"; // (a trie of its pictures, not merged with other automata)
    drawn.make = [own = drawn.name()](const FilterLine& l, const FilterValues& v, const FilterResources&) -> std::unique_ptr<Filter> {
        return std::make_unique<Drawn>(l, param_value(*find_filter(own), v, "keep") == "include");
    };
    out.push_back(drawn);
}

} // namespace sieve
