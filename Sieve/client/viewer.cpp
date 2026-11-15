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
// A file opens first in the viewer made for its kind (IDEAS 19.200): a picture as its pixels, a text as
// its text, a sound (WAV, FLAC, AIFF, Wave64, .au, CAF) as its waveform and a MIDI file as its notes,
// either played with Space, and a model (.obj, glTF, .glb) in three dimensions as a model on the models
// line is; its bytes are a view behind it.
// That is what the thing is. The buttons along the top show what else was made of it on the way to
// the shelf: the PICTURE on the item (drawn again by the same painter, as large as its letters or
// the screen need), its COVER, and its TITLE (a book's whole title page). F saves what is shown:
// the thing itself as F on the item page saves it, a picture or a cover as a PNG, a title as text.
// Nothing is made larger than the window: text is drawn a row at a time, only the rows in view,
// and a picture is sampled into one texture the size of the view whenever the view moves.
//
// Keys: the wheel scrolls down and up, Shift and the wheel (or a sideways wheel) across, Ctrl and
// the wheel zooms about the pointer; dragging moves it. Arrows or WASD move it, PgUp and PgDn by a
// screen, Home and End to the top and the foot, + and - zoom (Shift: twice as far), 0 fits it to
// the window and 1 is one to one. N and B turn a book's page or a film's frame, Space plays or
// stops a film, or a sound file's sound. Tab (Shift: back) or a click on a button changes the view, F saves. Esc or Z closes it.

#include "hallway.hpp"
#include "gpu_memory.hpp"
#include "picture_source.hpp"

#include "cli/flac_wav.hpp"
#include "sieve/importers.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <fstream>
#include <functional>
#include <string>
#include <string_view>

