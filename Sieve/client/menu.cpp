#include "menu.hpp"

#include "theme.hpp"

#include "sieve/alphabet.hpp"
#include "sieve/corridor.hpp"
#include "sieve/image.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <vector>

namespace hallway {

namespace {

const std::vector<std::string> kLines = {"text", "image", "audio", "video"};
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

// The largest state space the limits allow, in bits: the fixed scale of the map.
double max_bits()
{
    const double text = Limits::kTextLength * std::log2(95.0);
    const double image = double(Limits::kImageSide) * Limits::kImageSide * 24;
    const double audio = Limits::kNotes * std::log2(double(kNoteSymbols));
    const double video = double(Limits::kVideoSide) * Limits::kVideoSide * Limits::kVideoFrames * 24;
    return std::max({text, image, audio, video});
}

void text(SDL_Renderer* r, float x, float y, const std::string& s, float scale, SDL_Color c)
{
    SDL_SetRenderScale(r, scale, scale);
    SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);
    SDL_RenderDebugText(r, x / scale, y / scale, s.c_str());
    SDL_SetRenderScale(r, 1, 1);
}

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
    s.mode = a.get("mode", s.mode);
    s.key = a.get("key", s.key);
    s.length = std::min(parse_u32(a, "length", s.length), Limits::kTextLength);
    s.alphabet = a.get("alphabet", s.alphabet);
    s.canon = a.get("canon", s.canon);
    s.model = a.get("model") != "none";
    s.image_w = std::min(parse_u32(a, "image-width", s.image_w), Limits::kImageSide);
    s.image_h = std::min(parse_u32(a, "image-height", s.image_h), Limits::kImageSide);
    s.image_palette = a.get("image-palette", s.image_palette);
    s.notes = std::min(parse_u32(a, "notes", s.notes), Limits::kNotes);
    s.video_w = std::min(parse_u32(a, "video-width", s.video_w), Limits::kVideoSide);
    s.video_h = std::min(parse_u32(a, "video-height", s.video_h), Limits::kVideoSide);
    s.frames = std::min(parse_u32(a, "video-frames", s.frames), Limits::kVideoFrames);
    s.video_palette = a.get("video-palette", s.video_palette);
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

Menu::Menu(SDL_Window* window, SDL_Renderer* renderer, Settings settings) : window_(window), r_(renderer), s_(std::move(settings)) {}

int Menu::row_count() const { return 16; }

void Menu::adjust(int dir, int step)
{
    auto num = [&](uint32_t& v, uint32_t hi) {
        int64_t n = int64_t(v);
        if (step == 0) n = dir > 0 ? n * 2 : n / 2; // PgUp/PgDn: double or halve (towards powers of two)
        else n += int64_t(dir) * step;
        v = uint32_t(std::clamp<int64_t>(n, 1, hi));
    };
    switch (row_)
    {
    case 0: s_.start_line = cycle(kLines, s_.start_line, dir); break;
    case 1: s_.mode = cycle(kModes, s_.mode, dir); break;
    case 2: break; // key: typed
    case 3: num(s_.length, Limits::kTextLength); break;
    case 4: s_.alphabet = cycle(kAlphabets, s_.alphabet, dir); break;
    case 5: s_.canon = cycle(kCanons, s_.canon, dir); break;
    case 6: s_.model = !s_.model; break;
    case 7: num(s_.image_w, Limits::kImageSide); break;
    case 8: num(s_.image_h, Limits::kImageSide); break;
    case 9: s_.image_palette = cycle(kPalettes, s_.image_palette, dir); break;
    case 10: num(s_.notes, Limits::kNotes); break;
    case 11: num(s_.video_w, Limits::kVideoSide); break;
    case 12: num(s_.video_h, Limits::kVideoSide); break;
    case 13: num(s_.frames, Limits::kVideoFrames); break;
    case 14: s_.video_palette = cycle(kPalettes, s_.video_palette, dir); break;
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

void Menu::handle(const SDL_Event& e, bool& done, Result& result)
{
    if (e.type == SDL_EVENT_QUIT) { done = true; result = Result::Quit; return; }
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
    case SDLK_S:
        if (row_ != 2) fit_ = !fit_;
        break;
    case SDLK_BACKSPACE:
        if (row_ == 2 && !s_.key.empty()) s_.key.pop_back();
        break;
    case SDLK_RETURN:
    case SDLK_KP_ENTER:
        if (s_.key.empty()) s_.key = "sieve";
        done = true;
        result = Result::Enter;
        break;
    case SDLK_ESCAPE:
        done = true;
        result = Result::Quit;
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
    return result;
}

void Menu::render()
{
    int w = 0, h = 0;
    SDL_GetCurrentRenderOutputSize(r_, &w, &h);
    const float W = float(w), H = float(h);
    const SDL_Color white{255, 255, 255, 255}, grey{150, 150, 150, 255};
    SDL_SetRenderDrawColor(r_, 0, 0, 0, 255);
    SDL_RenderClear(r_);
    SDL_SetRenderDrawBlendMode(r_, SDL_BLENDMODE_BLEND);

    text(r_, 20, 16, "SIEVE - set up the four lines", 3, white);
    text(r_, 20, 48, "Every possible unit of each line has a book. Change a line's shape to change its state space.", 1, grey);

    // Settings.
    struct Row
    {
        const char* section;
        std::string label, value;
    };
    const std::vector<Row> rows = {
        {"START", "line", s_.start_line},
        {nullptr, "ordering", s_.mode},
        {nullptr, "key", s_.key + (row_ == 2 ? "_" : "")},
        {"TEXT", "length", std::to_string(s_.length) + " characters"},
        {nullptr, "alphabet", s_.alphabet + " (" + std::to_string(alphabet_size(s_.alphabet)) + " symbols)"},
        {nullptr, "warp rules", "canon-text-" + s_.canon},
        {nullptr, "model", s_.model ? "default (guided ordering available)" : "none"},
        {"IMAGE", "width", std::to_string(s_.image_w) + " px"},
        {nullptr, "height", std::to_string(s_.image_h) + " px"},
        {nullptr, "palette", s_.image_palette + " (" + std::to_string(palette_size(s_.image_palette)) + " colours)"},
        {"AUDIO", "notes", std::to_string(s_.notes)},
        {"VIDEO", "width", std::to_string(s_.video_w) + " px"},
        {nullptr, "height", std::to_string(s_.video_h) + " px"},
        {nullptr, "frames", std::to_string(s_.frames)},
        {nullptr, "palette", s_.video_palette + " (" + std::to_string(palette_size(s_.video_palette)) + " colours)"},
        {nullptr, ">> ENTER THE HALLWAY <<", ""},
    };
    float y = 80;
    for (int i = 0; i < int(rows.size()); ++i)
    {
        const Row& r = rows[size_t(i)];
        if (r.section)
        {
            y += 8;
            const std::string sec = r.section;
            const int li = sec == "TEXT" ? 0 : sec == "IMAGE" ? 1 : sec == "AUDIO" ? 2 : sec == "VIDEO" ? 3 : 4;
            text(r_, 20, y, r.section, 2, li < 4 ? kThemes[li].edge : white);
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
    text(r_, 20, H - 40, "Up/Down choose   Left/Right change (Shift x10, Ctrl x100)   PgUp/PgDn double/halve", 1, grey);
    text(r_, 20, H - 26, "S map scale   type to edit the key   Enter walk in   Esc quit", 1, grey);

    // The map: one bar per line, length proportional to its size in bits.
    const LineSize sizes[4] = {
        line_size(alphabet_size(s_.alphabet), s_.length),
        line_size(palette_size(s_.image_palette), uint64_t(s_.image_w) * s_.image_h),
        line_size(kNoteSymbols, s_.notes),
        line_size(palette_size(s_.video_palette), uint64_t(s_.video_w) * s_.video_h * s_.frames),
    };
    double scale_bits = max_bits();
    if (fit_)
    {
        scale_bits = 1;
        for (const auto& z : sizes) scale_bits = std::max(scale_bits, z.bits);
    }
    const float x0 = 620, pitch = std::max(150.0f, (W - x0 - 20) / 4);
    const float label = 118, top = 200, bottom = H - 60, span = bottom - top, min_bar = 12;
    text(r_, x0, 80, "MAP  (length = size in bits; one copy each)", 1, white);
    text(r_, x0, 92, fit_ ? "scale: fitted to these settings (S: fixed)"
                          : "scale: fixed to the largest the limits allow, " + fixed(scale_bits, 0) + " bits (S: fit)",
         1, grey);
    for (int i = 0; i < 4; ++i)
    {
        const Theme& th = kThemes[i];
        const float x = x0 + i * pitch;
        const LineSize& z = sizes[i];
        // Labels above the bar, so a full-length bar never runs into them.
        text(r_, x, label, th.name, 2, th.edge);
        const size_t caret = z.units.find(" = ");
        text(r_, x, label + 22, z.units.substr(0, caret) + " units", 1, th.edge);
        text(r_, x, label + 34, z.units.substr(caret + 3), 1, th.edge);
        text(r_, x, label + 46, fixed(z.bits, 0) + " bits", 1, th.edge);
        text(r_, x, label + 58, "~10^" + fixed(std::max(0.0, z.bits * std::log10(2.0) - std::log10(128.0)), 1) + " tiles", 1, th.edge);
        text(r_, x, label + 70, z.padding ? std::to_string(z.padding) + " empty slots" : "fills whole tiles", 1, th.edge);
        // The bar: the line's own two colours; never shorter than min_bar, never past the bottom.
        const float len = std::clamp(float(z.bits / scale_bits) * span, min_bar, span);
        const SDL_FRect bar{x + 8, top, 40, len};
        SDL_SetRenderDrawColor(r_, th.bg.r, th.bg.g, th.bg.b, 255);
        SDL_RenderFillRect(r_, &bar);
        SDL_SetRenderDrawColor(r_, th.edge.r, th.edge.g, th.edge.b, 255);
        SDL_RenderRect(r_, &bar);
        const SDL_FRect inner{x + 9, top + 1, 38, len - 2};
        SDL_RenderRect(r_, &inner);
    }
}

} // namespace hallway
