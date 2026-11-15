// Sieve hallway — the one footer every screen draws its keys through (IDEAS 19.224).
//
// A screen with keys or descriptions along its foot hands them here as one or more lines, each a list of
// entries apart by two spaces or more, with the room it has. They are laid out by sieve::wrap_footer
// (sieve/footer_wrap.hpp) in the font's own widths, broken only between entries, and drawn with the last
// line where the screen always drew it and any further lines above, so a key added later wraps onto a
// line above rather than off the side of the window, and nothing has to be shortened to fit. A footer
// that fits is drawn exactly as before.
//
// Each footer drawn is noted under the screen's name, so the readout can say (with --footers) how many
// lines it took and how wide the widest was against the window: CI checks at a narrow window that none
// runs off the side.
#pragma once

#include <SDL3/SDL.h>

#include <cstddef>
#include <ostream>
#include <string>
#include <vector>

namespace hallway {

// A list of keys laid out in lines of at most `width` characters (one byte each), as the viewer had
// it before every footer was measured in the font: kept for any caller that counts characters.
std::vector<std::string> wrap_keys(const std::string& s, size_t width);

// One line of a footer as written, and its colour.
struct FooterLine
{
    std::string text;
    SDL_Color colour;
};

// Where a footer's lines stand against the x given: from it rightwards, or centred on it.
enum class FooterAlign { Left, Centre };

// A footer's lines as they will be drawn, top first, each in its line's colour: every line laid out in
// `room` pixels at `scale`.
std::vector<FooterLine> footer_rows(const std::vector<FooterLine>& lines, float room, float scale = 1);

// How much taller than written a footer comes out in `room`: its extra rows times `pitch`. A screen
// moves what stands just above its footer up by this, so a wrapped footer never draws over it.
float footer_extra(const std::vector<FooterLine>& lines, float room, float pitch, float scale = 1);

// Draws a footer: its last row with its top at `y` (where the screen drew its last line before), each
// row before it `pitch` higher, from `x` (or centred on it), in `room` pixels at `scale`. Noted under
// `screen` for the readout. Returns how many rows it drew.
size_t draw_footer(SDL_Renderer* r, const char* screen, float x, float y, float room, const std::vector<FooterLine>& lines, float pitch,
                   FooterAlign align = FooterAlign::Left, float scale = 1);
// The same for a footer of one line.
size_t draw_footer(SDL_Renderer* r, const char* screen, float x, float y, float room, const std::string& text, SDL_Color colour, float pitch = 12,
                   FooterAlign align = FooterAlign::Left, float scale = 1);

// --footers: each footer drawn since the last picture said as "footer NAME: N lines, widest W of window
// X (room R)", widths in pixels, in the order the screens were first drawn; then forgotten, so the next
// picture says its own. Off unless asked, so the readout is otherwise as it was.
void set_footer_report(bool on);
bool footer_report();
void say_footers(std::ostream& out);

} // namespace hallway
