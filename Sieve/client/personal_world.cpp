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
#include "sieve/diffusion.hpp"
#include "sieve/importers.hpp"
#include "sieve/paths.hpp"
#include "sieve/sha256.hpp"
#include "sieve/text.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace hallway {

namespace {

constexpr float kPi = 3.14159265358979f;
constexpr float kPlaceAhead = 1.5f; // (metres before you an object is placed: a shelf item is 0.4 m tall)
constexpr float kEye = 1.7f, kFov = 70.0f * kPi / 180.0f, kHalf = 320.0f, kCell = 10.0f, kNear = 0.1f;
// The turning rings' radius (IDEAS 19.221): a little larger than the farthest point of what is held from
// its middle, so a ring is never inside the thing however it is turned (the same for all three: they
// meet as a gimbal's rings do).
float ring_radius(float reach) { return reach * 1.12f + 0.03f; }

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
// (IDEAS 19.222: the panel where the day's length is typed, while day and night is on)
const std::string& kDayLength = declare("personal.day", "length", "L");
// (IDEAS 19.209: the author's tools, the first the ground shaper. Edward asked for 1 to take it out and #
// to put it away; the backslash is the key in #'s place on a US keyboard, so the same key puts it away
// whichever layout the keyboard has. Raise, lower and flatten are the mouse buttons too: left, right and
// middle, which the registry does not hold.)
const std::string& kToolOut = declare("personal.tools", "shaper", "1");
const std::string& kToolAway = declare("personal.tools", "put_away", "#,\\");
const std::string& kRaise = declare("personal.tools", "raise", "Z");
const std::string& kLower = declare("personal.tools", "lower", "X");
const std::string& kFlatten = declare("personal.tools", "flatten", "C");
const std::string& kBrushBigger = declare("personal.tools", "bigger", "]");
const std::string& kBrushSmaller = declare("personal.tools", "smaller", "[");
// (IDEAS 19.187's next step: a model of your own brought in from a file, pressed while the catalogue is
// out. A section of its own, after the others, so the rows already on Settings > Controls > Keys stay
// where they were.)
const std::string& kImport = declare("personal.objects", "import", "I");
// (IDEAS 19.187's "the world itself as an address on the WORLDS line": its panel, where Return goes to it
// in the hallway. Its own section, after the others, as the import key's is, so the rows before it stay.)
const std::string& kWorldsAddress = declare("personal.worlds", "address", "N");
const std::string& kWorldsGo = declare("personal.worlds", "go", "Return,Keypad Enter");
} // namespace pk

namespace {

// A draw in [0, 1) from the world's stream (splitmix64, the one copy diffusion keeps).
float unit(sieve::diffusion::SplitMix& g) { return float(g.next() >> 40) / float(1ull << 24); }

SDL_FColor mix(SDL_FColor a, SDL_FColor b, float t)
{
    t = std::clamp(t, 0.0f, 1.0f);
    return {a.r + (b.r - a.r) * t, a.g + (b.g - a.g) * t, a.b + (b.b - a.b) * t, a.a + (b.a - a.a) * t};
}

const SDL_FColor kSkyTop{0.13f, 0.36f, 0.86f, 1}, kSkyLow{0.72f, 0.85f, 0.98f, 1}, kGrass{0.27f, 0.60f, 0.16f, 1};

} // namespace

PersonalWorld::~PersonalWorld()
{
    batch_.release();
    for (auto& [sha, tex] : covers_) gpu::destroy(tex);
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

// A character's spheres as placed: each centre (its lowest sphere's foot at the place) turned with it,
// and its radius, in metres.
template <class F>
static void each_sphere(const sieve::Character& c, float yaw, float pitch, float roll, F&& f)
{
    int32_t low, high;
    c.vertical(low, high);
    const float m = 1.0f / float(sieve::kCharacterUnitsPerMetre);
    for (const auto& s : c.spheres)
    {
        const float q[3] = {float(s.x) * m, float(s.y - low) * m, float(s.z) * m};
        f(s, turned(q, yaw, pitch, roll), float(s.r) * m);
    }
}

PersonalWorld::PersonalWorld(SDL_Window* window, SDL_Renderer* renderer, std::vector<sieve::Character> characters, bool first_is_yours, std::filesystem::path world_file, uint64_t seed)
    : file_(std::move(world_file)), window_(window), r_(renderer), seed_(seed)
{
    // (your characters, each once: a store's copy of yours is yours)
    first_is_yours_ = first_is_yours && !characters.empty();
    for (auto& c : characters)
    {
        std::string id = c.id();
        if (std::find(char_ids_.begin(), char_ids_.end(), id) != char_ids_.end()) continue;
        characters_.push_back(std::move(c));
        char_ids_.push_back(std::move(id));
    }
    rows_ = catalogue();
    if (!file_.empty())
        for (const std::string& sha : dimension_favourites(favourites_list_of(file_)))
            if (auto f = read_favourite(file_.parent_path().parent_path(), sha))
            {
                favourites_.push_back(*f);
                rows_.push_back("fav:" + sha);
            }
    // Your characters after them: yours, then your store's.
    for (size_t ci = 0; ci < characters_.size(); ++ci) rows_.push_back(ci == 0 && first_is_yours_ ? "char:you" : "char:" + char_ids_[ci]);
    load(); // (what was placed before, and the world's seed)
    // The hill: one broad swell near the middle, a few smaller ones about it, all from the seed (the
    // world file's, where it has one).
    sieve::diffusion::SplitMix s(seed_ ^ 0x5049455253ull); // ("PIERS": the world's own stream)
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
    // The stars (IDEAS 19.222), from a stream of their own so the hill and the clouds stay as they were:
    // points spread evenly over the sphere (a height and an angle each, uniform), seen only at night.
    {
        sieve::diffusion::SplitMix st(seed_ ^ 0x5354415253ull); // ("STARS")
        for (int i = 0; i < 400; ++i)
        {
            const float y = unit(st) * 2 - 1, a = unit(st) * 2 * kPi, rr = std::sqrt(std::max(0.0f, 1 - y * y));
            stars_.push_back({rr * std::cos(a), y, rr * std::sin(a)});
        }
    }
    // The hill at every cell corner, and each cell's shade, once (the same expressions render() and
    // ground() used each time before).
    {
        const int n = int(2 * kHalf / kCell);
        corner_h_.resize(size_t(n + 1) * size_t(n + 1));
        // (a shaped world's changes, read by load(), added at each corner: IDEAS 19.209)
        for (int i = 0; i <= n; ++i)
            for (int j = 0; j <= n; ++j)
            {
                const size_t k = size_t(i) * size_t(n + 1) + size_t(j);
                corner_h_[k] = seed_height(-kHalf + float(i) * kCell, -kHalf + float(j) * kCell);
                if (!shaped_.empty()) corner_h_[k] += shaped_[k];
            }
        cell_vary_.resize(size_t(n) * size_t(n));
        for (int i = 0; i < n; ++i)
            for (int j = 0; j < n; ++j)
            {
                sieve::diffusion::SplitMix s(seed_ ^ (uint64_t(uint32_t(i)) << 32 | uint32_t(j)));
                cell_vary_[size_t(i) * size_t(n) + size_t(j)] = 1.0f + (unit(s) - 0.5f) * 0.10f;
            }
    }
    // Your characters where they have always stood, until the world says where they are: yours a few
    // steps ahead of where you begin, those in your store in a ring round the top of the hill.
    for (size_t ci = 0; ci < characters_.size(); ++ci)
    {
        Placed p;
        p.kind = rows_[rows_.size() - characters_.size() + ci];
        if (ci == 0 && first_is_yours_) p.x = 0, p.z = -52;
        else
        {
            const size_t k = ci - (first_is_yours_ ? 1 : 0), of = characters_.size() - (first_is_yours_ ? 1 : 0);
            const float a = 2 * kPi * float(k) / float(std::max<size_t>(1, of));
            p.x = swells_.front().x + 14 * std::cos(a), p.z = swells_.front().z + 14 * std::sin(a);
        }
        p.y = ground(p.x, p.z);
        default_chars_.push_back(p);
    }
    if (!characters_placed_) placed_.insert(placed_.end(), default_chars_.begin(), default_chars_.end());
    // (a v1 world's objects stand on the ground, which is known only now)
    for (Placed& p : placed_)
        if (std::isnan(p.y)) p.y = ground(p.x, p.z);
    // Your own models last, as the world names them (after the characters, whose defaults above are
    // found by counting back from the end of the rows as they stood before these).
    for (const std::string& sha : models_) rows_.push_back("model:" + sha);
}

// The ground as drawn: the height of the cell's triangle there (each 10 m cell two flat triangles
// between its corners' heights), so what stands on it stands on the grass you see.
float PersonalWorld::ground(float x, float z) const
{
    const float fi = std::floor((x + kHalf) / kCell), fj = std::floor((z + kHalf) / kCell);
    const float x0 = -kHalf + fi * kCell, z0 = -kHalf + fj * kCell, u = (x - x0) / kCell, v = (z - z0) / kCell;
    const int n = int(2 * kHalf / kCell);
    float h00, h10, h01, h11;
    if (fi >= 0 && fj >= 0 && fi < float(n) && fj < float(n) && !corner_h_.empty())
    {
        // (the corners worked out once; the same values height() gives there)
        const size_t i = size_t(fi), j = size_t(fj), w = size_t(n + 1);
        h00 = corner_h_[i * w + j], h10 = corner_h_[(i + 1) * w + j], h01 = corner_h_[i * w + j + 1], h11 = corner_h_[(i + 1) * w + j + 1];
    }
    else h00 = height(x0, z0), h10 = height(x0 + kCell, z0), h01 = height(x0, z0 + kCell), h11 = height(x0 + kCell, z0 + kCell);
    // (split from (x0, z0) to (x1, z1): below the diagonal, the triangle 00-10-11; above it 00-11-01)
    return u >= v ? h00 + (h10 - h00) * u + (h11 - h10) * v : h00 + (h11 - h01) * u + (h01 - h00) * v;
}

float PersonalWorld::height(float x, float z) const
{
    // (nothing added where nothing is shaped, so such a world's every height is what it always was)
    return shaped_.empty() ? seed_height(x, z) : seed_height(x, z) + shaped_at(x, z);
}

// What the shaper changed at (x, z): the corners' changes across the cell's two triangles, split as the
// ground is (ground()), so you stand on the shaped grass you see. Nothing outside the grid.
float PersonalWorld::shaped_at(float x, float z) const
{
    const int n = int(2 * kHalf / kCell);
    if (shaped_.empty() || !(x >= -kHalf && x <= kHalf && z >= -kHalf && z <= kHalf)) return 0;
    const int i = std::min(n - 1, int(std::floor((x + kHalf) / kCell))), j = std::min(n - 1, int(std::floor((z + kHalf) / kCell)));
    const float u = (x - (-kHalf + float(i) * kCell)) / kCell, v = (z - (-kHalf + float(j) * kCell)) / kCell;
    const size_t w = size_t(n + 1);
    const float d00 = shaped_[size_t(i) * w + size_t(j)], d10 = shaped_[size_t(i + 1) * w + size_t(j)], d01 = shaped_[size_t(i) * w + size_t(j + 1)],
                d11 = shaped_[size_t(i + 1) * w + size_t(j + 1)];
    return u >= v ? d00 + (d10 - d00) * u + (d11 - d10) * v : d00 + (d11 - d01) * u + (d01 - d00) * v;
}

size_t PersonalWorld::shaped_corners() const
{
    size_t c = 0;
    for (const float d : shaped_) c += d != 0 ? 1 : 0;
    return c;
}

float PersonalWorld::seed_height(float x, float z) const
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
    const Vec3 c = eye_space(x, y, z);
    depth = c.z;
    if (c.z < kNear) return false;
    const Point2 p = eye_camera().project_camera(c);
    sx = p.x, sy = p.y;
    return true;
}

// A point of the world as your eye sees it (x right, y up, z ahead), turned by your yaw then your
// pitch. Written out here rather than taken from Camera::to_camera: the same for every look up to
// straight up or down, but past them (--pworld-look 90 and more) Camera's right and up change sign
// and the ground and the clouds would be drawn turned over (review 2, 14 November 2026).
Vec3 PersonalWorld::eye_space(float x, float y, float z) const
{
    const float cy = std::cos(yaw_), sy0 = std::sin(yaw_), cp = std::cos(pitch_), sp = std::sin(pitch_);
    const float dx = x - px_, dy = y - (height(px_, pz_) + kEye), dz = z - pz_;
    const float x1 = dx * cy - dz * sy0, z1 = dx * sy0 + dz * cy;
    return {x1, dy * cp - z1 * sp, dy * sp + z1 * cp};
}

Camera PersonalWorld::eye_camera() const
{
    Camera c;
    c.pos = {px_, height(px_, pz_) + kEye, pz_};
    c.yaw = yaw_;
    c.pitch = pitch_;
    c.fov_y = kFov;
    c.near_z = kNear;
    c.update(int(W_), int(H_));
    return c;
}

