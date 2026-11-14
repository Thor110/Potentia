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
#pragma once

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

} // namespace hallway
