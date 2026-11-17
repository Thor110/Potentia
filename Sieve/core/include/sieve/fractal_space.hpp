// Sieve — the fractal dimension's whole space (list X, X1; IDEAS 19.235): every ruleset the reader
// takes (sieve-fractal-v1, fractal.hpp), each at an address of its own, generated rather than charted
// (Edward, 15 November 2026: "we could / should look at procedurally generating all possible fractals",
// "we could in theory take the ruleset we use and generate the state space according to that").
//
// "fractal-space-v2" (list X2), the numbering: every v1 ruleset first, numbered as fractal-space-v1
// numbered them (so their addresses are as they were, whatever the longest), then every v2 ruleset
// (sieve-fractal-v2: an angle with a tenth, or a scale; their addresses come after every v1 ruleset to
// the longest, so move with it), numbered alike: the shorter first; within a length the L-systems without a
// scale, by the angle as written (its characters, then its value); then those with one, by how many
// characters the angle and then the scale are written in, then the angle's value and the scale's; then
// its axiom and rules as below, with " and ' among the symbols where there is a scale (in the order of
// their character codes).
//
// "fractal-space-v3" (list X3): fractal-space-v2's, then every v3 ruleset (sieve-fractal-v3) of 44 to
// `longest` characters, the shorter first; within a length, by its parts in the order they are written,
// each part's texts counted by their lengths: the first part's shorter texts first, then by its text's
// place among those of its length, then the rest alike (so a ruleset's address is its parts' places
// read as the digits of one number). A part's texts of one length are in this order: a number, its
// sign (none first), then the fewer decimals, then its whole part's value, then its decimals' value; an
// angle, the decimals first, then 360/N; an optional line, absent first, then each of its texts; a word,
// its symbols in the order of their codes, the first the most significant; ignore's symbols, as a
// choice of them in order of code, the earlier choices first; a digit's definition, none, then a turn,
// then a scale; the rules, symbol by symbol from A to z, each symbol's rules in the order written, a
// rule's context left absent first, then each letter.
//
// "fractal-space-v4" (list X4): fractal-space-v3's, then every v4 ruleset (sieve-fractal-v4: an IFS, an
// escape or a converge formula) of `longest` characters or fewer, from the shortest there is (26:
// "escape z"), the shorter first; within a length by its parts as v3's are, and: an IFS before a formula;
// a number that may be 0, 0 first (its sign none); "escape" before "converge"; a formula's texts of one
// length by its first token's kind (+ - * / ^, then abs conj cos exp log sin, then c i z, then a
// number), then that token's length, its place among those, then the rest alike.
//
// "fractal-space-v5" (list Y2): fractal-space-v4's, then every v5 ruleset (sieve-fractal-v5: a cellular
// automaton) of `longest` characters or fewer, from the shortest there is (30: "elementary 0"), the shorter
// first; within a length by its parts as v4's are: an elementary automaton before a life-like one; its rule
// by its value; an absent start before one; a start's row by its 0s and 1s (0 first), the first the most
// significant; a life-like rule's counts as a choice of 0-8 in order, none first, then the earlier choices.
//
// "fractal-space-v6" (list X5): fractal-space-v5's, then every v6 ruleset (sieve-fractal-v6: an attractor, a
// flow or an aggregate) of `longest` characters or fewer, from the shortest there is (29: "aggregate 1"), the
// shorter first; within a length by its parts as v4's are: an attractor before a flow before an aggregate;
// a formula's texts as v4's are, its tokens + - * / ^, then abs cos exp log sin, then x y (and z), then a
// number; an absent start before one; a seed by its value.
//
// "fractal-space-v1", the numbering:
//   - every ruleset of 44 to `longest` characters (44 is the shortest there is: "lsystem 1", an axiom of
//     one symbol and one rule of one); the shorter first;
//   - within a length: its family, substitution first, then the L-systems by angle, 1 to 359; then its
//     axiom, the shorter first, words of one length in the order of their symbols' character codes;
//     then its rules, symbol by symbol in the order of their character codes (substitution: 0-9, a-z;
//     L-system: A-Z, a-z), each symbol with a rule before it without one (as their texts sort), a
//     rule's word the shorter first and then in the order of its symbols. A ruleset has one rule at least.
// So each ruleset the reader takes is at exactly one address and every address is one, the same on
// every machine. format-fractal-v1 (FILTER-PLUGINS) passes these and also their rules in another
// order, which the reader refuses; this space holds the reader's alone. Rulesets that draw the same
// thing (a rule rewriting a symbol to itself, a rule for a symbol never met) are each their own
// address, as two texts are; telling them apart is a filter's work.
#pragma once

