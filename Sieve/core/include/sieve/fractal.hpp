// Sieve — the fractal dimension's rulesets (IDEAS 19.191, 19.235; list S, step 1): a fractal written as
// a short rule, grown to a depth and drawn. The dimension is a line like the others: the text line over
// ascii96 with the format-fractal-v1 plugin ticked holds every ruleset of this grammar at its address,
// and its shelves show the fractals charted by hand (data/fractals/chart.txt).
//
// "sieve-fractal-v1", a ruleset: lines of text, each ending in a line feed.
//     sieve-fractal-v1
//     substitution              or   lsystem ANGLE     (whole degrees, 1 to 359)
//     axiom WORD
//     rule SYMBOL WORD          (any number, 1 or more: each symbol rewritten once, in the order of
//                                its character code)
// A WORD is 1 to 64 symbols. A substitution's symbols are 0-9 and a-z; at each step every symbol
// with a rule becomes its word, every other one stays. An L-system's symbols are A-Z, a-z and
// + - [ ] |, and only letters have rules; drawn as a turtle draws: F and G a step forward drawing a
// line, f a step forward without one, + a turn of ANGLE to the left, - to the right, | a half turn,
// [ the turtle's place and heading kept, ] the last kept taken back; other letters draw nothing.
//
// Drawing: a substitution as its words one under another, depth 0 at the top, each row stretched to
// the picture's width and each symbol a colour of its own, so its self-similarity is seen as the rows
// repeat; an L-system as the turtle's path at that depth, fitted to the picture. A word is grown only
// up to kFractalMost symbols (a limit chosen, so the deepest a rule may go is said, not run out of
// memory).
#pragma once

#include "sieve/image.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace sieve {

inline constexpr const char* kFractalVersion = "sieve-fractal-v1";
inline constexpr const char* kFractalChartVersion = "sieve-fractal-chart-v1";
inline constexpr size_t kFractalWordMost = 64;           // a word's symbols at most
inline constexpr size_t kFractalMost = size_t(1) << 21;  // symbols a ruleset is grown to at most
inline constexpr uint32_t kFractalDepthMost = 64;        // steps at most

struct Fractal
{
    enum class Family { Substitution, LSystem } family = Family::Substitution;
    uint32_t angle = 0; // an L-system's turn, whole degrees
    std::string axiom;
    std::vector<std::pair<char, std::string>> rules; // in order of symbol

    std::string text() const; // as written above
    // Its word after `depth` steps; nothing if longer than kFractalMost.
    std::optional<std::string> grow(uint32_t depth) const;
    // The deepest it grows to within kFractalMost (at most kFractalDepthMost).
    uint32_t deepest() const;
};
// Reads a ruleset, checked: nothing where it is not written as above, `why` saying what.
std::optional<Fractal> read_fractal(const std::string& text, std::string* why = nullptr);

// The ruleset at `depth` drawn on a picture of width x height (RGBA; a dark ground, lines or tiles in
// colour). Nothing where it does not grow that far within kFractalMost.
std::optional<RgbaImage> draw_fractal(const Fractal& f, uint32_t depth, uint32_t width, uint32_t height);

// An L-system's turtle path at a depth: the lines drawn, as (x1, y1, x2, y2), in turtle steps.
std::vector<std::array<double, 4>> fractal_lines(const Fractal& f, const std::string& word);

// The shelves, charted by hand: "sieve-fractal-chart-v1", then "fractal ID TITLE" a line; each
// ruleset is ID.fractal beside the chart. ID is 1 to 64 of a-z, 0-9 and '-'.
struct ChartedFractal
{
    std::string id, title;
    Fractal fractal;
    std::string text; // the ruleset as its file holds it
};
// Reads the chart in `folder` and every ruleset it names. Throws std::invalid_argument naming the
// first entry that is not as written.
std::vector<ChartedFractal> read_fractal_chart(const std::filesystem::path& folder);

} // namespace sieve
