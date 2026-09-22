// Sieve hallway — a wireframe walk along the four lines.
//
// All four lines share one endless corridor lined with bookcases; every book is one unit and the
// address increases as you walk forward. Each line repeats along the corridor with its own
// period, and a checkered start line marks where each repeat begins. Black doors in the walls
// lead to the next line (left wall) or the previous line (right wall) at the same corridor
// position, so every door lines up with a door in every other line. One tile of geometry is
// built once and repeated.
//
// In the guided ordering (text, with a model) each book is a point on the entropy-ordered line,
// 2^-zoom apart, showing the unit whose stretch of the line contains it. Probable text owns
// long stretches, so at any zoom the shelves are mostly readable; zooming out (-) shows the
// likeliest continuations, zooming in (=) the fine structure around one unit.

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "camera.hpp"
#include "menu.hpp"
#include "theme.hpp"
#include "synth.hpp"
#include "world.hpp"

#include "cli/args.hpp"
#include "cli/image_io.hpp"
#include "cli/lines.hpp"

#include "sieve/audio.hpp"
#include "sieve/corridor.hpp"
#include "sieve/guided.hpp"
#include "sieve/image.hpp"
#include "sieve/utf8.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

using namespace sieve;
using namespace sieve::cli;
using namespace hallway;

namespace {

constexpr float kPi = 3.14159265358979f;

// ---------------------------------------------------------------- themes (theme.hpp)

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
        // (Books are drawn separately, slot by slot: see Hallway::build_books.)
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

// Short form of a (possibly enormous) tile number for the readout.
std::string short_number(const std::string& dec)
{
    const bool neg = !dec.empty() && dec[0] == '-';
    const std::string d = neg ? dec.substr(1) : dec;
    if (d.size() <= 16) return dec;
    return (neg ? "-" : "") + d.substr(0, 6) + "..." + d.substr(d.size() - 4) + " (" + std::to_string(d.size()) + " digits)";
}

// Saves what the renderer has drawn as a PNG.
bool save_render(SDL_Renderer* r, const std::string& path)
{
    SDL_Surface* s = SDL_RenderReadPixels(r, nullptr);
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

class Hallway
{
public:
    Hallway(SDL_Window* window, SDL_Renderer* renderer, std::vector<Line> lines)
        : window_(window), r_(renderer), lines_(std::move(lines)), tile_geometry_(build_tile()), book_geometry_(build_books())
    {
        // Bounds of one tile's geometry (plus its doors), for skipping tiles out of view.
        tile_lo_ = {-kHalfWidth, 0, -0.5f};
        tile_hi_ = {kHalfWidth, kHeight, kTile};
        for (const Segment& g : tile_geometry_)
            for (const Vec3& v : {g.a, g.b})
            {
                tile_lo_ = {std::min(tile_lo_.x, v.x), std::min(tile_lo_.y, v.y), std::min(tile_lo_.z, v.z)};
                tile_hi_ = {std::max(tile_hi_.x, v.x), std::max(tile_hi_.y, v.y), std::max(tile_hi_.z, v.z)};
            }
        rebase();
    }

    // ---- where you are
    //
    // One corridor position is shared by all four lines: tile_ (any integer) plus the camera's
    // place inside that tile. Each line repeats along the corridor with its own period (its loop),
    // so the book in slot k of tile t is unit ((t mod loop tiles) * 128 + k) of the current line,
    // or empty padding. Doors change the line and keep the position.

    const Line& line() const { return lines_[size_t(li_)]; }
    const Theme& theme() const { return kThemes[li_]; }
    Camera& camera() { return cam_; }
    bool guided_on() const { return guided_ && line().guided != nullptr; }
    const GuidedLine& guided() const { return *line().guided; }
    std::string ordering_name() const { return guided_on() ? "guided" : to_string(mode_); }

    void set_line(int li)
    {
        li_ = li;
        rebase();
    }
    void set_mode(AddressMode m)
    {
        mode_ = m;
        rebase();
    }
    void enable_guided()
    {
        guided_ = true;
        rebase();
    }
    // Moves along the corridor by whole tiles (walking, jumping). Every line moves with you.
    void move_tiles(int64_t d)
    {
        if (d == 0) return;
        tile_ += d;
        loop_tile_ = offset_loop_tile(d);
        for (int i = 0; i < 4; ++i) all_loop_tiles_[i] = offset_loop_tile(all_loops_[i], all_loop_tiles_[i], d);
        cache_.clear();
    }

    // The loop of the current line in the current ordering, and which of its tiles you are in.
    // A full modulo is only needed when the line, ordering or zoom changes.
    void rebase()
    {
        loop_ = LineLoop(guided_on() ? BigUint::pow(2, zoom_) : line().space.size());
        loop_tile_ = loop_.loop_tile(tile_);
        // Every line's loop too, for the double flag where all four start together.
        for (int i = 0; i < 4; ++i)
        {
            const Line& l = lines_[size_t(i)];
            all_loops_[i] = LineLoop(guided_ && l.guided ? BigUint::pow(2, zoom_) : l.space.size());
            all_loop_tiles_[i] = all_loops_[i].loop_tile(tile_);
        }
        cache_.clear();
    }

    // Everything the hallway shows about one book slot, computed once.
    struct Book
    {
        bool empty = false;   // padding after the last unit of a loop
        BigUint index;        // the unit's position in its loop: its raw address, or its point / 2^-zoom
        Space::Digits unit;
        std::string hex;      // raw: the address. guided: the book's point, at the zoom's precision
        double fraction = 0;  // how far along the loop (display only)
        bool guided = false;  // guided only:
        size_t bits = 0;      //   length of the unit's own address (its information, within 2 bits)
        std::string own_hex;  //   the unit's own address
    };

    const Book& book(int64_t dt, uint32_t slot)
    {
        const int64_t key = dt * int64_t(kBooksPerTile) + slot;
        auto it = cache_.find(key);
        if (it != cache_.end()) return it->second;
        if (cache_.size() > 1024) cache_.clear();
        Book b;
        const auto idx = loop_.unit_index(offset_loop_tile(dt), slot);
        if (!idx) b.empty = true;
        else
        {
            b.index = *idx;
            const Space& sp = line().space;
            if (guided_on())
            {
                const GuidedLine& g = guided();
                BigUint point = b.index;
                point <<= g.scale_bits() - zoom_;
                b.guided = true;
                b.unit = g.unit_at(point);
                b.hex = g.hex_of(point, zoom_);
                b.fraction = g.fraction(point);
                const auto c = g.code(b.unit);
                b.bits = c.bits;
                b.own_hex = c.hex;
            }
            else
            {
                const auto address = b.index.to_digits(sp.base(), sp.unit_length());
                b.unit = sp.unit_of_address(address, mode_);
                b.hex = sp.hex_of(address);
                b.fraction = sp.fraction_of(address);
            }
        }
        return cache_.emplace(key, std::move(b)).first->second;
    }

    // The unit's position in its loop under the current ordering. In guided order the zoom is
    // set to the length of the unit's address, so the unit sits exactly on a book.
    BigUint index_of(const Space::Digits& unit)
    {
        if (guided_on())
        {
            const GuidedLine& g = guided();
            const auto c = g.code(unit);
            zoom_ = uint32_t(std::max<size_t>(1, c.bits));
            BigUint i = c.point;
            i >>= g.scale_bits() - zoom_;
            return i;
        }
        return BigUint::from_digits(line().space.address_digits(unit, mode_), line().space.base());
    }

    // Teleport to a unit's slot in the first copy of the loop (where position = address), face
    // it, and optionally open it.
    void place(const BigUint& index, bool open)
    {
        tile_ = LineLoop::tile_of(index);
        rebase();
        const uint32_t slot = LineLoop::slot_of(index);
        face(slot);
        in_hand_.reset();
        if (open)
        {
            in_hand_ = book(0, slot);
            in_hand_where_ = "tile " + short_number(tile_.to_decimal()) + ", slot " + std::to_string(slot);
        }
    }

    // Stand in the corridor facing the book in `slot` of the current tile.
    void face(uint32_t slot)
    {
        const BookSlot b = BookSlot::of(0, slot);
        Vec3 f[4];
        book_face(0, b.side, b.row, b.col, f);
        const Vec3 centre = (f[0] + f[2]) * 0.5f;
        const float sx = b.side == Side::Left ? 1.0f : -1.0f;
        cam_.pos = {0.3f * sx, 1.6f, centre.z + 1.2f};
        const Vec3 d = centre - cam_.pos;
        cam_.yaw = std::atan2(d.x, d.z);
        cam_.pitch = std::atan2(d.y, std::sqrt(d.x * d.x + d.z * d.z));
    }

    void message(const std::string& m)
    {
        message_ = m;
        message_until_ = SDL_GetTicks() + 6000;
    }

    // The book the camera is looking at, or else the first book of the tile: the reference for
    // changing ordering or zoom.
    const Book* reference_book()
    {
        if (hover_)
        {
            const Book& b = book(hover_->tile, hover_->slot());
            if (!b.empty) return &b;
        }
        const Book& b = book(0, 0);
        return b.empty ? nullptr : &b;
    }

    // positional -> scrambled -> guided (if this line has a model) -> positional, keeping the book
    // you are looking at in front of you.
    void cycle_ordering()
    {
        const Book* ref = reference_book();
        const Space::Digits unit = ref ? ref->unit : Space::Digits{};
        if (guided_on()) { guided_ = false; mode_ = AddressMode::Positional; }
        else if (mode_ == AddressMode::Positional) mode_ = AddressMode::Scrambled;
        else if (line().guided) guided_ = true;
        else mode_ = AddressMode::Positional;
        if (ref) place(index_of(unit), false);
        else rebase();
        const char* what = guided_on() ? " (each book is a point 2^-zoom along; likely text owns long stretches; - and = zoom)"
                           : mode_ == AddressMode::Positional ? " (neighbours share their beginning)"
                                                              : " (neighbours are unrelated)";
        message("ordering: " + ordering_name() + what);
    }

    void zoom_to(uint32_t d) { zoom_by(int(d) - int(zoom_)); }

    // Guided order: change the spacing of the books, keeping the point you are looking at.
    void zoom_by(int delta)
    {
        if (!guided_on())
        {
            message("zoom is for the guided ordering: press M until it says guided");
            return;
        }
        const Book* ref = reference_book();
        BigUint point;
        if (ref)
        {
            point = ref->index;
            point <<= guided().scale_bits() - zoom_;
        }
        zoom_ = uint32_t(std::clamp<int64_t>(int64_t(zoom_) + delta, 1, int64_t(guided().scale_bits())));
        point >>= guided().scale_bits() - zoom_;
        place(point, false);
        message("zoom 2^-" + std::to_string(zoom_) + ": books " +
                (zoom_ < 64 ? "1/" + std::to_string(uint64_t(1) << zoom_) : "2^-" + std::to_string(zoom_)) + " of the line apart" +
                (delta < 0 ? " (zoomed out: the likeliest ways on)" : " (zoomed in)"));
    }

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
            place(index_of(trail_[0]), true);
            message("warped: " + w.report.front() + (trail_.size() > 1 ? "   (N / B: next / previous unit of the trail)" : ""));
            return true;
        }
        catch (const std::exception& e)
        {
            message(std::string("warp failed: ") + e.what());
            return false;
        }
    }