void PersonalWorld::step(float dt, const bool* keys)
{
    const Keys& K = Keys::get();
    const bool menu = cat_panel_.open || (place_panel_.open && held_ >= 0) || day_panel_.open || worlds_panel_.open; // (a panel's own keys are not walked with)
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
    // The ground shaper (IDEAS 19.209), while it is out and nothing is in your hand or open: a mouse
    // button held, else a key held, shapes the ground where the crosshair meets it, this frame's share;
    // let go, what was shaped is saved (once a stroke, not every frame).
    if (tool_out_ && held_ < 0 && !menu && author_)
    {
        const int by_key = K.held(pk::kRaise, keys) ? 1 : K.held(pk::kLower, keys) ? 2 : K.held(pk::kFlatten, keys) ? 3 : 0;
        // (a key still down from what was open over the world, Z that closed the viewer, shapes nothing
        // until it has been let go)
        if (!by_key) shape_keys_wait_ = false;
        const int mode = mouse_shape_ ? mouse_shape_ : shape_keys_wait_ ? 0 : by_key;
        // The brush stays where the stroke began while you neither move nor look elsewhere: aimed afresh
        // each frame, the ground it raises would rise into the crosshair's line and draw the brush
        // towards you (and a hollow it lowers, away), so holding still would not shape one place.
        const std::array<float, 4> view{px_, pz_, yaw_, pitch_};
        if (mode && (!stroke_on_ || view != stroke_view_)) stroke_on_ = aim_ground(stroke_x_, stroke_z_), stroke_view_ = view;
        if (mode && stroke_on_) shape(mode, dt, stroke_x_, stroke_z_);
        if (!mode) stroke_on_ = false;
        if (!mode && shaped_unsaved_) shaped_unsaved_ = false, save();
    }
    if (held_ >= 0 && !place_panel_.open)
    {
        // In your hand: two metres along the crosshair, never under the grass (with Place open, where
        // its numbers put it instead).
        Placed& p = placed_[size_t(held_)];
        const float ey = height(px_, pz_) + kEye;
        const Vec3 look = eye_camera().forward();
        const float dx = look.x, dy = look.y, dz = look.z;
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
    if (shaped_unsaved_) shaped_unsaved_ = false, save(); // (the key let go: as a stroke ends)
}

// ---- the ground shaper (IDEAS 19.209)

void PersonalWorld::set_shaper(int radius_m, int strength_m_s)
{
    shaper_radius_ = float(std::clamp(radius_m, kShaperRadiusMin, kShaperRadiusMax));
    shaper_strength_ = float(std::clamp(strength_m_s, kShaperStrengthMin, kShaperStrengthMax));
}

bool PersonalWorld::aim_ground(float& x, float& z) const
{
    // Along the crosshair in half-metre steps to 300 m, then halved down to where it crosses the grass.
    const float ey = height(px_, pz_) + kEye;
    const Vec3 d = eye_camera().forward();
    float t0 = 0;
    for (float t = 0.5f; t <= 300.0f; t += 0.5f)
    {
        const float wx = px_ + d.x * t, wz = pz_ + d.z * t;
        if (std::fabs(wx) > kHalf || std::fabs(wz) > kHalf) return false; // (past the world's edge)
        if (ey + d.y * t <= ground(wx, wz))
        {
            float lo = t0, hi = t;
            for (int k = 0; k < 20; ++k)
            {
                const float m = (lo + hi) / 2;
                (ey + d.y * m <= ground(px_ + d.x * m, pz_ + d.z * m) ? hi : lo) = m;
            }
            x = px_ + d.x * hi, z = pz_ + d.z * hi;
            return true;
        }
        t0 = t;
    }
    return false;
}

// One frame's shaping at (cx, cz): each cell corner within the radius moved by up to strength x dt
// metres, all of it at the middle and none at the edge ((1 - (d/r)^2)^2, a soft round brush): up (1),
// down (2), or towards the ground's height at the middle (3). What stood on the ground there stays on it.
void PersonalWorld::shape(int mode, float dt, float cx, float cz)
{
    constexpr float kMost = 500.0f; // (no corner moved further than this from the seed's height)
    const int n = int(2 * kHalf / kCell);
    const size_t w = size_t(n + 1);
    if (shaped_.empty()) shaped_.assign(w * w, 0.0f);
    const float r = shaper_radius_, step = shaper_strength_ * dt, middle = ground(cx, cz);
    // (what stands on the ground near the brush, found before it moves: it is moved with it)
    std::vector<size_t> on_ground;
    for (size_t i = 0; i < placed_.size(); ++i)
    {
        const Placed& p = placed_[i];
        if (int(i) == held_ || (p.x - cx) * (p.x - cx) + (p.z - cz) * (p.z - cz) > (r + kCell) * (r + kCell)) continue;
        if (std::fabs(p.y - ground(p.x, p.z)) < 1e-3f) on_ground.push_back(i);
    }
    std::vector<size_t> chars_on;
    for (size_t i = 0; i < default_chars_.size(); ++i)
        if (std::fabs(default_chars_[i].y - ground(default_chars_[i].x, default_chars_[i].z)) < 1e-3f) chars_on.push_back(i);
    const int i0 = std::max(0, int(std::floor((cx - r + kHalf) / kCell))), i1 = std::min(n, int(std::ceil((cx + r + kHalf) / kCell)));
    const int j0 = std::max(0, int(std::floor((cz - r + kHalf) / kCell))), j1 = std::min(n, int(std::ceil((cz + r + kHalf) / kCell)));
    for (int i = i0; i <= i1; ++i)
        for (int j = j0; j <= j1; ++j)
        {
            const float x = -kHalf + float(i) * kCell, z = -kHalf + float(j) * kCell;
            const float q = ((x - cx) * (x - cx) + (z - cz) * (z - cz)) / (r * r);
            if (q >= 1) continue;
            const float most = step * (1 - q) * (1 - q);
            const size_t k = size_t(i) * w + size_t(j);
            const float by = mode == 1 ? most : mode == 2 ? -most : std::clamp(middle - corner_h_[k], -most, most);
            shaped_[k] = std::clamp(shaped_[k] + by, -kMost, kMost);
            corner_h_[k] = seed_height(x, z) + shaped_[k];
        }
    for (const size_t i : on_ground)
    {
        Placed& p = placed_[i];
        p.y = ground(p.x, p.z);
        if (p.physics) p.resting = false; // (to settle again on the new ground)
    }
    for (const size_t i : chars_on) default_chars_[i].y = ground(default_chars_[i].x, default_chars_[i].z);
    // (anything dropped near it falls or is lifted onto the new ground)
    for (size_t i = 0; i < placed_.size(); ++i)
        if (placed_[i].physics && int(i) != held_ && (placed_[i].x - cx) * (placed_[i].x - cx) + (placed_[i].z - cz) * (placed_[i].z - cz) < (r + kCell) * (r + kCell))
            placed_[i].resting = false;
    shaped_unsaved_ = true;
}

std::string PersonalWorld::shaper_report() const
{
    if (!tool_out_) return {};
    char b[200];
    std::snprintf(b, sizeof b, "out, radius %.0f m, strength %.0f m a second", double(shaper_radius_), double(shaper_strength_));
    std::string t = b;
    float x = 0, z = 0;
    if (aim_ground(x, z)) std::snprintf(b, sizeof b, "; aimed at %.2f, %.2f, ground %.2f m", double(x), double(z), double(ground(x, z)));
    else std::snprintf(b, sizeof b, "; aimed at no ground");
    return t + b;
}

void PersonalWorld::draw_shaper()
{
    // Where it would shape: a ring on the grass at the brush's edge and one at half of it (where it does
    // a little over half as much), and a cross at its middle; in amber, read on grass and sky alike.
    float cx = stroke_x_, cz = stroke_z_; // (a stroke's own place while it lasts)
    if (!stroke_on_ && !aim_ground(cx, cz)) return;
    SDL_SetRenderDrawBlendMode(r_, SDL_BLENDMODE_BLEND);
    for (const auto& [r, alpha] : {std::pair{shaper_radius_, 230}, std::pair{shaper_radius_ * 0.5f, 120}})
    {
        constexpr int kSides = 72;
        SDL_SetRenderDrawColor(r_, 255, 196, 64, Uint8(alpha));
        float px = 0, py = 0;
        bool had = false;
        for (int k = 0; k <= kSides; ++k)
        {
            const float a = 2 * kPi * float(k) / kSides, x = cx + r * std::cos(a), z = cz + r * std::sin(a);
            float sx, sy, dep;
            const bool ok = project(x, ground(x, z) + 0.15f, z, sx, sy, dep);
            if (ok && had) SDL_RenderLine(r_, px, py, sx, sy);
            had = ok, px = sx, py = sy;
        }
    }
    float sx, sy, dep;
    if (project(cx, ground(cx, cz) + 0.15f, cz, sx, sy, dep))
    {
        SDL_SetRenderDrawColor(r_, 255, 196, 64, 255);
        SDL_RenderLine(r_, sx - 5, sy, sx + 5, sy);
        SDL_RenderLine(r_, sx, sy - 5, sx, sy + 5);
    }
}

// ---- day and night (IDEAS 19.222)

bool PersonalWorld::parse_day_length(const std::string& digits, uint64_t& out)
{
    if (digits.empty()) return false;
    uint64_t v = 0;
    for (const char c : digits)
    {
        if (c < '0' || c > '9') return false;
        const uint64_t d = uint64_t(c - '0');
        // (v * 10 + d would pass 2^64 - 1: refused here, before it wraps)
        if (v > (UINT64_MAX - d) / 10) return false;
        v = v * 10 + d;
    }
    if (v == 0) return false; // (a day of no length has no time of day)
    out = v;
    return true;
}

double PersonalWorld::day_phase() const
{
    if (fixed_phase_ >= 0) return std::fmod(fixed_phase_, 1.0);
    // Whole seconds since 1970 and the part of this one, apart: the remainder of the seconds by the
    // length is exact in 64 bits for any length, where a double of all the nanoseconds would not be.
    const auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    const uint64_t t = ns < 0 ? 0 : uint64_t(ns), secs = t / 1000000000u, part = t % 1000000000u;
    const uint64_t into = secs % day_length_;
    return std::min((double(into) + double(part) / 1e9) / double(day_length_), std::nextafter(1.0, 0.0));
}

PersonalWorld::Sky PersonalWorld::sky_at(double phase) const
{
    Sky s{};
    // The sun goes round once a day about the sky's axis (up and to the north, (0, 0.6, 0.8)): rising
    // due east (+x) at a quarter of the day, highest at noon in the south (-z), about 53 degrees up,
    // setting due west at three quarters and as far below the north at midnight.
    const float a = float(2 * 3.14159265358979 * (phase - 0.25));
    s.sun[0] = std::cos(a), s.sun[1] = 0.8f * std::sin(a), s.sun[2] = -0.6f * std::sin(a);
    const float e = s.sun[1];
    auto smooth = [](float lo, float hi, float x) {
        const float t = std::clamp((x - lo) / (hi - lo), 0.0f, 1.0f);
        return t * t * (3 - 2 * t);
    };
    s.day = smooth(-0.12f, 0.22f, e);
    const float warm = std::max(0.0f, 1.0f - std::fabs(e + 0.02f) / 0.22f); // (dawn and dusk: the sun near the horizon)
    const SDL_FColor night_top{0.01f, 0.02f, 0.07f, 1}, night_low{0.05f, 0.07f, 0.16f, 1};
    const SDL_FColor dusk_top{0.22f, 0.24f, 0.50f, 1}, dusk_low{0.98f, 0.56f, 0.30f, 1};
    s.top = mix(mix(night_top, kSkyTop, s.day), dusk_top, warm * 0.55f);
    s.low = mix(mix(night_low, kSkyLow, s.day), dusk_low, warm * 0.75f);
    s.light = mix(mix(SDL_FColor{0.20f, 0.24f, 0.40f, 1}, SDL_FColor{1, 1, 1, 1}, s.day), SDL_FColor{1.0f, 0.70f, 0.48f, 1}, warm * 0.6f);
    s.stars = std::clamp(1.0f - s.day * 1.8f, 0.0f, 1.0f);
    return s;
}

std::string PersonalWorld::day_report() const
{
    if (!day_on_) return {};
    const double phase = day_phase();
    const Sky s = sky_at(phase);
    const int minutes = std::min(1439, int(phase * 1440.0));
    // (the sun's height to a tenth of a degree; under half of one, on the horizon, so the word does not
    // rest on the sign of a rounding error at dawn and dusk)
    const double up = double(std::asin(std::clamp(s.sun[1], -1.0f, 1.0f))) * 180.0 / double(kPi);
    char sun[48];
    if (std::fabs(up) < 0.05) std::snprintf(sun, sizeof sun, "on the horizon");
    else std::snprintf(sun, sizeof sun, "%.1f degrees %s", std::fabs(up), up > 0 ? "up" : "down");
    char b[240];
    std::snprintf(b, sizeof b, "on, %llu seconds a day%s; time %02d:%02d (%s); sun %s; daylight %.2f; stars %.2f", (unsigned long long)day_length_,
                  day_written_ ? "" : " (the default)", minutes / 60, minutes % 60, fixed_phase_ >= 0 ? "set" : "the clock's", sun, double(s.day), double(s.stars));
    return b;
}

// The day panel: digits typed, Return sets the day's length (refused past 2^64 - 1 or at 0, and said),
// Backspace takes back a digit, its own key or Esc closes it.
void PersonalWorld::day_key(SDL_Keycode k, SDL_Keymod mod)
{
    if (Keys::get().is(pk::kDayLength, k, mod))
    {
        day_panel_.open = false, day_edit_.clear();
        return;
    }
    if (k >= SDLK_0 && k <= SDLK_9) day_edit_ += char('0' + (k - SDLK_0));
    else if (k >= SDLK_KP_1 && k <= SDLK_KP_9) day_edit_ += char('1' + (k - SDLK_KP_1));
    else if (k == SDLK_KP_0) day_edit_ += '0';
    else if (k == SDLK_BACKSPACE && !day_edit_.empty()) day_edit_.pop_back();
    else if ((k == SDLK_RETURN || k == SDLK_KP_ENTER) && !day_edit_.empty())
    {
        uint64_t v = 0;
        if (parse_day_length(day_edit_, v))
        {
            day_length_ = v, day_written_ = true;
            note_ = trf("pworld.day.set", {std::to_string(v)});
            save();
        }
        else note_ = trf("pworld.day.refused", {day_edit_});
        day_edit_.clear();
    }
    // (more digits than the largest has can only be refused: kept to a few past it, for the note)
    if (day_edit_.size() > 24) day_edit_.resize(24);
}

void PersonalWorld::render()
{
    int w = 0, h = 0;
    SDL_GetRenderOutputSize(r_, &w, &h);
    W_ = float(w), H_ = float(h);
    cat_panel_.step();
    place_panel_.step();
    if (day_on_) day_panel_.step();
    worlds_panel_.step();
    if (held_ < 0) place_panel_.open = false, place_panel_.shown = 0;
    SDL_SetRenderDrawBlendMode(r_, SDL_BLENDMODE_BLEND);
    // The sky's colours, the sun and the light on things: as they always were, or, with day and night on
    // (IDEAS 19.222), as the time of day has them, worked out afresh each frame.
    SDL_FColor sky_top = kSkyTop, sky_low = kSkyLow;
    float sun[3] = {-0.45f, 0.80f, -0.40f};
    Sky sky{};
    if (day_on_)
    {
        sky = sky_at(day_phase());
        sky_top = sky.top, sky_low = sky.low;
        for (int k = 0; k < 3; ++k) sun[k] = sky.sun[k];
    }
    auto lit_by = [&](SDL_FColor c) { return SDL_FColor{c.r * sky.light.r, c.g * sky.light.g, c.b * sky.light.b, c.a}; };
    // (Real Graphics off: the wireframe's lines dimmer at night, not coloured, so they stay readable on black)
    const float wire_dim = 0.4f + 0.6f * sky.day;
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
        const SDL_FColor top = mix(sky_low, sky_top, std::clamp((horizon - 0) / (H_ * 0.6f), 0.0f, 1.0f));
        std::vector<SDL_Vertex> v = {{{0, 0}, top, {}}, {{W_, 0}, top, {}}, {{W_, hy}, sky_low, {}}, {{0, hy}, sky_low, {}},
                                     {{0, hy}, sky_low, {}}, {{W_, hy}, sky_low, {}}, {{W_, H_}, sky_low, {}}, {{0, H_}, sky_low, {}}};
        const int idx[] = {0, 1, 2, 0, 2, 3, 4, 5, 6, 4, 6, 7};
        SDL_RenderGeometry(r_, nullptr, v.data(), int(v.size()), idx, 12);
    }
    if (day_on_)
    {
        const float hy = std::clamp(horizon, 0.0f, H_);
        {
            // The stars, fixed on the sky and turning with it about its axis, as the sun does; those above
            // the horizon, brighter as the day's light goes.
            const float a = float(2 * 3.14159265358979 * (day_phase() - 0.25)), ca = std::cos(a), sa = std::sin(a);
            const float ax[3] = {0, 0.6f, 0.8f};
            const float eye_y = height(px_, pz_) + kEye;
            if (sky.stars > 0)
                for (const auto& st : stars_)
                {
                    // (Rodrigues: v cos a + (axis x v) sin a + axis (axis . v)(1 - cos a))
                    const float cx = ax[1] * st[2] - ax[2] * st[1], cy = ax[2] * st[0] - ax[0] * st[2], cz = ax[0] * st[1] - ax[1] * st[0];
                    const float dot = ax[0] * st[0] + ax[1] * st[1] + ax[2] * st[2];
                    const float d[3] = {st[0] * ca + cx * sa + ax[0] * dot * (1 - ca), st[1] * ca + cy * sa + ax[1] * dot * (1 - ca), st[2] * ca + cz * sa + ax[2] * dot * (1 - ca)};
                    if (d[1] < 0.02f) continue;
                    float sx, sy, dep;
                    if (!project(px_ + d[0] * 2000, eye_y + d[1] * 2000, pz_ + d[2] * 2000, sx, sy, dep) || sy > hy) continue;
                    // (each its own brightness, from where it is: some stars brighter than others)
                    const float b = 0.45f + 0.55f * std::fabs(std::sin(st[0] * 97.0f + st[2] * 41.0f));
                    SDL_SetRenderDrawColorFloat(r_, 1, 1, 0.92f, sky.stars * b * std::min(1.0f, d[1] * 8));
                    const SDL_FRect pt{sx - 1, sy - 1, b > 0.85f ? 3.0f : 2.0f, b > 0.85f ? 3.0f : 2.0f};
                    SDL_RenderFillRect(r_, &pt);
                }
            // The sun's disc while it is up (or just below: its glow shows first), white at noon, warmer low.
            float sx, sy, dep;
            if (sun[1] > -0.06f && project(px_ + sun[0] * 2000, eye_y + sun[1] * 2000, pz_ + sun[2] * 2000, sx, sy, dep))
            {
                const float rad = f * 0.035f;
                const SDL_FColor core = mix(SDL_FColor{1.0f, 0.62f, 0.32f, 1}, SDL_FColor{1.0f, 0.98f, 0.88f, 1}, std::clamp(sun[1] / 0.3f, 0.0f, 1.0f));
                constexpr int kSides = 32;
                if (wireframe_)
                {
                    // (Real Graphics off: its outline, as the clouds are drawn)
                    SDL_SetRenderDrawColorFloat(r_, core.r, core.g, core.b, 1);
                    outline_ellipse(r_, sx, sy, rad, rad, kSides);
                }
                else
                for (const auto& [r, alpha] : {std::pair{rad * 3.0f, 0.25f}, std::pair{rad, 1.0f}})
                {
                    std::vector<SDL_Vertex> g;
                    const SDL_FColor in{core.r, core.g, core.b, alpha}, out{core.r, core.g, core.b, r > rad ? 0.0f : alpha};
                    for (int k = 0; k < kSides; ++k)
                    {
                        const float a0 = 2 * kPi * float(k) / kSides, a1 = 2 * kPi * float(k + 1) / kSides;
                        g.push_back({{sx, sy}, in, {}});
                        g.push_back({{sx + r * std::cos(a0), sy + r * std::sin(a0)}, out, {}});
                        g.push_back({{sx + r * std::cos(a1), sy + r * std::sin(a1)}, out, {}});
                    }
                    SDL_RenderGeometry(r_, nullptr, g.data(), int(g.size()), nullptr, 0);
                }
            }
        }
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
                const float m = day_on_ ? wire_dim : 1.0f;
                SDL_SetRenderDrawColorFloat(r_, 0.55f * c[3] * m, 0.55f * c[3] * m, 0.60f * c[3] * m, 1);
                outline_ellipse(r_, c[0], c[1], c[2], c[2] * 0.6f, kSides);
                continue;
            }
            std::vector<SDL_Vertex> v;
            SDL_FColor in{c[3], c[3], c[3], 0.9f}, out{c[3], c[3], c[3], 0.0f};
            if (day_on_) in = lit_by(in), out = lit_by(out); // (grey at night, warm at dawn and dusk)
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
                const size_t w = size_t(n + 1);
                const float h00 = corner_h_[size_t(i) * w + size_t(j)], h10 = corner_h_[size_t(i + 1) * w + size_t(j)], h01 = corner_h_[size_t(i) * w + size_t(j + 1)],
                            h11 = corner_h_[size_t(i + 1) * w + size_t(j + 1)];
                // (a faint variation of the cell's own, from the seed)
                const float vary = cell_vary_[size_t(i) * size_t(n) + size_t(j)];
                const SDL_FColor g{kGrass.r * vary, kGrass.g * vary, kGrass.b * vary, 1};
                world.push_back({{Pt{x0, h00, z0}, Pt{x1, h10, z0}, Pt{x1, h11, z1}}, g, 0.10f / 30.0f, 3}); // (not the diagonal)
                world.push_back({{Pt{x0, h00, z0}, Pt{x1, h11, z1}, Pt{x0, h01, z1}}, g, 0.10f / 30.0f, 6});
            }
        // The objects after the ground, all of them: a small thing near a large cell of grass would
        // otherwise be painted over by it (the cell's middle can be nearer than the thing). One
        // hidden behind a hill is left out, by a line of sight to its top over the height field.
        struct Picture
        {
            std::array<Pt, 4> w;
            Pt n; // its front's outward normal
            std::shared_ptr<const sieve::RgbaImage> img;
        };
        std::vector<Picture> pics;
        std::vector<size_t> chars_seen; // (the characters placed that are in sight, drawn below as spheres)
        {
            const float eye_y = height(px_, pz_) + kEye;
            for (const Placed& p : placed_)
            {
                // (to its top: an object whose top can be seen over the hill is drawn, even where its base sits
                // below the grass, as a bookcase set down on a slope does; before 14 November 2026 the line went
                // to half a metre above its base, and such a one was left out in plain sight)
                float low = 0, high = 0.4f, across = 0;
                extent_of(p, low, high, across);
                const float g = p.y + std::max(high, 0.1f);
                bool seen = true;
                const float dx = p.x - px_, dz = p.z - pz_, d = std::sqrt(dx * dx + dz * dz);
                for (float u = 2.0f / std::max(d, 2.0f); u < 1.0f && seen; u += 2.0f / std::max(d, 2.0f))
                    seen = ground(px_ + dx * u, pz_ + dz * u) < eye_y + (g - eye_y) * u;
                if (!seen) continue;
                if (is_char(p.kind))
                {
                    chars_seen.push_back(size_t(&p - placed_.data()));
                    continue;
                }
                for (const auto& [t, c] : faces_of(p)) world.push_back({t, c, 0, 7, true});
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
            std::array<float, 6> nz{}; // each point's nearness (1/depth), for the depth test
        };
        std::vector<Tri> tris;
        tris.reserve(world.size());
        // The objects go to the hallway's renderer (MeshBatch: a depth test at every pixel, laid over the
        // ground), seen by the same camera as the ground.
        const Camera eye_cam = eye_camera();
        const float eye = eye_cam.pos.y;
        // (eye_space's turn, its sines and the eye's height worked out once for the whole ground)
        const float cy = std::cos(yaw_), sy0 = std::sin(yaw_), cp = std::cos(pitch_), sp = std::sin(pitch_);
        auto cam = [&](const Pt& w) {
            const float dx = w[0] - px_, dy = w[1] - eye, dz = w[2] - pz_;
            const float x1 = dx * cy - dz * sy0, z1 = dx * sy0 + dz * cy;
            return Pt{x1, dy * cp - z1 * sp, dy * sp + z1 * cp};
        };
        batch_.begin_overlay(eye_cam, int(W_), int(H_));
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
            float lit = std::max(0.0f, (nx * sun[0] + ny * sun[1] + nz * sun[2]) / nl);
            if (day_on_) lit *= sky.day; // (the sun's own light only while it is up)
            const float cxw = (t[0][0] + t[1][0] + t[2][0]) / 3, cyw = (t[0][1] + t[1][1] + t[2][1]) / 3, czw = (t[0][2] + t[1][2] + t[2][2]) / 3;
            const float shade = 0.55f + 0.45f * lit + w.lift * cyw;
            const float dist = std::sqrt((cxw - px_) * (cxw - px_) + (czw - pz_) * (czw - pz_));
            SDL_FColor c{std::clamp(w.base.r * shade, 0.0f, 1.0f), std::clamp(w.base.g * shade, 0.0f, 1.0f), std::clamp(w.base.b * shade, 0.0f, 1.0f), 1};
            if (day_on_ && !wireframe_) c = lit_by(c); // (the light's colour: white by day, warm low, dim and blue at night)
            if (wireframe_)
            {
                c = mix(w.base, SDL_FColor{0, 0, 0, 1}, std::pow(std::clamp(dist / 700.0f, 0.0f, 1.0f), 1.5f));
                if (day_on_) c = SDL_FColor{c.r * wire_dim, c.g * wire_dim, c.b * wire_dim, 1};
            }
            else c = mix(c, sky_low, std::pow(std::clamp(dist / 700.0f, 0.0f, 1.0f), 1.5f));
            if (w.obj)
            {
                // (filled black in the wireframe, its edges then drawn where nothing nearer covers them)
                const Vec3 f3[3] = {{t[0][0], t[0][1], t[0][2]}, {t[1][0], t[1][1], t[1][2]}, {t[2][0], t[2][1], t[2][2]}};
                batch_.add_face(f3, wireframe_ ? 0xFF000000u : argb_of(c));
                if (wireframe_)
                    for (int k = 0; k < 3; ++k)
                        if ((w.edges >> k) & 1u) batch_.add_edge(f3[k], f3[(k + 1) % 3], argb_of(c));
                continue;
            }
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
                const Point2 s = eye_cam.project_camera({q[0], q[1], q[2]});
                tr.p[size_t(k)] = {s.x, s.y};
                tr.nz[size_t(k)] = 1.0f / q[2];
                sum += q[2];
            }
            tr.depth = sum / float(no);
            tris.push_back(tr);
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
        // The objects' pictures (favourites'), on their fronts turned towards you; then everything given to
        // the renderer drawn over the ground.
        for (const Picture& pic : pics)
        {
            const float vx = pic.w[0][0] - px_, vy = pic.w[0][1] - eye, vz = pic.w[0][2] - pz_;
            if (vx * pic.n[0] + vy * pic.n[1] + vz * pic.n[2] >= 0) continue;
            Vec3 q[4];
            for (int k = 0; k < 4; ++k) q[k] = {pic.w[size_t(k)][0], pic.w[size_t(k)][1], pic.w[size_t(k)][2]};
            batch_.add_picture(q, *pic.img);
        }
        // Your characters, where they are placed (as objects, above): at their own size (4096 units a
        // metre), turned as they are placed. Their spheres and bones go to the same renderer as the
        // objects, so a bookcase between you and a character hides what it covers, pixel by pixel, and
        // spheres cut into one another where they meet (IDEAS 19.207). Drawn as the Character Creator
        // draws them: lit from the upper left, or as rings in the wireframe.
        for (const size_t i : chars_seen)
        {
            const Placed& p = placed_[i];
            const sieve::Character* c = character(p.kind);
            if (!c) continue;
            std::vector<Vec3> pos(c->spheres.size());
            each_sphere(*c, p.yaw, p.pitch, p.roll, [&](const sieve::Sphere& s, const std::array<float, 3>& w, float) {
                pos[size_t(&s - c->spheres.data())] = Vec3{p.x + w[0], p.y + w[1], p.z + w[2]};
            });
            for (size_t k = 0; k < c->spheres.size(); ++k)
                if (c->spheres[k].parent >= 0) batch_.add_edge(pos[k], pos[size_t(c->spheres[k].parent)], 0xFF3C3C46u);
            MeshBatch::SphereLook look;
            look.style = wireframe_ ? MeshBatch::SphereLook::Style::Ring : MeshBatch::SphereLook::Style::Shaded;
            look.argb = p.kind == "char:you" ? 0xFFF2EBD9u : 0xFFCCD1E6u;
            if (day_on_ && !wireframe_)
            {
                // (in the light of the time of day, as everything else is)
                const uint32_t a = look.argb;
                auto ch = [](uint32_t v, float m) { return uint32_t(std::clamp(float(v & 0xFFu) * m, 0.0f, 255.0f)); };
                look.argb = 0xFF000000u | ch(a >> 16, sky.light.r) << 16 | ch(a >> 8, sky.light.g) << 8 | ch(a, sky.light.b);
            }
            look.ambient = 0.35f;
            look.min_radius = 1.5f;
            for (size_t k = 0; k < c->spheres.size(); ++k) batch_.add_sphere(pos[k], float(c->spheres[k].r) / float(sieve::kCharacterUnitsPerMetre), look);
        }
        if (held_ >= 0 && turn_rings_) add_rings();
        batch_.draw(r_);
    }
    const SDL_Color white{255, 255, 255, 255}, grey{200, 200, 210, 255};
    if (tool_out_ && held_ < 0) draw_shaper(); // (IDEAS 19.209: where the ground shaper would shape)
    if (show_map_) draw_map();
    if (show_compass_) draw_compass();
    // The crosshair, and what it rests on: the keys for it (an author's; a visitor may only open).
    SDL_SetRenderDrawColor(r_, 255, 255, 255, 200);
    SDL_RenderLine(r_, W_ / 2 - 6, H_ / 2, W_ / 2 + 6, H_ / 2);
    SDL_RenderLine(r_, W_ / 2, H_ / 2 - 6, W_ / 2, H_ / 2 + 6);
    if (held_ >= 0)
    {
        draw_text(r_, W_ / 2 + 14, H_ / 2 + 10, trf("pworld.holding", {label_of(placed_[size_t(held_)].kind)}), 1, white);
        // The turn chosen, in its axis's colour, and what the wheel does to it (IDEAS 19.221).
        if (turn_ >= 0)
        {
            static const char* const kAxis[3] = {"y", "z", "x"};
            const std::string* keys[3] = {&pk::kTurn, &pk::kTip, &pk::kRoll};
            const uint32_t c = axis_argb_[turn_ == 0 ? 1 : turn_ == 1 ? 2 : 0];
            char step[16];
            std::snprintf(step, sizeof step, "%g", double(wheel_step_ * 180.0f / kPi));
            const std::string line = trf("pworld.turning", {kAxis[turn_], Keys::get().first(*keys[turn_]), step});
            draw_text(r_, W_ / 2 + 15, H_ / 2 + 29, line, 1, SDL_Color{0, 0, 0, 200}); // (a shadow: read on grass and sky alike)
            draw_text(r_, W_ / 2 + 14, H_ / 2 + 28, line, 1, SDL_Color{Uint8(c >> 16), Uint8(c >> 8), Uint8(c), 255});
        }
    }
    else if (tool_out_)
    {
        // The ground shaper out (IDEAS 19.209): what it is, its radius and strength.
        char rs[16], ss[16];
        std::snprintf(rs, sizeof rs, "%.0f", double(shaper_radius_));
        std::snprintf(ss, sizeof ss, "%.0f", double(shaper_strength_));
        const std::string line = trf("pworld.shaper", {rs, ss});
        draw_text(r_, W_ / 2 + 15, H_ / 2 + 11, line, 1, SDL_Color{0, 0, 0, 200}); // (a shadow)
        draw_text(r_, W_ / 2 + 14, H_ / 2 + 10, line, 1, SDL_Color{255, 196, 64, 255});
    }
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
        if (covers_on_)
        {
            // Favourites' covers (IDEAS 19.190): each one's picture small in the margin left of its row, where
            // nothing else is drawn, and the chosen row's larger just below the list (beside it, it would cover
            // the long rows' ends, which run past the plate), its shape kept. A row with no picture is left as
            // it was.
            auto fit = [](const SDL_Texture* t, float box, float& w, float& h) {
                const float tw = float(t->w), th = float(t->h), k = box / std::max(tw, th);
                w = tw * k, h = th * k;
            };
            for (size_t i = 0; i < cat.size(); ++i)
            {
                if (cat[i].rfind("fav:", 0) != 0) continue;
                SDL_Texture* t = cover_of(cat[i].substr(4));
                if (!t) continue;
                float w, h;
                fit(t, 16, w, h);
                const SDL_FRect d{x + 3 + (16 - w) / 2, 75 + float(i) * 18 + (16 - h) / 2, w, h};
                SDL_RenderTexture(r_, t, nullptr, &d);
                if (int(i) == catalogue_row_)
                {
                    float bw, bh;
                    fit(t, 160, bw, bh);
                    const SDL_FRect frame{x, plate.y + plate.h + 6, bw + 12, bh + 12};
                    SDL_SetRenderDrawColor(r_, 0, 0, 0, 170);
                    SDL_RenderFillRect(r_, &frame);
                    const SDL_FRect big{frame.x + 6, frame.y + 6, bw, bh};
                    SDL_RenderTexture(r_, t, nullptr, &big);
                }
            }
        }
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
    if (day_on_ && day_panel_.shown > 0)
    {
        // Day and night (IDEAS 19.222): the day's length, typed here, and the time of day it gives now.
        const float x = day_panel_.left(380);
        const SDL_FRect plate{x, 48, 370, 34 + 4 * 18};
        SDL_SetRenderDrawColor(r_, 0, 0, 0, 170);
        SDL_RenderFillRect(r_, &plate);
        draw_text(r_, x + 22, 56, tr("pworld.day.title"), 1, white);
        draw_text(r_, x + 22, 76, trf("pworld.day.length", {day_edit_.empty() ? std::to_string(day_length_) : day_edit_ + "_"}), 1, white);
        const int minutes = std::min(1439, int(day_phase() * 1440.0));
        char hm[16];
        std::snprintf(hm, sizeof hm, "%02d:%02d", minutes / 60, minutes % 60);
        draw_text(r_, x + 22, 94, trf("pworld.day.time", {hm}), 1, grey);
        draw_text(r_, x + 22, 112, tr("pworld.day.most"), 1, grey);
    }
    draw_worlds(); // (the worlds line's panel, while it is out: IDEAS 19.187)
    // Their tabs at the left edge while they are out: Place while something is held, else the catalogue.
    if (held_ >= 0) place_panel_.draw_tab(r_, 2, 80, tr("pworld.place.tab"));
    else if (!worlds_panel_.open) cat_panel_.draw_tab(r_, 2, 80, tr("pworld.catalogue.tab")); // (not over the worlds line's panel)
    if (!note_.empty()) draw_text(r_, 20, H_ - 42, note_, 1, white);
    {
        // The footer: the keys as they are set (Settings > Controls > Keys), each by what it does.
        const Keys& K = Keys::get();
        auto f = [&](const std::string& id) { return K.first(id); };
        std::string foot;
        if (day_on_ && day_panel_.open) foot = trf("pworld.footer.day", {f(pk::kDayLength)});
        else if (worlds_panel_.open) foot = trf("pworld.footer.worlds", {f(pk::kWorldsGo), f(pk::kWorldsAddress)});
        else if (cat_panel_.open)
        {
            foot = trf("pworld.footer.catalogue", {f(pk::kChooseUp), f(pk::kChooseDown), f(pk::kPlace), f(pk::kCatalogue)});
            // (the import key said once you have a model of your own, so a catalogue without one looks as
            // it always has; Settings > Controls > Keys and the manual name it before then)
            if (!models_.empty()) foot += "   " + trf("pworld.footer.import", {f(pk::kImport)});
        }
        else if (place_panel_.open && held_ >= 0) foot = trf("pworld.footer.place", {f(pk::kDrop), f(pk::kExact)});
        else if (held_ >= 0)
            foot = trf("pworld.footer.holding", {f(pk::kTurn), f(pk::kTip), f(pk::kRoll), f(pk::kTurnBack), f(pk::kTipBack), f(pk::kRollBack), f(pk::kExact), f(pk::kPlaceHere), f(pk::kDrop), f(pk::kPutBack)});
        else if (tool_out_)
            foot = trf("pworld.footer.shaper", {f(pk::kRaise), f(pk::kLower), f(pk::kFlatten), f(pk::kBrushSmaller) + "/" + f(pk::kBrushBigger), f(pk::kToolAway)});
        else
            foot = trf("pworld.footer", {f(pk::kForward) + " " + f(pk::kLeft) + " " + f(pk::kBack) + " " + f(pk::kRight), f(pk::kLookUp) + "/" + f(pk::kLookDown), f(pk::kRun),
                                         f(pk::kCatalogue), f(pk::kPickUp), f(pk::kOpen), f(pk::kTakeAway), f(pk::kZoomIn) + "/" + f(pk::kZoomOut)});
        if (day_on_ && !cat_panel_.open && held_ < 0 && !day_panel_.open && !worlds_panel_.open) foot += "   " + trf("pworld.footer.day_key", {f(pk::kDayLength)});
        draw_text(r_, 20, H_ - 24, foot, 1, white);
    }
    if (in_place_state_ == InPlace::State::Making)
    {
        // A favourite being made to open in place (IDEAS 19.208): small, by the crosshair, its dots
        // counting one to three as it is made, and the world going on round it.
        const std::string line = tr("pworld.calculating") + std::string(size_t(1 + (SDL_GetTicks() - in_place_since_) / 400 % 3), '.');
        draw_text(r_, W_ / 2 + 15, H_ / 2 - 21, line, 1, SDL_Color{0, 0, 0, 200}); // (a shadow)
        draw_text(r_, W_ / 2 + 14, H_ / 2 - 22, line, 1, SDL_Color{255, 90, 90, 255});
    }
    if (pause_open_) draw_pause(); // (over everything)
    if (in_place_state_ == InPlace::State::Open && in_place_) in_place_->draw(W_, H_); // (the hallway's viewer, over the world)
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
        if (!inside(q) || is_char(p.kind)) continue;
        SDL_SetRenderDrawColor(r_, 255, p.physics && !p.resting ? 220 : 255, p.physics && !p.resting ? 60 : 255, 255);
        const SDL_FRect d{q.x - 1.5f, q.y - 1.5f, 3, 3};
        SDL_RenderFillRect(r_, &d);
    }
    // Your characters, as rings where they stand.
    SDL_SetRenderDrawColor(r_, 200, 205, 230, 255);
    for (const Placed& p : placed_)
    {
        const SDL_FPoint q = at(p.x, p.z);
        if (!is_char(p.kind) || !inside(q)) continue;
        outline_ellipse(r_, q.x, q.y, 3, 3, 8);
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

std::filesystem::path shelf_items_file() { return sieve::cli::find_data_folder("objects", "shelf-items.sieve") / "shelf-items.sieve"; }

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
                for (MeshTri& t : moved->tris)
                    for (Vec3& v : t.p) v.x += 1.65f, v.z -= 3.0f;
                fit_bounds(*moved);
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

SDL_Texture* PersonalWorld::cover_of(const std::string& sha)
{
    if (const auto it = covers_.find(sha); it != covers_.end()) return it->second;
    SDL_Texture* tex = nullptr;
    if (auto img = picture_of(sha))
    {
        tex = gpu::create(r_, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, int(img->width), int(img->height));
        if (tex)
        {
            SDL_UpdateTexture(tex, nullptr, img->rgba.data(), int(img->width) * 4);
            SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
            // (the small one at 16 pixels: the nearest pixel of a 128-pixel picture would drop most of it)
            SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_LINEAR);
        }
    }
    covers_[sha] = tex;
    return tex;
}

