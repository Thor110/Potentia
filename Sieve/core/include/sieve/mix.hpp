// Sieve — mixing (`mix-v1`, IDEAS §19.102, SPECIFICATIONS §12.2): a file coded bit by bit, every bit
// predicted by several models at once and their predictions mixed by weights that learn as the file
// goes, the mixed prediction then coded exactly by a binary arithmetic coder. The address is the
// code, an exact integer; every step is integer arithmetic, so the same on every machine.
//
// The models (each a table of counters, found by a hash of its context and the bits of the byte so
// far): the bytes before, 0 to 6 of them (orders 0 to 6), and the word so far (its letters, case
// aside). A counter holds a probability (16 bits) and how often it has been seen (to 255): it moves
// a 1/(n + 2) of the way to each bit seen, so it learns fast at first and settles. The mixer: the
// models' probabilities stretched (ln(p / (1 - p)), in a fixed table of 4,096), weighted (16.16
// fixed point, one set of weights for each partial byte), summed and squashed back; each weight then
// moves by its input times the error, times 2, divided by 2^12, held within +-64. The coder: 32-bit
// ranges, a byte out whenever the top bytes agree (lpaq's). The one parameter, B, the tables' size (2^B counters
// each, 16 to 22), is chosen by the file's size and written with the way ("B22").
//
// Measured (26 October 2026): Don Quixote (pg996.txt, 2,391,678 bytes), 565,599 bytes, LZMA2 (one
// 7z stream) 685,794: 17.5% smaller; about 0.4 MB/s, coding and decoding alike.
#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace sieve {

inline constexpr const char* kMixVersion = "mix-v1";

struct MixParams
{
    uint32_t table_bits = 22; // B: 16 to 22
    void check() const;       // throws std::invalid_argument out of range
    std::string text() const; // "B22"
    static MixParams parse(std::string_view text);
    // The size for a file: 2^B counters at least twice its bytes, from 2^16 to 2^22.
    static MixParams for_size(uint64_t bytes);
    bool operator==(const MixParams&) const = default;
};

// The code for a file, and the file back from its code (its size given). mix_file throws
// std::invalid_argument on a code that ends before the file does or holds more than it.
std::vector<uint8_t> mix_code(std::span<const uint8_t> file, const MixParams& p);
std::vector<uint8_t> mix_file(std::span<const uint8_t> code, const MixParams& p, uint64_t size);

} // namespace sieve
