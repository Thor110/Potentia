// Potentia CLI — image file input/output (PNG, JPEG, BMP, GIF, TGA via stb_image; PNG output).
#pragma once

#include "potentia/image.hpp"

#include <string>
#include <vector>

namespace potentia::cli {

// Loads every frame of an image file (animated GIFs give several frames).
std::vector<RgbaImage> load_image_frames(const std::string& path);

// Writes RGB pixels as a PNG, each pixel scaled up to a scale x scale block.
void write_png(const std::string& path, uint32_t width, uint32_t height, const std::vector<Rgb>& pixels, uint32_t scale);

} // namespace potentia::cli
