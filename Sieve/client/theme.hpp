// Sieve hallway — the two colours of each line (SPECIFICATIONS §5.4).
#pragma once

#include <SDL3/SDL.h>

namespace hallway {

struct Theme
{
    SDL_Color bg, edge;
    const char* name;  // in the hallway ("PAGES LINE"); the text line's units are pages
    const char* title; // in the setup menu
};

// Two colours per line: a solid background and the colour of every edge. Doors are solid black.
// Order: text, image, audio, video (the order of the doors).
inline constexpr Theme kThemes[4] = {
    {{0, 0, 0, 255}, {255, 255, 255, 255}, "PAGES", "PAGES"},
    {{0, 0, 140, 255}, {0, 255, 255, 255}, "IMAGE", "IMAGE"},
    {{0, 90, 0, 255}, {255, 176, 0, 255}, "AUDIO", "AUDIO"},
    {{140, 0, 0, 255}, {255, 255, 0, 255}, "VIDEO", "VIDEO"},
};

// The planned books line (SPECIFICATIONS §11): works made of units of one line above. Not yet in
// the corridor; its colours are fixed here so they are settled before it arrives.
inline constexpr Theme kBooksTheme = {{150, 150, 150, 255}, {0, 0, 0, 255}, "BOOKS", "BOOKS"}; // grey, black edges

} // namespace hallway
