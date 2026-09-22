// Sieve hallway — the two colours of each line (SPECIFICATIONS §5.4).
#pragma once

#include <SDL3/SDL.h>

namespace hallway {

struct Theme
{
    SDL_Color bg, edge;
    const char* name;  // in the hallway ("PAGES LINE"); the text line's units are pages
    const char* title; // in the setup menu
    const char* key;   // its language key ("line.pages"): the name shown, in the chosen language
};

// Two colours per line: a solid background and the colour of every edge. Doors are solid black.
// Order: text, image, audio, video (the order of the doors).
inline constexpr Theme kThemes[4] = {
    {{0, 0, 0, 255}, {255, 255, 255, 255}, "PAGES", "PAGES", "line.pages"},
    {{0, 0, 140, 255}, {0, 255, 255, 255}, "IMAGE", "IMAGE", "line.image"},
    {{0, 90, 0, 255}, {255, 176, 0, 255}, "AUDIO", "AUDIO", "line.audio"},
    {{140, 0, 0, 255}, {255, 255, 0, 255}, "VIDEO", "VIDEO", "line.video"},
};

// The books line (SPECIFICATIONS §11): a cover from the image line, a title and pages from the
// pages line.
inline constexpr Theme kBooksTheme = {{150, 150, 150, 255}, {0, 0, 0, 255}, "BOOKS", "BOOKS", "line.books"}; // grey, black edges

} // namespace hallway