    // An address (hex), a percentage (P%), or a corridor tile (@T).
    bool go_to(std::string input)
    {
        try
        {
            input.erase(std::remove(input.begin(), input.end(), ' '), input.end());
            if (input.empty()) throw std::invalid_argument("nothing to go to");
            if (input[0] == '@')
            {
                tile_ = TileIndex::parse(input.substr(1));
                rebase();
                face(0);
                in_hand_.reset();
                trail_.clear();
                message("went to corridor tile " + short_number(tile_.to_decimal()));
                return true;
            }
            BigUint index;
            if (input.back() == '%')
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
                // floor(p / 10^decimals * loop units)
                index = loop_.units();
                index.mul_small(uint32_t(std::stoul(digits)));
                for (uint32_t i = 0; i < decimals; ++i) index.divmod_small(10);
                if (index >= loop_.units()) throw std::invalid_argument("a percentage must be below 100%");
            }
            else if (guided_on())
            {
                const GuidedLine& g = guided();
                const BigUint point = g.point_of(input);
                zoom_ = uint32_t(std::clamp<size_t>(4 * input.size(), 1, g.scale_bits()));
                index = point;
                index >>= g.scale_bits() - zoom_;
            }
            else index = BigUint::from_digits(line().space.parse_address(input), line().space.base());
            trail_.clear();
            place(index, true);
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
        place(index_of(trail_[trail_index_]), true);
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
        while (cam_.pos.z >= kTile) { cam_.pos.z -= kTile; move_tiles(1); }
        while (cam_.pos.z < 0) { cam_.pos.z += kTile; move_tiles(-1); }
        if (cam_.pos.x < -(kHalfWidth + 0.05f)) cross(Side::Left);
        else if (cam_.pos.x > kHalfWidth + 0.05f) cross(Side::Right);
    }

