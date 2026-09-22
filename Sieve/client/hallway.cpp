// Sieve hallway — a wireframe walk along the four lines.
//
// Each line is an endless corridor lined with bookcases; every book is one unit and the address
// increases as you walk forward. Black doors in the walls lead to the next line (left wall) or the
// previous line (right wall), at the same fractional position; you come back in through the
// opposite door, in the next line's colours. One tile of geometry is built once and repeated.

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "camera.hpp"
#include "synth.hpp"
#include "world.hpp"

#include "cli/args.hpp"
#include "cli/image_io.hpp"
#include "cli/lines.hpp"

#include "sieve/audio.hpp"
#include "sieve/image.hpp"
#include "sieve/utf8.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

using namespace sieve;
using namespace sieve::cli;
using namespace hallway;

namespace {

constexpr float kPi = 3.14159265358979f;

// ---------------------------------------------------------------- themes

struct Theme
{
    SDL_Color bg, edge;
    const char* name;
};

// Two colours per line: a solid background and the colour of every edge. Doors are solid black.
constexpr Theme kThemes[4] = {
    {{0, 0, 0, 255}, {255, 255, 255, 255}, "TEXT"},
    {{0, 0, 140, 255}, {0, 255, 255, 255}, "IMAGE"},
    {{0, 90, 0, 255}, {255, 176, 0, 255}, "AUDIO"},
    {{140, 0, 0, 255}, {255, 255, 0, 255}, "VIDEO"},
};
constexpr LineKind kLineOrder[4] = {LineKind::Text, LineKind::Image, LineKind::Audio, LineKind::Video};

SDL_Color mix(SDL_Color a, SDL_Color b, float t)
{
    auto m = [t](Uint8 x, Uint8 y) { return Uint8(float(x) + (float(y) - float(x)) * t + 0.5f); };
    return {m(a.r, b.r), m(a.g, b.g), m(a.b, b.b), 255};
}

// ---------------------------------------------------------------- geometry built once

struct Segment
{
    Vec3 a, b;
};

std::vector<Segment> build_tile()
{
    std::vector<Segment> s;
    auto rect = [&](Vec3 p0, Vec3 p1, Vec3 p2, Vec3 p3) {
        s.push_back({p0, p1}); s.push_back({p1, p2}); s.push_back({p2, p3}); s.push_back({p3, p0});
    };
    for (float sx : {-1.0f, 1.0f})
    {
        const float wall = sx * kHalfWidth, face = sx * kCaseFront;
        // Floor and ceiling edges along the tile, and the frame across its start.
        s.push_back({{wall, 0, 0}, {wall, 0, kTile}});
        s.push_back({{wall, kHeight, 0}, {wall, kHeight, kTile}});
        s.push_back({{wall, 0, 0}, {wall, kHeight, 0}});
        // Bookcase: front outline, depth edges back to the wall, shelf boards.
        const float top = kRowTop + 0.2f;
        rect({face, 0, 0}, {face, 0, kShelfEnd}, {face, top, kShelfEnd}, {face, top, 0});
        for (float z : {0.0f, kShelfEnd})
            for (float y : {0.0f, top}) s.push_back({{face, y, z}, {wall, y, z}});
        for (int r = 0; r <= kRows; ++r)
        {
            const float y = kRowTop - r * kRowHeight;
            s.push_back({{face, y, 0}, {face, y, kShelfEnd}});
        }
        // Books.
        Vec3 f[4];
        for (int r = 0; r < kRows; ++r)
            for (int c = 0; c < kCols; ++c)
            {
                book_face(0, sx < 0 ? Side::Left : Side::Right, r, c, f);
                rect(f[0], f[1], f[2], f[3]);
            }
        // Door frame.
        rect({wall, 0, kDoorStart}, {wall, 0, kDoorEnd}, {wall, kDoorTop, kDoorEnd}, {wall, kDoorTop, kDoorStart});
    }
    s.push_back({{-kHalfWidth, 0, 0}, {kHalfWidth, 0, 0}});
    s.push_back({{-kHalfWidth, kHeight, 0}, {kHalfWidth, kHeight, 0}});
    // Centre-line marks on the floor give a sense of motion.
    for (float z = 0.5f; z < kTile; z += 2.0f) s.push_back({{0, 0, z}, {0, 0, z + 0.6f}});
    return s;
}

// ---------------------------------------------------------------- text helpers

std::string short_address(const std::string& hex)
{
    if (hex.size() <= 28) return hex;
    return hex.substr(0, 12) + "..." + hex.substr(hex.size() - 12) + " (" + std::to_string(hex.size()) + " digits)";
}

std::vector<std::string> wrap(const std::string& s, size_t width)
{
    std::vector<std::string> out;
    for (size_t i = 0; i < s.size(); i += width) out.push_back(s.substr(i, width));
    if (out.empty()) out.push_back("");
    return out;
}

std::string ascii(const std::string& utf8)
{
    std::string out;
    for (char32_t c : utf8_decode(utf8)) out.push_back(c < 0x80 ? char(c) : '?');
    return out;
}

std::string percent(double f)
{
    char buf[48];
    std::snprintf(buf, sizeof buf, "%.10f%%", f * 100.0);
    return buf;
}

// ---------------------------------------------------------------- the hallway

enum class Input { None, Warp, Goto };

struct Crossing
{
    int from_line;
    Space::Digits from_base;
    int64_t from_tile;
    Side exit_side;
    int to_line;
};

class Hallway
{
public:
    Hallway(SDL_Window* window, SDL_Renderer* renderer, std::vector<Line> lines)
        : window_(window), r_(renderer), lines_(std::move(lines)), tile_geometry_(build_tile())
    {
        // Bounds of one tile's geometry (plus its doors), for skipping tiles out of view.
        tile_lo_ = {-kHalfWidth, 0, 0};
        tile_hi_ = {kHalfWidth, kHeight, kTile};
        for (const Segment& g : tile_geometry_)
            for (const Vec3& v : {g.a, g.b})
            {
                tile_lo_ = {std::min(tile_lo_.x, v.x), std::min(tile_lo_.y, v.y), std::min(tile_lo_.z, v.z)};
                tile_hi_ = {std::max(tile_hi_.x, v.x), std::max(tile_hi_.y, v.y), std::max(tile_hi_.z, v.z)};
            }
    }

