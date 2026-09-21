// Sieve — the image and video lines (SPECIFICATIONS §3.1, §6.2).
//
// An image unit is WIDTH x HEIGHT pixels, each one digit: an index into a fixed palette,
// row-major from the top-left. A video unit is FRAMES such images in order.
//
// "canon-image-v1", applied to any source image:
//   1. Composite onto black: channel = (channel * alpha + 127) / 255 (integer division).
//   2. Resample to WIDTH x HEIGHT by exact area averaging. In integer coordinates where a
//      source pixel is WIDTH units wide (HEIGHT tall) and a target pixel is source-width units
//      wide (source-height tall), each target channel is the overlap-weighted sum S of source
//      channels, divided by D = srcW * srcH and rounded half up: (2S + D) / (2D).
//   3. Quantise each pixel to the nearest palette colour by squared RGB distance; ties go to
//      the lowest palette index.
// Aspect ratio is not preserved: the image is stretched to fill the unit.
#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace sieve {

inline constexpr const char* kImageCanonVersion = "canon-image-v1";

struct Rgb
{
    uint8_t r = 0, g = 0, b = 0;
    friend bool operator==(const Rgb&, const Rgb&) = default;
};

class Palette
{
public:
    enum class Kind { Table, Rgb332, Rgb24 };

    Palette(std::string id, std::string description, std::vector<Rgb> colours);
    Palette(std::string id, std::string description, Kind procedural);

    const std::string& id() const { return id_; }
    const std::string& description() const { return description_; }
    uint32_t size() const { return size_; }
    Rgb colour(uint32_t index) const;
    uint32_t nearest(Rgb c) const; // nearest by squared distance, ties -> lowest index

private:
    std::string id_, description_;
    Kind kind_;
    std::vector<Rgb> colours_;
    uint32_t size_;
};

// Built-in palettes. Index 0 is black in every palette, so padding frames are black.
//   mono    2 colours: black, white
//   ega16   the 16-colour EGA/CGA palette
//   rgb332  256 colours: 3 bits red, 3 bits green, 2 bits blue
//   rgb24   16,777,216 colours: every 24-bit RGB colour (no quantisation loss)
const Palette& palette_by_id(std::string_view id); // throws std::invalid_argument
std::vector<std::string> palette_ids();

struct RgbaImage
{
    uint32_t width = 0, height = 0;
    std::vector<uint8_t> rgba; // width * height * 4 bytes, row-major
};

struct ImageFormat
{
    uint32_t width = 10, height = 10;
    uint32_t frames = 1; // 1 for the image line
    const Palette* palette = nullptr;

    uint32_t pixels_per_frame() const { return width * height; }
    uint32_t unit_length() const { return width * height * frames; }
    // e.g. "image/mono/10x10" or "video/mono/5x5x8"
    std::string symbols_id() const;
};

struct ImageCanonReport
{
    uint32_t source_width = 0, source_height = 0;
    uint32_t source_frames = 0;
    uint32_t transparent_pixels = 0; // pixels with alpha < 255 (across all frames)
    uint32_t frames_dropped = 0;     // source frames beyond FRAMES
    uint32_t frames_padded = 0;      // black frames added to reach FRAMES
};

// Canonicalise one frame (steps 1-3) to palette indices, row-major.
std::vector<uint32_t> canonicalise_frame(const RgbaImage& image, const ImageFormat& format);

// Canonicalise a whole unit: an image (one frame) or a video (up to FRAMES frames).
std::vector<uint32_t> canonicalise_image(const std::vector<RgbaImage>& frames, const ImageFormat& format,
                                         ImageCanonReport* report = nullptr);

// Unit digits -> RGB pixels (all frames, frame after frame, row-major).
std::vector<Rgb> render_image(const std::vector<uint32_t>& digits, const ImageFormat& format);

} // namespace sieve
