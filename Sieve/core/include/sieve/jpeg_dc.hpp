// Sieve — a JPEG at an eighth of its size, from each block's average alone (2 November 2026).
//
// A JPEG's pixels are cut into blocks of 8 by 8, and each block's first coefficient (its DC) is the
// block's average: 8 times (the mean of its samples less 128), over its quantiser. So a picture an
// eighth as wide and as high is had from those alone, each block one pixel, with no inverse DCT,
// keeping one number a block (a sixty-fourth of the pixels) rather than the picture. For a picture
// too large to decode whole (sieve-view: Edward, "what happens with a real 300mb image? Rare but
// they exist"). Baseline and extended sequential JPEGs (every coefficient's code is read past, as
// the averages lie between them) and progressive ones (their first DC scans; the scans of the other
// coefficients are stepped over unread). Huffman coding only; one component (grey) or three (YCbCr,
// as JFIF), any sampling; anything else gives nothing.
#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <utility>
#include <vector>

namespace sieve {

struct EighthJpeg
{
    uint32_t width = 0, height = 0;            // the picture's own size
    uint32_t eighth_w = 0, eighth_h = 0;       // the eighth's: ceil(width / 8) by ceil(height / 8)
    std::vector<uint8_t> rgba;                 // eighth_w * eighth_h pixels, 4 bytes each
};

// The JPEG's size from its frame header alone (nothing decoded), where it is one this reads.
std::optional<std::pair<uint32_t, uint32_t>> jpeg_size(std::span<const uint8_t> jpeg);

// The eighth, or nothing where the JPEG is not one this reads or is damaged.
std::optional<EighthJpeg> jpeg_eighth(std::span<const uint8_t> jpeg);

} // namespace sieve
