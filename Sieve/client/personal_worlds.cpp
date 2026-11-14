// Sieve hallway — your personal dimension as one item of the worlds line (IDEAS 19.187's "the world
// itself as an address on the WORLDS line"; personal_world.hpp says what is carried and what is left
// out, sieve/worldspace.hpp what a world of that line is).
//
// The worlds line cannot hold what a personal dimension holds as it is: its worlds are a few models of
// the models line, each a handful of vertices on a coarse grid, standing in the cells of a grid of their
// own and turned only by quarter turns. So the unit is worked out the way the importers work out theirs
// (a mesh brought into the models line, a picture into the image line): the nearest thing the line can
// hold, and an exact account of what it could not.
#include "personal_world.hpp"

#include "font.hpp"
#include "strings.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <exception>
#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace hallway {

namespace {

// The twelve faces of a box whose corner i is (i & 1 ? high x : low x, i & 2 ? high y : low y, i & 4 ?
// high z : low z), each wound so its normal points out of the box, as a mesh brought in would be.
constexpr std::array<std::array<uint32_t, 3>, 12> kBoxFaces = {{{0, 4, 6}, {0, 6, 2}, {1, 3, 7}, {1, 7, 5}, {0, 1, 5}, {0, 5, 4},
                                                                 {2, 6, 7}, {2, 7, 3}, {0, 2, 3}, {0, 3, 1}, {4, 5, 7}, {4, 7, 6}}};

// A box's high digit along one axis: its half-width `v` in model widths (1 fills the cell), as the
// nearest step of the models line's grid, at least the step just past the middle (a box is never
// flat) and at most the grid's last; its low digit is the high one's mirror, C - 1 - d.
uint32_t high_digit(float v, uint32_t c)
{
    const float d = std::round((v * float(c) + float(c) - 1) / 2);
    return uint32_t(std::clamp(d, float(c / 2), float(c - 1)));
}

// Words to a width, for the panel.
std::vector<std::string> wrap_to(const std::string& s, size_t cols)
{
    std::vector<std::string> out;
    std::string line;
    size_t i = 0;
    while (i < s.size())
    {
        const size_t sp = s.find(' ', i);
        const std::string word = s.substr(i, sp == std::string::npos ? std::string::npos : sp - i);
        i = sp == std::string::npos ? s.size() : sp + 1;
        if (!line.empty() && line.size() + 1 + word.size() > cols) out.push_back(line), line.clear();
        line += (line.empty() ? "" : " ") + word;
    }
    if (!line.empty()) out.push_back(line);
    return out;
}

std::string fixed(double v, int places)
{
    char b[48];
    std::snprintf(b, sizeof b, "%.*f", places, v);
    return b;
}

const char* mode_name(sieve::AddressMode m) { return m == sieve::AddressMode::Scrambled ? "scrambled" : "positional"; }

} // namespace