    // ---- state changes

    void set_line(int li) { li_ = li; invalidate(); }
    void set_mode(AddressMode m) { mode_ = m; invalidate(); }
    AddressMode mode() const { return mode_; }
    Camera& camera() { return cam_; }
    void set_tile(int64_t t) { tile_ = t; }

    const Line& line() const { return lines_[size_t(li_)]; }
    const Theme& theme() const { return kThemes[li_]; }

    void set_base(Space::Digits base)
    {
        base_ = std::move(base);
        tile_ = 0;
        invalidate();
    }
    void forget_doors() { crossings_.clear(); }

    // Stand in front of the first book of the walk (left wall, top shelf) and look at it.
    void face_first_book()
    {
        // Stand in the middle of the corridor, facing the left wall's first book (top shelf).
        cam_.pos = {0.3f, 1.6f, kBookPitch * 0.5f + 1.2f};
        const float dz = -1.2f, dx = -kCaseFront - 0.3f;
        cam_.yaw = std::atan2(dx, dz);
        const float book_y = kRowTop - kRowHeight + 0.02f + 0.17f;
        cam_.pitch = std::atan2(book_y - 1.6f, std::sqrt(dx * dx + dz * dz));
    }

    void message(const std::string& m)
    {
        message_ = m;
        message_until_ = SDL_GetTicks() + 6000;
    }

    // Everything the hallway shows about one book, computed once. Books are addressed by their
    // offset from the base unit: the address digits are stepped (no scrambling) and unscrambled
    // once, and the hex and fraction come from the same address digits.
    struct Book
    {
        Space::Digits unit, address;
        std::string hex;
        double fraction = 0;
    };

    const Book& book(int64_t offset)
    {
        const Space& sp = line().space;
        if (dirty_)
        {
            base_address_ = sp.address_digits(base_, mode_);
            dirty_ = false;
        }
        auto it = cache_.find(offset);
        if (it != cache_.end()) return it->second;
        if (cache_.size() > 512) cache_.clear();
        Book b;
        b.address = sp.step_address(base_address_, offset);
        b.unit = offset == 0 ? base_ : sp.unit_of_address(b.address, mode_);
        b.hex = sp.hex_of(b.address);
        b.fraction = sp.fraction_of(b.address);
        return cache_.emplace(offset, std::move(b)).first->second;
    }
    const Space::Digits& unit_at(int64_t offset) { return book(offset).unit; }

    bool warp(const std::string& input)
    {
        try
        {
            Args a;
            a.opts["line"] = to_string(line().kind);
            if (line().kind == LineKind::Image || line().kind == LineKind::Video) a.opts["file"] = input;
            else a.positional = {input};
            const WarpInput w = read_warp_input(line(), a);
            if (w.units.empty()) throw std::invalid_argument("nothing left after canonicalisation");
            trail_ = w.units;
            trail_index_ = 0;
            forget_doors();
            set_base(trail_[0]);
            face_first_book();
            in_hand_.reset();
            message("warped: " + w.report.front() + (trail_.size() > 1 ? "   (N / B: next / previous unit of the trail)" : ""));
            return true;
        }
        catch (const std::exception& e)
        {
            message(std::string("warp failed: ") + e.what());
            return false;
        }
    }

    bool go_to(std::string input)
    {
        try
        {
            input.erase(std::remove(input.begin(), input.end(), ' '), input.end());
            Space::Digits unit;
            if (!input.empty() && input.back() == '%')
            {
                // "36.5%" -> 365 / 10^3
                const std::string num = input.substr(0, input.size() - 1);
                const size_t dot = num.find('.');
                std::string digits = num;
                uint32_t decimals = 2;
                if (dot != std::string::npos)
                {
                    digits = num.substr(0, dot) + num.substr(dot + 1);
                    decimals += uint32_t(num.size() - dot - 1);
                }
                if (digits.empty() || digits.size() > 9 || digits.find_first_not_of("0123456789") != std::string::npos)
                    throw std::invalid_argument("write a percentage like 50% or 36.25% (up to 7 decimals)");
                unit = unit_at_fraction(line().space, std::stoull(digits), decimals, mode_);
            }
            else unit = line().space.unit_at(input, mode_);
            trail_.clear();
            forget_doors();
            set_base(unit);
            face_first_book();
            in_hand_.reset();
            message("went to " + input);
            return true;
        }
        catch (const std::exception& e)
        {
            message(std::string("go to failed: ") + e.what());
            return false;
        }
    }

