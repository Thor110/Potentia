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
#include <memory>
#include <optional>
#include <string>
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

// ---- The survivors numbered (list F, F-B; IDEAS 19.250) ----
//
// Compact needs more than how many pass: the k-th of them, and a survivor's number. The counting above does
// not give them (a difference of counts has no order), and the line's own order (the text, left to right)
// cannot be counted with part of a ruleset fixed, as a symbol's reach waits on rules written after it. So
// the survivors are numbered in an order of their own, in which counting with part of a body fixed is what
// the counting by sets already does:
//   - by the set S the axiom reaches: its size s, then how many of it are F or G and f, then which symbols
//     (under canonical, its renamable symbols are the first names: one set for each size);
//   - then the characters the reached symbols' lines take (the axiom's and S's rules), the rest being the
//     rules of the symbols never reached;
//   - then the reached part, in the order the axiom reaches it: the axiom's symbols left to right, then each
//     reached symbol's rules in the order met (as filters/fractals.cpp's reach walks them), each choice in
//     turn (a word ended first, then each symbol in order of code; a rule's contexts, none first, then each
//     letter; no further rule first);
//   - then the rules of the symbols never reached, symbol by symbol in order of code, the fewer characters
//     first, each rule by its contexts, then its word's length and its word.
// The count of what can follow a choice in the reached part is, as in the counting by sets, a sum over
// which of S's symbols not yet met the rest reaches (by inclusion and exclusion, the met ones and the
// queue's alike): from a table of the rules' counts for that S (about (s + 1)(s + 2) / 2 rows of the
// body's length, for each regime: tens of MB at s = 52 and 230 characters), worked out once for a set's
// size and kept while it is in use.

// The symbols of a family's bodies.
struct FractalBodyAlphabet
{
    std::string symbols;           // every symbol a word holds, in order of code
    std::string ruled;             // those with rules, in order of code (the rest have none)
    std::string drawing, fixed;    // an L-system's F and G, and f (empty for a substitution)
    bool substitution = false;
    uint32_t word_most = 64;
    std::string context;           // v3: the letters a context names (empty: no contexts)
    bool many_rules = false;       // v3: any number of rules a symbol
    FractalBodyKinds kinds() const;
};
FractalBodyAlphabet fractal_substitution_alphabet();
FractalBodyAlphabet fractal_lsystem_alphabet();
FractalBodyAlphabet fractal_scaled_alphabet();
FractalBodyAlphabet fractal_v3_alphabet();

// A body: its axiom and its rules in the order written (by symbol, a symbol's rules in their order).
struct FractalBody
{
    struct Rule
    {
        char symbol = 0, left = 0, right = 0; // (left and right: v3's contexts, 0 for none)
        std::string word;
    };
    std::string axiom;
    std::vector<Rule> rules;
    uint32_t length() const; // its characters, as the space writes them
    std::string text() const; // "axiom ...\n" and "rule ...\n" lines
};

class FractalBodyNumbering
{
public:
    // The bodies of 0 to `top` characters that pass the ticked filters (at least one), numbered.
    FractalBodyNumbering(FractalBodyAlphabet alphabet, uint32_t top, const FractalStructureFilters& filters);
    ~FractalBodyNumbering();
    FractalBodyNumbering(const FractalBodyNumbering&) = delete;
    FractalBodyNumbering& operator=(const FractalBodyNumbering&) = delete;
    uint32_t top() const;
    const FractalBodyAlphabet& alphabet() const;
    // How many of exactly r characters pass (fractal_bodies_passing's figure).
    const BigUint& count(uint32_t r) const;
    // The k-th of those of r characters (k below count(r); throws std::invalid_argument past it).
    FractalBody at(uint32_t r, BigUint k) const;
    // A body's number among those of its length: nothing if it does not pass, or is not of this alphabet.
    std::optional<BigUint> index(const FractalBody& body) const;

    struct Impl; // (named here so that fractal_count.cpp's walk can name it; defined there)

private:
    std::unique_ptr<Impl> impl_;
};

} // namespace sieve
