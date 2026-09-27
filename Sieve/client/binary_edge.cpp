// Sieve hallway -- the binary line's edge: the drop where the other wall would be, the short wall
// standing on it, and the rain falling past it through the open air.

#include "hallway.hpp"

namespace hallway::hall {

// The characters the rain can fall as: code points from every block, kept only if the font
// can actually draw them. With Sieve's own small font that is Latin and Greek; with Unifont
// (tools/fetch_unifont.py) it is most of what Unicode has.
const std::vector<char32_t>& Hallway::rain_glyphs()
{
    if (!rain_pool_.empty() && rain_font_ == font_name()) return rain_pool_;
    rain_font_ = font_name();
    rain_pool_.clear();
    for (const sieve::Block& b : sieve::blocks())
    {
        // A sample from each block, so no one block can flood the rain: the big CJK and
        // emoji blocks would otherwise be almost all of it.
        const uint32_t span = b.range.count();
        const uint32_t step = std::max<uint32_t>(1, span / 48);
        for (uint32_t c = b.range.first; c <= b.range.last; c += step)
            if (font_has(char32_t(c))) rain_pool_.push_back(char32_t(c));
    }
    // Nothing drawable at all (a font with no glyphs): fall back to plain hex digits, which
    // is what the edge is underneath anyway.
    if (rain_pool_.empty())
        for (char32_t c = U'0'; c <= U'9'; ++c) rain_pool_.push_back(c);
    return rain_pool_;
}

uint32_t Hallway::hash3(uint32_t a, uint32_t b, uint32_t c)
{
    uint32_t h = a * 0x9E3779B1u ^ b * 0x85EBCA77u ^ c * 0xC2B2AE3Du;
    h ^= h >> 15;
    h *= 0x2545F491u;
    h ^= h >> 13;
    return h;
}

// The heights that can be seen from the camera at a distance x out beyond the edge: the line
// of sight has to pass through the opening, above the floor and below the ceiling, and where
// it crosses the edge depends only on how far out the point is, so the band is exact. The
// short wall is drawn over the rain afterwards, so what it hides needs no test here.
// Returns false when nothing out there can be seen at all.
bool Hallway::visible_band(float x, float& lo, float& hi) const
{
    const float edge = drop_sign() * kEdgeRail;
    const Vec3& e = cam_.pos;
    const float s = (edge - e.x) / (x - e.x); // how far along the sight line the edge is
    if (s <= 0.0f || s >= 1.0f) return false;
    lo = e.y - e.y / s;                       // seen just over the floor's edge
    hi = e.y + (kHeight - e.y) / s;           // seen just under the ceiling's edge
    return true;
}

void Hallway::draw_rain(int back, int ahead)
{
    const std::vector<char32_t>& pool = rain_glyphs();
    const SDL_Color ink = kEdgeInk;
    const float sx = drop_sign();
    const double now = double(SDL_GetTicksNS()) / 1e9;
    const int per_tile = int(kTile / kRainSpacing);
    for (int t = -back; t <= ahead; ++t)
    {
        // The tile's low 32 bits and its sign are plenty to seed a streak: the pattern would
        // only come round again four billion tiles away.
        const TileIndex tile = tile_ + int64_t(t);
        const uint32_t tlo = tile.magnitude.low_bits(32), thi = tile.negative ? 1u : 0u;
        for (int k = 0; k < per_tile; ++k)
        {
            const uint32_t seed = hash3(tlo, thi * 977u + uint32_t(k), 0x5EED);
            // Where it stands: out from the edge by up to kRainReach, more of them near than
            // far (the square of an even spread), and a little off its place along the
            // corridor so the streaks do not line up.
            const float u = float(seed & 0xFFFF) / 65535.0f;
            const float dist = 0.2f + kRainReach * u * u;
            const float x = sx * (kEdgeRail + dist);
            const float z = float(t) * kTile + (float(k) + float((seed >> 16) & 255) / 255.0f) * kRainSpacing;
            // How it falls. A streak covers the whole height that can be seen at its distance
            // through the opening, which grows with the distance, and falls through it in turn.
            const float speed = kRainSpeed * (0.5f + float((seed >> 3) & 255) / 255.0f);
            const int len = 4 + int((seed >> 24) & 7);
            const float top = kHeight + dist * 1.5f, bottom = -dist * 1.5f;
            const float span = top - bottom + float(len) * kRainGlyph;
            const double fallen = now * speed + double(seed % 1000);
            const uint32_t cycle = uint32_t(fallen / span);
            if (hash3(seed, cycle, 0x0FA11) % kRainSparse != 0) continue;
            const float head = top - float(std::fmod(fallen, double(span)));
            // Faint with distance, as rain in air is.
            const float fade = 1.0f - 0.8f * dist / kRainReach;
            // Only the part of the streak inside the band that can be seen at its distance.
            float lo = 0, hi = 0;
            if (!visible_band(x, lo, hi)) continue;
            const float sy0 = std::max(head, lo), sy1 = std::min(head + float(len) * kRainGlyph, hi);
            if (sy0 >= sy1) continue;
            const Vec3 hp{x, sy0, z}, tp{x, sy1, z};
            const Vec3 hc = cam_.to_camera(Vec3{x, head, z});
            if (hc.z < cam_.near_z) continue;
            const float px = kRainGlyph / hc.z * cam_.focal(); // one character, in pixels
            if (px < 4.0f)
            {
                // Too small to read: the streak as a line, clipped to what can be seen.
                if (auto seg = cam_.project_segment(hp, tp))
                {
                    SDL_SetRenderDrawColor(r_, Uint8(float(ink.r) * fade), Uint8(float(ink.g) * fade),
                                           Uint8(float(ink.b) * fade), Uint8(200.0f * fade));
                    SDL_RenderLine(r_, seg->first.first.x, seg->first.first.y, seg->first.second.x, seg->first.second.y);
                }
                continue;
            }
            for (int g = 0; g < len; ++g)
            {
                // A character is drawn whole or not at all: whole means all of it is in the band.
                const float gy = head + float(g) * kRainGlyph;
                if (gy - 0.5f * kRainGlyph < lo || gy + 0.5f * kRainGlyph > hi) continue;
                const Vec3 p{x, gy, z};
                const Vec3 c = cam_.to_camera(p);
                if (c.z < cam_.near_z) continue;
                const Point2 at = cam_.project_camera(c);
                // The head is bright and the tail fades, as rain does.
                const float tail = 1.0f - float(g) / float(len);
                const float b = (0.35f + 0.65f * tail) * fade;
                const SDL_Color ch = g == 0 ? SDL_Color{255, 255, 255, Uint8(255.0f * fade)}
                                            : SDL_Color{Uint8(float(ink.r) * b), Uint8(float(ink.g) * b),
                                                        Uint8(float(ink.b) * b), Uint8((90.0f + 165.0f * tail) * fade)};
                const uint32_t row = uint32_t(int64_t(std::floor((head + float(g) * kRainGlyph) / kRainGlyph)));
                const char32_t cp = pool[hash3(seed, row, uint32_t(now * 2.0)) % pool.size()];
                const float scale = c.z > 0 ? (kRainGlyph / c.z * cam_.focal()) / 8.0f : 1.0f;
                draw_text(r_, at.x - 4.0f * scale, at.y - 4.0f * scale, utf8_encode(std::u32string(1, cp)), scale, ch);
            }
        }
    }
}

// The edge: the rain beyond it, then the short wall laid over the rain's foot.
void Hallway::draw_binary_edge(const bool* visible, int back, int ahead, bool real)
{
    draw_rain(back, ahead);
    const float sx = drop_sign(), x = sx * kEdgeRail;
    // The rain is drawn over the models rather than depth-tested against them, so the short
    // wall's two faces are laid back over the top, near tile last. Above the wall the opening
    // is clear and the rain shows; below it, the rain is behind the wall, as it should be.
    auto rail_colour = [](float lit) {
        auto c = [lit](float v) { return Uint8(std::min(255.0f, v * 255.0f * lit)); };
        return SDL_Color{c(kRailColour.r), c(kRailColour.g), c(kRailColour.b), 255};
    };
    const SDL_Color face = real ? rail_colour(0.38f) : theme().bg;
    const float inner = sx * (kEdgeRail - 0.12f), outer = x;
    for (int t = ahead; t >= -back; --t)
    {
        if (!visible[t + back]) continue;
        const float z0 = float(t) * kTile, z1 = z0 + kTile;
        fill({{inner, 0, z0}, {inner, 0, z1}, {inner, kEdgeRailTop, z1}, {inner, kEdgeRailTop, z0}}, face);
        fill({{inner, kEdgeRailTop, z0}, {inner, kEdgeRailTop, z1}, {outer, kEdgeRailTop, z1}, {outer, kEdgeRailTop, z0}},
             real ? rail_colour(0.62f) : face);
    }
}

} // namespace hallway::hall
