// Sieve hallway — your personal dimension as a world of its own (see personal_world.hpp).
#include "personal_world.hpp"

#include "font.hpp"
#include "mesh.hpp"
#include "music.hpp"
#include "strings.hpp"

#include "cli/sieve_cache.hpp"
#include "cli/dictionaries.hpp"
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

PersonalWorld::PersonalWorld(SDL_Window* window, SDL_Renderer* renderer, std::vector<sieve::Character> characters, std::filesystem::path world_file, uint64_t seed)
    : window_(window), r_(renderer), characters_(std::move(characters)), file_(std::move(world_file)), seed_(seed)
{
    load(); // (what was placed before, and the world's seed)
    // The hill: one broad swell near the middle, a few smaller ones about it, all from the seed.
    uint64_t s = seed ^ 0x5049455253ull; // ("PIERS": the world's own stream)
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
    const float speed = (keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT]) ? 12.0f : 4.0f;
    float fx = 0, fz = 0;
    if (keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_UP]) fz += 1;
    if (keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_DOWN]) fz -= 1;
    if (keys[SDL_SCANCODE_D]) fx += 1;
    if (keys[SDL_SCANCODE_A]) fx -= 1;
    if (keys[SDL_SCANCODE_LEFT]) yaw_ -= 1.6f * dt;
    if (keys[SDL_SCANCODE_RIGHT]) yaw_ += 1.6f * dt;
    if (fx != 0 || fz != 0)
    {
        const float n = std::sqrt(fx * fx + fz * fz);
        fx /= n, fz /= n;
        px_ += (fz * std::sin(yaw_) + fx * std::cos(yaw_)) * speed * dt;
        pz_ += (fz * std::cos(yaw_) - fx * std::sin(yaw_)) * speed * dt;
        px_ = std::clamp(px_, -kHalf + 5, kHalf - 5);
        pz_ = std::clamp(pz_, -kHalf + 5, kHalf - 5);
    }
    wind_ += dt * 2.0f;
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
    SDL_SetRenderDrawBlendMode(r_, SDL_BLENDMODE_BLEND);
    // The sky: deep blue overhead to pale at the horizon (where a level line meets the screen).
    const float f = (H_ / 2) / std::tan(kFov / 2), horizon = H_ / 2 + std::tan(pitch_) * f;
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
            float lift; // (the grass's lighter crests: its height adds this much light a metre)
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
                world.push_back({{Pt{x0, h00, z0}, Pt{x1, h10, z0}, Pt{x1, h11, z1}}, g, 0.10f / 30.0f});
                world.push_back({{Pt{x0, h00, z0}, Pt{x1, h11, z1}, Pt{x0, h01, z1}}, g, 0.10f / 30.0f});
            }
        // The objects after the ground, all of them: a small thing near a large cell of grass would
        // otherwise be painted over by it (the cell's middle can be nearer than the thing). One
        // hidden behind a hill is left out, by a line of sight to its top over the height field.
        const size_t ground_tris = world.size(); // (the ground's; the rest are objects')
        {
            std::vector<std::pair<std::array<Pt, 3>, SDL_FColor>> faces;
            const float eye_y = height(px_, pz_) + kEye;
            for (const Placed& p : placed_)
            {
                const float g = ground(p.x, p.z);
                bool seen = true;
                const float dx = p.x - px_, dz = p.z - pz_, d = std::sqrt(dx * dx + dz * dz);
                for (float u = 2.0f / std::max(d, 2.0f); u < 1.0f && seen; u += 2.0f / std::max(d, 2.0f))
                    seen = ground(px_ + dx * u, pz_ + dz * u) < eye_y + (g + 0.5f - eye_y) * u;
                if (seen) faces_of(p, g, faces);
            }
            for (const auto& [t, c] : faces) world.push_back({t, c, 0});
        }
        struct Tri
        {
            float depth;
            std::array<SDL_FPoint, 6> p;
            int count;
            SDL_FColor c;
        };
        std::vector<Tri> tris;
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
            if (ny < 0) nx = -nx, ny = -ny, nz = -nz;
            const float lit = std::max(0.0f, (nx * sun[0] + ny * sun[1] + nz * sun[2]) / nl);
            const float cxw = (t[0][0] + t[1][0] + t[2][0]) / 3, cyw = (t[0][1] + t[1][1] + t[2][1]) / 3, czw = (t[0][2] + t[1][2] + t[2][2]) / 3;
            const float shade = 0.55f + 0.45f * lit + w.lift * cyw;
            const float dist = std::sqrt((cxw - px_) * (cxw - px_) + (czw - pz_) * (czw - pz_));
            SDL_FColor c{std::clamp(w.base.r * shade, 0.0f, 1.0f), std::clamp(w.base.g * shade, 0.0f, 1.0f), std::clamp(w.base.b * shade, 0.0f, 1.0f), 1};
            c = mix(c, kSkyLow, std::pow(std::clamp(dist / 700.0f, 0.0f, 1.0f), 1.5f));
            // Cut against the near plane (Sutherland-Hodgman), then onto the screen.
            std::array<Pt, 3> in{};
            std::array<Pt, 6> outp{};
            int no = 0;
            for (int k = 0; k < 3; ++k) in[size_t(k)] = cam(t[size_t(k)]);
            for (int k = 0; k < 3; ++k)
            {
                const Pt& a = in[size_t(k)];
                const Pt& b = in[size_t((k + 1) % 3)];
                const bool ain = a[2] >= kNear, bin = b[2] >= kNear;
                if (ain) outp[size_t(no++)] = a;
                if (ain != bin)
                {
                    const float u = (kNear - a[2]) / (b[2] - a[2]);
                    outp[size_t(no++)] = {a[0] + (b[0] - a[0]) * u, a[1] + (b[1] - a[1]) * u, kNear};
                }
            }
            if (no < 3) continue;
            Tri tr{};
            tr.count = no;
            tr.c = c;
            float sum = 0;
            for (int k = 0; k < no; ++k)
            {
                const Pt& q = outp[size_t(k)];
                tr.p[size_t(k)] = {W_ / 2 + q[0] / q[2] * f, H_ / 2 - q[1] / q[2] * f};
                sum += q[2];
            }
            tr.depth = sum / float(no) - (size_t(&w - world.data()) >= ground_tris ? 1e6f : 0.0f); // (objects after the ground)
            tris.push_back(tr);
        }
        std::sort(tris.begin(), tris.end(), [](const Tri& a, const Tri& b) { return a.depth > b.depth; });
        std::vector<SDL_Vertex> v;
        v.reserve(tris.size() * 12);
        for (const Tri& t : tris)
            for (int k = 1; k + 1 < t.count; ++k)
                for (const int q : {0, k, k + 1}) v.push_back({t.p[size_t(q)], t.c, {}});
        SDL_RenderGeometry(r_, nullptr, v.data(), int(v.size()), nullptr, 0);
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
            SDL_SetRenderDrawColor(r_, 60, 60, 70, 255);
            for (const auto& s : c.spheres)
                if (s.parent >= 0)
                {
                    float x1, y1, d1, x2, y2, d2;
                    if (at(s, x1, y1, d1) && at(c.spheres[size_t(s.parent)], x2, y2, d2)) SDL_RenderLine(r_, x1, y1, x2, y2);
                }
            for (const auto& s : c.spheres)
            {
                float sx, sy, d;
                if (!at(s, sx, sy, d)) continue;
                const float rad = std::max(1.5f, float(s.r) * unitm * f / d);
                const SDL_FColor col = ci == 0 ? SDL_FColor{0.95f, 0.92f, 0.85f, 1} : SDL_FColor{0.80f, 0.82f, 0.90f, 1};
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
    draw_text(r_, 20, 16, tr("pworld.title"), 2, white);
    if (catalogue_open_)
    {
        // The catalogue: a list on a dark plate at the left, the one chosen marked.
        const auto& cat = catalogue();
        const SDL_FRect plate{12, 48, 260, 34 + float(cat.size()) * 18};
        SDL_SetRenderDrawColor(r_, 0, 0, 0, 170);
        SDL_RenderFillRect(r_, &plate);
        draw_text(r_, 22, 56, tr("pworld.catalogue"), 1, white);
        for (size_t i = 0; i < cat.size(); ++i)
            draw_text(r_, 22, 76 + float(i) * 18, (int(i) == catalogue_row_ ? "> " : "  ") + tr("pworld.object." + cat[i]), 1, int(i) == catalogue_row_ ? white : grey);
    }
    if (!note_.empty()) draw_text(r_, 20, H_ - 42, note_, 1, white);
    draw_text(r_, 20, H_ - 24, tr(catalogue_open_ ? "pworld.footer.catalogue" : "pworld.footer"), 1, white);
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
    static const std::vector<std::string> kAll = {"pages", "image", "audio", "video", "books", "models", "binary", "tracks", "movies", "worlds", "ai"};
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
            const std::string file = id == "books" ? "book.obj" : "book-" + id + ".obj";
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
            if (m) out.mesh[id] = m;
            else out.note += (out.note.empty() ? "" : ", ") + file;
        }
        return out;
    }();
    return kItems;
}

} // namespace