PersonalWorld::WorldsUnit PersonalWorld::worlds_unit(const sieve::WorldSpace& ws, const std::vector<WorldsBox>& boxes)
{
    WorldsUnit u;
    const sieve::ModelSpace& ms = ws.model_space();
    const uint32_t g = ws.grid(), c = ms.coords(), v = ms.vertices(), f = ms.face_count();
    u.boxes = v >= 8 && f >= 12;
    u.carried = std::min<size_t>(boxes.size(), ws.slots());
    // (the cover and the title left at 0, and every slot model 0 in cell 0, unturned, until filled)
    u.parts.cover.assign(ws.cover_space().unit_length(), 0);
    if (ws.title_space()) u.parts.title.assign(ws.title_space()->unit_length(), 0);
    u.parts.slots.resize(ws.slots());
    if (u.carried == 0) return u;
    // The grid laid over the carried boxes' middles, centred on them: its cells as wide as the widest
    // box (so each fills its cell at most) or as the arrangement over G (so all of it fits), whichever
    // is wider; a centimetre at the least, for a world of one flat thing.
    float lo[3], hi[3], widest = 0;
    for (int k = 0; k < 3; ++k) lo[k] = std::numeric_limits<float>::max(), hi[k] = std::numeric_limits<float>::lowest();
    for (size_t i = 0; i < u.carried; ++i)
        for (int k = 0; k < 3; ++k)
        {
            lo[k] = std::min(lo[k], boxes[i].centre[k]), hi[k] = std::max(hi[k], boxes[i].centre[k]);
            widest = std::max(widest, 2 * boxes[i].half[k]);
        }
    const float span = std::max({hi[0] - lo[0], hi[1] - lo[1], hi[2] - lo[2]});
    u.cell = std::max({widest, span / float(g), 0.01f});
    const auto& turns = sieve::turns();
    for (size_t i = 0; i < u.carried; ++i)
    {
        const WorldsBox& b = boxes[i];
        sieve::WorldSpace::Slot& s = u.parts.slots[i];
        // Its place: the cell its middle falls in, and how far that cell's middle is from it.
        uint32_t cell[3];
        float off2 = 0;
        for (int k = 0; k < 3; ++k)
        {
            const float mid = (lo[k] + hi[k]) / 2;
            const float at = std::floor((b.centre[k] - mid) / u.cell + float(g) / 2);
            cell[k] = uint32_t(std::clamp(at, 0.0f, float(g - 1)));
            const float d = mid + (float(cell[k]) + 0.5f - float(g) / 2) * u.cell - b.centre[k];
            off2 += d * d;
        }
        s.x = cell[0], s.y = cell[1], s.z = cell[2];
        u.off = std::max(u.off, std::sqrt(off2));
        // Its turn: the one of the 24 nearest its own, the one whose matrix agrees with its turn most
        // (the trace of one's transpose times the other: 3 when they are the same turn), and the angle
        // between them.
        float best = -4;
        for (uint32_t t = 0; t < sieve::kTurns; ++t)
        {
            float score = 0;
            for (int k = 0; k < 3; ++k) score += float(turns[t].sign[size_t(k)]) * b.turn[k][turns[t].perm[size_t(k)]];
            if (score > best + 1e-6f) best = score, s.turn = t;
        }
        u.turn_off = std::max(u.turn_off, std::acos(std::clamp((best - 1) / 2, -1.0f, 1.0f)) * 180.0f / 3.14159265f);
        if (!u.boxes) continue; // (only its place: the models line has no box)
        // Its shape: its box, in its own frame, on the models line's grid (a half-width of the cell's
        // half-width is 1).
        uint32_t high[3];
        for (int k = 0; k < 3; ++k) high[k] = high_digit(2 * b.half[k] / u.cell, c);
        sieve::ModelSpace::Parts mp;
        for (uint32_t n = 0; n < v; ++n)
        {
            const uint32_t corner = std::min<uint32_t>(n, 7); // (more vertices than a box has: the last corner again)
            for (int k = 0; k < 3; ++k) mp.verts.push_back((corner >> k) & 1 ? high[k] : c - 1 - high[k]);
        }
        for (uint32_t n = 0; n < f; ++n)
            for (int k = 0; k < 3; ++k) mp.faces.push_back(n < kBoxFaces.size() ? kBoxFaces[n][size_t(k)] : 0); // (more faces: empty)
        s.model = ms.index_of(mp, sieve::AddressMode::Positional);
    }
    return u;
}

// Worked out afresh each time the panel opens, from the world as it stands and the worlds line as the
// settings make it now.
void PersonalWorld::open_worlds()
{
    worlds_asked_ = true;
    worlds_.reset();
    worlds_why_.clear(), worlds_hex_.clear(), worlds_other_hex_.clear();
    if (!worlds_line_of_) worlds_why_ = "no worlds line is lent to this world";
    else
    {
        try
        {
            const WorldsLine line = worlds_line_of_();
            if (!line.space) worlds_why_ = line.why.empty() ? "the worlds line could not be made" : line.why;
            else
            {
                const sieve::WorldSpace& ws = *line.space;
                worlds_ = worlds_unit(ws);
                worlds_mode_ = line.mode;
                const sieve::AddressMode other = line.mode == sieve::AddressMode::Scrambled ? sieve::AddressMode::Positional : sieve::AddressMode::Scrambled;
                worlds_hex_ = ws.hex_of(ws.index_of(worlds_->parts, line.mode));
                worlds_other_hex_ = ws.hex_of(ws.index_of(worlds_->parts, other));
                worlds_grid_ = ws.grid(), worlds_slots_ = ws.slots();
            }
        }
        catch (const std::exception& e)
        {
            worlds_.reset();
            worlds_why_ = e.what();
        }
    }
    worlds_panel_.open = true;
    cat_panel_.open = false, day_panel_.open = false, place_panel_.open = false;
}

// The panel's go-to-it key (personal.worlds/go, Return unless moved; its own key and Esc close it, where
// personal_world.cpp takes them, and pass only this key here).
void PersonalWorld::worlds_key(SDL_Keycode, SDL_Keymod)
{
    if (!worlds_)
    {
        note_ = trf("pworld.worlds.none", {worlds_why_});
        return;
    }
    worlds_go_ = worlds_->parts;
    note_ = tr("pworld.worlds.going");
}

