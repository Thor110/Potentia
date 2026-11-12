// Sieve's pictures, read for showing: a picture file's header checked and its pixels made within the
// memory free, and a .sieve of pictures read from its listing first, refused there where it is not
// pictures within the limits, then installed into a folder of its own and each picture read back.
// Shared by sieve-view and the thumbnail handler Explorer asks for a .simage's picture (5 November
// 2026); what they do with the pixels is their own.

#include "picture_source.hpp"

#include "cli/image_io.hpp"
#include "cli/locate.hpp"
#include "cli/pack.hpp" // memory_free
#include "cli/vault.hpp" // take_checked_frames
#include "sieve/jpeg_dc.hpp"

#include <algorithm>
#include <atomic>
#include <cctype>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <optional>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace sieve::view {

namespace fs = std::filesystem;
using namespace sieve::cli;

namespace {

// The limits a .sieve's listing and a picture's header are held to before anything is decoded:
// a count of files, and memory, half of what is free when it is asked (or where that cannot be
// had, 2 GiB; --budget gives it instead).
constexpr size_t kMaxFiles = 4096;
constexpr uint32_t kMaxSide = uint32_t(1) << 24; // pixels a side (stb_image's own limit)
uint64_t g_budget = 0;                           // (set_memory_budget: --budget)

} // namespace

void set_memory_budget(uint64_t bytes) { g_budget = bytes; }

uint64_t memory_budget()
{
    if (g_budget) return g_budget;
    const uint64_t free_now = memory_free();
    return free_now ? free_now / 2 : uint64_t(2) << 30;
}

