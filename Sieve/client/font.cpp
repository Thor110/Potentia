#include "font.hpp"

#include "cli/dictionaries.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

namespace hallway {

namespace fs = std::filesystem;

namespace {

struct Glyph
{
    uint8_t width = 8;          // pixels: 8 or 16
    std::vector<uint16_t> rows; // top row first, leftmost pixel = bit (width - 1)
};

struct Font
{
    std::string name;
    int height = 8;
    std::unordered_map<char32_t, Glyph> glyphs;
    // Glyph atlases, one per block of 256 code points, made on first use for one renderer:
    // 16 x 16 cells of 16 x height pixels, white with the glyph in alpha.
    SDL_Renderer* renderer = nullptr;
    std::unordered_map<char32_t, SDL_Texture*> atlases;

    ~Font() { release(); }
    void release()
    {
        for (auto& [block, tex] : atlases)
            if (tex) SDL_DestroyTexture(tex);
        atlases.clear();
        renderer = nullptr;
    }
    SDL_Texture* atlas(SDL_Renderer* r, char32_t block)
    {
        if (renderer != r)
        {
            release();
            renderer = r;
        }
        auto it = atlases.find(block);
        if (it != atlases.end()) return it->second;
        const int cw = 16, ch = height;
        std::vector<uint32_t> px(size_t(cw * 16) * size_t(ch * 16), 0x00FFFFFFu);
        for (int i = 0; i < 256; ++i)
        {
            const auto g = glyphs.find(block * 256 + char32_t(i));
            if (g == glyphs.end()) continue;
            const int ox = (i % 16) * cw, oy = (i / 16) * ch;
            for (int y = 0; y < ch && y < int(g->second.rows.size()); ++y)
                for (int x = 0; x < g->second.width; ++x)
                    if (g->second.rows[size_t(y)] >> (g->second.width - 1 - x) & 1)
                        px[size_t(oy + y) * size_t(cw * 16) + size_t(ox + x)] = 0xFFFFFFFFu;
        }
        SDL_Texture* tex = SDL_CreateTexture(r, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, cw * 16, ch * 16);
        if (tex)
        {
            SDL_UpdateTexture(tex, nullptr, px.data(), cw * 16 * 4);
            SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
            SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_NEAREST);
        }
        atlases[block] = tex;
        return tex;
    }
};

std::unique_ptr<Font> g_font, g_fallback;
std::string g_name = "debug";

fs::path font_folder()
{
    std::vector<fs::path> folders = {fs::path("data") / "fonts"};
    if (const fs::path exe = sieve::cli::executable_dir(); !exe.empty())
    {
        folders.push_back(exe / "fonts");
        folders.push_back(exe / ".." / "data" / "fonts");
    }
    for (const auto& f : folders)
    {
        std::error_code ec;
        if (fs::is_directory(f, ec)) return f;
    }
    return folders.front();
}

int hex_digit(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}

std::unique_ptr<Font> load(const std::string& name)
{
    if (name.empty() || name.find_first_of("/\\.") != std::string::npos) return nullptr;
    std::ifstream in(font_folder() / (name + ".hex"), std::ios::binary);
    if (!in) return nullptr;
    auto f = std::make_unique<Font>();
    f->name = name;
    f->height = 16;
    std::string line;
    while (std::getline(in, line))
    {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue;
        if (line[0] == '#')
        {
            if (const size_t p = line.find("height:"); p != std::string::npos)
            {
                try { f->height = std::clamp(std::stoi(line.substr(p + 7)), 1, 64); } catch (...) {}
            }
            continue;
        }
        const size_t colon = line.find(':');
        if (colon == std::string::npos) continue;
        char32_t cp = 0;
        bool ok = colon > 0 && colon <= 6;
        for (size_t i = 0; ok && i < colon; ++i)
        {
            const int d = hex_digit(line[i]);
            ok = d >= 0;
            cp = cp * 16 + char32_t(d);
        }
        const std::string hex = line.substr(colon + 1);
        const size_t bits = hex.size() * 4;
        if (!ok || bits % size_t(f->height) != 0) continue;
        const size_t width = bits / size_t(f->height);
        if (width != 8 && width != 16) continue;
        Glyph g;
        g.width = uint8_t(width);
        const size_t per_row = width / 4;
        for (int y = 0; y < f->height && ok; ++y)
        {
            uint16_t row = 0;
            for (size_t k = 0; k < per_row; ++k)
            {
                const int d = hex_digit(hex[size_t(y) * per_row + k]);
                ok = ok && d >= 0;
                row = uint16_t(row * 16 + (d < 0 ? 0 : d));
            }
            g.rows.push_back(row);
        }
        if (ok) f->glyphs[cp] = std::move(g);
    }
    if (f->glyphs.empty()) return nullptr;
    return f;
}

// Next code point of UTF-8 text; malformed bytes read as U+FFFD.
char32_t next_cp(const std::string& s, size_t& i)
{
    const unsigned char c = static_cast<unsigned char>(s[i++]);
    if (c < 0x80) return c;
    // Lead bytes C2..F4 only: C0/C1 would be overlong, F5 and up beyond U+10FFFF.
    const int n = c >= 0xF5 ? -1 : c >= 0xF0 ? 3 : c >= 0xE0 ? 2 : c >= 0xC2 ? 1 : -1;
    if (n < 0) return 0xFFFD;
    char32_t cp = c & (0x3F >> n);
    for (int k = 0; k < n; ++k)
    {
        if (i >= s.size() || (static_cast<unsigned char>(s[i]) & 0xC0) != 0x80) return 0xFFFD;
        cp = (cp << 6) | (static_cast<unsigned char>(s[i++]) & 0x3F);
    }
    static constexpr char32_t kMin[4] = {0, 0x80, 0x800, 0x10000};
    if (cp < kMin[n] || cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF)) return 0xFFFD; // overlong, too big, surrogate
    return cp;
}