    void step_trail(int dir)
    {
        if (trail_.size() < 2) return;
        trail_index_ = (trail_index_ + trail_.size() + size_t(dir)) % trail_.size();
        set_base(trail_[trail_index_]);
        face_first_book();
        in_hand_.reset();
        message("unit " + std::to_string(trail_index_ + 1) + " of " + std::to_string(trail_.size()) + " of the trail");
    }

    // ---- movement and doors

    void update(float dt, const bool* keys)
    {
        if (input_ != Input::None) return;
        const float speed = (keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT]) ? 9.0f : 3.0f;
        const Vec3 fwd = {std::sin(cam_.yaw), 0, std::cos(cam_.yaw)};
        const Vec3 right = {std::cos(cam_.yaw), 0, -std::sin(cam_.yaw)};
        Vec3 move{};
        if (keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_UP]) move = move + fwd;
        if (keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_DOWN]) move = move - fwd;
        if (keys[SDL_SCANCODE_D]) move = move + right;
        if (keys[SDL_SCANCODE_A]) move = move - right;
        if (keys[SDL_SCANCODE_LEFT]) cam_.yaw -= 1.8f * dt;
        if (keys[SDL_SCANCODE_RIGHT]) cam_.yaw += 1.8f * dt;
        if (dot(move, move) > 0) move_by(normalize(move) * (speed * dt));
    }

    void move_by(Vec3 d)
    {
        auto in_door = [](float z) { return z > kDoorStart + 0.15f && z < kDoorEnd - 0.15f; };
        Vec3 p = cam_.pos + d;
        // Walls stop you except in a doorway; once in a doorway you cannot slide along inside the wall.
        if (std::fabs(p.x) > kWalkLimit && !in_door(p.z))
        {
            if (std::fabs(cam_.pos.x) > kWalkLimit) p.z = cam_.pos.z;
            else p.x = std::clamp(p.x, -kWalkLimit, kWalkLimit);
        }
        cam_.pos = p;
        while (cam_.pos.z >= kTile) { cam_.pos.z -= kTile; ++tile_; }
        while (cam_.pos.z < 0) { cam_.pos.z += kTile; --tile_; }
        if (cam_.pos.x < -(kHalfWidth + 0.05f)) cross(Side::Left);
        else if (cam_.pos.x > kHalfWidth + 0.05f) cross(Side::Right);
    }

    void jump_tiles(int64_t n)
    {
        tile_ += n;
        message("jumped " + std::to_string(n) + " tiles (" + std::to_string(n * kBooksPerTile) + " units)");
    }

    void cross(Side side)
    {
        const Side back = side == Side::Left ? Side::Right : Side::Left;
        const float emerge = (kHalfWidth - 0.1f) * (side == Side::Left ? 1.0f : -1.0f);
        if (!crossings_.empty() && crossings_.back().to_line == li_ && crossings_.back().exit_side == back)
        {
            // Back through the door you came in by: return to exactly where you were. Every door you
            // passed is remembered, so a whole chain of doors can be retraced exactly.
            const Crossing c = crossings_.back();
            crossings_.pop_back();
            set_line(c.from_line);
            base_ = c.from_base;
            tile_ = c.from_tile;
            invalidate();
            cam_.pos.x = emerge; // out of the door you originally went through
            message(std::string("back in the ") + theme().name + " line, exactly where you left it");
            return;
        }
        const int to = side == Side::Left ? (li_ + 1) % 4 : (li_ + 3) % 4;
        const Book& here = book(tile_ * kBooksPerTile);
        Crossing c{li_, base_, tile_, side, to};
        const Space::Digits there = door_map(line().space, here.unit, mode_, lines_[size_t(to)].space);
        const double f = here.fraction;
        set_line(to);
        set_base(there);
        cam_.pos.x = emerge;
        crossings_.push_back(c);
        message(std::string("through the door into the ") + theme().name + " line, at the same " + percent(f) +
                " along. Go back through it to return.");
    }

    // ---- input events

    void handle(const SDL_Event& e, bool& quit)
    {
        if (e.type == SDL_EVENT_QUIT) quit = true;
        if (input_ != Input::None)
        {
            if (e.type == SDL_EVENT_TEXT_INPUT) text_ += e.text.text;
            if (e.type == SDL_EVENT_KEY_DOWN)
            {
                if (e.key.key == SDLK_ESCAPE) close_input();
                else if (e.key.key == SDLK_BACKSPACE && !text_.empty())
                {
                    // Remove one UTF-8 character.
                    do text_.pop_back();
                    while (!text_.empty() && (static_cast<unsigned char>(text_.back()) & 0xC0) == 0x80);
                }
                else if (e.key.key == SDLK_V && (e.key.mod & SDL_KMOD_CTRL))
                {
                    if (char* clip = SDL_GetClipboardText()) { text_ += clip; SDL_free(clip); }
                }
                else if (e.key.key == SDLK_RETURN || e.key.key == SDLK_KP_ENTER)
                {
                    const Input which = input_;
                    const std::string t = text_;
                    close_input();
                    if (!t.empty()) which == Input::Warp ? warp(t) : go_to(t);
                }
            }
            return;
        }
        if (e.type == SDL_EVENT_MOUSE_MOTION && SDL_GetWindowRelativeMouseMode(window_))
        {
            cam_.yaw += e.motion.xrel * 0.0025f;
            cam_.pitch = std::clamp(cam_.pitch - e.motion.yrel * 0.0025f, -1.45f, 1.45f);
        }
        if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
        {
            if (!SDL_GetWindowRelativeMouseMode(window_)) SDL_SetWindowRelativeMouseMode(window_, true);
            else if (e.button.button == SDL_BUTTON_LEFT) take_or_return();
        }
        if (e.type == SDL_EVENT_MOUSE_WHEEL && e.wheel.y != 0) jump_tiles(e.wheel.y > 0 ? 1 : -1);
        if (e.type != SDL_EVENT_KEY_DOWN) return;
        switch (e.key.key)
        {
        case SDLK_ESCAPE:
            if (in_hand_) { in_hand_.reset(); synth_.stop(); }
            else if (SDL_GetWindowRelativeMouseMode(window_)) SDL_SetWindowRelativeMouseMode(window_, false);
            break;
        case SDLK_TAB: SDL_SetWindowRelativeMouseMode(window_, !SDL_GetWindowRelativeMouseMode(window_)); break;
        case SDLK_Q:
            if (e.key.mod & SDL_KMOD_CTRL) quit = true;
            break;
        case SDLK_E: take_or_return(); break;
        case SDLK_T: open_input(Input::Warp); break;
        case SDLK_G: open_input(Input::Goto); break;
        case SDLK_M:
            set_mode(mode_ == AddressMode::Positional ? AddressMode::Scrambled : AddressMode::Positional);
            message(std::string("ordering: ") + to_string(mode_) +
                    (mode_ == AddressMode::Positional ? " (neighbours share their beginning)" : " (neighbours are unrelated)"));
            break;
        case SDLK_N: step_trail(1); break;
        case SDLK_B: step_trail(-1); break;
        case SDLK_P:
            if (in_hand_ && line().kind == LineKind::Audio)
            {
                const std::string err = synth_.play(unit_at(*in_hand_));
                message(err.empty() ? "playing" : err);
            }
            break;
        case SDLK_PAGEUP: jump_tiles(1000); break;
        case SDLK_PAGEDOWN: jump_tiles(-1000); break;
        case SDLK_RIGHTBRACKET: jump_tiles(1000000); break;
        case SDLK_LEFTBRACKET: jump_tiles(-1000000); break;
        case SDLK_HOME:
            tile_ = 0;
            face_first_book();
            message("back to the start of this walk");
            break;
        default: break;
        }
    }

    void take_or_return()
    {
        if (in_hand_) { in_hand_.reset(); synth_.stop(); return; }
        if (hover_) in_hand_ = hover_->offset();
    }

    void open_input(Input which)
    {
        input_ = which;
        text_.clear();
        SDL_StartTextInput(window_);
    }
    void close_input()
    {
        input_ = Input::None;
        SDL_StopTextInput(window_);
    }

    void take_hovered() { if (hover_) in_hand_ = hover_->offset(); }

    // One-line summary of where you are (for scripted walks and testing).
    std::string status()
    {
        const Book& here = book(tile_ * kBooksPerTile);
        return std::string(theme().name) + " line, tile " + std::to_string(tile_) + ", x " +
               std::to_string(cam_.pos.x) + ", " + percent(here.fraction) + " along, first book " +
               short_address(here.hex) + (message_.empty() ? "" : "  | " + message_);
    }

    // ---- drawing

    void render()
    {
        int w = 0, h = 0;
        SDL_GetCurrentRenderOutputSize(r_, &w, &h);
        cam_.update(w, h);
        const Theme& th = theme();
        SDL_SetRenderDrawColor(r_, th.bg.r, th.bg.g, th.bg.b, 255);
        SDL_RenderClear(r_);
        SDL_SetRenderDrawBlendMode(r_, SDL_BLENDMODE_BLEND);

        hover_ = pick_book(cam_.pos, cam_.forward(), tile_, 5.0f);

        constexpr int kBack = 6, kAhead = 7;
        // Only tiles that can appear on screen are drawn (usually about half of them).
        bool visible[kBack + kAhead + 1];
        for (int t = -kBack; t <= kAhead; ++t)
        {
            const Vec3 shift{0, 0, t * kTile};
            visible[t + kBack] = cam_.box_visible(tile_lo_ + shift, tile_hi_ + shift);
        }
        // Doors: solid black.
        for (int t = -kBack; t <= kAhead; ++t)
            for (float sx : {-1.0f, 1.0f})
            {
                if (!visible[t + kBack]) continue;
                const float z0 = t * kTile, x = sx * kHalfWidth;
                fill({{x, 0, z0 + kDoorStart}, {x, 0, z0 + kDoorEnd}, {x, kDoorTop, z0 + kDoorEnd}, {x, kDoorTop, z0 + kDoorStart}},
                     SDL_Color{0, 0, 0, 255});
            }
        // The book you are looking at.
        if (hover_)
        {
            Vec3 f[4];
            book_face(float(hover_->tile - tile_) * kTile, hover_->side, hover_->row, hover_->col, f);
            SDL_Color c = th.edge;
            c.a = 110;
            fill({f[0], f[1], f[2], f[3]}, c);
        }
        // Every edge, faded towards the background with distance.
        constexpr int kBuckets = 12;
        std::vector<std::vector<SDL_FPoint>> buckets(kBuckets);
        for (int t = -kBack; t <= kAhead; ++t)
        {
            if (!visible[t + kBack]) continue;
            const float z0 = t * kTile;
            for (const Segment& s : tile_geometry_)
            {
                const auto p = cam_.project_segment({s.a.x, s.a.y, s.a.z + z0}, {s.b.x, s.b.y, s.b.z + z0});
                if (!p) continue;
                const float fade = std::clamp((p->second - 10.0f) / 45.0f, 0.0f, 1.0f);
                auto& b = buckets[size_t(std::min(kBuckets - 1, int(fade * kBuckets)))];
                b.push_back({p->first.first.x, p->first.first.y});
                b.push_back({p->first.second.x, p->first.second.y});
            }
        }
        for (int i = 0; i < kBuckets; ++i)
        {
            const SDL_Color c = mix(th.edge, th.bg, float(i) / kBuckets);
            SDL_SetRenderDrawColor(r_, c.r, c.g, c.b, 255);
            const auto& b = buckets[size_t(i)];
            for (size_t k = 0; k + 1 < b.size(); k += 2) SDL_RenderLine(r_, b[k].x, b[k].y, b[k + 1].x, b[k + 1].y);
        }
        draw_hud(w, h);
    }

    void fill(const std::vector<Vec3>& quad, SDL_Color c)
    {
        const auto pts = cam_.project_polygon(quad);
        if (pts.size() < 3) return;
        std::vector<SDL_Vertex> v;
        const SDL_FColor fc{c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, c.a / 255.0f};
        for (size_t i = 1; i + 1 < pts.size(); ++i)
            for (size_t k : {size_t(0), i, i + 1}) v.push_back({{pts[k].x, pts[k].y}, fc, {0, 0}});
        SDL_RenderGeometry(r_, nullptr, v.data(), int(v.size()), nullptr, 0);
    }

    // Text in the debug font (8x8 pixels per character at scale 1).
    void text(float x, float y, const std::string& s, float scale, SDL_Color c)
    {
        SDL_SetRenderScale(r_, scale, scale);
        SDL_SetRenderDrawColor(r_, c.r, c.g, c.b, c.a);
        SDL_RenderDebugText(r_, x / scale, y / scale, s.c_str());
        SDL_SetRenderScale(r_, 1, 1);
    }

    void panel(float x, float y, float w, float h)
    {
        const Theme& th = theme();
        SDL_SetRenderDrawColor(r_, th.bg.r, th.bg.g, th.bg.b, 230);
        const SDL_FRect box{x, y, w, h};
        SDL_RenderFillRect(r_, &box);
        SDL_SetRenderDrawColor(r_, th.edge.r, th.edge.g, th.edge.b, 255);
        SDL_RenderRect(r_, &box);
    }

    void draw_pixels(const Space::Digits& unit, float x, float y, float size, int frame)
    {
        const ImageFormat& f = line().image;
        const auto px = render_image(unit, f);
        const float cell = size / float(std::max(f.width, f.height));
        const size_t base = size_t(frame) * f.width * f.height;
        for (uint32_t py = 0; py < f.height; ++py)
            for (uint32_t pxi = 0; pxi < f.width; ++pxi)
            {
                const Rgb c = px[base + size_t(py) * f.width + pxi];
                SDL_SetRenderDrawColor(r_, c.r, c.g, c.b, 255);
                const SDL_FRect cellr{x + pxi * cell, y + py * cell, cell, cell};
                SDL_RenderFillRect(r_, &cellr);
            }
        const Theme& th = theme();
        SDL_SetRenderDrawColor(r_, th.edge.r, th.edge.g, th.edge.b, 255);
        const SDL_FRect border{x - 1, y - 1, f.width * cell + 2, f.height * cell + 2};
        SDL_RenderRect(r_, &border);
    }

    std::string one_line_preview(const Space::Digits& u)
    {
        if (line().kind == LineKind::Text) return "\"" + ascii(utf8_encode(line().space.text_of(u))) + "\"";
        if (line().kind == LineKind::Audio) return notes_to_notation(u);
        return "";
    }

    void draw_hud(int w, int h)
    {
        const Theme& th = theme();
        const SDL_Color ink = th.edge;
        const float W = float(w), H = float(h);

        // Crosshair.
        SDL_SetRenderDrawColor(r_, ink.r, ink.g, ink.b, 255);
        SDL_RenderLine(r_, W / 2 - 8, H / 2, W / 2 + 8, H / 2);
        SDL_RenderLine(r_, W / 2, H / 2 - 8, W / 2, H / 2 + 8);

        // Top bar: where you are.
        panel(-1, -1, W + 2, 44);
        const std::string where = std::string(th.name) + " LINE   " + to_string(mode_) + "   tile " +
                                  std::to_string(tile_) + "   " +
                                  percent(book(tile_ * kBooksPerTile).fraction) + " along";
        text(10, 7, where, 2, ink);
        text(10, 28, line().space.id() + "   doors: left wall -> " + kThemes[(li_ + 1) % 4].name +
                         ", right wall -> " + kThemes[(li_ + 3) % 4].name, 1, ink);

        // The book you are looking at.
        if (hover_ && !in_hand_)
        {
            const Book& bk = book(hover_->offset());
            const Space::Digits& u = bk.unit;
            const float ph = line().kind == LineKind::Image || line().kind == LineKind::Video ? 150 : 96;
            panel(10, H - ph - 44, std::min(W - 20, 900.0f), ph);
            char label[160];
            std::snprintf(label, sizeof label, "%s wall, shelf %d, book %d   offset %+lld",
                          hover_->side == Side::Left ? "left" : "right", hover_->row + 1, hover_->col + 1,
                          static_cast<long long>(hover_->offset()));
            float y = H - ph - 36;
            text(20, y, label, 2, ink);
            y += 22;
            text(20, y, "address " + short_address(bk.hex), 1, ink);
            y += 12;
            text(20, y, percent(bk.fraction) + " along the line      E / click: take it off the shelf", 1, ink);
            y += 16;
            if (line().kind == LineKind::Image || line().kind == LineKind::Video) draw_pixels(u, 20, y, 60, 0);
            else text(20, y, wrap(one_line_preview(u), size_t(std::max(20.0f, (std::min(W - 20, 900.0f) - 40) / 16)))[0], 2, ink);
        }

        if (in_hand_) draw_in_hand(W, H);

        // Messages, input box and help.
        if (!message_.empty() && SDL_GetTicks() < message_until_) text(12, H - 36, ascii(message_), 1, ink);
        if (input_ != Input::None)
        {
            const std::string prompt = input_ == Input::Warp
                                           ? (line().kind == LineKind::Image || line().kind == LineKind::Video
                                                  ? "WARP to picture file > "
                                                  : line().kind == LineKind::Audio ? "WARP to notes > " : "WARP to text > ")
                                           : "GO TO address or percent > ";
            panel(10, H / 2 + 40, W - 20, 34);
            text(20, H / 2 + 49, prompt + ascii(text_) + ((SDL_GetTicks() / 400) % 2 ? "_" : " "), 2, ink);
        }
        text(12, H - 18,
             "WASD move  mouse look  Shift run  E take  T warp  G go to  M ordering  wheel/PgUp/PgDn/[ ] jump  "
             "Home start  N/B trail  Tab mouse  Ctrl+Q quit",
             1, ink);
    }

    void draw_in_hand(float W, float H)
    {
        const Theme& th = theme();
        const SDL_Color ink = th.edge;
        const Book& bk = book(*in_hand_);
        const Space::Digits& u = bk.unit;
        const float pw = std::min(W - 40, 1000.0f), ph = std::min(H - 120, 640.0f);
        const float x = (W - pw) / 2, y = 50;
        panel(x, y, pw, ph);
        float cy = y + 12;
        text(x + 14, cy, "IN HAND   offset " + std::to_string(*in_hand_) + "   " + percent(bk.fraction) +
                             " along", 2, ink);
        cy += 28;
        const size_t cols2 = size_t((pw - 28) / 16), cols1 = size_t((pw - 28) / 8);
        switch (line().kind)
        {
        case LineKind::Text:
            for (const auto& l : wrap(ascii(utf8_encode(line().space.text_of(u))), cols2))
            {
                text(x + 14, cy, l, 2, ink);
                cy += 20;
                if (cy > y + ph - 120) break;
            }
            break;
        case LineKind::Audio:
            for (const auto& l : wrap(notes_to_notation(u), cols2))
            {
                text(x + 14, cy, l, 2, ink);
                cy += 20;
            }
            text(x + 14, cy + 6, "P: play    Esc: stop and put back", 1, ink);
            cy += 20;
            break;
        case LineKind::Image:
        case LineKind::Video:
        {
            const int frames = int(line().image.frames);
            const int frame = frames > 1 ? int((SDL_GetTicks() / 250) % Uint64(frames)) : 0;
            const float size = std::min(ph - 170, pw - 28);
            draw_pixels(u, x + 14, cy, size, frame);
            if (frames > 1)
                text(x + 24 + size, cy, "frame " + std::to_string(frame + 1) + "/" + std::to_string(frames), 1, ink);
            cy += size + 10;
            break;
        }
        }
        cy = std::max(cy, y + ph - 110);
        text(x + 14, cy, "address (" + std::string(to_string(mode_)) + ")", 1, ink);
        cy += 12;
        int shown = 0;
        for (const auto& l : wrap(bk.hex, cols1))
        {
            if (++shown > 6) { text(x + 14, cy, "...", 1, ink); break; }
            text(x + 14, cy, l, 1, ink);
            cy += 10;
        }
        text(x + 14, y + ph - 16, "E / click / Esc: put it back", 1, ink);
    }

    bool save_screenshot(const std::string& path)
    {
        SDL_Surface* s = SDL_RenderReadPixels(r_, nullptr);
        if (!s) return false;
        SDL_Surface* rgb = SDL_ConvertSurface(s, SDL_PIXELFORMAT_RGB24);
        SDL_DestroySurface(s);
        if (!rgb) return false;
        std::vector<Rgb> px(size_t(rgb->w) * rgb->h);
        for (int y = 0; y < rgb->h; ++y)
        {
            const auto* row = static_cast<const Uint8*>(rgb->pixels) + size_t(y) * rgb->pitch;
            for (int x = 0; x < rgb->w; ++x) px[size_t(y) * rgb->w + x] = {row[x * 3], row[x * 3 + 1], row[x * 3 + 2]};
        }
        write_png(path, uint32_t(rgb->w), uint32_t(rgb->h), px, 1);
        SDL_DestroySurface(rgb);
        return true;
    }

