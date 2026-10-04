// Sieve hallway — the application's own settings (graphics, controls, language), kept in
// sieve-hallway.ini next to the executable. The state-space settings of the setup menu travel
// as command-line options instead, and the filters have sieve-filters.ini.
//
//   [graphics]
//   resolution = 1600x900       ; chosen on first start from the display (see detect_display)
//   fullscreen = off
//   vsync = on                  ; wait for the display (no tearing; frame rate capped at its refresh)
//   edge_glow = off             ; Geometry Edge Glow
//   real_graphics = off         ; Real Graphics (at most one of these two is on)
//   door_portals = off          ; the doorways filled with procedural data noise
//   model_cache_mb = 64         ; memory for the rendered faces of the models line's crates
//   angle_decimals = 1          ; decimal places on the compass's degree readout (0 to 20)
//   fps_counter = off           ; frames per second, top right of the hallway
//   [controls]
//   mouse_sensitivity = 100     ; percent
//   invert_mouse_y = off
//   [language]
//   language = en
#pragma once

#include <SDL3/SDL.h>

#include <charconv>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <vector>

namespace hallway {

// "WxH" as two whole numbers (a resolution, --size). Parsed with std::from_chars rather than
// sscanf, which MSVC deprecates.
inline bool parse_size(const std::string& s, int& w, int& h)
{
    const size_t x = s.find('x');
    if (x == std::string::npos) return false;
    const auto a = std::from_chars(s.data(), s.data() + x, w);
    const auto b = std::from_chars(s.data() + x + 1, s.data() + s.size(), h);
    return a.ec == std::errc() && a.ptr == s.data() + x && b.ec == std::errc() && b.ptr == s.data() + s.size();
}

// "a,b,..." as exactly n numbers (--pose, --walk).
inline bool parse_floats(const std::string& s, float* out, size_t n)
{
    const char* p = s.c_str();
    for (size_t i = 0; i < n; ++i)
    {
        char* end = nullptr;
        out[i] = std::strtof(p, &end);
        if (end == p) return false;
        p = end;
        if (i + 1 < n)
        {
            if (*p != ',') return false;
            ++p;
        }
    }
    return *p == 0;
}

struct Resolution
{
    int w = 0, h = 0;
    bool operator==(const Resolution&) const = default;
    std::string str() const { return std::to_string(w) + "x" + std::to_string(h); }
};

// The primary display: its desktop resolution, and the resolutions to offer (its fullscreen
// modes and the common sizes that fit it), largest first, without duplicates.
struct DisplayInfo
{
    Resolution desktop;
    std::vector<Resolution> modes;
};
DisplayInfo detect_display();
// The resolution to start with when none is saved: the largest offered size with the desktop's
// shape that leaves room around a window (or the desktop itself in fullscreen).
Resolution default_resolution(const DisplayInfo& d);

// The most decimal places Angle Precision offers: the bearing is worked out exactly (a big-integer
// division), so every one of them is right, and 20 still fits under the compass (359.999...°).
inline constexpr int kMaxAngleDecimals = 20;

struct AppSettings
{
    Resolution resolution;       // {0, 0}: not chosen yet
    bool fullscreen = false;
    bool vsync = true;           // wait for the display's refresh
    bool edge_glow = false;      // Geometry Edge Glow
    bool real_graphics = false;  // Real Graphics
    bool door_portals = false;   // Door Portals: procedural data noise in the doorways
    int graphics_memory_gb = 4;  // the graphics card's memory, which the setup menu's budget keeps within
    // The two below describe the generated world rather than how it is drawn, so they are set in
    // the setup menu, not in Graphics. They are kept here, in [world], so that they are saved.
    int model_cache_mb = 64;     // the display cache: memory for the pictures on items, 8..4096
    int angle_decimals = 1;      // decimal places on the compass's degree readout, 0..kMaxAngleDecimals
    int filter_memory_mb = 512;  // the filter memory: the most one count's tables may take (plugin.hpp), 64..1048576
    int counting_memory_pct = 50; // installed memory the setup menu's counts may take at once (menu.cpp), 5..100
    int merge_cache_pct = 50;    // the filter memory kept for merged automata between counts (plugin.hpp), 0..100
    int unit_time_ms = 50;       // the time budget: the longest one unit may take to open (plugin.hpp), 5..60000
    bool fps_counter = false;    // show frames per second in the hallway
    int mouse_sensitivity = 100; // percent, 10..400
    bool invert_mouse_y = false;
    std::string language = "en";

    // Geometry Edge Glow and Real Graphics exclude each other; both may be off. Turning Real
    // Graphics on also turns Door Portals on, which is otherwise free to be set either way:
    // nothing ever turns it off for you.
    void set_edge_glow(bool on);
    void set_real_graphics(bool on);

    static std::filesystem::path default_path();
    static AppSettings load(const std::filesystem::path& path); // missing file: defaults
    bool save(const std::filesystem::path& path) const;         // false if it cannot be written
};

// Sizes the window (or switches to fullscreen) to match the settings.
void apply_video(SDL_Window* window, const AppSettings& s);

} // namespace hallway
