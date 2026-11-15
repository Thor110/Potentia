// Sieve hallway — reading and writing sieve-hallway.ini, and choosing sensible defaults for a
// display nobody has configured yet. See app_settings.hpp for what the file holds.
//
// Two rules shape the loading. An unrecognised or malformed setting is not an error: the default
// stands, and the file is rewritten in good order the next time anything is saved, so a file from
// an older build still starts; one that lacks an entry is rewritten at start (app_main.cpp), so
// that it gains the entries added since it was written. And the first start has no file at all,
// so detect_display() picks a resolution from what the display actually reports rather than
// assuming one, which is why the defaults live in code here and not in a shipped ini.
#include "app_settings.hpp"

#include "cli/dictionaries.hpp"
#include "cli/draft.hpp"
#include "sieve/llm.hpp"
#include "sieve/text.hpp"

#include <SDL3/SDL.h>

#include <algorithm>
#include <climits>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <set>
#include <sstream>

namespace hallway {

namespace fs = std::filesystem;

namespace {

using sieve::trim;

bool on_off(const std::string& v, bool def)
{
    if (v == "on" || v == "yes" || v == "true" || v == "1") return true;
    if (v == "off" || v == "no" || v == "false" || v == "0") return false;
    return def;
}

const char* on_off(bool b) { return b ? "on" : "off"; }

int hex_digit(char c) { return c >= '0' && c <= '9' ? c - '0' : c >= 'a' && c <= 'f' ? c - 'a' + 10 : c >= 'A' && c <= 'F' ? c - 'A' + 10 : -1; }

// A whole-number setting held to lo..hi; one that is not a number leaves the setting as it was.
template <class T>
void read_int(const std::string& v, int lo, int hi, T& out)
{
    try
    {
        out = std::clamp(std::stoi(v), lo, hi);
    }
    catch (...)
    {
    }
}

// The same for a setting that is refused (left as it was) when out of range, rather than held to it.
template <class T>
void read_int_in(const std::string& v, int lo, int hi, T& out)
{
    try
    {
        const int n = std::stoi(v);
        if (n >= lo && n <= hi) out = n;
    }
    catch (...)
    {
    }
}

// Common window sizes, offered when they fit the display.
constexpr Resolution kCommon[] = {{3840, 2160}, {2560, 1600}, {2560, 1440}, {1920, 1200}, {1920, 1080}, {1680, 1050}, {1600, 900},
                                  {1440, 900},  {1366, 768},  {1280, 800},  {1280, 720},  {1024, 768}};

} // namespace

bool parse_axis_colours(const std::string& text, uint32_t out[3])
{
    uint32_t c[3] = {};
    size_t at = 0;
    for (int k = 0; k < 3; ++k)
    {
        while (at < text.size() && text[at] == ' ') ++at;
        if (at + 7 > text.size() || text[at] != '#') return false;
        uint32_t v = 0;
        for (size_t i = 1; i <= 6; ++i)
        {
            const int d = hex_digit(text[at + i]);
            if (d < 0) return false;
            v = v << 4 | uint32_t(d);
        }
        c[k] = 0xFF000000u | v;
        at += 7;
        while (at < text.size() && text[at] == ' ') ++at;
        if (k < 2 && (at >= text.size() || text[at++] != ',')) return false;
    }
    if (at != text.size()) return false;
    for (int k = 0; k < 3; ++k) out[k] = c[k];
    return true;
}

DisplayInfo detect_display()
{
    DisplayInfo d;
    const SDL_DisplayID id = SDL_GetPrimaryDisplay();
    if (const SDL_DisplayMode* m = id ? SDL_GetDesktopDisplayMode(id) : nullptr) d.desktop = {m->w, m->h};
    if (d.desktop.w <= 0 || d.desktop.h <= 0) d.desktop = {1280, 720}; // no display information (offscreen)
    if (id)
    {
        int n = 0;
        if (SDL_DisplayMode** modes = SDL_GetFullscreenDisplayModes(id, &n))
        {
            for (int i = 0; i < n; ++i) d.modes.push_back({modes[i]->w, modes[i]->h});
            SDL_free(modes);
        }
    }
    d.modes.push_back(d.desktop);
    for (const Resolution& r : kCommon)
        if (r.w <= d.desktop.w && r.h <= d.desktop.h) d.modes.push_back(r);
    std::sort(d.modes.begin(), d.modes.end(), [](const Resolution& a, const Resolution& b) {
        return a.w != b.w ? a.w > b.w : a.h > b.h;
    });
    d.modes.erase(std::unique(d.modes.begin(), d.modes.end()), d.modes.end());
    // Below this the menus no longer fit, even scaled.
    std::erase_if(d.modes, [](const Resolution& r) { return r.w < 800 || r.h < 600; });
    if (d.modes.empty()) d.modes.push_back(d.desktop);
    return d;
}

Resolution default_resolution(const DisplayInfo& d)
{
    // Same shape as the desktop (within 2%), and smaller in both directions so the window, its
    // title bar and the taskbar fit.
    const double aspect = double(d.desktop.w) / d.desktop.h;
    for (const Resolution& r : d.modes)
        if (r.w < d.desktop.w && r.h < d.desktop.h && std::abs(double(r.w) / r.h - aspect) < 0.02 * aspect) return r;
    for (const Resolution& r : d.modes)
        if (r.w < d.desktop.w && r.h < d.desktop.h) return r;
    return d.modes.back();
}

void AppSettings::set_edge_glow(bool on)
{
    edge_glow = on;
    if (on) real_graphics = false;
}

void AppSettings::set_real_graphics(bool on)
{
    real_graphics = on;
    if (on)
    {
        edge_glow = false;
        door_portals = true; // it comes with Wireframe off, but stands on its own afterwards
    }
}

fs::path AppSettings::default_path()
{
    const fs::path exe = sieve::cli::executable_dir();
    return (exe.empty() ? fs::path(".") : exe) / "sieve-hallway.ini";
}

AppSettings AppSettings::load(const fs::path& path)
{
    AppSettings s;
    std::ifstream in(path);
    std::string line, section;
    std::set<std::string> seen; // "section.key", for `complete`
    while (std::getline(in, line))
    {
        line = trim(line);
        if (line.empty() || line[0] == ';' || line[0] == '#') continue;
        if (line.front() == '[' && line.back() == ']')
        {
            section = trim(line.substr(1, line.size() - 2));
            continue;
        }
        const size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        const std::string key = trim(line.substr(0, eq));
        std::string value = trim(line.substr(eq + 1));
        if (const size_t c = value.find(" ;"); c != std::string::npos) value = trim(value.substr(0, c));
        if (!value.empty() && value[0] == ';') value.clear(); // nothing set, only the comment
        seen.insert(section + "." + key);
        // Unknown or malformed entries are ignored: this file only holds preferences.
        if (section == "graphics")
        {
            if (key == "resolution")
            {
                int w = 0, h = 0;
                if (parse_size(value, w, h) && w >= 320 && h >= 200 && w <= 16384 && h <= 16384) s.resolution = {w, h};
            }
            else if (key == "fullscreen") s.fullscreen = on_off(value, false);
            else if (key == "vsync") s.vsync = on_off(value, true);
            else if (key == "edge_glow") s.edge_glow = on_off(value, false);
            // Wireframe (IDEAS 19.223), the models' switch the other way round; real_graphics, its
            // name before, still read, so a file written before the change keeps its choice.
            else if (key == "wireframe") s.real_graphics = !on_off(value, true);
            else if (key == "real_graphics") s.real_graphics = on_off(value, false);
            else if (key == "door_portals") s.door_portals = on_off(value, false);
            else if (key == "graphics_memory_gb") read_int(value, 1, 128, s.graphics_memory_gb);
            else if (key == "view_rooms") read_int(value, 2, 64, s.view_rooms);
            else if (key == "picture_rooms") read_int(value, 0, 8, s.picture_rooms);
            else if (key == "fps_counter") s.fps_counter = on_off(value, false);
            else if (key == "axis_colours")
            {
                uint32_t c[3];
                if (parse_axis_colours(value, c)) s.axis_colours = value; // (else the default stays)
            }
        }
        // [world], and [graphics] too, where files written before the two moved kept them.
        if (section == "world" || section == "graphics")
        {
            if (key == "angle_decimals") read_int(value, 0, kMaxAngleDecimals, s.angle_decimals);
            else if (key == "filter_memory_mb") read_int(value, 64, 1048576, s.filter_memory_mb);
            else if (key == "counting_memory_pct") read_int(value, 5, 100, s.counting_memory_pct);
            else if (key == "merge_cache_pct") read_int(value, 0, 100, s.merge_cache_pct);
            else if (key == "unit_time_ms") read_int(value, 5, 60000, s.unit_time_ms);
            else if (key == "item_memory_pct") read_int(value, 5, 90, s.item_memory_pct);
            else if (key == "model_cache_mb") read_int(value, 8, INT_MAX, s.model_cache_mb); // (no roof but the int's)
        }
        else if (section == "controls")
        {
            if (key == "mouse_sensitivity") read_int(value, 10, 400, s.mouse_sensitivity);
            else if (key == "invert_mouse_y") s.invert_mouse_y = on_off(value, false);
            else if (key == "read_aloud") s.read_aloud = on_off(value, false);
            else if (key == "text_to_voice") s.text_to_voice = on_off(value, false);
        }
        else if (section == "character")
        {
            if (key == "line") s.character = value;
            else if (key == "animation") s.character_animation = value;
            else if (key == "actions") s.character_actions = value;
            else if (key == "speaks") s.character_speaks = value;
            else if (key == "drawing" && value.size() == 1 && value[0] >= '0' && value[0] <= '6') s.character_drawing = value[0] - '0';
            else if (key == "voxel_size" && value.size() == 1 && value[0] >= '0' && value[0] <= '1') s.character_voxel_size = value[0] - '0';
            else if (key == "voxel_base_mm") read_int_in(value, 1, 100, s.character_voxel_base_mm);
            else if (key == "voxel_across") read_int_in(value, 1, 8, s.character_voxel_across);
            else if (key == "voxel_joined") s.character_voxel_joined = on_off(value, false);
            else if (key == "skin_blend") read_int_in(value, 0, 200, s.character_skin_blend);
            else if (key == "skin_detail") read_int_in(value, 0, 3, s.character_skin_detail);
            else if (key == "beside_you") s.character_beside = on_off(value, true);
            else if (key == "max_hall_scale") read_int_in(value, 1, 64, s.character_max_scale);
        }
        else if (section == "replays")
        {
            if (key == "cached") s.replays_cached = value;
            else if (key == "max_minutes" || key == "min_free_mb")
                read_int(value, 1, 1 << 20, key == "max_minutes" ? s.replays_max_minutes : s.replays_min_free_mb);
        }
        else if (section == "online")
        {
            if (key == "enabled") s.online = on_off(value, false);
        }
        else if (section == "keys")
        {
            if (!key.empty()) s.keys[key] = value; // (a key changed on Potentia Settings > Controls > Keys)
        }
        else if (section == "personal")
        {
            if (key == "map") s.personal_map = on_off(value, true);
            else if (key == "compass") s.personal_compass = on_off(value, true);
            else if (key == "hallway_tools") s.personal_hallway_tools = on_off(value, false);
            else if (key == "turning_rings") s.personal_turning_rings = on_off(value, true);
            else if (key == "wheel_step_degrees") read_int(value, 1, 90, s.personal_wheel_step);
            else if (key == "day_night") s.personal_day_night = on_off(value, false);
            else if (key == "catalogue_covers") s.personal_catalogue_covers = on_off(value, true);
            else if (key == "open_in_place") s.personal_open_in_place = on_off(value, true);
            else if (key == "shaper_radius") read_int(value, 10, 80, s.personal_shaper_radius);
            else if (key == "shaper_strength") read_int(value, 1, 20, s.personal_shaper_strength);
        }
        else if (section == "game")
        {
            if (key == "ai_explorers") s.ai_explorers = on_off(value, false);
        }
        else if (section == "raid")
        {
            if (key == "drives") s.raid_drives = value;
            else if (key == "share") s.raid_share = on_off(value, false);
            else if (key == "serve") s.raid_serve = on_off(value, false);
            else if (key == "peers") s.raid_peers = value;
            else if (key == "key") s.raid_key = value;
            else if (key == "listen" && !value.empty()) s.raid_listen = value;
            else if (key == "held") s.raid_held = value;
            else if (key == "copies") read_int(value, 1, 16, s.raid_copies);
            else if (key == "space_mb" || key == "per_peer_mb")
                read_int(value, 1, 1 << 20, key == "space_mb" ? s.raid_space_mb : s.raid_per_peer_mb);
        }
        else if (section == "models")
        {
            if (key == "text_model") s.text_model = value;
            else if (key == "picture_model") s.picture_model = value;
            else if (key == "widen_weights") s.widen_weights = value == "auto" ? 2 : on_off(value, s.widen_weights == 1) ? 1 : 0;
        }
        else if (section == "locator")
        {
            if (key == "search_seconds")
            {
                try
                {
                    const double v = std::stod(value);
                    if (v >= 0 && v <= 1e7) s.locator_search = v;
                }
                catch (...)
                {
                }
            }
            else if (key == "search_for") s.locator_search_run = value == "run";
            else if (key == "optimise_archives") s.locator_archives = s.locator_temporary = s.locator_permanent = value == "1";
            else if (key == "compressed_temporary") s.locator_temporary = value == "1";
            else if (key == "compressed_permanent") s.locator_permanent = value == "1";
            else if (key == "search_every_wave") s.locator_every_wave = value == "1";
            else if (key == "optimise_search") s.locator_optimise = value == "1";
            else if (key == "sound" && (value == "temporary" || value == "permanent" || value == "off")) s.locator_sound = value;
            else if (key == "sound_to_flac" && value == "1") s.locator_sound = "permanent"; // (an older settings file's box)
            else if (key == "priority" && (value == "size" || value == "balanced" || value == "speed")) s.locator_priority = value;
            else if (key == "prioritise_decoding" && value == "1") s.locator_priority = "speed"; // (an older settings file's box)
            else if (key == "more_formats") s.locator_more = value == "1";
            else if (key == "compare") s.locator_compare = value != "0";
            else if (key == sieve::cli::draft_ini_key()) s.locator_draft = value == "1"; // (the draft in the making: its key from its number)
            else if (key.size() > 6 && key[0] == 'v' && key.compare(key.size() - 6, 6, "_draft") == 0) {} // (an older draft's switch, once frozen: read and let go)
            else if (key == "keep_log") s.locator_log = value == "1";
            else if (key == "search_share_packing" || key == "search_share_weighing")
            {
                try
                {
                    const double v = std::stod(value);
                    if (v == -1 || (v >= 0 && v <= 1)) (key == "search_share_packing" ? s.locator_share_packing : s.locator_share_weighing) = v;
                }
                catch (...)
                {
                }
            }
        }
        else if (section == "manual")
        {
            if (key == "keep_log") s.manual_log = value == "1";
            else if (key == "undo_depth")
            {
                const int d = std::atoi(value.c_str());
                if (d == 0 || d == 10 || d == 100 || d == 1000) s.manual_undo = d;
            }
            else if (key == "fresh_random") s.manual_fresh = value == "1";
            else if (key == "width")
            {
                const int w = std::atoi(value.c_str());
                if (w >= 2 && w <= 16) s.manual_width = w;
            }
        }
        else if (section == "setup") s.setup.set_entry(key, value); // a value it cannot have is left out
        else if (section == "paths")
        {
            if (key == "ffmpeg") s.ffmpeg = value;
        }
        else if (section == "language" && key == "language" && !value.empty()) s.language = value;
    }
    if (s.edge_glow && s.real_graphics) s.real_graphics = false; // they exclude each other
    // Every entry save() writes, looked for in the file read. With no file at all nothing is
    // missing: the first start writes the file once it has chosen a resolution.
    if (in.is_open())
        for (const Row& r : s.rows())
            if (!seen.count(r.section + "." + r.key)) s.complete = false;
    return s;
}

namespace {

// Seconds as few digits as say them ("0", "0.05", "2.5", "30").
std::string seconds_text(double v)
{
    char b[32];
    std::snprintf(b, sizeof b, "%.3f", v);
    std::string t = b;
    while (!t.empty() && t.back() == '0') t.pop_back();
    if (!t.empty() && t.back() == '.') t.pop_back();
    return t.empty() ? "0" : t;
}

// A learned share (0 to 1, three places), or -1 not yet measured.
std::string share_text(double v) { return v < 0 ? "-1" : seconds_text(v); }

} // namespace

std::vector<AppSettings::Row> AppSettings::rows() const
{
    std::vector<Row> r = {
        {"graphics", "resolution", resolution.w > 0 ? resolution.str() : std::string(), "the window's size, WxH; empty: chosen from the display at start"},
        {"graphics", "fullscreen", on_off(fullscreen), "on: fullscreen at the resolution; off: a window"},
        {"graphics", "vsync", on_off(vsync), "on: wait for the display; off: frames as fast as they can be drawn (may tear)"},
        {"graphics", "edge_glow", on_off(edge_glow), "Geometry Edge Glow, drawn on the wireframe (turning it on turns Wireframe on)"},
        {"graphics", "wireframe", on_off(!real_graphics), "Wireframe: everything drawn as lines; off, the models in the meshes folder (real_graphics, its name before, is still read the other way round)"},
        {"graphics", "door_portals", on_off(door_portals), "Door Portals: procedural data noise in the doorways"},
        {"graphics", "graphics_memory_gb", std::to_string(graphics_memory_gb), "the graphics card's memory, for the setup menu's budget, 1 to 128"},
        {"graphics", "view_rooms", std::to_string(view_rooms), "View Distance: rooms drawn and kept either side of yours, 2 to 64"},
        {"graphics", "picture_rooms", std::to_string(picture_rooms), "Picture Distance: rooms either side of yours with item pictures of their own, 0 to 8"},
        {"graphics", "fps_counter", on_off(fps_counter), "frames per second, top right of the hallway"},
        {"graphics", "axis_colours", axis_colours, "the colours of the axes x, y and z wherever they are drawn (the rings round what you hold in your personal dimension, IDEAS 19.221): three #RRGGBB apart by commas; red, green and blue (" + std::string(kAxisColoursDefault) + ") by default"},
        {"world", "model_cache_mb", std::to_string(model_cache_mb), "the display cache: memory for the pictures on items, 8 MB up"},
        {"world", "angle_decimals", std::to_string(angle_decimals), "decimal places on the compass's degree readout, 0 to " + std::to_string(kMaxAngleDecimals)},
        {"world", "filter_memory_mb", std::to_string(filter_memory_mb), "the most one count of the filters' survivors may take for its tables, 64 MB up"},
        {"world", "counting_memory_pct", std::to_string(counting_memory_pct), "percent of installed memory the setup menu's counts may take at once, 5 to 100"},
        {"world", "merge_cache_pct", std::to_string(merge_cache_pct), "percent of the filter memory kept for merged filters between counts, 0 to 100"},
        {"world", "unit_time_ms", std::to_string(unit_time_ms), "the time budget: the longest one unit may take to open, 5 to 60000 ms"},
        {"world", "item_memory_pct", std::to_string(item_memory_pct), "percent of installed memory the items around you may take, 5 to 90"},
        {"controls", "mouse_sensitivity", std::to_string(mouse_sensitivity), "percent, 10 to 400"},
        {"controls", "invert_mouse_y", on_off(invert_mouse_y), "on: moving the mouse up looks down"},
        {"controls", "read_aloud", on_off(read_aloud), "on: the item chosen in a menu is read aloud, in Sieve's own voice (or Windows' own without it)"},
        {"controls", "text_to_voice", on_off(text_to_voice), "on: the AI's replies in the CHAT tab are spoken aloud, in Sieve's own voice (or Windows' own without it)"},
        {"models", "text_model", text_model, "a model folder CHAT talks with; empty: the model in hand"},
        {"models", "picture_model", picture_model, "a picture model's run folder DRAW uses; empty: the first trained one"},
        {"models", "widen_weights", widen_weights == 2 ? "auto" : on_off(widen_weights == 1), "on: a model file's BF16 and F16 weights widened as they load (twice the memory); off: as stored; auto: widened where the widened model fits in ITEM MEMORY's share of installed memory; the same numbers either way"},
        {"locator", "search_seconds", seconds_text(locator_search), "the File Locator's search for shorter addresses: seconds, 0 for no limit"},
        {"locator", "search_for", locator_search_run ? "run" : "file", "file: those seconds for each file; run: for the whole run, shared among its files"},
        {"locator", "optimise_archives", (locator_temporary || locator_permanent) ? "1" : "0", "1: either half of Archive and compressed formats ticked (as Archive and Compressed Formats, the one box before v31, read it: both); the next two say which; 0: off"},
        {"locator", "compressed_temporary", locator_temporary ? "1" : "0", "1: Archive and compressed formats: temporary ticked (an archive or a compressed file made again exactly on installing, where Sieve can); 0: off"},
        {"locator", "compressed_permanent", locator_permanent ? "1" : "0", "1: Archive and compressed formats: permanent ticked (an archive or a compressed file carried as what it holds, installed so); 0: off"},
        {"locator", "sound", locator_sound, "Sound as FLAC (v34). temporary (the default): only what Sieve makes again byte for byte on installing (a WAV, AIFF, RF64, Wave64, .au or CAF as its samples coded by FLAC; an MP3 as packMP3 codes it), an archive's sound included; any other sound carried as its bytes. permanent: also installed as other bytes of the same samples where smaller (those, WavPack, Monkey's Audio, TTA and ALAC as name.flac, through ffmpeg for the last four; a FLAC re-coded). off: sound left to the ways every file has"},
        {"locator", "optimise_search", locator_optimise ? "1" : "0", "1: Optimise Search ticked (the default): Try every way chosen for each file by its kind, from what was measured (off for every file with Priority: Decoding speed; else on for a JPEG, a file over 16 MiB or under 4 KiB, off between), its own box unused; 0: Try every way as its box says"},
        {"locator", "search_every_wave", locator_every_wave ? "1" : "0", "1: Try every way ticked: the search's shortcuts off (the rope through every wave, even on text and on bytes already packed; mixing tried on every file, whole); 0: off"},
        {"locator", "priority", locator_priority, "size: the smallest; balanced: mixing only where it saves 4% of what it carries (the default); speed: no file mixed (quickest to install, largest)"},
        {"character", "line", character, "your character (IDEAS 19.148): a seed-made one as the first line of its .character file; one holding models, or of given spheres (an imported skeleton), as its whole text with ';' between lines; empty: the default"},
        {"character", "speaks", character_speaks, "the languages your character speaks (IDEAS 19.183): ids of the language catalogue (data/languages), separated by commas; empty: none chosen"},
        {"character", "actions", character_actions, "your character's actions (IDEAS 19.186): the SHA-256 of its map of each action (idle, walk, run, ...) to the animation that plays it, read from the actions folder beside this file; empty: none, and its one animation plays whatever it does"},
        {"character", "animation", character_animation, "the animation your character plays (IDEAS 19.170): its SHA-256, read from the animations folder beside this file; empty: none"},
        {"character", "drawing", std::to_string(character_drawing), "how characters are drawn everywhere with Wireframe off (IDEAS 19.214): the Character Creator, the CHARACTER page, your personal dimension and the hallway; one setting, changed on Potentia Settings > Graphics (Character Drawing) and by the Creator's drawing row alike: 0 pixels, 1 wireframe, 2 voxels, 3 shaded (the default), 4 skin (one smooth surface over the spheres, IDEAS 19.225), 5 quads (the skin as its four-sided faces, their sides drawn), 6 smooth (the skin lit at its corners, its normals averaged across the faces round them; both IDEAS 19.214); with Wireframe on, the wireframe"},
        {"character", "voxel_size", std::to_string(character_voxel_size), "how big a voxel is (IDEAS 19.216), for characters drawn as voxels or pixels everywhere, on Potentia Settings > Graphics > Character Voxels: 0 the character's height in 16 cells (the default, as before); 1 one lattice for every character, cells of voxel_base_mm times a power of two, each character taking the coarsest at which its smallest sphere is still voxel_across cells wide (never more than 256 cells across its widest side), its grid anchored at its centre"},
        {"character", "voxel_base_mm", std::to_string(character_voxel_base_mm), "the lattice's finest cell, in millimetres, 1 to 100 (10 by default); used with voxel_size 1"},
        {"character", "voxel_across", std::to_string(character_voxel_across), "on the lattice, a character's smallest sphere is at least this many cells wide, 1 to 8 (2 by default); used with voxel_size 1"},
        {"character", "voxel_joined", on_off(character_voxel_joined), "on: greedy meshing (IDEAS 19.215), the voxels' faces that lie in one plane, face the same way and share a colour drawn as one rectangle, the lines between their cells drawn as before, so the picture is the same with fewer faces; off: each face drawn on its own (the default)"},
        {"character", "skin_blend", std::to_string(character_skin_blend), "the skin (drawings 4 to 6, IDEAS 19.225), on Potentia Settings > Graphics > Character Voxels (Skin Blend): how full it fills the creases where spheres meet, as a percentage of the spheres' mean radius, 0 to 200 (50 by default); 0 is the spheres' plain union, every crease kept"},
        {"character", "skin_detail", std::to_string(character_skin_detail), "the skin's grid: the voxels' cell (by voxel_size's rule) halved this many times, 0 to 3 (1 by default); finer is smoother and costs more (never more than 160 cells across a character's widest side; past that its cells are doubled)"},
        {"character", "beside_you", on_off(character_beside), "on: your character is you in the hallway, not drawn, its size setting the hallway's scale and its actions played (the default); off: left out there"},
        {"raid", "drives", raid_drives, "the infinite RAID (IDEAS 19.176): folders, separated by commas (a spare drive, a second disk), where each character saved with Ctrl+S is also kept as a new version, every piece on two of them; empty: not kept there"},
        {"replays", "max_minutes", std::to_string(replays_max_minutes), "the longest a recording runs (F12 in the hallway starts and stops one), in minutes; at it, the recording stops and is kept (30 by default)"},
        {"replays", "min_free_mb", std::to_string(replays_min_free_mb), "the least free space, in MB, kept where recordings are written: checked every few seconds while one runs, and below it (or below twice the recording's size, if more) the recording stops and is kept (256 by default)"},
        {"replays", "cached", replays_cached, "recordings kept as .sieve that are decoded once into the cache (beside the filters') rather than each time they are played, useful for long ones: their files' SHA-256s, separated by commas (the replay viewer's checkbox, one a recording); empty: none"},
        {"online", "enabled", on_off(online), "off (the default): nothing in Sieve reaches another machine, whatever else is set (Sieve needs no network); on: the online systems may run where their own settings are on too (the infinite RAID's share and serve)"},
        {"personal", "map", on_off(personal_map), "your personal dimension's map, top right: the hill from above, you and what is placed (IDEAS 19.197); on by default"},
        {"personal", "compass", on_off(personal_compass), "your personal dimension's compass, along the top: which way you face, north being +z (IDEAS 19.197); on by default"},
        {"personal", "turning_rings", on_off(personal_turning_rings), "rings round what you hold in your personal dimension, one for each way it turns (R, T, G), coloured by its axis (axis_colours), the turn last chosen bright (IDEAS 19.221); on by default"},
        {"personal", "wheel_step_degrees", std::to_string(personal_wheel_step), "once R, T or G has chosen a turn, how far each notch of the mouse wheel turns what you hold about it, in degrees, 1 to 90 (15 by default)"},
        {"personal", "day_night", on_off(personal_day_night), "day and night in your personal dimension (IDEAS 19.222): on, the sun crosses the sky once a day, the sky, the light and the stars at night following it, the time of day from the clock and the day's length kept in the dimension's world file (L there sets it, 1 to 18446744073709551615 seconds; 1440 without one); off (the default): always day, as before"},
        {"personal", "catalogue_covers", on_off(personal_catalogue_covers), "favourites' covers in your personal dimension's catalogue (Tab there; IDEAS 19.190): each favourite's own picture (favourites/SHA.png, the one its front shows once placed) small beside its row, and the chosen one's larger just below the list, so they can be told apart before placing; on by default"},
        {"personal", "open_in_place", on_off(personal_open_in_place), "O on a placed favourite in your personal dimension (IDEAS 19.208): on (the default), it opens in the hallway's own viewer over the dimension, a small \"calculating...\" by the crosshair while its line is made in the background (kept while you stay in the dimension), Esc closing it back to the dimension; where it cannot open so, or off, O goes into the hallway on the favourite's line, as before"},
        {"personal", "shaper_radius", std::to_string(personal_shaper_radius), "the ground shaper's brush in your personal dimension (IDEAS 19.209; 1 takes it out there): how far round the crosshair's point it shapes the ground, in metres, 10 to 80 (20 by default); the wheel changes it there for the visit"},
        {"personal", "shaper_strength", std::to_string(personal_shaper_strength), "how fast the ground shaper raises, lowers or flattens the ground at its brush's middle, in metres a second, 1 to 20 (4 by default), less towards its edge"},
        {"personal", "hallway_tools", on_off(personal_hallway_tools), "the hallway's own tools (Restart Sieve, the node graph, the navigator) in your dimension's pause menu too; off by default"},
        {"game", "ai_explorers", on_off(ai_explorers), "AI Explorers in the state space (IDEAS 19.189): off (the default) or on; not built yet, so nothing reads it until they are"},
        {"raid", "share", on_off(raid_share), "OPT IN ONLY (Potentia Settings > Infinite RAID): on: each Ctrl+S also sends your save, sealed under your key so only you can read it, to the machines in peers; off (the default): it never leaves this machine"},
        {"raid", "copies", std::to_string(raid_copies), "with share on: how many of the machines keep each piece of your save, 1 to 16 (3 by default; with fewer machines, each of them); Check my shares puts back any piece kept on fewer"},
        {"raid", "peers", raid_peers, "the machines your save is sent to with share on: HOST:PORT, separated by commas (each running sieve raid serve, or Sieve with serve on); empty: none"},
        {"raid", "key", raid_key, "your key's file (sieve-raid-key-v1); keep it safe and keep a copy, as without it what others keep for you cannot be read by anyone; empty: raid.key beside these settings, made when share is first turned on"},
        {"raid", "serve", on_off(raid_serve), "OPT IN ONLY (Potentia Settings > Infinite RAID): on: while Sieve runs, this machine keeps others' sealed saves for whoever connects to listen, up to the limits below (it cannot read them); off (the default): it keeps none"},
        {"raid", "listen", raid_listen, "with serve on: the address answered on, HOST:PORT (0.0.0.0: every network this machine is on; 127.0.0.1: this machine alone)"},
        {"raid", "held", raid_held, "with serve on: the folder others' saves are kept in; empty: raid-held beside these settings"},
        {"raid", "space_mb", std::to_string(raid_space_mb), "with serve on: the most kept for others in all, in MiB (1024 by default)"},
        {"raid", "per_peer_mb", std::to_string(raid_per_peer_mb), "with serve on: the most kept for any one address, in MiB (64 by default)"},
        {"character", "max_hall_scale", std::to_string(character_max_scale), "how many times the hallway may grow so that your character fits its doors, 1 to 64 (a larger character does not fit)"},
        {"locator", "compare", locator_compare ? "1" : "0", "1: Compare with zip and 7z ticked (the default): what zip (deflate 9) and 7z (LZMA2 9e) make of it and of the instructions, beside Sieve's; 0: Sieve's figures alone, quicker on a large folder"},
        {"locator", "more_formats", locator_more ? "1" : "0", "1: More formats ticked: the comparison with gzip, bzip2, zstd, brotli and lz4 too, each the whole as one stream (slower: brotli above all); 0: zip and 7z only"},
        {"locator", sieve::cli::draft_ini_key(), locator_draft ? "1" : "0", "1: " + sieve::cli::draft_manifest() + " in the making, for testing only (docs/NEXT-VERSION.md): what it makes is a draft, not to be published; 0: off, the newest frozen version made"},
        {"locator", "keep_log", locator_log ? "1" : "0", "1: Keep a log ticked: each reading and saving writes what it reported as it went (every best so far, with its time), how long it took and its phases' times to a text file in the logs folder beside these settings, and each install its decoding and writing times; 0: off"},
        {"locator", "search_share_packing", share_text(locator_share_packing), "learned: the share of the search's time packing has used here, 0 to 1, for the time estimate; -1: not measured yet"},
        {"locator", "search_share_weighing", share_text(locator_share_weighing), "learned: the share of the search's time weighing has used here, 0 to 1, for the time estimate; -1: not measured yet"},
    };
    r.push_back({"manual", "keep_log", manual_log ? "1" : "0", "1: Manual Navigation's Keep a log ticked (the default): every step written to a CSV in the logs folder beside these settings as it lands, its head naming the file (its path, size and SHA-256), so Load Log can replay it; 0: off"});
    r.push_back({"manual", "undo_depth", std::to_string(manual_undo), "Manual Navigation's undo depth: 0 for all the steps (the default), or 10, 100, 1000"});
    r.push_back({"manual", "fresh_random", manual_fresh ? "1" : "0", "1: FRESH RANDOM BYTES ticked (the default): with no file loaded, a new example of random bytes each time the width changes; 0: the example keeps its bytes, random ones added or the end cut"});
    r.push_back({"manual", "width", std::to_string(manual_width), "Manual Navigation's width: bytes a row, and the example's side (2 to 16; 2 by default, an example of 4 bytes)"});
    for (const Settings::Entry& e : setup.entries()) r.push_back({"setup", e.key, e.value, e.note});
    r.push_back({"paths", "ffmpeg", ffmpeg, "the ffmpeg program; empty: SIEVE_FFMPEG, then one beside Sieve, then the PATH"});
    r.push_back({"language", "language", language, "the language file's name in data/lang (en: English)"});
    // Only the keys changed from their defaults, so none are written until one is (Potentia Settings > Controls > Keys).
    for (const auto& [id, k] : keys) r.push_back({"keys", id, k, "changed from its default on Potentia Settings > Controls > Keys"});
    return r;
}

std::string AppSettings::text() const
{
    std::ostringstream o;
    o << "; Sieve hallway settings: every one there is, each with what it is. The main menu's Settings\n"
      << "; and the setup menu edit them, and so may you: the hallway reads this file at start, and its\n"
      << "; options (--length 64, --ffmpeg PATH, ...) win over it for that run. A line it cannot read\n"
      << "; leaves that setting as it was. A comment starts a line with ';' or follows \" ;\".\n";
    std::string section;
    for (const Row& r : rows())
    {
        if (r.section != section)
        {
            section = r.section;
            o << "\n[" << section << "]\n";
        }
        // The comments line up in a column, unless the entry is longer than it.
        std::string entry = r.key + " = " + r.value;
        entry.resize(std::max<size_t>(entry.size() + 1, 40), ' ');
        o << entry << "; " << r.note << "\n";
    }
    return o.str();
}

bool AppSettings::save(const fs::path& path) const
{
    AppSettings kept = *this;
    if (run_only && *run_only) (*run_only)(kept);
    const std::string t = kept.text();
    std::ofstream out(path, std::ios::binary);
    out << t;
    return bool(out);
}

void apply_video(SDL_Window* window, const AppSettings& s)
{
    if (s.resolution.w <= 0) return;
    if (s.fullscreen)
    {
        // Exclusive fullscreen at the chosen resolution when the display has it, else the desktop.
        SDL_DisplayMode mode;
        const SDL_DisplayID id = SDL_GetDisplayForWindow(window);
        if (id && SDL_GetClosestFullscreenDisplayMode(id, s.resolution.w, s.resolution.h, 0.0f, true, &mode))
            SDL_SetWindowFullscreenMode(window, &mode);
        else SDL_SetWindowFullscreenMode(window, nullptr);
        SDL_SetWindowFullscreen(window, true);
    }
    else
    {
        SDL_SetWindowFullscreen(window, false);
        SDL_SetWindowSize(window, s.resolution.w, s.resolution.h);
        SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    }
    SDL_SyncWindow(window);
}

void apply_widen_weights(const AppSettings& s)
{
    if (s.widen_weights == 2)
        sieve::llm::set_widen_weights_auto(uint64_t(std::max(0, SDL_GetSystemRAM())) * (uint64_t(1) << 20) / 100 * uint64_t(s.item_memory_pct));
    else sieve::llm::set_widen_weights(s.widen_weights == 1);
}

} // namespace hallway
