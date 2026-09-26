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

// The models line (SPECIFICATIONS §12): clay behind green wireframe, because a mesh is drawn
// as lines and a mesh is modelled in clay.
inline constexpr Theme kModelsTheme = {{140, 120, 0, 255}, {0, 255, 0, 255}, "MODELS", "MODELS", "line.models"};

// The books line (SPECIFICATIONS §11): a cover from the image line, a title and pages from the
// pages line.
inline constexpr Theme kBooksTheme = {{150, 150, 150, 255}, {0, 0, 0, 255}, "BOOKS", "BOOKS", "line.books"}; // grey, black edges

// The binary line (SPECIFICATIONS §12.1): black, with green edges and green text. It is an
// ordinary line with two colours like any other; what is unusual about it is its shape, not its
// palette — one wall of shelves, and on the other side the edge and the drop.
inline constexpr Theme kBinaryTheme = {{0, 0, 0, 255}, {32, 220, 80, 255}, "BINARY", "BINARY", "line.binary"};
inline constexpr SDL_Color kEdgeInk = kBinaryTheme.edge; // the rain off the edge

} // namespace hallway
