#pragma once

// v34 (8 November 2026, Edward: "work through all of the things on that list next"): a PCX
// picture (ZSoft's) carried as its pixels, made again byte for byte on installing.
//
// A PCX of 8 bits a plane (one plane, colour-mapped or grey; three, RGB; four, RGBA), run-length coded
// (its encoding byte 1), is its 128-byte header, its pixels coded, then whatever follows them (a
// 256-colour palette: 12 and 768 bytes). Each line of the picture is its planes one after another,
// each of the header's bytes a plane line; coded plane line by plane line, as every PCX writer measured
// codes them (ImageMagick, and the specification's own example): the longest run of one byte (up to
// 63) as 0xC0 plus its length, then the byte, where it is longer than one or the byte is 0xC0 or more,
// else the byte alone.
//
// Where a plane line is longer than the picture is wide (its bytes padded to an even number), a writer
// may code the padding with the rest (ImageMagick) or alone (Pillow: the picture's width coded, then
// each padding byte as a byte alone): the skeleton's first byte says which (0, 1).
//
// What is carried ("pcx <skeleton size> <inner size>"): the pixels, each line's planes interleaved
// (pixel by pixel, the planes' bytes one after another: RGB as RGB), then the skeleton: the coder's
// byte, the header and whatever followed the pixels.

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace sieve::cli {

struct PcxOpened
{
    std::vector<uint8_t> inner; // the pixels interleaved, then the skeleton
    std::string recipe;         // "pcx <skeleton size> <inner size>"
    uint32_t width = 0, planes = 0; // the bytes of a plane line, and the planes: mixing's shape
    size_t skeleton = 0;            // the skeleton's bytes, at the end of inner
};

// Whether b starts as a PCX does (0x0A, version 0 to 5, encoding 1).
bool is_pcx(const std::vector<uint8_t>& b);

// The PCX opened, where making it again from what is carried gives back b exactly; else nothing.
std::optional<PcxOpened> pcx_open(const std::vector<uint8_t>& b);

// The PCX made again (the reader's side: a header not as pcx_open takes it is refused).
std::vector<uint8_t> pcx_remake(const std::vector<uint8_t>& inner, size_t skeleton);

// The shape of the pixels carried, read from the skeleton's header: (bytes of a plane line, planes).
std::optional<std::pair<uint32_t, uint32_t>> pcx_shape(const std::vector<uint8_t>& inner, size_t skeleton);

} // namespace sieve::cli