private:
    SDL_Window* window_;
    SDL_Renderer* r_;
    std::vector<Line> lines_;
    std::vector<Segment> tile_geometry_;
    Vec3 tile_lo_, tile_hi_;
    Camera cam_;
    int li_ = 0;
    AddressMode mode_ = AddressMode::Positional;
    Space::Digits base_;
    int64_t tile_ = 0;
    std::unordered_map<int64_t, Book> cache_;
    Space::Digits base_address_; // base_'s address digits in mode_ (valid unless dirty_)
    bool dirty_ = true;
    void invalidate()
    {
        dirty_ = true;
        cache_.clear();
    }
    std::optional<BookSlot> hover_;
    std::optional<int64_t> in_hand_;
    std::vector<Space::Digits> trail_;
    size_t trail_index_ = 0;
    std::vector<Crossing> crossings_; // doors passed through, most recent last
    Input input_ = Input::None;
    std::string text_;
    std::string message_;
    Uint64 message_until_ = 0;
    Synth synth_;
};

// ---------------------------------------------------------------- options

const char* kUsage =
    "hallway - walk the Sieve's four lines in 3D\n\n"
    "Usage: hallway [options]\n\n"
    "Lines (the same meaning as in the sieve tool):\n"
    "  --line text|image|audio|video  line to start in (default text)\n"
    "  --length L          text: characters per book (default 32)\n"
    "  --alphabet ID       text: lower27 (default), babel29, ascii95\n"
    "  --canon v2|v1       text: warp rules (default v2)\n"
    "  --image-width W  --image-height H  --image-palette ID    image line (10, 10, mono)\n"
    "  --notes N           audio: notes per book (default 16)\n"
    "  --video-width W  --video-height H  --video-frames F  --video-palette ID   video (5, 5, 8, mono)\n"
    "  --key K             scramble key (default sieve)\n"
    "  --mode positional|scrambled   starting ordering (default positional)\n\n"
    "Start:\n"
    "  --warp INPUT        warp on start (text, notes, or a picture file for image/video)\n"
    "  --goto ADDR|P%      go to an address or a percentage on start\n\n"
    "Screenshots (for documentation and testing):\n"
    "  --screenshot PATH   render one frame to a PNG and exit\n"
    "  --size WxH          window size (default 1280x720)\n"
    "  --pose X,Z,YAW,PITCH  camera position and angles in degrees\n"
    "  --tile N            start N tiles along\n"
    "  --take              take the book you are looking at off the shelf\n"
    "  --walk DX,DZ;...    walk these distances in metres first (doors work as when walking)\n\n"
    "Controls: WASD move, mouse look, Shift run, E or click take a book, T warp, G go to,\n"
    "M switch ordering, wheel/PgUp/PgDn/[ ] jump 1/1000/1000000 tiles, Home back to the start,\n"
    "N/B next/previous unit of a warped trail, P play an audio book, Tab free the mouse,\n"
    "Esc close or free the mouse, Ctrl+Q quit. Black doors lead to the next line (left wall)\n"
    "or the previous line (right wall).\n";

