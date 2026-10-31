// Sieve CLI -- a PNG re-saved at its smallest (v32 in the making, `pngr`).
//
// What Compressed Formats' permanent box installs for a PNG: the same picture, every chunk as it
// was but its pixels' (IDATs), which are written again as one IDAT, the rows filtered and deflated
// as comes out smallest of the ways tried. The `.sieve` carries the PNG without its IDATs (its head)
// and its rows unfiltered, which pack as pixels do; installing makes the PNG from them by zlib.

#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace sieve::cli {

struct PngResaved
{
    std::vector<uint8_t> inner; // what is carried: the head, then the rows unfiltered
    std::vector<uint8_t> png;   // what is installed
    std::string recipe;         // "pngr <filter> <strategy> <split> <head size> <inner size>"
};

// The PNG re-saved, from its head and rows, every way tried, the smallest kept; nothing where the
// PNG does not read (a CRC wrong, the IDATs apart, interlaced, a filter or size that is not PNG's,
// the deflate not one zlib stream of exactly its rows).
std::optional<PngResaved> png_resave(const std::vector<uint8_t>& png);

// The PNG from what is carried, by the recipe's words after "pngr": filter 0 to 4 for every row,
// or 5 for each row the filter whose bytes, read as signed, sum to the least (the lower filter of
// equals); strategy 0 zlib's default, 1 its filtered; at level 9, memory level 9, a window of 15
// bits. The IDAT goes at `split` bytes into the head. Refused where the head does not read.
std::vector<uint8_t> png_resave_make(const std::vector<uint8_t>& inner, int filter, int strategy, uint64_t split, uint64_t head);

} // namespace sieve::cli
