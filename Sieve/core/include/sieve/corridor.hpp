// Sieve — the shared corridor (SPECIFICATIONS §5.1, §7).
//
// All four lines lie along one endless corridor of book slots, numbered by a single signed
// position. Tiles hold kBooksPerTile slots. Each line repeats forever along the corridor: a line
// of N units takes T = ceil(N / kBooksPerTile) tiles, so one copy of it spans T whole tiles, and
// copy c starts at tile c * T. Within a copy, slot i holds the unit whose (raw) address is i;
// the slots from N to T * kBooksPerTile - 1 are empty padding.
//
// Doors keep the position and only change which line reads it, so every door lines up with a
// door in every other line, and stepping back through a door returns you exactly. Different
// line sizes simply repeat at different rates. A line whose size is a multiple of
// kBooksPerTile (any power of two at least that large) fills its tiles exactly, with no padding,
// and when every size is a power of two the loops nest: each line's start line falls on a start
// line of every smaller one.
//
// The start of every copy is marked in the hallway by a checkered start line.
#pragma once

#include "sieve/biguint.hpp"

#include <cstdint>
#include <optional>
#include <string>

namespace sieve {

// Book slots per tile: how the one address of a unit is cut into a corridor coordinate, as
// tile = index >> bits and slot = index & mask. Must be a power of two, so that the split is
// exact and a power-of-two line fills whole tiles.
//
// It is a setting (the setup menu's GLOBAL section), not a constant, because how many units
// stand on a wall is a choice about the corridor and not about the lines. **It does not change
// any address**: the unit's index is what it always was, and only the tile and slot that name
// its place in the corridor differ. A tile number written down under one setting therefore
// means something else under another, which is why the readout and `sieve info` both say which
// is in force.
//
// Set once at start-up, before any LineLoop is built, and never afterwards.
uint32_t books_per_tile();
unsigned books_per_tile_bits();
void set_books_per_tile(uint32_t n); // throws std::invalid_argument unless n is a power of two in 2..4096

// A tile number on the corridor: any integer, as sign and magnitude.
struct TileIndex
{
    bool negative = false; // never true with a zero magnitude
    BigUint magnitude;

    static TileIndex of(int64_t t);
    TileIndex& operator+=(int64_t delta);
    friend TileIndex operator+(TileIndex a, int64_t d) { return a += d; }
    friend bool operator==(const TileIndex& a, const TileIndex& b) { return a.negative == b.negative && a.magnitude == b.magnitude; }
    std::string to_decimal() const;
    static TileIndex parse(std::string_view dec); // optional leading '-'
};

// One line's loop along the corridor.
class LineLoop
{
public:
    explicit LineLoop(BigUint units); // units >= 1

    const BigUint& units() const { return units_; }
    const BigUint& tiles() const { return tiles_; }     // ceil(units / kBooksPerTile)
    uint32_t padding() const { return padding_; }        // empty slots at the end of each copy
    bool fills_whole_tiles() const { return padding_ == 0; }

    // Which tile of the loop corridor tile t shows: t mod tiles, in [0, tiles).
    BigUint loop_tile(const TileIndex& t) const;
    // The address (unit index) in slot k of loop tile lt, or nothing for a padding slot.
    std::optional<BigUint> unit_index(const BigUint& loop_tile, uint32_t slot) const;
    // Position of a unit in the first copy: tile index * kBooksPerTile + slot.
    static TileIndex tile_of(const BigUint& unit_index);
    static uint32_t slot_of(const BigUint& unit_index);

private:
    BigUint units_, tiles_;
    uint32_t padding_ = 0;
};

} // namespace sieve
