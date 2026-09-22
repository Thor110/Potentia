#include "app_settings.hpp"

#include "cli/dictionaries.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>
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
    if (on) edge_glow = false;
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
        // Unknown or malformed entries are ignored: this file only holds preferences.
        if (section == "graphics")
        {
            if (key == "resolution")
            {
                int w = 0, h = 0;
                if (std::sscanf(value.c_str(), "%dx%d", &w, &h) == 2 && w >= 320 && h >= 200 && w <= 16384 && h <= 16384) s.resolution = {w, h};
            }
            else if (key == "fullscreen") s.fullscreen = on_off(value, false);
            else if (key == "vsync") s.vsync = on_off(value, true);
            else if (key == "edge_glow") s.edge_glow = on_off(value, false);
            else if (key == "real_graphics") s.real_graphics = on_off(value, false);
            else if (key == "fps_counter") s.fps_counter = on_off(value, false);
        }
        else if (section == "controls")
        {
            if (key == "mouse_sensitivity")
            {
                try { s.mouse_sensitivity = std::clamp(std::stoi(value), 10, 400); } catch (...) {}
            }
            else if (key == "invert_mouse_y") s.invert_mouse_y = on_off(value, false);
        }
        else if (section == "language" && key == "language" && !value.empty()) s.language = value;
    }
    if (s.edge_glow && s.real_graphics) s.real_graphics = false; // they exclude each other
    return s;
}

bool AppSettings::save(const fs::path& path) const
{
    std::ostringstream o;
    o << "; Sieve hallway settings, edited by the main menu's Settings (hand edits are welcome).\n"
      << "\n[graphics]\n"
      << "resolution = " << (resolution.w > 0 ? resolution.str() : std::string()) << "\n"
      << "fullscreen = " << on_off(fullscreen) << "\n"
      << "vsync = " << on_off(vsync) << "   ; off: frames as fast as they can be drawn (may tear)\n"
      << "edge_glow = " << on_off(edge_glow) << "       ; Geometry Edge Glow\n"
      << "real_graphics = " << on_off(real_graphics) << "   ; Real Graphics (only one of the two is on)\n"
      << "fps_counter = " << on_off(fps_counter) << "\n"
      << "\n[controls]\n"
      << "mouse_sensitivity = " << mouse_sensitivity << "   ; percent\n"
      << "invert_mouse_y = " << on_off(invert_mouse_y) << "\n"
      << "\n[language]\n"
      << "language = " << language << "\n";
    std::ofstream out(path, std::ios::binary);
    out << o.str();
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

} // namespace hallway