std::string PersonalWorld::covers_report() const
{
    if (!cat_panel_.open) return "";
    size_t favs = 0, with = 0;
    for (const std::string& k : rows_)
        if (k.rfind("fav:", 0) == 0)
        {
            ++favs;
            with += picture_of(k.substr(4)) ? 1 : 0;
        }
    if (favs == 0) return "";
    std::string t = std::string(covers_on_ ? "on" : "off") + ", " + std::to_string(with) + " of " + std::to_string(favs) + " favourites with one";
    if (catalogue_row_ >= 0 && size_t(catalogue_row_) < rows_.size() && rows_[size_t(catalogue_row_)].rfind("fav:", 0) == 0)
    {
        const auto img = picture_of(rows_[size_t(catalogue_row_)].substr(4));
        t += img ? "; the chosen row's " + std::to_string(img->width) + "x" + std::to_string(img->height) + (covers_on_ ? ", shown" : ", not shown") : "; the chosen row's none";
    }
    return t;
}

std::string PersonalWorld::mesh_kind(const std::string& kind) const
{
    if (kind.rfind("fav:", 0) != 0) return kind;
    const Favourite* f = favourite(kind.substr(4));
    return f ? f->line : "pages"; // (one no longer kept: drawn as a page until it is taken away)
}

// ---- your own objects (IDEAS 19.187's next step; SPECIFICATIONS 12.0r's `model SHA`)

