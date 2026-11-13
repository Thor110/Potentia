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
//   [controls]   mouse_sensitivity, invert_mouse_y, read_aloud, text_to_voice
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
#include <functional>
#include <memory>
#include <map>
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
    bool read_aloud = false; // the menus' chosen item spoken (speech.hpp; Windows)
    bool text_to_voice = false; // the AI's replies in the CHAT tab spoken (speech.hpp)
    // The models given as files (the setup menu's AI rows): a language model's folder for CHAT,
    // and a picture model's run folder for DRAW. Empty: the model in hand, and the first trained
    // picture model in the training folders.
    std::string text_model, picture_model;
    // How a model given as a file keeps its BF16 and F16 weights (llm::set_widen_weights): 0 off, as
    // stored (half the memory); 1 on, widened to 32-bit floats as it loads; 2 auto, widened where
    // the widened model fits in ITEM MEMORY's share of installed memory. The same numbers either way.
    int widen_weights = 0;
    // The File Locator's search for shorter addresses ([locator]): its time in seconds (0: no
    // limit), for each file or for the whole run (`search_run`), the other worked out from the
    // files counted. Kept so that the first search of the next run has it too.
    double locator_search = 0;
    bool locator_search_run = false;
    bool locator_every_wave = false; // the rope's search through every wave, even on packed bytes
    // What the packer weighs ways by: size, balanced (the default since 7 November 2026: mixing only
    // where it saves 4% of what it carries) or speed (Prioritise Decoding Speed: no file mixed).
    std::string locator_priority = "balanced";
    // [character] (IDEAS 19.148): your character as its settings line (v1), or its whole text on one
    // line with ';' between its lines (v2, v3: sieve/character.hpp); empty: the
    // default character), how the CHARACTER page draws it (0 pixels to 3 shaded), and whether it
    // walks beside you in the hallway.
    std::string character;
    // The animation it plays (IDEAS 19.170): its SHA-256, the file of that name in the animations
    // folder beside the settings file (the store); empty: none.
    std::string character_animation;
    // The languages it speaks (IDEAS 19.183): ids of the language catalogue, separated by commas.
    std::string character_speaks;
    // Its actions (IDEAS 19.186): the SHA-256 of its map of actions to animations (sieve-actions-v1),
    // read from the actions folder beside the settings file; empty: none, so the one animation plays.
    std::string character_actions;
    // [raid] drives (IDEAS 19.176): folders, separated by commas, where each character saved with
    // Ctrl+S is also kept in the infinite RAID as a new version; empty: not kept there.
    std::string raid_drives;
    // The RAID between machines (IDEAS 19.176 stage 3, SPECIFICATIONS 12.0o), opt in only, each behind
    // a red box in Settings > RAID. raid_share: each Ctrl+S also sends your save, sealed under your
    // key, to raid_peers (HOST:PORT, separated by commas). raid_serve: while Sieve runs, this machine
    // keeps others' sealed saves, answering on raid_listen, in raid_held (empty: "raid-held" beside
    // the settings), up to raid_space_mb in all and raid_per_peer_mb for any one address. raid_key:
    // your key's file (empty: "raid.key" beside the settings; made when sharing is first ticked).
    bool raid_share = false, raid_serve = false;
    // Online (Edward, 13 November 2026: "online isn't a requirement and ... it is off by default"):
    // every system that reaches another machine (the infinite RAID's sharing and serving, and any to
    // come) runs only while this is on, whatever its own setting says; off by default, so Sieve needs
    // no network at all.
    bool online = false;
    // Game systems (Settings > Game Systems): AI Explorers, AI-driven explorers walking the state
    // space (IDEAS 19.189), off by default; the switch they will answer to, ready before they come.
    bool ai_explorers = false;
    // Your personal dimension's features (IDEAS 19.197): the map and the compass, each on by default and
    // each to be turned off (Personal Dimensions > Features).
    bool personal_map = true, personal_compass = true;
    // Keys changed from their defaults (IDEAS 19.199; keys.hpp): "SECTION/ID" to its keys, as [keys] keeps
    // them. Only what differs is kept, so a new default reaches everyone who has not changed it.
    std::map<std::string, std::string> keys;
    std::string raid_peers, raid_key, raid_listen = "0.0.0.0:47100", raid_held;
    int raid_space_mb = 1024, raid_per_peer_mb = 64;
    // Recordings kept packed (.sieve) that are decoded once into the cache rather than each time they
    // are played (IDEAS 19.184; the replay viewer's checkbox, one a recording): their SHA-256s, by commas.
    std::string replays_cached;
    // A recording's longest (F12 in the hallway), and the least free space kept where recordings are
    // written: below it, a recording stops and keeps what it has (Edward, 13 November 2026).
    int replays_max_minutes = 30, replays_min_free_mb = 256;
    // How many machines keep each piece of a shared save (Edward, 13 November 2026: 3 by default).
    int raid_copies = 3;
    int character_drawing = 3; // shaded (Edward, 13 November 2026); the wireframe while Real Graphics is off
    bool character_beside = true;
    int character_max_scale = 16;   // how many times the hallway may grow to fit it, 1 to 64
    bool locator_compare = true;     // Compare with zip and 7z: what they make of it worked out and shown beside Sieve's (IDEAS 19.124)
    bool locator_more = false;       // More formats: the comparison with gzip, bzip2, zstd, brotli and lz4 too (0.13.78)
    std::string locator_sound = "temporary"; // Sound as FLAC (v34): "temporary", "permanent" or "off" (pack.hpp's SoundMode)
    bool locator_optimise = true;    // Optimise Search: Try every way chosen for each file by its kind (ticked unless unticked)
    bool locator_draft = false;      // the next version's draft (cli/draft.hpp; [locator] v33_draft, its name from the version)
    bool locator_log = false;        // Keep a log: each run's reports and times written to a text file
    // Manual Navigation ([manual]): Keep a log (every step written to a CSV in the logs folder as it
    // lands; on unless unticked) and the undo depth (0: all, the default).
    bool manual_log = true;
    int manual_undo = 0;
    bool manual_fresh = true; // FRESH RANDOM BYTES: a new example each time the width changes
    int manual_width = 2;     // bytes a row, and the example's side: 2 to 16
    // "Optimise Compressed Formats" ([locator] optimise_archives, the key's old name kept): off unless
    // ticked; read, both halves below ticked (written: either is).
    bool locator_archives = false;
    // v31: which of its two halves are ticked: temporary (made again exactly on installing) and
    // permanent (carried as what it holds); [locator] compressed_temporary, compressed_permanent. An
    // older file's optimise_archives = 1 ticks both (what the one box did).
    bool locator_temporary = false, locator_permanent = false;
    // The share of the search's limit runs here have used, packing and weighing (-1: not measured
    // yet): the File Locator's estimate counts that much of the search, not all of it.
    double locator_share_packing = -1, locator_share_weighing = -1;
    // The setup menu's settings ([setup]): every line's shape and the GLOBAL settings, as they
    // were when the setup menu last changed them. Its ffmpeg is not kept here but below.
    Settings setup;
    // The ffmpeg program ([paths]) that reads and writes the picture, video and sound formats Sieve
    // does not read itself. Empty: SIEVE_FFMPEG, then one beside Sieve, then the PATH
    // (cli/media_decode.hpp). --ffmpeg wins over it.
    std::string ffmpeg;
    std::string language = "en";
    // The values this run was given on its command line (--search-every-wave, --filter-memory ...):
    // save() writes each back as the file had it, unless it was changed since (not saved). (0.13.78:
    // they were written to the file by the next setting saved, and so kept for every run after.)
    std::shared_ptr<const std::function<void(AppSettings&)>> run_only;
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

// Tells the engine how to keep a model file's weights (llm::set_widen_weights): off, on, or auto
// against ITEM MEMORY's share of installed memory as it is now. Called at the start, once the
// options are read, and again when ITEM MEMORY changes (the review of 6 November 2026: auto had
// been worked out before --item-memory was read, and never again).
void apply_widen_weights(const AppSettings& s);

} // namespace hallway
