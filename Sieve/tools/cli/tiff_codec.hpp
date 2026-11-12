#pragma once

// v34 (8 November 2026, Edward: "work through all of the things on that list next"): a TIFF's
// strips made again exactly from their pixels, as libtiff (4.x, the library ImageMagick, GIMP, Pillow
// and most programs write TIFFs with) codes them. Written here from libtiff's algorithms (its licence:
// third_party_licenses/libtiff); nothing of libtiff is linked.
//
// LZW (compression 5): codes written most significant bit first, 9 bits wide to start; a clear code
// (256) first; the longest string already in the table taken each time and its code written, then
// that string one byte longer given the next free code (from 258); just after, where the next free
// code is 4,094, a clear code (at the width then) and the table emptied, else where it is past the
// widest the width holds (511, 1,023, 2,047) the width grows by a bit, else where 10,000 bytes or more
// have been read since the last look (counted from the strip's start, not from a clear), the ratio of
// bytes read to bits written since the last clear, times 256 (a 24.8 fraction), is worked out: where it
// is no better than the last, a clear code and the table emptied (and the last forgotten), else it is
// kept as the last. At the end the string left over is written, then (as above, with the next free
// code one more) a clear code or a wider width, then the end code (257), then the bits left over to a
// whole byte, filled with zeros.
//
// PackBits (compression 32773): each row (the strip cut into rows of the row's bytes, the last
// shorter) coded alone: runs of a byte (2 to 128 of it) as 1 - n and the byte; other bytes as literals
// of up to 128, their count less one first; a run of two after a literal, and a single byte after that,
// joined into the literal where it stays under 128 bytes.
//
// Horizontal differencing (predictor 2): each row, from its end, each sample less the one a pixel
// before it (mod 2^bits), 8 or 16 bits a sample, 16-bit samples in the TIFF's own byte order.

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace sieve::cli {

// A strip's bytes coded as libtiff's LZW codes them.
std::vector<uint8_t> tiff_lzw_encode(const uint8_t* p, size_t n);
// An LZW strip decoded (TIFF's codes, most significant bit first, a width growing one code early),
// to its end code; nothing where it is not one, or makes more than `limit` bytes.
std::optional<std::vector<uint8_t>> tiff_lzw_decode(const uint8_t* p, size_t n, size_t limit);

// A strip's bytes coded as libtiff's PackBits codes them, row by row.
std::vector<uint8_t> tiff_packbits_encode(const uint8_t* p, size_t n, size_t row);
// A PackBits strip decoded until it makes `size` bytes; nothing where it does not, or does not use
// exactly its n bytes.
std::optional<std::vector<uint8_t>> tiff_packbits_decode(const uint8_t* p, size_t n, size_t size);

// Horizontal differencing, done (`predict`) and undone (`unpredict`), in place: rows of `row` bytes
// (the bytes a whole number of rows), each sample of `bytes` (1 or 2, `big` its order) less the one
// `stride` samples before it in its row.
bool tiff_predict(std::vector<uint8_t>& v, size_t row, size_t stride, int bytes, bool big);
bool tiff_unpredict(std::vector<uint8_t>& v, size_t row, size_t stride, int bytes, bool big);

} // namespace sieve::cli
