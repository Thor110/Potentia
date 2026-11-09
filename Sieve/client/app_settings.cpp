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

#include <SDL3/SDL.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <set>
#include <sstream>

namespace hallway {

namespace fs = std::filesystem;

namespace {

std::string trim(const std::string& s)
{
    const auto a = s.find_first_not_of(" \t\r");
    if (a == std::string::npos) return "";
    return s.substr(a, s.find_last_not_of(" \t\r") - a + 1);
}

bool on_off(const std::string& v, bool def)
{
    if (v == "on" || v == "yes" || v == "true" || v == "1") return true;
    if (v == "off" || v == "no" || v == "false" || v == "0") return false;
    return def;
}

const char* on_off(bool b) { return b ? "on" : "off"; }

// Common window sizes, offered when they fit the display.
constexpr Resolution kCommon[] = {{3840, 2160}, {2560, 1600}, {2560, 1440}, {1920, 1200}, {1920, 1080}, {1680, 1050}, {1600, 900},
                                  {1440, 900},  {1366, 768},  {1280, 800},  {1280, 720},  {1024, 768}};

} // namespace

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
        door_portals = true; // it comes with Real Graphics, but stands on its own afterwards
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
            else if (key == "real_graphics") s.real_graphics = on_off(value, false);
            else if (key == "door_portals") s.door_portals = on_off(value, false);
            else if (key == "graphics_memory_gb")
            {
                try { s.graphics_memory_gb = std::clamp(std::stoi(value), 1, 128); } catch (...) {}
            }
            else if (key == "view_rooms")
            {
                try { s.view_rooms = std::clamp(std::stoi(value), 2, 64); } catch (...) {}
            }
            else if (key == "picture_rooms")
            {
                try { s.picture_rooms = std::clamp(std::stoi(value), 0, 8); } catch (...) {}
            }
            else if (key == "fps_counter") s.fps_counter = on_off(value, false);
        }
        // [world], and [graphics] too, where files written before the two moved kept them.
        if (section == "world" || section == "graphics")
        {
            if (key == "angle_decimals")
            {
                try { s.angle_decimals = std::clamp(std::stoi(value), 0, kMaxAngleDecimals); } catch (...) {}
            }
            else if (key == "filter_memory_mb")
            {
                try { s.filter_memory_mb = std::clamp(std::stoi(value), 64, 1048576); } catch (...) {}
            }
            else if (key == "counting_memory_pct")
            {
                try { s.counting_memory_pct = std::clamp(std::stoi(value), 5, 100); } catch (...) {}
            }
            else if (key == "merge_cache_pct")
            {
                try { s.merge_cache_pct = std::clamp(std::stoi(value), 0, 100); } catch (...) {}
            }
            else if (key == "unit_time_ms")
            {
                try { s.unit_time_ms = std::clamp(std::stoi(value), 5, 60000); } catch (...) {}
            }
            else if (key == "item_memory_pct")
            {
                try { s.item_memory_pct = std::clamp(std::stoi(value), 5, 90); } catch (...) {}
            }
            else if (key == "model_cache_mb")
            {
                try { s.model_cache_mb = std::max(std::stoi(value), 8); } catch (...) {}
            }
        }
        else if (section == "controls")
        {
            if (key == "mouse_sensitivity")
            {
                try { s.mouse_sensitivity = std::clamp(std::stoi(value), 10, 400); } catch (...) {}
            }
            else if (key == "invert_mouse_y") s.invert_mouse_y = on_off(value, false);
            else if (key == "read_aloud") s.read_aloud = on_off(value, false);
            else if (key == "text_to_voice") s.text_to_voice = on_off(value, false);
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
        {"graphics", "edge_glow", on_off(edge_glow), "Geometry Edge Glow (only one of it and Real Graphics is on)"},
        {"graphics", "real_graphics", on_off(real_graphics), "Real Graphics (only one of it and Geometry Edge Glow is on)"},
        {"graphics", "door_portals", on_off(door_portals), "Door Portals: procedural data noise in the doorways"},
        {"graphics", "graphics_memory_gb", std::to_string(graphics_memory_gb), "the graphics card's memory, for the setup menu's budget, 1 to 128"},
        {"graphics", "view_rooms", std::to_string(view_rooms), "View Distance: rooms drawn and kept either side of yours, 2 to 64"},
        {"graphics", "picture_rooms", std::to_string(picture_rooms), "Picture Distance: rooms either side of yours with item pictures of their own, 0 to 8"},
        {"graphics", "fps_counter", on_off(fps_counter), "frames per second, top right of the hallway"},
        {"world", "model_cache_mb", std::to_string(model_cache_mb), "the display cache: memory for the pictures on items, 8 MB up"},
        {"world", "angle_decimals", std::to_string(angle_decimals), "decimal places on the compass's degree readout, 0 to " + std::to_string(kMaxAngleDecimals)},
        {"world", "filter_memory_mb", std::to_string(filter_memory_mb), "the most one count of the filters' survivors may take for its tables, 64 MB up"},
        {"world", "counting_memory_pct", std::to_string(counting_memory_pct), "percent of installed memory the setup menu's counts may take at once, 5 to 100"},
        {"world", "merge_cache_pct", std::to_string(merge_cache_pct), "percent of the filter memory kept for merged filters between counts, 0 to 100"},
        {"world", "unit_time_ms", std::to_string(unit_time_ms), "the time budget: the longest one unit may take to open, 5 to 60000 ms"},
        {"world", "item_memory_pct", std::to_string(item_memory_pct), "percent of installed memory the items around you may take, 5 to 90"},
        {"controls", "mouse_sensitivity", std::to_string(mouse_sensitivity), "percent, 10 to 400"},
        {"controls", "invert_mouse_y", on_off(invert_mouse_y), "on: moving the mouse up looks down"},
        {"controls", "read_aloud", on_off(read_aloud), "on: the item chosen in a menu is read aloud by the system's voice (Windows)"},
        {"controls", "text_to_voice", on_off(text_to_voice), "on: the AI's replies in the CHAT tab are spoken aloud by the system's voice (Windows), until Sieve's voice model speaks them"},
        {"models", "text_model", text_model, "a model folder CHAT talks with; empty: the model in hand"},
        {"models", "picture_model", picture_model, "a picture model's run folder DRAW uses; empty: the first trained one"},
        {"models", "widen_weights", widen_weights == 2 ? "auto" : on_off(widen_weights == 1), "on: a model file's BF16 and F16 weights widened as they load (twice the memory); off: as stored; auto: widened where the widened model fits in ITEM MEMORY's share of installed memory; the same numbers either way"},
        {"locator", "search_seconds", seconds_text(locator_search), "the File Locator's search for shorter addresses: seconds, 0 for no limit"},
        {"locator", "search_for", locator_search_run ? "run" : "file", "file: those seconds for each file; run: for the whole run, shared among its files"},
        {"locator", "optimise_archives", (locator_temporary || locator_permanent) ? "1" : "0", "1: either half of Archive and compressed formats ticked (as Archive and Compressed Formats, the one box before v31, read it: both); the next two say which; 0: off"},
        {"locator", "compressed_temporary", locator_temporary ? "1" : "0", "1: Archive and compressed formats: temporary ticked (an archive or a compressed file made again exactly on installing, where Sieve can); 0: off"},
        {"locator", "compressed_permanent", locator_permanent ? "1" : "0", "1: Archive and compressed formats: permanent ticked (an archive or a compressed file carried as what it holds, installed so); 0: off"},
        {"locator", "sound", locator_sound, "Sound as FLAC (with the v34 draft on). temporary (the default): only what Sieve makes again byte for byte on installing (a WAV, AIFF, RF64, Wave64, .au or CAF as its samples coded by FLAC; an MP3 as packMP3 codes it), an archive's sound included; any other sound carried as its bytes. permanent: also installed as other bytes of the same samples where smaller (those, WavPack, Monkey's Audio, TTA and ALAC as name.flac, through ffmpeg for the last four; a FLAC re-coded). off: sound left to the ways every file has"},
        {"locator", "optimise_search", locator_optimise ? "1" : "0", "1: Optimise Search ticked (the default): Try every way chosen for each file by its kind, from what was measured (off for every file with Priority: Decoding speed; else on for a JPEG, a file over 16 MiB or under 4 KiB, off between), its own box unused; 0: Try every way as its box says"},
        {"locator", "search_every_wave", locator_every_wave ? "1" : "0", "1: Try every way ticked: the search's shortcuts off (the rope through every wave, even on text and on bytes already packed; mixing tried on every file, whole); 0: off"},
        {"locator", "priority", locator_priority, "size: the smallest; balanced: mixing only where it saves 4% of what it carries (the default); speed: no file mixed (quickest to install, largest)"},
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
