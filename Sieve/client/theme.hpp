// Sieve hallway — a line's two colours (SPECIFICATIONS §5.4); each line's own are in dimensions.hpp.
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

// Each line's two colours are in its entry in kDimensions (dimensions.hpp).

} // namespace hallway