namespace {

// What one own model's .sieve may unfold to, and what a file brought in may be: far above any model
// the catalogue would want, there only so a file that is not one cannot fill the memory.
constexpr uint64_t kOwnModelRoof = uint64_t(64) << 20;
// The most triangles an own model may have: every face of everything in sight is drawn in software each
// frame (MeshBatch), so a model of millions would stop the world; a limit chosen, not measured.
constexpr size_t kOwnModelTriangles = 100000;

bool is_sha(const std::string& s)
{
    return s.size() == 64 && std::all_of(s.begin(), s.end(), [](char c) { return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'); });
}

// A file's bytes, or nothing where it cannot be read or is larger than the roof.
std::optional<std::string> read_capped(const std::filesystem::path& f)
{
    std::error_code ec;
    const auto size = std::filesystem::file_size(f, ec);
    if (ec || size > kOwnModelRoof) return std::nullopt;
    std::ifstream in(f, std::ios::binary);
    std::string data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    if (!in && data.size() != size) return std::nullopt;
    return data;
}

// A model moved to stand as every object does: its lowest point at y = 0 (its foot on what it rests
// on), its middle over the place in x and z. Only how it is drawn: the .obj kept is as it came.
std::shared_ptr<const Mesh> stood(const Mesh& m)
{
    auto out = std::make_shared<Mesh>(m);
    const float dx = -(m.lo.x + m.hi.x) / 2, dy = -m.lo.y, dz = -(m.lo.z + m.hi.z) / 2;
    for (MeshTri& t : out->tris)
        for (Vec3& v : t.p) v.x += dx, v.y += dy, v.z += dz;
    fit_bounds(*out);
    return out;
}

} // namespace

const Mesh* PersonalWorld::mesh_of(const std::string& kind) const
{
    if (is_model(kind)) return own_model(kind.substr(6)).mesh.get();
    const auto it = shelf_items().mesh.find(mesh_kind(kind));
    return it == shelf_items().mesh.end() ? nullptr : it->second.get();
}

// Read from objects/SHA.sieve the first time it is asked for: every member checked as the .sieve is
// read, then the .obj among them whose SHA-256 is the name (so a file renamed, or changed, is not taken
// for another), with the .mtl it names from beside it in the same .sieve.
const PersonalWorld::OwnModel& PersonalWorld::own_model(const std::string& sha) const
{
    if (const auto it = model_meshes_.find(sha); it != model_meshes_.end()) return it->second;
    OwnModel m;
    std::error_code ec;
    const std::filesystem::path f = file_.empty() ? std::filesystem::path() : store() / "objects" / (sha + ".sieve");
    if (f.empty() || !std::filesystem::is_regular_file(f, ec)) m.why = "not in the store";
    else
        try
        {
            std::map<std::string, std::string> text; // (each member by its file name)
            for (auto& [name, bytes] : sieve::cli::files_in_sieve(f, kOwnModelRoof))
                text[sieve::u8(sieve::from_u8(name).filename())] = std::string(bytes.begin(), bytes.end());
            const auto obj = std::find_if(text.begin(), text.end(), [&](const auto& t) {
                return sieve::from_u8(t.first).extension() == ".obj" && sieve::Sha256::hex_hash(t.second) == sha;
            });
            if (obj == text.end()) m.why = "no .obj in it whose SHA-256 is its name";
            else
            {
                std::istringstream in(obj->second);
                const auto mesh = read_obj(in, "objects/" + sha + ".sieve: " + obj->first, [&](const std::string& lib) {
                    const auto l = text.find(sieve::u8(sieve::from_u8(lib).filename()));
                    return l == text.end() ? std::string() : l->second;
                });
                if (!mesh) m.why = "no faces";
                else if (mesh->tris.size() > kOwnModelTriangles) m.why = "more than " + std::to_string(kOwnModelTriangles) + " triangles";
                else m.mesh = stood(*mesh), m.name = obj->first;
            }
        }
        catch (const std::exception& x)
        {
            m.why = x.what();
        }
    return model_meshes_[sha] = m;
}

// A model from a file into the store and the catalogue. A glTF (.gltf, .glb) is read by the importers
// (its buffers beside it, never outside its folder) and kept as the .obj of its triangles, so every
// own object is kept and read one way; an .obj is kept exactly as it came, with the .mtl its first
// mtllib names where that lies beside it. The .sieve is packed as Sieve packs a folder, read back and
// checked before it is kept; one already in the store (the same .obj) is not packed again.
bool PersonalWorld::import_model(const std::string& path)
{
    if (file_.empty())
    {
        note_ = tr("pworld.import.no_store");
        return false;
    }
    const std::filesystem::path src = sieve::from_u8(path), folder = src.parent_path();
    const auto bytes = read_capped(src);
    if (!bytes)
    {
        note_ = trf("pworld.import.unreadable", {path});
        return false;
    }
    // A file beside it, by the name the model gives (a glTF's buffer, an .obj's material library).
    const auto beside = [&](const std::string& name) -> std::optional<std::string> {
        if (!sieve::import_uri_is_local(name)) return std::nullopt;
        return read_capped(folder / sieve::from_u8(name));
    };
    std::string obj, mtl, mtl_name;
    const std::string obj_name = sieve::u8(src.stem()) + ".obj";
    const size_t first = bytes->find_first_not_of(" \t\r\n");
    const bool gltf = bytes->rfind("glTF", 0) == 0 || (first != std::string::npos && (*bytes)[first] == '{');
    try
    {
        if (gltf)
        {
            const sieve::ImportedModel im = sieve::read_model(*bytes, beside); // (a buffer's URI comes decoded)
            for (const auto& v : im.mesh.verts) obj += "v " + sieve::nine_digits(v.x) + " " + sieve::nine_digits(v.y) + " " + sieve::nine_digits(v.z) + "\n";
            for (const auto& f : im.mesh.faces) obj += "f " + std::to_string(f.a + 1) + " " + std::to_string(f.b + 1) + " " + std::to_string(f.c + 1) + "\n";
        }
        else
        {
            obj = *bytes;
            std::istringstream lines(obj);
            for (std::string l; std::getline(lines, l);)
            {
                if (!l.empty() && l.back() == '\r') l.pop_back();
                if (l.rfind("mtllib ", 0) != 0) continue;
                const std::string name = l.substr(l.find_first_not_of(' ', 7) == std::string::npos ? l.size() : l.find_first_not_of(' ', 7));
                if (const auto t = beside(name))
                {
                    mtl = *t;
                    mtl_name = sieve::u8(sieve::from_u8(name).filename());
                }
                break; // (the first library only)
            }
        }
    }
    catch (const std::exception& x)
    {
        note_ = trf("pworld.import.refused", {sieve::u8(src.filename()), x.what()});
        return false;
    }
    {
        // Read as it will be drawn before it is kept: a file with no faces, or too many, is refused here.
        std::istringstream in(obj);
        const auto mesh = read_obj(in, obj_name, [&](const std::string& lib) { return sieve::from_u8(lib).filename() == sieve::from_u8(mtl_name).filename() ? mtl : std::string(); });
        const std::string why = !mesh ? "no faces" : mesh->tris.size() > kOwnModelTriangles ? "more than " + std::to_string(kOwnModelTriangles) + " triangles" : "";
        if (!why.empty())
        {
            note_ = trf("pworld.import.refused", {sieve::u8(src.filename()), why});
            return false;
        }
    }
    const std::string sha = sieve::Sha256::hex_hash(obj);
    const std::filesystem::path dest = store() / "objects" / (sha + ".sieve");
    std::error_code ec;
    if (!std::filesystem::is_regular_file(dest, ec))
    {
        const std::filesystem::path dir = std::filesystem::temp_directory_path(ec) / ("sieve-model-" + sha.substr(0, 16) + "-" + std::to_string(SDL_GetTicksNS()));
        try
        {
            std::filesystem::create_directories(dir);
            std::filesystem::create_directories(dest.parent_path());
            {
                std::ofstream o(dir / sieve::from_u8(obj_name), std::ios::binary);
                o << obj;
                if (!mtl_name.empty() && mtl_name != obj_name)
                {
                    std::ofstream m(dir / sieve::from_u8(mtl_name), std::ios::binary);
                    m << mtl;
                }
                if (!o) throw std::runtime_error("cannot write " + sieve::u8(dir));
            }
            sieve::cli::pack_folder_sieve(dir, dest, kOwnModelRoof);
        }
        catch (const std::exception& x)
        {
            std::filesystem::remove_all(dir, ec);
            note_ = trf("pworld.import.refused", {sieve::u8(src.filename()), x.what()});
            return false;
        }
        std::filesystem::remove_all(dir, ec);
    }
    model_meshes_.erase(sha); // (read afresh from what is now kept)
    const OwnModel& m = own_model(sha);
    if (!m.mesh)
    {
        note_ = trf("pworld.import.refused", {sieve::u8(src.filename()), m.why});
        return false;
    }
    const std::string row = "model:" + sha;
    if (std::find(models_.begin(), models_.end(), sha) == models_.end())
    {
        models_.push_back(sha);
        rows_.push_back(row);
        save();
        note_ = trf("pworld.import.kept", {m.name, std::to_string(m.mesh->tris.size()), sha.substr(0, 12)});
    }
    else note_ = trf("pworld.import.already", {m.name, sha.substr(0, 12)});
    catalogue_row_ = int(std::find(rows_.begin(), rows_.end(), row) - rows_.begin()); // (chosen, to be placed with the place key)
    return true;
}

void SDLCALL PersonalWorld::import_chosen(void* self, const char* const* files, int)
{
    auto* w = static_cast<PersonalWorld*>(self);
    {
        std::lock_guard<std::mutex> lock(w->import_mx_);
        w->import_from_ = files && files[0] ? files[0] : "";
    }
    w->import_open_ = false; // (after the path is there: run() takes it once it sees this)
}

void PersonalWorld::import_dialog()
{
    if (import_open_) return;
    static const SDL_DialogFileFilter kModels[] = {{"Models (OBJ, glTF)", "obj;gltf;glb"}};
    import_open_ = true, import_waiting_ = true;
    SDL_SetWindowRelativeMouseMode(window_, false); // (the mouse is the dialog's while it is open)
    open_file_dialog(import_chosen, this, window_, kModels, 1, nullptr, false, false, this);
}

std::string PersonalWorld::models_report() const
{
    if (models_.empty()) return "";
    size_t drawn = 0;
    std::string each;
    for (const std::string& sha : models_)
    {
        const OwnModel& m = own_model(sha);
        drawn += m.mesh ? 1 : 0;
        each += "; " + sha.substr(0, 12) + " " + (m.mesh ? m.name + ", " + std::to_string(m.mesh->tris.size()) + " triangles" : "not drawn: " + m.why);
    }
    return std::to_string(models_.size()) + " in the catalogue, " + std::to_string(drawn) + " drawn" + each;
}

const sieve::Character* PersonalWorld::character(const std::string& kind) const
{
    if (!is_char(kind)) return nullptr;
    if (kind == "char:you") return first_is_yours_ ? &characters_.front() : nullptr;
    for (size_t ci = first_is_yours_ ? 1 : 0; ci < characters_.size(); ++ci)
        if (kind.compare(5, std::string::npos, char_ids_[ci]) == 0) return &characters_[ci];
    return nullptr;
}

std::string PersonalWorld::label_of(const std::string& kind) const
{
    if (is_model(kind))
    {
        const OwnModel& m = own_model(kind.substr(6));
        return m.mesh ? trf("pworld.model", {m.name, std::to_string(m.mesh->tris.size())}) : trf("pworld.model.gone", {kind.substr(6, 12)});
    }
    if (kind == "char:you") return tr("pworld.char.you");
    if (is_char(kind)) return character(kind) ? trf("pworld.char", {kind.substr(5, 12)}) : trf("pworld.char.gone", {kind.substr(5, 12)});
    if (kind.rfind("fav:", 0) != 0) return tr("pworld.object." + kind);
    const Favourite* f = favourite(kind.substr(4));
    if (!f) return trf("pworld.fav.gone", {kind.substr(4, 12)});
    return trf("pworld.fav", {f->title.empty() ? f->sha.substr(0, 12) : f->title.substr(0, 40), f->line});
}

void PersonalWorld::extent_of(const Placed& p, float& low, float& high, float& across) const
{
    if (p.ext_turns[0] == p.yaw && p.ext_turns[1] == p.pitch && p.ext_turns[2] == p.roll && p.ext_kind == p.kind)
    {
        low = p.ext[0], high = p.ext[1], across = p.ext[2];
        return;
    }
    extent_fresh(p, low, high, across);
    p.ext_kind = p.kind, p.ext_turns[0] = p.yaw, p.ext_turns[1] = p.pitch, p.ext_turns[2] = p.roll;
    p.ext[0] = low, p.ext[1] = high, p.ext[2] = across;
}

void PersonalWorld::extent_fresh(const Placed& p, float& low, float& high, float& across) const
{
    low = 0, high = 0.4f, across = 0.2f;
    if (const sieve::Character* c = character(p.kind))
    {
        low = 1e9f, high = -1e9f, across = 0;
        each_sphere(*c, p.yaw, p.pitch, p.roll, [&](const sieve::Sphere&, const std::array<float, 3>& w, float r) {
            low = std::min(low, w[1] - r), high = std::max(high, w[1] + r);
            across = std::max(across, std::sqrt(w[0] * w[0] + w[2] * w[2]) + r);
        });
        if (low > high) low = 0, high = 0.4f, across = 0.2f;
        return;
    }
    const Mesh* mp = mesh_of(p.kind);
    if (!mp) return;
    const Mesh& m = *mp;
    low = 1e9f, high = -1e9f, across = 0;
    for (int c = 0; c < 8; ++c)
    {
        const float q[3] = {(c & 1) ? m.hi.x : m.lo.x, (c & 2) ? m.hi.y : m.lo.y, (c & 4) ? m.hi.z : m.lo.z};
        const auto w = turned(q, p.yaw, p.pitch, p.roll);
        low = std::min(low, w[1]), high = std::max(high, w[1]);
        across = std::max(across, std::sqrt(w[0] * w[0] + w[2] * w[2]));
    }
}

// An object's faces in the world: its shelf item's triangles (or your own model's), turned and placed.
const std::vector<PersonalWorld::Face>& PersonalWorld::faces_of(const Placed& p) const
{
    const float at[6] = {p.x, p.y, p.z, p.yaw, p.pitch, p.roll};
    if (std::equal(at, at + 6, p.face_at) && p.face_kind == p.kind) return p.faces;
    std::copy(at, at + 6, p.face_at);
    p.face_kind = p.kind;
    std::vector<Face>& out = p.faces;
    out.clear();
    const Mesh* m = mesh_of(p.kind);
    if (!m) return out;
    for (const MeshTri& t : m->tris)
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
    return out;
}

// Where a favourite's picture goes on it, in the world: on its front (+X), its line's face rectangle
// (as the hallway puts the picture on the item on its shelf), a few millimetres proud of the face;
// corners top left, top right, bottom right, bottom left as you face it (your right is its -Z), and
// the front's outward normal.
bool PersonalWorld::picture_quad(const Placed& p, std::array<std::array<float, 3>, 4>& w, std::array<float, 3>& n) const
{
    const auto it = shelf_items().mesh.find(mesh_kind(p.kind));
    if (it == shelf_items().mesh.end()) return false;
    const std::string medium = mesh_kind(p.kind);
    auto fit = face_rects_.find(medium);
    if (fit == face_rects_.end()) fit = face_rects_.emplace(medium, load_face_rect(medium)).first;
    const FaceRect& fr = fit->second;
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
    for (size_t i = 0; i < placed_.size(); ++i)
    {
        if (int(i) == skip) continue;
        const Placed& o = placed_[i];
        float low, high, across;
        extent_of(o, low, high, across);
        if ((o.x - x) * (o.x - x) + (o.z - z) * (o.z - z) > across * across + 0.01f || o.y + low > top) continue;
        for (const auto& [t, c] : faces_of(o))
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
    for (size_t i = 0; i < placed_.size(); ++i)
    {
        if (int(i) == skip) continue;
        if (is_char(placed_[i].kind))
        {
            const float t = char_hit(placed_[i], o, d);
            if (t > 0 && t < bt)
            {
                bt = t;
                if (which) *which = int(i);
            }
            continue;
        }
        {
            // (nothing farther across than the reach and its own size can be met within the reach)
            float low, high, across;
            extent_of(placed_[i], low, high, across);
            const float hx = placed_[i].x - o[0], hz = placed_[i].z - o[2], r = reach + across + 0.01f;
            if (hx * hx + hz * hz > r * r) continue;
        }
        for (const auto& [t, c] : faces_of(placed_[i]))
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

// A line from `o` along `d` (a unit vector) against a character's spheres: the nearest it meets, or -1.
float PersonalWorld::char_hit(const Placed& p, const float o[3], const float d[3]) const
{
    const sieve::Character* c = character(p.kind);
    if (!c) return -1;
    float best = -1;
    each_sphere(*c, p.yaw, p.pitch, p.roll, [&](const sieve::Sphere&, const std::array<float, 3>& w, float r) {
        const float m[3] = {o[0] - (p.x + w[0]), o[1] - (p.y + w[1]), o[2] - (p.z + w[2])};
        const float b = m[0] * d[0] + m[1] * d[1] + m[2] * d[2], cc = m[0] * m[0] + m[1] * m[1] + m[2] * m[2] - r * r, disc = b * b - cc;
        if (disc < 0) return;
        const float t = -b - std::sqrt(disc);
        if (t > 0 && (best < 0 || t < best)) best = t;
    });
    return best;
}

// Your characters all where a world without them placed by hand stands them (so nothing of them need
// be written).
bool PersonalWorld::chars_at_default() const
{
    size_t n = 0;
    for (const Placed& p : placed_)
    {
        if (!is_char(p.kind)) continue;
        ++n;
        bool found = false;
        for (const Placed& d : default_chars_)
            found = found || (d.kind == p.kind && d.x == p.x && d.z == p.z && d.yaw == p.yaw && p.pitch == 0 && p.roll == 0 && !p.physics && std::fabs(p.y - d.y) < 1e-4f);
        if (!found) return false;
    }
    return n == default_chars_.size();
}

int PersonalWorld::aimed() const
{
    // Along the crosshair from your eye: the nearest face of anything placed, within 6 m (each object's
    // own triangles, so a large thing does not hide a small one before it).
    const float o[3] = {px_, height(px_, pz_) + kEye, pz_};
    const Vec3 look = eye_camera().forward();
    const float d[3] = {look.x, look.y, look.z};
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
    if (in_place_state_ == InPlace::State::Open && in_place_)
    {
        // The viewer open in place (IDEAS 19.208): the key is its, as a key pressed in run() would be.
        SDL_Event e{};
        e.type = SDL_EVENT_KEY_DOWN;
        e.key.key = k;
        e.key.mod = mod;
        e.key.down = true;
        in_place_->event(e);
        in_place_frame();
        return;
    }
    if (k == SDLK_ESCAPE) escape();
    else if (pause_open_) pause_key(k);
    else key(k, mod);
}

// ---- the pause menu (pause_rows.hpp: the same rows as the hallway's, less its tools unless asked for)

void PersonalWorld::escape()
{
    if (in_place_state_ == InPlace::State::Making && in_place_)
    {
        // A favourite still being made to open in place: not opened when it is (its line still kept).
        in_place_->cancel();
        in_place_state_ = InPlace::State::Idle;
        in_place_last_ = "not opened: Esc while it was made";
        return;
    }
    if (pause_open_)
    {
        pause_open_ = false;
        SDL_SetWindowRelativeMouseMode(window_, true);
        return;
    }
    if (cat_panel_.open || place_panel_.open || day_panel_.open || worlds_panel_.open)
    {
        cat_panel_.open = place_panel_.open = day_panel_.open = worlds_panel_.open = false;
        day_edit_.clear();
        return;
    }
    if (held_ >= 0)
    {
        put_back_held(); // (put back where it was)
        return;
    }
    pause_rows_ = pause_rows(PausePlace::Personal, hallway_tools_);
    pause_open_ = true;
    mouse_shape_ = 0; // (a button let go while paused is not seen: the shaper stops here)
    if (shaped_unsaved_) shaped_unsaved_ = false, save();
    pause_row_ = 0;
    SDL_SetWindowRelativeMouseMode(window_, false);
}

bool PersonalWorld::pause_select(const std::string& id)
{
    if (!pause_open_) return false;
    const int i = pause_row_named(pause_rows_, id);
    if (i < 0) return false;
    pause_row_ = i;
    return true;
}

void PersonalWorld::pause_key(SDL_Keycode k)
{
    bool choose = false;
    pause_row_ = pause_row_step(k, pause_row_, int(pause_rows_.size()), choose);
    if (choose) pause_choose(pause_row_);
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

void PersonalWorld::pick_up(int which)
{
    held_ = which;
    held_from_ = placed_[size_t(which)];
    note_.clear();
    turn_ = -1, wheel_left_ = 0;
}

// ---- turning what you hold (IDEAS 19.221)

void PersonalWorld::set_turning(bool rings, int wheel_step_degrees, const uint32_t axis_argb[3])
{
    turn_rings_ = rings;
    wheel_step_ = float(std::clamp(wheel_step_degrees, 1, 90)) * kPi / 180.0f;
    for (int k = 0; k < 3; ++k) axis_argb_[k] = axis_argb[k];
}

void PersonalWorld::wheel(float notches)
{
    if (held_ < 0 && tool_out_ && !pause_open_)
    {
        // The ground shaper out (IDEAS 19.209): its radius, two metres a whole notch (a fifth of a cell).
        wheel_left_ += notches;
        const int whole = int(wheel_left_);
        wheel_left_ -= float(whole);
        shaper_radius_ = std::clamp(shaper_radius_ + 2.0f * float(whole), float(kShaperRadiusMin), float(kShaperRadiusMax));
        return;
    }
    if (held_ < 0 || turn_ < 0 || pause_open_)
    {
        wheel_left_ = 0;
        return;
    }
    // Whole notches only, so a touchpad's small movements add up to the same steps a wheel's do, and
    // the turns stay on the step's grid as the keys' do.
    wheel_left_ += notches;
    const int whole = int(wheel_left_);
    if (whole == 0) return;
    wheel_left_ -= float(whole);
    Placed& p = placed_[size_t(held_)];
    float& turn = turn_ == 0 ? p.yaw : turn_ == 1 ? p.pitch : p.roll;
    turn += float(whole) * wheel_step_;
}

bool PersonalWorld::held_bounds(float centre[3], float& reach) const
{
    if (held_ < 0) return false;
    const Placed& p = placed_[size_t(held_)];
    // Every point of it in the world: its faces' corners, or a character's spheres (each as its centre
    // and radius, so the reach is to the far side of the sphere).
    std::vector<std::array<float, 4>> pts;
    if (const sieve::Character* c = is_char(p.kind) ? character(p.kind) : nullptr)
        each_sphere(*c, p.yaw, p.pitch, p.roll, [&](const sieve::Sphere&, const std::array<float, 3>& w, float r) { pts.push_back({p.x + w[0], p.y + w[1], p.z + w[2], r}); });
    else
        for (const auto& [t, col] : faces_of(p))
            for (const auto& q : t) pts.push_back({q[0], q[1], q[2], 0});
    if (pts.empty()) return false;
    float lo[3] = {INFINITY, INFINITY, INFINITY}, hi[3] = {-INFINITY, -INFINITY, -INFINITY};
    for (const auto& q : pts)
        for (int k = 0; k < 3; ++k) lo[k] = std::min(lo[k], q[size_t(k)] - q[3]), hi[k] = std::max(hi[k], q[size_t(k)] + q[3]);
    for (int k = 0; k < 3; ++k) centre[k] = (lo[k] + hi[k]) / 2;
    reach = 0;
    for (const auto& q : pts)
    {
        const float dx = q[0] - centre[0], dy = q[1] - centre[1], dz = q[2] - centre[2];
        reach = std::max(reach, std::sqrt(dx * dx + dy * dy + dz * dz) + q[3]);
    }
    return true;
}

// A ring for each way what you hold turns, in the plane that turn moves it in: the yaw's level (about
// the vertical, y), the pitch's about its side as the yaw has turned it (z), the roll's about its front
// as the yaw and the pitch have turned it (x): the axes the keys turn it about, in the order turned()
// takes them, so each ring shows exactly the way its key moves it. Coloured by the axis (the setting),
// the turn last chosen bright and doubled, the others faint. They go to the renderer with the objects,
// so the half of a ring behind the thing is hidden by it, which shows which way round it runs.
void PersonalWorld::add_rings()
{
    float c[3], reach = 0;
    if (!held_bounds(c, reach)) return;
    const float radius = ring_radius(reach);
    const Placed& p = placed_[size_t(held_)];
    const float ex[3] = {1, 0, 0}, ey[3] = {0, 1, 0}, ez[3] = {0, 0, 1};
    for (int turn = 0; turn < 3; ++turn)
    {
        std::array<float, 3> u, v;
        if (turn == 0) u = {1, 0, 0}, v = {0, 0, 1};
        else if (turn == 1) u = {0, 1, 0}, v = turned(ex, p.yaw, 0, 0);
        else u = turned(ey, p.yaw, p.pitch, 0), v = turned(ez, p.yaw, p.pitch, 0);
        const uint32_t rgb = axis_argb_[turn == 0 ? 1 : turn == 1 ? 2 : 0] & 0x00FFFFFFu;
        const bool bright = turn == turn_;
        const uint32_t argb = (bright ? 0xFF000000u : 0x90000000u) | rgb;
        constexpr int kSides = 72;
        for (int line = 0; line < (bright ? 2 : 1); ++line) // (the bright one twice, a hair apart: a thicker line)
        {
            const float r = radius * (line ? 1.012f : 1.0f);
            Vec3 prev{};
            for (int k = 0; k <= kSides; ++k)
            {
                const float a = 2 * kPi * float(k) / kSides, ca = std::cos(a) * r, sa = std::sin(a) * r;
                const Vec3 q{c[0] + u[0] * ca + v[0] * sa, c[1] + u[1] * ca + v[1] * sa, c[2] + u[2] * ca + v[2] * sa};
                if (k > 0) batch_.add_edge(prev, q, argb);
                prev = q;
            }
        }
    }
}

std::string PersonalWorld::turning_report() const
{
    if (held_ < 0) return "";
    const Placed& p = placed_[size_t(held_)];
    static const char* const kChosen[3] = {"yaw (y)", "pitch (z)", "roll (x)"};
    const float deg = 180.0f / kPi;
    char b[200];
    std::snprintf(b, sizeof b, "%s chosen, wheel %g degrees a notch; yaw %.2f, pitch %.2f, roll %.2f degrees", turn_ < 0 ? "none" : kChosen[turn_], double(wheel_step_ * deg),
                  double(p.yaw * deg), double(p.pitch * deg), double(p.roll * deg));
    std::string t = b;
    float c[3], reach = 0;
    if (!turn_rings_) t += "; rings off";
    else if (held_bounds(c, reach))
    {
        std::snprintf(b, sizeof b, "; rings %.2f m about %.2f, %.2f, %.2f, x #%06X, y #%06X, z #%06X", double(ring_radius(reach)), double(c[0]), double(c[1]), double(c[2]),
                      unsigned(axis_argb_[0] & 0xFFFFFFu), unsigned(axis_argb_[1] & 0xFFFFFFu), unsigned(axis_argb_[2] & 0xFFFFFFu));
        t += b;
    }
    return t;
}

void PersonalWorld::place_held()
{
    turn_ = -1, wheel_left_ = 0; // (the wheel turns nothing until the next thing held has a turn chosen)
    Placed& p = placed_[size_t(held_)];
    p.physics = false, p.resting = true;
    note_ = trf("pworld.placed_here", {label_of(p.kind)});
    held_ = -1;
    place_panel_.open = false;
    save();
}

void PersonalWorld::drop_held()
{
    turn_ = -1, wheel_left_ = 0; // (the wheel turns nothing until the next thing held has a turn chosen)
    Placed& p = placed_[size_t(held_)];
    p.physics = true, p.resting = false, p.vx = p.vy = p.vz = 0;
    note_ = trf("pworld.dropped", {label_of(p.kind)});
    held_ = -1;
    place_panel_.open = false;
    save();
}

void PersonalWorld::put_back_held()
{
    turn_ = -1, wheel_left_ = 0; // (the wheel turns nothing until the next thing held has a turn chosen)
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
    // Day and night (IDEAS 19.222): while its panel is open it takes the keys (digits, Return, Backspace);
    // its key opens it where nothing else is open or held. With day and night off, the key is nothing.
    if (day_on_ && day_panel_.open)
    {
        day_key(k, mod);
        return;
    }
    // Your world on the worlds line (IDEAS 19.187): its key opens the panel (the unit worked out as the
    // world stands) and closes it; while it is open, it takes the keys (Return: go to it).
    if (K.is(pk::kWorldsAddress, k, mod) && (held_ < 0 || worlds_panel_.open))
    {
        if (worlds_panel_.open) worlds_panel_.open = false;
        else open_worlds();
        return;
    }
    if (worlds_panel_.open)
    {
        if (K.is(pk::kWorldsGo, k, mod)) worlds_key(k, mod);
        return;
    }
    if (day_on_ && held_ < 0 && K.is(pk::kDayLength, k, mod))
    {
        day_panel_.open = true, cat_panel_.open = false, day_edit_.clear();
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
    // The ground shaper (IDEAS 19.209): taken out and put away while nothing is in your hand and the
    // catalogue is shut; while out, its radius changed a step a press. Only an author's: a visitor is told.
    if (held_ < 0 && !cat_panel_.open)
    {
        if (K.is(pk::kToolOut, k, mod))
        {
            if (!author_) note_ = tr("pworld.shaper.not_author");
            else tool_out_ = true, note_.clear();
            return;
        }
        if (K.is(pk::kToolAway, k, mod))
        {
            tool_out_ = false, mouse_shape_ = 0;
            return;
        }
        if (tool_out_ && (K.is(pk::kBrushBigger, k, mod) || K.is(pk::kBrushSmaller, k, mod)))
        {
            wheel(K.is(pk::kBrushBigger, k, mod) ? 1.0f : -1.0f);
            return;
        }
    }

    const int n = int(rows_.size());
    if (held_ >= 0)
    {
        // In your hand: turned 15 degrees a press, then placed where it is or dropped; put back.
        Placed& p = placed_[size_t(held_)];
        const float step = kPi / 12;
        // Each press also chooses that turn for the wheel and its ring (IDEAS 19.221).
        if (K.is(pk::kTurn, k, mod)) p.yaw += step, turn_ = 0;
        else if (K.is(pk::kTurnBack, k, mod)) p.yaw -= step, turn_ = 0;
        else if (K.is(pk::kTip, k, mod)) p.pitch += step, turn_ = 1;
        else if (K.is(pk::kTipBack, k, mod)) p.pitch -= step, turn_ = 1;
        else if (K.is(pk::kRoll, k, mod)) p.roll += step, turn_ = 2;
        else if (K.is(pk::kRollBack, k, mod)) p.roll -= step, turn_ = 2;
        else if (K.is(pk::kPlaceHere, k, mod)) place_held();
        else if (K.is(pk::kDrop, k, mod)) drop_held();
        else if (K.is(pk::kPutBack, k, mod)) put_back_held();
        return;
    }
    if (!cat_panel_.open && (K.is(pk::kPickUp, k, mod) || K.is(pk::kOpen, k, mod)))
    {
        const int a = aimed();
        if (a < 0) return;
        if (K.is(pk::kPickUp, k, mod) && author_) tool_out_ = false, mouse_shape_ = 0, pick_up(a); // (the shaper put away: a hand holds one thing)
        else if (placed_[size_t(a)].kind.rfind("fav:", 0) == 0 && favourite(placed_[size_t(a)].kind.substr(4)))
        {
            const std::string sha = placed_[size_t(a)].kind.substr(4);
            if (!in_place_) opened_ = sha; // (read on its own line: run() goes there)
            else if (in_place_state_ != InPlace::State::Making)
            {
                // In place (IDEAS 19.208): its line made, or the one kept, and it opened over the world.
                in_place_sha_ = sha;
                in_place_last_.clear();
                shape_keys_wait_ = true, mouse_shape_ = 0; // (the shaper's keys close the viewer too)
                in_place_since_ = SDL_GetTicks();
                in_place_state_ = InPlace::State::Making;
                in_place_->begin(*favourite(sha));
                in_place_frame(); // (a line kept opens at once)
            }
        }
        else
        {
            // (a visitor's E too) A character, and a favourite no longer kept, each say why they do not open.
            const std::string& kind = placed_[size_t(a)].kind;
            note_ = trf(is_char(kind) ? "pworld.open.char" : is_model(kind) ? "pworld.open.model" : kind.rfind("fav:", 0) == 0 ? "pworld.open.gone" : "pworld.open", {label_of(kind)});
        }
        return;
    }
    if (cat_panel_.open && K.is(pk::kImport, k, mod))
    {
        // A model of your own from a file (an author's: the store is theirs): the system's file dialog.
        if (file_.empty()) note_ = tr("pworld.import.no_store");
        else if (author_) import_dialog();
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
            // (a model of yours not in the store is kept, unseen, until it is: not taken away unseen)
            if (is_model(placed_[i].kind) && !mesh_of(placed_[i].kind)) continue;
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

void PersonalWorld::save()
{
    if (file_.empty()) return;
    // v1 while everything stands on the ground, turned only about the vertical and placed; v2 (the
    // 3D place, three turns, placed or dropped) as soon as anything is more, or the file held lines
    // this build keeps without reading.
    // Your characters are written only once they are not all where they stand by default (then all of
    // them, and "characters placed", so one taken away stays away).
    const bool chars = characters_placed_ || !chars_at_default();
    // A day's length set (IDEAS 19.222) is a v2 line, so a world with one is v2.
    // So is ground shaped (IDEAS 19.209).
    const bool shaped = shaped_corners() > 0;
    // So is a model of your own (its `model` line, and model:SHA where it is placed).
    // So is music of its own (IDEAS 19.220): the Media Player's, where it is editing this world.
    if (MusicPlayer* mp = music(); mp && !file_.empty() && mp->personal_world() == file_)
    {
        // (the line as read, unless the music has changed since: then as the player writes it)
        const auto own = mp->personal();
        const auto mine = music_line_.empty() ? std::nullopt : personal_music_of(music_line_);
        const std::string now = own ? personal_music_line(*own) : std::string(), was = mine ? personal_music_line(*mine) : std::string();
        if (now != was) music_line_ = now;
    }
    bool v2 = !kept_lines_.empty() || chars || day_written_ || shaped || !models_.empty() || !music_line_.empty();
    for (const Placed& p : placed_)
        if (!is_char(p.kind)) v2 = v2 || p.kind.rfind("fav:", 0) == 0 || p.physics || p.pitch != 0 || p.roll != 0 || std::fabs(p.y - ground(p.x, p.z)) > 1e-4f;
    std::error_code ec;
    std::filesystem::create_directories(file_.parent_path(), ec);
    std::ofstream out(file_, std::ios::binary);
    out << (v2 ? "sieve-personal-world-v2" : "sieve-personal-world-v1") << "\nseed " << seed_ << "\n";
    if (day_written_) out << "day " << day_length_ << "\n";
    if (shaped)
    {
        // Each corner the shaper moved, by its place and how far (only those: the change, not the grid).
        const int n = int(2 * kHalf / kCell);
        for (int i = 0; i <= n; ++i)
            for (int j = 0; j <= n; ++j)
                if (const float d = shaped_[size_t(i) * size_t(n + 1) + size_t(j)]; d != 0)
                    out << "ground " << int(-kHalf + float(i) * kCell) << " " << int(-kHalf + float(j) * kCell) << " " << sieve::nine_digits(d) << "\n";
    }
    for (const std::string& sha : models_) out << "model " << sha << "\n"; // (before anything placed that names it)
    if (!music_line_.empty()) out << music_line_ << "\n";
    for (const std::string& l : kept_lines_) out << l << "\n";
    if (chars) out << "characters placed\n";
    for (const Placed& p : placed_)
        if (is_char(p.kind) && !chars) continue;
        else if (v2)
            out << "place " << p.kind << " " << sieve::nine_digits(p.x) << " " << sieve::nine_digits(p.y) << " " << sieve::nine_digits(p.z) << " " << sieve::nine_digits(p.yaw) << " " << sieve::nine_digits(p.pitch) << " " << sieve::nine_digits(p.roll) << " "
                << (p.physics ? "physics" : "fixed") << "\n";
        else out << "place " << p.kind << " " << sieve::nine_digits(p.x) << " " << sieve::nine_digits(p.z) << " " << sieve::nine_digits(p.yaw) << "\n";
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
        if (version == 2 && word == "day")
        {
            // The day's length (IDEAS 19.222): whole seconds, 1 to 2^64 - 1, nothing after it. One that
            // cannot be read is left out and said, as any line known but not readable is.
            std::string n, rest;
            uint64_t v = 0;
            // (as written: no sign, no leading zeros, so "060" is not as written)
            if ((ss >> n) && !(ss >> rest) && n[0] != '0' && parse_day_length(n, v)) day_length_ = v, day_written_ = true;
            else ++skipped;
            continue;
        }
        if (version == 2 && word == "ground")
        {
            // A cell corner the ground shaper moved (IDEAS 19.209): X and Z whole metres on the 10 m grid,
            // -320 to 320, then the change in metres (at most 500 either way), nothing after it; a corner
            // given twice, or anything else not so written, is left out and said.
            const int n = int(2 * kHalf / kCell);
            long long gx = 0, gz = 0;
            float d = 0;
            std::string rest, sx, sz;
            // (X and Z as written: a sign only when negative, no leading zeros)
            const auto as_written = [](const std::string& s, long long& v) {
                if (s.empty() || s.size() > 6) return false;
                for (size_t i = 0; i < s.size(); ++i)
                    if (!(s[i] >= '0' && s[i] <= '9') && !(i == 0 && s[i] == '-' && s.size() > 1)) return false;
                v = std::stoll(s);
                return std::to_string(v) == s;
            };
            const bool ok = bool(ss >> sx >> sz >> d) && as_written(sx, gx) && as_written(sz, gz) && !(ss >> rest) && std::isfinite(d) && std::fabs(d) <= 500.0f && gx >= -kHalf && gx <= kHalf && gz >= -kHalf && gz <= kHalf &&
                            (gx + (long long)kHalf) % (long long)kCell == 0 && (gz + (long long)kHalf) % (long long)kCell == 0;
            if (!ok)
            {
                ++skipped;
                continue;
            }
            if (shaped_.empty()) shaped_.assign(size_t(n + 1) * size_t(n + 1), 0.0f);
            const size_t k = size_t((gx + (long long)kHalf) / (long long)kCell) * size_t(n + 1) + size_t((gz + (long long)kHalf) / (long long)kCell);
            if (shaped_[k] != 0 || d == 0) ++skipped; // (twice, or a change of nothing: not as written)
            else shaped_[k] = d;
            continue;
        }
        const auto& cat = catalogue();
        if (version == 2 && word == "characters" && (ss >> how) && how == "placed")
        {
            characters_placed_ = true;
            continue;
        }
        if (version == 2 && word == "model")
        {
            // A model of your own in the catalogue: its SHA (64 hex digits, small letters), nothing after
            // it, named once. Its .sieve is read from the store when it is first drawn; one not there is
            // still listed (and written back), drawn as nothing until it is.
            std::string sha, rest;
            if ((ss >> sha) && !(ss >> rest) && is_sha(sha) && std::find(models_.begin(), models_.end(), sha) == models_.end()) models_.push_back(sha);
            else ++skipped;
            continue;
        }
        if (version == 2 && word == "music")
        {
            // Its music (IDEAS 19.220): one line, every value as the Media Player could set it; another
            // after the first, or one not so written, is left out and said.
            if (music_line_.empty() && personal_music_of(l)) music_line_ = l;
            else ++skipped;
            continue;
        }
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
        const bool model = version == 2 && is_model(p.kind) && is_sha(p.kind.substr(6));
        if (ok && (std::find(cat.begin(), cat.end(), p.kind) != cat.end() || (version == 2 && p.kind.rfind("fav:", 0) == 0 && p.kind.size() == 68) ||
                   (version == 2 && (p.kind == "char:you" || (is_char(p.kind) && p.kind.size() == 69))) || model))
        {
            placed_.push_back(p);
            // (a model placed with no `model` line before it: listed all the same, so it can be placed again)
            if (model && std::find(models_.begin(), models_.end(), p.kind.substr(6)) == models_.end()) models_.push_back(p.kind.substr(6));
        }
        else ++skipped;
    }
    if (skipped) note_ = trf("pworld.skipped", {std::to_string(skipped)});
}

std::string PersonalWorld::music_report() const
{
    if (music_line_.empty()) return "";
    const auto m = personal_music_of(music_line_);
    return m ? "its own: " + personal_music_summary(*m) : "";
}

std::string PersonalWorld::write_music(const std::filesystem::path& world, const std::string& line)
{
    if (world.empty()) return tr("pworld.music.no_world");
    // (the world as it is, read and written back as it writes itself: a v1 world becomes v2 with the
    // line, as it would with any other v2 line; no characters given, so those it names are kept as
    // lines are and the default ones written as they would be: not at all)
    PersonalWorld w(nullptr, nullptr, {}, false, world);
    // A world with lines its reading left out is not written: writing it back would drop them without a
    // word (the dimension says so when it opens; here nothing would). The music stays unwritten until the
    // world is opened, what it left out seen to, and saved.
    if (!w.note_.empty()) return tr("pworld.music.not_written") + " " + w.note_;
    w.music_line_ = line;
    w.save();
    return w.note_;
}

std::string PersonalWorld::music_line_of(const std::filesystem::path& world)
{
    std::ifstream in(world, std::ios::binary);
    std::string l;
    if (!std::getline(in, l)) return "";
    if (!l.empty() && l.back() == '\r') l.pop_back();
    if (l != "sieve-personal-world-v2") return "";
    while (std::getline(in, l))
    {
        if (!l.empty() && l.back() == '\r') l.pop_back();
        if (l.rfind("music ", 0) == 0 || l == "music") return personal_music_of(l) ? l : "";
    }
    return "";
}

// ---- a favourite opened in place (IDEAS 19.208)

void PersonalWorld::in_place_frame()
{
    if (!in_place_ || in_place_state_ == InPlace::State::Idle) return;
    const InPlace::State s = in_place_->poll();
    if (s == InPlace::State::Failed)
    {
        // It cannot open here: the old way, into the hallway on its line (run() goes there), and why.
        in_place_last_ = "the hallway instead: " + in_place_->why();
        note_ = trf("pworld.inplace.fell", {in_place_->why()});
        opened_ = in_place_sha_;
        in_place_state_ = InPlace::State::Idle;
        return;
    }
    if (in_place_state_ == InPlace::State::Open && s != InPlace::State::Open)
    {
        // Closed (Esc or Z): back to the world, the mouse looking about again as the viewer found it.
        in_place_last_ = "closed, back in your dimension";
        in_place_state_ = InPlace::State::Idle;
        return;
    }
    in_place_state_ = s;
}

std::string PersonalWorld::in_place_report() const
{
    if (in_place_sha_.empty()) return "";
    const Favourite* f = favourite(in_place_sha_);
    const std::string what = (f ? f->line : std::string("?")) + " " + in_place_sha_;
    switch (in_place_state_)
    {
    case InPlace::State::Making: return "calculating " + what;
    case InPlace::State::Open: return "open, " + what + "; " + (in_place_ ? in_place_->report() : std::string());
    default: return what + ": " + in_place_last_;
    }
}

std::string PersonalWorld::report() const
{
    // (your characters counted apart from the objects, though they are placed as objects are)
    size_t chars = 0;
    for (const Placed& p : placed_) chars += is_char(p.kind) ? 1 : 0;
    char b[160];
    std::snprintf(b, sizeof b, "seed %llu; you at %.1f, %.1f, ground %.2f m; %zu characters; %zu objects placed", (unsigned long long)seed_, double(px_), double(pz_),
                  double(height(px_, pz_)), chars, placed_.size() - chars);
    std::string t = b;
    if (const size_t c = shaped_corners(); c > 0) t += "; ground shaped at " + std::to_string(c) + " corners"; // (IDEAS 19.209; nothing said where none is)
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

// Your world as one item of the worlds line (IDEAS 19.187; the rest in personal_worlds.cpp): each object
// placed, but for your characters, as its box. A kind with no mesh (a model no longer in the store) is
// the size extent_fresh gives it.
PersonalWorld::WorldsUnit PersonalWorld::worlds_unit(const sieve::WorldSpace& ws) const
{
    std::vector<WorldsBox> boxes;
    std::vector<const Placed*> objects;
    size_t chars = 0;
    for (const Placed& p : placed_)
    {
        if (is_char(p.kind))
        {
            ++chars;
            continue;
        }
        Vec3 lo{-0.1f, 0, -0.1f}, hi{0.1f, 0.4f, 0.1f};
        if (const Mesh* m = mesh_of(p.kind)) lo = m->lo, hi = m->hi;
        WorldsBox b;
        const float mid[3] = {(lo.x + hi.x) / 2, (lo.y + hi.y) / 2, (lo.z + hi.z) / 2};
        const auto c = turned(mid, p.yaw, p.pitch, p.roll);
        b.centre[0] = p.x + c[0], b.centre[1] = p.y + c[1], b.centre[2] = p.z + c[2];
        b.half[0] = (hi.x - lo.x) / 2, b.half[1] = (hi.y - lo.y) / 2, b.half[2] = (hi.z - lo.z) / 2;
        // Its turn as a matrix, column j where its own axis j goes. turned() takes an object's own x, y and
        // z (front, up, side) onto the world's as a mirror would (unturned, its front is the world's +z and
        // its side +x), and the worlds line's 24 turns are turns, never mirrors; a box is the same box
        // mirrored through its own middle, so its side is taken the other way round (the third column
        // negated), which makes it a turn and changes nothing of where the box is.
        for (int j = 0; j < 3; ++j)
        {
            const float e[3] = {j == 0 ? 1.0f : 0.0f, j == 1 ? 1.0f : 0.0f, j == 2 ? 1.0f : 0.0f};
            const auto col = turned(e, p.yaw, p.pitch, p.roll);
            for (int i = 0; i < 3; ++i) b.turn[i][j] = (j == 2 ? -1.0f : 1.0f) * col[size_t(i)];
        }
        boxes.push_back(b);
        objects.push_back(&p);
    }
    WorldsUnit u = worlds_unit(ws, boxes);
    u.objects = objects.size();
    u.characters = chars;
    for (size_t i = 0; i < u.carried; ++i)
    {
        u.favourites += objects[i]->kind.rfind("fav:", 0) == 0 ? 1 : 0;
        u.own += is_model(objects[i]->kind) ? 1 : 0;
        u.dropped += objects[i]->physics ? 1 : 0;
    }
    return u;
}

PersonalWorld::Result PersonalWorld::run()
{
    SDL_SetWindowRelativeMouseMode(window_, true);
    // Its own music where it has some (the Media Player's PERSONAL, IDEAS 19.220), else WORLD's as before.
    if (MusicPlayer* mp = music()) mp->set_personal(file_, music_line_.empty() ? std::nullopt : personal_music_of(music_line_));
    music_mode(MusicMode::Personal);
    Uint64 last = SDL_GetTicksNS();
    system_.clear();
    worlds_go_.reset(); // (the last Go to it taken already, by whoever ran this)
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
            if (in_place_state_ == InPlace::State::Open && in_place_)
            {
                // A favourite open in the hallway's viewer over the world (IDEAS 19.208): the keys and
                // the mouse are the viewer's until Esc or Z closes it.
                in_place_->event(e);
                in_place_frame();
                continue;
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
                    if (const int i = pause_row_at(r_, e, pause_rects_); i >= 0)
                    {
                        pause_row_ = i;
                        if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN && e.button.button == SDL_BUTTON_LEFT) pause_choose(i);
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
            if (worlds_go_)
            {
                // The worlds line's Go to it (IDEAS 19.187): into the hallway there, whoever ran this
                // taking worlds_parts(); the panel closed for when you come back.
                save();
                worlds_panel_.open = false, worlds_panel_.shown = 0;
                SDL_SetWindowRelativeMouseMode(window_, false);
                return Result::GoToWorld;
            }
            if (tool_out_ && held_ < 0 && (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN || e.type == SDL_EVENT_MOUSE_BUTTON_UP))
            {
                // The ground shaper out (IDEAS 19.209): left raises, right lowers, middle flattens, while held.
                const int mode = e.button.button == SDL_BUTTON_LEFT ? 1 : e.button.button == SDL_BUTTON_RIGHT ? 2 : e.button.button == SDL_BUTTON_MIDDLE ? 3 : 0;
                if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN && mode) mouse_shape_ = mode;
                else if (e.type == SDL_EVENT_MOUSE_BUTTON_UP && mode == mouse_shape_) mouse_shape_ = 0;
                continue;
            }
            if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN && e.button.button == SDL_BUTTON_LEFT)
            {
                // (a click: pick up what the crosshair is on, or place what you hold)
                if (held_ >= 0) place_held();
                else if (const int a = aimed(); a >= 0 && author_) pick_up(a);
            }
            if (e.type == SDL_EVENT_MOUSE_WHEEL) wheel(e.wheel.y);
            if (e.type == SDL_EVENT_MOUSE_MOTION)
            {
                yaw_ += e.motion.xrel * 0.0025f;
                pitch_ = std::clamp(pitch_ - e.motion.yrel * 0.0025f, -1.3f, 1.3f);
            }
        }
        if (import_waiting_ && !import_open_)
        {
            // The import key's dialog has answered (a file, or none): the mouse back to looking about,
            // and the file brought in.
            import_waiting_ = false;
            SDL_SetWindowRelativeMouseMode(window_, true);
            std::string from;
            {
                std::lock_guard<std::mutex> lock(import_mx_);
                from.swap(import_from_);
            }
            if (!from.empty()) import_model(from);
        }
        in_place_frame(); // (a favourite being made to open in place: made yet?)
        if (!opened_.empty())
        {
            // (it could not open in place: the old way, into the hallway on its line)
            save();
            SDL_SetWindowRelativeMouseMode(window_, false);
            return Result::Open;
        }
        const Uint64 now = SDL_GetTicksNS();
        const float dt = std::min(0.1f, float(double(now - last) / 1e9));
        last = now;
        // (paused, or the viewer open over it: all still)
        if (!pause_open_ && in_place_state_ != InPlace::State::Open) step(dt, SDL_GetKeyboardState(nullptr));
        render();
        present(r_);
    }
}

} // namespace hallway
