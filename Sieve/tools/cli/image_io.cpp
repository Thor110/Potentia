#include "image_io.hpp"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_HDR
#define STBI_NO_LINEAR
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <mutex>
#include <stdexcept>

namespace sieve::cli {

namespace {

std::string read_bytes(const std::string& path)
{
    std::ifstream in(std::filesystem::path(path), std::ios::binary);
    if (!in) throw std::runtime_error("cannot open '" + path + "'");
    return std::string(std::istreambuf_iterator<char>(in), {});
}

void append(void* ctx, void* data, int size)
{
    auto* out = static_cast<std::string*>(ctx);
    out->append(static_cast<const char*>(data), static_cast<size_t>(size));
}

// stb_image_write keeps its level and filter in globals, which every PNG it writes reads. stb_png
// changes them to make a PNG again exactly (v25) and puts them back; every other writer must wait
// while it does, or an item's PNG made on another thread at the same time (an install decodes files
// at once) is written at the other's level and comes out other bytes, refused by its SHA-256.
std::mutex g_stb_write;

} // namespace

std::vector<RgbaImage> load_image_frames(const std::string& path)
{
    const std::string bytes = read_bytes(path);
    return decode_image_frames(reinterpret_cast<const uint8_t*>(bytes.data()), bytes.size(), path);
}

bool image_info(const uint8_t* data, size_t size, uint32_t& width, uint32_t& height)
{
    if (size == 0 || size > 0x7FFFFFFFu) return false;
    int w = 0, h = 0, comp = 0;
    if (!stbi_info_from_memory(data, static_cast<int>(size), &w, &h, &comp) || w <= 0 || h <= 0) return false;
    width = uint32_t(w);
    height = uint32_t(h);
    return true;
}

std::vector<RgbaImage> decode_image_frames(const uint8_t* bytes_in, size_t size, const std::string& path)
{
    if (size > 0x7FFFFFFFu) throw std::runtime_error("cannot decode '" + path + "': too large");
    const auto* data = reinterpret_cast<const stbi_uc*>(bytes_in);
    const int len = static_cast<int>(size);
    std::vector<RgbaImage> frames;

    const bool is_gif = size >= 6 && bytes_in[0] == 'G' && bytes_in[1] == 'I' && bytes_in[2] == 'F';
    if (is_gif)
    {
        int* delays = nullptr;
        int w = 0, h = 0, n = 0, comp = 0;
        stbi_uc* pixels = stbi_load_gif_from_memory(data, len, &delays, &w, &h, &n, &comp, 4);
        if (!pixels) throw std::runtime_error("cannot decode '" + path + "': " + stbi_failure_reason());
        const size_t frame_bytes = size_t(w) * h * 4;
        for (int i = 0; i < n; ++i)
            frames.push_back({uint32_t(w), uint32_t(h),
                              std::vector<uint8_t>(pixels + i * frame_bytes, pixels + (i + 1) * frame_bytes)});
        stbi_image_free(pixels);
        STBI_FREE(delays);
        return frames;
    }

    int w = 0, h = 0, comp = 0;
    stbi_uc* pixels = stbi_load_from_memory(data, len, &w, &h, &comp, 4);
    if (!pixels) throw std::runtime_error("cannot decode '" + path + "': " + stbi_failure_reason());
    frames.push_back({uint32_t(w), uint32_t(h), std::vector<uint8_t>(pixels, pixels + size_t(w) * h * 4)});
    stbi_image_free(pixels);
    return frames;
}

std::string encode_png(uint32_t width, uint32_t height, const std::vector<Rgb>& pixels, uint32_t scale)
{
    if (scale == 0) scale = 1;
    // The PNG writer takes int sizes, and the whole image is held in memory: keep it under 2^30 bytes.
    if (uint64_t(width) * scale * 3 > 0x7FFFFFFFull || uint64_t(width) * scale * height * scale * 3 > (uint64_t(1) << 30))
        throw std::invalid_argument("the picture would be too large to save at scale " + std::to_string(scale) + " (use a smaller --scale)");
    if (pixels.size() != size_t(width) * height) throw std::invalid_argument("pixel count does not match size");
    const uint32_t W = width * scale, H = height * scale;
    std::vector<uint8_t> rgb(size_t(W) * H * 3);
    for (uint32_t y = 0; y < H; ++y)
        for (uint32_t x = 0; x < W; ++x)
        {
            const Rgb& p = pixels[size_t(y / scale) * width + x / scale];
            uint8_t* o = &rgb[(size_t(y) * W + x) * 3];
            o[0] = p.r; o[1] = p.g; o[2] = p.b;
        }
    std::string png;
    std::lock_guard<std::mutex> lock(g_stb_write); // (the writer's settings: see g_stb_write)
    if (!stbi_write_png_to_func(append, &png, int(W), int(H), 3, rgb.data(), int(W) * 3))
        throw std::runtime_error("cannot encode PNG");
    return png;
}

void write_png(const std::string& path, uint32_t width, uint32_t height, const std::vector<Rgb>& pixels, uint32_t scale)
{
    const std::string png = encode_png(width, height, pixels, scale);
    std::ofstream out(std::filesystem::path(path), std::ios::binary);
    if (!out) throw std::runtime_error("cannot write '" + path + "'");
    out.write(png.data(), static_cast<std::streamsize>(png.size()));
    out.close(); // (checked once closed: a small file is written as it closes)
    if (!out) throw std::runtime_error("cannot write '" + path + "'");
}

bool png_pixels(const uint8_t* data, size_t size, std::vector<uint8_t>& pixels, int& width, int& height, int& components)
{
    if (size < 8 || size > 0x7FFFFFFF || std::memcmp(data, "\x89PNG\r\n\x1a\n", 8) != 0) return false;
    if (stbi_is_16_bit_from_memory(data, int(size))) return false;
    int w = 0, h = 0, c = 0;
    unsigned char* px = stbi_load_from_memory(data, int(size), &w, &h, &c, 0);
    if (!px) return false;
    pixels.assign(px, px + size_t(w) * size_t(h) * size_t(c));
    stbi_image_free(px);
    width = w;
    height = h;
    components = c;
    return true;
}

std::string stb_png(const std::vector<uint8_t>& pixels, int width, int height, int components, int level, int filter)
{
    // (stb_image_write keeps its settings in globals: set and put back under the lock every
    // writer takes, g_stb_write.)
    std::lock_guard<std::mutex> lock(g_stb_write);
    const int keep_level = stbi_write_png_compression_level, keep_filter = stbi_write_force_png_filter;
    stbi_write_png_compression_level = level;
    stbi_write_force_png_filter = filter;
    int len = 0;
    unsigned char* out = stbi_write_png_to_mem(pixels.data(), width * components, width, height, components, &len);
    stbi_write_png_compression_level = keep_level;
    stbi_write_force_png_filter = keep_filter;
    if (!out) throw std::runtime_error("stb_image_write could not write a PNG");
    std::string r(reinterpret_cast<const char*>(out), size_t(len));
    STBIW_FREE(out);
    return r;
}

} // namespace sieve::cli
