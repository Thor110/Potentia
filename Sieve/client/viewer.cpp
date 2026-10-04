// The item viewer: the thing in your hands over the whole window, scrolled both ways and zoomed.
//
// The item page draws the thing at the page's size, so a long page stops at the foot of it and a
// picture is as large as the page allows. Z, or a click on the thing, opens it here instead, as
// large as you like:
//   - a page: the whole of it, a character to a cell, in rows as wide as the picture on the item
//     lays them out (item_faces.cpp paint_text), so what opens is the picture on the item, larger;
//   - a picture or a film: its pixels, one to one or as large as you like, a frame at a time;
//   - a book: the page open in it (N and B turn the pages here too);
//   - a track: its notes; a model: its .obj text; a file: all of it as a hex dump.
// Nothing is made larger than the window: text is drawn a row at a time, only the rows in view,
// and a picture is sampled into one texture the size of the view whenever the view moves.
//
// Keys: the wheel scrolls down and up, Shift and the wheel (or a sideways wheel) across, Ctrl and
// the wheel zooms about the pointer; dragging moves it. Arrows or WASD move it, PgUp and PgDn by a
// screen, Home and End to the top and the foot, + and - zoom (Shift: twice as far), 0 fits it to
// the window and 1 is one to one. N and B turn a book's page or a film's frame, Space plays or
// stops a film. Esc or Z closes it.

#include "hallway.hpp"
#include "gpu_memory.hpp"

#include <algorithm>
#include <cmath>

