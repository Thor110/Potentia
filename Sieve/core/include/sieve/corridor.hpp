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
// Set once at start-up, before any LineLoop is built, and never afterwards -- and that is
// enforced, not merely asked for: a LineLoop divides its length by the tile size when it is built
// and keeps the answer, so a later change would leave its tile count and its slots measured in
// different sizes. The first LineLoop built therefore latches the setting.
uint32_t books_per_tile();
unsigned books_per_tile_bits();
// Throws std::invalid_argument unless n is a power of two in 2..4096, and std::logic_error if it
// would change the size after a line has already been built against it.
void set_books_per_tile(uint32_t n);

// A tile number on the corridor: any integer, as sign and magnitude.
struct TileIndex
{
    bool negative = false; // never true with a zero magnitude
    BigUint magnitude;

    static TileIndex of(int64_t t); // from an ordinary integer
    TileIndex& operator+=(int64_t delta);
    friend TileIndex operator+(TileIndex a, int64_t d) { return a += d; }
    friend bool operator==(const TileIndex& a, const TileIndex& b) { return a.negative == b.negative && a.magnitude == b.magnitude; }
    std::string to_decimal() const; // with a leading '-' when negative
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

// A unit's bearing round its loop, as the compass and the navigator write it: floor(v * 360 *
// 10^d / units) with d decimal places ("90.10"). The navigator goes from a bearing A to the first
// unit at or past it, ceil(A * units / (360 * 10^d)), round to the start past the last unit.
std::string bearing_of(const BigUint& v, const BigUint& units, int decimals);
BigUint unit_at_bearing(const BigUint& scaled_bearing, const BigUint& units, int decimals); // A * 10^d, < 360 * 10^d

// Variable length addressing: the shortest way found to write down where unit v stands on a loop
// of `units`, by the ways there are to get there. Either its position itself, in hex without
// leading zeros (a unit near the start is short to name); or a bearing of 0 to max_decimals places,
// typed into the navigator, then a walk of some units forward (+) or back (-) from where it lands,
// in hex. For each number of places the bearing just below the unit and the one just above it
// are tried (just above 359.9... is 0, and then the walk back goes round the loop to the end).
// Fewest characters wins, then fewer places. A unit that the bearing lands on exactly needs no
// walk at all. Across every unit of the loop, no way of writing them can be shorter on the whole
// than the position itself (there are only so many short strings); this finds the units it can.
struct ShortestPath
{
    bool by_bearing = false;
    int decimals = 0;         // the bearing's places
    std::string bearing;      // as typed ("90.1"); empty when by_bearing is false
    bool back = false;        // the walk is backwards
    BigUint walk;             // units to walk from where the bearing lands (0: none)
    std::string written;      // the whole of it: "3f2a", "90.1", "90.1+3f", "0-1a"
    size_t chars = 0;         // written.size()
    double bits = 0;          // decimal digits at log2(10), hex digits at 4, a walk's direction at 1
};
// On a long loop (past 4096 bits), bearings far from the unit are first estimated from the
// numbers' leading bits and skipped when their walk could not be short enough; `prune` false
// works every one out exactly (the tests check the two agree).
ShortestPath shortest_path(const BigUint& v, const BigUint& units, int max_decimals, bool prune = true);

} // namespace sieve
