// Sieve hallway — the two colours of each line (SPECIFICATIONS §5.4).
#pragma once

#include <SDL3/SDL.h>

namespace hallway {

struct Theme
{
    SDL_Color bg, edge;
    const char* name;
};

// Two colours per line: a solid background and the colour of every edge. Doors are solid black.
// Order: text, image, audio, video (the order of the doors).
inline constexpr Theme kThemes[4] = {
    {{0, 0, 0, 255}, {255, 255, 255, 255}, "TEXT"},
    {{0, 0, 140, 255}, {0, 255, 255, 255}, "IMAGE"},
    {{0, 90, 0, 255}, {255, 176, 0, 255}, "AUDIO"},
    {{140, 0, 0, 255}, {255, 255, 0, 255}, "VIDEO"},
};

} // namespace hallway
