// Sieve — the pause menus' rows, as data (IDEAS 19.196: menus as data; Edward, 14 November 2026: "the
// personal dimensions should open a pause menu like the hallway ... setting one up specifically for the
// personal dimensions as a default it wouldn't include hallway tools, but they could be enabled by the
// user incase they are making an interpretation of sieve itself").
//
// One list of every row a pause menu can have, in the order they are shown, each saying where it is
// shown: in the hallway's pause menu, in your dimension's, and whether it is a hallway tool (one that
// needs a hallway walked: the setup menu, the node graph, the navigator). Your dimension leaves the
// hallway tools out unless Personal Dimensions > Features asks for them; chosen there, they go into
// the hallway and open there. A row's name is "pause.<id>" in the language files, and the line under
// the list "pause.help.<id>".
//
// Both menus are worked the same way, so how a row is pointed at and how the keys move the choice
// are here too.
#pragma once

#include <SDL3/SDL.h>

#include <string>
#include <vector>

namespace hallway {

struct PauseRow
{
    const char* id;
    bool hallway;      // in the hallway's pause menu
    bool personal;     // in your dimension's, as it is at first
    bool hallway_tool; // needs a hallway walked (in your dimension only where asked for)
};

inline constexpr PauseRow kPauseRows[] = {
    {"resume", true, true, false},     {"restart", true, false, true},  {"graph", true, false, true},
    {"navigator", true, false, true},  {"locator", true, true, false},  {"media", true, true, false},
    {"designer", true, true, false},   {"harness", true, true, false},  {"settings", true, true, false},
    {"cipher", true, true, false},     {"personal", true, true, false}, {"systems", true, true, false},
    {"leave", false, true, false},     {"exit", true, true, false},
};

enum class PausePlace { Hallway, Personal };

// The rows a pause menu shows, in order.
inline std::vector<std::string> pause_rows(PausePlace where, bool hallway_tools = false)
{
    std::vector<std::string> out;
    for (const PauseRow& r : kPauseRows)
        if (where == PausePlace::Hallway ? r.hallway : (r.personal || (hallway_tools && r.hallway_tool))) out.push_back(r.id);
    return out;
}

inline bool is_hallway_tool(const std::string& id)
{
    for (const PauseRow& r : kPauseRows)
        if (id == r.id) return r.hallway_tool;
    return false;
}

// The row under the pointer, for a mouse motion or button press, among the rows as last drawn
// (which do not overlap); -1 where it is over none.
// The row of that name in a pause menu's rows, or -1: the first, as both menus' pause_select take it
// (Item:ID presses).
inline int pause_row_named(const std::vector<std::string>& rows, const std::string& id)
{
    for (size_t i = 0; i < rows.size(); ++i)
        if (rows[i] == id) return int(i);
    return -1;
}

inline int pause_row_at(SDL_Renderer* r, const SDL_Event& e, const std::vector<SDL_FRect>& rows)
{
    float x = e.type == SDL_EVENT_MOUSE_MOTION ? e.motion.x : e.button.x;
    float y = e.type == SDL_EVENT_MOUSE_MOTION ? e.motion.y : e.button.y;
    SDL_RenderCoordinatesFromWindow(r, x, y, &x, &y);
    for (size_t i = 0; i < rows.size(); ++i)
    {
        const SDL_FRect& f = rows[i];
        if (x >= f.x && x < f.x + f.w && y >= f.y && y < f.y + f.h) return int(i);
    }
    return -1;
}

// The row a key moves the choice to among `n` rows: Up or W back one, Down or S on one, both
// wrapping round; any other key (or no rows) leaves it. `choose` is set for the keys that choose the
// row (Return, keypad Enter and Space). Escape is each menu's own.
inline int pause_row_step(SDL_Keycode k, int row, int n, bool& choose)
{
    choose = false;
    if (n == 0) return row;
    if (k == SDLK_UP || k == SDLK_W) return (row + n - 1) % n;
    if (k == SDLK_DOWN || k == SDLK_S) return (row + 1) % n;
    choose = k == SDLK_RETURN || k == SDLK_KP_ENTER || k == SDLK_SPACE;
    return row;
}

} // namespace hallway
