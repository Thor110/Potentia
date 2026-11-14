// Sieve hallway — your personal dimension as a world of its own (see personal_world.hpp).
#include "personal_world.hpp"

#include "font.hpp"
#include "gpu_memory.hpp"
#include "keys.hpp"
#include "mesh.hpp"
#include "pause_rows.hpp"
#include "music.hpp"
#include "strings.hpp"

#include "cli/sieve_cache.hpp"
#include "cli/dictionaries.hpp"
#include "cli/image_io.hpp"
#include "sieve/paths.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace hallway {

namespace {

constexpr float kPi = 3.14159265358979f;
constexpr float kPlaceAhead = 1.5f; // (metres before you an object is placed: a shelf item is 0.4 m tall)
constexpr float kEye = 1.7f, kFov = 70.0f * kPi / 180.0f, kHalf = 320.0f, kCell = 10.0f, kNear = 0.1f;

} // namespace

// The keys of your personal dimension (IDEAS 19.199): each named by what it does, under its section, so it
// is listed on Settings > Controls > Keys and can be changed there.
namespace pk {
const std::string& declare(const char* section, const char* id, const char* defaults)
{
    return Keys::get().declare({section, id, std::string("keys.") + section + "." + id, defaults});
}
const std::string& kForward = declare("personal.moving", "forward", "W,Up");
const std::string& kBack = declare("personal.moving", "back", "S,Down");
const std::string& kLeft = declare("personal.moving", "left", "A");
const std::string& kRight = declare("personal.moving", "right", "D");
const std::string& kTurnLeft = declare("personal.moving", "turn_left", "Left");
const std::string& kTurnRight = declare("personal.moving", "turn_right", "Right");
const std::string& kRun = declare("personal.moving", "run", "Left Shift,Right Shift");
const std::string& kLookUp = declare("personal.moving", "look_up", "PageUp");
const std::string& kLookDown = declare("personal.moving", "look_down", "PageDown");
const std::string& kCatalogue = declare("personal.catalogue", "open", "Tab");
const std::string& kChooseUp = declare("personal.catalogue", "up", "Up");
const std::string& kChooseDown = declare("personal.catalogue", "down", "Down");
const std::string& kPlace = declare("personal.catalogue", "place", "Return,Keypad Enter");
const std::string& kTakeAway = declare("personal.catalogue", "take_away", "Backspace");
const std::string& kPickUp = declare("personal.holding", "pick_up", "E");
const std::string& kOpen = declare("personal.holding", "open", "O");
const std::string& kTurn = declare("personal.holding", "turn", "R");
const std::string& kTurnBack = declare("personal.holding", "turn_back", "Shift+R");
const std::string& kTip = declare("personal.holding", "tip", "T");
const std::string& kTipBack = declare("personal.holding", "tip_back", "Shift+T");
const std::string& kRoll = declare("personal.holding", "roll", "G");
const std::string& kRollBack = declare("personal.holding", "roll_back", "Shift+G");
const std::string& kPlaceHere = declare("personal.holding", "place_here", "Return,Keypad Enter");
const std::string& kDrop = declare("personal.holding", "drop", "Q");
const std::string& kPutBack = declare("personal.holding", "put_back", "Escape");
const std::string& kExact = declare("personal.holding", "exact", "Tab");
const std::string& kZoomIn = declare("personal.minimap", "zoom_in", "=,Shift+=,Plus,Keypad +");
const std::string& kZoomOut = declare("personal.minimap", "zoom_out", "-,Keypad -");
} // namespace pk

namespace {

uint64_t splitmix(uint64_t& s)
{
    uint64_t z = (s += 0x9E3779B97F4A7C15ull);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    return z ^ (z >> 31);
}
float unit(uint64_t& s) { return float(splitmix(s) >> 40) / float(1ull << 24); } // [0, 1)

SDL_FColor mix(SDL_FColor a, SDL_FColor b, float t)
{
    t = std::clamp(t, 0.0f, 1.0f);
    return {a.r + (b.r - a.r) * t, a.g + (b.g - a.g) * t, a.b + (b.b - a.b) * t, a.a + (b.a - a.a) * t};
}

const SDL_FColor kSkyTop{0.13f, 0.36f, 0.86f, 1}, kSkyLow{0.72f, 0.85f, 0.98f, 1}, kGrass{0.27f, 0.60f, 0.16f, 1};

} // namespace

PersonalWorld::~PersonalWorld() { gpu::destroy(layer_.tex); }

