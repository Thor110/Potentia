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
#include <optional>
#include <string>
#include <vector>

namespace sieve {

inline constexpr const char* kFractalSpaceVersion = "fractal-space-v2";
inline constexpr uint32_t kFractalSpaceShortest = 44;
inline constexpr uint32_t kFractalSpaceLongestMost = 512; // a longest beyond this is refused (its tables grow with it)
// The longest charted ruleset (Penrose's kites and darts) is 201 characters, so every charted one is on
// the line at the default (list X2, 16 November 2026; it was 96 in X1, the Gosper curve's 82 the longest).
inline constexpr uint32_t kFractalSpaceLongestDefault = 208;

struct Fractal;

class FractalSpace
{
public:
    // Every ruleset of 44 to `longest` characters (v1's, then v2's). Throws std::invalid_argument for a longest below
    // 44 or above kFractalSpaceLongestMost.
    explicit FractalSpace(uint32_t longest);
    uint32_t longest() const { return longest_; }
    const BigUint& count() const { return total_; }   // all of them
    const BigUint& count_v1() const { return total_v1_; } // v1's, the first of them
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
    BigUint total_v1_, total_;
};

} // namespace sieve
