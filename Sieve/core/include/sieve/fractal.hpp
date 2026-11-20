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
// "sieve-fractal-v4" (list X4) adds the families beyond the L-system, the fractals no rewriting draws
// (Edward, 16 November 2026: "we should seek to include every possible fractal ever found, known or
// even theorised"). Its second line names the family:
//     sieve-fractal-v4
//     ifs                       an iterated function system: one or more lines
//     map A B C D E F           each a map of the plane, (x, y) to (A x + B y + E, C x + D y + F)
// or
//     sieve-fractal-v4
//     escape FORMULA            or   converge FORMULA
//     julia RE IM               (optional)
//     view X Y SPAN             (optional: the picture's centre and the width its shorter side shows;
//                                without it 0 0 4)
// A NUMBER is a decimal: an optional -, a whole part of 0 to 999 with no leading zero, and up to 6
// decimals, the last not 0; never -0. A map's numbers and julia's and view's centre may be 0; SPAN is
// more than 0. A FORMULA is z's next value, written in prefix (Polish) notation, its tokens one space
// apart: + - * / ^ take two (the first the left); abs (each part's absolute value, as the Burning Ship
// takes), conj (the conjugate), cos, exp, log (its principal value) and sin take one; z, c, i and a NUMBER
// (not 0) take none. Read from the left, never more than 8 values are awaited at once (a bound chosen:
// the formulas charted await 4 at most). escape: without julia, c is the point and z starts at 0; with
// it, c is julia's RE + IM i and z starts at the point; a point has escaped once |z| passes 1000.
// converge: the same, and a point has settled once z moves less than 10^-9 in a step (Newton's method
// written out, as z - (z^3 - 1) / (3 z^2), settles on a root). The maps of an IFS in another order, or
// the same formula written another way, are other texts at addresses of their own.
//
// "sieve-fractal-v5" (list Y2) adds cellular automata, as a family of the fractal dimension and not a
// dimension of their own (Edward, 16 November 2026: "perhaps cellular automata should be considered fractals
// instead", "I agree, we just need to make sure it is documented somewhere as well"; IDEAS 19.239): a rule
// applied generation after generation, as a ruleset is depth after depth. Its second line names the family:
//     sieve-fractal-v5
//     elementary RULE           Wolfram's numbering, 0 to 255: a row of cells, each 0 or 1, and each
//     start WORD                 cell's next its rule's bit at (left x 4 + itself x 2 + right) of the row
//                                before; START (optional) the first row, 1 to 64 of 0 and 1, "1" without it
// or
//     sieve-fractal-v5
//     life BIRTH/SURVIVAL       a plane of cells: B then the counts of live neighbours (of the eight
//     start ROW/ROW/...          round it) a dead cell comes alive at, / S and those a live one stays alive
//                                at, each 0 to 8 in order, each once, either none (B3/S23 is Conway's Game
//                                of Life); START the first field, its rows top first, each 1 to 64 of 0
//                                and 1, the shorter ones 0 to the right
// "sieve-fractal-v6" (list X5) adds the strange attractors, the flows and the aggregates: a point moved by a
// map again and again, a point carried along by three equations, and particles that wander until they stick.
//     sieve-fractal-v6
//     attractor                 a map of the plane:
//     x FORMULA                 x's next value, and
//     y FORMULA                 y's, each from x and y (Henon's: x <- 1 - 1.4 x^2 + y, y <- 0.3 x)
//     from X Y                  (optional: where it starts; 0 0 without it)
// or
//     sieve-fractal-v6
//     flow STEP                 three equations, how fast x, y and z change, followed STEP at a time (by
//     x FORMULA                  the classical Runge-Kutta rule, four looks a step):
//     y FORMULA                  Lorenz's, 10 (y - x), x (28 - z) - y, x y - 8/3 z
//     z FORMULA
//     from X Y Z                (optional: where it starts; 1 1 1 without it)
// or
//     sieve-fractal-v6
//     aggregate SEED            diffusion-limited aggregation (1 to 4294967295): particles let go far off
//                                one at a time, each wandering a cell at a time until it touches the
//                                cluster, which grows from one cell; where and how each wanders, from SEED
// A FORMULA is a v4 formula's form over real numbers: + - * / ^ take two, abs cos exp log sin one, and x, y
// (and in a flow z) and a NUMBER (not 0) none; never more than 8 values awaited. STEP and the start's
// numbers are v4 NUMBERs (STEP more than 0). A value that is not a finite number ends the path there.
//
// Both are on an endless field of 0 beyond what is written (whatever the rule makes of an all-0 or all-1
// neighbourhood the field beyond becomes, so a rule that wakes empty space, as rule 1 or B0, wakes all of
// it), and grow a cell each way a generation.
//
// Depth: an IFS's at depth d is its first map's fixed point (where the map shrinks, so it is on the
// figure; (0, 0) where there is none) with every sequence of up to d maps applied to it (1 + n + ... + n^d
// points for n maps); an escape or converge picture's at depth d is each point followed for at most
// 4 x 2^d steps (fractal_iterations); an automaton's is its generation d, its first row or field depth 0
// (to kFractalGenerationsMost); an attractor's, 1024 x 2^d points of its path (the first 64 left out, so it
// has settled), a flow's 1024 x 2^d steps, an aggregate's 16 x 2^d particles. So depth is time for every family: its pictures at depths 0, 1, 2 ... are
// the frames of it growing.
//
// Drawing: a substitution as its words one under another, depth 0 at the top, each row stretched to
// the picture's width and each symbol a colour of its own, so its self-similarity is seen as the rows
// repeat; an L-system as the turtle's path at that depth, fitted to the picture; an elementary automaton
// as its rows one under another, generation 0 at the top, so the picture is its history (rule 90 from one
// cell is Sierpinski's triangle); a life-like one as its field at that generation, square cells fitted; an
// attractor as its points and a flow as its path (seen from the side three-dimensional L-systems are),
// each coloured along it, fitted; an aggregate as its cells, coloured by when each stuck. A word is grown only
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
inline constexpr const char* kFractalVersion4 = "sieve-fractal-v4";
inline constexpr const char* kFractalVersion5 = "sieve-fractal-v5";
inline constexpr uint32_t kFractalGenerationsMost = 1024; // an automaton's depth at most
inline constexpr const char* kFractalVersion6 = "sieve-fractal-v6";
inline constexpr uint32_t kFractalPathDeepest = 12;      // an attractor's or a flow's depth at most
inline constexpr uint32_t kFractalAggregateDeepest = 10; // an aggregate's
inline constexpr uint32_t kFractalAwaitedMost = 8;      // a v4 formula's values awaited at once, at most
inline constexpr uint32_t kFractalEscapeDeepest = 10;   // an escape or converge picture's depth at most
inline constexpr double kFractalEscapeRadius = 1000;    // |z| past this has escaped
inline constexpr double kFractalSettled = 1e-9;         // a step shorter than this has settled
inline constexpr size_t kFractalWordMost = 64;           // a word's symbols at most (v1, v2)
inline constexpr size_t kFractalWordMostV3 = 255;        // and in v3
inline constexpr size_t kFractalMost = size_t(1) << 21;  // symbols a ruleset is grown to at most
inline constexpr uint32_t kFractalDepthMost = 64;        // steps at most

