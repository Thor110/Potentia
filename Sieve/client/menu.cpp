#include "menu.hpp"

#include "font.hpp"
#include "strings.hpp"
#include "theme.hpp"

#include "cli/dictionaries.hpp"
#include "cli/models.hpp"

#include "sieve/alphabet.hpp"
#include "sieve/audio.hpp"
#include "sieve/filter.hpp"
#include "sieve/corridor.hpp"
#include "sieve/image.hpp"

#include <algorithm>
#include <array>
#include <climits>
#include <cstdint>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <vector>

namespace hallway {

namespace {

const std::vector<std::string> kLines = {"text", "image", "audio", "video", "books"};
const std::vector<std::string> kModes = {"positional", "scrambled", "guided"};
const std::vector<std::string> kAlphabets = {"lower27", "babel29", "ascii95"};
const std::vector<std::string> kCanons = {"v2", "v1"};
const std::vector<std::string> kPalettes = {"mono", "ega16", "rgb332", "rgb24"};
constexpr uint32_t kNoteSymbols = 104;

std::string cycle(const std::vector<std::string>& v, const std::string& cur, int dir)
{
    auto it = std::find(v.begin(), v.end(), cur);
    const int i = it == v.end() ? 0 : int(it - v.begin());
    const int n = int(v.size());
    return v[size_t(((i + dir) % n + n) % n)];
}

uint32_t palette_size(const std::string& id) { return sieve::palette_by_id(id).size(); }
uint32_t alphabet_size(const std::string& id) { return sieve::alphabet_by_id(id).size(); }

uint32_t parse_u32(const sieve::cli::Args& a, const char* key, uint32_t def) { return a.has(key) ? a.get_positive(key, def) : def; }

// What this machine can open. The design has no limit; these only describe the hardware of the
// day, and a future machine only needs them raised. An address of this many bits is one number
// held in memory; the arithmetic on it grows with the square of its length.
constexpr double kSlowBits = 4.0e6;    // above this, opening a line takes noticeable time
constexpr double kTooLargeBits = 8.0e9; // above this, one address would need a gigabyte

void text(SDL_Renderer* r, float x, float y, const std::string& s, float scale, SDL_Color c) { draw_text(r, x, y, s, scale, c); }

std::string fixed(double v, int d)
{
    char b[48];
    std::snprintf(b, sizeof b, "%.*f", d, v);
    return b;
}

} // namespace

// ---------------------------------------------------------------- settings

Settings Settings::from_args(const sieve::cli::Args& a)
{
    Settings s;
    s.start_line = a.get("line", s.start_line);
    if (s.start_line == "pages") s.start_line = "text"; // another name for the text line
    s.mode = a.get("mode", s.mode);
    s.key = a.get("key", s.key);
    s.length = parse_u32(a, "length", s.length);
    s.alphabet = a.get("alphabet", s.alphabet);
    s.canon = a.get("canon", s.canon);
    s.model = a.get("model") != "none";
    s.image_w = parse_u32(a, "image-width", s.image_w);
    s.image_h = parse_u32(a, "image-height", s.image_h);
    s.image_palette = a.get("image-palette", s.image_palette);
    s.notes = parse_u32(a, "notes", s.notes);
    s.video_w = parse_u32(a, "video-width", s.video_w);
    s.video_h = parse_u32(a, "video-height", s.video_h);
    s.frames = parse_u32(a, "video-frames", s.frames);
    s.video_palette = a.get("video-palette", s.video_palette);
    s.book_pages = parse_u32(a, "book-pages", s.book_pages);
    return s;
}

void Settings::apply(sieve::cli::Args& a) const
{
    a.opts["line"] = start_line;
    a.opts["mode"] = mode;
    a.opts["key"] = key;
    a.opts["length"] = std::to_string(length);
    a.opts["alphabet"] = alphabet;
    a.opts["canon"] = canon;
    if (model) a.opts.erase("model");
    else a.opts["model"] = "none";
    a.opts["image-width"] = std::to_string(image_w);
    a.opts["image-height"] = std::to_string(image_h);
    a.opts["image-palette"] = image_palette;
    a.opts["notes"] = std::to_string(notes);
    a.opts["video-width"] = std::to_string(video_w);
    a.opts["video-height"] = std::to_string(video_h);
    a.opts["video-frames"] = std::to_string(frames);
    a.opts["video-palette"] = video_palette;
    a.opts["book-pages"] = std::to_string(book_pages);
}

LineSize line_size(uint32_t base, uint64_t length)
{
    LineSize z;
    z.bits = double(length) * std::log2(double(base));
    // base^length mod 128, by repeated squaring; padding fills the last tile to 128 slots.
    uint64_t m = 1, b = base % sieve::kBooksPerTile;
    for (uint64_t e = length; e; e >>= 1, b = b * b % sieve::kBooksPerTile)
        if (e & 1) m = m * b % sieve::kBooksPerTile;
    const bool tiny = z.bits < 7; // fewer units than one tile
    uint64_t exact = 1;
    if (tiny)
        for (uint64_t i = 0; i < length; ++i) exact *= base;
    z.padding = tiny ? uint32_t(sieve::kBooksPerTile - exact) : uint32_t((sieve::kBooksPerTile - m) % sieve::kBooksPerTile);
    const double log10 = z.bits * std::log10(2.0);
    z.units = std::to_string(base) + "^" + std::to_string(length) + (log10 < 15 ? " = " + fixed(std::pow(10.0, log10), 0) : " = ~10^" + fixed(log10, 1));
    return z;
}

// ---------------------------------------------------------------- menu

std::array<LineSize, 5> Menu::line_sizes() const
{
    const LineSize page = line_size(alphabet_size(s_.alphabet), s_.length);
    const LineSize image = line_size(palette_size(s_.image_palette), uint64_t(s_.image_w) * s_.image_h);
    // Books: a cover, a title and book_pages pages, so |cover| * |page|^(pages + 1) books. The
    // padding follows from both factors mod 128.
    LineSize books;
    const uint64_t parts = uint64_t(s_.book_pages) + 1;
    books.bits = image.bits + double(parts) * page.bits;
    {
        auto modpow = [](uint64_t b, uint64_t e) {
            uint64_t m = 1;
            for (b %= sieve::kBooksPerTile; e; e >>= 1, b = b * b % sieve::kBooksPerTile)
                if (e & 1) m = m * b % sieve::kBooksPerTile;
            return m;
        };
        const uint64_t m = modpow(palette_size(s_.image_palette), uint64_t(s_.image_w) * s_.image_h) *
                           modpow(alphabet_size(s_.alphabet), parts * s_.length) % sieve::kBooksPerTile;
        books.padding = uint32_t((sieve::kBooksPerTile - m) % sieve::kBooksPerTile); // books.bits >= 7 always
        const double log10 = books.bits * std::log10(2.0);
        books.units = "cov*pg^" + std::to_string(parts) + " = ~10^" + fixed(log10, 1);
    }
    std::array<LineSize, 5> out{page, image, line_size(kNoteSymbols, s_.notes),
                                line_size(palette_size(s_.video_palette), uint64_t(s_.video_w) * s_.video_h * s_.frames), books};
    // A unit has at most 2^32 - 1 positions: a larger picture cannot be opened at all.
    const uint64_t kMaxPositions = 0xFFFFFFFFull;
    if (uint64_t(s_.image_w) * s_.image_h > kMaxPositions) out[1].bits = out[4].bits = HUGE_VAL;
    if (uint64_t(s_.video_w) * s_.video_h * s_.frames > kMaxPositions) out[3].bits = HUGE_VAL;
    return out;
}

// The colour to write a line's name in on the menu's black background: its edge colour, or its
// background colour when the edges are too dark to read (books: black edges on grey).
SDL_Color menu_ink(const Theme& th)
{
    const int lum = (th.edge.r * 3 + th.edge.g * 6 + th.edge.b) / 10;
    return lum < 60 ? th.bg : th.edge;
}

bool Menu::too_large() const
{
    for (const auto& z : line_sizes())
        if (z.bits > kTooLargeBits) return true;
    return false;
}

Menu::Menu(SDL_Window* window, SDL_Renderer* renderer, Settings settings, sieve::cli::FilterConfig filters, std::string filters_path)
    : window_(window), r_(renderer), s_(std::move(settings)), cfg_(std::move(filters)), cfg_path_(std::move(filters_path))
{
}

int Menu::row_count() const { return 17; }

void Menu::adjust(int dir, int step)
{
    // No upper limit but what a setting can hold: the state spaces are meant to grow without end.
    auto num = [&](uint32_t& v) {
        int64_t n = int64_t(v);
        if (step == 0) n = dir > 0 ? n * 2 : n / 2; // PgUp/PgDn: double or halve (towards powers of two)
        else n += int64_t(dir) * step;
        v = uint32_t(std::clamp<int64_t>(n, 1, int64_t(UINT32_MAX)));
    };
    switch (row_)
    {
    case 0: s_.start_line = cycle(kLines, s_.start_line, dir); break;
    case 1: s_.mode = cycle(kModes, s_.mode, dir); break;
    case 2: break; // key: typed
    case 3: num(s_.length); break;
    case 4: s_.alphabet = cycle(kAlphabets, s_.alphabet, dir); break;
    case 5: s_.canon = cycle(kCanons, s_.canon, dir); break;
    case 6: s_.model = !s_.model; break;
    case 7: num(s_.image_w); break;
    case 8: num(s_.image_h); break;
    case 9: s_.image_palette = cycle(kPalettes, s_.image_palette, dir); break;
    case 10: num(s_.notes); break;
    case 11: num(s_.video_w); break;
    case 12: num(s_.video_h); break;
    case 13: num(s_.frames); break;
    case 14: s_.video_palette = cycle(kPalettes, s_.video_palette, dir); break;
    case 15: num(s_.book_pages); break;
    default: break;
    }
}

void Menu::press(SDL_Keycode key, SDL_Keymod mod)
{
    SDL_Event e{};
    e.type = SDL_EVENT_KEY_DOWN;
    e.key.key = key;
    e.key.mod = mod;
    bool done = false;
    Result r = Result::Enter;
    handle(e, done, r);
}

void Menu::handle(const SDL_Event& event, bool& done, Result& result)
{
    // Mouse positions in the menu's own coordinates (it may be drawn scaled: see render).
    SDL_Event e = event;
    SDL_ConvertEventToRenderCoordinates(r_, &e);
    if (e.type == SDL_EVENT_QUIT) { done = true; result = Result::Quit; return; }
    auto inside = [](const SDL_FRect& r, float x, float y) { return x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h; };
    if (e.type == SDL_EVENT_MOUSE_WHEEL && overlay_ >= 0)
    {
        const int last = std::max(0, int(overlay_rows().size()) - 1);
        oscroll_ = std::clamp(oscroll_ - int(e.wheel.y), 0, last);
        orow_ = std::clamp(std::max(orow_, oscroll_), 0, last);
        return;
    }
    if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
    {
        const float mx = e.button.x, my = e.button.y;
        if (overlay_ >= 0)
        {
            if (!inside(box_, mx, my)) { save_filters(); overlay_ = -1; return; }
            for (const auto& [r, i] : row_rects_)
                if (inside(r, mx, my))
                {
                    orow_ = i;
                    overlay_change(e.button.button == SDL_BUTTON_RIGHT ? -1 : 1, false);
                }
            return;
        }
        for (int i = 0; i < 5; ++i)
            if (inside(magnifier_[i], mx, my)) open_filters(i);
        return;
    }
    if (overlay_ >= 0)
    {
        if (e.type == SDL_EVENT_KEY_DOWN) overlay_key(e.key.key, (e.key.mod & SDL_KMOD_SHIFT) != 0);
        return;
    }
    if (e.type == SDL_EVENT_TEXT_INPUT && row_ == 2)
    {
        for (const char* p = e.text.text; *p; ++p)
            if (s_.key.size() < 40 && (std::isalnum(static_cast<unsigned char>(*p)) || *p == '-' || *p == '_')) s_.key.push_back(*p);
        return;
    }
    if (e.type != SDL_EVENT_KEY_DOWN) return;
    const bool shift = (e.key.mod & SDL_KMOD_SHIFT) != 0, ctrl = (e.key.mod & SDL_KMOD_CTRL) != 0;
    const int step = ctrl ? 100 : shift ? 10 : 1;
    switch (e.key.key)
    {
    case SDLK_UP: row_ = (row_ + row_count() - 1) % row_count(); break;
    case SDLK_DOWN: row_ = (row_ + 1) % row_count(); break;
    case SDLK_LEFT: adjust(-1, step); break;
    case SDLK_RIGHT: adjust(1, step); break;
    case SDLK_PAGEUP: adjust(1, 0); break;
    case SDLK_PAGEDOWN: adjust(-1, 0); break;
    case SDLK_BACKSPACE:
        if (row_ == 2 && !s_.key.empty()) s_.key.pop_back();
        break;
    case SDLK_F:
        // The filters of the line whose settings are selected.
        // Rows 0-2 (start, ordering, key) and ENTER belong to no line; F on them does nothing.
        if (row_ >= 3 && row_ <= 15) open_filters(row_ >= 7 && row_ <= 9 ? 1 : row_ == 10 ? 2 : row_ == 15 ? 4 : row_ >= 11 ? 3 : 0);
        break;
    case SDLK_RETURN:
    case SDLK_KP_ENTER:
        if (s_.key.empty()) s_.key = "sieve";
        if (too_large()) break; // the map says which line; nothing to open on this machine
        save_filters();
        done = true;
        result = Result::Enter;
        break;
    case SDLK_ESCAPE:
        done = true;
        result = Result::Back;
        break;
    default: break;
    }
}

Menu::Result Menu::run()
{
    SDL_SetWindowRelativeMouseMode(window_, false);
    SDL_StartTextInput(window_);
    bool done = false;
    Result result = Result::Quit;
    while (!done)
    {
        SDL_Event e;
        while (SDL_PollEvent(&e)) handle(e, done, result);
        render();
        SDL_RenderPresent(r_);
    }
    SDL_StopTextInput(window_);
    SDL_SetRenderLogicalPresentation(r_, 0, 0, SDL_LOGICAL_PRESENTATION_DISABLED); // the hallway draws at full size
    return result;
}

void Menu::render()
{
    // The menu needs about 1240x700 (settings on the left, five bars on the right). In a smaller
    // window it is drawn at that size and scaled down to fit, instead of running off the edge.
    constexpr int kMinW = 1240, kMinH = 700;
    int w = 0, h = 0;
    SDL_GetRenderOutputSize(r_, &w, &h);
    if (w < kMinW || h < kMinH)
    {
        w = std::max(w, kMinW);
        h = std::max(h, kMinH);
        SDL_SetRenderLogicalPresentation(r_, w, h, SDL_LOGICAL_PRESENTATION_LETTERBOX);
    }
    else SDL_SetRenderLogicalPresentation(r_, 0, 0, SDL_LOGICAL_PRESENTATION_DISABLED);
    const float W = float(w), H = float(h);
    const SDL_Color white{255, 255, 255, 255}, grey{150, 150, 150, 255};
    SDL_SetRenderDrawColor(r_, 0, 0, 0, 255);
    SDL_RenderClear(r_);
    SDL_SetRenderDrawBlendMode(r_, SDL_BLENDMODE_BLEND);

    text(r_, 20, 16, tr("setup.title"), 3, white);
    text(r_, 20, 48, tr("setup.subtitle"), 1, grey);

    // Settings.
    struct Row
    {
        int section; // -1: none; 0-4: the line (its colour); 5: START
        std::string label, value;
    };
    auto n = [](uint32_t v) { return std::to_string(v); };
    const std::string start_key = s_.start_line == "text" ? "line.pages" : "line." + s_.start_line;
    const std::vector<Row> rows = {
        {5, tr("setup.line"), tr(start_key)},
        {-1, tr("setup.ordering"), tr("ordering." + s_.mode)},
        {-1, tr("setup.key"), s_.key + (row_ == 2 ? "_" : "")},
        {0, tr("setup.length"), trf("setup.length.value", {n(s_.length)})},
        {-1, tr("setup.alphabet"), trf("setup.alphabet.value", {s_.alphabet, n(alphabet_size(s_.alphabet))})},
        {-1, tr("setup.canon"), "canon-text-" + s_.canon},
        {-1, tr("setup.model"), tr(s_.model ? "setup.model.default" : "setup.model.none")},
        {1, tr("setup.width"), trf("setup.px", {n(s_.image_w)})},
        {-1, tr("setup.height"), trf("setup.px", {n(s_.image_h)})},
        {-1, tr("setup.palette"), trf("setup.palette.value", {s_.image_palette, n(palette_size(s_.image_palette))})},
        {2, tr("setup.notes"), n(s_.notes)},
        {3, tr("setup.width"), trf("setup.px", {n(s_.video_w)})},
        {-1, tr("setup.height"), trf("setup.px", {n(s_.video_h)})},
        {-1, tr("setup.frames"), n(s_.frames)},
        {-1, tr("setup.palette"), trf("setup.palette.value", {s_.video_palette, n(palette_size(s_.video_palette))})},
        {4, tr("setup.book_pages"), trf("setup.book_pages.value", {n(s_.book_pages)})},
        {-1, tr("setup.enter"), ""},
    };
    float y = 80;
    for (int i = 0; i < int(rows.size()); ++i)
    {
        const Row& r = rows[size_t(i)];
        if (r.section >= 0)
        {
            y += 8;
            const int li = r.section;
            const std::string head = li == 5 ? tr("setup.start") : tr(li == 4 ? kBooksTheme.key : kThemes[li].key);
            text(r_, 20, y, head, 2, li < 4 ? kThemes[li].edge : li == 4 ? menu_ink(kBooksTheme) : white);
            y += 20;
        }
        if (i == row_)
        {
            SDL_SetRenderDrawColor(r_, 255, 255, 255, 40);
            const SDL_FRect sel{14, y - 4, 560, 18};
            SDL_RenderFillRect(r_, &sel);
        }
        text(r_, 24, y, std::string(i == row_ ? "> " : "  ") + r.label, 1, white);
        text(r_, 200, y, r.value, 1, i == row_ ? white : grey);
        y += 18;
    }
    text(r_, 20, H - 40, tr("setup.footer1"), 1, grey);
    text(r_, 20, H - 26, tr("setup.footer2"), 1, grey);
    if (too_large()) text(r_, 20, H - 60, tr("setup.too_large"), 1, white);

    // The map: one bar per line, length proportional to its size in bits.
    const auto sizes = line_sizes();
    // The longest line spans the full height: nothing can leave the screen, however large.
    double scale_bits = 1;
    for (const auto& z : sizes)
        if (std::isfinite(z.bits)) scale_bits = std::max(scale_bits, z.bits);
    const float x0 = 620, pitch = std::max(124.0f, (W - x0 - 20) / 5);
    const float label = 118, top = 216, bottom = H - 60, span = bottom - top, min_bar = 12;
    text(r_, x0, 80, tr("map.title"), 1, white);
    text(r_, x0, 92, trf("map.scale", {fixed(scale_bits, 0)}), 1, grey);
    for (int i = 0; i < 5; ++i)
    {
        const bool books = i == 4;
        Theme th = books ? kBooksTheme : kThemes[i];
        const SDL_Color ink = menu_ink(th);
        const float x = x0 + i * pitch;
        const LineSize& z = sizes[size_t(i)];
        // Labels above the bar, so a full-length bar never runs into them.
        const size_t cols = size_t(std::max(8.0f, (i == 4 ? W - x - 8 : pitch - 8) / 8));
        auto clip = [&](const std::string& t) { return text_cells(t) <= cols ? t : fit_cells(t, cols - 2) + ".."; };
        const SDL_Color edge = th.edge;
        th.edge = ink; // labels in a readable colour; the bar keeps the line's own edges
        {
        // Magnifying glass: opens this line's filters.
        magnifier_[i] = {x - 2, label - 2, 20, 20};
        SDL_SetRenderDrawColor(r_, th.edge.r, th.edge.g, th.edge.b, 255);
        for (int k = 0; k < 16; ++k)
        {
            const float a0 = float(k) / 16 * 6.2831853f, a1 = float(k + 1) / 16 * 6.2831853f;
            SDL_RenderLine(r_, x + 6 + 5 * std::cos(a0), label + 6 + 5 * std::sin(a0), x + 6 + 5 * std::cos(a1), label + 6 + 5 * std::sin(a1));
        }
        SDL_RenderLine(r_, x + 10, label + 10, x + 15, label + 15);
        SDL_RenderLine(r_, x + 11, label + 10, x + 16, label + 15);
        text(r_, x + 22, label, tr(th.key), 2, th.edge);
        }
        const size_t caret = z.units.find(" = ");
        text(r_, x, label + 22, clip(trf("map.units", {z.units.substr(0, caret)})), 1, th.edge);
        text(r_, x, label + 34, clip(z.units.substr(caret + 3)), 1, th.edge);
        text(r_, x, label + 46, clip(trf("map.bits", {fixed(z.bits, 0)})), 1, th.edge);
        text(r_, x, label + 58, clip(trf("map.tiles", {fixed(std::max(0.0, z.bits * std::log10(2.0) - std::log10(128.0)), 1)})), 1, th.edge);
        text(r_, x, label + 70, clip(z.padding ? trf("map.empty_slots", {std::to_string(z.padding)}) : tr("map.whole_tiles")), 1, th.edge);
        if (z.bits > kTooLargeBits) text(r_, x, label - 14, clip(tr("map.too_large")), 1, white);
        else if (z.bits > kSlowBits) text(r_, x, label - 14, clip(tr("map.slow")), 1, grey);
        // The bar: the line's own two colours; never shorter than min_bar, never past the bottom.
        const float len = std::isfinite(z.bits) ? std::clamp(float(z.bits / scale_bits) * span, min_bar, span) : span;
        const SDL_FRect bar{x + 8, top, 40, len};
        SDL_SetRenderDrawColor(r_, th.bg.r, th.bg.g, th.bg.b, 255);
        SDL_RenderFillRect(r_, &bar);
        SDL_SetRenderDrawColor(r_, edge.r, edge.g, edge.b, 255);
        SDL_RenderRect(r_, &bar);
        const SDL_FRect inner{x + 9, top + 1, 38, len - 2};
        SDL_RenderRect(r_, &inner);
        // What survives the ticked filters, where it can be counted exactly: a filled bar inside.
        const StackInfo& info = stack_info(i);
        if (info.survivor_bits >= 0)
        {
            const float slen = std::clamp(float(info.survivor_bits / scale_bits) * span, 3.0f, len - 4);
            const SDL_FRect sv{x + 14, top + 2, 28, slen};
            SDL_SetRenderDrawColor(r_, th.edge.r, th.edge.g, th.edge.b, 200);
            SDL_RenderFillRect(r_, &sv);
            text(r_, x, label + 82, clip(trf("map.survivors", {fixed(info.survivor_bits, 0)})), 1, th.edge);
        }
        else
        {
            size_t ticked = 0;
            if (books)
                for (const auto& part : cfg_.books.parts) ticked += part.enabled.size();
            else ticked = cfg_.lines[i].enabled.size();
            if (ticked) text(r_, x, label + 82, clip(trf("map.ticked", {std::to_string(ticked)})), 1, th.edge);
        }
    }
    if (overlay_ >= 0) render_overlay(W, H);
}


// ---------------------------------------------------------------- filters overlay

sieve::FilterLine Menu::filter_line_of(int i) const
{
    sieve::FilterLine f;
    switch (i)
    {
    case 0:
    {
        const sieve::Alphabet& a = sieve::alphabet_by_id(s_.alphabet);
        f = {"text", a.id(), a.size(), s_.length, &a, 0, 0, 0};
        break;
    }
    case 1:
        f = {"image", "image/" + s_.image_palette + "/" + std::to_string(s_.image_w) + "x" + std::to_string(s_.image_h),
             palette_size(s_.image_palette), s_.image_w * s_.image_h, nullptr, s_.image_w, s_.image_h, 1};
        break;
    case 2: f = {"audio", sieve::kNotesSymbolsId, kNoteSymbols, s_.notes, nullptr, 0, 0, 0}; break;
    default:
        f = {"video", "video/" + s_.video_palette + "/" + std::to_string(s_.video_w) + "x" + std::to_string(s_.video_h) + "x" + std::to_string(s_.frames),
             palette_size(s_.video_palette), s_.video_w * s_.video_h * s_.frames, nullptr, s_.video_w, s_.video_h, s_.frames};
        break;
    }
    return f;
}

sieve::FilterLine Menu::book_part_line(int part) const
{
    if (part == 0) return filter_line_of(1);
    sieve::FilterLine f = filter_line_of(0);
    if (part == 2) f.length = uint32_t(std::min<uint64_t>(uint64_t(f.length) * s_.book_pages, UINT32_MAX));
    return f;
}

sieve::cli::LineFilters& Menu::filters_of(const ORow& row)
{
    return overlay_ == 4 ? cfg_.books.parts[std::max(0, row.part)] : cfg_.lines[overlay_];
}

const sieve::cli::LineFilters& Menu::filters_of(const ORow& row) const
{
    return overlay_ == 4 ? cfg_.books.parts[std::max(0, row.part)] : cfg_.lines[overlay_];
}

sieve::cli::FilterMode& Menu::mode_of(int line) { return line == 4 ? cfg_.books.mode : cfg_.lines[line].mode; }

const Menu::StackInfo& Menu::stack_info(int i)
{
    if (i == 4) return book_stack_info();
    if (line_sizes()[size_t(i)].bits > kTooLargeBits)
    {
        info_[i] = StackInfo{"too large", tr("status.too_large"), -1};
        return info_[i];
    }
    const sieve::FilterLine fl = filter_line_of(i);
    const sieve::cli::LineFilters& lf = cfg_.lines[i];
    std::string key = fl.symbols_id + "/" + std::to_string(fl.length) + "/" + to_string(lf.mode) + ":";
    for (const auto& n : lf.enabled) key += n + ",";
    for (const auto& [n, vals] : lf.values)
        for (const auto& [k, v] : vals) key += n + "." + k + "=" + v + ";";
    StackInfo& info = info_[i];
    if (info.key == key) return info;
    info = StackInfo{key, "", -1};
    if (line_sizes()[size_t(i)].bits > kTooLargeBits)
    {
        info.status = tr("status.too_large");
        return info;
    }
    try
    {
        const sieve::FilterStack st = sieve::cli::build_stack(fl, lf);
        if (st.empty()) info.status = tr("status.none_units");
        else if (st.ranker())
        {
            const sieve::BigUint& n = st.ranker()->count();
            info.survivor_bits = n.is_zero() ? 0 : n.log10_approx() / std::log10(2.0);
            info.status = trf(n.is_zero() ? "status.survivors_none" : "status.survivors",
                              {n.log10_approx() < 15 ? n.to_decimal() : "~10^" + fixed(n.log10_approx(), 1)});
        }
        else info.status = trf("status.not_countable", {st.compact_blocker()});
    }
    catch (const std::exception& e)
    {
        info.status = trf("status.error", {e.what()});
    }
    return info;
}

// The books line: survivors = cover survivors * title survivors * body survivors, each part
// counted exactly where its stack can rank (a part with no filters keeps all its units).
const Menu::StackInfo& Menu::book_stack_info()
{
    StackInfo& info = info_[4];
    if (line_sizes()[4].bits > kTooLargeBits)
    {
        info = StackInfo{"too large", tr("status.too_large"), -1};
        return info;
    }
    std::string key = std::string("books/") + to_string(cfg_.books.mode) + "/";
    for (int part = 0; part < 3; ++part)
    {
        const sieve::FilterLine fl = book_part_line(part);
        const sieve::cli::LineFilters& lf = cfg_.books.parts[part];
        key += fl.symbols_id + "/" + std::to_string(fl.length) + ":";
        for (const auto& n : lf.enabled) key += n + ",";
        for (const auto& [n, vals] : lf.values)
            for (const auto& [k, v] : vals) key += n + "." + k + "=" + v + ";";
        key += "|";
    }
    if (info.key == key) return info;
    info = StackInfo{key, "", -1};
    static const char* const names[3] = {"cover", "title", "pages"};
    try
    {
        double bits = 0, log10 = 0;
        bool exact = true, any = false, none_survive = false;
        std::string blocker;
        for (int part = 0; part < 3; ++part)
        {
            const sieve::FilterLine fl = book_part_line(part);
            const sieve::cli::LineFilters& lf = cfg_.books.parts[part];
            const double all = double(fl.length) * std::log2(double(fl.base));
            if (lf.enabled.empty() || (part == 2 && s_.book_pages == 0))
            {
                bits += all;
                continue;
            }
            const sieve::FilterStack st = sieve::cli::build_stack(fl, lf);
            if (st.empty()) { bits += all; continue; }
            any = true;
            if (!st.ranker())
            {
                exact = false;
                if (blocker.empty()) blocker = std::string(names[part]) + ": " + st.compact_blocker();
                continue;
            }
            const sieve::BigUint& n = st.ranker()->count();
            if (n.is_zero()) none_survive = true;
            else bits += n.log10_approx() / std::log10(2.0);
        }
        log10 = bits * std::log10(2.0);
        if (!any) info.status = tr("status.none_books");
        else if (!exact) info.status = trf("status.not_countable", {blocker});
        else if (none_survive)
        {
            info.survivor_bits = 0;
            info.status = trf("status.survivors_none", {"0"});
        }
        else
        {
            info.survivor_bits = bits;
            info.status = trf("status.books", {fixed(log10, 1)});
        }
    }
    catch (const std::exception& e)
    {
        info.status = trf("status.error", {e.what()});
    }
    return info;
}

std::vector<Menu::ORow> Menu::overlay_rows() const
{
    std::vector<ORow> rows{{ORow::Kind::Mode, "", ""}};
    if (overlay_ == 4)
    {
        for (int part = 0; part < 3; ++part)
        {
            rows.push_back({ORow::Kind::Header, "", "", part});
            const sieve::cli::LineFilters& lf = cfg_.books.parts[part];
            for (const sieve::FilterSpec* f : sieve::filters_for(book_part_line(part)))
            {
                rows.push_back({ORow::Kind::Filter, f->name(), "", part});
                if (lf.is_enabled(f->name()))
                    for (const auto& p : f->params) rows.push_back({ORow::Kind::Param, f->name(), p.key, part});
            }
        }
        return rows;
    }
    const sieve::cli::LineFilters& lf = cfg_.lines[overlay_];
    for (const sieve::FilterSpec* f : sieve::filters_for(filter_line_of(overlay_)))
    {
        rows.push_back({ORow::Kind::Filter, f->name(), ""});
        if (lf.is_enabled(f->name()))
            for (const auto& p : f->params) rows.push_back({ORow::Kind::Param, f->name(), p.key});
    }
    return rows;
}

void Menu::save_filters()
{
    try
    {
        cfg_.save(cfg_path_);
    }
    catch (const std::exception&)
    {
        // Read-only folder: the settings still apply to this session.
    }
}

void Menu::overlay_change(int dir, bool big)
{
    const auto rows = overlay_rows();
    if (orow_ < 0 || orow_ >= int(rows.size())) return;
    const ORow& row = rows[size_t(orow_)];
    sieve::cli::LineFilters& lf = filters_of(row);
    using sieve::cli::FilterMode;
    switch (row.kind)
    {
    case ORow::Kind::Header: return;
    case ORow::Kind::Mode:
    {
        const FilterMode order[4] = {FilterMode::Off, FilterMode::Mark, FilterMode::Hide, FilterMode::Compact};
        FilterMode& mode = mode_of(overlay_);
        int m = 0;
        while (order[m] != mode) ++m;
        mode = order[((m + dir) % 4 + 4) % 4];
        break;
    }
    case ORow::Kind::Filter: lf.set_enabled(row.filter, !lf.is_enabled(row.filter)); break;
    case ORow::Kind::Param:
    {
        const sieve::FilterSpec* spec = sieve::find_filter(row.filter);
        const sieve::FilterParam* p = nullptr;
        for (const auto& q : spec->params)
            if (q.key == row.key) p = &q;
        sieve::FilterValues& vals = lf.values[row.filter];
        if (p->kind == sieve::FilterParam::Kind::Integer)
        {
            int64_t v = 0;
            try { v = sieve::param_int(*spec, vals, p->key); } catch (const std::exception&) { v = std::stoll(p->default_value); }
            v = std::clamp<int64_t>(v + dir * p->step * (big ? 10 : 1), p->min, p->max);
            vals[p->key] = std::to_string(v);
        }
        else
        {
            // Registered data: cycle through the registry's ids ("" = the default).
            std::vector<std::string> choices{""};
            try
            {
                if (p->key == "dictionary")
                    for (const auto& e : sieve::cli::load_registry().entries) choices.push_back(e.id);
                if (p->key == "model")
                    for (const auto& e : sieve::cli::load_model_registry().entries)
                        if (e.symbols == (overlay_ == 4 ? book_part_line(row.part) : filter_line_of(overlay_)).symbols_id) choices.push_back(e.id);
            }
            catch (const std::exception&)
            {
            }
            const std::string cur = sieve::param_value(*spec, vals, p->key);
            auto it = std::find(choices.begin(), choices.end(), cur);
            const int i = it == choices.end() ? 0 : int(it - choices.begin());
            vals[p->key] = choices[size_t(((i + dir) % int(choices.size()) + int(choices.size())) % int(choices.size()))];
        }
        break;
    }
    }
    save_filters();
}

void Menu::overlay_key(SDL_Keycode key, bool shift)
{
    const int n = int(overlay_rows().size());
    switch (key)
    {
    case SDLK_UP: orow_ = (orow_ + n - 1) % n; break;
    case SDLK_DOWN: orow_ = (orow_ + 1) % n; break;
    case SDLK_LEFT: overlay_change(-1, shift); break;
    case SDLK_RIGHT:
    case SDLK_SPACE:
    case SDLK_RETURN:
    case SDLK_KP_ENTER: overlay_change(1, shift); break;
    case SDLK_ESCAPE:
    case SDLK_F:
        save_filters();
        overlay_ = -1;
        break;
    default: break;
    }
    orow_ = std::clamp(orow_, 0, int(overlay_rows().size()) - 1);
}

void Menu::render_overlay(float W, float H)
{
    const SDL_Color white{255, 255, 255, 255}, grey{150, 150, 150, 255};
    const Theme& th = overlay_ == 4 ? kBooksTheme : kThemes[overlay_];
    const SDL_Color title_ink = overlay_ == 4 ? menu_ink(kBooksTheme) : th.edge;
    box_ = {600, 70, W - 614, H - 124};
    SDL_SetRenderDrawColor(r_, 0, 0, 0, 250);
    SDL_RenderFillRect(r_, &box_);
    SDL_SetRenderDrawColor(r_, 255, 255, 255, 255);
    SDL_RenderRect(r_, &box_);
    const SDL_FRect inner{box_.x + 2, box_.y + 2, box_.w - 4, box_.h - 4};
    SDL_RenderRect(r_, &inner);
    const float x = box_.x + 14;
    const size_t cols = size_t((box_.w - 60) / 8);
    text(r_, x, box_.y + 10, trf("filters.title", {tr(th.key)}), 2, title_ink);
    text(r_, x, box_.y + 32, tr(overlay_ == 4 ? "filters.intro.books" : "filters.intro"), 1, grey);

    const sieve::cli::FilterMode mode = overlay_ == 4 ? cfg_.books.mode : cfg_.lines[overlay_].mode;
    const auto rows = overlay_rows();
    // Layout: each row's height, then scroll so the selected row stays in view.
    struct Item
    {
        std::vector<std::string> lines;
        float h;
    };
    std::vector<Item> items;
    for (const ORow& row : rows)
    {
        Item it;
        const sieve::cli::LineFilters& lf = filters_of(row);
        if (row.kind == ORow::Kind::Mode)
            it.lines = {trf("filters.mode", {tr(std::string("mode.") + to_string(mode))}), "    " + tr("filters.mode.help1"),
                        "    " + tr("filters.mode.help2")};
        else if (row.kind == ORow::Kind::Header)
        {
            static const char* const heads[3] = {"filters.part.cover", "filters.part.title", "filters.part.pages"};
            const std::string head = heads[row.part];
            std::string sub = row.part == 2 ? trf(head + ".help", {std::to_string(uint64_t(s_.book_pages) * s_.length)}) : tr(head + ".help");
            it.lines = {"-- " + tr(head) + " --"};
            while (!sub.empty())
            {
                size_t cut = sub.size() <= cols - 4 ? sub.size() : sub.rfind(' ', cols - 4);
                if (cut == std::string::npos || cut == 0) cut = std::min(sub.size(), cols - 4);
                it.lines.push_back("    " + sub.substr(0, cut));
                sub = sub.substr(std::min(sub.size(), cut + 1));
            }
        }
        else if (row.kind == ORow::Kind::Filter)
        {
            const sieve::FilterSpec* f = sieve::find_filter(row.filter);
            it.lines = {std::string(lf.is_enabled(row.filter) ? "[x] " : "[ ] ") + f->name()};
            std::string d = tr_or("filter." + f->name(), f->description);
            while (!d.empty())
            {
                size_t cut = d.size() <= cols - 4 ? d.size() : d.rfind(' ', cols - 4);
                if (cut == std::string::npos || cut == 0) cut = std::min(d.size(), cols - 4);
                it.lines.push_back("    " + d.substr(0, cut));
                d = d.substr(std::min(d.size(), cut + 1));
            }
        }
        else
        {
            const sieve::FilterSpec* f = sieve::find_filter(row.filter);
            const auto vit = lf.values.find(row.filter);
            std::string v;
            try { v = sieve::param_value(*f, vit == lf.values.end() ? sieve::FilterValues{} : vit->second, row.key); } catch (...) {}
            std::string desc;
            for (const auto& p : f->params)
                if (p.key == row.key) desc = tr_or("filter." + f->name() + "." + p.key, p.description);
            it.lines = {"      " + row.key + " = " + (v.empty() ? tr("filters.default") : v) + "     " + desc};
        }
        it.h = float(it.lines.size()) * 12 + 8;
        items.push_back(it);
    }
    const float top = box_.y + 54, bottom = box_.y + box_.h - 44;
    // Scroll: first row shown.
    oscroll_ = std::clamp(oscroll_, 0, std::max(0, int(items.size()) - 1));
    if (orow_ < oscroll_) oscroll_ = orow_;
    for (;;)
    {
        float h = 0;
        for (int i = oscroll_; i <= orow_ && i < int(items.size()); ++i) h += items[size_t(i)].h;
        if (h <= bottom - top || oscroll_ >= orow_) break;
        ++oscroll_;
    }
    row_rects_.clear();
    float y = top;
    for (int i = oscroll_; i < int(items.size()); ++i)
    {
        const Item& it = items[size_t(i)];
        if (y + it.h > bottom) break;
        const SDL_FRect r{box_.x + 6, y - 3, box_.w - 12, it.h};
        row_rects_.emplace_back(r, i);
        if (i == orow_)
        {
            SDL_SetRenderDrawColor(r_, 255, 255, 255, 40);
            SDL_RenderFillRect(r_, &r);
        }
        for (size_t k = 0; k < it.lines.size(); ++k)
            text(r_, x, y + float(k) * 12, (k == 0 && i == orow_ ? "> " : "  ") + it.lines[k], 1, k == 0 ? white : grey);
        y += it.h;
    }
    if (oscroll_ > 0) text(r_, box_.x + box_.w - 90, top - 12, tr("filters.more_above"), 1, grey);
    if (y < bottom && false) {}
    const StackInfo& info = stack_info(overlay_);
    text(r_, x, box_.y + box_.h - 36, info.status.substr(0, cols), 1, white);
    text(r_, x, box_.y + box_.h - 20, tr("filters.footer"), 1, grey);
}

} // namespace hallway
