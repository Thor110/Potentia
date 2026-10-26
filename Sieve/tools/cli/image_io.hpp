// Sieve CLI — image file input/output (PNG, JPEG, BMP, GIF, TGA via stb_image; PNG output).
#pragma once

#include "sieve/image.hpp"

#include <string>
#include <vector>

namespace sieve::cli {

// Loads every frame of an image file (animated GIFs give several frames).
std::vector<RgbaImage> load_image_frames(const std::string& path);

// The same from bytes in memory. `what` names them in an error.
std::vector<RgbaImage> decode_image_frames(const uint8_t* data, size_t size, const std::string& what);

// Whether the bytes are a picture stb_image can read (by its header alone, without decoding), and
// if so its size.
bool image_info(const uint8_t* data, size_t size, uint32_t& width, uint32_t& height);

// Writes RGB pixels as a PNG, each pixel scaled up to a scale x scale block.
// The same picture as a PNG file's bytes, in memory.
std::string encode_png(uint32_t width, uint32_t height, const std::vector<Rgb>& pixels, uint32_t scale);
void write_png(const std::string& path, uint32_t width, uint32_t height, const std::vector<Rgb>& pixels, uint32_t scale);

// v25 (a PNG made again by stb_image_write, Sieve's own PNG writer, as the hallway's screenshots
// are): a PNG's pixels as stb_image reads them, row by row, `components` bytes a pixel (1 to 4, as
// many as the PNG holds); false for anything else (16 bits a channel: stb_image gives 8).
bool png_pixels(const uint8_t* data, size_t size, std::vector<uint8_t>& pixels, int& width, int& height, int& components);
// stb_image_write's PNG of those pixels at a zlib level (0 to 9; its default 8) and a filter (-1,
// its default, choosing each row's; else 0 to 4 for every row). Its settings are put back after.
std::string stb_png(const std::vector<uint8_t>& pixels, int width, int height, int components, int level, int filter);

} // namespace sieve::cli
