// Sieve's icon on a window's title bar and task bar entry, from the pixels compiled into the
// program (icon_pixels.h, written by tools/make_icon.py), so it needs no file beside the program.
// On Windows the programs also carry the icon as a resource (data/icons/sieve.rc.in), which is
// what Explorer shows for the file itself.
#pragma once

#include "icon_pixels.h"

#include <SDL3/SDL.h>

inline void set_window_icon(SDL_Window* window)
{
    if (!window) return;
    SDL_Surface* s = SDL_CreateSurfaceFrom(kSieveIconSize, kSieveIconSize, SDL_PIXELFORMAT_RGBA32,
                                           const_cast<unsigned char*>(kSieveIcon), kSieveIconSize * 4);
    if (!s) return;
    SDL_SetWindowIcon(window, s);
    SDL_DestroySurface(s);
}
