// Sieve hallway — bitmap fonts for every piece of on-screen text.
//
// A font is a file in the fonts folder (data/fonts while developing, fonts/ next to the
// executable), in GNU Unifont's .hex format: one glyph per line, "code point:rows in hex", top
// row first, leftmost pixel in the high bit. A "# height: N" line gives the glyph height (default
// 16, as in Unifont); the width follows from the length of the hex (8 or 16 pixels).
//
// Each language file names its font on its first line ("font = sieve8x8"). Text is laid out on
// the 8-pixel lines the menus were designed for: every glyph is drawn 8 * scale pixels tall,
// keeping its shape, so an 8x8 glyph is one 8-pixel cell wide and a 16x16 one (Unifont's CJK)
// too, while Unifont's 8x16 letters come out half a cell wide. The built-in font, sieve8x8,
// is 8x8 and covers ASCII,
// Latin-1 (the accented letters of most western European languages), Greek, box drawing and
// hiragana. A character the font lacks is drawn from the fallback font, else as '?'.
#pragma once

#include <SDL3/SDL.h>

#include <string>

namespace hallway {

// Loads a font by name (a file NAME.hex in the fonts folder). "debug" is SDL's built-in ASCII
// font. Returns false, keeping the current font, if there is no such file.
bool set_font(const std::string& name);
const std::string& font_name();

// Draws UTF-8 text with its top-left corner at (x, y), at `scale` (1 = 8-pixel cells).
void draw_text(SDL_Renderer* r, float x, float y, const std::string& utf8, float scale, SDL_Color c);
// Width in pixels at `scale`, and in 8-pixel cells (rounded up).
float text_width(const std::string& utf8, float scale);
size_t text_cells(const std::string& utf8);
// The longest prefix of `utf8` that fits in `cells` cells (never splits a character).
std::string fit_cells(const std::string& utf8, size_t cells);

// Frees the glyph textures; call before destroying the renderer.
void release_fonts();

} // namespace hallway