    void jump_tiles(int64_t n)
    {
        move_tiles(n);
        message("jumped " + std::to_string(n) + " tiles (" + std::to_string(n * int64_t(kBooksPerTile)) + " books)");
    }

    // A door keeps your corridor position and changes the line reading it: left wall to the next
    // line, right wall to the previous one. You come in through the opposite wall's door.
    void cross(Side side)
    {
        const int to = side == Side::Left ? (li_ + 1) % 4 : (li_ + 3) % 4;
        cam_.pos.x = (kHalfWidth - 0.1f) * (side == Side::Left ? 1.0f : -1.0f);
        in_hand_.reset();
        set_line(to);
        message(std::string("through the door into the ") + theme().name + " line, at the same corridor tile " +
                short_number(tile_.to_decimal()));
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
        case SDLK_M: cycle_ordering(); break;
        case SDLK_F1:
            menu_requested_ = true;
            quit = true;
            break;
        case SDLK_MINUS:
        case SDLK_KP_MINUS: zoom_by((e.key.mod & SDL_KMOD_SHIFT) ? -8 : -1); break;
        case SDLK_EQUALS:
        case SDLK_KP_PLUS: zoom_by((e.key.mod & SDL_KMOD_SHIFT) ? 8 : 1); break;
        case SDLK_N: step_trail(1); break;
        case SDLK_B: step_trail(-1); break;
        case SDLK_P:
            if (in_hand_ && line().kind == LineKind::Audio)
            {
                const std::string err = synth_.play(in_hand_->unit);
                message(err.empty() ? "playing" : err);
            }
            break;
        case SDLK_PAGEUP: jump_tiles(1000); break;
        case SDLK_PAGEDOWN: jump_tiles(-1000); break;
        case SDLK_RIGHTBRACKET: jump_tiles(1000000); break;
        case SDLK_LEFTBRACKET: jump_tiles(-1000000); break;
        case SDLK_HOME:
            tile_ = TileIndex{};
            rebase();
            face(0);
            in_hand_.reset();
            message("corridor tile 0: the start line of every line");
            break;
        default: break;
        }
    }

    void take_or_return()
    {
        if (in_hand_) { in_hand_.reset(); synth_.stop(); return; }
        take_hovered();
    }

    void take_hovered()
    {
        if (!hover_) return;
        const Book& b = book(hover_->tile, hover_->slot());
        if (b.empty) return;
        in_hand_ = b;
        TileIndex t = tile_;
        t += hover_->tile;
        in_hand_where_ = "tile " + short_number(t.to_decimal()) + ", slot " + std::to_string(hover_->slot());
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

    // One-line summary of where you are (for scripted walks and testing).
    std::string status()
    {
        const Book& first = book(0, 0);
        std::string where = std::string(theme().name) + " line, " + ordering_name();
        if (guided_on()) where += " zoom " + std::to_string(zoom_);
        where += ", tile " + short_number(tile_.to_decimal()) + ", x " + std::to_string(cam_.pos.x) + ", ";
        if (first.empty) where += "first slot empty (padding)";
        else
        {
            where += percent(first.fraction) + " along, first book " + short_address(first.hex);
            if (line().kind == LineKind::Text) where += " \"" + ascii(utf8_encode(line().space.text_of(first.unit))) + "\"";
            if (first.guided) where += " (" + std::to_string(first.bits) + " bits)";
        }
        return where + (message_.empty() ? "" : "  | " + message_);
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

        hover_ = pick_book(cam_.pos, cam_.forward(), 0, 5.0f);

        constexpr int kBack = 6, kAhead = 7;
        // Only tiles that can appear on screen are drawn (usually about half of them).
        bool visible[kBack + kAhead + 1];
        for (int t = -kBack; t <= kAhead; ++t)
        {
            const Vec3 shift{0, 0, t * kTile};
            visible[t + kBack] = cam_.box_visible(tile_lo_ + shift, tile_hi_ + shift);
        }
        // Doors: solid black. Start lines: checkered, where a loop of this line begins.
        for (int t = -kBack; t <= kAhead; ++t)
        {
            if (!visible[t + kBack]) continue;
            const float z0 = t * kTile;
            for (float sx : {-1.0f, 1.0f})
            {
                const float x = sx * kHalfWidth;
                fill({{x, 0, z0 + kDoorStart}, {x, 0, z0 + kDoorEnd}, {x, kDoorTop, z0 + kDoorEnd}, {x, kDoorTop, z0 + kDoorStart}},
                     SDL_Color{0, 0, 0, 255});
            }
            if (offset_loop_tile(t).is_zero())
            {
                draw_start_line(z0);
                if (all_start(t)) draw_start_line(z0 + 1.0f); // every line starts here: a double flag
            }
        }
        // The book you are looking at.
        if (hover_ && !book(hover_->tile, hover_->slot()).empty)
        {
            Vec3 f[4];
            book_face(float(hover_->tile) * kTile, hover_->side, hover_->row, hover_->col, f);
            SDL_Color c = th.edge;
            c.a = 110;
            fill({f[0], f[1], f[2], f[3]}, c);
        }
        // Every edge, faded towards the background with distance. Padding slots have no book.
        constexpr int kBuckets = 12;
        std::vector<std::vector<SDL_FPoint>> buckets(kBuckets);
        auto add = [&](const Segment& s, float z0) {
            const auto p = cam_.project_segment({s.a.x, s.a.y, s.a.z + z0}, {s.b.x, s.b.y, s.b.z + z0});
            if (!p) return;
            const float fade = std::clamp((p->second - 10.0f) / 45.0f, 0.0f, 1.0f);
            auto& b = buckets[size_t(std::min(kBuckets - 1, int(fade * kBuckets)))];
            b.push_back({p->first.first.x, p->first.first.y});
            b.push_back({p->first.second.x, p->first.second.y});
        };
        for (int t = -kBack; t <= kAhead; ++t)
        {
            if (!visible[t + kBack]) continue;
            const float z0 = t * kTile;
            for (const Segment& s : tile_geometry_) add(s, z0);
            const uint32_t books = books_in_tile(t);
            for (uint32_t k = 0; k < books; ++k)
                for (const Segment& s : book_geometry_[k]) add(s, z0);
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

    // A checkered strip across the floor and a checkered banner overhead at z0: the start (and
    // end) of a loop of the current line.
    void draw_start_line(float z0)
    {
        const Theme& th = theme();
        constexpr int kSquares = 16;
        const float sq = 2 * kHalfWidth / kSquares;
        for (int row = 0; row < 2; ++row)
            for (int i = 0; i < kSquares; ++i)
            {
                const SDL_Color c = (i + row) % 2 ? th.bg : th.edge;
                const float x0 = -kHalfWidth + i * sq, x1 = x0 + sq;
                const float za = z0 - sq + row * sq, zb = za + sq;
                fill({{x0, 0.001f, za}, {x1, 0.001f, za}, {x1, 0.001f, zb}, {x0, 0.001f, zb}}, c);
                const float ya = kHeight - (row + 1) * sq, yb = ya + sq;
                fill({{x0, ya, z0}, {x1, ya, z0}, {x1, yb, z0}, {x0, yb, z0}}, c);
            }
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
        const Book& first = book(0, 0);
        const std::string where = std::string(th.name) + " LINE   " + ordering_name() +
                                  (guided_on() ? "  zoom 2^-" + std::to_string(zoom_) : std::string()) + "   tile " +
                                  short_number(tile_.to_decimal()) +
                                  (first.empty ? std::string("   (end of loop: padding)") : "   " + percent(first.fraction) + " along");
        text(10, 7, where, 2, ink);
        const std::string loop = "loop " + short_number(loop_.tiles().to_decimal()) + " tiles" +
                                 (loop_.fills_whole_tiles() ? std::string() : " (+" + std::to_string(loop_.padding()) + " empty slots)");
        text(10, 28, line().space.id() + (guided_on() ? "   model " + line().model_id : std::string()) + "   " + loop + "   doors: left -> " +
                         kThemes[(li_ + 1) % 4].name + ", right -> " + kThemes[(li_ + 3) % 4].name, 1, ink);

        // The book you are looking at.
        if (hover_ && !in_hand_)
        {
            const Book& bk = book(hover_->tile, hover_->slot());
            const float ph = line().kind == LineKind::Image || line().kind == LineKind::Video ? 150 : 96;
            panel(10, H - ph - 44, std::min(W - 20, 900.0f), ph);
            char label[160];
            std::snprintf(label, sizeof label, "%s wall, shelf %d, book %d   slot %u", hover_->side == Side::Left ? "left" : "right",
                          hover_->row + 1, hover_->col + 1, hover_->slot());
            float y = H - ph - 36;
            text(20, y, label, 2, ink);
            y += 22;
            if (bk.empty)
                text(20, y, "empty shelf space: the loop ends in this tile; the start line follows", 1, ink);
            else
            {
                const Space::Digits& u = bk.unit;
                if (bk.guided)
                {
                    char b1[16], b2[16];
                    std::snprintf(b1, sizeof b1, "%.2f", double(bk.bits) / double(u.size()));
                    std::snprintf(b2, sizeof b2, "%.2f", std::log2(double(line().space.base())));
                    text(20, y, "point " + short_address(bk.hex) + "   own address " + std::to_string(bk.bits) + " bits (" + b1 +
                                    " bits/char; raw " + b2 + ")",
                         1, ink);
                }
                else text(20, y, "address " + short_address(bk.hex), 1, ink);
                y += 12;
                text(20, y, percent(bk.fraction) + " along the loop      E / click: take it off the shelf", 1, ink);
                y += 16;
                if (line().kind == LineKind::Image || line().kind == LineKind::Video) draw_pixels(u, 20, y, 60, 0);
                else text(20, y, wrap(one_line_preview(u), size_t(std::max(20.0f, (std::min(W - 20, 900.0f) - 40) / 16)))[0], 2, ink);
            }
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
                                           : "GO TO address, percent or @tile > ";
            panel(10, H / 2 + 40, W - 20, 34);
            text(20, H / 2 + 49, prompt + ascii(text_) + ((SDL_GetTicks() / 400) % 2 ? "_" : " "), 2, ink);
        }
        text(12, H - 18,
             std::string("WASD move  Shift run  E take  T warp  G go to  M ordering  ") + (guided_on() ? "-/= zoom  " : "") +
                 "wheel/PgUp/[ ] jump  Home start  N/B trail  F1 menu  Tab mouse  Ctrl+Q quit",
             1, ink);
    }

    void draw_in_hand(float W, float H)
    {
        const Theme& th = theme();
        const SDL_Color ink = th.edge;
        const Book& bk = *in_hand_;
        const Space::Digits& u = bk.unit;
        const float pw = std::min(W - 40, 1000.0f), ph = std::min(H - 120, 640.0f);
        const float x = (W - pw) / 2, y = 50;
        panel(x, y, pw, ph);
        float cy = y + 12;
        text(x + 14, cy, "IN HAND   " + in_hand_where_ + "   " + percent(bk.fraction) + " along", 2, ink);
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
        text(x + 14, cy, bk.guided ? "address (guided, " + std::to_string(bk.bits) + " bits)" : "address (" + std::string(to_string(mode_)) + ")", 1, ink);
        cy += 12;
        int shown = 0;
        for (const auto& l : wrap(bk.guided ? bk.own_hex : bk.hex, cols1))
        {
            if (++shown > 6) { text(x + 14, cy, "...", 1, ink); break; }
            text(x + 14, cy, l, 1, ink);
            cy += 10;
        }
        text(x + 14, y + ph - 16, "E / click / Esc: put it back", 1, ink);
    }

    bool menu_requested() const { return menu_requested_; }
    void put_back() { in_hand_.reset(); }

private:
    // The loop tile of the tile dt away from yours. Only needs a full modulo for small loops;
    // for big ones a step of a few tiles wraps at most once.
    BigUint offset_loop_tile(int64_t dt) const { return offset_loop_tile(loop_, loop_tile_, dt); }

    // True if tile dt away is the start of a loop on all four lines at once.
    bool all_start(int64_t dt) const
    {
        for (int i = 0; i < 4; ++i)
            if (!offset_loop_tile(all_loops_[i], all_loop_tiles_[i], dt).is_zero()) return false;
        return true;
    }

    static BigUint offset_loop_tile(const LineLoop& loop, const BigUint& loop_tile, int64_t dt)
    {
        const BigUint& tiles = loop.tiles();
        if (tiles.bit_length() <= 62)
        {
            const int64_t n = int64_t(std::stoull(tiles.to_decimal()));
            const int64_t cur = int64_t(std::stoull(loop_tile.to_decimal()));
            return BigUint(uint64_t(((cur + dt % n) % n + n) % n));
        }
        BigUint r = loop_tile;
        if (dt >= 0)
        {
            r += BigUint(uint64_t(dt));
            if (r >= tiles) r -= tiles;
        }
        else
        {
            const BigUint d(0 - uint64_t(dt));
            if (r >= d) r -= d;
            else
            {
                r += tiles;
                r -= d;
            }
        }
        return r;
    }

    // How many books tile dt holds: all of them, except the last tile of a padded loop.
    uint32_t books_in_tile(int64_t dt) const
    {
        if (loop_.fills_whole_tiles()) return kBooksPerTile;
        BigUint lt = offset_loop_tile(dt);
        lt.add_small(1);
        return lt == loop_.tiles() ? kBooksPerTile - loop_.padding() : kBooksPerTile;
    }

    // The books of one tile, slot by slot (4 edges each), drawn separately so padding can be bare.
    static std::vector<std::array<Segment, 4>> build_books()
    {
        std::vector<std::array<Segment, 4>> out(kBooksPerTile);
        for (uint32_t k = 0; k < uint32_t(kBooksPerTile); ++k)
        {
            const BookSlot b = BookSlot::of(0, k);
            Vec3 f[4];
            book_face(0, b.side, b.row, b.col, f);
            out[k] = {Segment{f[0], f[1]}, Segment{f[1], f[2]}, Segment{f[2], f[3]}, Segment{f[3], f[0]}};
        }
        return out;
    }

    SDL_Window* window_;
    SDL_Renderer* r_;
    std::vector<Line> lines_;
    std::vector<Segment> tile_geometry_;
    std::vector<std::array<Segment, 4>> book_geometry_;
    Vec3 tile_lo_, tile_hi_;
    Camera cam_;
    int li_ = 0;
    AddressMode mode_ = AddressMode::Positional;
    bool guided_ = false; // guided ordering selected (applies on lines with a model)
    uint32_t zoom_ = 20;  // guided: books are 2^-zoom_ of the line apart, so the loop has 2^zoom_ books
    TileIndex tile_;      // the corridor tile you are in, shared by every line
    LineLoop loop_{BigUint(1)};
    BigUint loop_tile_;   // tile_ mod loop_.tiles()
    LineLoop all_loops_[4] = {LineLoop(BigUint(1)), LineLoop(BigUint(1)), LineLoop(BigUint(1)), LineLoop(BigUint(1))};
    BigUint all_loop_tiles_[4];
    std::unordered_map<int64_t, Book> cache_;
    std::optional<BookSlot> hover_;
    std::optional<Book> in_hand_;
    std::string in_hand_where_;
    std::vector<Space::Digits> trail_;
    size_t trail_index_ = 0;
    Input input_ = Input::None;
    std::string text_;
    std::string message_;
    Uint64 message_until_ = 0;
    bool menu_requested_ = false;
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
    "  --mode positional|scrambled|guided   starting ordering (default positional)\n"
    "  --model ID|PATH|none  text: model for the guided ordering (default: the alphabet's default)\n"
    "  --zoom D            guided: books 2^-D of the line apart (after --warp/--goto set it)\n\n"
    "Start:\n"
    "  --warp INPUT        warp on start (text, notes, or a picture file for image/video)\n"
    "  --goto ADDR|P%|@T   go to an address, a percentage, or corridor tile T on start\n\n"
    "Menu:\n"
    "  A setup menu opens first: adjust every line's state space and see the four lines as a map.\n"
    "  --no-menu           go straight into the hallway (F1 opens the menu from the hallway)\n"
    "  --menu              with --screenshot: a picture of the menu (--press keys go to the menu)\n\n"
    "Screenshots (for documentation and testing):\n"
    "  --screenshot PATH   render one frame to a PNG and exit\n"
    "  --size WxH          window size (default 1280x720)\n"
    "  --pose X,Z,YAW,PITCH  camera position and angles in degrees\n"
    "  --tile N            then move N tiles along the corridor\n"
    "  --take              take the book you are looking at off the shelf\n"
    "  --walk DX,DZ;...    walk these distances in metres first (doors work as when walking)\n"
    "  --press K,K,...     then press these keys (e.g. M,M,-,Shift+=), printing where you are\n\n"
    "Controls: WASD move, mouse look, Shift run, E or click take a book, T warp, G go to,\n"
    "M switch ordering (positional, scrambled, guided), - and = zoom out/in (guided; Shift: 8x),\n"
    "wheel/PgUp/PgDn/[ ] jump 1/1000/1000000 tiles, Home to corridor tile 0 (every line's start line),\n"
    "N/B next/previous unit of a warped trail, P play an audio book, F1 the setup menu, Tab free the mouse,\n"
    "Esc close or free the mouse, Ctrl+Q quit.\n\n"
    "All four lines share one corridor, 128 books per tile. Each line repeats along it; a\n"
    "checkered start line marks where each repeat begins. Black doors lead to the next line\n"
    "(left wall) or the previous line (right wall) at the same corridor position.\n";

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
            if (a.has("model")) la.opts["model"] = a.get("model");
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

// "M,M,-,Shift+=" -> key presses.
std::vector<std::pair<SDL_Keycode, SDL_Keymod>> parse_presses(const std::string& spec)
{
    std::vector<std::pair<SDL_Keycode, SDL_Keymod>> out;
    for (size_t start = 0; start < spec.size();)
    {
        const size_t end = std::min(spec.find(',', start), spec.size());
        std::string name = spec.substr(start, end - start);
        SDL_Keymod mod = SDL_KMOD_NONE;
        if (name.rfind("Shift+", 0) == 0) { mod = SDL_KMOD_LSHIFT; name = name.substr(6); }
        const SDL_Keycode key = name == "-" ? SDLK_MINUS : name == "=" ? SDLK_EQUALS : SDL_GetKeyFromName(name.c_str());
        if (key == SDLK_UNKNOWN) throw std::invalid_argument("--press: unknown key '" + name + "'");
        out.emplace_back(key, mod);
        start = end + 1;
    }
    return out;
}

// Builds the hallway from options, and applies the start-up and scripting options.
std::unique_ptr<Hallway> make_hallway(SDL_Window* window, SDL_Renderer* renderer, const Args& a, bool scripted)
{
    std::vector<Line> lines = make_lines(a);
    int start_line = 0;
    for (int i = 0; i < 4; ++i)
        if (a.get("line", "text") == to_string(kLineOrder[i])) start_line = i;
    if (a.has("line") && a.get("line") != to_string(kLineOrder[start_line]))
        throw std::invalid_argument("unknown line '" + a.get("line") + "'");
    const bool text_has_model = lines[0].guided != nullptr;
    const LineKind start_kind = lines[size_t(start_line)].kind;

    auto hall = std::make_unique<Hallway>(window, renderer, std::move(lines));
    hall->set_line(start_line);
    if (a.has("mode"))
    {
        if (a.get("mode") == "guided")
        {
            if (text_has_model) hall->enable_guided();
            else
            {
                std::cerr << "guided ordering needs a model for the text line (see: sieve models); using positional\n";
                hall->message("guided ordering needs a text model: using positional");
            }
        }
        else hall->set_mode(address_mode_from_string(a.get("mode")));
    }
    if (a.has("warp")) hall->warp(a.get("warp"));
    else if (a.has("goto")) hall->go_to(a.get("goto"));
    else if (start_kind == LineKind::Text)
    {
        hall->warp("welcome to the sieve");
        hall->put_back();
    }
    else hall->go_to("50%");
    if (!scripted) return hall;

    if (a.has("zoom")) hall->zoom_to(a.get_positive("zoom", 20));
    if (a.has("tile")) hall->move_tiles(std::stoll(a.get("tile")));
    if (a.has("pose"))
    {
        float x = 0, z = 0, yaw = 0, pitch = 0;
        if (std::sscanf(a.get("pose").c_str(), "%f,%f,%f,%f", &x, &z, &yaw, &pitch) != 4)
            throw std::invalid_argument("--pose expects X,Z,YAW,PITCH");
        hall->camera().pos = {x, 1.6f, z};
        hall->camera().yaw = yaw * kPi / 180;
        hall->camera().pitch = pitch * kPi / 180;
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
            for (int i = 0; i < steps; ++i) hall->move_by({dx / steps, 0, dz / steps});
            std::cout << hall->status() << "\n";
            start = end + 1;
        }
    }
    if (a.has("press"))
        for (const auto& [key, mod] : parse_presses(a.get("press")))
        {
            SDL_Event e{};
            e.type = SDL_EVENT_KEY_DOWN;
            e.key.key = key;
            e.key.mod = mod;
            bool quit = false;
            hall->render(); // so the key sees what you are looking at
            hall->handle(e, quit);
            std::cout << hall->status() << "\n";
        }
    return hall;
}

int run(const Args& a)
{
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
    auto finish = [&] {
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 0;
    };

    Settings settings = Settings::from_args(a);
    if (shot && a.has("menu"))
    {
        // A picture of the setup menu (for documentation and testing); --press keys go to the menu.
        Menu menu(window, renderer, settings);
        if (a.has("press"))
            for (const auto& [key, mod] : parse_presses(a.get("press"))) menu.press(key, mod);
        menu.render();
        if (!save_render(renderer, a.get("screenshot"))) throw std::runtime_error(std::string("screenshot failed: ") + SDL_GetError());
        std::cout << "saved " << a.get("screenshot") << "\n";
        return finish();
    }
    if (shot)
    {
        auto hall = make_hallway(window, renderer, a, true);
        hall->render(); // computes what you are looking at
        if (a.has("take")) hall->take_hovered();
        hall->render();
        if (!save_render(renderer, a.get("screenshot"))) throw std::runtime_error(std::string("screenshot failed: ") + SDL_GetError());
        std::cout << "saved " << a.get("screenshot") << "\n";
        return finish();
    }

    // Interactive: the setup menu first (unless --no-menu), then the hallway; F1 goes back.
    bool show_menu = !a.has("no-menu");
    bool first = true;
    while (true)
    {
        if (show_menu)
        {
            Menu menu(window, renderer, settings);
            if (menu.run() == Menu::Result::Quit) break;
            settings = menu.settings();
        }
        Args ha = a;
        if (show_menu) settings.apply(ha);
        if (!first)
            for (const char* k : {"warp", "goto", "zoom", "tile", "pose", "walk", "press"}) ha.opts.erase(k);
        auto hall = make_hallway(window, renderer, ha, first);
        first = false;
        SDL_SetWindowRelativeMouseMode(window, true);
        bool quit = false;
        Uint64 last = SDL_GetTicksNS();
        while (!quit)
        {
            SDL_Event e;
            while (SDL_PollEvent(&e)) hall->handle(e, quit);
            const Uint64 now = SDL_GetTicksNS();
            const float dt = std::min(0.1f, float(now - last) / 1e9f);
            last = now;
            hall->update(dt, SDL_GetKeyboardState(nullptr));
            hall->render();
            SDL_RenderPresent(renderer);
        }
        if (!hall->menu_requested()) break;
        show_menu = true;
    }
    return finish();
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