struct Fractal
{
    enum class Family { Substitution, LSystem, Ifs, Escape, Converge, Elementary, Life, Attractor, Flow, Aggregate } family = Family::Substitution;
    uint32_t version = 1;      // sieve-fractal-v1 to v6
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
    // v4: an IFS's maps; an escape or converge formula's tokens; julia's and view's numbers. Each
    // number as written and its value.
    struct Map
    {
        std::array<std::string, 6> text;
        std::array<double, 6> value{};
    };
    std::vector<Map> maps;
    std::vector<std::string> formula;
    bool julia = false, view = false;
    std::array<std::string, 2> julia_text;
    std::array<double, 2> julia_value{};
    std::array<std::string, 3> view_text;
    std::array<double, 3> view_value{0, 0, 4};

    // v5: an elementary automaton's rule (0 to 255); a life-like one's birth and survival counts (bit k
    // for k neighbours) and their text; the start as written ("" for none: one live cell) and its rows.
    uint32_t rule_number = 0;
    uint16_t birth = 0, survival = 0;
    std::string start;
    std::vector<std::string> start_rows;
    std::string life_rule_text() const;
    // v6: an attractor's or a flow's formulas (x, y and a flow's z), its start and a flow's step, each
    // number as written and its value; an aggregate's seed is `seed`.
    std::array<std::vector<std::string>, 3> path_formula;
    bool from = false;
    std::array<std::string, 3> from_text;
    std::array<double, 3> from_value{};
    std::string step_text;
    double step_value = 0;

    std::string view_value_text() const { return view_text[0].empty() ? "0 0 4" : view_text[0] + " " + view_text[1] + " " + view_text[2]; }
    // A substitution or L-system: grown as a word (the others are not).
    bool rewrites() const { return family == Family::Substitution || family == Family::LSystem; }
    bool is_formula() const { return family == Family::Escape || family == Family::Converge; }
    bool is_automaton() const { return family == Family::Elementary || family == Family::Life; }
    bool is_path() const { return family == Family::Attractor || family == Family::Flow; }

