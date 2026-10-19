// Sieve — the tightened rope (`rope-wave-v1`, IDEAS §19.87, SPECIFICATIONS §12.2): a file's values
// predicted by a wave that runs through them, and what the wave leaves coded by how often each byte
// of it occurs. Edward's filter: keep bytes and propagate noise, searched together for the shortest
// result.
//
// Parameters (RopeParams, written "L8 W949 S1 P3 K0"):
//   L  the value length, 8, 16 or 32 bits: the file read as values of L bits, little-endian; the
//      bytes past the last whole value are carried as they are.
//   W  the values in a line, for propagation (1: each value a line of its own, so that "the line
//      before" is the value before).
//   S  how far back "the value before" is, in values (1 to 8).
//   P  the phases (Edward's 0/1/2/3), each value's prediction:
//        0  the rope alone;
//        1  the value before (S back, in the same line);
//        2  the value before and the line before (W back), averaged (rounded down); with only one
//           of them, that one;
//        3  with both and the diagonal between them (W + S back): value before + line before -
//           diagonal, held within 0 .. 2^L - 1; else as 2.
//      A value with no neighbour in its phase is predicted by the rope.
//   K  keep (0 or 1): the kept values, at the start, a third, the centre (an odd count only: an even
//      count's centre is a gap between two values), two thirds and the end, the points rounded
//      down (n / 3, 2n / 3, (n - 1) / 2). The rope is drawn straight between them: at a value i
//      between kept points a and b, x_a + floor((2 (x_b - x_a)(i - a) + (b - a)) / (2 (b - a))).
//      Without keep the rope is 0.
// What is left, e_i = (x_i - prediction_i) mod 2^L for every value that is not kept, is cut into
// L / 8 planes of bytes (the lowest byte of every e_i, then the next), and each plane is coded by
// sieve-weights-v1's table and rANS coder (SPECIFICATIONS §12.0b): exact integers, the same on
// every machine.
//
// The code: for each plane, varint k and k pairs (varint gap, varint frequency) as sieve-weights-
// v1's tables (k = 0 for an empty plane, and nothing more), then varint w and w 32-bit words
// little-endian (the plane's rANS stream); then the kept values, in order, L / 8 bytes each as in
// the file, and the bytes past the last whole value. The file's size and the parameters are given
// beside the code (the instructions hold both).
#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace sieve {

inline constexpr const char* kRopeWaveVersion = "rope-wave-v1";
inline constexpr const char* kRopeWaveColumnsVersion = "rope-wave-v2"; // v1 with columns (C1)

struct RopeParams
{
    uint32_t bits = 8, width = 1, step = 1, phases = 0;
    bool keep = false;
    // rope-wave-v2 (written " C1" after K): what is left counted column by column (the value's
    // position mod W), a table for each, and a column of one byte value given by its table alone.
    // 0: rope-wave-v1, one table for every value.
    uint32_t columns = 0;
    void check() const; // throws std::invalid_argument for parameters out of range
    std::string text() const; // "L8 W949 S1 P3 K0"
    static RopeParams parse(std::string_view text);
    bool operator==(const RopeParams&) const = default;
};

// The code for a file, and the file back from its code (its size given). rope_file throws
// std::invalid_argument on a code that does not give exactly a file of that size.
std::vector<uint8_t> rope_code(std::span<const uint8_t> file, const RopeParams& p);
std::vector<uint8_t> rope_file(std::span<const uint8_t> code, const RopeParams& p, uint64_t size);

// The code's size in bytes, estimated without coding (the planes' entropy, their tables and their
// streams' two words): for the search.
double rope_estimate(std::span<const uint8_t> file, const RopeParams& p);

// Pulling tight: the parameters that give the shortest code, searched over every value length,
// phase and step, keep or not, and the widths at which the file best matches itself shifted by a
// line, within `seconds` (0: no limit); the best found by then on a timeout.
struct RopeSearch
{
    RopeParams best;
    double estimate = 0; // bytes
    size_t tried = 0;
    bool timed_out = false;
};
RopeSearch rope_search(std::span<const uint8_t> file, double seconds = 0);

} // namespace sieve
