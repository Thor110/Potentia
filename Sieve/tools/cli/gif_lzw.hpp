#pragma once

// v34 (6 November 2026, Edward: "build the gif route for gifenc first"): a GIF carried as its
// skeleton and each frame's colour indices, its LZW undone, made again exactly on installing.
//
// What is carried is each frame's colour indices, one byte each, width times height of them, in the
// order the frames come, then the skeleton: the GIF with each frame's LZW sub-blocks (and their
// closing empty block) taken out, everything else as it was, the frame's LZW code size byte
// included. (The indices first, so a picture's rows start where the bytes do, as "skr" has them.)
// Made again, each frame's indices are coded as compress codes them (Weiner's LZWEncoder, and
// gifenc after it): a clear code first, a new code for every new string until the table holds
// 4096, then a clear code; cut into sub-blocks of one size, the last shorter. A GIF made any other way is not given back by this, and is left as it is (the opener
// makes it again and compares before it is ever chosen).
//
// Measured on six GIFs gifenc wrote (results/gif-opened-2026-11-06.md): every one of their 346
// frames given back byte for byte; and Pillow 12's GIFs the same.

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace sieve::cli {

// Whether b starts as a GIF does ("GIF87a" or "GIF89a").
bool is_gif(const std::vector<uint8_t>& b);

struct GifOpened
{
    std::vector<uint8_t> inner; // every frame's indices, then the skeleton
    std::string recipe;         // "gif <sub-block size> <skeleton size> <inner size>"
    size_t frames = 0;
};

// The GIF opened, where making it again from what is carried gives back b exactly; else nothing.
std::optional<GifOpened> gif_open(const std::vector<uint8_t>& b);

// The GIF made again from its skeleton and indices (the reader's side: anything not as gif_open
// writes it is refused, never followed).
std::vector<uint8_t> gif_remake(const std::vector<uint8_t>& inner, size_t skeleton, size_t block);

// The width every frame has, from the skeleton, where they all have the same (mixing's shape for
// the indices); else nothing.
std::optional<uint32_t> gif_width(const std::vector<uint8_t>& inner, size_t skeleton);

} // namespace sieve::cli
