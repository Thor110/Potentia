#include "sieve/image.hpp"

#include <algorithm>
#include <stdexcept>

namespace sieve {

namespace {

constexpr uint8_t kLevels3[8] = {0, 36, 73, 109, 146, 182, 219, 255};
constexpr uint8_t kLevels2[4] = {0, 85, 170, 255};

uint32_t sq(int v) { return static_cast<uint32_t>(v * v); }
uint32_t dist2(Rgb a, Rgb b) { return sq(a.r - b.r) + sq(a.g - b.g) + sq(a.b - b.b); }

// Index of the nearest level, ties to the lower level.
template <size_t N>
uint32_t nearest_level(const uint8_t (&levels)[N], uint8_t v)
{
    uint32_t best = 0, best_d = sq(levels[0] - v);
    for (uint32_t i = 1; i < N; ++i)
    {
        const uint32_t d = sq(levels[i] - v);
        if (d < best_d) { best = i; best_d = d; }
    }
    return best;
}

const std::vector<Palette>& registry()
{
    static const std::vector<Palette> palettes = {
        Palette("mono", "2 colours: black, white", {{0, 0, 0}, {255, 255, 255}}),
        Palette("ega16", "the 16-colour EGA/CGA palette",
                {{0x00, 0x00, 0x00}, {0x00, 0x00, 0xAA}, {0x00, 0xAA, 0x00}, {0x00, 0xAA, 0xAA},
                 {0xAA, 0x00, 0x00}, {0xAA, 0x00, 0xAA}, {0xAA, 0x55, 0x00}, {0xAA, 0xAA, 0xAA},
                 {0x55, 0x55, 0x55}, {0x55, 0x55, 0xFF}, {0x55, 0xFF, 0x55}, {0x55, 0xFF, 0xFF},
                 {0xFF, 0x55, 0x55}, {0xFF, 0x55, 0xFF}, {0xFF, 0xFF, 0x55}, {0xFF, 0xFF, 0xFF}}),
        Palette("rgb332", "256 colours: 3 bits red, 3 bits green, 2 bits blue", Palette::Kind::Rgb332),
        Palette("rgb24", "every 24-bit colour (16,777,216)", Palette::Kind::Rgb24),
    };
    return palettes;
}

} // namespace

Palette::Palette(std::string id, std::string description, std::vector<Rgb> colours)
    : id_(std::move(id)), description_(std::move(description)), kind_(Kind::Table), colours_(std::move(colours)),
      size_(static_cast<uint32_t>(colours_.size()))
{
    if (size_ < 2) throw std::invalid_argument("a palette needs at least 2 colours");
}

Palette::Palette(std::string id, std::string description, Kind procedural)
    : id_(std::move(id)), description_(std::move(description)), kind_(procedural),
      size_(procedural == Kind::Rgb332 ? 256u : 16777216u)
{
}

Rgb Palette::colour(uint32_t i) const
{
    if (i >= size_) throw std::out_of_range("palette index out of range");
    switch (kind_)
    {
    case Kind::Table: return colours_[i];
    case Kind::Rgb332: return {kLevels3[i >> 5], kLevels3[(i >> 2) & 7], kLevels2[i & 3]};
    case Kind::Rgb24: return {uint8_t(i >> 16), uint8_t(i >> 8), uint8_t(i)};
    }
    return {};
}

uint32_t Palette::nearest(Rgb c) const
{
    switch (kind_)
    {
    case Kind::Rgb24: return (uint32_t(c.r) << 16) | (uint32_t(c.g) << 8) | c.b;
    case Kind::Rgb332:
        // The palette is a product grid, so per-channel nearest (ties low) is the lowest-index
        // colour at minimum total distance.
        return (nearest_level(kLevels3, c.r) << 5) | (nearest_level(kLevels3, c.g) << 2) | nearest_level(kLevels2, c.b);
    case Kind::Table:
    {
        uint32_t best = 0, best_d = dist2(colours_[0], c);
        for (uint32_t i = 1; i < size_; ++i)
        {
            const uint32_t d = dist2(colours_[i], c);
            if (d < best_d) { best = i; best_d = d; }
        }
        return best;
    }
    }
    return 0;
}

const Palette& palette_by_id(std::string_view id)
{
    for (const auto& p : registry())
        if (p.id() == id) return p;
    throw std::invalid_argument("unknown palette '" + std::string(id) + "' (mono|ega16|rgb332|rgb24)");
}

std::vector<std::string> palette_ids()
{
    std::vector<std::string> ids;
    for (const auto& p : registry()) ids.push_back(p.id());
    return ids;
}

std::string ImageFormat::symbols_id() const
{
    const std::string pal = palette ? palette->id() : "?";
    std::string s = (frames > 1 ? "video/" : "image/") + pal + "/" + std::to_string(width) + "x" + std::to_string(height);
    if (frames > 1) s += "x" + std::to_string(frames);
    return s;
}

std::vector<uint32_t> canonicalise_frame(const RgbaImage& img, const ImageFormat& f)
{
    if (!f.palette) throw std::invalid_argument("image format has no palette");
    if (f.width == 0 || f.height == 0) throw std::invalid_argument("image size must be at least 1x1");
    if (img.width == 0 || img.height == 0 || img.rgba.size() != size_t(img.width) * img.height * 4)
        throw std::invalid_argument("source image is empty or malformed");

    const uint64_t sw = img.width, sh = img.height, tw = f.width, th = f.height;

    // Step 1: composite onto black.
    std::vector<uint8_t> rgb(sw * sh * 3);
    for (uint64_t i = 0; i < sw * sh; ++i)
    {
        const uint32_t a = img.rgba[i * 4 + 3];
        for (int c = 0; c < 3; ++c) rgb[i * 3 + c] = static_cast<uint8_t>((img.rgba[i * 4 + c] * a + 127) / 255);
    }

    // Step 2: exact area-average resample. Source pixel i spans [i*tw, (i+1)*tw); target x spans [x*sw, (x+1)*sw).
    auto overlaps = [](uint64_t n_src, uint64_t n_dst, uint64_t t, std::vector<std::pair<uint64_t, uint64_t>>& out) {
        out.clear();
        const uint64_t lo = t * n_src, hi = (t + 1) * n_src; // target span in units
        for (uint64_t i = lo / n_dst; i < n_src && i * n_dst < hi; ++i)
        {
            const uint64_t a = std::max(lo, i * n_dst), b = std::min(hi, (i + 1) * n_dst);
            if (b > a) out.emplace_back(i, b - a);
        }
    };
    const uint64_t den = sw * sh;
    std::vector<uint32_t> digits(tw * th);
    // Column overlaps are the same for every target row: work them out once.
    std::vector<std::vector<std::pair<uint64_t, uint64_t>>> columns(tw);
    for (uint64_t x = 0; x < tw; ++x) overlaps(sw, tw, x, columns[x]);
    std::vector<std::pair<uint64_t, uint64_t>> oy;
    for (uint64_t y = 0; y < th; ++y)
    {
        overlaps(sh, th, y, oy);
        for (uint64_t x = 0; x < tw; ++x)
        {
            uint64_t sum[3] = {0, 0, 0};
            for (const auto& [sy, wy] : oy)
                for (const auto& [sx, wx] : columns[x])
                {
                    const uint64_t w = wx * wy;
                    const uint8_t* p = &rgb[(sy * sw + sx) * 3];
                    sum[0] += p[0] * w; sum[1] += p[1] * w; sum[2] += p[2] * w;
                }
            Rgb c;
            c.r = static_cast<uint8_t>((2 * sum[0] + den) / (2 * den));
            c.g = static_cast<uint8_t>((2 * sum[1] + den) / (2 * den));
            c.b = static_cast<uint8_t>((2 * sum[2] + den) / (2 * den));
            // Step 3.
            digits[y * tw + x] = f.palette->nearest(c);
        }
    }
    return digits;
}

std::vector<uint32_t> canonicalise_image(const std::vector<RgbaImage>& frames, const ImageFormat& f, ImageCanonReport* report)
{
    if (frames.empty()) throw std::invalid_argument("no image frames given");
    if (f.frames == 0) throw std::invalid_argument("frame count must be at least 1");
    ImageCanonReport r;
    r.source_width = frames[0].width;
    r.source_height = frames[0].height;
    r.source_frames = static_cast<uint32_t>(frames.size());
    std::vector<uint32_t> digits;
    digits.reserve(f.unit_length());
    for (uint32_t i = 0; i < f.frames; ++i)
    {
        if (i < frames.size())
        {
            for (size_t p = 3; p < frames[i].rgba.size(); p += 4)
                if (frames[i].rgba[p] != 255) ++r.transparent_pixels;
            const auto d = canonicalise_frame(frames[i], f);
            digits.insert(digits.end(), d.begin(), d.end());
        }
        else
        {
            digits.insert(digits.end(), f.pixels_per_frame(), 0u); // black
            ++r.frames_padded;
        }
    }
    if (frames.size() > f.frames) r.frames_dropped = static_cast<uint32_t>(frames.size() - f.frames);
    if (report) *report = r;
    return digits;
}

std::vector<Rgb> render_image(const std::vector<uint32_t>& digits, const ImageFormat& f)
{
    if (!f.palette) throw std::invalid_argument("image format has no palette");
    if (digits.size() != f.unit_length()) throw std::invalid_argument("digit count does not match the image format");
    std::vector<Rgb> out;
    out.reserve(digits.size());
    for (uint32_t d : digits) out.push_back(f.palette->colour(d));
    return out;
}

} // namespace sieve
