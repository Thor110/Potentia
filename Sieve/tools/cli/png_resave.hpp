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

// v32 in the making (`skr`), for temporary: a PNG made again exactly from its skeleton (v21's `sk`,
// its IDATs' deflate by preflate) whose rows are carried unfiltered. PNG's filters are sums and
// differences modulo 256, so each row's can be undone and done again exactly given its filter byte;
// preflate sees the same filtered stream either way, so its corrections are the same. What is
// carried: every row unfiltered, one after another (a picture, from the first byte), then the rows'
// filter bytes, then the skeleton.
struct PngUnfiltered
{
    std::vector<uint8_t> inner; // the rows unfiltered, their filter bytes, the skeleton
    std::string recipe;         // "skr <skeleton size> <row bytes> <pixel bytes> <rows> <inner size>"
};

// From `sk`'s inner bytes (its skeleton of `skeleton` bytes, then the PNG's rows as filtered, each
// with its filter byte) and the PNG itself, for its IHDR: nothing where the PNG is interlaced, a
// filter byte is not 0 to 4, or the rows are not exactly what the IHDR gives.
std::optional<PngUnfiltered> png_unfiltered(const std::vector<uint8_t>& sk_inner, uint64_t skeleton, const std::vector<uint8_t>& png);

// `sk`'s inner bytes again, from what `skr` carries: the skeleton, then each row filtered by its
// filter byte. Refused where the recipe's words and the bytes do not agree.
std::vector<uint8_t> png_unfiltered_sk_inner(const std::vector<uint8_t>& inner, uint64_t skeleton, uint64_t row_bytes, uint64_t pixel_bytes, uint64_t rows);

} // namespace sieve::cli