namespace hallway::hall {

namespace {

// A character's cell at zoom 1: the font's 8 pixels across, and a line's 10 down (font.hpp).
constexpr float kCellW = 8, kCellH = 10;
// The bands above the view (what is open, the zoom, where you are) and below it (the keys).
constexpr float kHead = 40, kFoot = 22;

// The columns of `n` characters laid out a character to a square cell in a rectangle `aspect`
// times as tall as it is wide, as paint_text lays out a page on an item: the fewest whose rows,
// as many as the rectangle holds, take every character.
size_t page_columns(size_t n, double aspect)
{
    if (n == 0) return 1;
    aspect = std::max(aspect, 1e-3);
    size_t c = std::max<size_t>(1, size_t(std::sqrt(double(n) / aspect)));
    while (double(c) * std::floor(double(c) * aspect) < double(n)) ++c;
    return c;
}

// How much of `text` is writing: the padding at its end is not (paint_text leaves it off too).
size_t written_length(const std::u32string& text)
{
    size_t n = text.size();
    while (n > 0 && (text[n - 1] == U' ' || text[n - 1] == 0)) --n;
    return n;
}

// `text` in rows as an item's picture lays it out: the columns of its writing in a page of
// `aspect`, a new row at each line break.
std::vector<std::u32string> page_rows(const std::u32string& text, double aspect, size_t& cols)
{
    const size_t n = written_length(text);
    cols = page_columns(n, aspect);
    std::vector<std::u32string> rows(1);
    for (size_t i = 0; i < n; ++i)
    {
        if (text[i] == U'\n')
        {
            rows.emplace_back();
            continue;
        }
        if (rows.back().size() >= cols) rows.emplace_back();
        rows.back() += text[i] == 0 ? U' ' : text[i];
    }
    return rows;
}

std::vector<std::u32string> utf32_rows(const std::vector<std::string>& lines)
{
    std::vector<std::u32string> rows;
    rows.reserve(lines.size());
    for (const std::string& l : lines) rows.push_back(utf8_decode(l));
    return rows;
}

// The hex dump's row width: the offset, sixteen bytes and the sixteen characters beside them, as
// the item page shows a file (hud.cpp draw_file).
constexpr size_t kHexPer = 16, kHexCols = 6 + 2 + 3 * kHexPer + 1 + kHexPer;

} // namespace

// What the view shows, and where on it: the area below the heading and above the keys.
SDL_FRect Hallway::view_area() const
{
    int w = 0, h = 0;
    SDL_GetCurrentRenderOutputSize(r_, &w, &h);
    return {0, kHead, float(w), std::max(1.0f, float(h) - kHead - kFoot)};
}

// The thing's size at zoom 1: its characters' cells, or its pixels.
float Hallway::view_content_w() const { return view_.picture ? float(view_.f.width) : float(view_.cols) * kCellW; }
float Hallway::view_content_h() const { return view_.picture ? float(view_.f.height) : float(view_.row_count) * kCellH; }

// One row of the text: laid out already, or (a file) worked out from its bytes as it is shown.
std::u32string Hallway::view_row(size_t r) const
{
    if (view_.bytes.empty()) return r < view_.rows.size() ? view_.rows[r] : std::u32string{};
    const size_t at = r * kHexPer;
    if (at >= view_.bytes.size()) return {};
    char b[24];
    std::snprintf(b, sizeof b, "%06zx  ", at);
    std::string row = b, chars;
    for (size_t i = at; i < at + kHexPer; ++i)
        if (i < view_.bytes.size())
        {
            std::snprintf(b, sizeof b, "%02x ", view_.bytes[i]);
            row += b;
            chars += view_.bytes[i] >= 0x20 && view_.bytes[i] < 0x7F ? char(view_.bytes[i]) : '.';
        }
        else row += "   ";
    return utf8_decode(row + " " + chars);
}

void Hallway::view_book_page()
{
    const uint32_t n = books_->pages();
    view_.heading = trf("view.book", {title_text(*in_hand_), std::to_string(book_page_ + 1), std::to_string(n)});
    if (n == 0)
    {
        view_.rows = {utf8_decode(tr("hand.no_pages"))};
        view_.cols = view_.rows[0].size();
    }
    else
    {
        view_.rows = page_rows(line().space.text_of(in_hand_->parts->pages[size_t(book_page_)]), view_aspect_, view_.cols);
    }
    view_.row_count = view_.rows.size();
}

// The zoom at which the whole thing fits the view, and the least and most it may be zoomed: down to
// half of that (or of one to one, if that is less), and up to where a pixel or a letter fills a
// good part of the view.
float Hallway::view_fit_zoom() const
{
    const SDL_FRect a = view_area();
    return std::min(a.w / std::max(1.0f, view_content_w()), a.h / std::max(1.0f, view_content_h()));
}

void Hallway::view_set_zoom(float z)
{
    const SDL_FRect a = view_area();
    const float least = std::min(view_fit_zoom(), 1.0f) * 0.5f;
    const float most = std::max(least, std::min(a.w, a.h) / (view_.picture ? 4.0f : 4.0f * kCellH));
    view_zoom_ = std::clamp(z, least, most);
}

// Zoom by `factor`, keeping the point under (sx, sy) on the screen where it is.
void Hallway::view_zoom_at(float factor, float sx, float sy)
{
    const SDL_FRect a = view_area();
    const float cx = view_x_ + (sx - a.x) / view_zoom_, cy = view_y_ + (sy - a.y) / view_zoom_;
    view_set_zoom(view_zoom_ * factor);
    view_x_ = cx - (sx - a.x) / view_zoom_;
    view_y_ = cy - (sy - a.y) / view_zoom_;
}

// Inside the thing, or centred on it where it is smaller than the view.
void Hallway::view_clamp()
{
    const SDL_FRect a = view_area();
    auto one = [](float& o, float content, float shown) {
        o = content <= shown ? -(shown - content) * 0.5f : std::clamp(o, 0.0f, content - shown);
    };
    one(view_x_, view_content_w(), a.w / view_zoom_);
    one(view_y_, view_content_h(), a.h / view_zoom_);
    view_dirty_ = true;
}

void Hallway::open_viewer()
{
    if (!in_hand_ || withheld(*in_hand_)) return;
    const Book& bk = *in_hand_;
    view_ = ViewDoc{};
    // A page opens shaped as the picture on a page is: the pages line's display, less its margins.
    view_aspect_ = 1.0;
    {
        const double a = double(load_face_rect("pages").aspect());
        view_aspect_ = std::max(0.1, (a - 0.125) / (1.0 - 0.125)); // a sixteenth of the width either side
    }
    const std::string title = title_text(bk);
    const std::string named = title.empty() ? std::string() : "\"" + title + "\"   ";
    if (bk.model)
    {
        view_.rows = utf32_rows(split_lines(model_space_->to_obj(*bk.model)));
        view_.heading = named + tr("view.model");
    }
    else if (bk.parts) view_book_page();
    else if (bk.is_file)
    {
        view_.bytes = file_of(bk);
        view_.row_count = std::max<size_t>(1, (view_.bytes.size() + kHexPer - 1) / kHexPer);
        view_.cols = kHexCols;
        view_.heading = named + trf("hand.binary.bytes", {std::to_string(view_.bytes.size())});
        if (view_.bytes.empty()) view_.rows = {utf8_decode(tr("hand.binary.empty"))};
    }
    else switch (line().kind)
    {
    case LineKind::Text:
    {
        const std::u32string page = line().space.text_of(bk.unit);
        view_.rows = page_rows(page, view_aspect_, view_.cols);
        view_.heading = named + trf("view.page", {std::to_string(page.size())});
        break;
    }
    case LineKind::Audio:
        view_.rows = utf32_rows(wrap_words(notes_to_notation(note_set_of(line().space.symbols_id()), bk.unit), 64));
        view_.heading = named + tr("view.notes");
        break;
    case LineKind::Image:
    case LineKind::Video:
        view_.picture = true;
        view_.f = line().image;
        view_.rgb = render_image(bk.unit, view_.f);
        view_.heading = named + trf("view.picture", {std::to_string(view_.f.width), std::to_string(view_.f.height)});
        break;
    }
    if (!view_.picture && view_.bytes.empty())
    {
        if (view_.rows.empty()) view_.rows.emplace_back();
        view_.row_count = view_.rows.size();
        for (const auto& r : view_.rows) view_.cols = std::max(view_.cols, r.size());
        view_.cols = std::max<size_t>(1, view_.cols);
    }
    view_open_ = true;
    view_had_mouse_ = SDL_GetWindowRelativeMouseMode(window_);
    SDL_SetWindowRelativeMouseMode(window_, false);
    // A picture opens to fit the view; text at the size the item page draws it, from the top.
    view_x_ = view_y_ = 0;
    view_set_zoom(view_.picture ? view_fit_zoom() : 2.0f);
    view_clamp();
    message(trf("msg.view", {std::to_string(size_t(view_content_w())), std::to_string(size_t(view_content_h()))}));
}

void Hallway::close_viewer()
{
    view_open_ = false;
    view_drag_ = false;
    view_ = ViewDoc{}; // a picture's pixels, a file's bytes
    if (view_tex_) gpu::destroy(view_tex_);
    view_tex_ = nullptr;
    view_tex_w_ = view_tex_h_ = 0;
    view_px_ = {};
    SDL_SetWindowRelativeMouseMode(window_, view_had_mouse_);
}

// Where the thing in hand is drawn on its page: a click there opens it in the viewer. With the mouse
// held for turning (no pointer), the point is the middle of the screen, where the crosshair is.
bool Hallway::over_hand_view(const SDL_Event& e) const
{
    if (!in_hand_ || hand_tab_ != 0 || hand_view_rect_.w <= 0) return false;
    float x = 0, y = 0;
    if (SDL_GetWindowRelativeMouseMode(window_))
    {
        int w = 0, h = 0;
        SDL_GetCurrentRenderOutputSize(r_, &w, &h);
        x = float(w) * 0.5f;
        y = float(h) * 0.5f;
    }
    else SDL_RenderCoordinatesFromWindow(r_, e.button.x, e.button.y, &x, &y);
    const SDL_FPoint p{x, y};
    return SDL_PointInRectFloat(&p, &hand_view_rect_);
}

void Hallway::viewer_event(const SDL_Event& e, bool& quit)
{
    const SDL_FRect a = view_area();
    // A key carries its own modifiers (and a scripted one, --press, only there); the wheel does not.
    const SDL_Keymod mod = e.type == SDL_EVENT_KEY_DOWN ? e.key.mod : SDL_GetModState();
    switch (e.type)
    {
    case SDL_EVENT_QUIT: quit = true; return;
    case SDL_EVENT_MOUSE_WHEEL:
    {
        float dx = e.wheel.x, dy = e.wheel.y;
        if (e.wheel.direction == SDL_MOUSEWHEEL_FLIPPED)
        {
            dx = -dx;
            dy = -dy;
        }
        float mx = 0, my = 0;
        SDL_RenderCoordinatesFromWindow(r_, e.wheel.mouse_x, e.wheel.mouse_y, &mx, &my);
        if (mod & SDL_KMOD_CTRL) view_zoom_at(std::pow(1.25f, dy), mx, my);
        else
        {
            if (mod & SDL_KMOD_SHIFT) std::swap(dx, dy), dx = -dx; // the wheel's turn, across
            // Three rows a notch, or a tenth of the view for a picture.
            const float step_y = view_.picture ? a.h * 0.1f / view_zoom_ : 3 * kCellH;
            const float step_x = view_.picture ? a.w * 0.1f / view_zoom_ : 3 * kCellW;
            view_x_ += dx * step_x;
            view_y_ -= dy * step_y;
        }
        break;
    }
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
        if (e.button.button == SDL_BUTTON_LEFT) view_drag_ = true;
        break;
    case SDL_EVENT_MOUSE_BUTTON_UP:
        if (e.button.button == SDL_BUTTON_LEFT) view_drag_ = false;
        break;
    case SDL_EVENT_MOUSE_MOTION:
        if (!view_drag_) return;
        view_x_ -= e.motion.xrel / view_zoom_;
        view_y_ -= e.motion.yrel / view_zoom_;
        break;
    case SDL_EVENT_KEY_DOWN:
    {
        const float across = a.w / view_zoom_, down = a.h / view_zoom_;
        const float big = (mod & SDL_KMOD_SHIFT) ? 2.0f : 1.25f;
        switch (e.key.key)
        {
        case SDLK_ESCAPE:
        case SDLK_Z: close_viewer(); return;
        case SDLK_UP: case SDLK_W: view_y_ -= down * 0.1f; break;
        case SDLK_DOWN: case SDLK_S: view_y_ += down * 0.1f; break;
        case SDLK_LEFT: case SDLK_A: view_x_ -= across * 0.1f; break;
        case SDLK_RIGHT: case SDLK_D: view_x_ += across * 0.1f; break;
        case SDLK_PAGEUP: view_y_ -= down * 0.9f; break;
        case SDLK_PAGEDOWN: view_y_ += down * 0.9f; break;
        case SDLK_HOME: view_y_ = 0; if (mod & SDL_KMOD_CTRL) view_x_ = 0; break;
        case SDLK_END: view_y_ = view_content_h(); break;
        case SDLK_EQUALS: case SDLK_KP_PLUS: view_zoom_at(big, a.x + a.w * 0.5f, a.y + a.h * 0.5f); break;
        case SDLK_MINUS: case SDLK_KP_MINUS: view_zoom_at(1.0f / big, a.x + a.w * 0.5f, a.y + a.h * 0.5f); break;
        case SDLK_0: case SDLK_KP_0: view_set_zoom(view_fit_zoom()); break;
        case SDLK_1: case SDLK_KP_1: view_zoom_at(1.0f / view_zoom_, a.x + a.w * 0.5f, a.y + a.h * 0.5f); break;
        case SDLK_N:
        case SDLK_B:
        {
            const int dir = e.key.key == SDLK_N ? 1 : -1;
            if (in_hand_ && in_hand_->parts)
            {
                turn_page(dir);
                view_book_page();
                view_y_ = 0;
            }
            else if (view_.picture && view_.f.frames > 1)
            {
                view_.playing = false;
                view_.frame = int((uint32_t(view_.frame) + view_.f.frames + uint32_t(dir)) % view_.f.frames);
            }
            break;
        }
        case SDLK_SPACE: view_.playing = !view_.playing; break;
        default: return;
        }
        message(trf("msg.view.at", {std::to_string(int(std::lround(view_zoom_ * 100))),
                                    std::to_string(size_t(std::max(0.0f, view_x_))), std::to_string(size_t(std::max(0.0f, view_y_)))}));
        break;
    }
    default: return;
    }
    view_clamp();
}

void Hallway::draw_viewer(float W, float H)
{
    const Theme& th = theme();
    const SDL_Color ink = th.edge, dim = mix(th.edge, th.bg, 0.45f);
    panel(0, 0, W, H, 255);
    const SDL_FRect a = view_area();
    if (view_tex_w_ != int(a.w) || view_tex_h_ != int(a.h)) view_clamp(); // the window changed size
    const float z = view_zoom_;
    SDL_Rect clip{int(a.x), int(a.y), int(a.w), int(a.h)};
    SDL_SetRenderClipRect(r_, &clip);
    if (view_.picture)
    {
        // A film plays at the item page's four frames a second, until N, B or Space stops it.
        const uint32_t frames = std::max(1u, view_.f.frames);
        if (frames > 1 && view_.playing && SDL_GetTicks() >= view_.next_frame)
        {
            view_.frame = int((uint32_t(view_.frame) + 1) % frames);
            view_.next_frame = SDL_GetTicks() + 250;
            view_dirty_ = true;
        }
        const int vw = int(a.w), vh = int(a.h);
        if (!view_tex_ || view_tex_w_ != vw || view_tex_h_ != vh)
        {
            if (view_tex_) gpu::destroy(view_tex_);
            view_tex_ = gpu::create(r_, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, vw, vh);
            view_tex_w_ = vw;
            view_tex_h_ = vh;
            view_dirty_ = true;
        }
        if (view_tex_ && view_dirty_)
        {
            // Every pixel of the view from the pixel of the picture under it: as much work as the
            // view has pixels, however large the picture or the zoom.
            view_px_.resize(size_t(vw) * size_t(vh));
            const uint32_t ground = 0xFF000000u | uint32_t(th.bg.r) << 16 | uint32_t(th.bg.g) << 8 | th.bg.b;
            const size_t base = size_t(view_.frame) * view_.f.width * view_.f.height;
            for (int y = 0; y < vh; ++y)
            {
                const float fy = view_y_ + (float(y) + 0.5f) / z;
                const bool in_y = fy >= 0 && fy < float(view_.f.height);
                uint32_t* out = view_px_.data() + size_t(y) * size_t(vw);
                for (int x = 0; x < vw; ++x)
                {
                    const float fx = view_x_ + (float(x) + 0.5f) / z;
                    if (!in_y || fx < 0 || fx >= float(view_.f.width)) { out[x] = ground; continue; }
                    const size_t i = base + size_t(fy) * view_.f.width + size_t(fx);
                    const Rgb& c = i < view_.rgb.size() ? view_.rgb[i] : Rgb{};
                    out[x] = 0xFF000000u | uint32_t(c.r) << 16 | uint32_t(c.g) << 8 | uint32_t(c.b);
                }
            }
            SDL_UpdateTexture(view_tex_, nullptr, view_px_.data(), vw * 4);
            view_dirty_ = false;
        }
        if (view_tex_) SDL_RenderTexture(r_, view_tex_, nullptr, &a);
    }
    else
    {
        view_tex_w_ = int(a.w);
        view_tex_h_ = int(a.h);
        const float cw = kCellW * z, ch = kCellH * z;
        const size_t rows = view_.row_count, cols = view_.cols;
        const size_t r0 = size_t(std::max(0.0f, std::floor(view_y_ / kCellH)));
        const size_t r1 = std::min(rows, size_t(std::max(0.0f, std::ceil((view_y_ + a.h / z) / kCellH))));
        const size_t c0 = size_t(std::max(0.0f, std::floor(view_x_ / kCellW)));
        const size_t c1 = std::min(cols, size_t(std::max(0.0f, std::ceil((view_x_ + a.w / z) / kCellW))));
        auto sx = [&](size_t c) { return a.x + (float(c) * kCellW - view_x_) * z; };
        auto sy = [&](size_t r) { return a.y + (float(r) * kCellH - view_y_) * z; };
        // Letters smaller than the letters on items setting are drawn as bars, as the picture on an
        // item draws them: the words' shapes are still there.
        if (kCellW * z >= float(letters_px_))
            for (size_t r = r0; r < r1; ++r)
            {
                const std::u32string row = view_row(r);
                if (c0 < row.size()) text(sx(c0), sy(r), utf8_encode(row.substr(c0, c1 - c0)), z, ink);
            }
        else
        {
            // Bars for the words, a row to each pixel of the view at most, and runs closer than a
            // pixel drawn as one.
            SDL_SetRenderDrawColor(r_, ink.r, ink.g, ink.b, 255);
            const size_t step = std::max<size_t>(1, size_t(1.0f / ch));
            for (size_t r = r0; r < r1; r += step)
            {
                const std::u32string row = view_row(r);
                const float y = sy(r) + ch * 0.3f, h = std::max(1.0f, ch * 0.5f);
                size_t c = c0;
                while (c < std::min(c1, row.size()))
                {
                    while (c < std::min(c1, row.size()) && (row[c] == U' ' || row[c] == 0)) ++c;
                    size_t e = c;
                    for (;;)
                    {
                        while (e < std::min(c1, row.size()) && row[e] != U' ' && row[e] != 0) ++e;
                        size_t gap = e;
                        while (gap < std::min(c1, row.size()) && (row[gap] == U' ' || row[gap] == 0)) ++gap;
                        if (gap < std::min(c1, row.size()) && float(gap - e) * cw < 1.0f) e = gap;
                        else break;
                    }
                    if (e > c)
                    {
                        const SDL_FRect bar{sx(c), y, std::max(1.0f, float(e - c) * cw * 0.95f), h};
                        SDL_RenderFillRect(r_, &bar);
                    }
                    c = e;
                }
            }
        }
    }
    // Where the view is in the whole: a bar down the right and along the foot, where it does not all fit.
    SDL_SetRenderDrawColor(r_, dim.r, dim.g, dim.b, 255);
    const float cw_all = view_content_w() * z, ch_all = view_content_h() * z;
    if (ch_all > a.h)
    {
        const SDL_FRect bar{a.x + a.w - 5, a.y + a.h * std::max(0.0f, view_y_ * z) / ch_all, 4, std::max(6.0f, a.h * a.h / ch_all)};
        SDL_RenderFillRect(r_, &bar);
    }
    if (cw_all > a.w)
    {
        const SDL_FRect bar{a.x + a.w * std::max(0.0f, view_x_ * z) / cw_all, a.y + a.h - 5, std::max(6.0f, a.w * a.w / cw_all), 4};
        SDL_RenderFillRect(r_, &bar);
    }
    SDL_SetRenderClipRect(r_, nullptr);
    // The heading: what is open, and the zoom, the frame and the rows in view.
    std::string where = trf("view.zoom", {std::to_string(int(std::lround(z * 100)))});
    if (view_.picture && view_.f.frames > 1)
        where += "   " + trf("hand.frame", {std::to_string(view_.frame + 1), std::to_string(view_.f.frames)});
    if (!view_.picture)
    {
        const size_t first = size_t(std::max(0.0f, view_y_ / kCellH)) + 1;
        const size_t last = std::min(view_.row_count, size_t(std::max(0.0f, (view_y_ + a.h / z) / kCellH)));
        where += "   " + trf("view.rows", {std::to_string(std::min(first, view_.row_count)), std::to_string(std::max(first, last)),
                                           std::to_string(view_.row_count)});
    }
    const float ww = text_width(where, 1);
    text(14, 10, fit(view_.heading, W - ww - 48, 2), 2, ink);
    text(W - 14 - ww, 14, where, 1, ink);
    SDL_SetRenderDrawColor(r_, ink.r, ink.g, ink.b, 255);
    SDL_RenderLine(r_, 0, a.y - 1, W, a.y - 1);
    SDL_RenderLine(r_, 0, a.y + a.h, W, a.y + a.h);
    text(14, H - kFoot + 7, fit(tr(view_.picture ? (view_.f.frames > 1 ? "view.keys.film" : "view.keys") : in_hand_ && in_hand_->parts ? "view.keys.book" : "view.keys"),
                                W - 28, 1),
         1, ink);
}

} // namespace hallway::hall