    double turn_degrees() const { return version == 3 ? angle_degrees : double(angle_tenths) / 10.0; }
    std::string angle_written() const; // the angle as its text writes it
    std::string scale_written() const; // its scale for " and ' as written ("" for none)

    std::string text() const; // as written above
    // Its word after `depth` steps; nothing if longer than kFractalMost, or for a v4 family (not grown).
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
// The same in three dimensions, as (x1, y1, z1, x2, y2, z2): a v3 ruleset's turtle as it moves, not seen
// from a side; v1's and v2's in the plane z = 0 (the models line's fractals, fractal-modelled-v1).
std::vector<std::array<double, 6>> fractal_lines3(const Fractal& f, const std::string& word);
// An IFS's points at a depth, as (x, y, the last map applied); nothing past kFractalMost points.
std::optional<std::vector<std::array<double, 3>>> fractal_points(const Fractal& f, uint32_t depth);
// The steps an escape or converge picture follows a point for at a depth: 4 x 2^depth.
inline uint32_t fractal_iterations(uint32_t depth) { return uint32_t(4) << depth; }
// An automaton's cells at a generation, row by row, each 0 or 1, on the window it has grown to (its start's
// size and a cell more each way a generation); an elementary one's every generation to that one, one row
// each. An aggregate's at a depth: its cells, on the square that holds them, and when each stuck (`when`,
// from 1). Nothing for another family, or past its deepest.
struct AutomatonCells
{
    uint32_t width = 0, height = 0;
    std::vector<uint8_t> cells; // [y * width + x]
    uint8_t beyond = 0;         // the field outside the window
    std::vector<uint32_t> when; // an aggregate's: the particle that filled each cell (0: none)
};
std::optional<AutomatonCells> fractal_cells(const Fractal& f, uint32_t depth);
// fractal_cells at each generation from `from` to `to`, a life-like automaton grown once for them all (nothing
// where fractal_cells gives nothing at one of them).
std::optional<std::vector<AutomatonCells>> fractal_cells_from(const Fractal& f, uint32_t from, uint32_t to);
// An attractor's or a flow's path at a depth: its points, as (x, y, z; z 0 for an attractor), up to the first
// that is not a finite number. Nothing for another family or past kFractalPathDeepest.
std::optional<std::vector<std::array<double, 3>>> fractal_path(const Fractal& f, uint32_t depth);

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

// An L-system's or an IFS's box-counting dimension, measured on its drawing (list X2; IDEAS 19.235: "a measure of how
// fractal a drawing is ... could rank what is left"): grown as deep as it goes within
// kFractalDimensionSymbols, its lines laid on grids of 4, 8, 16 ... cells a side over the square that holds
// them, down to cells no smaller than its median line (and at most 1024 a side), the cells they touch
// counted, and the slope of log(cells) against log(cells a side) fitted by least squares. A line is 1, a
// filled square 2, the Koch curve about 1.26 (log 4 / log 3). Nothing for a substitution (its rows are a
// sequence, not a figure), nor for an escape or converge picture (not measured), or where fewer than three
// grids fit. An IFS's points (as many as kFractalDimensionPoints: its points crowd where its maps shrink
// least, so it takes more than a path's lines) are laid on grids alike, down to the one where the cells
// they touch pass a quarter of the points.
inline constexpr size_t kFractalDimensionSymbols = size_t(1) << 16;
inline constexpr size_t kFractalDimensionPoints = size_t(1) << 20;
struct FractalDimension
{
    double value = 0;
    uint32_t depth = 0, grids = 0; // the depth drawn, and how many grids the slope was fitted over
};
std::optional<FractalDimension> fractal_box_dimension(const Fractal& f);

// The deepest a ruleset grows to within `symbols` symbols (at most deepest()): how deep the hallway draws
// one, and how deep fractal-drawn-v2 draws the charted ones.
uint32_t fractal_deepest_within(const Fractal& f, size_t symbols);
// The chart the other lines' filters judge by (fractal-drawn-v2): the programs say where it is (the
// fractals folder beside them, or data/fractals), and it is read once. Throws std::invalid_argument
// where no folder was set or the chart cannot be read.
void set_fractal_chart_folder(const std::filesystem::path& folder);
std::shared_ptr<const std::vector<ChartedFractal>> fractal_chart();

} // namespace sieve
