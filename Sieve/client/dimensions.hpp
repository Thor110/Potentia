// Sieve hallway — the dimensions: the one list every part of the client reads them from.
//
// Each dimension is defined once, here: what it is, its names, its two colours, and the unit line
// it is, if it is one. The table's order is the order of the doors, so reordering the corridor is
// reordering this table and nothing else.
//
// A dimension has two numbers, and they are kept apart:
//   - its door, `li` (0 .. kLines - 1): where it stands in the corridor, which is all that walking,
//     the map's columns, the HUD's rings and the key that moves a fifth at each door go by;
//   - its identity, `Media`: what it is, whatever door it stands at. Anything configured or saved per
//     dimension (filters, music modes, settings) is keyed by this, or by its id, never by a door.
#pragma once

#include "theme.hpp"

#include "cli/lines.hpp"

#include <iterator>
#include <optional>
#include <string_view>

namespace hallway {

// The kinds of media on the shelves: a dimension's identity. This order is the order of anything
// saved as a list per dimension (the music settings' modes), so it only ever grows at the end.
enum class Media { Pages, Image, Audio, Video, Books, Models, Binary };
inline constexpr int kMedia = 7;

struct Dimension
{
    Media media;
    const char* id;   // "pages": the music files, the setup menu's focus, Real Graphics' folders
    const char* line; // the --line name and the setup menu's start line ("text" for pages)
    std::optional<sieve::cli::LineKind> unit; // the unit line it is (text, image, audio or video), if it is one
    Theme theme;      // its two colours: a solid background and the colour of every edge (SPECIFICATIONS §5.4)
    int music_mode;   // WORLD's mode on it until changed in the media player (music.hpp: 0 lydian ... 6 locrian)
};

using sieve::cli::LineKind;

// In door order. Walking left goes to the next and right to the previous, and binary, last, wraps
// round the outside of the rest: the corridor runs binary | pages ... models | binary.
inline constexpr Dimension kDimensions[] = {
    {Media::Pages, "pages", "text", LineKind::Text, {{0, 0, 0, 255}, {255, 255, 255, 255}, "PAGES", "PAGES", "line.pages"}, 1},
    {Media::Image, "image", "image", LineKind::Image, {{0, 0, 140, 255}, {0, 255, 255, 255}, "IMAGE", "IMAGE", "line.image"}, 0},
    {Media::Audio, "audio", "audio", LineKind::Audio, {{0, 90, 0, 255}, {255, 176, 0, 255}, "AUDIO", "AUDIO", "line.audio"}, 2},
    {Media::Video, "video", "video", LineKind::Video, {{140, 0, 0, 255}, {255, 255, 0, 255}, "VIDEO", "VIDEO", "line.video"}, 3},
    // Books (SPECIFICATIONS §11): a cover from the image line, a title and pages from the pages line.
    // Grey, with black edges.
    {Media::Books, "books", "books", std::nullopt, {{150, 150, 150, 255}, {0, 0, 0, 255}, "BOOKS", "BOOKS", "line.books"}, 4},
    // Models (SPECIFICATIONS §12): clay behind green wireframe, because a mesh is drawn as lines and
    // a mesh is modelled in clay.
    {Media::Models, "models", "models", std::nullopt, {{140, 120, 0, 255}, {0, 255, 0, 255}, "MODELS", "MODELS", "line.models"}, 5},
    // Binary (SPECIFICATIONS §12.1): black, with green edges and green text. What is unusual about it
    // is its shape, not its palette: one wall of shelves, and on the other side the edge and the drop.
    {Media::Binary, "binary", "binary", std::nullopt, {{0, 0, 0, 255}, {32, 220, 80, 255}, "BINARY", "BINARY", "line.binary"}, 6},
};
inline constexpr int kLines = int(std::size(kDimensions));
static_assert(kLines == kMedia, "every medium has a door");

constexpr int line_of(Media m)
{
    for (int i = 0; i < kLines; ++i)
        if (kDimensions[i].media == m) return i;
    return -1;
}
constexpr int line_of(LineKind k)
{
    for (int i = 0; i < kLines; ++i)
        if (kDimensions[i].unit == k) return i;
    return -1;
}
// A door from a name: an id ("pages") or a --line name ("text"); -1 if neither.
constexpr int line_named(std::string_view name)
{
    for (int i = 0; i < kLines; ++i)
        if (name == kDimensions[i].id || name == kDimensions[i].line) return i;
    return -1;
}

inline constexpr int kBooksLine = line_of(Media::Books), kModelsLine = line_of(Media::Models), kBinaryLine = line_of(Media::Binary);
static_assert(kBinaryLine == kLines - 1, "binary is last: the corridor starts and ends at it");
static_assert(line_of(LineKind::Text) >= 0 && line_of(LineKind::Image) >= 0 && line_of(LineKind::Audio) >= 0 && line_of(LineKind::Video) >= 0,
              "every unit line has a door");

constexpr const Theme& theme_of(int li) { return kDimensions[li].theme; }
constexpr const Theme& theme_of(Media m) { return kDimensions[line_of(m)].theme; }
inline constexpr SDL_Color kEdgeInk = theme_of(Media::Binary).edge; // the rain off the edge

// The colour to write a line's name in on black: its edge colour, or its background colour when
// the edges are too dark to read (books: black edges on grey).
constexpr SDL_Color menu_ink(const Theme& th)
{
    const int lum = (th.edge.r * 3 + th.edge.g * 6 + th.edge.b) / 10;
    return lum < 60 ? th.bg : th.edge;
}

} // namespace hallway
