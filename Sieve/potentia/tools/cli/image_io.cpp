#include "image_io.hpp"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_HDR
#define STBI_NO_LINEAR
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>

namespace potentia::cli {

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

} // namespace

std::vector<RgbaImage> load_image_frames(const std::string& path)
{
    const std::string bytes = read_bytes(path);
    const auto* data = reinterpret_cast<const stbi_uc*>(bytes.data());
    const int len = static_cast<int>(bytes.size());
    std::vector<RgbaImage> frames;

    const bool is_gif = bytes.size() >= 6 && bytes.compare(0, 3, "GIF") == 0;
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

void write_png(const std::string& path, uint32_t width, uint32_t height, const std::vector<Rgb>& pixels, uint32_t scale)
{
    if (scale == 0) scale = 1;
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
    if (!stbi_write_png_to_func(append, &png, int(W), int(H), 3, rgb.data(), int(W) * 3))
        throw std::runtime_error("cannot encode PNG");
    std::ofstream out(std::filesystem::path(path), std::ios::binary);
    if (!out) throw std::runtime_error("cannot write '" + path + "'");
    out.write(png.data(), static_cast<std::streamsize>(png.size()));
}

} // namespace potentia::cli
