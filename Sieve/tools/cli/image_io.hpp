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
void write_png(const std::string& path, uint32_t width, uint32_t height, const std::vector<Rgb>& pixels, uint32_t scale);

} // namespace sieve::cli
