// Sieve hallway — a panel that slides in from an edge, as the setup menu's settings do (Edward, 13
// November 2026: "all sliding windows should work the same way as they do in the sieve setup menu
// throughout the system").
//
// One behaviour everywhere: Tab (or the screen's own key) sends it in or out over kSlideMs; while it is
// out, a tab stands at its edge with its name written down it, and a click on that tab brings it back.
// The setup menu's settings panel slides at this speed with this tab (menu.cpp); the personal world's
// catalogue and Place panels are SlidePanels. A panel is a small piece of state, so screens that use it
// can be arranged, taken out or put elsewhere by a person's own layout later (IDEAS 19.196).
#pragma once

#include "font.hpp"

#include <SDL3/SDL.h>

#include <algorithm>
#include <string>
#include <vector>

namespace hallway {

inline constexpr float kSlideMs = 180;  // the time to slide all the way in or out
inline constexpr float kSlideTabW = 20; // the tab at the edge while it is out

struct SlidePanel
{
    float shown = 0;   // as drawn: 0 out of sight, 1 in its place
    bool open = false; // where it is sliding to
    Uint64 tick = 0;
    SDL_FRect tab{};   // its tab, while it is out (to be clicked)

    void toggle() { open = !open; }
    // Each frame: a step of the slide, by the time since the last.
    void step()
    {
        const Uint64 now = SDL_GetTicksNS();
        const float d = tick ? float(double(now - tick) / 1e6) / kSlideMs : 0.0f;
        tick = now;
        shown = open ? std::min(1.0f, shown + d) : std::max(0.0f, shown - d);
    }
    // All the way to where it is going (a picture taken at once).
    void settle() { shown = open ? 1.0f : 0.0f; }
    // Its left edge, for a panel `width` wide sliding in from the left.
    float left(float width) const { return -(1 - shown) * width; }
    bool clicked(float x, float y) const { return tab.w > 0 && x >= tab.x && x < tab.x + tab.w && y >= tab.y && y < tab.y + tab.h; }
    // Its tab at the left edge, `y` down, while it is mostly out: its name, one letter under another.
    void draw_tab(SDL_Renderer* r, float y, const std::string& word)
    {
        tab = {};
        if (shown >= 0.5f) return;
        std::vector<std::string> letters;
        for (size_t i = 0; i < word.size();)
        {
            size_t j = i + 1;
            while (j < word.size() && (uint8_t(word[j]) & 0xC0) == 0x80) ++j; // one UTF-8 character
            letters.push_back(word.substr(i, j - i));
            i = j;
        }
        tab = {2, y, kSlideTabW - 4, float(letters.size()) * 12 + 8};
        SDL_SetRenderDrawColor(r, 255, 255, 255, 30);
        SDL_RenderFillRect(r, &tab);
        SDL_SetRenderDrawColor(r, 150, 150, 150, 255);
        SDL_RenderRect(r, &tab);
        for (size_t k = 0; k < letters.size(); ++k) draw_text(r, tab.x + (tab.w - 8) / 2, tab.y + 4 + float(k) * 12, letters[k], 1, SDL_Color{150, 150, 150, 255});
    }
};

} // namespace hallway