namespace {

// One face (a convex polygon of up to six points on the screen, each with its nearness, 1/depth)
// filled into the layer where it is nearer than what is there. Nearness, unlike depth, goes evenly
// across the screen, so it is interpolated as the screen's own coordinates are.
template <class Layer>
void fill_face(Layer& L, const SDL_FPoint* p, const float* nz, int count, uint32_t argb)
{
    for (int k = 1; k + 1 < count; ++k)
    {
        const SDL_FPoint a = p[0], b = p[k], c = p[k + 1];
        const float area = (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
        if (std::fabs(area) < 1e-6f) continue;
        const int x0 = std::max(0, int(std::floor(std::min({a.x, b.x, c.x})))), x1 = std::min(L.w - 1, int(std::ceil(std::max({a.x, b.x, c.x}))));
        const int y0 = std::max(0, int(std::floor(std::min({a.y, b.y, c.y})))), y1 = std::min(L.h - 1, int(std::ceil(std::max({a.y, b.y, c.y}))));
        if (x0 > x1 || y0 > y1) continue;
        L.x0 = std::min(L.x0, x0), L.y0 = std::min(L.y0, y0), L.x1 = std::max(L.x1, x1), L.y1 = std::max(L.y1, y1);
        const float inv = 1.0f / area;
        for (int y = y0; y <= y1; ++y)
        {
            const float py = float(y) + 0.5f;
            for (int x = x0; x <= x1; ++x)
            {
                const float px = float(x) + 0.5f;
                const float l0 = ((b.x - px) * (c.y - py) - (b.y - py) * (c.x - px)) * inv;
                const float l1 = ((c.x - px) * (a.y - py) - (c.y - py) * (a.x - px)) * inv;
                const float l2 = 1.0f - l0 - l1;
                if (l0 < 0 || l1 < 0 || l2 < 0) continue;
                const float n = l0 * nz[0] + l1 * nz[k] + l2 * nz[k + 1];
                const size_t i = size_t(y) * size_t(L.w) + size_t(x);
                if (n <= L.near[i]) continue;
                L.near[i] = n;
                L.px[i] = argb;
            }
        }
    }
}

// An edge, drawn where it is not behind what is drawn (a little leeway, so a face's own edge shows).
template <class Layer>
void edge_line(Layer& L, SDL_FPoint a, SDL_FPoint b, float na, float nb, uint32_t argb)
{
    const int steps = std::max(1, int(std::ceil(std::max(std::fabs(b.x - a.x), std::fabs(b.y - a.y)))));
    for (int s = 0; s <= steps; ++s)
    {
        const float u = float(s) / float(steps);
        const int x = int(a.x + (b.x - a.x) * u), y = int(a.y + (b.y - a.y) * u);
        if (x < 0 || y < 0 || x >= L.w || y >= L.h) continue;
        const float n = na + (nb - na) * u;
        const size_t i = size_t(y) * size_t(L.w) + size_t(x);
        if (n < L.near[i] * 0.995f) continue;
        L.px[i] = argb;
        L.x0 = std::min(L.x0, x), L.y0 = std::min(L.y0, y), L.x1 = std::max(L.x1, x), L.y1 = std::max(L.y1, y);
    }
}

// A face with a picture on it: its colour at each pixel read from the picture, the picture's place
// (u, v from 0 to 1) carried across as u/depth and v/depth, so it is not skewed by perspective.
template <class Layer>
void fill_picture(Layer& L, const SDL_FPoint* p, const float* nz, const float (*uv)[2], const sieve::RgbaImage& img)
{
    const SDL_FPoint a = p[0], b = p[1], c = p[2];
    const float area = (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
    if (std::fabs(area) < 1e-6f || img.width == 0 || img.height == 0) return;
    const int x0 = std::max(0, int(std::floor(std::min({a.x, b.x, c.x})))), x1 = std::min(L.w - 1, int(std::ceil(std::max({a.x, b.x, c.x}))));
    const int y0 = std::max(0, int(std::floor(std::min({a.y, b.y, c.y})))), y1 = std::min(L.h - 1, int(std::ceil(std::max({a.y, b.y, c.y}))));
    if (x0 > x1 || y0 > y1) return;
    L.x0 = std::min(L.x0, x0), L.y0 = std::min(L.y0, y0), L.x1 = std::max(L.x1, x1), L.y1 = std::max(L.y1, y1);
    const float inv = 1.0f / area;
    for (int y = y0; y <= y1; ++y)
        for (int x = x0; x <= x1; ++x)
        {
            const float px = float(x) + 0.5f, py = float(y) + 0.5f;
            const float l0 = ((b.x - px) * (c.y - py) - (b.y - py) * (c.x - px)) * inv;
            const float l1 = ((c.x - px) * (a.y - py) - (c.y - py) * (a.x - px)) * inv;
            const float l2 = 1.0f - l0 - l1;
            if (l0 < 0 || l1 < 0 || l2 < 0) continue;
            const float n = l0 * nz[0] + l1 * nz[1] + l2 * nz[2];
            const size_t i = size_t(y) * size_t(L.w) + size_t(x);
            if (n <= L.near[i]) continue;
            const float u = (l0 * uv[0][0] * nz[0] + l1 * uv[1][0] * nz[1] + l2 * uv[2][0] * nz[2]) / n;
            const float v = (l0 * uv[0][1] * nz[0] + l1 * uv[1][1] * nz[1] + l2 * uv[2][1] * nz[2]) / n;
            const uint32_t tx = std::min(img.width - 1, uint32_t(std::max(0.0f, u) * float(img.width)));
            const uint32_t ty = std::min(img.height - 1, uint32_t(std::max(0.0f, v) * float(img.height)));
            const uint8_t* q = img.rgba.data() + (size_t(ty) * img.width + tx) * 4;
            L.near[i] = n;
            L.px[i] = 0xFF000000u | uint32_t(q[0]) << 16 | uint32_t(q[1]) << 8 | q[2];
        }
}

uint32_t argb_of(SDL_FColor c)
{
    auto u = [](float v) { return uint32_t(std::clamp(v, 0.0f, 1.0f) * 255.0f + 0.5f); };
    return 0xFF000000u | u(c.r) << 16 | u(c.g) << 8 | u(c.b);
}

} // namespace

PersonalWorld::PersonalWorld(SDL_Window* window, SDL_Renderer* renderer, std::vector<sieve::Character> characters, std::filesystem::path world_file, uint64_t seed)
    : window_(window), r_(renderer), characters_(std::move(characters)), file_(std::move(world_file)), seed_(seed)
{
    rows_ = catalogue();
    if (!file_.empty())
        for (const std::string& sha : dimension_favourites(favourites_list_of(file_)))
            if (auto f = read_favourite(file_.parent_path().parent_path(), sha))
            {
                favourites_.push_back(*f);
                rows_.push_back("fav:" + sha);
            }
    load(); // (what was placed before, and the world's seed)
    // The hill: one broad swell near the middle, a few smaller ones about it, all from the seed (the
    // world file's, where it has one).
    uint64_t s = seed_ ^ 0x5049455253ull; // ("PIERS": the world's own stream)
    swells_.push_back({(unit(s) - 0.5f) * 40.0f, 40.0f + (unit(s) - 0.5f) * 40.0f, 150.0f + unit(s) * 40.0f, 26.0f + unit(s) * 8.0f});
    for (int i = 0; i < 6; ++i)
        swells_.push_back({(unit(s) - 0.5f) * 500.0f, (unit(s) - 0.5f) * 500.0f, 40.0f + unit(s) * 60.0f, 3.0f + unit(s) * 10.0f});
    // The clouds: soft shapes of a few discs each, high up.
    for (int i = 0; i < 10; ++i)
    {
        Cloud c{(unit(s) - 0.5f) * 1200.0f, 150.0f + unit(s) * 90.0f, (unit(s) - 0.2f) * 1100.0f, 25.0f + unit(s) * 35.0f, {}};
        const int puffs = 4 + int(unit(s) * 4);
        for (int k = 0; k < puffs; ++k) c.puffs.push_back({(unit(s) - 0.5f) * 2.2f * c.size, (unit(s) - 0.3f) * 0.5f * c.size, (0.45f + unit(s) * 0.4f) * c.size});
        clouds_.push_back(std::move(c));
    }
    // (a v1 world's objects stand on the ground, which is known only now)
    for (Placed& p : placed_)
        if (std::isnan(p.y)) p.y = ground(p.x, p.z);
}

// The ground as drawn: the height of the cell's triangle there (each 10 m cell two flat triangles
// between its corners' heights), so what stands on it stands on the grass you see.
float PersonalWorld::ground(float x, float z) const
{
    const float fi = std::floor((x + kHalf) / kCell), fj = std::floor((z + kHalf) / kCell);
    const float x0 = -kHalf + fi * kCell, z0 = -kHalf + fj * kCell, u = (x - x0) / kCell, v = (z - z0) / kCell;
    const float h00 = height(x0, z0), h10 = height(x0 + kCell, z0), h01 = height(x0, z0 + kCell), h11 = height(x0 + kCell, z0 + kCell);
    // (split from (x0, z0) to (x1, z1): below the diagonal, the triangle 00-10-11; above it 00-11-01)
    return u >= v ? h00 + (h10 - h00) * u + (h11 - h10) * v : h00 + (h11 - h01) * u + (h01 - h00) * v;
}

float PersonalWorld::height(float x, float z) const
{
    float h = 0;
    for (const Swell& w : swells_)
    {
        const float dx = x - w.x, dz = z - w.z;
        h += w.rise * std::exp(-(dx * dx + dz * dz) / (w.radius * w.radius));
    }
    return h;
}

// A point of the world on the screen: false where it is behind you (nearer than kNear).
bool PersonalWorld::project(float x, float y, float z, float& sx, float& sy, float& depth) const
{
    const float dx = x - px_, dy = y - (height(px_, pz_) + kEye), dz = z - pz_;
    const float cy = std::cos(yaw_), sy0 = std::sin(yaw_), cp = std::cos(pitch_), sp = std::sin(pitch_);
    const float x1 = dx * cy - dz * sy0, z1 = dx * sy0 + dz * cy;
    const float y2 = dy * cp - z1 * sp, z2 = dy * sp + z1 * cp;
    depth = z2;
    if (z2 < kNear) return false;
    const float f = (H_ / 2) / std::tan(kFov / 2);
    sx = W_ / 2 + x1 / z2 * f;
    sy = H_ / 2 - y2 / z2 * f;
    return true;
}

void PersonalWorld::step(float dt, const bool* keys)
{
    const Keys& K = Keys::get();
    const bool menu = cat_panel_.open || (place_panel_.open && held_ >= 0); // (a panel's own keys are not walked with)
    const float speed = K.held(pk::kRun, keys) ? 12.0f : 4.0f;
    float fx = 0, fz = 0;
    if (!menu)
    {
        if (K.held(pk::kForward, keys)) fz += 1;
        if (K.held(pk::kBack, keys)) fz -= 1;
        if (K.held(pk::kRight, keys)) fx += 1;
        if (K.held(pk::kLeft, keys)) fx -= 1;
        if (K.held(pk::kTurnLeft, keys)) yaw_ -= 1.6f * dt;
        if (K.held(pk::kTurnRight, keys)) yaw_ += 1.6f * dt;
    }
    float mx = 0, mz = 0; // (how you move, metres a second, for knocks)
    if (fx != 0 || fz != 0)
    {
        const float n = std::sqrt(fx * fx + fz * fz);
        fx /= n, fz /= n;
        mx = (fz * std::sin(yaw_) + fx * std::cos(yaw_)) * speed, mz = (fz * std::cos(yaw_) - fx * std::sin(yaw_)) * speed;
        px_ += mx * dt, pz_ += mz * dt;
        px_ = std::clamp(px_, -kHalf + 5, kHalf - 5);
        pz_ = std::clamp(pz_, -kHalf + 5, kHalf - 5);
    }
    wind_ += dt * 2.0f;
    if (held_ >= 0 && !place_panel_.open)
    {
        // In your hand: two metres along the crosshair, never under the grass (with Place open, where
        // its numbers put it instead).
        Placed& p = placed_[size_t(held_)];
        const float ey = height(px_, pz_) + kEye;
        const float dx = std::sin(yaw_) * std::cos(pitch_), dy = std::sin(pitch_), dz = std::cos(yaw_) * std::cos(pitch_);
        float low, high, across;
        extent_of(p, low, high, across);
        // Where the crosshair meets a surface facing up within reach (a shelf board, a table top, the
        // grass), it rests there; else it floats two metres along it.
        float t = 2.0f;
        const bool hit = surface_along(px_, ey, pz_, dx, dy, dz, 3.0f, held_, t);
        p.x = px_ + dx * t, p.z = pz_ + dz * t, p.y = ey + dy * t;
        if (hit) p.y -= low;
        p.y = std::max(p.y, surface_below(p.x, p.y + low + 0.05f, p.z, held_) - low);
    }
    physics_step(dt, mx, mz);
}

void PersonalWorld::hold(SDL_Scancode key, float seconds)
{
    bool keys[SDL_SCANCODE_COUNT] = {};
    keys[key] = true;
    for (float t = 0; t < seconds; t += 1.0f / 60) step(1.0f / 60, keys);
}

void PersonalWorld::render()
{
    int w = 0, h = 0;
    SDL_GetRenderOutputSize(r_, &w, &h);
    W_ = float(w), H_ = float(h);
    cat_panel_.step();
    place_panel_.step();
    if (held_ < 0) place_panel_.open = false, place_panel_.shown = 0;
    SDL_SetRenderDrawBlendMode(r_, SDL_BLENDMODE_BLEND);
    // The sky: deep blue overhead to pale at the horizon (where a level line meets the screen).
    const float f = (H_ / 2) / std::tan(kFov / 2), horizon = H_ / 2 + std::tan(pitch_) * f;
    if (wireframe_)
    {
        // Real Graphics off: everything as the wireframe on black (Edward, 13 November 2026: "everything
        // should be wireframe when real graphics are off").
        SDL_SetRenderDrawColor(r_, 0, 0, 0, 255);
        SDL_RenderClear(r_);
    }
    else
    {
        const float hy = std::clamp(horizon, 0.0f, H_);
        const SDL_FColor top = mix(kSkyLow, kSkyTop, std::clamp((horizon - 0) / (H_ * 0.6f), 0.0f, 1.0f));
        std::vector<SDL_Vertex> v = {{{0, 0}, top, {}}, {{W_, 0}, top, {}}, {{W_, hy}, kSkyLow, {}}, {{0, hy}, kSkyLow, {}},
                                     {{0, hy}, kSkyLow, {}}, {{W_, hy}, kSkyLow, {}}, {{W_, H_}, kSkyLow, {}}, {{0, H_}, kSkyLow, {}}};
        const int idx[] = {0, 1, 2, 0, 2, 3, 4, 5, 6, 4, 6, 7};
        SDL_RenderGeometry(r_, nullptr, v.data(), int(v.size()), idx, 12);
    }
    // The clouds, far first: each a few soft white discs, drifting with the wind.
    {
        std::vector<std::pair<float, std::array<float, 4>>> discs; // depth; x, y, radius, shade
        for (const Cloud& c : clouds_)
            for (const auto& p : c.puffs)
            {
                float sx, sy, d;
                const float x = std::fmod(c.x + wind_ + 1800.0f, 1200.0f) - 600.0f;
                if (!project(x + p[0], c.y + p[1], c.z, sx, sy, d)) continue;
                discs.push_back({d, {sx, sy, p[2] * f / d, p[1] < 0 ? 0.92f : 1.0f}});
            }
        std::sort(discs.begin(), discs.end(), [](const auto& a, const auto& b) { return a.first > b.first; });
        for (const auto& [d, c] : discs)
        {
            constexpr int kSides = 28;
            if (wireframe_)
            {
                SDL_SetRenderDrawColorFloat(r_, 0.55f * c[3], 0.55f * c[3], 0.60f * c[3], 1);
                for (int k = 0; k < kSides; ++k)
                {
                    const float a0 = 2 * kPi * float(k) / kSides, a1 = 2 * kPi * float(k + 1) / kSides;
                    SDL_RenderLine(r_, c[0] + c[2] * std::cos(a0), c[1] + c[2] * 0.6f * std::sin(a0), c[0] + c[2] * std::cos(a1), c[1] + c[2] * 0.6f * std::sin(a1));
                }
                continue;
            }
            std::vector<SDL_Vertex> v;
            const SDL_FColor in{c[3], c[3], c[3], 0.9f}, out{c[3], c[3], c[3], 0.0f};
            for (int k = 0; k < kSides; ++k)
            {
                const float a0 = 2 * kPi * float(k) / kSides, a1 = 2 * kPi * float(k + 1) / kSides;
                v.push_back({{c[0], c[1]}, in, {}});
                v.push_back({{c[0] + c[2] * std::cos(a0), c[1] + c[2] * 0.6f * std::sin(a0)}, out, {}});
                v.push_back({{c[0] + c[2] * std::cos(a1), c[1] + c[2] * 0.6f * std::sin(a1)}, out, {}});
            }
            SDL_RenderGeometry(r_, nullptr, v.data(), int(v.size()), nullptr, 0);
        }
    }
    // The hill and the objects placed on it, as one set of triangles: each cut where it passes behind
    // you, shaded by its slope against the sun, hazed into the sky with distance, and drawn far first.
    {
        const float sun[3] = {-0.45f, 0.80f, -0.40f};
        using Pt = std::array<float, 3>;
        struct World
        {
            std::array<Pt, 3> t;
            SDL_FColor base;
            float lift;          // (the grass's lighter crests: its height adds this much light a metre)
            unsigned edges = 7; // the wireframe's: bit k, the edge from point k to the next, is drawn
            bool obj = false;    // an object's face (its winding outward), not the ground's
        };
        std::vector<World> world;
        const int n = int(2 * kHalf / kCell);
        world.reserve(size_t(n) * size_t(n) * 2 + placed_.size() * 64);
        for (int i = 0; i < n; ++i)
            for (int j = 0; j < n; ++j)
            {
                const float x0 = -kHalf + float(i) * kCell, z0 = -kHalf + float(j) * kCell, x1 = x0 + kCell, z1 = z0 + kCell;
                const float h00 = height(x0, z0), h10 = height(x1, z0), h01 = height(x0, z1), h11 = height(x1, z1);
                // (a faint variation of the cell's own, from the seed)
                uint64_t s = seed_ ^ (uint64_t(uint32_t(i)) << 32 | uint32_t(j));
                const float vary = 1.0f + (unit(s) - 0.5f) * 0.10f;
                const SDL_FColor g{kGrass.r * vary, kGrass.g * vary, kGrass.b * vary, 1};
                world.push_back({{Pt{x0, h00, z0}, Pt{x1, h10, z0}, Pt{x1, h11, z1}}, g, 0.10f / 30.0f, 3}); // (not the diagonal)
                world.push_back({{Pt{x0, h00, z0}, Pt{x1, h11, z1}, Pt{x0, h01, z1}}, g, 0.10f / 30.0f, 6});
            }
        // The objects after the ground, all of them: a small thing near a large cell of grass would
        // otherwise be painted over by it (the cell's middle can be nearer than the thing). One
        // hidden behind a hill is left out, by a line of sight to its top over the height field.
        const size_t ground_tris = world.size(); // (the ground's; the rest are objects')
        struct Picture
        {
            std::array<Pt, 4> w;
            Pt n; // its front's outward normal
            std::shared_ptr<const sieve::RgbaImage> img;
        };
        std::vector<Picture> pics;
        {
            std::vector<std::pair<std::array<Pt, 3>, SDL_FColor>> faces;
            const float eye_y = height(px_, pz_) + kEye;
            for (const Placed& p : placed_)
            {
                const float g = p.y;
                bool seen = true;
                const float dx = p.x - px_, dz = p.z - pz_, d = std::sqrt(dx * dx + dz * dz);
                for (float u = 2.0f / std::max(d, 2.0f); u < 1.0f && seen; u += 2.0f / std::max(d, 2.0f))
                    seen = ground(px_ + dx * u, pz_ + dz * u) < eye_y + (g + 0.5f - eye_y) * u;
                if (!seen) continue;
                faces.clear();
                faces_of(p, faces);
                for (const auto& [t, c] : faces) world.push_back({t, c, 0, 7, true});
                // A favourite's picture on its front, where the hallway put it on its shelf (its line's
                // face rectangle), a few millimetres proud of the face. (Not in the wireframe.)
                if (!wireframe_ && p.kind.rfind("fav:", 0) == 0)
                    if (auto img = picture_of(p.kind.substr(4)))
                    {
                        Picture pic;
                        pic.img = img;
                        if (picture_quad(p, pic.w, pic.n)) pics.push_back(std::move(pic));
                    }
            }
        }
        struct Tri
        {
            float depth;
            std::array<SDL_FPoint, 6> p;
            int count;
            SDL_FColor c;
            std::array<bool, 6> edge{}; // the wireframe's: the edge from point k to the next drawn
            bool object = false;
            std::array<float, 6> nz{}; // each point's nearness (1/depth), for the depth test
        };
        std::vector<Tri> tris, objs;
        tris.reserve(world.size());
        const float cy = std::cos(yaw_), sy0 = std::sin(yaw_), cp = std::cos(pitch_), sp = std::sin(pitch_), eye = height(px_, pz_) + kEye;
        auto cam = [&](const Pt& w) {
            const float dx = w[0] - px_, dy = w[1] - eye, dz = w[2] - pz_;
            const float x1 = dx * cy - dz * sy0, z1 = dx * sy0 + dz * cy;
            return Pt{x1, dy * cp - z1 * sp, dy * sp + z1 * cp};
        };
        for (const World& w : world)
        {
            const auto& t = w.t;
            // Its slope, against the sun (either side of a face: the one towards the sky).
            const float ax = t[1][0] - t[0][0], ay = t[1][1] - t[0][1], az = t[1][2] - t[0][2];
            const float bx = t[2][0] - t[0][0], by = t[2][1] - t[0][1], bz = t[2][2] - t[0][2];
            float nx = ay * bz - az * by, ny = az * bx - ax * bz, nz = ax * by - ay * bx;
            const float nl = std::max(1e-6f, std::sqrt(nx * nx + ny * ny + nz * nz));
            if (w.obj)
            {
                // An object's face turned away from the eye is behind the object itself: not drawn.
                const float vx = t[0][0] - px_, vy = t[0][1] - eye, vz = t[0][2] - pz_;
                if (vx * nx + vy * ny + vz * nz > 0) continue;
            }
            else if (ny < 0) nx = -nx, ny = -ny, nz = -nz; // (the ground: the side towards the sky)
            const float lit = std::max(0.0f, (nx * sun[0] + ny * sun[1] + nz * sun[2]) / nl);
            const float cxw = (t[0][0] + t[1][0] + t[2][0]) / 3, cyw = (t[0][1] + t[1][1] + t[2][1]) / 3, czw = (t[0][2] + t[1][2] + t[2][2]) / 3;
            const float shade = 0.55f + 0.45f * lit + w.lift * cyw;
            const float dist = std::sqrt((cxw - px_) * (cxw - px_) + (czw - pz_) * (czw - pz_));
            SDL_FColor c{std::clamp(w.base.r * shade, 0.0f, 1.0f), std::clamp(w.base.g * shade, 0.0f, 1.0f), std::clamp(w.base.b * shade, 0.0f, 1.0f), 1};
            if (wireframe_) c = mix(w.base, SDL_FColor{0, 0, 0, 1}, std::pow(std::clamp(dist / 700.0f, 0.0f, 1.0f), 1.5f));
            else c = mix(c, kSkyLow, std::pow(std::clamp(dist / 700.0f, 0.0f, 1.0f), 1.5f));
            // Cut against the near plane (Sutherland-Hodgman), then onto the screen.
            std::array<Pt, 3> in{};
            std::array<Pt, 6> outp{};
            std::array<bool, 6> oute{};
            int no = 0;
            for (int k = 0; k < 3; ++k) in[size_t(k)] = cam(t[size_t(k)]);
            for (int k = 0; k < 3; ++k)
            {
                const Pt& a = in[size_t(k)];
                const Pt& b = in[size_t((k + 1) % 3)];
                const bool ain = a[2] >= kNear, bin = b[2] >= kNear;
                const bool e = (w.edges >> k) & 1u;
                if (ain) oute[size_t(no)] = e, outp[size_t(no++)] = a;
                if (ain != bin)
                {
                    // (leaving: the next edge runs along the near plane, never drawn; entering: it is
                    // the rest of this one)
                    const float u = (kNear - a[2]) / (b[2] - a[2]);
                    oute[size_t(no)] = !ain && e;
                    outp[size_t(no++)] = {a[0] + (b[0] - a[0]) * u, a[1] + (b[1] - a[1]) * u, kNear};
                }
            }
            if (no < 3) continue;
            Tri tr{};
            tr.count = no;
            tr.c = c;
            tr.edge = oute;
            float sum = 0;
            for (int k = 0; k < no; ++k)
            {
                const Pt& q = outp[size_t(k)];
                tr.p[size_t(k)] = {W_ / 2 + q[0] / q[2] * f, H_ / 2 - q[1] / q[2] * f};
                tr.nz[size_t(k)] = 1.0f / q[2];
                sum += q[2];
            }
            tr.object = size_t(&w - world.data()) >= ground_tris;
            tr.depth = sum / float(no);
            if (tr.object) objs.push_back(tr);
            else tris.push_back(tr);
        }
        // The ground far first (its cells are alike in size, so their middles order them); the
        // objects after it, by the depth test (draw_objects).
        std::sort(tris.begin(), tris.end(), [](const Tri& a, const Tri& b) { return a.depth > b.depth; });
        if (wireframe_)
        {
            // Far first, the ground as its open grid (the cells' sides); the objects after, by the depth
            // layer below. (The ground is not filled: on a slope seen at a low angle its long thin
            // cells cannot be put in order by their middles, and a cell drawn out of order would hide
            // lines that show. Objects behind a hill are already left out, by their line of sight.)
            for (const Tri& t : tris)
            {
                SDL_SetRenderDrawColorFloat(r_, t.c.r, t.c.g, t.c.b, 1);
                for (int k = 0; k < t.count; ++k)
                    if (t.edge[size_t(k)])
                    {
                        const SDL_FPoint& a = t.p[size_t(k)];
                        const SDL_FPoint& b = t.p[size_t((k + 1) % t.count)];
                        SDL_RenderLine(r_, a.x, a.y, b.x, b.y);
                    }
            }
        }
        else
        {
        std::vector<SDL_Vertex> v;
        v.reserve(tris.size() * 12);
        for (const Tri& t : tris)
            for (int k = 1; k + 1 < t.count; ++k)
                for (const int q : {0, k, k + 1}) v.push_back({t.p[size_t(q)], t.c, {}});
        SDL_RenderGeometry(r_, nullptr, v.data(), int(v.size()), nullptr, 0);
        }
        // The objects: every face filled into the depth layer (black in the wireframe, its edges then
        // drawn where they are not behind a face), and the layer laid over the ground.
        {
            DepthLayer& L = layer_;
            const int lw = std::max(1, int(W_)), lh = std::max(1, int(H_));
            if (L.w != lw || L.h != lh || !L.tex)
            {
                gpu::destroy(L.tex);
                L.w = lw, L.h = lh;
                L.px.assign(size_t(lw) * size_t(lh), 0);
                L.near.assign(size_t(lw) * size_t(lh), 0.0f);
                L.tex = gpu::create(r_, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, lw, lh);
                if (L.tex) SDL_SetTextureBlendMode(L.tex, SDL_BLENDMODE_BLEND);
                L.x0 = L.y0 = 0, L.x1 = lw - 1, L.y1 = lh - 1;
            }
            // (only the region drawn on the last frame is cleared)
            for (int y = L.y0; y <= L.y1; ++y)
            {
                const size_t row = size_t(y) * size_t(L.w);
                std::fill(L.px.begin() + std::ptrdiff_t(row + size_t(L.x0)), L.px.begin() + std::ptrdiff_t(row + size_t(L.x1) + 1), 0u);
                std::fill(L.near.begin() + std::ptrdiff_t(row + size_t(L.x0)), L.near.begin() + std::ptrdiff_t(row + size_t(L.x1) + 1), 0.0f);
            }
            L.x0 = L.w, L.y0 = L.h, L.x1 = -1, L.y1 = -1;
            for (const Tri& t : objs) fill_face(L, t.p.data(), t.nz.data(), t.count, wireframe_ ? 0xFF000000u : argb_of(t.c));
            for (const Picture& pic : pics)
            {
                // (its front turned towards you, and wholly before you)
                const float vx = pic.w[0][0] - px_, vy = pic.w[0][1] - eye, vz = pic.w[0][2] - pz_;
                if (vx * pic.n[0] + vy * pic.n[1] + vz * pic.n[2] >= 0) continue;
                SDL_FPoint sp[4];
                float nz[4];
                bool before = true;
                for (int k = 0; k < 4 && before; ++k)
                {
                    const Pt q = cam(pic.w[size_t(k)]);
                    before = q[2] >= kNear;
                    sp[k] = {W_ / 2 + q[0] / q[2] * f, H_ / 2 - q[1] / q[2] * f};
                    nz[k] = 1.0f / q[2];
                }
                if (!before) continue;
                const float uv[4][2] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
                const SDL_FPoint t1[3] = {sp[0], sp[1], sp[2]}, t2[3] = {sp[0], sp[2], sp[3]};
                const float n1[3] = {nz[0], nz[1], nz[2]}, n2[3] = {nz[0], nz[2], nz[3]};
                const float uv1[3][2] = {{uv[0][0], uv[0][1]}, {uv[1][0], uv[1][1]}, {uv[2][0], uv[2][1]}};
                const float uv2[3][2] = {{uv[0][0], uv[0][1]}, {uv[2][0], uv[2][1]}, {uv[3][0], uv[3][1]}};
                fill_picture(L, t1, n1, uv1, *pic.img);
                fill_picture(L, t2, n2, uv2, *pic.img);
            }
            if (wireframe_)
                for (const Tri& t : objs)
                    for (int k = 0; k < t.count; ++k)
                        if (t.edge[size_t(k)])
                        {
                            const int k2 = (k + 1) % t.count;
                            edge_line(L, t.p[size_t(k)], t.p[size_t(k2)], t.nz[size_t(k)], t.nz[size_t(k2)], argb_of(t.c));
                        }
            if (L.tex && L.x1 >= L.x0 && L.y1 >= L.y0)
            {
                const SDL_Rect rr{L.x0, L.y0, L.x1 - L.x0 + 1, L.y1 - L.y0 + 1};
                SDL_UpdateTexture(L.tex, &rr, L.px.data() + size_t(L.y0) * size_t(L.w) + size_t(L.x0), L.w * 4);
                const SDL_FRect fr{float(rr.x), float(rr.y), float(rr.w), float(rr.h)};
                SDL_RenderTexture(r_, L.tex, &fr, &fr);
            }
        }
    }
    // Your characters on the hill: yours a few steps ahead of where you began, those in your store in
    // a ring round the top; each sphere a disc, each bone a line, at its own size (4096 units a metre).
    {
        const Swell& top = swells_.front();
        for (size_t ci = 0; ci < characters_.size(); ++ci)
        {
            const sieve::Character& c = characters_[ci];
            float bx, bz;
            if (ci == 0) bx = 0, bz = -52;
            else
            {
                const float a = 2 * kPi * float(ci - 1) / float(std::max<size_t>(1, characters_.size() - 1));
                bx = top.x + 14 * std::cos(a), bz = top.z + 14 * std::sin(a);
            }
            int32_t low = 0;
            for (const auto& s : c.spheres) low = std::min(low, s.y - s.r);
            const float ground = height(bx, bz), unitm = 1.0f / float(sieve::kCharacterUnitsPerMetre);
            auto at = [&](const sieve::Sphere& s, float& sx, float& sy, float& d) {
                return project(bx + float(s.x) * unitm, ground + float(s.y - low) * unitm, bz + float(s.z) * unitm, sx, sy, d);
            };
            // (a sphere hidden behind something placed, a bookcase between you and it, is not drawn, nor
            // its bones)
            auto hidden = [&](const sieve::Sphere& s) {
                if (placed_.empty()) return false;
                const float wx = bx + float(s.x) * unitm, wy = ground + float(s.y - low) * unitm, wz = bz + float(s.z) * unitm;
                const float ey = height(px_, pz_) + kEye, vx = wx - px_, vy = wy - ey, vz = wz - pz_, len = std::sqrt(vx * vx + vy * vy + vz * vz);
                const float o[3] = {px_, ey, pz_}, dir[3] = {vx / len, vy / len, vz / len};
                return ray_objects(o, dir, len, held_, nullptr) < len - 0.05f;
            };
            SDL_SetRenderDrawColor(r_, 60, 60, 70, 255);
            for (const auto& s : c.spheres)
                if (s.parent >= 0 && !hidden(s) && !hidden(c.spheres[size_t(s.parent)]))
                {
                    float x1, y1, d1, x2, y2, d2;
                    if (at(s, x1, y1, d1) && at(c.spheres[size_t(s.parent)], x2, y2, d2)) SDL_RenderLine(r_, x1, y1, x2, y2);
                }
            for (const auto& s : c.spheres)
            {
                float sx, sy, d;
                if (!at(s, sx, sy, d)) continue;
                if (hidden(s)) continue;
                const float rad = std::max(1.5f, float(s.r) * unitm * f / d);
                const SDL_FColor col = ci == 0 ? SDL_FColor{0.95f, 0.92f, 0.85f, 1} : SDL_FColor{0.80f, 0.82f, 0.90f, 1};
                if (wireframe_)
                {
                    SDL_SetRenderDrawColorFloat(r_, col.r, col.g, col.b, 1);
                    constexpr int kRing = 20;
                    for (int k = 0; k < kRing; ++k)
                    {
                        const float a0 = 2 * kPi * float(k) / kRing, a1 = 2 * kPi * float(k + 1) / kRing;
                        SDL_RenderLine(r_, sx + rad * std::cos(a0), sy + rad * std::sin(a0), sx + rad * std::cos(a1), sy + rad * std::sin(a1));
                    }
                    continue;
                }
                std::vector<SDL_Vertex> v;
                constexpr int kSides = 20;
                for (int k = 0; k < kSides; ++k)
                {
                    const float a0 = 2 * kPi * float(k) / kSides, a1 = 2 * kPi * float(k + 1) / kSides;
                    v.push_back({{sx, sy}, col, {}});
                    v.push_back({{sx + rad * std::cos(a0), sy + rad * std::sin(a0)}, mix(col, SDL_FColor{0.4f, 0.4f, 0.5f, 1}, 0.5f), {}});
                    v.push_back({{sx + rad * std::cos(a1), sy + rad * std::sin(a1)}, mix(col, SDL_FColor{0.4f, 0.4f, 0.5f, 1}, 0.5f), {}});
                }
                SDL_RenderGeometry(r_, nullptr, v.data(), int(v.size()), nullptr, 0);
            }
        }
    }
    const SDL_Color white{255, 255, 255, 255}, grey{200, 200, 210, 255};
    if (show_map_) draw_map();
    if (show_compass_) draw_compass();
    // The crosshair, and what it rests on: the keys for it (an author's; a visitor may only open).
    SDL_SetRenderDrawColor(r_, 255, 255, 255, 200);
    SDL_RenderLine(r_, W_ / 2 - 6, H_ / 2, W_ / 2 + 6, H_ / 2);
    SDL_RenderLine(r_, W_ / 2, H_ / 2 - 6, W_ / 2, H_ / 2 + 6);
    if (held_ >= 0) draw_text(r_, W_ / 2 + 14, H_ / 2 + 10, trf("pworld.holding", {label_of(placed_[size_t(held_)].kind)}), 1, white);
    else if (const int a = aimed(); a >= 0)
        draw_text(r_, W_ / 2 + 14, H_ / 2 + 10,
                  author_ ? trf("pworld.aim", {label_of(placed_[size_t(a)].kind), Keys::get().first(pk::kPickUp), Keys::get().first(pk::kOpen)})
                          : trf("pworld.aim.visitor", {label_of(placed_[size_t(a)].kind), Keys::get().first(pk::kOpen)}),
                  1, white);
    draw_text(r_, 20, 16, tr("pworld.title"), 2, white);
    if (cat_panel_.shown > 0)
    {
        // The catalogue: a list on a dark plate sliding in at the left, the one chosen marked.
        const auto& cat = rows_;
        const float x = cat_panel_.left(300);
        const SDL_FRect plate{x, 48, 290, 34 + float(cat.size()) * 18};
        SDL_SetRenderDrawColor(r_, 0, 0, 0, 170);
        SDL_RenderFillRect(r_, &plate);
        draw_text(r_, x + 22, 56, tr("pworld.catalogue"), 1, white);
        for (size_t i = 0; i < cat.size(); ++i)
            draw_text(r_, x + 22, 76 + float(i) * 18, (int(i) == catalogue_row_ ? "> " : "  ") + label_of(cat[i]), 1, int(i) == catalogue_row_ ? white : grey);
    }
    if (place_panel_.shown > 0 && held_ >= 0)
    {
        // Place: the thing in your hand's exact place (metres) and turns (degrees), each row editable.
        const Placed& p = placed_[size_t(held_)];
        const float x = place_panel_.left(300);
        const SDL_FRect plate{x, 48, 290, 34 + 8 * 18};
        SDL_SetRenderDrawColor(r_, 0, 0, 0, 170);
        SDL_RenderFillRect(r_, &plate);
        draw_text(r_, x + 22, 56, trf("pworld.place.title", {label_of(p.kind)}), 1, white);
        const float deg = 180.0f / kPi;
        const double values[6] = {p.x, p.y, p.z, p.yaw * deg, p.pitch * deg, p.roll * deg};
        static const char* const kRows[7] = {"pworld.place.x", "pworld.place.y", "pworld.place.z", "pworld.place.yaw", "pworld.place.pitch", "pworld.place.roll", "pworld.place.go"};
        for (int i = 0; i < 7; ++i)
        {
            std::string line = (i == place_row_ ? "> " : "  ") + tr(kRows[i]);
            if (i < 6)
            {
                char b[48];
                std::snprintf(b, sizeof b, i < 3 ? "%.3f" : "%.2f", values[i]);
                line += "  " + (i == place_row_ && !place_edit_.empty() ? place_edit_ + "_" : std::string(b));
                if (i == 1)
                {
                    std::snprintf(b, sizeof b, "  (%.3f %s)", double(p.y - ground(p.x, p.z)), tr("pworld.place.above").c_str());
                    line += b;
                }
            }
            draw_text(r_, x + 22, 76 + float(i) * 18, line, 1, i == place_row_ ? white : grey);
        }
    }
    // Their tabs at the left edge while they are out: Place while something is held, else the catalogue.
    if (held_ >= 0) place_panel_.draw_tab(r_, 80, tr("pworld.place.tab"));
    else cat_panel_.draw_tab(r_, 80, tr("pworld.catalogue.tab"));
    if (!note_.empty()) draw_text(r_, 20, H_ - 42, note_, 1, white);
    {
        // The footer: the keys as they are set (Settings > Controls > Keys), each by what it does.
        const Keys& K = Keys::get();
        auto f = [&](const std::string& id) { return K.first(id); };
        std::string foot;
        if (cat_panel_.open) foot = trf("pworld.footer.catalogue", {f(pk::kChooseUp), f(pk::kChooseDown), f(pk::kPlace), f(pk::kCatalogue)});
        else if (place_panel_.open && held_ >= 0) foot = trf("pworld.footer.place", {f(pk::kDrop), f(pk::kExact)});
        else if (held_ >= 0)
            foot = trf("pworld.footer.holding", {f(pk::kTurn), f(pk::kTip), f(pk::kRoll), f(pk::kTurnBack), f(pk::kTipBack), f(pk::kRollBack), f(pk::kExact), f(pk::kPlaceHere), f(pk::kDrop), f(pk::kPutBack)});
        else
            foot = trf("pworld.footer", {f(pk::kForward) + " " + f(pk::kLeft) + " " + f(pk::kBack) + " " + f(pk::kRight), f(pk::kLookUp) + "/" + f(pk::kLookDown), f(pk::kRun),
                                         f(pk::kCatalogue), f(pk::kPickUp), f(pk::kOpen), f(pk::kTakeAway), f(pk::kZoomIn) + "/" + f(pk::kZoomOut)});
        draw_text(r_, 20, H_ - 24, foot, 1, white);
    }
    if (pause_open_) draw_pause(); // (over everything)
}

// ---- the map and the compass (IDEAS 19.197)

// The compass, along the top: a strip of the 120 degrees about where you face, a tick every 15, N, E,
// S and W at 0, 90, 180 and 270 (north +z, east +x), the way you face marked in its middle with its
// bearing in degrees.
void PersonalWorld::draw_compass()
{
    const float cx = W_ / 2, top = 12, w = 360, span = 120;
    float heading = std::fmod(yaw_ * 180.0f / kPi, 360.0f);
    if (heading < 0) heading += 360;
    const SDL_FRect strip{cx - w / 2, top, w, 22};
    SDL_SetRenderDrawColor(r_, 0, 0, 0, wireframe_ ? 255 : 120);
    SDL_RenderFillRect(r_, &strip);
    SDL_SetRenderDrawColor(r_, 200, 200, 210, 255);
    SDL_RenderRect(r_, &strip);
    for (int d = 0; d < 360; d += 15)
    {
        float off = float(d) - heading;
        while (off > 180) off -= 360;
        while (off < -180) off += 360;
        if (std::fabs(off) > span / 2) continue;
        const float x = cx + off / span * w;
        const bool cardinal = d % 90 == 0;
        SDL_RenderLine(r_, x, top + 22, x, top + (cardinal ? 12 : 17));
        if (cardinal)
        {
            static const char* const kN[4] = {"N", "E", "S", "W"};
            draw_text(r_, x - 4, top + 3, kN[d / 90], 1, SDL_Color{255, 255, 255, 255});
        }
    }
    SDL_SetRenderDrawColor(r_, 255, 210, 80, 255);
    SDL_RenderLine(r_, cx, top - 2, cx, top + 24);
    char b[16];
    std::snprintf(b, sizeof b, "%03d", int(std::lround(heading)) % 360);
    draw_text(r_, cx - text_width(b, 1) / 2, top + 28, b, 1, SDL_Color{255, 210, 80, 255});
}

// The map, top right: the whole world from above, north up, its ground shaded by height (contours
// while Real Graphics is off), what is placed as dots, your characters as rings, you as an arrow.
void PersonalWorld::draw_map()
{
    constexpr int kCells = 32;
    const float size = 180, x0 = W_ - size - 16, y0 = 16, cell = size / kCells;
    const SDL_FRect frame{x0, y0, size, size};
    SDL_SetRenderDrawColor(r_, 0, 0, 0, wireframe_ ? 255 : 150);
    SDL_RenderFillRect(r_, &frame);
    // What it shows: the whole world at zoom 0; each step in, half as far each way, centred on you.
    const float half = kHalf / float(1 << map_zoom_);
    const float cxw = map_zoom_ ? px_ : 0.0f, czw = map_zoom_ ? pz_ : 0.0f;
    auto at = [&](float wx, float wz) { return SDL_FPoint{x0 + (wx - cxw + half) / (2 * half) * size, y0 + (half - (wz - czw)) / (2 * half) * size}; };
    auto inside = [&](SDL_FPoint q) { return q.x >= x0 && q.x <= x0 + size && q.y >= y0 && q.y <= y0 + size; };
    float lo = 1e9f, hi = -1e9f, hs[kCells][kCells];
    for (int i = 0; i < kCells; ++i)
        for (int j = 0; j < kCells; ++j)
        {
            hs[i][j] = height(cxw - half + (float(i) + 0.5f) * 2 * half / kCells, czw + half - (float(j) + 0.5f) * 2 * half / kCells);
            lo = std::min(lo, hs[i][j]), hi = std::max(hi, hs[i][j]);
        }
    const float band = std::max(1.0f, (hi - lo) / 8); // (eight bands of height)
    for (int i = 0; i < kCells; ++i)
        for (int j = 0; j < kCells; ++j)
        {
            const float t = (hs[i][j] - lo) / std::max(1e-3f, hi - lo);
            if (!wireframe_)
            {
                SDL_SetRenderDrawColorFloat(r_, kGrass.r * (0.6f + 0.6f * t), kGrass.g * (0.6f + 0.6f * t), kGrass.b * (0.6f + 0.6f * t), 1);
                const SDL_FRect c{x0 + float(i) * cell, y0 + float(j) * cell, cell + 0.5f, cell + 0.5f};
                SDL_RenderFillRect(r_, &c);
            }
            else
            {
                // A contour where the band of height changes to the next cell east or south.
                SDL_SetRenderDrawColorFloat(r_, kGrass.r, kGrass.g, kGrass.b, 1);
                const int b0 = int((hs[i][j] - lo) / band);
                if (i + 1 < kCells && int((hs[i + 1][j] - lo) / band) != b0)
                    SDL_RenderLine(r_, x0 + float(i + 1) * cell, y0 + float(j) * cell, x0 + float(i + 1) * cell, y0 + float(j + 1) * cell);
                if (j + 1 < kCells && int((hs[i][j + 1] - lo) / band) != b0)
                    SDL_RenderLine(r_, x0 + float(i) * cell, y0 + float(j + 1) * cell, x0 + float(i + 1) * cell, y0 + float(j + 1) * cell);
            }
        }
    SDL_SetRenderDrawColor(r_, 200, 200, 210, 255);
    SDL_RenderRect(r_, &frame);
    // What is placed: a dot each (yellow while dropped and moving).
    for (const Placed& p : placed_)
    {
        const SDL_FPoint q = at(p.x, p.z);
        if (!inside(q)) continue;
        SDL_SetRenderDrawColor(r_, 255, p.physics && !p.resting ? 220 : 255, p.physics && !p.resting ? 60 : 255, 255);
        const SDL_FRect d{q.x - 1.5f, q.y - 1.5f, 3, 3};
        SDL_RenderFillRect(r_, &d);
    }
    // Your characters, as rings where they stand (the first before you, the rest round the hilltop).
    SDL_SetRenderDrawColor(r_, 200, 205, 230, 255);
    for (size_t ci = 0; ci < characters_.size(); ++ci)
    {
        float bx = 0, bz = -52;
        if (ci > 0)
        {
            const float a = 2 * kPi * float(ci - 1) / float(std::max<size_t>(1, characters_.size() - 1));
            bx = swells_.empty() ? 0 : swells_[0].x + 14 * std::cos(a), bz = swells_.empty() ? 0 : swells_[0].z + 14 * std::sin(a);
        }
        const SDL_FPoint q = at(bx, bz);
        if (!inside(q)) continue;
        for (int k = 0; k < 8; ++k)
        {
            const float a0 = 2 * kPi * float(k) / 8, a1 = 2 * kPi * float(k + 1) / 8;
            SDL_RenderLine(r_, q.x + 3 * std::cos(a0), q.y + 3 * std::sin(a0), q.x + 3 * std::cos(a1), q.y + 3 * std::sin(a1));
        }
    }
    // You: an arrow pointing the way you face.
    const SDL_FPoint me = at(px_, pz_);
    const float fx = std::sin(yaw_), fz = std::cos(yaw_); // (forward; on the map, +z is up)
    const SDL_FPoint tip{me.x + fx * 8, me.y - fz * 8}, l{me.x - fx * 4 - fz * 4, me.y + fz * 4 - fx * 4}, r{me.x - fx * 4 + fz * 4, me.y + fz * 4 + fx * 4};
    const SDL_FColor gold{1.0f, 0.82f, 0.31f, 1};
    if (wireframe_)
    {
        SDL_SetRenderDrawColorFloat(r_, gold.r, gold.g, gold.b, 1);
        const SDL_FPoint loop[4] = {tip, l, r, tip};
        SDL_RenderLines(r_, loop, 4);
    }
    else
    {
        const SDL_Vertex v[3] = {{tip, gold, {}}, {l, gold, {}}, {r, gold, {}}};
        SDL_RenderGeometry(r_, nullptr, v, 3, nullptr, 0);
    }
    {
        char b[48];
        std::snprintf(b, sizeof b, "N ^   %.0f m across", double(2 * half));
        draw_text(r_, x0 + 4, y0 + size + 4, b, 1, SDL_Color{200, 200, 210, 255});
    }
}

// ---- the catalogue of objects, and what is placed

// The catalogue is Sieve's own shelf items, one for each line (Edward, 13 November 2026: "the
// default placeable objects can just be the sieve books for shelves"): a page, a picture, a cassette
// and so on, at their size on the shelves. They are drawn from shelf-items.sieve in data/objects,
// not from the meshes folder, which anyone may change for Real Graphics: "they also need sieving and
// preserving somewhere aside from the overridable ones". The .sieve is read in memory, every member
// checked against its SHA-256 as installing checks it, nothing written to disk.
const std::vector<std::string>& PersonalWorld::catalogue()
{
    // (the shelf items, one a line, then the bookcase they stand on in the hallway)
    static const std::vector<std::string> kAll = {"pages", "image", "audio", "video", "books", "models", "binary", "tracks", "movies", "worlds", "ai", "bookcase"};
    return kAll;
}

namespace {

constexpr uint64_t kShelfItemsRoof = uint64_t(1) << 20; // (what the pack may unfold to: it is 20 kB)

struct ShelfItems
{
    std::map<std::string, std::shared_ptr<const Mesh>> mesh; // by catalogue id
    std::string note;                                        // why some or all could not be read
};

std::filesystem::path shelf_items_file()
{
    namespace fs = std::filesystem;
    std::vector<fs::path> where = {fs::path("data") / "objects"};
    if (const fs::path exe = sieve::cli::executable_dir(); !exe.empty())
    {
        where.push_back(exe / "objects");
        where.push_back(exe / ".." / "data" / "objects");
    }
    for (const fs::path& f : where)
    {
        std::error_code ec;
        if (fs::is_regular_file(f / "shelf-items.sieve", ec)) return f / "shelf-items.sieve";
    }
    return where.front() / "shelf-items.sieve";
}

const ShelfItems& shelf_items()
{
    static const ShelfItems kItems = [] {
        ShelfItems out;
        std::map<std::string, std::string> text; // member's file name to its text
        try
        {
            for (auto& [name, bytes] : sieve::cli::files_in_sieve(shelf_items_file(), kShelfItemsRoof))
                text[std::filesystem::path(sieve::from_u8(name)).filename().string()] = std::string(bytes.begin(), bytes.end());
        }
        catch (const std::exception& x)
        {
            out.note = x.what();
            return out;
        }
        for (const std::string& id : PersonalWorld::catalogue())
        {
            // (the books line's item is the template book; every other line's is book-LINE)
            const std::string file = id == "books" ? "book.obj" : id == "bookcase" ? "bookshelf.obj" : "book-" + id + ".obj";
            const auto it = text.find(file);
            std::shared_ptr<const Mesh> m;
            if (it != text.end())
            {
                std::istringstream in(it->second);
                m = read_obj(in, "shelf-items.sieve: " + file, [&](const std::string& lib) {
                    const auto l = text.find(std::filesystem::path(lib).filename().string());
                    return l == text.end() ? std::string() : l->second;
                });
            }
            if (m && id == "bookcase")
            {
                // The corridor's left-wall case (its front at x = -1.65, along z 0 to 6), moved to stand as
                // every object does: its front at x = 0, facing +X, centred across z.
                auto moved = std::make_shared<Mesh>(*m);
                moved->lo = {1e9f, 1e9f, 1e9f}, moved->hi = {-1e9f, -1e9f, -1e9f};
                for (MeshTri& t : moved->tris)
                    for (Vec3& v : t.p)
                    {
                        v.x += 1.65f, v.z -= 3.0f;
                        moved->lo = {std::min(moved->lo.x, v.x), std::min(moved->lo.y, v.y), std::min(moved->lo.z, v.z)};
                        moved->hi = {std::max(moved->hi.x, v.x), std::max(moved->hi.y, v.y), std::max(moved->hi.z, v.z)};
                    }
                m = moved;
            }
            if (m) out.mesh[id] = m;
            else out.note += (out.note.empty() ? "" : ", ") + file;
        }
        return out;
    }();
    return kItems;
}

} // namespace

std::string PersonalWorld::catalogue_note() { return shelf_items().note; }

const Favourite* PersonalWorld::favourite(const std::string& sha) const
{
    for (const Favourite& f : favourites_)
        if (f.sha == sha) return &f;
    return nullptr;
}

std::shared_ptr<const sieve::RgbaImage> PersonalWorld::picture_of(const std::string& sha) const
{
    if (const auto it = pictures_.find(sha); it != pictures_.end()) return it->second;
    std::shared_ptr<const sieve::RgbaImage> img;
    const auto file = file_.parent_path().parent_path() / "favourites" / (sha + ".png");
    std::error_code ec;
    if (std::filesystem::is_regular_file(file, ec))
        try
        {
            auto frames = sieve::cli::load_image_frames(sieve::u8(file));
            if (!frames.empty() && frames[0].width && frames[0].width <= 4096 && frames[0].height <= 4096) img = std::make_shared<const sieve::RgbaImage>(std::move(frames[0]));
        }
        catch (const std::exception&) {} // (a picture that cannot be read: the item drawn plain)
    pictures_[sha] = img;
    return img;
}

std::string PersonalWorld::mesh_kind(const std::string& kind) const
{
    if (kind.rfind("fav:", 0) != 0) return kind;
    const Favourite* f = favourite(kind.substr(4));
    return f ? f->line : "pages"; // (one no longer kept: drawn as a page until it is taken away)
}

std::string PersonalWorld::label_of(const std::string& kind) const
{
    if (kind.rfind("fav:", 0) != 0) return tr("pworld.object." + kind);
    const Favourite* f = favourite(kind.substr(4));
    if (!f) return trf("pworld.fav.gone", {kind.substr(4, 12)});
    return trf("pworld.fav", {f->title.empty() ? f->sha.substr(0, 12) : f->title.substr(0, 40), f->line});
}

// An object's model point turned by its yaw, pitch and roll and moved to its place. The model's front
// faces +X, its up +Y, its side +Z: roll turns about the front, pitch about the side, then yaw about
// the vertical, so +X ends along the yaw's forward ((sin, cos) in x and z) tipped by the pitch.
static std::array<float, 3> turned(const float m[3], float yaw, float pitch, float roll)
{
    const float cr = std::cos(roll), sr = std::sin(roll), cp = std::cos(pitch), sp = std::sin(pitch), cy = std::cos(yaw), sy = std::sin(yaw);
    // roll, about X
    const float x1 = m[0], y1 = m[1] * cr - m[2] * sr, z1 = m[1] * sr + m[2] * cr;
    // pitch, about Z (the front up for a positive pitch)
    const float x2 = x1 * cp - y1 * sp, y2 = x1 * sp + y1 * cp, z2 = z1;
    // yaw: +X onto (sin, cos), +Z onto (cos, -sin)
    return {x2 * sy + z2 * cy, y2, x2 * cy - z2 * sy};
}

void PersonalWorld::extent_of(const Placed& p, float& low, float& high, float& across) const
{
    low = 0, high = 0.4f, across = 0.2f;
    const auto it = shelf_items().mesh.find(mesh_kind(p.kind));
    if (it == shelf_items().mesh.end()) return;
    const Mesh& m = *it->second;
    low = 1e9f, high = -1e9f, across = 0;
    for (int c = 0; c < 8; ++c)
    {
        const float q[3] = {(c & 1) ? m.hi.x : m.lo.x, (c & 2) ? m.hi.y : m.lo.y, (c & 4) ? m.hi.z : m.lo.z};
        const auto w = turned(q, p.yaw, p.pitch, p.roll);
        low = std::min(low, w[1]), high = std::max(high, w[1]);
        across = std::max(across, std::sqrt(w[0] * w[0] + w[2] * w[2]));
    }
}

// An object's faces in the world: its shelf item's triangles, turned and placed.
void PersonalWorld::faces_of(const Placed& p, std::vector<std::pair<std::array<std::array<float, 3>, 3>, SDL_FColor>>& out) const
{
    const auto it = shelf_items().mesh.find(mesh_kind(p.kind));
    if (it == shelf_items().mesh.end()) return;
    for (const MeshTri& t : it->second->tris)
    {
        std::array<std::array<float, 3>, 3> w{};
        for (int k = 0; k < 3; ++k)
        {
            const float q[3] = {t.p[k].x, t.p[k].y, t.p[k].z};
            const auto r = turned(q, p.yaw, p.pitch, p.roll);
            w[size_t(k)] = {p.x + r[0], p.y + r[1], p.z + r[2]};
        }
        // Wound so its normal points out of the object (the model's own normal, turned with it), so
        // a face turned away can be left out.
        const float nq[3] = {t.n.x, t.n.y, t.n.z};
        const auto n = turned(nq, p.yaw, p.pitch, p.roll);
        const float ax = w[1][0] - w[0][0], ay = w[1][1] - w[0][1], az = w[1][2] - w[0][2], bx = w[2][0] - w[0][0], by = w[2][1] - w[0][1], bz = w[2][2] - w[0][2];
        if ((ay * bz - az * by) * n[0] + (az * bx - ax * bz) * n[1] + (ax * by - ay * bx) * n[2] < 0) std::swap(w[1], w[2]);
        out.push_back({w, t.kd});
    }
}

// Where a favourite's picture goes on it, in the world: on its front (+X), its line's face rectangle
// (as the hallway puts the picture on the item on its shelf), a few millimetres proud of the face;
// corners top left, top right, bottom right, bottom left as you face it (your right is its -Z), and
// the front's outward normal.
bool PersonalWorld::picture_quad(const Placed& p, std::array<std::array<float, 3>, 4>& w, std::array<float, 3>& n) const
{
    const auto it = shelf_items().mesh.find(mesh_kind(p.kind));
    if (it == shelf_items().mesh.end()) return false;
    const FaceRect fr = load_face_rect(mesh_kind(p.kind));
    const float hx = it->second->hi.x + 0.004f;
    const float corner[4][3] = {{hx, fr.top, fr.half_width}, {hx, fr.top, -fr.half_width}, {hx, fr.bottom, -fr.half_width}, {hx, fr.bottom, fr.half_width}};
    for (int k = 0; k < 4; ++k)
    {
        const auto r = turned(corner[k], p.yaw, p.pitch, p.roll);
        w[size_t(k)] = {p.x + r[0], p.y + r[1], p.z + r[2]};
    }
    const float fx[3] = {1, 0, 0};
    n = turned(fx, p.yaw, p.pitch, p.roll);
    return true;
}

// ---- holding, placing, dropping (IDEAS 19.192), and physics

// The highest surface facing up under (x, z) at or below `top`: the grass, or a face of anything
// placed but `skip` (a shelf board, a box's lid), so things rest on one another.
float PersonalWorld::surface_below(float x, float top, float z, int skip) const
{
    float best = ground(x, z);
    std::vector<std::pair<std::array<std::array<float, 3>, 3>, SDL_FColor>> faces;
    for (size_t i = 0; i < placed_.size(); ++i)
    {
        if (int(i) == skip) continue;
        const Placed& o = placed_[i];
        float low, high, across;
        extent_of(o, low, high, across);
        if ((o.x - x) * (o.x - x) + (o.z - z) * (o.z - z) > across * across + 0.01f || o.y + low > top) continue;
        faces.clear();
        faces_of(o, faces);
        for (const auto& [t, c] : faces)
        {
            const float ax = t[1][0] - t[0][0], ay = t[1][1] - t[0][1], az = t[1][2] - t[0][2];
            const float bx = t[2][0] - t[0][0], by = t[2][1] - t[0][1], bz = t[2][2] - t[0][2];
            const float nx = ay * bz - az * by, ny = az * bx - ax * bz, nz = ax * by - ay * bx;
            const float nl = std::sqrt(nx * nx + ny * ny + nz * nz);
            if (nl < 1e-9f || std::fabs(ny) / nl < 0.7f) continue; // (only what faces up or down: a floor to stand on)
            // (x, z) inside the triangle's shadow on the ground, and its height there
            const float d = ax * bz - az * bx;
            if (std::fabs(d) < 1e-12f) continue;
            const float u = ((x - t[0][0]) * bz - (z - t[0][2]) * bx) / d, v = (ax * (z - t[0][2]) - az * (x - t[0][0])) / d;
            if (u < -1e-4f || v < -1e-4f || u + v > 1 + 1e-4f) continue;
            const float h = t[0][1] + u * ay + v * by;
            if (h <= top && h > best) best = h;
        }
    }
    return best;
}

// Along a line from (x, y, z) in direction (dx, dy, dz): the first surface facing up within `reach`
// (the grass, or anything placed but `skip`); `t` its distance. False: none.
bool PersonalWorld::surface_along(float x, float y, float z, float dx, float dy, float dz, float reach, int skip, float& t) const
{
    // Stepped a few centimetres at a time, each step asking what is beneath: cheap, and the same on
    // every machine.
    constexpr float kStep = 0.04f;
    for (float s = kStep; s <= reach; s += kStep)
    {
        const float px = x + dx * s, py = y + dy * s, pz = z + dz * s;
        if (surface_below(px, py + 0.001f, pz, skip) >= py - kStep * std::max(0.0f, -dy) - 1e-3f)
        {
            t = s;
            return true;
        }
    }
    return false;
}

// The nearest face of anything placed (but `skip`) along a line from `o` in direction `d` (a unit
// vector), nearer than `reach`: its distance, and which it is in `which`; `reach` itself if none.
float PersonalWorld::ray_objects(const float o[3], const float d[3], float reach, int skip, int* which) const
{
    float bt = reach;
    if (which) *which = -1;
    std::vector<std::pair<std::array<std::array<float, 3>, 3>, SDL_FColor>> faces;
    for (size_t i = 0; i < placed_.size(); ++i)
    {
        if (int(i) == skip) continue;
        faces.clear();
        faces_of(placed_[i], faces);
        for (const auto& [t, c] : faces)
        {
            // (Moller and Trumbore's test, either side of the face)
            const float e1[3] = {t[1][0] - t[0][0], t[1][1] - t[0][1], t[1][2] - t[0][2]}, e2[3] = {t[2][0] - t[0][0], t[2][1] - t[0][1], t[2][2] - t[0][2]};
            const float pv[3] = {d[1] * e2[2] - d[2] * e2[1], d[2] * e2[0] - d[0] * e2[2], d[0] * e2[1] - d[1] * e2[0]};
            const float det = e1[0] * pv[0] + e1[1] * pv[1] + e1[2] * pv[2];
            if (std::fabs(det) < 1e-9f) continue;
            const float tv[3] = {o[0] - t[0][0], o[1] - t[0][1], o[2] - t[0][2]};
            const float u = (tv[0] * pv[0] + tv[1] * pv[1] + tv[2] * pv[2]) / det;
            if (u < 0 || u > 1) continue;
            const float qv[3] = {tv[1] * e1[2] - tv[2] * e1[1], tv[2] * e1[0] - tv[0] * e1[2], tv[0] * e1[1] - tv[1] * e1[0]};
            const float v = (d[0] * qv[0] + d[1] * qv[1] + d[2] * qv[2]) / det;
            if (v < 0 || u + v > 1) continue;
            const float dist = (e2[0] * qv[0] + e2[1] * qv[1] + e2[2] * qv[2]) / det;
            if (dist > 0 && dist < bt)
            {
                bt = dist;
                if (which) *which = int(i);
            }
        }
    }
    return bt;
}

int PersonalWorld::aimed() const
{
    // Along the crosshair from your eye: the nearest face of anything placed, within 6 m (each object's
    // own triangles, so a large thing does not hide a small one before it).
    const float o[3] = {px_, height(px_, pz_) + kEye, pz_};
    const float d[3] = {std::sin(yaw_) * std::cos(pitch_), std::sin(pitch_), std::cos(yaw_) * std::cos(pitch_)};
    int which = -1;
    ray_objects(o, d, 6.0f, held_, &which);
    return which;
}

void PersonalWorld::physics_step(float dt, float move_x, float move_z)
{
    constexpr float kStep = 1.0f / 120, kGravity = 9.81f;
    physics_left_ += dt;
    while (physics_left_ >= kStep)
    {
        physics_left_ -= kStep;
        for (size_t i = 0; i < placed_.size(); ++i)
        {
            Placed& p = placed_[i];
            if (!p.physics || int(i) == held_) continue;
            float low, high, across;
            extent_of(p, low, high, across);
            // Walked into: knocked the way you walk, and up a little.
            const float hx = p.x - px_, hz = p.z - pz_;
            if ((move_x != 0 || move_z != 0) && hx * hx + hz * hz < (across + 0.35f) * (across + 0.35f) && p.y + low < height(px_, pz_) + 1.0f)
            {
                p.vx = move_x * 1.2f, p.vz = move_z * 1.2f, p.vy = std::max(p.vy, 1.5f);
                p.resting = false;
            }
            // Against the other dropped things: pushed apart along the line between them.
            for (size_t j = 0; j < placed_.size(); ++j)
            {
                if (j == i || int(j) == held_) continue;
                const Placed& o = placed_[j];
                if (!o.physics) continue; // (a placed thing is a surface to land on, below, not a body to push)
                float l2, h2, a2;
                extent_of(o, l2, h2, a2);
                const float ox = p.x - o.x, oz = p.z - o.z, d = std::sqrt(ox * ox + oz * oz), r = across + a2;
                if (d < r && d > 1e-4f && p.y + low < o.y + h2 && o.y + l2 < p.y + high)
                {
                    const float push = (r - d) / d * 0.5f;
                    p.x += ox * push, p.z += oz * push;
                    const float along = (p.vx * ox + p.vz * oz) / d;
                    if (along < 0) p.vx -= 1.4f * along * ox / d, p.vz -= 1.4f * along * oz / d;
                    p.resting = false;
                }
            }
            if (p.resting) continue;
            p.vy -= kGravity * kStep;
            const float was_y = p.y; // (where it was: a thin board passed in one step still stops it)
            p.x += p.vx * kStep, p.y += p.vy * kStep, p.z += p.vz * kStep;
            p.x = std::clamp(p.x, -kHalf + 1, kHalf - 1), p.z = std::clamp(p.z, -kHalf + 1, kHalf - 1);
            const float floor = surface_below(p.x, std::max(p.y, was_y) + low + 0.05f, p.z, int(i)) - low; // (the grass, or a shelf or anything beneath)
            if (p.y <= floor)
            {
                // On the grass: a bounce that loses most of its speed, and friction across it.
                p.y = floor;
                p.vy = p.vy < -1.0f ? -p.vy * 0.3f : 0.0f;
                p.vx *= 0.92f, p.vz *= 0.92f;
                if (p.vy == 0 && p.vx * p.vx + p.vz * p.vz < 0.01f)
                {
                    p.vx = p.vz = 0;
                    p.resting = true;
                    unsaved_rest_ = true;
                }
            }
        }
    }
    if (unsaved_rest_)
    {
        bool all = true;
        for (const Placed& p : placed_) all = all && (!p.physics || p.resting);
        if (all) unsaved_rest_ = false, save(); // (where each came to rest)
    }
}

// Place: Up/Down choose a row; Left/Right move it a step (a tenth of a metre, or 5 degrees; Shift ten
// times that); a number typed (digits, '.', '-') and Return sets it exactly; Return on "place it here"
// (or with nothing typed) places it; Backspace takes back a digit; Q still drops it.
void PersonalWorld::place_key(SDL_Keycode k, SDL_Keymod mod)
{
    Placed& p = placed_[size_t(held_)];
    const float deg = kPi / 180.0f;
    float* const vals[6] = {&p.x, &p.y, &p.z, &p.yaw, &p.pitch, &p.roll};
    const bool digit = (k >= SDLK_0 && k <= SDLK_9) || k == SDLK_PERIOD || k == SDLK_MINUS || (k >= SDLK_KP_1 && k <= SDLK_KP_0) || k == SDLK_KP_PERIOD || k == SDLK_KP_MINUS;
    if (k == SDLK_UP) place_row_ = (place_row_ + 6) % 7, place_edit_.clear();
    else if (k == SDLK_DOWN) place_row_ = (place_row_ + 1) % 7, place_edit_.clear();
    else if ((k == SDLK_LEFT || k == SDLK_RIGHT) && place_row_ < 6)
    {
        const float step = (place_row_ < 3 ? 0.1f : 5.0f * deg) * ((mod & SDL_KMOD_SHIFT) ? 10.0f : 1.0f);
        *vals[place_row_] += k == SDLK_RIGHT ? step : -step;
        place_edit_.clear();
    }
    else if (digit && place_row_ < 6 && place_edit_.size() < 16)
    {
        char c = '0';
        if (k >= SDLK_0 && k <= SDLK_9) c = char('0' + (k - SDLK_0));
        else if (k >= SDLK_KP_1 && k <= SDLK_KP_9) c = char('1' + (k - SDLK_KP_1));
        else if (k == SDLK_KP_0) c = '0';
        else if (k == SDLK_PERIOD || k == SDLK_KP_PERIOD) c = '.';
        else c = '-';
        place_edit_ += c;
    }
    else if (k == SDLK_BACKSPACE && !place_edit_.empty()) place_edit_.pop_back();
    else if (k == SDLK_RETURN || k == SDLK_KP_ENTER)
    {
        if (!place_edit_.empty() && place_row_ < 6)
        {
            try
            {
                const float v = std::stof(place_edit_);
                *vals[place_row_] = place_row_ < 3 ? v : v * deg;
            }
            catch (...)
            {
                note_ = tr("pworld.place.bad");
            }
            place_edit_.clear();
            return;
        }
        place_held(); // (where its numbers put it)
    }
    else if (Keys::get().is(pk::kDrop, k, mod)) drop_held();
}

void PersonalWorld::press(SDL_Keycode k, SDL_Keymod mod)
{
    if (k == SDLK_ESCAPE) escape();
    else if (pause_open_) pause_key(k);
    else key(k, mod);
}

// ---- the pause menu (pause_rows.hpp: the same rows as the hallway's, less its tools unless asked for)

void PersonalWorld::escape()
{
    if (pause_open_)
    {
        pause_open_ = false;
        SDL_SetWindowRelativeMouseMode(window_, true);
        return;
    }
    if (cat_panel_.open || place_panel_.open)
    {
        cat_panel_.open = place_panel_.open = false;
        return;
    }
    if (held_ >= 0)
    {
        put_back_held(); // (put back where it was)
        return;
    }
    pause_rows_ = pause_rows(PausePlace::Personal, hallway_tools_);
    pause_open_ = true;
    pause_row_ = 0;
    SDL_SetWindowRelativeMouseMode(window_, false);
}

bool PersonalWorld::pause_select(const std::string& id)
{
    if (!pause_open_) return false;
    for (size_t i = 0; i < pause_rows_.size(); ++i)
        if (pause_rows_[i] == id)
        {
            pause_row_ = int(i);
            return true;
        }
    return false;
}

void PersonalWorld::pause_key(SDL_Keycode k)
{
    const int n = int(pause_rows_.size());
    if (n == 0) return;
    if (k == SDLK_UP || k == SDLK_W) pause_row_ = (pause_row_ + n - 1) % n;
    else if (k == SDLK_DOWN || k == SDLK_S) pause_row_ = (pause_row_ + 1) % n;
    else if (k == SDLK_RETURN || k == SDLK_KP_ENTER || k == SDLK_SPACE) pause_choose(pause_row_);
}

void PersonalWorld::pause_choose(int row)
{
    if (row < 0 || row >= int(pause_rows_.size())) return;
    const std::string& id = pause_rows_[size_t(row)];
    if (id == "resume") escape();
    else if (id == "leave") leave_ = true;
    else if (id == "exit") quit_ = true;
    else system_ = id; // (opened by whoever ran this world, which comes back to it where it can)
}

void PersonalWorld::draw_pause()
{
    const SDL_Color ink{255, 255, 255, 255}, dim{170, 170, 180, 255};
    SDL_SetRenderDrawBlendMode(r_, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r_, 0, 0, 0, 170);
    const SDL_FRect all{0, 0, W_, H_};
    SDL_RenderFillRect(r_, &all);
    const float rowh = std::min(32.0f, (H_ - 140) / float(std::max<size_t>(1, pause_rows_.size())));
    const float bw = 460, bh = 70 + rowh * float(pause_rows_.size()) + 50, bx = (W_ - bw) / 2, by = (H_ - bh) / 2;
    SDL_SetRenderDrawColor(r_, 20, 30, 40, 235);
    const SDL_FRect box{bx, by, bw, bh};
    SDL_RenderFillRect(r_, &box);
    SDL_SetRenderDrawColor(r_, 255, 255, 255, 255);
    SDL_RenderRect(r_, &box);
    const std::string title = tr("pause.title");
    draw_text(r_, bx + (bw - text_width(title, 3)) / 2, by + 18, title, 3, ink);
    pause_rects_.clear();
    float y = by + 70;
    for (size_t i = 0; i < pause_rows_.size(); ++i)
    {
        const std::string label = tr_or("pause." + pause_rows_[i] + ".personal", tr("pause." + pause_rows_[i]));
        const SDL_FRect r{bx + 20, y - 4, bw - 40, rowh - 4};
        pause_rects_.push_back(r);
        if (int(i) == pause_row_)
        {
            SDL_SetRenderDrawColor(r_, 255, 255, 255, 50);
            SDL_RenderFillRect(r_, &r);
        }
        draw_text(r_, bx + (bw - text_width(label, 2)) / 2, y, label, 2, int(i) == pause_row_ ? ink : dim);
        y += rowh;
    }
    const std::string& id = pause_rows_[size_t(pause_row_)];
    std::string help = tr_or("pause.help." + id + ".personal", tr("pause.help." + id)); // (its own words here, where it has them)
    if (is_hallway_tool(id)) help = trf("pause.help.hallway_tool", {help});
    // (on two lines where one is too narrow, broken at a space)
    const size_t fits = size_t(std::max(8.0f, (bw - 30) / 8));
    std::string second;
    if (help.size() > fits)
    {
        const size_t cut = help.rfind(' ', fits);
        second = help.substr(cut == std::string::npos ? fits : cut + 1);
        help = help.substr(0, cut == std::string::npos ? fits : cut);
        if (second.size() > fits) second = second.substr(0, fits - 3) + "...";
    }
    draw_text(r_, bx + (bw - text_width(help, 1)) / 2, by + bh - 38, help, 1, dim);
    if (!second.empty()) draw_text(r_, bx + (bw - text_width(second, 1)) / 2, by + bh - 24, second, 1, dim);
}

void PersonalWorld::place_held()
{
    Placed& p = placed_[size_t(held_)];
    p.physics = false, p.resting = true;
    note_ = trf("pworld.placed_here", {label_of(p.kind)});
    held_ = -1;
    place_panel_.open = false;
    save();
}

void PersonalWorld::drop_held()
{
    Placed& p = placed_[size_t(held_)];
    p.physics = true, p.resting = false, p.vx = p.vy = p.vz = 0;
    note_ = trf("pworld.dropped", {label_of(p.kind)});
    held_ = -1;
    place_panel_.open = false;
    save();
}

void PersonalWorld::put_back_held()
{
    placed_[size_t(held_)] = held_from_;
    held_ = -1;
    place_panel_.open = false;
    note_.clear();
}

// Every key here is asked of the registry (keys.hpp) by what it does, never by its key code, so each
// can be changed on Settings > Controls > Keys and is listed there under its section.
void PersonalWorld::key(SDL_Keycode k, SDL_Keymod mod)
{
    const Keys& K = Keys::get();
    if (K.is(pk::kLookUp, k, mod) || K.is(pk::kLookDown, k, mod))
    {
        pitch_ = std::clamp(pitch_ + (K.is(pk::kLookUp, k, mod) ? 1.0f : -1.0f) * 5.0f * kPi / 180.0f, -1.3f, 1.3f);
        return;
    }
    if (show_map_ && (K.is(pk::kZoomIn, k, mod) || K.is(pk::kZoomOut, k, mod)) && !(place_panel_.open && held_ >= 0))
    {
        map_zoom_ = std::clamp(map_zoom_ + (K.is(pk::kZoomIn, k, mod) ? 1 : -1), 0, kMapZooms - 1);
        return;
    }
    if (K.is(held_ >= 0 ? pk::kExact : pk::kCatalogue, k, mod))
    {
        // One panel at a time: Place while something is in your hand, else the catalogue.
        if (held_ >= 0) place_panel_.toggle(), cat_panel_.open = false, place_edit_.clear();
        else cat_panel_.toggle(), place_panel_.open = false;
        return;
    }
    if (place_panel_.open && held_ >= 0)
    {
        place_key(k, mod);
        return;
    }

    const int n = int(rows_.size());
    if (held_ >= 0)
    {
        // In your hand: turned 15 degrees a press, then placed where it is or dropped; put back.
        Placed& p = placed_[size_t(held_)];
        const float step = kPi / 12;
        if (K.is(pk::kTurn, k, mod)) p.yaw += step;
        else if (K.is(pk::kTurnBack, k, mod)) p.yaw -= step;
        else if (K.is(pk::kTip, k, mod)) p.pitch += step;
        else if (K.is(pk::kTipBack, k, mod)) p.pitch -= step;
        else if (K.is(pk::kRoll, k, mod)) p.roll += step;
        else if (K.is(pk::kRollBack, k, mod)) p.roll -= step;
        else if (K.is(pk::kPlaceHere, k, mod)) place_held();
        else if (K.is(pk::kDrop, k, mod)) drop_held();
        else if (K.is(pk::kPutBack, k, mod)) put_back_held();
        return;
    }
    if (!cat_panel_.open && (K.is(pk::kPickUp, k, mod) || K.is(pk::kOpen, k, mod)))
    {
        const int a = aimed();
        if (a < 0) return;
        if (K.is(pk::kPickUp, k, mod) && author_)
        {
            held_ = a;
            held_from_ = placed_[size_t(a)];
            note_.clear();
        }
        else if (placed_[size_t(a)].kind.rfind("fav:", 0) == 0 && favourite(placed_[size_t(a)].kind.substr(4)))
            opened_ = placed_[size_t(a)].kind.substr(4); // (read on its own line: run() goes there)
        else note_ = trf("pworld.open", {label_of(placed_[size_t(a)].kind)}); // (a visitor's E too)
        return;
    }
    if (cat_panel_.open && K.is(pk::kChooseUp, k, mod)) catalogue_row_ = (catalogue_row_ + n - 1) % n;
    else if (cat_panel_.open && K.is(pk::kChooseDown, k, mod)) catalogue_row_ = (catalogue_row_ + 1) % n;
    else if (cat_panel_.open && K.is(pk::kPlace, k, mod))
    {
        // Before you, on the ground, turned to face you.
        Placed p;
        p.kind = rows_[size_t(catalogue_row_)];
        p.x = px_ + kPlaceAhead * std::sin(yaw_), p.z = pz_ + kPlaceAhead * std::cos(yaw_), p.yaw = yaw_ + kPi;
        p.y = ground(p.x, p.z);
        placed_.push_back(p);
        note_ = trf("pworld.placed", {label_of(p.kind), std::to_string(placed_.size())});
        save();
    }
    else if (K.is(pk::kTakeAway, k, mod))
    {
        // The one nearest you, within eight metres, taken away.
        size_t best = placed_.size();
        float bd = 64;
        for (size_t i = 0; i < placed_.size(); ++i)
        {
            const float d = (placed_[i].x - px_) * (placed_[i].x - px_) + (placed_[i].z - pz_) * (placed_[i].z - pz_);
            if (d < bd) bd = d, best = i;
        }
        if (best == placed_.size()) note_ = tr("pworld.none_near");
        else
        {
            note_ = trf("pworld.removed", {label_of(placed_[best].kind)});
            placed_.erase(placed_.begin() + long(best));
            if (held_ > int(best)) --held_;
            save();
        }
    }
}

namespace {
std::string g9(double v)
{
    char b[40];
    std::snprintf(b, sizeof b, "%.9g", v);
    return b;
}
} // namespace

void PersonalWorld::save()
{
    if (file_.empty()) return;
    // v1 while everything stands on the ground, turned only about the vertical and placed; v2 (the
    // 3D place, three turns, placed or dropped) as soon as anything is more, or the file held lines
    // this build keeps without reading.
    bool v2 = !kept_lines_.empty();
    for (const Placed& p : placed_) v2 = v2 || p.kind.rfind("fav:", 0) == 0 || p.physics || p.pitch != 0 || p.roll != 0 || std::fabs(p.y - ground(p.x, p.z)) > 1e-4f;
    std::error_code ec;
    std::filesystem::create_directories(file_.parent_path(), ec);
    std::ofstream out(file_, std::ios::binary);
    out << (v2 ? "sieve-personal-world-v2" : "sieve-personal-world-v1") << "\nseed " << seed_ << "\n";
    for (const std::string& l : kept_lines_) out << l << "\n";
    for (const Placed& p : placed_)
        if (v2)
            out << "place " << p.kind << " " << g9(p.x) << " " << g9(p.y) << " " << g9(p.z) << " " << g9(p.yaw) << " " << g9(p.pitch) << " " << g9(p.roll) << " "
                << (p.physics ? "physics" : "fixed") << "\n";
        else out << "place " << p.kind << " " << g9(p.x) << " " << g9(p.z) << " " << g9(p.yaw) << "\n";
    if (!out) note_ = trf("pworld.unwritable", {file_.string()});
}

void PersonalWorld::load()
{
    if (file_.empty()) return;
    std::ifstream in(file_, std::ios::binary);
    if (!in) return;
    std::string l;
    size_t skipped = 0, line = 0;
    int version = 0;
    while (std::getline(in, l))
    {
        if (!l.empty() && l.back() == '\r') l.pop_back();
        ++line;
        if (line == 1)
        {
            version = l == "sieve-personal-world-v1" ? 1 : l == "sieve-personal-world-v2" ? 2 : 0;
            if (!version) return; // not one: left as it is, nothing read
            continue;
        }
        std::istringstream ss(l);
        std::string word, how;
        Placed p;
        if (!(ss >> word)) continue;
        if (word == "seed")
        {
            uint64_t v = 0;
            if (ss >> v) seed_ = v;
            continue;
        }
        const auto& cat = catalogue();
        if (word != "place")
        {
            // v2: kept as it is and written back, so a later build's lines are never lost here.
            if (version == 2) kept_lines_.push_back(l);
            else ++skipped;
            continue;
        }
        bool ok = false;
        if (version == 1)
        {
            ok = bool(ss >> p.kind >> p.x >> p.z >> p.yaw);
            p.y = std::nanf(""); // (on the ground, once the ground is made)
        }
        else
        {
            ok = bool(ss >> p.kind >> p.x >> p.y >> p.z >> p.yaw >> p.pitch >> p.roll >> how) && (how == "fixed" || how == "physics");
            p.physics = how == "physics";
            p.resting = !p.physics; // (a dropped one settles from where it was written)
        }
        if (ok && (std::find(cat.begin(), cat.end(), p.kind) != cat.end() || (version == 2 && p.kind.rfind("fav:", 0) == 0 && p.kind.size() == 68))) placed_.push_back(p);
        else ++skipped;
    }
    if (skipped) note_ = trf("pworld.skipped", {std::to_string(skipped)});
}

std::string PersonalWorld::report() const
{
    char b[160];
    std::snprintf(b, sizeof b, "seed %llu; you at %.1f, %.1f, ground %.2f m; %zu characters; %zu objects placed", (unsigned long long)seed_, double(px_), double(pz_),
                  double(height(px_, pz_)), characters_.size(), placed_.size());
    std::string t = b;
    for (size_t i = 0; i < placed_.size(); ++i)
        if (int(i) == held_ || placed_[i].physics || std::fabs(placed_[i].y - ground(placed_[i].x, placed_[i].z)) > 1e-3f)
        {
            // (each one held, dropped or off the ground: where it is)
            char o[160];
            std::snprintf(o, sizeof o, "; %s %s at %.2f, %.2f, %.2f%s", placed_[i].kind.c_str(), int(i) == held_ ? "held" : placed_[i].physics ? "dropped" : "placed",
                          double(placed_[i].x), double(placed_[i].y - ground(placed_[i].x, placed_[i].z)), double(placed_[i].z),
                          placed_[i].physics && !placed_[i].resting ? " (moving)" : "");
            t += o;
        }
    return t;
}

PersonalWorld::Result PersonalWorld::run()
{
    SDL_SetWindowRelativeMouseMode(window_, true);
    music_mode(MusicMode::World);
    Uint64 last = SDL_GetTicksNS();
    system_.clear();
    for (;;)
    {
        SDL_Event e;
        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_EVENT_QUIT)
            {
                save();
                return Result::Quit;
            }
            if (e.type == SDL_EVENT_KEY_DOWN && e.key.key == SDLK_ESCAPE)
            {
                escape();
                continue;
            }
            if (pause_open_)
            {
                // (the pause menu takes the keys and the mouse while it is in)
                if (e.type == SDL_EVENT_KEY_DOWN && !e.key.repeat) pause_key(e.key.key);
                if (e.type == SDL_EVENT_MOUSE_MOTION || e.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
                {
                    float x = e.type == SDL_EVENT_MOUSE_MOTION ? e.motion.x : e.button.x, y = e.type == SDL_EVENT_MOUSE_MOTION ? e.motion.y : e.button.y;
                    SDL_RenderCoordinatesFromWindow(r_, x, y, &x, &y);
                    for (size_t i = 0; i < pause_rects_.size(); ++i)
                    {
                        const SDL_FRect& r = pause_rects_[i];
                        if (x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h)
                        {
                            pause_row_ = int(i);
                            if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN && e.button.button == SDL_BUTTON_LEFT) pause_choose(int(i));
                        }
                    }
                }
                if (leave_ || quit_ || !system_.empty())
                {
                    save();
                    pause_open_ = false;
                    SDL_SetWindowRelativeMouseMode(window_, false);
                    const Result r = quit_ ? Result::Quit : leave_ ? Result::Back : Result::System;
                    leave_ = quit_ = false; // (a system comes back to this world: its pause menu closed)
                    return r;
                }
                continue;
            }
            if (e.type == SDL_EVENT_KEY_DOWN && !e.key.repeat) key(e.key.key, e.key.mod);
            if (!opened_.empty())
            {
                save();
                SDL_SetWindowRelativeMouseMode(window_, false);
                return Result::Open;
            }
            if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN && e.button.button == SDL_BUTTON_LEFT)
            {
                // (a click: pick up what the crosshair is on, or place what you hold)
                if (held_ >= 0) place_held();
                else if (const int a = aimed(); a >= 0 && author_) held_ = a, held_from_ = placed_[size_t(a)], note_.clear();
            }
            if (e.type == SDL_EVENT_MOUSE_MOTION)
            {
                yaw_ += e.motion.xrel * 0.0025f;
                pitch_ = std::clamp(pitch_ - e.motion.yrel * 0.0025f, -1.3f, 1.3f);
            }
        }
        const Uint64 now = SDL_GetTicksNS();
        const float dt = std::min(0.1f, float(double(now - last) / 1e9));
        last = now;
        if (!pause_open_) step(dt, SDL_GetKeyboardState(nullptr)); // (paused: all still)
        render();
        present(r_);
    }
}

} // namespace hallway
