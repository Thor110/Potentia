// Sieve's pictures, read for showing (picture_source.cpp): shared by sieve-view and the thumbnail
// handler. Every limit a hostile file could push against is checked before anything is decoded.
#pragma once

#include "sieve/image.hpp" // RgbaImage

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace sieve::view {

// A picture to show: where it came from, its own file's bytes, and its pixels once made.
struct Picture
{
    std::string name;          // its name (in a .sieve, its path there)
    std::vector<uint8_t> file; // the picture file's own bytes
    uint32_t width = 0, height = 0; // its own size
    uint32_t scale = 1;             // 1: its pixels; 8: an eighth of them (too large to decode whole)
    std::string reduced;            // where reduced, why
    bool made = false;
    RgbaImage pixels;
    // An animated GIF's other frames, where asked for (make_pixels' all_frames) and they fit the
    // memory: frame k (from 1) is more[k - 1], each as large as pixels; delays_ms holds every
    // frame's delay, the first's too, as the GIF gives it. Where they do not fit, the first alone,
    // and why in one_frame.
    std::vector<RgbaImage> more;
    std::vector<int> delays_ms;
    size_t frames = 1;     // the frames the GIF holds (by its blocks)
    std::string one_frame; // where only the first is shown though it holds more, why
    const RgbaImage& frame(size_t k) const { return k == 0 || more.empty() ? pixels : more[std::min(k, more.size()) - 1]; }
};

std::string u8(const std::filesystem::path& p);
std::filesystem::path from_u8(const std::string& s);
std::string lower(std::string s);
std::string human_bytes(uint64_t n);
bool picture_name(const std::string& name); // a picture by its extension
bool sieve_name(const std::string& name);   // a .simage or .sieve by its extension
std::vector<uint8_t> read_all(const std::filesystem::path& p);

// Half the memory free when asked (2 GiB where that cannot be had), or what set_memory_budget gave.
uint64_t memory_budget();
void set_memory_budget(uint64_t bytes); // 0: as above

// The header checked, then the pixels made: the first frame (with all_frames, an animated GIF's
// every frame where they fit the memory); a JPEG too large for that, an eighth from its blocks'
// averages; else refused (an exception), with why.
void make_pixels(Picture& p, bool all_frames = false);

// A GIF's frames, counted from its blocks without decoding them (0 where it is not a GIF read so
// far as that).
size_t gif_frame_count(const std::vector<uint8_t>& b);

// A .sieve's pictures, its listing read and checked first (refused, with why, where it is not
// pictures within the limits), then installed into a folder of its own and read back; each with its
// file's bytes, its pixels not yet made.
std::vector<Picture> pictures_of_sieve(const std::filesystem::path& file, std::string* listing_said = nullptr);

// The picture made smaller, its longer side `side` at most (never larger than it is), each pixel the
// mean of those it covers, weighted by their alpha so a transparent pixel lends no colour: a
// thumbnail (sieve-thumbs.dll; sieve-view --thumbnail).
RgbaImage shrink_to(const RgbaImage& img, uint32_t side);

} // namespace sieve::view