void PersonalWorld::draw_worlds()
{
    if (worlds_panel_.shown <= 0) return;
    const float width = std::min(W_ - 40, 640.0f);
    const size_t cols = size_t(std::max(20.0f, (width - 44) / 8));
    std::vector<std::pair<std::string, bool>> lines; // (white, else grey)
    auto add = [&](const std::string& s, bool white) {
        for (const std::string& l : wrap_to(s, cols)) lines.push_back({l, white});
    };
    if (!worlds_) add(trf("pworld.worlds.none", {worlds_why_}), true);
    else
    {
        const WorldsUnit& u = *worlds_;
        add(trf("pworld.worlds.address", {mode_name(worlds_mode_), std::to_string(worlds_hex_.size())}), true);
        for (size_t i = 0; i < worlds_hex_.size(); i += cols) lines.push_back({worlds_hex_.substr(i, cols), true});
        add(trf("pworld.worlds.carried", {std::to_string(u.carried), std::to_string(u.objects), std::to_string(worlds_slots_)}), false);
        if (!u.boxes) add(tr("pworld.worlds.no_boxes"), false);
        if (u.carried > 0)
        {
            add(trf("pworld.worlds.places", {std::to_string(worlds_grid_), fixed(u.cell, 2), fixed(u.off, 2)}), false);
            add(trf("pworld.worlds.turns", {fixed(u.turn_off, 1)}), false);
        }
        // What the line cannot hold, said only where there is some of it.
        std::string left;
        auto also = [&](const std::string& s) { left += (left.empty() ? "" : "; ") + s; };
        if (u.objects > u.carried) also(trf("pworld.worlds.left.past", {std::to_string(u.objects - u.carried)}));
        if (u.characters > 0) also(trf("pworld.worlds.left.characters", {std::to_string(u.characters)}));
        if (u.carried > 0) also(tr("pworld.worlds.left.meshes"));
        if (u.favourites > 0) also(tr("pworld.worlds.left.favourites"));
        if (u.dropped > 0) also(tr("pworld.worlds.left.dropped"));
        also(tr("pworld.worlds.left.world"));
        add(trf("pworld.worlds.left", {left}), false);
    }
    const float x = worlds_panel_.left(width + 10);
    const SDL_FRect plate{x, 48, width, 34 + float(lines.size()) * 18};
    SDL_SetRenderDrawColor(r_, 0, 0, 0, 170);
    SDL_RenderFillRect(r_, &plate);
    const SDL_Color white{255, 255, 255, 255}, grey{200, 200, 200, 255};
    draw_text(r_, x + 22, 56, tr("pworld.worlds.title"), 1, white);
    for (size_t i = 0; i < lines.size(); ++i) draw_text(r_, x + 22, 76 + float(i) * 18, lines[i].first, 1, lines[i].second ? white : grey);
}

std::string PersonalWorld::worlds_report() const
{
    if (!worlds_asked_) return "";
    if (!worlds_) return "none: " + worlds_why_;
    const WorldsUnit& u = *worlds_;
    const sieve::AddressMode other = worlds_mode_ == sieve::AddressMode::Scrambled ? sieve::AddressMode::Positional : sieve::AddressMode::Scrambled;
    std::string t = std::string(mode_name(worlds_mode_)) + " " + worlds_hex_ + "; " + mode_name(other) + " " + worlds_other_hex_;
    t += "; carried " + std::to_string(u.carried) + " of " + std::to_string(u.objects) + " objects in " + std::to_string(worlds_slots_) + " slots";
    t += " (" + std::to_string(u.favourites) + " favourites, " + std::to_string(u.own) + " of your own, " + std::to_string(u.dropped) + " dropped)";
    t += u.boxes ? ", each as its box" : ", their places only (the models line holds no box)";
    if (u.carried > 0)
        t += "; grid " + std::to_string(worlds_grid_) + ", a cell " + fixed(u.cell, 3) + " m, at most " + fixed(u.off, 3) + " m off; turns at most " + fixed(u.turn_off, 1) +
             " degrees off";
    t += "; left out: " + std::to_string(u.characters) + " characters, " + std::to_string(u.objects - u.carried) + " objects past the slots";
    for (size_t i = 0; i < u.parts.slots.size(); ++i)
    {
        const sieve::WorldSpace::Slot& s = u.parts.slots[i];
        t += "; slot " + std::to_string(i + 1) + " cell " + std::to_string(s.x) + "." + std::to_string(s.y) + "." + std::to_string(s.z) + " turn " + std::to_string(s.turn) +
             (s.model.is_zero() ? " model 0" : "");
    }
    if (worlds_go_) t += "; go to it chosen";
    return t;
}

} // namespace hallway