namespace hallway::hall {

namespace {

// A character's cell at zoom 1: the font's 8 pixels across, and a line's 10 down (font.hpp).
constexpr float kCellW = 8, kCellH = 10;
// The bands above the view (what is open, the zoom, where you are) and below it (the keys).
constexpr float kHead = 60, kFoot = 22, kKeyLine = 12; // (kKeyLine: each further line of keys)

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

// A picture as ARGB, from the RGB the image line renders.
std::vector<uint32_t> argb_of(const std::vector<Rgb>& rgb)
{
    std::vector<uint32_t> px(rgb.size());
    for (size_t i = 0; i < rgb.size(); ++i) px[i] = 0xFF000000u | uint32_t(rgb[i].r) << 16 | uint32_t(rgb[i].g) << 8 | uint32_t(rgb[i].b);
    return px;
}

std::vector<std::u32string> utf32_rows(const std::vector<std::string>& lines)
{
    std::vector<std::u32string> rows;
    rows.reserve(lines.size());
    for (const std::string& l : lines) rows.push_back(utf8_decode(l));
    return rows;
}

// A waveform drawn `w` x `h`: each channel a band, its silence line halfway between the two colours,
// and in each column the samples from its lowest to its highest (env_of(c) gives channel c's spans, a
// column each), reaching back to the middle of the column before, as the item page draws it.
std::vector<uint32_t> draw_wave(uint32_t channels, double full, uint32_t w, uint32_t h, SDL_Color bg, SDL_Color ink,
                                const std::function<std::vector<sieve::PcmSpan>(uint32_t)>& env_of)
{
    const uint32_t back = argb(bg), fore = argb(ink);
    const uint32_t dim = 0xFF000000u | uint32_t((bg.r + ink.r) / 2) << 16 | uint32_t((bg.g + ink.g) / 2) << 8 | uint32_t((bg.b + ink.b) / 2);
    std::vector<uint32_t> px(size_t(w) * h, back);
    for (uint32_t c = 0; c < channels; ++c)
    {
        const uint32_t top = uint32_t(uint64_t(h) * c / channels), bottom = uint32_t(uint64_t(h) * (c + 1) / channels);
        if (bottom <= top) continue;
        const double band = double(bottom - top), mid = double(top) + band / 2.0, half = band * 0.45;
        const std::vector<sieve::PcmSpan> env = env_of(c);
        for (uint32_t x = 0; x < w && x < env.size(); ++x)
        {
            const uint32_t m = std::min(bottom - 1, uint32_t(mid));
            px[size_t(m) * w + x] = dim;
            const double before = x ? (double(env[x - 1].lo) + double(env[x - 1].hi)) / 2 : double(env[x].lo);
            const double hi = std::max(double(env[x].hi), before), lo = std::min(double(env[x].lo), before);
            const int64_t y0 = std::lround(mid - hi / full * half), y1 = std::lround(mid - lo / full * half);
            for (int64_t y = std::max<int64_t>(top, y0); y <= std::min<int64_t>(int64_t(bottom) - 1, y1); ++y) px[size_t(y) * w + x] = fore;
        }
    }
    return px;
}

// Channel c of interleaved samples drawn `columns` wide: each column's lowest and highest sample (a
// column with no sample of its own takes the one it falls on), as sieve::pcm_envelope does a unit's.
std::vector<sieve::PcmSpan> interleaved_envelope(const std::vector<int32_t>& s, uint32_t channels, uint32_t c, uint32_t columns)
{
    std::vector<sieve::PcmSpan> env(columns);
    const uint64_t frames = s.size() / channels;
    if (frames == 0) return env;
    for (uint32_t x = 0; x < columns; ++x)
    {
        const uint64_t a = frames * x / columns, b = std::max(a + 1, frames * (x + 1) / columns);
        int32_t lo = s[size_t(a) * channels + c], hi = lo;
        for (uint64_t i = a + 1; i < b && i < frames; ++i)
        {
            const int32_t v = s[size_t(i) * channels + c];
            lo = std::min(lo, v);
            hi = std::max(hi, v);
        }
        env[x] = {lo, hi};
    }
    return env;
}

// The most samples (every channel counted) a sound file may have to open in the viewer: 2^26, about 12
// minutes and 40 seconds of CD sound. Its samples are refused before they are decoded where it says it
// has more, so a few bytes of FLAC claiming hours cannot fill the memory.
constexpr uint64_t kViewerMostSamples = uint64_t(1) << 26;

// The tempo a MIDI file sets first (its first FF 51 03 meta event), in quarter notes a minute, 1 to 1000;
// 120 where it sets none, as MIDI has it.
uint32_t midi_tempo(const std::vector<uint8_t>& m)
{
    for (size_t i = 0; i + 6 <= m.size(); ++i)
        if (m[i] == 0xFF && m[i + 1] == 0x51 && m[i + 2] == 0x03)
        {
            const uint32_t us = uint32_t(m[i + 3]) << 16 | uint32_t(m[i + 4]) << 8 | m[i + 5];
            if (us == 0) break;
            return std::clamp<uint32_t>(uint32_t(std::lround(60000000.0 / double(us))), 1, 1000);
        }
    return 120;
}

// Whether a text file reads as a model: a glTF's JSON (an object with an "asset" member), or an .obj
// (a line of a vertex, "v ", and one of a face, "f ").
bool text_is_model(const std::vector<uint8_t>& b)
{
    const std::string_view t(reinterpret_cast<const char*>(b.data()), b.size());
    const size_t first = t.find_first_not_of(" \t\r\n");
    if (first != std::string_view::npos && t[first] == '{') return t.find("\"asset\"") != std::string_view::npos;
    bool v = false, f = false;
    for (size_t at = 0; at < t.size() && !(v && f);)
    {
        const size_t end = std::min(t.find('\n', at), t.size());
        const std::string_view l = t.substr(at, end - at);
        v = v || l.starts_with("v ");
        f = f || l.starts_with("f ");
        at = end + 1;
    }
    return v && f;
}

// The hex dump's row width: the offset, sixteen bytes and the sixteen characters beside them, as
// the item page shows a file (hud.cpp draw_file).
constexpr size_t kHexPer = 16, kHexCols = 6 + 2 + 3 * kHexPer + 1 + kHexPer;

} // namespace

// The keys under the view, as written: drawn through the one footer (footer.hpp), which lays them out in
// lines that fit the window, broken only between keys, the last line at the bottom.
std::string Hallway::view_keys() const
{
    const char* keys = view_kinds_.empty() ? "view.keys"
                       : view_kinds_[view_at_] == ViewKind::Shape ? "view.keys.shape"
                       : view_kinds_[view_at_] == ViewKind::Opened && !view_opened_.sound.empty() ? "view.keys.sound"
                       : view_.picture ? (view_.frames > 1 ? "view.keys.film" : "view.keys")
                       : in_hand_ && in_hand_->parts ? "view.keys.book"
                                                     : "view.keys";
    return tr(keys);
}

// What the view shows, and where on it: the area below the heading and above the keys.
SDL_FRect Hallway::view_area() const
{
    int w = 0, h = 0;
    SDL_GetCurrentRenderOutputSize(r_, &w, &h);
    const float foot = kFoot + footer_extra({{view_keys(), SDL_Color{}}}, float(w) - 28, kKeyLine);
    return {0, kHead, float(w), std::max(1.0f, float(h) - kHead - foot)};
}

// The thing's size at zoom 1: its characters' cells, or its pixels.
float Hallway::view_content_w() const { return view_.picture ? float(view_.pw) : float(view_.cols) * kCellW; }
float Hallway::view_content_h() const { return view_.picture ? float(view_.ph) : float(view_.row_count) * kCellH; }

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
    // A book's title is a page of its own (the TITLE view): the heading has the start of it.
    std::string title = utf8_encode(unit_line(LineKind::Text).space.text_of(in_hand_->parts->title));
    while (!title.empty() && title.back() == ' ') title.pop_back();
    view_.heading = (title.find_first_not_of(" ") == std::string::npos ? std::string() : "\"" + title + "\"   ") +
                    trf("view.book", {std::to_string(book_page_ + 1), std::to_string(n)});
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

std::string Hallway::view_label(ViewKind k) const
{
    switch (k)
    {
    case ViewKind::Picture: return tr("view.tab.picture");
    case ViewKind::Shape: return tr("view.tab.shape");
    case ViewKind::Cover: return tr("view.tab.cover");
    case ViewKind::Title: return tr("view.tab.title");
    case ViewKind::Opened:
        return tr(view_opened_.viewer == "text" || view_opened_.viewer == "model" ? "view.tab.text"
                  : view_opened_.viewer == "sound"                                ? "view.tab.sound"
                  : view_opened_.viewer == "notes"                                ? "view.tab.notes"
                                                                                  : "view.tab.pixels");
    case ViewKind::Raw: break;
    }
    // What the thing itself is, on this line.
    const Book& bk = *in_hand_;
    if (bk.model || bk.world) return tr("view.tab.obj");
    if (bk.ai) return tr("view.tab.weights");
    if (bk.parts) return tr("view.tab.page");
    if (bk.is_file) return tr("view.tab.hex");
    switch (line().kind)
    {
    case LineKind::Text: return tr("view.tab.text");
    case LineKind::Audio: return tr(on_sound() ? "view.tab.sound" : "view.tab.notes");
    case LineKind::Video: return tr("view.tab.frames");
    default: return tr("view.tab.pixels");
    }
}

// How wide the picture on the item is drawn for the viewer: as wide as its letters need, or as the
// screen (a close-up's width at most, display.hpp closeup_width), whichever is more, while it fits
// the display cache on its own.
int Hallway::view_picture_px(double aspect) const
{
    int w = 0, h = 0;
    SDL_GetCurrentRenderOutputSize(r_, &w, &h);
    const int roomy = INT32_MAX / 2; // the picture is sampled to the view, so no texture limits it
    const int widest = widest_display_px(double(face_budget_mb_) * 1048576.0, 1.0, aspect, roomy);
    const int screen = closeup_width(kCloseUpScreen, w, roomy);
    return std::min(widest, display_px(std::max(face_px_, screen), letters_px_, aspect, display_text_here(), widest));
}

// The thing itself: its text, pixels, notes, .obj or bytes.
void Hallway::view_raw()
{
    const Book& bk = *in_hand_;
    const std::string title = title_text(bk);
    const std::string named = title.empty() ? std::string() : "\"" + title + "\"   ";
    if (bk.model)
    {
        view_.rows = utf32_rows(split_lines(model_space_->to_obj(*bk.model)));
        view_.heading = named + tr("view.model");
    }
    else if (bk.world)
    {
        view_.rows = utf32_rows(split_lines(world_space_->to_obj(*bk.world)));
        view_.heading = named + tr("view.world");
    }
    else if (bk.ai)
    {
        // A model: each of its tensors by name and shape, then its weights as digits (hex, 64 a row),
        // in the order its address reads them.
        std::vector<std::string> lines;
        static const char* hex = "0123456789abcdef";
        for (const AiSpace::Tensor& t : ai_space_->tensors())
        {
            lines.push_back(t.name + "  [" + std::to_string(t.rows) + " x " + std::to_string(t.cols) + "]");
            std::string row;
            for (uint64_t i = 0; i < uint64_t(t.rows) * t.cols; ++i)
            {
                const uint8_t d = (*bk.ai)[size_t(t.first + i)];
                if (ai_space_->shape().bits > 4) row.push_back(hex[d >> 4]);
                row.push_back(hex[d & 15]);
                if (row.size() >= 64)
                {
                    lines.push_back(row);
                    row.clear();
                }
            }
            if (!row.empty()) lines.push_back(row);
        }
        view_.rows = utf32_rows(lines);
        view_.heading = named + tr("view.ai");
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
        if (on_sound())
        {
            // A picture of the waveform: a column a sample where the view is wide enough (as wide as
            // the picture on an item may be drawn), each channel a band an eighth as tall as wide.
            const sieve::PcmFormat f = sieve::pcm_format_of(line().space.symbols_id());
            const uint64_t L = bk.unit.size() / f.channels;
            const double aspect = double(f.channels) / 8.0;
            const uint32_t w = uint32_t(std::max<uint64_t>(1, std::min<uint64_t>(L, uint64_t(view_picture_px(aspect)))));
            const uint32_t h = std::max<uint32_t>(f.channels, uint32_t(std::lround(double(w) * aspect)));
            view_.picture = true;
            view_.px = waveform_argb(bk.unit, w, h);
            view_.pw = w;
            view_.ph = h;
            view_.heading = named + trf("view.sound", {std::to_string(bk.unit.size() / f.channels), std::to_string(f.rate), std::to_string(f.bits),
                                                       std::to_string(f.channels)});
        }
        else
        {
            view_.rows = utf32_rows(wrap_words(audio_text(bk.unit, 0), 64));
            view_.heading = named + tr("view.notes");
        }
        break;
    case LineKind::Image:
    case LineKind::Video:
    {
        const ImageFormat& f = line().image;
        view_.picture = true;
        view_.px = argb_of(render_image(bk.unit, f));
        view_.pw = f.width;
        view_.ph = f.height;
        view_.frames = std::max(1u, f.frames);
        view_.heading = named + trf("view.picture", {std::to_string(f.width), std::to_string(f.height)});
        break;
    }
    }
}

std::vector<uint32_t> Hallway::waveform_argb(const Space::Digits& u, uint32_t w, uint32_t h) const
{
    const sieve::PcmFormat f = sieve::pcm_format_of(line().space.symbols_id());
    return draw_wave(f.channels, double(uint64_t(1) << (f.bits - 1)), w, h, theme().bg, theme().edge,
                     [&](uint32_t c) { return sieve::pcm_envelope(f, u, c, w); });
}

// Shows view `at` of the thing in hand, from the top, a picture fitted to the window.
void Hallway::view_show(size_t at)
{
    if (!in_hand_ || view_kinds_.empty()) return;
    if (view_job_.valid()) view_job_.wait(); // a picture still being drawn for the last view
    view_job_ = {};
    view_at_ = at % view_kinds_.size();
    // (an opened file's pixels are moved into the view, not copied, and moved back here: they were kept
    // twice while the viewer was open; review 1)
    if (view_.px_opened) view_opened_.px = std::move(view_.px);
    view_ = ViewDoc{};
    const Book& bk = *in_hand_;
    const std::string title = title_text(bk);
    const std::string named = title.empty() ? std::string() : "\"" + title + "\"   ";
    switch (view_kinds_[view_at_])
    {
    case ViewKind::Raw: view_raw(); break;
    case ViewKind::Cover:
    {
        // A book's cover, or a track's or a film's: a picture of the image line.
        const ImageFormat& f = unit_line(LineKind::Image).image;
        view_.picture = true;
        view_.px = argb_of(render_image(bk.parts ? bk.parts->cover : bk.cover, f));
        view_.pw = f.width;
        view_.ph = f.height;
        view_.heading = named + trf("view.cover", {std::to_string(f.width), std::to_string(f.height)});
        break;
    }
    case ViewKind::Title:
    {
        // A book's title is a whole page; any other title, as long as the line's titles are.
        const std::u32string t = bk.parts ? unit_line(LineKind::Text).space.text_of(bk.parts->title) : utf8_decode(title);
        view_.rows = page_rows(t, view_aspect_, view_.cols);
        view_.heading = trf("view.title", {std::to_string(written_length(t))});
        break;
    }
    case ViewKind::Shape:
    {
        // The model (or the world, every model placed) drawn in three dimensions, large and square,
        // turned as model_spin_ and model_tilt_ say (the item page's turn, so it opens as it was left).
        const int n = view_picture_px(1.0);
        view_.picture = true;
        view_.pw = uint32_t(n);
        view_.ph = uint32_t(n);
        const SDL_Color edge = theme().edge;
        if (bk.model) render_model_face(*model_space_, *bk.model, n, model_spin_, model_tilt_, edge, view_.px);
        else if (bk.world)
        {
            const WorldSpace::Mesh m = world_mesh(*world_space_, *bk.world);
            render_mesh_face(m.vertices, m.faces, n, model_spin_, model_tilt_, edge, view_.px);
        }
        else
        {
            // A model file, as its importer read it (sieve/importers.hpp).
            const OpenedFile& o = view_opened_;
            render_mesh_face(o.mesh.verts, o.mesh.faces, n, model_spin_, model_tilt_, edge, view_.px);
            view_.heading = named + trf(o.text.empty() ? "view.opened.model" : "view.opened.model.text",
                                        {o.format, std::to_string(o.mesh.verts.size()), std::to_string(o.mesh.faces.size())});
            break;
        }
        view_.heading = named + tr("view.shape");
        break;
    }
    case ViewKind::Opened:
    {
        OpenedFile& o = view_opened_;
        if (o.viewer == "text" || o.viewer == "model")
        {
            view_.rows = page_rows(utf8_decode(o.text), view_aspect_, view_.cols);
            view_.heading = named + trf("view.opened.text", {o.kind, std::to_string(o.text.size())});
        }
        else if (o.viewer == "notes")
        {
            // Each voice's notes, wrapped, a blank line between voices.
            std::vector<std::string> lines;
            size_t at = 0;
            for (uint32_t v = 1;; ++v)
            {
                const size_t end = o.text.find(" // ", at);
                if (v > 1) lines.emplace_back();
                const std::vector<std::string> w = wrap_words(trf("view.opened.voice", {std::to_string(v)}) + " " + o.text.substr(at, end == std::string::npos ? std::string::npos : end - at), 64);
                lines.insert(lines.end(), w.begin(), w.end());
                if (end == std::string::npos) break;
                at = end + 4;
            }
            view_.rows = utf32_rows(lines);
            view_.heading = named + trf("view.opened.notes", {std::to_string(o.voices), std::to_string(o.tempo)});
        }
        else
        {
            view_.picture = true;
            view_.px = std::move(o.px);
            view_.px_opened = true;
            view_.pw = o.w;
            view_.ph = o.h;
            view_.frames = o.frames;
            view_.heading = named + (o.viewer == "sound" ? trf("view.opened.sound", {o.kind, std::to_string(o.samples), std::to_string(o.rate),
                                                                                      std::to_string(o.bits), std::to_string(o.channels)})
                                                         : trf("view.opened.picture", {o.kind, std::to_string(o.w), std::to_string(o.h)}));
        }
        break;
    }
    case ViewKind::Picture:
    {
        // The picture on the item, drawn again by the same painter as the shelf's, larger, on a
        // worker: a long page's takes a moment.
        const double aspect = double(face_rect().aspect());
        const int w = view_picture_px(aspect), h = std::max(1, int(std::lround(double(w) * aspect)));
        view_.picture = true;
        view_.pw = uint32_t(w);
        view_.ph = uint32_t(h);
        view_.heading = named + trf("view.on_item", {std::to_string(w), std::to_string(h)});
        if (Painter paint = face_painter(bk))
            view_job_ = std::async(std::launch::async, [paint, w, h] {
                std::vector<uint32_t> px;
                paint(px, w, h);
                return px;
            });
        break;
    }
    }
    if (!view_.picture && view_.bytes.empty())
    {
        if (view_.rows.empty()) view_.rows.emplace_back();
        view_.row_count = view_.rows.size();
        for (const auto& r : view_.rows) view_.cols = std::max(view_.cols, r.size());
        view_.cols = std::max<size_t>(1, view_.cols);
    }
    // A picture opens to fit the view; text at the size the item page draws it, from the top.
    view_x_ = view_y_ = 0;
    view_set_zoom(view_.picture ? view_fit_zoom() : 2.0f);
    view_clamp();
    message(trf("msg.view", {view_label(view_kinds_[view_at_]), std::to_string(size_t(view_content_w())),
                             std::to_string(size_t(view_content_h()))}));
}

void Hallway::open_viewer()
{
    if (!in_hand_ || withheld(*in_hand_)) return;
    const Book& bk = *in_hand_;
    // A page opens shaped as the picture on a page is: the pages line's display, less its margins.
    {
        const double a = double(load_face_rect("pages").aspect());
        view_aspect_ = std::max(0.1, (a - 0.125) / (1.0 - 0.125)); // a sixteenth of the width either side
    }
    // The views it has: itself, and what was made of it for the shelf.
    view_kinds_ = {ViewKind::Raw};
    view_opened_ = OpenedFile{};
    view_.px_opened = false; // (what the view holds is not this file's)
    // A file opens first in the viewer made for its kind, where there is one; its bytes come next. A
    // model file opens in three dimensions, its text (where it is text) next.
    if (bk.is_file && open_file_by_kind(file_of(bk)))
    {
        if (view_opened_.viewer != "model" || !view_opened_.text.empty()) view_kinds_.insert(view_kinds_.begin(), ViewKind::Opened);
        if (view_opened_.viewer == "model") view_kinds_.insert(view_kinds_.begin(), ViewKind::Shape);
    }
    // A model or a world opens on itself in three dimensions, turned as on its page; its .obj text next.
    if (bk.model || bk.world) view_kinds_.insert(view_kinds_.begin(), ViewKind::Shape);
    if (face_painter(bk)) view_kinds_.push_back(ViewKind::Picture);
    if (bk.parts || !bk.cover.empty()) view_kinds_.push_back(ViewKind::Cover);
    if (bk.parts || !title_text(bk).empty()) view_kinds_.push_back(ViewKind::Title);
    view_open_ = true;
    view_had_mouse_ = SDL_GetWindowRelativeMouseMode(window_);
    SDL_SetWindowRelativeMouseMode(window_, false);
    view_show(0);
    // A file opens in the viewer made for its kind. Where none is made yet, it is shown as its bytes
    // and the kind is flagged, so the viewers still to be made are known (Edward, 13 November 2026: "if
    // one doesnt exist it should flag it to us and tell us a new viewer needs to be created").
    // (a kind whose viewer could not show this one is not flagged: it has a viewer, and why it could not
    // is said)
    if (bk.is_file && view_kinds_[0] == ViewKind::Raw && !view_.bytes.empty() && view_opened_.viewer.empty())
        flag_missing_viewer(file_type(view_.bytes, view_.bytes.size()));
    // Where the viewer made for its kind could not show it, why (after the view's own message, so it is
    // the one read).
    if (!view_opened_.said.empty()) message(view_opened_.said);
}

// The viewers made for kinds of file (file-kinds-v1), as data: a kind not listed has none yet, and is
// shown as its bytes and flagged. Pictures are made by sieve-view's reader (picture_source.cpp: every
// limit a hostile file could push against checked before anything is decoded); text is shown as text;
// sound is read as the FLAC way reads it (cli/flac_wav.hpp) and MIDI as the audio line's notes3 reads
// it. Besides the table, a sound file of a kind file-kinds-v1 does not name (AIFF, Wave64, .au, CAF)
// is known by its first bytes, and a model by its own (a .glb) or by its text (a glTF's JSON, an .obj).
// MP3 and Ogg have no decoder here yet, so they are flagged.
namespace {
struct FileViewer
{
    const char* kind;
    const char* viewer;
};
constexpr FileViewer kFileViewers[] = {
    {"PNG", "picture"}, {"JPG", "picture"}, {"GIF", "picture"}, {"BMP", "picture"},
    {"TXT", "text"},    {"BOOK", "text"},   {"MANIFEST", "text"},
    {"WAV", "sound"},   {"FLAC", "sound"},  {"MID", "notes"},
};
const char* viewer_of_kind(const std::string& kind)
{
    for (const FileViewer& v : kFileViewers)
        if (kind == v.kind) return v.viewer;
    return nullptr;
}
bool has_file_viewer(const std::string& kind) { return viewer_of_kind(kind) != nullptr; }
} // namespace

// Makes the file ready in the viewer made for its kind; false where its kind has none, or that viewer
// could not show it (a picture cut short: it is then shown as its bytes, and why is said).
bool Hallway::open_file_by_kind(const std::vector<uint8_t>& bytes)
{
    OpenedFile& o = view_opened_;
    o.kind = file_type(bytes, bytes.size());
    if (bytes.empty()) return false;
    const bool glb = bytes.size() >= 12 && std::memcmp(bytes.data(), "glTF", 4) == 0;
    if ((glb || (o.kind == "TXT" && text_is_model(bytes))) && open_model_file(bytes)) return true;
    const char* viewer = viewer_of_kind(o.kind);
    if (!viewer && cli::is_sound(bytes)) viewer = "sound";
    if (!viewer) return false;
    o.viewer = viewer;
    if (o.viewer == "text")
    {
        o.text.assign(bytes.begin(), bytes.end());
        return true;
    }
    if (o.viewer == "sound") return open_sound_file(bytes);
    if (o.viewer == "notes") return open_midi_file(bytes);
    try
    {
        sieve::view::Picture p;
        p.file = bytes;
        sieve::view::make_pixels(p, true);
        o.w = p.pixels.width;
        o.h = p.pixels.height;
        o.frames = uint32_t(1 + p.more.size());
        o.px.reserve(size_t(o.w) * o.h * o.frames);
        for (size_t f = 0; f < o.frames; ++f)
        {
            const RgbaImage& img = p.frame(f);
            for (size_t i = 0; i + 3 < img.rgba.size(); i += 4)
                o.px.push_back(uint32_t(img.rgba[i + 3]) << 24 | uint32_t(img.rgba[i]) << 16 | uint32_t(img.rgba[i + 1]) << 8 | img.rgba[i + 2]);
        }
        return o.w > 0 && o.h > 0;
    }
    catch (const std::exception& e)
    {
        o.why_not = e.what();
        o.said = trf("view.opened.not", {o.kind, o.why_not});
        return false;
    }
}

// A sound file: its samples read (refused before decoding where there are more than the viewer takes),
// its waveform drawn as the audio line draws a unit's, and the sound kept as one channel to play.
bool Hallway::open_sound_file(const std::vector<uint8_t>& bytes)
{
    OpenedFile& o = view_opened_;
    std::string why;
    std::optional<cli::SoundSamples> s = cli::sound_samples(bytes, kViewerMostSamples, &why);
    if (!s || s->channels == 0 || s->rate == 0 || s->samples.size() < s->channels)
    {
        o.why_not = s ? tr("view.opened.silent") : why;
        o.said = trf("view.opened.not.sound", {o.kind, o.why_not});
        return false;
    }
    if (o.kind == "?") o.kind = cli::sound_name(bytes);
    o.rate = s->rate;
    o.channels = s->channels;
    o.bits = s->bits;
    o.samples = s->samples.size() / s->channels;
    const double aspect = double(o.channels) / 8.0;
    o.w = uint32_t(std::max<uint64_t>(1, std::min<uint64_t>(o.samples, uint64_t(view_picture_px(aspect)))));
    o.h = std::max<uint32_t>(o.channels, uint32_t(std::lround(double(o.w) * aspect)));
    o.px = draw_wave(o.channels, double(uint64_t(1) << (o.bits - 1)), o.w, o.h, theme().bg, theme().edge,
                     [&](uint32_t c) { return interleaved_envelope(s->samples, o.channels, c, o.w); });
    // Each frame's channels added and scaled to -1..1 (divided by the channels, so it cannot clip).
    const float full = float(uint64_t(1) << (o.bits - 1)) * float(o.channels);
    o.sound.resize(size_t(o.samples));
    for (size_t i = 0; i < o.sound.size(); ++i)
    {
        float v = 0;
        for (uint32_t c = 0; c < o.channels; ++c) v += float(s->samples[i * o.channels + c]);
        o.sound[i] = v / full;
    }
    return true;
}

// A MIDI file: read as the audio line reads one into open-ended notes (sieve/notes3.hpp), in a set as
// wide as MIDI (every pitch, 96 ticks a quarter, 8 levels, a voice a track with notes) at the tempo the
// file sets first; shown as their notation, and played as the line plays notes3.
bool Hallway::open_midi_file(const std::vector<uint8_t>& bytes)
{
    OpenedFile& o = view_opened_;
    try
    {
        o.tempo = midi_tempo(bytes);
        constexpr uint32_t kTpq = 96, kLongest = 65535, kLevels = 8;
        const sieve::Notes3Set wide = sieve::make_notes3_set(0, 127, kTpq, kLongest, kLevels, sieve::kNotes3MaxVoices, o.tempo, {});
        std::vector<std::string> report;
        o.text = sieve::midi_to_notes3(bytes, wide, &report);
        // What the reading left out (a chord's other notes, tracks past the fifteenth), said when the
        // viewer opens; its last line, that the tempo and instruments are the line's, is not so here
        // (the tempo is the file's own).
        std::string left;
        for (size_t k = 0; k + 1 < report.size(); ++k) left += (left.empty() ? "" : "; ") + report[k];
        if (!left.empty()) o.said = trf("view.opened.notes.left", {left});
        // Its voices, and the most events in one: the set made to fit them, every voice in one unit.
        size_t voices = 1, most = 0, events = 0;
        for (size_t at = 0; at <= o.text.size();)
        {
            const size_t sp = std::min(o.text.find(' ', at), o.text.size());
            const std::string_view w(o.text.data() + at, sp - at);
            if (w == "//")
            {
                ++voices;
                most = std::max(most, events);
                events = 0;
            }
            else if (!w.empty()) ++events;
            at = sp + 1;
        }
        most = std::max(most, events);
        if (most == 0) throw std::invalid_argument(tr("view.opened.no_notes"));
        o.voices = uint32_t(voices);
        const sieve::Notes3Set set = sieve::make_notes3_set(0, 127, kTpq, kLongest, kLevels, o.voices, o.tempo, {});
        const sieve::Notes3CanonResult r = sieve::canonicalise_notes3(o.text, set, uint32_t(std::min<size_t>(most, UINT32_MAX)));
        if (r.units.empty()) throw std::invalid_argument(tr("view.opened.no_notes"));
        o.sound = render_notes3(set, r.units[0], kSynthRate);
        o.rate = kSynthRate;
        return true;
    }
    catch (const std::exception& e)
    {
        o.why_not = e.what();
        o.said = trf("view.opened.not.notes", {o.kind, o.why_not});
        return false;
    }
}

// A model file (.obj, glTF, .glb), read by the importers (sieve/importers.hpp: what the file holds
// beyond its mesh is counted there, not shown here). A glTF whose buffers are files beside it cannot be
// read from the bytes alone, and is said so.
bool Hallway::open_model_file(const std::vector<uint8_t>& bytes)
{
    OpenedFile& o = view_opened_;
    try
    {
        sieve::ImportedModel m = sieve::read_model(std::string_view(reinterpret_cast<const char*>(bytes.data()), bytes.size()));
        if (m.mesh.faces.empty()) return false; // (a text with a stray "v " and "f " line is still text)
        o.viewer = "model";
        o.mesh = std::move(m.mesh);
        o.format = m.format;
        for (const sieve::ImportFormat& f : sieve::import_formats())
            if (f.id == m.format) o.format = std::string(f.name);
        if (o.kind == "TXT") o.text.assign(bytes.begin(), bytes.end());
        return true;
    }
    catch (const std::exception& e)
    {
        o.why_not = e.what();
        o.said = trf("view.opened.not.model", {o.why_not});
        return false;
    }
}

// Space on a sound or notes: plays it from the start, through the music player when there is one (which
// fades the music out under it); again, stops it.
void Hallway::view_play()
{
    if (view_sounding_ && SDL_GetTicks() >= view_sound_ends_) view_sounding_ = false; // (it has finished by itself)
    if (view_sounding_)
    {
        if (music()) music()->stop_item();
        synth_.stop();
        view_sounding_ = false;
        message(tr("msg.view.stopped"));
        return;
    }
    const OpenedFile& o = view_opened_;
    const std::string err = music() ? music()->play_samples(o.sound, o.rate) : synth_.play_samples(o.sound, o.rate);
    view_sounding_ = err.empty();
    view_sound_ends_ = SDL_GetTicks() + Uint64(double(o.sound.size()) * 1000.0 / double(std::max<uint32_t>(1, o.rate))) + 1;
    message(err.empty() ? tr("msg.playing") : err);
}

void Hallway::flag_missing_viewer(const std::string& kind)
{
    if (has_file_viewer(kind)) return;
    std::fprintf(stderr, "viewer: none for %s files yet\n", kind.c_str());
    // Noted once a kind, beside your settings: the list of viewers to make.
    bool noted = false;
    if (!favourites_base_.empty())
    {
        const auto list = favourites_base_ / "viewers-needed.txt";
        std::ifstream in(list);
        std::string l;
        bool listed = false;
        while (std::getline(in, l)) listed = listed || l == kind;
        in.close();
        if (!listed)
        {
            std::ofstream out(list, std::ios::app);
            out << kind << "\n"; // (one kind a line)
        }
        noted = true;
    }
    message(trf(noted ? "view.no_viewer.noted" : "view.no_viewer", {kind == "?" ? tr("view.kind_unknown") : kind}));
}

void Hallway::close_viewer()
{
    if (view_job_.valid()) view_job_.wait();
    view_job_ = {};
    view_open_ = false;
    view_drag_ = false;
    if (view_sounding_) // (the sound is the viewer's, so it stops with it)
    {
        if (music()) music()->stop_item();
        synth_.stop();
        view_sounding_ = false;
    }
    view_ = ViewDoc{}; // a picture's pixels, a file's bytes
    view_kinds_.clear();
    view_opened_ = OpenedFile{};
    view_buttons_.clear();
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
        if (e.button.button == SDL_BUTTON_LEFT)
        {
            // The buttons along the top: a view, or save; anywhere else, a drag.
            SDL_FPoint p{};
            SDL_RenderCoordinatesFromWindow(r_, e.button.x, e.button.y, &p.x, &p.y);
            for (const auto& [box, which] : view_buttons_)
                if (SDL_PointInRectFloat(&p, &box))
                {
                    if (which < 0) save_view();
                    else view_show(size_t(which));
                    return;
                }
            view_drag_ = true;
        }
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
        if (view_kinds_[view_at_] == ViewKind::Shape)
        {
            // In three dimensions, the arrows and WASD turn it (15 degrees about the vertical, 10 up and
            // down) rather than move across it.
            const SDL_Keycode k = e.key.key;
            const bool turn = k == SDLK_LEFT || k == SDLK_A || k == SDLK_RIGHT || k == SDLK_D, tip = k == SDLK_UP || k == SDLK_W || k == SDLK_DOWN || k == SDLK_S;
            if (turn || tip)
            {
                if (turn) model_spin_ += (k == SDLK_RIGHT || k == SDLK_D ? 1.0f : -1.0f) * 0.2618f;
                else model_tilt_ = std::clamp(model_tilt_ + (k == SDLK_UP || k == SDLK_W ? 1.0f : -1.0f) * 0.1745f, -1.5f, 1.5f);
                const float z = view_zoom_;
                view_show(view_at_);
                view_set_zoom(z);
                view_clamp();
                return;
            }
        }
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
                if (view_kinds_[view_at_] == ViewKind::Raw)
                {
                    view_book_page();
                    view_y_ = 0;
                }
            }
            else if (view_.picture && view_.frames > 1)
            {
                view_.playing = false;
                view_.frame = int((uint32_t(view_.frame) + view_.frames + uint32_t(dir)) % view_.frames);
            }
            break;
        }
        case SDLK_SPACE:
            if (view_kinds_[view_at_] == ViewKind::Opened && !view_opened_.sound.empty())
            {
                view_play();
                return;
            }
            view_.playing = !view_.playing;
            break;
        case SDLK_TAB: view_show(view_at_ + ((mod & SDL_KMOD_SHIFT) ? view_kinds_.size() - 1 : 1)); return;
        case SDLK_F: save_view(); return;
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
        // The picture on the item, once its worker has drawn it.
        if (view_job_.valid() && view_job_.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
        {
            view_.px = view_job_.get();
            view_dirty_ = true;
        }
        // A film plays at the item page's four frames a second, until N, B or Space stops it.
        const uint32_t frames = std::max(1u, view_.frames);
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
            const size_t base = size_t(view_.frame) * view_.pw * view_.ph;
            for (int y = 0; y < vh; ++y)
            {
                const float fy = view_y_ + (float(y) + 0.5f) / z;
                const bool in_y = fy >= 0 && fy < float(view_.ph);
                uint32_t* out = view_px_.data() + size_t(y) * size_t(vw);
                for (int x = 0; x < vw; ++x)
                {
                    const float fx = view_x_ + (float(x) + 0.5f) / z;
                    if (!in_y || fx < 0 || fx >= float(view_.pw)) { out[x] = ground; continue; }
                    const size_t i = base + size_t(fy) * view_.pw + size_t(fx);
                    out[x] = i < view_.px.size() ? view_.px[i] : ground;
                }
            }
            SDL_UpdateTexture(view_tex_, nullptr, view_px_.data(), vw * 4);
            view_dirty_ = false;
        }
        if (view_tex_) SDL_RenderTexture(r_, view_tex_, nullptr, &a);
        if (view_.px.empty())
        {
            const std::string wait = tr("view.drawing");
            text(a.x + (a.w - text_width(wait, 2)) * 0.5f, a.y + a.h * 0.5f - 8, wait, 2, ink);
        }
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
    if (view_.picture && view_.frames > 1)
        where += "   " + trf("hand.frame", {std::to_string(view_.frame + 1), std::to_string(view_.frames)});
    if (!view_.picture)
    {
        const size_t first = size_t(std::max(0.0f, view_y_ / kCellH)) + 1;
        const size_t last = std::min(view_.row_count, size_t(std::max(0.0f, (view_y_ + a.h / z) / kCellH)));
        where += "   " + trf("view.rows", {std::to_string(std::min(first, view_.row_count)), std::to_string(std::max(first, last)),
                                           std::to_string(view_.row_count)});
    }
    const float ww = text_width(where, 1);
    text(14, 8, fit(view_.heading, W - ww - 48, 2), 2, ink);
    text(W - 14 - ww, 12, where, 1, ink);
    // The buttons: the views, the one shown lit, and at the right, saving what is shown.
    view_buttons_.clear();
    float bx = 14;
    const float by = 32;
    auto button = [&](float x, const std::string& label, bool lit, int which) {
        const float bw = text_width(label, 1) + 16;
        const SDL_FRect box{x, by, bw, 18};
        SDL_SetRenderDrawColor(r_, ink.r, ink.g, ink.b, lit ? 90 : 0);
        if (lit) SDL_RenderFillRect(r_, &box);
        SDL_SetRenderDrawColor(r_, ink.r, ink.g, ink.b, 255);
        SDL_RenderRect(r_, &box);
        text(x + 8, by + 5, label, 1, ink);
        view_buttons_.emplace_back(box, which);
        return bw;
    };
    for (size_t i = 0; i < view_kinds_.size(); ++i) bx += button(bx, view_label(view_kinds_[i]), i == view_at_, int(i)) + 6;
    {
        const std::string save = tr(view_kinds_.empty() || view_kinds_[view_at_] == ViewKind::Raw || view_kinds_[view_at_] == ViewKind::Opened ? "view.save.raw"
                                    : view_kinds_[view_at_] == ViewKind::Title                    ? "view.save.text"
                                                                                                  : "view.save.png");
        button(W - 14 - (text_width(save, 1) + 16), save, false, -1);
    }
    SDL_SetRenderDrawColor(r_, ink.r, ink.g, ink.b, 255);
    SDL_RenderLine(r_, 0, a.y - 1, W, a.y - 1);
    SDL_RenderLine(r_, 0, a.y + a.h, W, a.y + a.h);
    draw_footer(r_, "viewer", 14, H - kFoot + 7, W - 28, view_keys(), ink, kKeyLine);
}

} // namespace hallway::hall
