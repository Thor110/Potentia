// Sieve hallway — presses that name a menu row (IDEAS 19.188): `Item:ID` in --press.
//
// A script (the CI steps, a picture's --press) used to reach a menu's row by counting Down presses
// from the top, so moving or adding a row changed every script that passed it. `Item:ID` chooses the
// row of that name instead, as Up/Down would, wherever it stands: on the main menu's screens (the ids
// of MainMenu::items) and on the hallway's pause menu (pause.cpp's). Each name is given a key code of
// its own past every real key's, so it travels through --press like any other key; a menu that does
// not know the name refuses it, saying so.
#pragma once

#include <SDL3/SDL.h>

#include <string>
#include <vector>

namespace hallway {

inline std::vector<std::string>& item_press_names()
{
    static std::vector<std::string> names;
    return names;
}
inline constexpr SDL_Keycode kItemPressBase = SDLK_SCANCODE_MASK | 0xF000; // (to 0xFFFE: kClickPress is 0xFFFF)

// The key code for Item:ID, the same one each time the name is given.
inline SDL_Keycode item_press(const std::string& id)
{
    auto& names = item_press_names();
    for (size_t i = 0; i < names.size(); ++i)
        if (names[i] == id) return kItemPressBase + SDL_Keycode(i);
    names.push_back(id);
    return kItemPressBase + SDL_Keycode(names.size() - 1);
}
// The name a key code stands for, or null for a real key.
inline const std::string* item_press_id(SDL_Keycode key)
{
    const auto& names = item_press_names();
    if (key < kItemPressBase || key >= kItemPressBase + SDL_Keycode(names.size())) return nullptr;
    return &names[size_t(key - kItemPressBase)];
}

} // namespace hallway