std::string PersonalWorld::catalogue_note() { return shelf_items().note; }

// An object's faces in the world: its shelf item's triangles, standing on the ground at its place
// (the model's foot, y = 0, on the grass), turned by its yaw. A model's front faces +X; turned so it
// faces back along the yaw it was placed with.
void PersonalWorld::faces_of(const Placed& p, float ground, std::vector<std::pair<std::array<std::array<float, 3>, 3>, SDL_FColor>>& out)
{
    const auto it = shelf_items().mesh.find(p.kind);
    if (it == shelf_items().mesh.end()) return;
    const float c = std::cos(p.yaw), s = std::sin(p.yaw);
    for (const MeshTri& t : it->second->tris)
    {
        std::array<std::array<float, 3>, 3> w{};
        for (int k = 0; k < 3; ++k)
        {
            // (+X turned onto the yaw's own forward, (sin, cos) in x and z)
            const Vec3& q = t.p[k];
            w[size_t(k)] = {p.x + q.x * s + q.z * c, ground + q.y, p.z + q.x * c - q.z * s};
        }
        out.push_back({w, t.kd});
    }
}

void PersonalWorld::press(SDL_Keycode k) { key(k); }

void PersonalWorld::key(SDL_Keycode k)
{
    const int n = int(catalogue().size());
    if (k == SDLK_TAB)
    {
        catalogue_open_ = !catalogue_open_;
        return;
    }
    if (catalogue_open_ && k == SDLK_UP) catalogue_row_ = (catalogue_row_ + n - 1) % n;
    else if (catalogue_open_ && k == SDLK_DOWN) catalogue_row_ = (catalogue_row_ + 1) % n;
    else if (catalogue_open_ && (k == SDLK_RETURN || k == SDLK_KP_ENTER))
    {
        // Four metres before you, on the ground, turned to face you.
        Placed p{catalogue()[size_t(catalogue_row_)], px_ + kPlaceAhead * std::sin(yaw_), pz_ + kPlaceAhead * std::cos(yaw_), yaw_ + kPi};
        placed_.push_back(p);
        note_ = trf("pworld.placed", {tr("pworld.object." + p.kind), std::to_string(placed_.size())});
        save();
    }
    else if (k == SDLK_BACKSPACE)
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
            note_ = trf("pworld.removed", {tr("pworld.object." + placed_[best].kind)});
            placed_.erase(placed_.begin() + long(best));
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
    std::error_code ec;
    std::filesystem::create_directories(file_.parent_path(), ec);
    std::ofstream out(file_, std::ios::binary);
    out << "sieve-personal-world-v1\nseed " << seed_ << "\n";
    for (const Placed& p : placed_) out << "place " << p.kind << " " << g9(p.x) << " " << g9(p.z) << " " << g9(p.yaw) << "\n";
    if (!out) note_ = trf("pworld.unwritable", {file_.string()});
}

