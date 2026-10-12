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
//
// "canon-image-v2" (the default) is v1 with one step between 2 and 3, so that a picture's own
// light and dark survive the fitting (a line drawing on paper averages to plain paper under v1):
//   2a. If every resampled pixel is already a palette colour, nothing changes (so a picture saved
//       from the line, at any whole scale, comes back as the same unit). Otherwise, take each
//       pixel's luminance Y = (299 R + 587 G + 114 B + 500) / 1000 (integer division), sort the
//       N values, and let k = N / 20 (integer division), lo = the k-th smallest (from 0) and hi =
//       the k-th largest. If hi > lo, each channel c becomes
//       ((clamp(c, lo, hi) - lo) * 510 + (hi - lo)) / (2 * (hi - lo)), the range lo..hi stretched
//       to 0..255 and rounded half up; the darkest and lightest twentieth are clipped. Each frame
//       of a video is levelled on its own.
#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace sieve {

inline constexpr const char* kImageCanonVersion = "canon-image-v1";
inline constexpr const char* kImageCanonVersion2 = "canon-image-v2";
// The share of pixels clipped at each end by canon-image-v2's levels: one in kImageLevelsClip.
inline constexpr uint64_t kImageLevelsClip = 20;

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
    // How a picture from outside is fitted: canon-image-v2's levels (the default), or v1's plain
    // average. It decides only where a file lands, not the line's units, so it is not in the id.
    enum class Fit { Average, Levels } fit = Fit::Levels;
    const char* canon_version() const { return fit == Fit::Levels ? kImageCanonVersion2 : kImageCanonVersion; }

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

// The same, a frame at a time, for frames read one after another (a long video need never be held
// whole): add each frame in order, then finish. canonicalise_image is this.
class ImageCanoniser
{
public:
    explicit ImageCanoniser(const ImageFormat& format);
    void add(const RgbaImage& frame);
    std::vector<uint32_t> finish(ImageCanonReport* report = nullptr);

private:
    ImageFormat f_;
    ImageCanonReport r_;
    std::vector<uint32_t> digits_;
    uint32_t kept_ = 0;
};

// Unit digits -> RGB pixels (all frames, frame after frame, row-major).
std::vector<Rgb> render_image(const std::vector<uint32_t>& digits, const ImageFormat& format);

} // namespace sieve