std::vector<Line> make_lines(const Args& a)
{
    std::vector<Line> lines;
    for (LineKind k : kLineOrder)
    {
        Args la;
        la.opts["line"] = to_string(k);
        la.opts["key"] = a.get("key", "sieve");
        switch (k)
        {
        case LineKind::Text:
            la.opts["length"] = a.get("length", "32");
            la.opts["alphabet"] = a.get("alphabet", "lower27");
            la.opts["canon"] = a.get("canon", "v2");
            break;
        case LineKind::Image:
            la.opts["width"] = a.get("image-width", "10");
            la.opts["height"] = a.get("image-height", "10");
            la.opts["palette"] = a.get("image-palette", "mono");
            break;
        case LineKind::Audio: la.opts["length"] = a.get("notes", "16"); break;
        case LineKind::Video:
            la.opts["width"] = a.get("video-width", "5");
            la.opts["height"] = a.get("video-height", "5");
            la.opts["frames"] = a.get("video-frames", "8");
            la.opts["palette"] = a.get("video-palette", "mono");
            break;
        }
        lines.push_back(make_line(la));
    }
    return lines;
}

int run(const Args& a)
{
    const std::vector<Line> lines = make_lines(a);
    int start_line = 0;
    for (int i = 0; i < 4; ++i)
        if (a.get("line", "text") == to_string(kLineOrder[i])) start_line = i;
    if (a.has("line") && a.get("line") != to_string(kLineOrder[start_line]))
        throw std::invalid_argument("unknown line '" + a.get("line") + "'");

    int w = 1280, h = 720;
    if (a.has("size") && std::sscanf(a.get("size").c_str(), "%dx%d", &w, &h) != 2)
        throw std::invalid_argument("--size expects WxH, e.g. 1280x720");
    const bool shot = a.has("screenshot");

    if (!SDL_Init(SDL_INIT_VIDEO)) throw std::runtime_error(std::string("SDL_Init failed: ") + SDL_GetError());
    SDL_Window* window = SDL_CreateWindow("Sieve - hallway", w, h, SDL_WINDOW_RESIZABLE | (shot ? SDL_WINDOW_HIDDEN : 0));
    if (!window) throw std::runtime_error(std::string("cannot open a window: ") + SDL_GetError());
    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
    if (!renderer) throw std::runtime_error(std::string("cannot create a renderer: ") + SDL_GetError());
    SDL_SetRenderVSync(renderer, 1);

    Hallway hall(window, renderer, lines);
    hall.set_line(start_line);
    if (a.has("mode")) hall.set_mode(address_mode_from_string(a.get("mode")));
    if (a.has("warp")) hall.warp(a.get("warp"));
    else if (a.has("goto")) hall.go_to(a.get("goto"));
    else if (lines[size_t(start_line)].kind == LineKind::Text) hall.warp("welcome to the sieve");
    else hall.go_to("50%");
    if (a.has("tile")) hall.set_tile(std::stoll(a.get("tile")));
    if (a.has("pose"))
    {
        float x = 0, z = 0, yaw = 0, pitch = 0;
        if (std::sscanf(a.get("pose").c_str(), "%f,%f,%f,%f", &x, &z, &yaw, &pitch) != 4)
            throw std::invalid_argument("--pose expects X,Z,YAW,PITCH");
        hall.camera().pos = {x, 1.6f, z};
        hall.camera().yaw = yaw * kPi / 180;
        hall.camera().pitch = pitch * kPi / 180;
    }

    if (a.has("walk"))
    {
        // "DX,DZ;DX,DZ;..." in metres, applied in 5 cm steps so walls and doors behave as when walking.
        std::string spec = a.get("walk");
        for (size_t start = 0; start < spec.size();)
        {
            const size_t end = std::min(spec.find(';', start), spec.size());
            float dx = 0, dz = 0;
            if (std::sscanf(spec.substr(start, end - start).c_str(), "%f,%f", &dx, &dz) != 2)
                throw std::invalid_argument("--walk expects DX,DZ;DX,DZ;...");
            const int steps = std::max(1, int(std::ceil(std::sqrt(dx * dx + dz * dz) / 0.05f)));
            for (int i = 0; i < steps; ++i) hall.move_by({dx / steps, 0, dz / steps});
            std::cout << hall.status() << "\n";
            start = end + 1;
        }
    }

    if (shot)
    {
        hall.render(); // computes what you are looking at
        if (a.has("take")) hall.take_hovered();
        hall.render();
        if (!hall.save_screenshot(a.get("screenshot"))) throw std::runtime_error(std::string("screenshot failed: ") + SDL_GetError());
        std::cout << "saved " << a.get("screenshot") << "\n";
    }
    else
    {
        SDL_SetWindowRelativeMouseMode(window, true);
        bool quit = false;
        Uint64 last = SDL_GetTicksNS();
        while (!quit)
        {
            SDL_Event e;
            while (SDL_PollEvent(&e)) hall.handle(e, quit);
            const Uint64 now = SDL_GetTicksNS();
            const float dt = std::min(0.1f, float(now - last) / 1e9f);
            last = now;
            hall.update(dt, SDL_GetKeyboardState(nullptr));
            hall.render();
            SDL_RenderPresent(renderer);
        }
    }
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

} // namespace

int main(int argc, char** argv)
{
    try
    {
        // Reuse the sieve argument parser: it expects a command first.
        std::vector<char*> args(argv, argv + argc);
        std::string command = "hallway";
        args.insert(args.begin() + 1, command.data());
        const Args a = parse_args(int(args.size()), args.data());
        if (a.help)
        {
            std::cout << kUsage;
            return 0;
        }
        return run(a);
    }
    catch (const std::exception& e)
    {
        std::cerr << "error: " << e.what() << "\n\n" << kUsage;
        return 1;
    }
}
