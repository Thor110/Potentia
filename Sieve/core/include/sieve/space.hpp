// Sieve — a text line's address space in raw mode (SPECIFICATIONS §3, §4.1).
//
// A unit is UNIT_LENGTH symbols, stored as digits (most-significant first, digit 0 = SPACE).
//   Positional address: the unit read as a base-|ALPHABET| number.
//   Scrambled address:  the unit passed through a keyed, invertible digit-Feistel permutation,
//                       then read as a base-|ALPHABET| number.
// Addresses are written as zero-padded lowercase hex of fixed width. Every address below
// |ALPHABET|^UNIT_LENGTH decodes to exactly one unit; every unit has exactly one address.
//
// A space of a range of lengths (list O1; Edward, 19 November 2026: pages of a minimum to a maximum length): every
// unit of MIN to MAX symbols, the shorter first, as the binary line holds its files. A unit's place is the units
// shorter than it, (B^L - B^MIN) / (B - 1), plus its digits read as a base-B number; the positional address is that
// place and the scrambled one shuffle-sha256-v1 of it over the whole range (keyed with the key, its domain the
// space's id), as the binary line's. An address is a number below the size, written as MAX + 1 base-B digits (the
// address digits), so a step along the loop adds modulo the size. With MIN = MAX the space is the fixed one above,
// every address as it was.
#pragma once

#include "sieve/alphabet.hpp"
#include "sieve/biguint.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace sieve {

class Shuffle; // (compact.hpp: a range's scrambled order)

enum class AddressMode { Positional, Scrambled };

const char* to_string(AddressMode m);
AddressMode address_mode_from_string(std::string_view s); // "positional" | "scrambled"

// Version tag of the scramble construction. Changing the construction changes every
// scrambled address, so it is versioned like a model.
inline constexpr const char* kScrambleVersion = "feistel-sha256-v1";
inline constexpr uint32_t kScrambleRounds = 8;

// The units of `min_length` up to `length` - 1 symbols over `base` symbols, in closed form:
// (B^length - B^min) / (B - 1); none when `length` is `min_length` or less.
BigUint units_shorter(uint32_t base, uint32_t min_length, uint32_t length);
// The length of the unit at a place in positional order of a range of lengths (the place below
// the range's units): the longest length whose shorter units do not pass the place, found by
// halving the range (units_shorter grows with the length).
uint32_t length_at_place(uint32_t base, uint32_t min_length, uint32_t max_length, const BigUint& place);

class Space
{
public:
    using Digits = std::vector<uint32_t>;

    // A text space over an alphabet.
    Space(const Alphabet& alphabet, uint32_t unit_length, std::string key = "sieve");
    // A space over any symbol set, identified by name and size (images, audio, video).
    Space(std::string symbols_id, uint32_t base, uint32_t unit_length, std::string key = "sieve");
    // A text space of a range of lengths, min_length to max_length (1 <= min <= max); min = max is the space above.
    Space(const Alphabet& alphabet, uint32_t min_length, uint32_t max_length, std::string key);
    ~Space();
    Space(const Space& o);
    Space& operator=(const Space& o);
    Space(Space&&) noexcept;
    Space& operator=(Space&&) noexcept;

    // Only valid for text spaces; throws std::logic_error otherwise.
    const Alphabet& alphabet() const;
    bool is_text() const { return alphabet_ != nullptr; }
    uint32_t base() const { return base_; }
    const std::string& symbols_id() const { return symbols_id_; }
    uint32_t unit_length() const { return length_; } // the longest unit (a range's maximum)
    uint32_t min_length() const { return min_length_; }
    bool is_range() const { return min_length_ < length_; }
    // Digits an address is written in: the unit length, or a range's maximum plus one.
    uint32_t address_length() const { return is_range() ? length_ + 1 : length_; }
    // A unit's place in positional order (any length in the range), and the unit at a place (below size()).
    BigUint place_of(const Digits& unit) const;
    Digits unit_at_place(const BigUint& place) const;
    // The units shorter than `length` (from min_length on): (B^length - B^min) / (B - 1).
    BigUint count_shorter(uint32_t length) const { return units_shorter(base_, min_length_, length); }
    const std::string& key() const { return key_; }

    // |ALPHABET|^UNIT_LENGTH, exact.
    const BigUint& size() const { return size_; }
    // Bits needed to write the largest address, and the matching hex width.
    size_t address_bits() const { return bits_; }
    size_t hex_width() const { return hex_width_; }
    // Parameter-set identifier, e.g. "lower27/L32/key=sieve/feistel-sha256-v1".
    // The address map depends only on base, length and key; the id also names the symbol set.
    std::string id() const;

    // Unit text <-> digits. Text must be exactly UNIT_LENGTH symbols of the alphabet (on a range, any length in it).
    Digits digits_of(std::u32string_view unit) const;
    std::u32string text_of(const Digits& unit) const;

    // Scramble maps unit digits to address digits; unscramble is its exact inverse. One length only (a range's
    // scrambled order is address_digits').
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
    uint32_t min_length_;
    std::unique_ptr<Shuffle> shuffle_; // a range's scrambled order (null for one length)
    void check_unit(const Digits& unit) const;
    std::string key_;
    BigUint size_;
    size_t bits_;
    size_t hex_width_;
};

// The address at a fraction p / 10^decimals of the way along a space (for "go to 36.5%").
Space::Digits unit_at_fraction(const Space& space, uint64_t numerator, uint32_t decimals, AddressMode mode);

} // namespace sieve
