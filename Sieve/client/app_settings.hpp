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
//   fps_counter = off           ; frames per second, top right of the hallway
//   [controls]
//   mouse_sensitivity = 100     ; percent
//   invert_mouse_y = off
//   [language]
//   language = en
#pragma once

#include <SDL3/SDL.h>

#include <filesystem>
#include <string>
#include <vector>

namespace hallway {

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

struct AppSettings
{
    Resolution resolution;       // {0, 0}: not chosen yet
    bool fullscreen = false;
    bool vsync = true;           // wait for the display's refresh
    bool edge_glow = false;      // Geometry Edge Glow
    bool real_graphics = false;  // Real Graphics
    bool fps_counter = false;    // show frames per second in the hallway
    int mouse_sensitivity = 100; // percent, 10..400
    bool invert_mouse_y = false;
    std::string language = "en";

    // Geometry Edge Glow and Real Graphics exclude each other; both may be off.
    void set_edge_glow(bool on);
    void set_real_graphics(bool on);

    static std::filesystem::path default_path();
    static AppSettings load(const std::filesystem::path& path); // missing file: defaults
    bool save(const std::filesystem::path& path) const;         // false if it cannot be written
};

// Sizes the window (or switches to fullscreen) to match the settings.
void apply_video(SDL_Window* window, const AppSettings& s);

} // namespace hallway