// The font and glyph that draw a code point (nullptr font: SDL's debug font).
std::pair<Font*, const Glyph*> find(char32_t cp)
{
    for (Font* f : {g_font.get(), g_fallback.get()})
        if (f)
            if (auto it = f->glyphs.find(cp); it != f->glyphs.end()) return {f, &it->second};
    for (Font* f : {g_font.get(), g_fallback.get()})
        if (f)
            if (auto it = f->glyphs.find(U'?'); it != f->glyphs.end()) return {f, &it->second};
    return {nullptr, nullptr};
}

// Advance in pixels at scale 1: a glyph is drawn 8 pixels tall, keeping its shape, so an 8x8
// glyph takes 8, a 16x16 one (a CJK character in Unifont) 8, and an 8x16 one 4.
float advance_of(char32_t cp)
{
    const auto [f, g] = find(cp);
    return f && g ? float(g->width) * 8.0f / float(f->height) : 8.0f;
}

} // namespace

bool set_font(const std::string& name)
{
    if (name == "debug")
    {
        g_font.reset();
        g_name = name;
        return true;
    }
    if (g_font && g_font->name == name) return true;
    auto f = load(name);
    if (!f) return false;
    g_font = std::move(f);
    g_name = name;
    if (!g_fallback && name != "sieve8x8") g_fallback = load("sieve8x8");
    return true;
}

const std::string& font_name() { return g_name; }

float text_width(const std::string& s, float scale)
{
    float w = 0;
    for (size_t i = 0; i < s.size();) w += advance_of(next_cp(s, i));
    return w * scale;
}

size_t text_cells(const std::string& s) { return size_t(std::ceil(text_width(s, 1) / 8.0f)); }

std::string fit_cells(const std::string& s, size_t cells)
{
    float w = 0;
    size_t i = 0;
    while (i < s.size())
    {
        size_t j = i;
        const float a = advance_of(next_cp(s, j));
        if (w + a > float(cells) * 8.0f + 0.01f) break;
        w += a;
        i = j;
    }
    return s.substr(0, i);
}

void draw_text(SDL_Renderer* r, float x, float y, const std::string& s, float scale, SDL_Color c)
{
    if (!g_font)
    {
        SDL_SetRenderScale(r, scale, scale);
        SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);
        SDL_RenderDebugText(r, x / scale, y / scale, s.c_str());
        SDL_SetRenderScale(r, 1, 1);
        return;
    }
    // Glyphs sit on the 8-pixel grid: a font `height` pixels tall is drawn 8 * scale tall.
    float cx = x;
    for (size_t i = 0; i < s.size();)
    {
        const char32_t cp = next_cp(s, i);
        const auto [f, g] = find(cp);
        const float cell = advance_of(cp) * scale;
        if (f && g && cp != U' ')
        {
            const char32_t code = f->glyphs.count(cp) ? cp : U'?';
            if (SDL_Texture* tex = f->atlas(r, code / 256))
            {
                SDL_SetTextureColorMod(tex, c.r, c.g, c.b);
                SDL_SetTextureAlphaMod(tex, c.a);
                const int i256 = int(code % 256);
                const float k = 8.0f * scale / float(f->height);
                const SDL_FRect src{float((i256 % 16) * 16), float((i256 / 16) * f->height), float(g->width), float(f->height)};
                const SDL_FRect dst{cx, y, float(g->width) * k, 8.0f * scale};
                SDL_RenderTexture(r, tex, &src, &dst);
            }
        }
        cx += cell;
    }
}

void release_fonts()
{
    if (g_font) g_font->release();
    if (g_fallback) g_fallback->release();
}

} // namespace hallway
