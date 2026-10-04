// How wide an item's display (the picture printed on its front) is drawn, from what is on it.
//
// One width for every line suits none of them: an image of 10x10 pixels needs little, a title of
// 32 letters a little more, and a page of 400 letters far more, since each letter needs a few
// pixels of its own before it can be drawn as a letter rather than a dash. So the display size
// setting is the least width, and a line whose items carry text is drawn wider, as wide as its
// letters need to reach the letter size setting, up to the widest the display cache can hold
// (widest_display_px). The hallway uses this to draw its displays and the setup menu to say what
// they will cost, so the two always agree.
#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>

namespace hallway {

// The width at which `chars` letters, laid out in a share `share` of a display `aspect` times
// as tall as it is wide, come out at `letters` pixels each. Laid out a letter to a square cell,
// the cells are sqrt(area / chars) across, and the area is w * (w * aspect * share); a fifth is
// added for the margins and for rows and columns coming out whole.
inline double width_for_letters(double chars, double share, double aspect, double letters)
{
    if (chars <= 0 || share <= 0 || aspect <= 0) return 0;
    return letters * std::sqrt(chars / (share * aspect)) * 1.2;
}

// What a line's items carry: a body of text (a page) and a title, or neither.
struct DisplayText
{
    double body = 0;       // letters in the body (a page's length), 0 for a picture or a mesh
    double body_share = 0; // how much of the height the body has
    double title = 0;      // letters in the title, 0 without one
};

// The widest a line's displays may be drawn: the widest power of two at which all `pictures` of
// them (the rooms with pictures, hallway.hpp face_rooms) fit the display cache's `cache_bytes`,
// and no wider than `texture_px`, the renderer's largest texture (gpu::max_texture_px). Where a
// picture of 1,024 pixels was the limit, a long page's display now widens as far as the cache
// lets every picture around you be drawn at it, and no further.
inline int widest_display_px(double cache_bytes, double pictures, double aspect, int texture_px)
{
    int px = 16;
    while (px <= texture_px / 2)
    {
        const double w = 2.0 * px;
        if (pictures * w * std::ceil(w * aspect) * 4.0 > cache_bytes) break;
        px *= 2;
    }
    return px;
}

// The width to draw a line's displays at: the display size setting, or wider for its letters,
// rounded up to a power of two (as the setting is) and no wider than `widest_px`
// (widest_display_px), which only ever holds the letters back, never the setting.
inline int display_px(int setting_px, int letters_px, double aspect, const DisplayText& t, int widest_px)
{
    // A title gets up to half the height when a third is too little (item_faces.cpp).
    const double need = std::max(width_for_letters(t.body, t.body_share, aspect, letters_px),
                                 width_for_letters(t.title, 0.5, aspect, letters_px));
    int px = std::max(16, setting_px);
    while (px < need && px <= widest_px / 2) px *= 2;
    return px;
}

// What each line's items carry. The pages line: a page, with a title across its top. The books
// line: its title is a whole page, set in the title band. The rest: a title only.
inline DisplayText display_text_pages(double page_chars, double title_chars) { return {page_chars, 0.6, title_chars}; }
inline DisplayText display_text_books(double page_chars) { return {0, 0, page_chars}; }
inline DisplayText display_text_titled(double title_chars) { return {0, 0, title_chars}; }

// Close-ups (hallway.hpp sharp_): the displays nearest you drawn again, as wide as they show on
// screen, up to the close-up size setting. 0 turns them off; kCloseUpScreen, the default, follows
// the screen (saved as "screen"); anything else is that many pixels.
inline constexpr uint32_t kCloseUpScreen = 1;
inline uint32_t closeup_setting(const std::string& v, uint32_t otherwise)
{
    if (v == "screen") return kCloseUpScreen;
    try { return uint32_t(std::stoul(v)); } catch (...) { return otherwise; }
}
inline std::string closeup_text(uint32_t setting) { return setting == kCloseUpScreen ? "screen" : std::to_string(setting); }

// The widest a close-up is drawn: the setting, or for kCloseUpScreen the power of two that covers
// the screen's width, since a display that shows wider than the screen shows no more of itself;
// never wider than texture_px, the renderer's widest texture. 2048 at 1920 x 1080, 4096 at 4K.
inline int closeup_width(uint32_t setting, int screen_w, int texture_px)
{
    if (setting == 0) return 0;
    int px = 256;
    if (setting != kCloseUpScreen) px = int(std::min<uint32_t>(setting, uint32_t(texture_px)));
    else
        while (px < screen_w && px <= texture_px / 2) px *= 2;
    return std::min(px, texture_px);
}

// How many close-ups are kept: as many at their widest as the graphics memory left beside the
// world and the display cache (`room_bytes`) holds, at least one, and no more than a room's items.
// They were 24, whatever the memory.
inline size_t closeup_count(double room_bytes, int width, double aspect, size_t room_items)
{
    if (width <= 0) return 0;
    const double one = double(width) * std::ceil(double(width) * aspect) * 4.0;
    const double fit = room_bytes > 0 ? std::floor(room_bytes / one) : 0.0;
    return std::clamp<size_t>(size_t(std::min(fit, double(room_items))), 1, std::max<size_t>(1, room_items));
}

// What the close-ups can take at once, whatever their number: each is drawn under twice as wide as
// it shows (the power of two above that width), so under four times its area on screen, and the
// displays on screen do not cover one another, so together under four screens; twice that for the
// ones kept a couple of seconds after they go out of view.
inline double closeup_bytes_most(int screen_w, int screen_h) { return 2.0 * 4.0 * double(screen_w) * double(screen_h) * 4.0; }

} // namespace hallway
