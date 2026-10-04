// Sieve hallway -- Door Portals: the procedural noise that fills a doorway, in the colours of the
// line it leads to. Drawn straight into a texture a pixel at a time, so it is kept cheap: a
// smoothed value noise from a table, a few octaves, and the depth test against the scene. And the
// portal titles: the sign over every doorway that names the line through it.

#include "hallway.hpp"
#include "gpu_memory.hpp"

namespace hallway::hall {

// Smoothstep as 256 steps, so the fade costs a lookup rather than three multiplies.
const std::array<uint32_t, 256>& Hallway::smooth_table()
{
    static const std::array<uint32_t, 256> t = [] {
        std::array<uint32_t, 256> a{};
        for (int i = 0; i < 256; ++i)
        {
            const float f = float(i) / 255.0f;
            a[size_t(i)] = uint32_t(255.0f * f * f * (3.0f - 2.0f * f) + 0.5f);
        }
        return a;
    }();
    return t;
}

// One lattice node of the portal's cloud, 0..255. `seed` separates the octaves.
float Hallway::noise_at(int x, int y, uint32_t seed)
{
    uint32_t h = uint32_t(x) * 0x9E3779B1u ^ uint32_t(y) * 0x85EBCA77u ^ seed * 0x27D4EB2Fu;
    h ^= h >> 15;
    h *= 0x2545F491u;
    h ^= h >> 13;
    return float(h >> 24);
}

// The line's own colour for its portal: whichever of its two is the brighter, so every line
// reads (white for PAGES, cyan for IMAGE, amber for AUDIO, yellow for VIDEO, grey for BOOKS).
SDL_Color Hallway::portal_colour(const Theme& th)
{
    auto luma = [](SDL_Color c) { return 2 * int(c.r) + 5 * int(c.g) + int(c.b); };
    return luma(th.edge) >= luma(th.bg) ? th.edge : th.bg;
}

// `depth` is the models' depth buffer when Real Graphics drew this frame, else null: the
// portal is drawn over that image, so without it a door far down the corridor would show
// through the bookcases between you and it.
void Hallway::draw_portal(const std::vector<Vec3>& quad, const Theme& dest, const float* depth, int dw, int dh)
{
    const std::vector<Point2> pts = cam_.project_polygon(quad);
    if (pts.size() < 3) return;
    int ow = 0, oh = 0;
    SDL_GetCurrentRenderOutputSize(r_, &ow, &oh);
    float lox = pts[0].x, hix = pts[0].x, loy = pts[0].y, hiy = pts[0].y;
    for (const Point2& p : pts)
    {
        lox = std::min(lox, p.x); hix = std::max(hix, p.x);
        loy = std::min(loy, p.y); hiy = std::max(hiy, p.y);
    }
    // The box is snapped down to the grain, so the noise cells sit on one fixed screen lattice
    // and the grain does not crawl as the door moves across the screen.
    const int x1 = std::clamp(int(std::ceil(hix)) + 1, 0, ow), y1 = std::clamp(int(std::ceil(hiy)) + 1, 0, oh);
    const int x0 = std::clamp(int(std::floor(lox)), 0, ow) / kGrain * kGrain;
    const int y0 = std::clamp(int(std::floor(loy)), 0, oh) / kGrain * kGrain;
    const int bw = x1 - x0, bh = y1 - y0;
    if (bw <= 0 || bh <= 0) return;
    const int nw = (bw + kGrain - 1) / kGrain, nh = (bh + kGrain - 1) / kGrain;

    // Each edge as the line nx*x + ny*y + c, positive inside (the winding decides the sign)
    // and scaled to pixels, so the smallest of them is the distance to the door frame.
    float area2 = 0;
    for (size_t i = 0, n = pts.size(); i < n; ++i)
    {
        const Point2 a = pts[i], b = pts[(i + 1) % n];
        area2 += a.x * b.y - b.x * a.y;
    }
    const float wind = area2 < 0 ? -1.0f : 1.0f;
    struct Edge { float nx, ny, c; };
    std::vector<Edge> edges;
    edges.reserve(pts.size());
    for (size_t i = 0, n = pts.size(); i < n; ++i)
    {
        const Point2 a = pts[i], b = pts[(i + 1) % n];
        float ex = b.x - a.x, ey = b.y - a.y;
        const float len = std::sqrt(ex * ex + ey * ey);
        if (len < 1e-4f) continue; // a degenerate edge says nothing about the inside
        ex /= len; ey /= len;
        // Distance from the line through a and b, positive on the inward side.
        edges.push_back({-wind * ey, wind * ex, wind * (ey * a.x - ex * a.y)});
    }
    if (edges.size() < 3) return;
    // The fade reaches this far in from the frame, in pixels: a share of the door on screen,
    // so a distant door is not all fade and a door in your face is not all noise.
    const float fall = std::clamp(0.16f * float(std::min(bw, bh)), 1.5f, 96.0f);
    const float inv_fall = 1.0f / fall;
    const std::array<uint32_t, 256>& kSmooth = smooth_table();

    // 1 / camera depth over the screen for the door's own plane: planar there, so three
    // numbers describe it. The door is a flat quad, so any three of its corners give it.
    float zA = 0, zB = 0, zC = 0;
    bool test_depth = depth && dw == ow && dh == oh;
    if (test_depth)
    {
        const Vec3 c0 = cam_.to_camera(quad[0]), c1 = cam_.to_camera(quad[1]), c2 = cam_.to_camera(quad[2]);
        const Vec3 n = hallway::cross(c1 - c0, c2 - c0);
        const float d = dot(n, c0);
        const float f = cam_.focal();
        const Point2 mid = cam_.centre();
        if (std::fabs(d) < 1e-6f || f <= 0) test_depth = false;
        else
        {
            zA = n.x / (f * d);
            zB = -n.y / (f * d);
            zC = n.z / d - zA * mid.x - zB * mid.y;
        }
    }
    // The door sits in the wall and the model's own door face is 6 cm behind it, so a small
    // margin keeps the portal from being rejected by the surface it belongs to.
    constexpr float kNearer = 1.002f;

    std::vector<uint32_t>& px = portal_.px;
    px.assign(size_t(nw) * size_t(nh), 0u);
    const uint32_t frame = portal_frame_;
    // The field is a smooth cloud with a little grain on it, not raw static: the cloud is
    // value noise on a lattice one node every kCloud cells, drifting downwards a quarter of a
    // cell a frame, and the grain is the old per-cell hash at a fifth of the strength. Pure
    // per-cell noise at full contrast flickers hard enough to be painful to look at.
    Octave coarse, fine;
    coarse.init(18, 1);
    fine.init(6, 2);
    const uint32_t drift = frame * 64, churn = frame >> 3; // the cloud flows, the grain stirs
    // Top to bottom across the door itself, so the shading does not slide about when the
    // door is clipped by the edge of the screen.
    const float span = std::max(1.0f, hiy - loy);
    // The colour at each of 256 brightnesses, so the inner loop is integer work and one lookup.
    const SDL_Color col = portal_colour(dest);
    uint32_t ramp[256];
    for (int i = 0; i < 256; ++i)
        ramp[i] = 0xFF000000u | (uint32_t(col.r * i / 255) << 16) | (uint32_t(col.g * i / 255) << 8) | uint32_t(col.b * i / 255);
    const int cx0 = x0 / kGrain, cy0 = y0 / kGrain; // this door's first cell on the lattice
    bool any = false; // a door wholly behind a bookcase is not uploaded or drawn at all
    for (int by = 0; by < nh; ++by)
    {
        const int y = y0 + by * kGrain;
        // The span of this scanline inside the polygon: every edge either bounds it on the
        // left or on the right, depending on which way it runs.
        float xl = float(x0), xr = float(x1);
        bool empty = false;
        const float fy = float(y) + 0.5f * kGrain;
        for (const Edge& e : edges)
        {
            const float row = e.ny * fy + e.c;
            if (std::fabs(e.nx) < 1e-6f)
            {
                if (row < 0) { empty = true; break; }
                continue;
            }
            const float cut = -row / e.nx;      // where this edge crosses the scanline
            if (e.nx > 0) xl = std::max(xl, cut); // inside lies to the right of it
            else xr = std::min(xr, cut);          // and to the left of this one
        }
        if (empty) continue;
        // The cells of this row whose centres are inside the door.
        const int sb = std::max(0, int(std::ceil((xl - float(x0)) / kGrain - 0.5f)));
        const int eb = std::min(nw, int(std::floor((xr - float(x0)) / kGrain - 0.5f)) + 1);
        if (sb >= eb) continue;
        uint32_t* row_px = px.data() + size_t(by) * size_t(nw) + size_t(sb);
        // This row's place on each cloud lattice, and the gentle top-to-bottom shading.
        const uint32_t yq = (uint32_t(cy0 + by) << 8) + drift;
        coarse.row(yq);
        fine.row(yq);
        coarse.start(cx0 + sb);
        fine.start(cx0 + sb);
        const float shade = 1.0f - 0.34f * std::clamp((fy - loy) / span, 0.0f, 1.0f);
        // The distance to the frame, stepped along the row: one add per edge per cell.
        float d[8], dd[8];
        const size_t ne = std::min<size_t>(edges.size(), 8);
        const float fx = float(x0 + sb * kGrain) + 0.5f * kGrain;
        for (size_t k = 0; k < ne; ++k)
        {
            d[k] = edges[k].nx * fx + edges[k].ny * fy + edges[k].c;
            dd[k] = edges[k].nx * kGrain;
        }
        // The octaves step with the loop, so a cell skipped for being outside the door or
        // behind a bookcase still moves them on and the cloud stays where it belongs.
        for (int bx = sb; bx < eb; ++bx, ++row_px, coarse.step(), fine.step())
        {
            float near = d[0];
            for (size_t k = 1; k < ne; ++k) near = std::min(near, d[k]);
            for (size_t k = 0; k < ne; ++k) d[k] += dd[k];
            if (near <= 0) continue;
            const int sx = x0 + bx * kGrain, sy = y;
            // Hidden by a model in front of it? (The cell's centre stands for the cell.)
            if (test_depth && depth[size_t(sy) * size_t(dw) + size_t(sx)] > (zA * float(sx) + zB * float(sy) + zC) * kNearer)
                continue;
            any = true;
            // How far in from the frame, 0..255, then smoothed so the field has no hard rim.
            const float t = near * inv_fall;
            const uint32_t f = kSmooth[t >= 1.0f ? 255 : uint32_t(t * 255.0f)];
            const int cxi = cx0 + bx;
            // Two octaves of cloud: broad shapes with finer ones inside them.
            const float cloud = coarse.value() * 0.62f + fine.value() * 0.38f;
            // The grain on top of it, at a fifth of the strength.
            uint32_t hsh = uint32_t(cxi) * 0x9E3779B1u ^ uint32_t(cy0 + by) * 0x85EBCA77u ^ churn * 0xC2B2AE3Du;
            hsh ^= hsh >> 15;
            hsh *= 0x2545F491u;
            hsh ^= hsh >> 13;
            // Lifted off black and held short of white, so the field reads as lit rather than
            // as sparks; the fade to the frame still takes it all the way down.
            const float v = (34.0f + (cloud * 0.79f + float(hsh >> 24) * 0.21f) * 0.72f) * shade;
            *row_px = ramp[(f * uint32_t(std::clamp(v, 0.0f, 255.0f))) >> 8];
        }
    }
    if (!any) return;
    if (!portal_.tex || portal_.w < nw || portal_.h < nh)
    {
        if (portal_.tex) gpu::destroy(portal_.tex);
        portal_.w = std::max(portal_.w, nw);
        portal_.h = std::max(portal_.h, nh);
        portal_.tex = gpu::create(r_, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, portal_.w, portal_.h);
        if (portal_.tex)
        {
            SDL_SetTextureBlendMode(portal_.tex, SDL_BLENDMODE_BLEND);
            SDL_SetTextureScaleMode(portal_.tex, SDL_SCALEMODE_NEAREST);
        }
    }
    if (!portal_.tex) return;
    const SDL_Rect area{0, 0, nw, nh};
    SDL_UpdateTexture(portal_.tex, &area, px.data(), nw * 4);
    // Nearest-neighbour back up to the grain: one noise cell becomes one kGrain square.
    const SDL_FRect src{0, 0, float(nw), float(nh)};
    const SDL_FRect dst{float(x0), float(y0), float(nw * kGrain), float(nh * kGrain)};
    SDL_RenderTexture(r_, portal_.tex, &src, &dst);
}

// ---- portal titles
//
// Every doorway has a sign over it naming the line it leads to, in that line's colours: the
// doors used to say nothing about what was through them, and the only way to know was the top
// bar or walking through. The sign is a texture per line, lettered once in the chosen language
// and kept, and drawn on the wall between the top of the door and the ceiling, facing the
// corridor, by the same perspective-correct drawing as the items' displays.

SDL_Texture* Hallway::sign_texture(int line)
{
    SignCache& c = signs_[size_t(line)];
    if (c.tex) return c.tex;
    const Theme& th = theme_of(line);
    const std::u32string name = utf8_decode(tr(th.key));
    constexpr int w = kSignPxW, h = kSignPxH;
    std::vector<uint32_t> px(size_t(w) * h);
    const uint32_t ground = 0xFF000000u | uint32_t(th.bg.r / 2) << 16 | uint32_t(th.bg.g / 2) << 8 | uint32_t(th.bg.b / 2);
    const uint32_t ink = 0xFF000000u | uint32_t(th.edge.r) << 16 | uint32_t(th.edge.g) << 8 | uint32_t(th.edge.b);
    std::fill(px.begin(), px.end(), ground);
    // A frame in the line's edge colour, then the name as large as fits inside it, centred.
    const int b = 6;
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x)
            if (x < b || y < b || x >= w - b || y >= h - b) px[size_t(y) * w + size_t(x)] = ink;
    const float room_w = float(w - 8 * b), room_h = float(h - 6 * b);
    const float cell = std::floor(std::min(room_h, room_w / float(std::max<size_t>(1, name.size()))));
    const float x0 = (float(w) - cell * float(name.size())) * 0.5f, y0 = (float(h) - cell) * 0.5f;
    for (size_t i = 0; i < name.size(); ++i)
        if (name[i] != U' ') paint_glyph(px.data(), w, h, x0 + float(i) * cell, y0, cell, cell, name[i], ink);
    c.tex = gpu::create(r_, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, w, h);
    if (!c.tex) return nullptr;
    SDL_UpdateTexture(c.tex, nullptr, px.data(), w * 4);
    SDL_SetTextureBlendMode(c.tex, SDL_BLENDMODE_BLEND);
    SDL_SetTextureScaleMode(c.tex, SDL_SCALEMODE_LINEAR);
    return c.tex;
}

