// Sieve — how many of the fractal dimension's rulesets pass its structural filters (list F, IDEAS 19.250):
// fractal-draws, fractal-grows, fractal-no-idle and fractal-canonical, counted exactly on the rulesets of
// sieve-fractal-v1 and v2 (an axiom and at most one rule a symbol) and v3 (any number of rules a symbol, with
// contexts; its header's lines, which reach nothing, counted apart) without growing any of them.
//
// Each of those filters asks about the symbols the axiom reaches, rule after rule (its reach,
// filters/fractals.cpp): whether F or G is among them, whether one of them has a rule of two symbols or more,
// whether every rule is for one of them, whether they are named in the order met. A ruleset's reach is not
// read off its text from left to right, so these are counted by the reach itself:
//   - Every body (axiom and rules) has one reach, a set S of the symbols that can have a rule. Those in S have
//     words of S and the symbols that cannot have a rule (+ - [ ] |, and v2's " and '); the rest of the
//     symbols are never reached, and their rules, if any, may be anything (or, under no-idle and canonical,
//     must not be there).
//   - The bodies of a set S of s symbols, every one reached (inner(s)), are found from all the bodies on S:
//     each reaches some U within S, its other symbols unreached with any rule over S, so
//         all(s) = sum over u <= s of C(s, u) inner(u) rule_s^(s - u),
//     and inner(s) is all(s) less the u < s terms (each a count, so the subtraction never goes below 0).
//   - Which s symbols S is matters only by their kinds (drawing F and G, f, and the 49 letters whose names
//     change nothing drawn, or every substitution symbol), so a reach of s symbols stands for C(R, s) sets of
//     them; draws keeps those holding F or G (a substitution, two symbols or more); canonical keeps, of each,
//     the one whose renamable symbols are the first names in the order met, inner(s) / m! where m of the s
//     are renamable (a renaming of them changes that order, so every order is as common as every other).
//   - Grows is everything less the bodies whose reached symbols have rules of one symbol at most.
// Checked against every ruleset of the smallest lengths, read and judged by the filters themselves
// (tests), and against an enumeration of every body over small alphabets (the prototype the list records).
#pragma once

#include "sieve/biguint.hpp"

#include <cstdint>
#include <vector>

namespace sieve {

// A v1 or v2 family's symbols by what they are to the filters.
struct FractalBodyKinds
{
    uint32_t inert = 0;     // symbols with no rule: + - [ ] |, and a scaled line's " and '
    uint32_t drawing = 0;   // F and G (an L-system)
    uint32_t fixed = 0;     // f: a rule, but its name matters (an L-system)
    uint32_t renamable = 0; // the rest: their names change nothing drawn (or, a substitution's, only a colour)
    bool substitution = false; // draws means two symbols or more reached, not F or G
    uint32_t word_most = 64;   // a word's symbols at most (kFractalWordMost; v3's kFractalWordMostV3)
    uint32_t context_letters = 0; // v3: a rule's context letter either side, or none (0: no contexts)
    bool many_rules = false;      // v3: any number of rules for a symbol, in the order written (v1, v2: one)
    uint32_t ruled() const { return drawing + fixed + renamable; }
    uint32_t alphabet() const { return inert + ruled(); }
};

// Which of the four are ticked.
struct FractalStructureFilters
{
    bool draws = false, grows = false, no_idle = false, canonical = false;
    bool any() const { return draws || grows || no_idle || canonical; }
};

// [r]: the bodies (axiom line and rule lines, as the space writes them) of exactly r characters that the
// reader takes (one rule at least) and that pass the ticked filters, r from 0 to top.
std::vector<BigUint> fractal_bodies_passing(const FractalBodyKinds& kinds, uint32_t top, const FractalStructureFilters& filters);

// The families as v1 and v2 write them.
FractalBodyKinds fractal_substitution_kinds();
FractalBodyKinds fractal_lsystem_kinds();
FractalBodyKinds fractal_scaled_kinds();
FractalBodyKinds fractal_v3_kinds();

} // namespace sieve
