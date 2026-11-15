// Sieve hallway — the one footer every screen draws its keys through (IDEAS 19.224); see footer.hpp.
#include "footer.hpp"

#include "font.hpp"

#include "sieve/footer_wrap.hpp"

#include <algorithm>
#include <cmath>
#include <string>
#include <utility>
#include <vector>

namespace hallway {

namespace {

// What --footers says of one screen's footer, as last drawn.
struct FooterSeen
{
    std::string screen;
    size_t rows = 0;
    float widest = 0, window = 0, room = 0;
};

bool g_report = false;
std::vector<FooterSeen> g_seen;

// The window's width as the screen draws in it: a screen drawn at a size of its own and scaled to fit
// (the setup menu in a small window) is measured against that size, which is the window it lays out in.
float window_width(SDL_Renderer* r)
{
    int lw = 0, lh = 0;
    SDL_RendererLogicalPresentation mode = SDL_LOGICAL_PRESENTATION_DISABLED;
    if (SDL_GetRenderLogicalPresentation(r, &lw, &lh, &mode) && mode != SDL_LOGICAL_PRESENTATION_DISABLED && lw > 0) return float(lw);
    int w = 0, h = 0;
    SDL_GetCurrentRenderOutputSize(r, &w, &h);
    return float(w);
}

} // namespace

std::vector<std::string> wrap_keys(const std::string& s, size_t width) { return sieve::wrap_footer(s, width); }

std::vector<FooterLine> footer_rows(const std::vector<FooterLine>& lines, float room, float scale)
{
    std::vector<FooterLine> out;
    const auto measure = [scale](const std::string& t) { return double(text_width(t, scale)); };
    for (const FooterLine& l : lines)
        for (std::string& row : sieve::wrap_footer(l.text, double(std::max(8.0f * scale, room)), measure)) out.push_back({std::move(row), l.colour});
    return out;
}

float footer_extra(const std::vector<FooterLine>& lines, float room, float pitch, float scale)
{
    const size_t rows = footer_rows(lines, room, scale).size();
    return float(rows > lines.size() ? rows - lines.size() : 0) * pitch;
}

size_t draw_footer(SDL_Renderer* r, const char* screen, float x, float y, float room, const std::vector<FooterLine>& lines, float pitch, FooterAlign align,
                   float scale)
{
    const std::vector<FooterLine> rows = footer_rows(lines, room, scale);
    float widest = 0;
    for (size_t i = 0; i < rows.size(); ++i)
    {
        const float w = text_width(rows[i].text, scale);
        widest = std::max(widest, w);
        const float ry = y - float(rows.size() - 1 - i) * pitch;
        draw_text(r, align == FooterAlign::Centre ? x - w / 2 : x, ry, rows[i].text, scale, rows[i].colour);
    }
    if (g_report)
    {
        const std::string name = screen;
        auto it = std::find_if(g_seen.begin(), g_seen.end(), [&](const FooterSeen& s) { return s.screen == name; });
        if (it == g_seen.end()) it = g_seen.insert(g_seen.end(), FooterSeen{name});
        *it = {name, rows.size(), widest, window_width(r), room};
    }
    return rows.size();
}

size_t draw_footer(SDL_Renderer* r, const char* screen, float x, float y, float room, const std::string& text, SDL_Color colour, float pitch, FooterAlign align,
                   float scale)
{
    return draw_footer(r, screen, x, y, room, std::vector<FooterLine>{{text, colour}}, pitch, align, scale);
}

void set_footer_report(bool on) { g_report = on; }
bool footer_report() { return g_report; }

void say_footers(std::ostream& out)
{
    for (const FooterSeen& s : g_seen)
        out << "footer " << s.screen << ": " << s.rows << (s.rows == 1 ? " line" : " lines") << ", widest " << int(std::ceil(s.widest)) << " of window "
            << int(s.window) << " (room " << int(std::floor(s.room)) << ")\n";
    g_seen.clear();
}

} // namespace hallway
