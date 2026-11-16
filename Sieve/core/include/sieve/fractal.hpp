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
// "sieve-fractal-v2" (list X2) widens the L-system for the fractals v1 cannot write (Edward, 15 November
// 2026: "implement as many fractals as we can track down"; IDEAS 19.235's spider webs):
//     sieve-fractal-v2
//     lsystem ANGLE             or   lsystem ANGLE scale RATIO
//     axiom WORD
//     rule SYMBOL WORD
// ANGLE is whole degrees (1 to 359) or degrees and one tenth (0.1 to 359.9, its tenth not 0: 22.5, 25.7);
// RATIO is 0.001 to 0.999, written with one to three decimals and no 0 at the end (0.5, 0.707). Its
// symbols are v1's, and with a scale also " (the step from here on RATIO times as long) and ' (RATIO
// times as short, so longer); [ keeps the step's length too. A v2 ruleset says something v1 cannot:
// its angle has a tenth, or it has a scale (anything else is written as v1, so each ruleset is written
// one way). Substitutions are v1's alone.
//
// "sieve-fractal-v3" (list X3) widens it again for the fractals v2 cannot write (Edward, 16 November 2026:
// "we should work towards the fractals that cant be written also"). Its lines, in this order:
//     sieve-fractal-v3
//     lsystem ANGLE             (0 < ANGLE < 360, at most 6 decimals, no 0 at the end; or 360/N, N 2-9999)
//     scale RATIO               (optional: 0.d, 1 to 6 decimals, the last not 0; " and ' as v2's)
//     seed N                    (optional, 1 to 4294967295: which of several rules each symbol takes)
//     ignore SYMBOLS            (optional: skipped when a context is matched; in order of code, each once)
//     define D turn DEGREES     (optional, a digit each, in digit order, each once: D turns DEGREES,
//     define D scale FACTOR      -359.999999 to 359.999999, not 0, left if more than 0; or D makes the
//                                step FACTOR times as long, 0 < FACTOR < 1000, not 1; at most 6 decimals)
//     axiom WORD
//     rule [L<]S[>R] WORD       (one or more, in the order of S's code; several for one S allowed)
// S, L and R are letters. A WORD is 1 to 255 symbols: letters, digits and + - [ ] | ! " ' & ^ \ /.
// Drawn as v2, and: ! swaps what + and - turn (kept by [ ]); & and ^ pitch down and up, \ and / roll
// left and right, by ANGLE (the turtle in three dimensions, as The Algorithmic Beauty of Plants
// draws); a digit defined turns or scales, one not defined does nothing. Where a symbol has several
// rules, at each step each place takes one of those that apply, chosen from the seed, the step and the
// place (so the same ruleset grows the same everywhere); a rule with a context (L before S, R after it,
// on its path, skipping the ignored symbols and whole branches) applies only where it matches, and
// where one does, the rules without a context for that symbol are not used. A v3 ruleset may say what
// v1 or v2 says: it is then another text, at an address of its own.
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
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace sieve {

inline constexpr const char* kFractalVersion = "sieve-fractal-v1";
inline constexpr const char* kFractalVersion2 = "sieve-fractal-v2";
inline constexpr const char* kFractalChartVersion = "sieve-fractal-chart-v1";
inline constexpr const char* kFractalChartVersion2 = "sieve-fractal-chart-v2";
inline constexpr const char* kFractalVersion3 = "sieve-fractal-v3";
inline constexpr size_t kFractalWordMost = 64;           // a word's symbols at most (v1, v2)
inline constexpr size_t kFractalWordMostV3 = 255;        // and in v3
inline constexpr size_t kFractalMost = size_t(1) << 21;  // symbols a ruleset is grown to at most
inline constexpr uint32_t kFractalDepthMost = 64;        // steps at most

struct Fractal
{
    enum class Family { Substitution, LSystem } family = Family::Substitution;
    uint32_t version = 1;      // sieve-fractal-v1 or v2
    uint32_t angle_tenths = 0; // an L-system's turn, in tenths of a degree (v1: whole degrees, so a multiple of 10)
    uint32_t scale = 0;        // v2: the step's ratio for " and ', in thousandths (0: none)
    // v3: each number as written (the text is made again from them) and its value.
    std::string angle_text, scale_text;
    double angle_degrees = 0, scale_ratio = 0; // (scale_ratio 0: none)
    uint32_t seed = 0;                         // (0: no seed line)
    std::string ignore;                        // symbols skipped when matching a context
    struct Define
    {
        char digit = '0';
        bool turn = true;  // turn by `value` degrees, or scale the step by it
        std::string text;  // the number as written
        double value = 0;
    };
    std::vector<Define> defines; // in digit order
    std::string axiom;
    std::vector<std::pair<char, std::string>> rules; // in order of symbol (v3: several for one symbol may follow each other)
    std::vector<std::pair<char, char>> contexts;     // v3: each rule's left and right context (0 for none), as rules

    double turn_degrees() const { return version == 3 ? angle_degrees : double(angle_tenths) / 10.0; }
    std::string angle_written() const; // the angle as its text writes it
    std::string scale_written() const; // its scale for " and ' as written ("" for none)

    std::string text() const; // as written above
    // Its word after `depth` steps; nothing if longer than kFractalMost.
    std::optional<std::string> grow(uint32_t depth) const;
    // The deepest it grows to within kFractalMost (at most kFractalDepthMost).
    uint32_t deepest() const;
};
// An angle in tenths of a degree as a ruleset writes it ("90", "22.5"); a ratio in thousandths ("0.707").
std::string fractal_angle_text(uint32_t tenths);
std::string fractal_scale_text(uint32_t thousandths);
// Reads a ruleset (v1 or v2), checked: nothing where it is not written as above, `why` saying what.
std::optional<Fractal> read_fractal(const std::string& text, std::string* why = nullptr);

// The ruleset at `depth` drawn on a picture of width x height (RGBA; a dark ground, lines or tiles in
// colour). Nothing where it does not grow that far within kFractalMost.
std::optional<RgbaImage> draw_fractal(const Fractal& f, uint32_t depth, uint32_t width, uint32_t height);

// An L-system's turtle path at a depth: the lines drawn, as (x1, y1, x2, y2), in turtle steps.
std::vector<std::array<double, 4>> fractal_lines(const Fractal& f, const std::string& word);

// The shelves, charted by hand: "sieve-fractal-chart-v1", then "fractal ID TITLE" a line; each
// ruleset is ID.fractal beside the chart. ID is 1 to 64 of a-z, 0-9 and '-'. "sieve-fractal-chart-v2"
// (list X2) says where each was transcribed from: after each "fractal" line, "source TEXT" (who it is
// after and where it was read: a book's figure, a page; Edward: each charted individually, with its
// attribution).
struct ChartedFractal
{
    std::string id, title;
    std::string source; // where it was transcribed from (v2's charts; "" in a v1 chart)
    Fractal fractal;
    std::string text; // the ruleset as its file holds it
};
// Reads the chart in `folder` and every ruleset it names. Throws std::invalid_argument naming the
// first entry that is not as written.
std::vector<ChartedFractal> read_fractal_chart(const std::filesystem::path& folder);

// An L-system's box-counting dimension, measured on its drawing (list X2; IDEAS 19.235: "a measure of how
// fractal a drawing is ... could rank what is left"): grown as deep as it goes within
// kFractalDimensionSymbols, its lines laid on grids of 4, 8, 16 ... cells a side over the square that holds
// them, down to cells no smaller than its median line (and at most 1024 a side), the cells they touch
// counted, and the slope of log(cells) against log(cells a side) fitted by least squares. A line is 1, a
// filled square 2, the Koch curve about 1.26 (log 4 / log 3). Nothing for a substitution (its rows are a
// sequence, not a figure), or where fewer than three grids fit.
inline constexpr size_t kFractalDimensionSymbols = size_t(1) << 16;
struct FractalDimension
{
    double value = 0;
    uint32_t depth = 0, grids = 0; // the depth drawn, and how many grids the slope was fitted over
};
std::optional<FractalDimension> fractal_box_dimension(const Fractal& f);

// The deepest a ruleset grows to within `symbols` symbols (at most deepest()): how deep the hallway draws
// one, and how deep fractal-drawn-v1 draws the charted ones.
uint32_t fractal_deepest_within(const Fractal& f, size_t symbols);
// The chart the other lines' filters judge by (fractal-drawn-v1): the programs say where it is (the
// fractals folder beside them, or data/fractals), and it is read once. Throws std::invalid_argument
// where no folder was set or the chart cannot be read.
void set_fractal_chart_folder(const std::filesystem::path& folder);
std::shared_ptr<const std::vector<ChartedFractal>> fractal_chart();

} // namespace sieve
