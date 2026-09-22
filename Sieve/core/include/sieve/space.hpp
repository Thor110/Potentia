// Sieve — a text line's address space in raw mode (SPECIFICATIONS §3, §4.1).
//
// A unit is UNIT_LENGTH symbols, stored as digits (most-significant first, digit 0 = SPACE).
//   Positional address: the unit read as a base-|ALPHABET| number.
//   Scrambled address:  the unit passed through a keyed, invertible digit-Feistel permutation,
//                       then read as a base-|ALPHABET| number.
// Addresses are written as zero-padded lowercase hex of fixed width. Every address below
// |ALPHABET|^UNIT_LENGTH decodes to exactly one unit; every unit has exactly one address.
#pragma once

#include "sieve/alphabet.hpp"
#include "sieve/biguint.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace sieve {

enum class AddressMode { Positional, Scrambled };

const char* to_string(AddressMode m);
AddressMode address_mode_from_string(std::string_view s); // "positional" | "scrambled"

// Version tag of the scramble construction. Changing the construction changes every
// scrambled address, so it is versioned like a model.
inline constexpr const char* kScrambleVersion = "feistel-sha256-v1";
inline constexpr uint32_t kScrambleRounds = 8;

class Space
{
public:
    using Digits = std::vector<uint32_t>;

    // A text space over an alphabet.
    Space(const Alphabet& alphabet, uint32_t unit_length, std::string key = "sieve");
    // A space over any symbol set, identified by name and size (images, audio, video).
    Space(std::string symbols_id, uint32_t base, uint32_t unit_length, std::string key = "sieve");

    // Only valid for text spaces; throws std::logic_error otherwise.
    const Alphabet& alphabet() const;
    bool is_text() const { return alphabet_ != nullptr; }
    uint32_t base() const { return base_; }
    const std::string& symbols_id() const { return symbols_id_; }
    uint32_t unit_length() const { return length_; }
    const std::string& key() const { return key_; }

    // |ALPHABET|^UNIT_LENGTH, exact.
    const BigUint& size() const { return size_; }
    // Bits needed to write the largest address, and the matching hex width.
    size_t address_bits() const { return bits_; }
    size_t hex_width() const { return hex_width_; }
    // Parameter-set identifier, e.g. "lower27/L32/key=sieve/feistel-sha256-v1".
    // The address map depends only on base, length and key; the id also names the symbol set.
    std::string id() const;

    // Unit text <-> digits. Text must be exactly UNIT_LENGTH symbols of the alphabet.
    Digits digits_of(std::u32string_view unit) const;
    std::u32string text_of(const Digits& unit) const;

    // Scramble maps unit digits to address digits; unscramble is its exact inverse.
    Digits scramble(Digits unit) const;
    Digits unscramble(Digits address) const;

    // The address as digits (the unit itself in positional order, its scramble otherwise), and back.
    // Callers that need both the hex address and the fraction should compute these once.
    Digits address_digits(const Digits& unit, AddressMode mode) const;
    Digits unit_of_address(Digits address, AddressMode mode) const;
    std::string hex_of(const Digits& address) const;    // zero-padded hex of address digits
    double fraction_of(const Digits& address) const;    // DISPLAY ONLY, see fraction()
    // Address digits `offset` places along the loop (pure digit arithmetic, no scrambling).
    Digits step_address(Digits address, int64_t offset) const;
    // Parses a hex address into address digits (range-checked; no unscrambling).
    Digits parse_address(std::string_view hex) const;

    // Unit -> hex address, and back. unit_at throws std::out_of_range for addresses
    // outside the space and std::invalid_argument for malformed hex.
    std::string address_of(const Digits& unit, AddressMode mode) const;
    Digits unit_at(std::string_view hex, AddressMode mode) const;

    // The unit `offset` places away along the line (negative = backwards), in the given
    // ordering. The line is a loop: stepping past the last address wraps to the first.
    // Walking the hallway is repeated neighbour() calls; the offset is exact at any length.
    Digits neighbour(const Digits& unit, AddressMode mode, int64_t offset) const;

    // Fractional position along the line, in [0, 1). DISPLAY ONLY — never used in the
    // address path (§4.3). Computed from the leading address digits.
    double fraction(const Digits& unit, AddressMode mode) const;

private:
    Digits round_function(uint32_t round, const uint32_t* src, size_t src_len, size_t count) const;

    const Alphabet* alphabet_ = nullptr;
    std::string symbols_id_;
    uint32_t base_;
    uint32_t length_;
    std::string key_;
    BigUint size_;
    size_t bits_;
    size_t hex_width_;
};

// Door mapping (SPECIFICATIONS §7.1). Returns the unit of `to` whose address (in `mode`) sits at
// the same fractional position as `unit`'s address in `from`:  b = floor(a * M / N), where
// N = |from| and M = |to|. Exact: both sizes are powers of small bases, so the product and quotient
// are done digit by digit with no rounding. When both spaces have the same size it is a bijection.
Space::Digits door_map(const Space& from, const Space::Digits& unit, AddressMode mode, const Space& to);

// The address at a fraction p / 10^decimals of the way along a space (for "go to 36.5%").
Space::Digits unit_at_fraction(const Space& space, uint64_t numerator, uint32_t decimals, AddressMode mode);

} // namespace sieve
