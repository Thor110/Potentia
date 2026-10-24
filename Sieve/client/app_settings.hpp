// Sieve hallway — the application's settings, kept in sieve-hallway.ini next to the executable:
// graphics, the world, controls, the models given as files, the setup menu's settings, the paths
// to other programs, and the language. The filters have sieve-filters.ini instead.
//
// Every entry the file can hold is written, each with its value and a comment saying what it is,
// and an entry with nothing set is written empty rather than left out, so the file is a list of
// every setting there is. The hallway's options (--length, --ffmpeg, ...) win over the file for
// the run they are given in. save() writes the file in this order:
//
//   [graphics]   resolution, fullscreen, vsync, edge_glow, real_graphics, door_portals,
//                graphics_memory_gb, view_rooms, picture_rooms, fps_counter
//   [world]      model_cache_mb, angle_decimals, filter_memory_mb, counting_memory_pct,
//                merge_cache_pct, unit_time_ms, item_memory_pct
//   [controls]   mouse_sensitivity, invert_mouse_y
//   [models]     text_model, picture_model, widen_weights
//   [setup]      every line's shape and the GLOBAL settings (Settings::entries, setup.hpp)
//   [paths]      ffmpeg
//   [language]   language
#pragma once

#include "setup.hpp"

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
    // View distance: the rooms drawn and kept either side of yours, 2..64, and the rooms
    // either side of yours whose items have pictures of their own (the rest borrow yours), 0..8.
    int view_rooms = 7;
    int picture_rooms = 1;
    // The two below describe the generated world rather than how it is drawn, so they are set in
    // the setup menu, not in Graphics. They are kept here, in [world], so that they are saved.
    int model_cache_mb = 64;     // the display cache: memory for the pictures on items, 8 MB up
    int angle_decimals = 1;      // decimal places on the compass's degree readout, 0..kMaxAngleDecimals
    int filter_memory_mb = 512;  // the filter memory: the most one count's tables may take (plugin.hpp), 64..1048576
    int counting_memory_pct = 50; // installed memory the setup menu's counts may take at once (menu.cpp), 5..100
    int merge_cache_pct = 50;    // the filter memory kept for merged automata between counts (plugin.hpp), 0..100
    int unit_time_ms = 50;       // the time budget: the longest one unit may take to open (plugin.hpp), 5..60000
    int item_memory_pct = 25;    // installed memory the hallway's items around you may take (menu.cpp), 5..90
    bool fps_counter = false;    // show frames per second in the hallway
    int mouse_sensitivity = 100; // percent, 10..400
    bool invert_mouse_y = false;
    // The models given as files (the setup menu's AI rows): a language model's folder for CHAT,
    // and a picture model's run folder for DRAW. Empty: the model in hand, and the first trained
    // picture model in the training folders.
    std::string text_model, picture_model;
    // How a model given as a file keeps its BF16 and F16 weights (llm::set_widen_weights): off, as
    // stored (half the memory); on, widened to 32-bit floats as it loads. The same numbers either way.
    bool widen_weights = false;
    // The File Locator's search for shorter addresses ([locator]): its time in seconds (0: no
    // limit), for each file or for the whole run (`search_run`), the other worked out from the
    // files counted. Kept so that the first search of the next run has it too.
    double locator_search = 0;
    bool locator_search_run = false;
    bool locator_every_wave = false; // the rope's search through every wave, even on packed bytes
    // "Optimise Archive Formats" ([locator] optimise_archives): off unless ticked.
    bool locator_archives = false;
    // The setup menu's settings ([setup]): every line's shape and the GLOBAL settings, as they
    // were when the setup menu last changed them. Its ffmpeg is not kept here but below.
    Settings setup;
    // The ffmpeg program ([paths]) that reads and writes the picture, video and sound formats Sieve
    // does not read itself. Empty: SIEVE_FFMPEG, then one beside Sieve, then the PATH
    // (cli/media_decode.hpp). --ffmpeg wins over it.
    std::string ffmpeg;
    std::string language = "en";
    // Whether the file loaded had every entry save() writes (not saved). An older file lacks the
    // entries added since it was written, and is written again at start so that it gains them.
    bool complete = true;

    // Geometry Edge Glow and Real Graphics exclude each other; both may be off. Turning Real
    // Graphics on also turns Door Portals on, which is otherwise free to be set either way:
    // nothing ever turns it off for you.
    void set_edge_glow(bool on);
    void set_real_graphics(bool on);

    static std::filesystem::path default_path();
    static AppSettings load(const std::filesystem::path& path); // missing file: defaults
    bool save(const std::filesystem::path& path) const;         // false if it cannot be written
    std::string text() const;                                   // what save() writes

    // One entry of the file: its section, key, value as written, and the comment beside it.
    struct Row
    {
        std::string section, key, value, note;
    };
    std::vector<Row> rows() const; // every entry, in the order save() writes them
};

// Sizes the window (or switches to fullscreen) to match the settings.
void apply_video(SDL_Window* window, const AppSettings& s);

} // namespace hallway
