// The fractal dimension's filters (list X, X1; IDEAS 19.235): rules that set aside the noise of the
// whole space (fractal-space-v1), every ruleset the reader takes. Most of it draws nothing, or never
// changes, or is another ruleset under other names (Edward, 16 November 2026: "it looks like there are a
// lot of fractals that aren't going to be worth looking at and figuring out a way to filter some of the
// noise there would also be useful in the long run").
//
// A unit is a ruleset's text (sieve-fractal-v1), a byte a symbol; a text the reader refuses fails every
// one of them. Each judges a ruleset by the symbols its axiom reaches, rule after rule, at any depth
// (its "reach"), without growing it. None of them can yet count how many of the space pass, so each
// judges only and the line has no compact mode.
#include "sieve/filter.hpp"
#include "sieve/fractal.hpp"

#include <map>
#include <memory>
#include <optional>
#include <set>
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
    std::map<char, const std::string*> rule;
    for (const auto& [c, w] : f.rules) rule[c] = &w;
    std::vector<char> met;
    std::set<char> seen;
    auto meet = [&](const std::string& w) {
        for (const char c : w)
            if (seen.insert(c).second) met.push_back(c);
    };
    meet(f.axiom);
    for (size_t k = 0; k < met.size(); ++k)
        if (const auto it = rule.find(met[k]); it != rule.end()) meet(*it->second);
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

bool draws(const Fractal& f)
{
    const std::vector<char> m = reach(f);
    if (f.family == Fractal::Family::Substitution) return m.size() >= 2;
    for (const char c : m)
        if (c == 'F' || c == 'G') return true;
    return false;
}

bool grows(const Fractal& f)
{
    const std::vector<char> m = reach(f);
    const std::set<char> met(m.begin(), m.end());
    for (const auto& [c, w] : f.rules)
        if (met.count(c) && w.size() >= 2) return true;
    return false;
}

bool no_idle(const Fractal& f)
{
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

} // namespace

void add_fractal_filters(std::vector<FilterSpec>& out)
{
    out.push_back(judge("fractal-draws",
                        "Draws something: an L-system whose axiom reaches F or G (a line drawn) at some depth; a "
                        "substitution whose axiom reaches two symbols or more (two colours). A ruleset that draws "
                        "nothing is a blank picture at every depth.",
                        draws));
    out.push_back(judge("fractal-grows",
                        "Grows: a symbol the axiom reaches has a rule of two symbols or more, so its word is longer at "
                        "every depth. One that does not is the same length at every depth, a picture that never "
                        "changes size.",
                        grows));
    out.push_back(judge("fractal-no-idle",
                        "No idle rules: no rule for a symbol the axiom never reaches, and no rule that rewrites a "
                        "symbol to itself. Either draws what the ruleset without that rule draws, which is at an "
                        "address of its own.",
                        no_idle));
    out.push_back(judge("fractal-canonical",
                        "One ruleset of each renaming: the symbols whose names change nothing drawn (an L-system's "
                        "letters but F, G and f) or only a colour (every substitution symbol) take the first names "
                        "free in the order the axiom reaches them, and every rule is for a symbol reached. Of the "
                        "charted fractals, the dragon curve (X and Y) is a renaming of A and B.",
                        canonical));
}

} // namespace sieve
