// Sieve — the fractal dimension's whole space (list X, X1; IDEAS 19.235): every ruleset the reader
// takes (sieve-fractal-v1, fractal.hpp), each at an address of its own, generated rather than charted
// (Edward, 15 November 2026: "we could / should look at procedurally generating all possible fractals",
// "we could in theory take the ruleset we use and generate the state space according to that").
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

inline constexpr const char* kFractalSpaceVersion = "fractal-space-v1";
inline constexpr uint32_t kFractalSpaceShortest = 44;
inline constexpr uint32_t kFractalSpaceLongestMost = 512; // a longest beyond this is refused (its tables grow with it)
inline constexpr uint32_t kFractalSpaceLongestDefault = 96; // the longest charted ruleset (the Gosper curve) is 82

class FractalSpace
{
public:
    // Every ruleset of 44 to `longest` characters. Throws std::invalid_argument for a longest below
    // 44 or above kFractalSpaceLongestMost.
    explicit FractalSpace(uint32_t longest);
    uint32_t longest() const { return longest_; }
    const BigUint& count() const { return total_; }   // all of them
    BigUint count_at(uint32_t length) const;           // those of exactly `length` characters
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
    static uint32_t header_length(int family_line_chars) { return 17 + uint32_t(family_line_chars) + 1; }
    BigUint at_length(uint32_t length, std::vector<BigUint>* per_angle_block) const;
    uint32_t longest_;
    Family sub_, ls_;
    std::vector<BigUint> by_length_; // [length]: rulesets of exactly that many characters
    BigUint total_;
};

} // namespace sieve