std::string lower(std::string s)
{
    for (char& c : s) c = char(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

std::string human_bytes(uint64_t n)
{
    char b[48];
    if (n < 1024) std::snprintf(b, sizeof b, "%llu bytes", (unsigned long long)n);
    else if (n < 1048576) std::snprintf(b, sizeof b, "%.1f KB", double(n) / 1024.0);
    else if (n < 1073741824ull) std::snprintf(b, sizeof b, "%.1f MB", double(n) / 1048576.0);
    else std::snprintf(b, sizeof b, "%.2f GB", double(n) / 1073741824.0);
    return b;
}

bool picture_name(const std::string& name)
{
    const std::string e = lower(fs::path(from_u8(name)).extension().string());
    return e == ".png" || e == ".jpg" || e == ".jpeg" || e == ".jpe" || e == ".jfif" || e == ".bmp" || e == ".gif" || e == ".tga";
}

bool sieve_name(const std::string& name)
{
    const std::string e = lower(fs::path(from_u8(name)).extension().string());
    return e == ".simage" || e == ".sieve";
}

namespace {

// A recipe that says the file is a picture made again: a JPEG from its brunsli form, a PNG by
// stb_image_write, from its rows unfiltered, or re-saved; (v34) a GIF from its frames' indices.
bool picture_recipe(const std::string& recipe)
{
    for (const char* k : {"jbr ", "stb ", "skr ", "pngr ", "gif "})
        if (recipe.rfind(k, 0) == 0) return true;
    return false;
}

} // namespace

std::vector<uint8_t> read_all(const fs::path& p)
{
    std::ifstream f(p, std::ios::binary);
    if (!f) throw std::runtime_error("cannot open " + u8(p));
    return std::vector<uint8_t>((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}


namespace {

bool is_jpeg_bytes(const std::vector<uint8_t>& b) { return b.size() >= 3 && b[0] == 0xFF && b[1] == 0xD8 && b[2] == 0xFF; }

// A PNG's size from its header (IHDR), for one stb_image will not read for its size.
std::optional<std::pair<uint32_t, uint32_t>> png_size(const std::vector<uint8_t>& b)
{
    if (b.size() < 24 || std::memcmp(b.data(), "\x89PNG\r\n\x1a\n", 8) != 0 || std::memcmp(b.data() + 12, "IHDR", 4) != 0) return std::nullopt;
    auto be32 = [&](size_t at) { return uint32_t(b[at]) << 24 | uint32_t(b[at + 1]) << 16 | uint32_t(b[at + 2]) << 8 | b[at + 3]; };
    return std::make_pair(be32(16), be32(20));
}

} // namespace

size_t gif_frame_count(const std::vector<uint8_t>& b)
{
    if (b.size() < 13 || (std::memcmp(b.data(), "GIF87a", 6) != 0 && std::memcmp(b.data(), "GIF89a", 6) != 0)) return 0;
    const size_t n = b.size();
    size_t at = 13, frames = 0;
    if (b[10] & 0x80) at += size_t(3) << ((b[10] & 7) + 1);
    // (each block's sub-blocks skipped by their lengths: a few steps a frame, nothing decoded)
    auto skip_sub_blocks = [&] {
        while (at < n && b[at]) at += 1 + size_t(b[at]);
        ++at;
    };
    while (at < n && b[at] != 0x3B)
    {
        if (b[at] == 0x21)
        {
            at += 2;
            skip_sub_blocks();
        }
        else if (b[at] == 0x2C)
        {
            if (n - at < 11) break;
            const uint8_t f = b[at + 9];
            at += 10;
            if (f & 0x80) at += size_t(3) << ((f & 7) + 1);
            ++at; // (the LZW code size)
            skip_sub_blocks();
            ++frames;
        }
        else break;
    }
    return frames;
}

// The header checked, then the pixels made: the first frame (with all_frames, an animated GIF's every
// frame where they fit); a JPEG too large for that, an eighth from its blocks' averages; else
// refused, with why.
void make_pixels(Picture& p, bool all_frames)
{
    // (made already: done, unless its other frames are asked for now and were not made then)
    if (p.made && (!all_frames || p.frames <= 1 || !p.more.empty() || !p.one_frame.empty())) return;
    p.made = false;
    p.one_frame.clear(), p.delays_ms.clear(), p.more.clear();
    uint32_t w = 0, h = 0;
    const bool jpeg = is_jpeg_bytes(p.file);
    const bool info = image_info(p.file.data(), p.file.size(), w, h);
    if (!info)
    {
        const auto size = jpeg ? jpeg_size(p.file) : png_size(p.file);
        if (!size) throw std::runtime_error(p.name + ": not a picture this viewer reads");
        std::tie(w, h) = *size;
    }
    if (w == 0 || h == 0 || w > kMaxSide || h > kMaxSide)
        throw std::runtime_error(p.name + ": " + std::to_string(w) + " by " + std::to_string(h) + " pixels, over " + std::to_string(kMaxSide) + " a side");
    p.width = w, p.height = h;
    // (stb_image's pixels and the viewer's RGBA of them, 4 bytes a pixel each at most; stb_image
    // itself decodes a JPEG's of under 2 GB and a PNG's of 1 GiB at most)
    const uint64_t pixels = uint64_t(w) * h, need = pixels * 8, budget = memory_budget();
    const bool stb_can = info && pixels * 4 < (uint64_t(1) << 31);
    if (stb_can && need <= budget)
    {
        // (A GIF's frames, every one as large as its screen, held at once by stb_image and again
        // here: each its 8 bytes a pixel, the lot within the budget, or the first alone.)
        const size_t count = gif_frame_count(p.file);
        p.frames = std::max<size_t>(count, 1);
        const bool every = all_frames && count > 1 && need * count <= budget;
        if (all_frames && count > 1 && !every)
            p.one_frame = "its " + std::to_string(count) + " frames need about " + human_bytes(need * count) + " to play, more than half the memory free (" +
                          human_bytes(budget) + ")";
        // (the frames the vault decoded to check them, where it has them for these very bytes: not
        // decoded a second time)
        std::vector<RgbaImage> frames;
        if (auto checked = vault::take_checked_frames(p.file))
        {
            frames = std::move(checked->frames);
            if (every) p.delays_ms = std::move(checked->delays_ms);
            else if (frames.size() > 1) frames.resize(1);
        }
        else frames = decode_image_frames(p.file.data(), p.file.size(), p.name, every ? &p.delays_ms : nullptr, !every);
        if (frames.empty()) throw std::runtime_error(p.name + ": no picture in it");
        p.pixels = std::move(frames.front());
        p.more.clear();
        for (size_t k = 1; k < frames.size(); ++k)
            if (frames[k].width == p.pixels.width && frames[k].height == p.pixels.height) p.more.push_back(std::move(frames[k]));
        if (p.more.size() + 1 != frames.size()) p.more.clear(), p.delays_ms.clear();
        // (the frames there were, where all were asked for: a GIF whose blocks count more than
        // decode, one cut short, is not decoded again on each call; review, 6 November 2026)
        if (every) p.frames = p.more.size() + 1;
        p.scale = 1;
    }
    else
    {
        const std::string why = need > budget ? "its pixels need about " + human_bytes(need) + " to decode whole, more than half the memory free (" + human_bytes(budget) + ")"
                                              : "too large for stb_image to decode whole";
        if (!jpeg) throw std::runtime_error(p.name + ": " + std::to_string(w) + " by " + std::to_string(h) + " pixels: " + why + " (only a JPEG is shown reduced)");
        if (pixels / 64 * 8 > budget) throw std::runtime_error(p.name + ": " + std::to_string(w) + " by " + std::to_string(h) + " pixels: even its eighth needs more than half the memory free");
        auto eighth = jpeg_eighth(p.file);
        if (!eighth) throw std::runtime_error(p.name + ": " + why + ", and its eighth could not be read (a baseline or progressive JPEG, grey or YCbCr, is)");
        p.pixels.width = eighth->eighth_w;
        p.pixels.height = eighth->eighth_h;
        p.pixels.rgba = std::move(eighth->rgba);
        p.scale = 8;
        p.reduced = why;
    }
    p.made = true;
}

// What a .sieve holds, read from its listing alone, and refused there if it is not pictures within
// the limits; else installed into a folder of its own and each picture's file read back.
std::vector<Picture> pictures_of_sieve(const fs::path& file, std::string* listing_said)
{
    // (its listing alone first, nothing decoded: cli/locate.hpp listing_of_installer)
    const Manifest listed = listing_of_installer(file);
    size_t n = 0;
    uint64_t all = 0;
    for (const ManifestEntry& e : listed.entries)
    {
        if (e.dir || e.member) continue;
        if (++n > kMaxFiles) throw std::runtime_error("more than " + std::to_string(kMaxFiles) + " files: not a .sieve of pictures this viewer opens");
        const std::string name = e.path;
        if (!picture_recipe(e.remake) && !picture_name(name))
            throw std::runtime_error(name + ": not a picture (by its recipe and its name), so the .sieve is not opened");
        if (e.size == kSizeAsMade) throw std::runtime_error(name + ": its size not said");
        all += e.size;
        // (every file is held at once while it is installed: half the memory free for them all)
        if (all > memory_budget())
            throw std::runtime_error("its pictures come to more than half the memory free (" + human_bytes(memory_budget()) + "), by its listing" +
                                     (e.size > memory_budget() ? " (" + name + " alone " + human_bytes(e.size) + ")" : std::string()));
    }
    if (n == 0) throw std::runtime_error("no files in it");
    if (listing_said) *listing_said = std::to_string(n) + (n == 1 ? " picture, " : " pictures, ") + human_bytes(all);
    // (then read again to install: each file decoded, stopping at its declared size)
    const Manifest m = installable_manifest(file, false);
    // (A folder of its own, removed after: installing checks each file's size and SHA-256 first.)
    std::error_code ec;
    static std::atomic<unsigned> serial{0};
    const auto now = std::chrono::steady_clock::now().time_since_epoch().count();
    const fs::path dest = fs::temp_directory_path(ec) / ("sieve-view-" + std::to_string(now) + "-" + std::to_string(serial++));
    struct Gone
    {
        fs::path p;
        ~Gone()
        {
            std::error_code e;
            fs::remove_all(p, e);
        }
    } gone{dest};
    install_tree(m, dest, false, nullptr);
    std::vector<Picture> out;
    for (const ManifestEntry& e : m.entries)
    {
        if (e.dir || e.member) continue;
        Picture p;
        p.name = e.path;
        p.file = read_all(dest / from_u8(e.path));
        out.push_back(std::move(p));
    }
    return out;
}

RgbaImage shrink_to(const RgbaImage& img, uint32_t side)
{
    const uint32_t w = img.width, h = img.height;
    if (w == 0 || h == 0 || side == 0 || img.rgba.size() < size_t(w) * h * 4) throw std::runtime_error("no picture to make smaller");
    const double k = std::min(1.0, double(side) / double(std::max(w, h)));
    RgbaImage out;
    out.width = std::max<uint32_t>(1, uint32_t(double(w) * k + 0.5));
    out.height = std::max<uint32_t>(1, uint32_t(double(h) * k + 0.5));
    out.rgba.resize(size_t(out.width) * out.height * 4);
    for (uint32_t y = 0; y < out.height; ++y)
    {
        const uint32_t y0 = uint32_t(uint64_t(y) * h / out.height), y1 = std::max(y0 + 1, uint32_t(uint64_t(y + 1) * h / out.height));
        for (uint32_t x = 0; x < out.width; ++x)
        {
            const uint32_t x0 = uint32_t(uint64_t(x) * w / out.width), x1 = std::max(x0 + 1, uint32_t(uint64_t(x + 1) * w / out.width));
            uint64_t r = 0, g = 0, b = 0, a = 0, n = 0;
            for (uint32_t sy = y0; sy < y1; ++sy)
            {
                const uint8_t* p = img.rgba.data() + (size_t(sy) * w + x0) * 4;
                for (uint32_t sx = x0; sx < x1; ++sx, p += 4)
                    r += uint64_t(p[0]) * p[3], g += uint64_t(p[1]) * p[3], b += uint64_t(p[2]) * p[3], a += p[3], ++n;
            }
            uint8_t* q = out.rgba.data() + (size_t(y) * out.width + x) * 4;
            // (the colour back from weighted to straight: by the alpha summed; none where all were clear)
            q[0] = uint8_t(a ? (r + a / 2) / a : 0), q[1] = uint8_t(a ? (g + a / 2) / a : 0), q[2] = uint8_t(a ? (b + a / 2) / a : 0);
            q[3] = uint8_t((a + n / 2) / n);
        }
    }
    return out;
}

} // namespace sieve::view