// The sign over the doorway in the wall at sx (-1 left, +1 right) of the tile starting at z0.
// The left wall's door leads to the next line and the right wall's to the previous, and the
// binary line's one door back to where you came from (door_to(), cross()).
void Hallway::draw_door_sign(float sx, float z0, std::vector<SDL_Vertex>& verts)
{
    SDL_Texture* tex = sign_texture(door_to(sx));
    if (!tex) return;
    // Just off the wall, so it is never lost in it.
    const float x = sx * (kHalfWidth - 0.01f);
    const float za = z0 + kDoorStart, zb = z0 + kDoorEnd;
    // The bookcases hide it. Nothing is depth-tested here, so the part of the sign behind a
    // bookcase is cut off by hand: a bookcase stands out from the wall to kCaseFront, higher than
    // the sign, along z 0..kShelfEnd of each tile, so the one before the door (seen from behind
    // it) or the one after it (seen from beyond it) covers every point of the sign whose line of
    // sight crosses the bookcase's front inside the bookcase's span. Along the wall that is a
    // single cut, where the line of sight grazes the bookcase's end.
    const Vec3 eye = cam_.pos;
    float lo = za, hi = zb;
    const float k = (sx * kCaseFront - eye.x) / (x - eye.x); // how far along the sight line the bookcase front is
    if (k > 0 && k < 1)
    {
        if (eye.z < za) lo = std::max(lo, eye.z + (z0 + kShelfEnd - eye.z) / k);   // behind this tile's bookcase
        if (eye.z > zb) hi = std::min(hi, eye.z + (z0 + kTile - eye.z) / k);       // behind the next tile's
    }
    if (hi - lo < 0.01f) return; // hidden
    // Corners in picture_face's order, the texture cut with the sign: across it, u runs from za
    // to zb on the left wall and from zb to za on the right, as a reader facing that wall sees it.
    const float span = zb - za;
    const float zl = sx < 0 ? lo : hi, zr = sx < 0 ? hi : lo;
    const float ul = sx < 0 ? (lo - za) / span : (zb - hi) / span, ur = sx < 0 ? (hi - za) / span : (zb - lo) / span;
    const Vec3 q[4] = {{x, kSignTop, zl}, {x, kSignTop, zr}, {x, kSignBottom, zr}, {x, kSignBottom, zl}};
    draw_face_image(tex, q, verts, ul, ur);
}

void Hallway::release_signs()
{
    for (SignCache& c : signs_)
        if (c.tex)
        {
            gpu::destroy(c.tex);
            c.tex = nullptr;
        }
}

} // namespace hallway::hall