#include "sieve/biguint.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace sieve {

inline constexpr const char* kFractalSpaceVersion = "fractal-space-v6";
inline constexpr uint32_t kFractalSpaceShortest = 44;
// A longest beyond this is refused (its tables grow with it: the space is counted in 0.65 s to 256
// characters and 4.0 s to 512, measured 16 November 2026; Voderberg's tiling, charted, is 590).
inline constexpr uint32_t kFractalSpaceLongestMost = 1024;
// Every charted ruleset but four is 256 or fewer, so all but those are on the line at the default (list X3;
// it was 208 in X2, 96 in X1): Voderberg's tiling (590 characters), and X4's Fractint Koch3 (272), the
// Sierpinski carpet as an IFS (331) and Fractint's name in 22 maps (690).
inline constexpr uint32_t kFractalSpaceLongestDefault = 256;

struct Fractal;
struct FractalSpaceV3; // (v3's counting, fractal_space.cpp)

class FractalSpace
{
public:
    // Every ruleset of 44 to `longest` characters (v1's, then v2's). Throws std::invalid_argument for a longest below
    // 44 or above kFractalSpaceLongestMost.
    explicit FractalSpace(uint32_t longest);
    uint32_t longest() const { return longest_; }
    const BigUint& count() const { return total_; }   // all of them
    const BigUint& count_v1() const { return total_v1_; } // v1's, the first of them
    BigUint count_v3() const;                             // v3's
    const BigUint& count_v4() const { return total_v4_; } // v4's
    const BigUint& count_v5() const { return total_v5_; } // v5's
    const BigUint& count_v6() const { return total_v6_; } // v6's, the last of them
    BigUint count_at(uint32_t length) const;           // those of exactly `length` characters (v1 and v2)
    // The ruleset at an address (below count()). Throws std::invalid_argument past the end.
    std::string at(const BigUint& index) const;
    // A ruleset's address: nothing where the reader does not take the text, or it is longer than
    // longest().
    std::optional<BigUint> index_of(const std::string& text) const;

private:
    // One family's counting: its word symbols and rule symbols, both in order of character code, and
    // ways[j][r], the ways rules for symbols j onwards make exactly r characters (none among them too).
    struct Family
    {
        std::string symbols, rule_symbols;
        std::vector<BigUint> words;              // words[n]: words of n symbols (n = 0 .. 64)
        std::vector<std::vector<BigUint>> ways;  // [rule_symbols.size() + 1][longest + 1]
        BigUint rules(size_t j, uint32_t r, bool any) const; // as ways, one rule at least unless `any`
        BigUint after_header(uint32_t r) const;  // an axiom and its rules in exactly r characters
    };
    // v2's L-system lines of one shape: angles of one width (and scales of one width), each line's
    // rulesets of one length as many as every other's in it.
    struct Group
    {
        const Family* family;
        uint32_t header;                      // the version and family lines' characters
        const std::vector<uint32_t>* angles;  // in tenths, in order
        const std::vector<uint32_t>* scales;  // in thousandths, in order; nullptr for none
        uint64_t lines() const { return angles->size() * (scales ? scales->size() : 1); }
    };
    BigUint at_length(uint32_t length, std::vector<BigUint>* per_angle_block) const;
    BigUint at_length_v2(uint32_t length) const;
    // A ruleset's axiom and rules from its place among those of its family line (r characters left).
    std::string body_at(const Family& f, uint32_t r, BigUint i) const;
    BigUint body_index(const Family& f, uint32_t r, const Fractal& fr) const;
    uint32_t longest_;
    Family sub_, ls_, scaled_;
    std::vector<uint32_t> angles_[6], tenths_[6], scales_[6]; // by width: every angle, those with a tenth, the scales
    std::vector<Group> groups_;      // v2's, in order
    std::vector<BigUint> by_length_; // [length]: v1 rulesets of exactly that many characters
    std::vector<BigUint> by_length_v2_;
    std::shared_ptr<const FractalSpaceV3> v3_, v4_, v5_, v6_; // (v4's to v6's counted by the same parts)
    BigUint total_v1_, total_v2_, total_v3_, total_v4_, total_v5_, total_v6_, total_;
};

} // namespace sieve