void PersonalWorld::load()
{
    if (file_.empty()) return;
    std::ifstream in(file_, std::ios::binary);
    if (!in) return;
    std::string l;
    size_t skipped = 0, line = 0;
    while (std::getline(in, l))
    {
        if (!l.empty() && l.back() == '\r') l.pop_back();
        ++line;
        if (line == 1)
        {
            if (l != "sieve-personal-world-v1") return; // not one: left as it is, nothing read
            continue;
        }
        std::istringstream ss(l);
        std::string word, kind;
        Placed p;
        if (!(ss >> word)) continue;
        if (word == "seed")
        {
            uint64_t v = 0;
            if (ss >> v) seed_ = v;
            continue;
        }
        const auto& cat = catalogue();
        if (word == "place" && (ss >> p.kind >> p.x >> p.z >> p.yaw) && std::find(cat.begin(), cat.end(), p.kind) != cat.end()) placed_.push_back(p);
        else ++skipped;
    }
    if (skipped) note_ = trf("pworld.skipped", {std::to_string(skipped)});
}

std::string PersonalWorld::report() const
{
    char b[160];
    std::snprintf(b, sizeof b, "seed %llu; you at %.1f, %.1f, ground %.2f m; %zu characters; %zu objects placed", (unsigned long long)seed_, double(px_), double(pz_),
                  double(height(px_, pz_)), characters_.size(), placed_.size());
    return b;
}

PersonalWorld::Result PersonalWorld::run()
{
    SDL_SetWindowRelativeMouseMode(window_, true);
    music_mode(MusicMode::World);
    Uint64 last = SDL_GetTicksNS();
    for (;;)
    {
        SDL_Event e;
        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_EVENT_QUIT) return Result::Quit;
            if (e.type == SDL_EVENT_KEY_DOWN && e.key.key == SDLK_ESCAPE)
            {
                if (catalogue_open_)
                {
                    catalogue_open_ = false;
                    continue;
                }
                SDL_SetWindowRelativeMouseMode(window_, false);
                return Result::Back;
            }
            if (e.type == SDL_EVENT_KEY_DOWN && !e.key.repeat) key(e.key.key);
            if (e.type == SDL_EVENT_MOUSE_MOTION)
            {
                yaw_ += e.motion.xrel * 0.0025f;
                pitch_ = std::clamp(pitch_ - e.motion.yrel * 0.0025f, -1.3f, 1.3f);
            }
        }
        const Uint64 now = SDL_GetTicksNS();
        const float dt = std::min(0.1f, float(double(now - last) / 1e9));
        last = now;
        step(dt, SDL_GetKeyboardState(nullptr));
        render();
        present(r_);
    }
}

} // namespace hallway
