// Sieve hallway — the music visualiser's rings (see rings.hpp).
#include "rings.hpp"

#include "font.hpp"
#include "strings.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

namespace hallway {

namespace {

constexpr float kPi = 3.14159265358979f;

// The colour wheel of the twelve notes: C red, round the hues to B.
SDL_FColor hue(int pc, float light)
{
    const float h = float(((pc % 12) + 12) % 12) / 12.0f * 6.0f;
    const float x = 1.0f - std::fabs(std::fmod(h, 2.0f) - 1.0f);
    float r = 0, g = 0, b = 0;
    if (h < 1) r = 1, g = x;
    else if (h < 2) r = x, g = 1;
    else if (h < 3) g = 1, b = x;
    else if (h < 4) g = x, b = 1;
    else if (h < 5) r = x, b = 1;
    else r = 1, b = x;
    const float l = std::clamp(light, 0.0f, 1.0f);
    return SDL_FColor{r * l, g * l, b * l, 1.0f};
}

// One sector of a ring: an arc band from r0 to r1, a0 to a1 (radians), filled.
void sector(SDL_Renderer* rr, float cx, float cy, float r0, float r1, float a0, float a1, SDL_FColor c)
{
    constexpr int kSteps = 6;
    std::vector<SDL_Vertex> v;
    v.reserve(kSteps * 6);
    for (int i = 0; i < kSteps; ++i)
    {
        const float t0 = a0 + (a1 - a0) * float(i) / kSteps, t1 = a0 + (a1 - a0) * float(i + 1) / kSteps;
        const SDL_FPoint p[4] = {{cx + r0 * std::cos(t0), cy + r0 * std::sin(t0)}, {cx + r1 * std::cos(t0), cy + r1 * std::sin(t0)},
                                 {cx + r1 * std::cos(t1), cy + r1 * std::sin(t1)}, {cx + r0 * std::cos(t1), cy + r0 * std::sin(t1)}};
        for (const int k : {0, 1, 2, 0, 2, 3}) v.push_back({p[k], c, {0, 0}});
    }
    SDL_RenderGeometry(rr, nullptr, v.data(), int(v.size()), nullptr, 0);
}

} // namespace

std::string Rings::note_name(int midi)
{
    static const char* kNames[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    return std::string(kNames[((midi % 12) + 12) % 12]) + std::to_string(midi / 12 - 1);
}

void Rings::step(float dt, const std::vector<RingNote>& now)
{
    now_ = now;
    time_ += dt;
    turn_ += dt * 0.08f;
    const float keep = std::pow(0.5f, dt / 0.35f);
    for (auto& o : light_)
        for (float& x : o) x *= keep;
    for (auto& o : over_)
        for (float& x : o) x *= keep;
    for (const RingNote& n : now)
    {
        const int m = int(std::lround(n.midi));
        if (m < 0 || m >= 12 * kOctaves) continue;
        float& l = light_[size_t(m / 12)][size_t(m % 12)];
        l = std::max(l, n.level);
        for (int k = 1; k <= kHarmonics; ++k)
        {
            // The k-th harmonic falls 12 log2(k) semitones above the note: in the colour of that note.
            const int h = int(std::lround(n.midi + 12.0f * std::log2(float(k))));
            float& o = over_[size_t(k - 1)][size_t(((h % 12) + 12) % 12)];
            o = std::max(o, n.level / float(k));
        }
    }
}

void Rings::zoom(int d) { zoom_ = std::clamp(zoom_ + d, 0, kZooms - 1); }

void Rings::draw(SDL_Renderer* r, float W, float H) const
{
    SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
    const SDL_FRect all{0, 0, W, H};
    SDL_RenderFillRect(r, &all);
    const float cx = W / 2, cy = H / 2 + 10, R = std::min(W, H) * 0.42f, hole = R * 0.12f;
    const float step = 2 * kPi / 12;
    if (zoom_ == 0 || zoom_ == 1)
    {
        // The rings: octaves (zoom 0) or harmonics (zoom 1), each twelve sectors.
        const int rings = zoom_ == 0 ? kOctaves : kHarmonics;
        const float dr = (R - hole) / float(rings);
        for (int ring = 0; ring < rings; ++ring)
            for (int pc = 0; pc < 12; ++pc)
            {
                const float l = zoom_ == 0 ? light_[size_t(ring)][size_t(pc)] : over_[size_t(ring)][size_t(pc)];
                const float a0 = turn_ + float(pc) * step - kPi / 2 - step / 2, a1 = a0 + step * 0.94f;
                sector(r, cx, cy, hole + dr * float(ring) + 1, hole + dr * float(ring + 1) - 1, a0, a1, hue(pc, 0.07f + 0.93f * l));
            }
    }
    else
    {
        // Each note's own wave, round a circle of its own size (lower notes inside), its colour the
        // note's, its swing how loud it is now.
        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_ADD);
        for (const RingNote& n : now_)
        {
            const int m = int(std::lround(n.midi));
            const float base = hole + (R - hole) * std::clamp((n.midi - 12.0f) / 108.0f, 0.0f, 1.0f);
            const int cycles = std::clamp(int(std::lround(std::pow(2.0f, (n.midi - 48.0f) / 12.0f) * 6.0f)), 2, 96);
            const SDL_FColor c = hue(m % 12, 1.0f);
            SDL_SetRenderDrawColorFloat(r, c.r, c.g, c.b, 1.0f);
            constexpr int kPoints = 720;
            SDL_FPoint pts[kPoints + 1];
            for (int i = 0; i <= kPoints; ++i)
            {
                const float a = 2 * kPi * float(i) / kPoints;
                const float rad = base + R * 0.05f * n.level * std::sin(a * float(cycles) + time_ * 6.0f);
                pts[i] = {cx + rad * std::cos(a + turn_), cy + rad * std::sin(a + turn_)};
            }
            SDL_RenderLines(r, pts, kPoints + 1);
        }
        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    }
    // The note names round the outside, and what is shown.
    for (int pc = 0; pc < 12; ++pc)
    {
        const float a = turn_ + float(pc) * step - kPi / 2, x = cx + (R + 18) * std::cos(a), y = cy + (R + 18) * std::sin(a);
        const std::string nm = note_name(pc + 60).substr(0, note_name(pc + 60).size() - 1);
        const SDL_FColor c = hue(pc, 1.0f);
        draw_text(r, x - text_width(nm, 1) / 2, y - 4, nm, 1, SDL_Color{Uint8(c.r * 255), Uint8(c.g * 255), Uint8(c.b * 255), 255});
    }
    const SDL_Color white{255, 255, 255, 255}, grey{150, 150, 150, 255};
    draw_text(r, 20, 16, tr("rings.title"), 2, white);
    draw_text(r, 20, 40, tr(zoom_ == 0 ? "rings.zoom0" : zoom_ == 1 ? "rings.zoom1" : "rings.zoom2"), 1, grey);
    std::string playing;
    for (const RingNote& n : now_) playing += (playing.empty() ? "" : "  ") + note_name(int(std::lround(n.midi)));
    draw_text(r, 20, 56, playing.empty() ? tr("rings.silent") : trf("rings.playing", {playing}), 1, grey);
    draw_text(r, 20, H - 24, tr("rings.footer"), 1, grey);
}

std::string Rings::report() const
{
    std::vector<std::pair<float, int>> lit;
    for (int o = 0; o < kOctaves; ++o)
        for (int pc = 0; pc < 12; ++pc)
            if (light_[size_t(o)][size_t(pc)] >= 0.05f) lit.push_back({light_[size_t(o)][size_t(pc)], o * 12 + pc});
    std::sort(lit.begin(), lit.end(), [](const auto& a, const auto& b) { return a.first > b.first || (a.first == b.first && a.second < b.second); });
    std::string t = "zoom " + std::to_string(zoom_) + ";";
    for (const auto& [l, m] : lit)
    {
        char b[16];
        std::snprintf(b, sizeof b, "%.2f", double(l));
        t += " " + note_name(m) + " " + b;
    }
    return t;
}

} // namespace hallway
