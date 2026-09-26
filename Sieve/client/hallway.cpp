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
#include "font.hpp"
#include "mesh.hpp"
#include "app_settings.hpp"
#include "main_menu.hpp"
#include "menu.hpp"
#include "strings.hpp"
#include "theme.hpp"
#include "synth.hpp"
#include "world.hpp"

#include "cli/args.hpp"
#include "cli/book.hpp"
#include "cli/filter_config.hpp"
#include "cli/image_io.hpp"
#include "cli/lines.hpp"

#include "sieve/audio.hpp"
#include "sieve/booksieve.hpp"
#include "sieve/bookspace.hpp"
#include "sieve/compact.hpp"
#include "sieve/corridor.hpp"
#include "sieve/guided.hpp"
#include "sieve/image.hpp"
#include "sieve/modelspace.hpp"
#include "sieve/utf8.hpp"

#include <algorithm>
#include <array>
#include <climits>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <cstdio>
#include <iostream>
#include <iterator>
#include <memory>
#include <optional>
#include <stdexcept>
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
// The corridor's lines: the four above, then books (made of pages and a picture).
// The corridor's lines, in door order: the four unit lines, books, models, and then binary,
// which is the line the others are bounded by. Walking left goes to the next line and right to
// the previous one, so the sequence runs
//
//     binary | pages  image  audio  video  books  models | binary
//
// with binary at both ends: the six no longer loop into one another, they start and finish at
// it. Binary is one line, not two -- it wraps around the outside of the other six, and which
// side of it you see the edge on depends on which end you walked out of (see binary_shelf_).
constexpr int kLines = 7;
constexpr int kBooksLine = 4, kModelsLine = 5, kBinaryLine = 6;
// The short wall on the binary line's open edge, under Real Graphics: dark, so the drop past it
// is what the eye goes to.
constexpr SDL_FColor kRailColour{0.42f, 0.44f, 0.42f, 1.0f};
// Tiles drawn behind and ahead of the one you are in (and kept in the book cache).
constexpr int kCacheBack = 6, kCacheAhead = 7;
constexpr int kBuckets = 12; // distance fades of the wireframe
const Theme& theme_of(int li)
{
    return li == kBooksLine ? kBooksTheme : li == kModelsLine ? kModelsTheme : li == kBinaryLine ? kBinaryTheme : kThemes[li];
}

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

// One tile's edges: the hallway (floor, ceiling, walls, door frames, floor marks) and/or the two
// bookcases. Real Graphics replaces each part with its model when there is one.
// `only`: 0 for both walls, or -1 / +1 for just that one. The binary line has one wall, because
// its other side is the edge (build_edge below), so it is built with only = -1 or +1.
std::vector<Segment> build_tile(bool hallway = true, bool shelves = true, int only = 0)
{
    std::vector<Segment> s, hall, cases;
    auto rect = [&](Vec3 p0, Vec3 p1, Vec3 p2, Vec3 p3) {
        s.push_back({p0, p1}); s.push_back({p1, p2}); s.push_back({p2, p3}); s.push_back({p3, p0});
    };
    for (float sx : {-1.0f, 1.0f})
    {
        if (only != 0 && sx != float(only)) continue;
        const float wall = sx * kHalfWidth, face = sx * kCaseFront;
        // Floor and ceiling edges along the tile, and the frame across its start.
        hall.push_back({{wall, 0, 0}, {wall, 0, kTile}});
        hall.push_back({{wall, kHeight, 0}, {wall, kHeight, kTile}});
        hall.push_back({{wall, 0, 0}, {wall, kHeight, 0}});
        // Bookcase: front outline, depth edges back to the wall, shelf boards.
        s.clear();
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
        cases.insert(cases.end(), s.begin(), s.end());
        // Door frame.
        s.clear();
        rect({wall, 0, kDoorStart}, {wall, 0, kDoorEnd}, {wall, kDoorTop, kDoorEnd}, {wall, kDoorTop, kDoorStart});
        hall.insert(hall.end(), s.begin(), s.end());
    }
    hall.push_back({{-kHalfWidth, 0, 0}, {kHalfWidth, 0, 0}});
    hall.push_back({{-kHalfWidth, kHeight, 0}, {kHalfWidth, kHeight, 0}});
    // Centre-line marks on the floor give a sense of motion.
    for (float z = 0.5f; z < kTile; z += 2.0f) hall.push_back({{0, 0, z}, {0, 0, z + 0.6f}});
    std::vector<Segment> out;
    if (hallway) out = hall;
    if (shelves) out.insert(out.end(), cases.begin(), cases.end());
    return out;
}

// The open side of one tile of the binary line (world.hpp). The space is an ordinary tile of
// corridor; this is what stands where its other wall would have been -- a short wall on the last
// edge of the floor, and past that nothing. `sx` is which side that is; the shelves are opposite.
std::vector<Segment> build_edge(float sx)
{
    std::vector<Segment> s;
    const float rail = sx * kEdgeRail;
    // The short wall: along the tile at the top, up each end, and posts in between.
    s.push_back({{rail, kEdgeRailTop, 0}, {rail, kEdgeRailTop, kTile}});
    s.push_back({{rail, 0, 0}, {rail, kEdgeRailTop, 0}});
    s.push_back({{rail, 0, kTile}, {rail, kEdgeRailTop, kTile}});
    for (float z = kTile / 6; z < kTile - 0.01f; z += kTile / 6) s.push_back({{rail, 0, z}, {rail, kEdgeRailTop, z}});
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

// Greedy word wrap: lines of at most `width` characters, broken at spaces where possible;
// trailing spaces (a page's padding) are dropped.
std::vector<std::string> wrap_words(std::string s, size_t width)
{
    while (!s.empty() && s.back() == ' ') s.pop_back();
    std::vector<std::string> out;
    size_t i = 0;
    while (i < s.size())
    {
        size_t end = std::min(s.size(), i + width);
        if (end < s.size())
        {
            const size_t sp = s.rfind(' ', end);
            if (sp != std::string::npos && sp > i) end = sp;
        }
        out.push_back(s.substr(i, end - i));
        i = end;
        while (i < s.size() && s[i] == ' ') ++i;
    }
    if (out.empty()) out.push_back("");
    return out;
}

// For the console readout only, which scripts read line by line and terminals may not be able
// to show. Text drawn in the hallway keeps its own characters: the font falls back per glyph.
std::string ascii(const std::string& utf8)
{
    std::string out;
    for (char32_t c : utf8_decode(utf8)) out.push_back(c < 0x80 ? char(c) : '?');
    return out;
}

std::string fixed(double v, int d)
{
    char b[48];
    std::snprintf(b, sizeof b, "%.*f", d, v);
    return b;
}

std::string percent(double f)
{
    char buf[48];
    std::snprintf(buf, sizeof buf, "%.10f%%", f * 100.0);
    return buf;
}

// The same position as an angle. Every line is a loop, so where you stand in it is a bearing:
// 0 degrees is where the loop starts and finishes, and it comes round to 0 again. How many
// decimal places is Angle Precision in Settings > Graphics.
std::string degrees(double f, int decimals)
{
    char fmt[16], buf[48];
    std::snprintf(fmt, sizeof fmt, "%%.%df", std::clamp(decimals, 0, 8));
    std::snprintf(buf, sizeof buf, fmt, f * 360.0);
    return std::string(buf) + "\xc2\xb0";
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

// Short form of a big number without converting all of it to decimal (which costs time
// quadratic in its length): exact below 20,000 bits; above that the digit count and the leading
// digits come from its logarithm (display only) and the last four digits are exact.
std::string short_big(const BigUint& v, bool negative = false)
{
    if (v.bit_length() <= 20000) return short_number((negative ? "-" : "") + v.to_decimal());
    const double lg = v.log10_approx();
    const uint64_t digits = uint64_t(std::floor(lg)) + 1;
    char lead[16];
    std::snprintf(lead, sizeof lead, "%06.0f", std::floor(std::pow(10.0, lg - std::floor(lg) + 5)));
    BigUint t = v;
    char tail[8];
    std::snprintf(tail, sizeof tail, "%04u", t.divmod_small(10000));
    return std::string(negative ? "-" : "") + lead + "..." + tail + " (~" + std::to_string(digits) + " digits)";
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
    // Real Graphics: a line's models (null where a file is missing: that part stays wireframe).
    struct Models
    {
        bool loaded = false;
        std::shared_ptr<const Mesh> hallway, bookshelf, book, marker;
        // Books more than a tile away: just their spines and tops (the rest cannot be seen there).
        std::shared_ptr<const Mesh> book_far;
        // The binary line: the same tile with one side taken out ([0] the right side gone, [1]
        // the left), and the short wall that stands on the edge where it went.
        std::shared_ptr<const Mesh> half[2], rail[2];
    };

    // The models line's shape, which is not a Line: V vertices, F triangles, a grid of C steps.
    struct ModelShape
    {
        uint32_t vertices = 8, faces = 12, coords = 16;
    };

    Hallway(SDL_Window* window, SDL_Renderer* renderer, std::vector<Line> lines, const FilterConfig& filters, uint32_t book_pages,
            ModelShape shape)
        : window_(window), r_(renderer), lines_(std::move(lines)), tile_geometry_(build_tile()), hall_geometry_(build_tile(true, false)), case_geometry_(build_tile(false, true)),
          bin_tile_{build_tile(true, true, -1), build_tile(true, true, 1)}, bin_hall_{build_tile(true, false, -1), build_tile(true, false, 1)},
          bin_case_{build_tile(false, true, -1), build_tile(false, true, 1)}, edge_geometry_{build_edge(1), build_edge(-1)},
          book_geometry_{build_books(false), build_books(true)}
    {
        // The books line: a cover from the image line, a title and book_pages pages from the pages line.
        books_ = std::make_unique<BookSpace>(lines_[1].space, lines_[0].space, book_pages);
        // The models line: V vertices and F triangles on a grid of C steps (SPECIFICATIONS §12).
        model_space_ = std::make_unique<ModelSpace>(shape.vertices, shape.faces, shape.coords, lines_[0].space.key());
        // Each line's filter stack and mode (sieve-filters.ini, edited in the setup menu).
        for (int i = 0; i < 4; ++i)
        {
            modes_[i] = filters.lines[i].mode;
            try
            {
                stacks_[i] = build_stack(lines_[size_t(i)], filters.lines[i]);
            }
            catch (const std::exception& e)
            {
                std::cerr << "filters for the " << to_string(lines_[size_t(i)].kind) << " line: " << e.what() << "\n";
                modes_[i] = FilterMode::Off;
                continue;
            }
            try
            {
                // Compact needs the survivors in every ordering; if that fails, the line hides instead.
                const Ranker* rk = stacks_[i].ranker();
                if (rk && !rk->count().is_zero())
                {
                    const Line& ln = lines_[size_t(i)];
                    compact_[i] = std::make_unique<CompactLine>(*rk, ln.space.key(), stacks_[i].id(),
                                                                ln.guided ? ln.guided->model_ptr() : nullptr);
                }
            }
            catch (const std::exception& e)
            {
                std::cerr << "compact for the " << to_string(lines_[size_t(i)].kind) << " line: " << e.what() << " (hiding instead)\n";
            }
        }
        // The books line: a stack per part (cover, title, and all pages as one text), one mode.
        modes_[kBooksLine] = filters.books.mode;
        try
        {
            book_stacks_ = build_book_stacks(lines_[1], lines_[0], book_pages, filters.books);
            book_sieve_ = std::make_unique<BookSieve>(*books_, book_stacks_.cover, book_stacks_.title, book_stacks_.pages);
        }
        catch (const std::exception& e)
        {
            std::cerr << "filters for the books line: " << e.what() << "\n";
            book_sieve_.reset();
            modes_[kBooksLine] = FilterMode::Off;
        }
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

    // The current line's units (the four unit lines; the books line has its own BookSpace).
    // The models line has no Line of its own (it is not made of one alphabet), so it borrows the
    // pages line's, as the books line does, for the few things that ask about a Line.
    const Line& line() const { return lines_[size_t(on_books() || on_models() || on_binary() ? 0 : li_)]; }
    bool on_books() const { return li_ == kBooksLine; }
    bool on_models() const { return li_ == kModelsLine; }
    // The binary line. It has no state space of its own yet -- its shelves stand empty, and how
    // they are addressed is still to be worked out (SPECIFICATIONS §12.1) -- so like the books
    // and models lines it borrows the pages line's Line for the few things that ask about one.
    bool on_binary() const { return li_ == kBinaryLine; }
    // Which wall of the binary line carries the shelves. The other side is the edge and the drop.
    // It is the wall you came in through, so which side you see the drop on depends on which end
    // of the corridor you walked out of: binary wraps around the outside of the other six lines,
    // and you meet it from either end.
    int shelf_side() const { return binary_shelf_; } // 0: shelves left, drop right. 1: the mirror.
    float drop_sign() const { return binary_shelf_ == 0 ? 1.0f : -1.0f; }
    // The current line's medium, and whether its books vary in size: pages, pictures and books do;
    // records (audio) and tapes (video) are all one size, as the real things are (world.hpp).
    Media media() const
    {
        if (on_books()) return Media::Books;
        if (on_models()) return Media::Models;
        if (on_binary()) return Media::Pages;
        switch (line().kind)
        {
        case LineKind::Image: return Media::Image;
        case LineKind::Audio: return Media::Audio;
        case LineKind::Video: return Media::Video;
        default: return Media::Pages;
        }
    }
    bool sizes_vary() const { return media_sizes_vary(media()); }
    const Theme& theme() const { return theme_of(li_); }
    Camera& camera() { return cam_; }
    bool guided_on() const { return !on_books() && !on_models() && !on_binary() && guided_ && line().guided != nullptr; }
    // The guided line in use: in compact mode, the one restricted to survivors.
    const GuidedLine& guided() const
    {
        return effective_mode() == FilterMode::Compact ? *compact_[li_]->guided() : *line().guided;
    }
    const CompactLine& compact() const { return *compact_[li_]; }
    std::string ordering_name() const { return tr(std::string("ordering.") + (guided_on() ? "guided" : to_string(mode_))); }

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
        for (int i = 0; i < kLines; ++i) all_loop_tiles_[i] = offset_loop_tile(all_loops_[i], all_loop_tiles_[i], d);
        // Keep the books already worked out that are still near enough to be drawn: they are the
        // same books, d tiles closer. (Books left behind are dropped, so the cache stays small.)
        std::unordered_map<int64_t, Book> shifted;
        if (d > -16 && d < 16)
        {
            const int64_t by = d * int64_t(sieve::books_per_tile());
            const int64_t lo = -int64_t(kCacheBack) * int64_t(sieve::books_per_tile()), hi = int64_t(kCacheAhead + 1) * int64_t(sieve::books_per_tile());
            for (auto& [key, b] : cache_)
                if (key - by >= lo && key - by < hi) shifted.emplace(key - by, std::move(b));
            // The rendered crate faces move with them: the same models, d tiles closer.
            std::unordered_map<int64_t, CrateFace> moved;
            for (auto& [key, cf] : crate_)
            {
                if (key - by >= lo && key - by < hi) moved.emplace(key - by, cf);
                else if (cf.tex) SDL_DestroyTexture(cf.tex);
            }
            crate_ = std::move(moved);
        }
        else clear_crate_faces(); // too far to be the same shelves: start the field again
        cache_ = std::move(shifted);
        refresh_labels();
    }

    // ---- filters
    //
    // Each line has a stack and a mode: off, mark (failing books dimmed), hide (failing books
    // left out, every address where it was) or compact (only survivors, closed up, in every
    // ordering: positional by survivor number, scrambled by a keyed shuffle of those numbers,
    // guided on the guided line restricted to survivors). Compact needs a stack that can rank its
    // survivors; otherwise the line hides instead.
    FilterMode effective_mode(int i) const
    {
        if (i == kBooksLine)
        {
            if (!book_sieve_ || book_sieve_->empty()) return FilterMode::Off;
            if (modes_[i] != FilterMode::Compact) return modes_[i];
            return books_compact() ? FilterMode::Compact : FilterMode::Hide;
        }
        const FilterStack& st = stacks_[i];
        if (st.empty()) return FilterMode::Off;
        const FilterMode m = modes_[i];
        if (m != FilterMode::Compact) return m;
        return compact_[i] ? FilterMode::Compact : FilterMode::Hide;
    }
    FilterMode effective_mode() const { return effective_mode(li_); }
    const FilterStack& stack() const { return stacks_[li_]; }
    // The books line compacts when every part with filters can rank its survivors, and some survive.
    bool books_compact() const { return book_sieve_ && book_sieve_->can_rank() && !book_sieve_->count().is_zero(); }
    // The models line has no filters yet (SPECIFICATIONS §12 sets out the three tiers to come).
    bool has_filters() const
    {
        if (on_models() || on_binary()) return false;
        return on_books() ? book_sieve_ && !book_sieve_->empty() : !stack().empty();
    }
    // The filters' part of the readout, worked out when the line or its settings change (the
    // survivor count is a big number, too slow to write out every frame).
    const std::string& filter_status() const { return filter_status_; }
    std::string compute_filter_status() const
    {
        if (on_books())
        {
            if (!has_filters()) return tr("hud.filters.none");
            const FilterMode m = effective_mode();
            const auto parts = std::to_string(book_stacks_.cover.size()) + "+" + std::to_string(book_stacks_.title.size()) + "+" +
                               std::to_string(book_stacks_.pages.size());
            std::string s = trf("hud.filters", {parts, tr(std::string("mode.") + to_string(m))});
            if (m == FilterMode::Compact) s += trf("hud.filters.books", {short_big(book_sieve_->count())});
            else if (modes_[li_] == FilterMode::Compact)
                s += book_sieve_->can_rank() ? tr("hud.filters.no_book") : trf("hud.filters.blocked", {book_sieve_->blocker()});
            return s;
        }
        const FilterStack& st = stack();
        if (st.empty()) return tr("hud.filters.none");
        const FilterMode m = effective_mode();
        std::string s = trf("hud.filters", {std::to_string(st.size()), tr(std::string("mode.") + to_string(m))});
        if (m == FilterMode::Compact) s += trf("hud.filters.units", {short_big(st.ranker()->count())});
        else if (modes_[li_] == FilterMode::Compact)
            s += st.ranker() ? tr("hud.filters.no_unit") : trf("hud.filters.blocked", {st.compact_blocker()});
        return s;
    }
    // How many units line i has in its loop, in the current ordering and filter mode.
    BigUint units_of(int i) const
    {
        // Binary is not counted yet: it stands for everything the other six do not address, and
        // its size is not a number we have. One loop tile keeps the shared machinery happy; every
        // place that would read a unit out of it is guarded by on_binary() instead.
        if (i == kBinaryLine) return BigUint(uint64_t(sieve::books_per_tile()));
        if (i == kModelsLine) return model_space_->size();
        if (i == kBooksLine) return effective_mode(i) == FilterMode::Compact ? book_sieve_->count() : books_->size();
        if (guided_ && lines_[size_t(i)].guided) return BigUint::pow(2, zoom_);
        if (effective_mode(i) == FilterMode::Compact) return compact_[i]->count();
        return lines_[size_t(i)].space.size();
    }

    // The loop of the current line in the current ordering, and which of its tiles you are in.
    // A full modulo is only needed when the line, ordering or zoom changes.
    // The readout's tile and loop labels, worked out when they change rather than every frame.
    void refresh_labels()
    {
        tile_label_ = short_big(tile_.magnitude, tile_.negative);
        loop_label_ = short_big(loop_.tiles());
    }

    void rebase()
    {
        // The guided zoom can never be finer than the line's own precision (16 bits per character).
        if (lines_[0].guided) zoom_ = std::clamp<uint32_t>(zoom_, 1, uint32_t(std::min<size_t>(lines_[0].guided->scale_bits(), UINT32_MAX)));
        loop_ = LineLoop(units_of(li_));
        loop_tile_ = loop_.loop_tile(tile_);
        // Every line's loop too, for the double flag where all four start together.
        for (int i = 0; i < kLines; ++i)
        {
            all_loops_[i] = LineLoop(units_of(i));
            all_loop_tiles_[i] = all_loops_[i].loop_tile(tile_);
        }
        cache_.clear();
        clear_crate_faces();
        filter_status_ = compute_filter_status();
        refresh_labels();
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
        bool passes = true;   // passes the line's filter stack
        std::string failed_by; // the first filter it fails
        bool survivor = false; // compact: a survivor, and its number (its place in positional order)
        BigUint survivor_number;
        std::optional<BookSpace::Parts> parts;       // the books line: cover, title and pages
        std::optional<ModelSpace::Parts> model;      // the models line: vertices and faces
    };

    const Book& book(int64_t dt, uint32_t slot)
    {
        const int64_t key = dt * int64_t(sieve::books_per_tile()) + slot;
        auto it = cache_.find(key);
        if (it != cache_.end()) return it->second;
        if (cache_.size() > 4096) cache_.clear(); // more than a screenful (14 tiles of 128 books)
        Book b;
        const auto idx = loop_.unit_index(offset_loop_tile(dt), slot);
        if (!idx) b.empty = true;
        else try
        {
            b.index = *idx;
            const Space& sp = line().space;
            const bool compact_here = effective_mode() == FilterMode::Compact;
            if (on_books())
            {
                if (compact_here)
                {
                    // Only surviving books stand here, closed up, like a compact line.
                    b.parts = book_sieve_->parts_at(b.index, mode_);
                    b.hex = book_sieve_->hex_of(b.index);
                    b.survivor = true;
                    b.survivor_number = mode_ == AddressMode::Positional ? b.index : book_sieve_->rank(*b.parts);
                    b.fraction = b.index.is_zero() ? 0.0 : std::pow(10.0, b.index.log10_approx() - book_sieve_->count().log10_approx());
                }
                else
                {
                    b.parts = books_->parts_at(b.index, mode_);
                    b.hex = books_->hex_of(b.index);
                    b.fraction = b.index.is_zero() ? 0.0 : std::pow(10.0, b.index.log10_approx() - books_->size().log10_approx());
                    if (book_sieve_ && !book_sieve_->empty())
                    {
                        b.failed_by = book_sieve_->first_failure(*b.parts);
                        b.passes = b.failed_by.empty();
                    }
                }
            }
            else if (on_models())
            {
                b.model = model_space_->parts_at(b.index, mode_);
                b.hex = model_space_->hex_of(b.index);
                b.fraction =
                    b.index.is_zero() ? 0.0 : std::pow(10.0, b.index.log10_approx() - model_space_->size().log10_approx());
            }
            else if (compact_here && !guided_on())
            {
                // Only survivors stand here: slot i holds the survivor whose compact address is i
                // (its survivor number, or in scrambled order a keyed shuffle of it).
                const CompactLine& cl = compact();
                b.unit = cl.unit_at(b.index, mode_);
                b.survivor = true;
                b.survivor_number = mode_ == AddressMode::Positional ? b.index : cl.ranker().rank(b.unit);
                b.hex = cl.hex_of(b.index);
                b.fraction = b.index.is_zero() ? 0.0 : std::pow(10.0, b.index.log10_approx() - cl.count().log10_approx());
            }
            else if (guided_on())
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
                if (compact_here)
                {
                    b.survivor = true;
                    b.survivor_number = compact().ranker().rank(b.unit);
                }
            }
            else
            {
                const auto address = b.index.to_digits(sp.base(), sp.unit_length());
                b.unit = sp.unit_of_address(address, mode_);
                b.hex = sp.hex_of(address);
                b.fraction = sp.fraction_of(address);
            }
            if (!on_books() && !on_models() && !b.survivor && !stack().empty())
            {
                const int fail = stack().first_failure(b.unit);
                b.passes = fail < 0;
                if (!b.passes) b.failed_by = stack().filter_name(size_t(fail));
            }
        }
        catch (const std::exception& e)
        {
            // Never let one book stop the frame: show the slot as empty and say why.
            b = Book{};
            b.empty = true;
            message(trf("msg.cannot_show", {e.what()}));
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
        if (effective_mode() == FilterMode::Compact) return compact().index_of(unit, mode_);
        return BigUint::from_digits(line().space.address_digits(unit, mode_), line().space.base());
    }

    // Go to a unit: its slot in the first copy of the loop, facing it. In compact mode a unit
    // that fails the stack has no shelf; it is shown in hand instead (SPECIFICATIONS 6.4).
    void go_to_unit(const Space::Digits& unit, bool open)
    {
        if (effective_mode() == FilterMode::Compact)
        {
            const int fail = stack().first_failure(unit);
            if (fail >= 0)
            {
                Book b;
                b.unit = unit;
                const auto address = line().space.address_digits(unit, AddressMode::Positional);
                b.hex = line().space.hex_of(address);
                b.fraction = line().space.fraction_of(address);
                b.passes = false;
                b.failed_by = stack().filter_name(size_t(fail));
                in_hand_ = b;
                hand_tab_ = 0;
                in_hand_where_ = trf("hand.not_shelved", {b.failed_by});
                return;
            }
        }
        place(index_of(unit), open);
    }

    // The books line: go to a book, face it, and optionally open it.
    void go_to_book(const BookSpace::Parts& p, bool open)
    {
        if (effective_mode() == FilterMode::Compact)
        {
            const std::string fail = book_sieve_->first_failure(p);
            if (!fail.empty())
            {
                // Not on the shelves: shown in hand, with its full (uncompacted) address.
                Book b;
                b.parts = p;
                const BigUint index = books_->index_of(p, AddressMode::Positional);
                b.index = index;
                b.hex = books_->hex_of(index);
                b.fraction = index.is_zero() ? 0.0 : std::pow(10.0, index.log10_approx() - books_->size().log10_approx());
                b.passes = false;
                b.failed_by = fail;
                drop_in_hand();
                in_hand_ = b;
                hand_tab_ = 0;
                in_hand_where_ = trf("hand.not_shelved", {fail});
                book_page_ = 0;
                return;
            }
            place(book_sieve_->index_of(p, mode_), open);
        }
        else place(books_->index_of(p, mode_), open);
        book_page_ = 0;
    }

    // A book record (sieve bind) as a book of this line: its title, cover and pages must have the
    // line's shape. A missing cover or title is shown blank, and missing pages are blank pages.
    BookSpace::Parts parts_of_record(const std::string& path)
    {
        std::ifstream in(std::filesystem::path(path), std::ios::binary);
        if (!in) throw std::runtime_error("cannot open '" + path + "'");
        const std::string text((std::istreambuf_iterator<char>(in)), {});
        const sieve::cli::Book book = parse_book(text);
        const auto decoded = decode_book(book);
        if (book_id(decoded) != book.id) throw std::runtime_error("the book's content does not match its id");
        try
        {
            return record_parts(decoded, *books_);
        }
        catch (const std::invalid_argument& e)
        {
            throw std::runtime_error(trf("msg.book_shape", {e.what()}));
        }
    }

    // Teleport to a unit's slot in the first copy of the loop (where position = address), face
    // it, and optionally open it.
    void place(const BigUint& index, bool open)
    {
        tile_ = LineLoop::tile_of(index);
        rebase();
        const uint32_t slot = LineLoop::slot_of(index);
        face(slot);
        drop_in_hand();
        book_page_ = 0;
        if (open)
        {
            in_hand_ = book(0, slot);
            hand_tab_ = 0;
            in_hand_where_ = trf("hand.where", {tile_label_, std::to_string(slot)});
        }
    }

    // Stand in the corridor facing the book in `slot` of the current tile.
    void face(uint32_t slot)
    {
        const BookSlot b = BookSlot::of(0, slot);
        Vec3 f[4];
        book_face(0, b.side, b.row, b.col, f, sizes_vary());
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
        if (on_books())
        {
            const Book* ref = reference_book();
            const auto parts = ref ? ref->parts : std::nullopt;
            mode_ = mode_ == AddressMode::Positional ? AddressMode::Scrambled : AddressMode::Positional;
            rebase();
            if (parts) go_to_book(*parts, false);
            message(trf(mode_ == AddressMode::Positional ? "msg.ordering.books_positional" : "msg.ordering.books_scrambled", {ordering_name()}));
            return;
        }
        const Book* ref = reference_book();
        const Space::Digits unit = ref ? ref->unit : Space::Digits{};
        if (guided_on()) { guided_ = false; mode_ = AddressMode::Positional; }
        else if (mode_ == AddressMode::Positional) mode_ = AddressMode::Scrambled;
        else if (line().guided) guided_ = true;
        else mode_ = AddressMode::Positional;
        rebase();
        if (ref) go_to_unit(unit, false);
        const char* what = guided_on() ? "msg.ordering.guided" : mode_ == AddressMode::Positional ? "msg.ordering.positional" : "msg.ordering.scrambled";
        message(trf(what, {ordering_name()}));
    }

    void zoom_to(uint32_t d) { zoom_by(int(d) - int(zoom_)); }

    // Guided order: change the spacing of the books, keeping the point you are looking at.
    void zoom_by(int delta)
    {
        if (!guided_on())
        {
            message(tr(line().guided && !on_books() ? "msg.zoom.press_m" : "msg.zoom.no_model"));
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
        message(trf(delta < 0 ? "msg.zoom.out" : "msg.zoom.in",
                    {std::to_string(zoom_), zoom_ < 64 ? "1/" + std::to_string(uint64_t(1) << zoom_) : "2^-" + std::to_string(zoom_)}));
    }

    bool warp(const std::string& input)
    {
        try
        {
            if (on_books())
            {
                // A book record, bound with sieve bind.
                go_to_book(parts_of_record(input), true);
                trail_.clear();
                message(trf("msg.opened_book", {input}));
                return true;
            }
            Args a;
            a.opts["line"] = to_string(line().kind);
            if (line().kind == LineKind::Image || line().kind == LineKind::Video) a.opts["file"] = input;
            else a.positional = {input};
            const WarpInput w = read_warp_input(line(), a);
            if (w.units.empty()) throw std::invalid_argument(tr("msg.warp.empty"));
            trail_ = w.units;
            trail_index_ = 0;
            go_to_unit(trail_[0], true);
            message(trf(trail_.size() > 1 ? "msg.warped.trail" : "msg.warped", {w.report.front()}));
            return true;
        }
        catch (const std::exception& e)
        {
            message(trf("msg.warp.failed", {e.what()}));
            return false;
        }
    }

    // An address (hex), a percentage (P%), or a corridor tile (@T).
    bool go_to(std::string input)
    {
        try
        {
            input.erase(std::remove(input.begin(), input.end(), ' '), input.end());
            if (input.empty()) throw std::invalid_argument(tr("msg.goto.empty"));
            if (input[0] == '@')
            {
                tile_ = TileIndex::parse(input.substr(1));
                rebase();
                face(0);
                drop_in_hand();
                trail_.clear();
                message(trf("msg.goto.tile", {tile_label_}));
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
                    throw std::invalid_argument(tr("msg.goto.percent"));
                // floor(p / 10^decimals * loop units)
                index = loop_.units();
                index.mul_small(uint32_t(std::stoul(digits)));
                for (uint32_t i = 0; i < decimals; ++i) index.divmod_small(10);
                if (index >= loop_.units()) throw std::invalid_argument(tr("msg.goto.below100"));
            }
            else if (guided_on())
            {
                const GuidedLine& g = guided();
                const BigUint point = g.point_of(input);
                zoom_ = uint32_t(std::clamp<size_t>(4 * input.size(), 1, g.scale_bits()));
                index = point;
                index >>= g.scale_bits() - zoom_;
            }
            else if (on_models()) index = model_space_->parse(input);
            else if (on_books()) index = effective_mode() == FilterMode::Compact ? book_sieve_->parse(input) : books_->parse(input);
            else if (effective_mode() == FilterMode::Compact) index = compact().parse(input); // a compact address, as the books show
            else index = BigUint::from_digits(line().space.parse_address(input), line().space.base());
            trail_.clear();
            place(index, true);
            message(trf("msg.goto.done", {input}));
            return true;
        }
        catch (const std::exception& e)
        {
            message(trf("msg.goto.failed", {e.what()}));
            return false;
        }
    }

    void step_trail(int dir)
    {
        if (trail_.size() < 2) return;
        trail_index_ = (trail_index_ + trail_.size() + size_t(dir)) % trail_.size();
        go_to_unit(trail_[trail_index_], true);
        message(trf("msg.trail", {std::to_string(trail_index_ + 1), std::to_string(trail_.size())}));
    }

    // ---- movement and doors

    void update(float dt, const bool* keys)
    {
        // Holding a book, you stand still: walking or turning under an open book is disorienting.
        if (input_ != Input::None || in_hand_) return;
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

    // How far left and right you may walk. Normally a wall on either side; on the binary line the
    // drop side has no wall, so you can walk out onto the walkway as far as the short wall, and
    // there is no door out there -- its one door is in its one wall of shelves.
    float walk_lo() const { return on_binary() && drop_sign() < 0 ? -(kEdgeRail - 0.35f) : -kWalkLimit; }
    float walk_hi() const { return on_binary() && drop_sign() > 0 ? kEdgeRail - 0.35f : kWalkLimit; }

    void move_by(Vec3 d)
    {
        // A doorway lets you through the wall; the binary line's open side has no wall and no
        // door, so out there the short wall stops you wherever you are along the tile.
        auto in_door = [this](float x, float z) {
            if (on_binary() && x * drop_sign() > 0) return false;
            return z > kDoorStart + 0.15f && z < kDoorEnd - 0.15f;
        };
        const float lo = walk_lo(), hi = walk_hi();
        Vec3 p = cam_.pos + d;
        // Walls stop you except in a doorway; once in a doorway you cannot slide along inside the wall.
        if ((p.x < lo || p.x > hi) && !in_door(p.x, p.z))
        {
            if (cam_.pos.x < lo || cam_.pos.x > hi) p.z = cam_.pos.z;
            else p.x = std::clamp(p.x, lo, hi);
        }
        cam_.pos = p;
        while (cam_.pos.z >= kTile) { cam_.pos.z -= kTile; move_tiles(1); }
        while (cam_.pos.z < 0) { cam_.pos.z += kTile; move_tiles(-1); }
        // The door is in the wall, and on the binary line there is only one wall.
        const bool left_door = !on_binary() || binary_shelf_ == 0, right_door = !on_binary() || binary_shelf_ == 1;
        if (cam_.pos.x < -(kHalfWidth + 0.05f) && left_door) cross(Side::Left);
        else if (cam_.pos.x > kHalfWidth + 0.05f && right_door) cross(Side::Right);
    }

    void jump_tiles(int64_t n)
    {
        if (in_hand_)
        {
            message(tr("msg.holding"));
            return;
        }
        move_tiles(n);
        message(trf("msg.jumped", {std::to_string(n), std::to_string(n * int64_t(sieve::books_per_tile()))}));
    }

    // A door keeps your corridor position and changes the line reading it: left wall to the next
    // line, right wall to the previous one. You come in through the opposite wall's door.
    void cross(Side side)
    {
        const int to = side == Side::Left ? (li_ + 1) % kLines : (li_ + kLines - 1) % kLines;
        cam_.pos.x = (kHalfWidth - 0.1f) * (side == Side::Left ? 1.0f : -1.0f);
        // You come out of binary's one wall of shelves, so that is the wall you came in through,
        // and the drop is on the other side. Which is why the edge is on your left at one end of
        // the corridor and on your right at the other: it is the same line, met from either end.
        if (to == kBinaryLine) binary_shelf_ = side == Side::Left ? 1 : 0;
        drop_in_hand();
        trail_.clear(); // a warped trail belongs to the line it was warped on
        set_line(to);
        message(trf("msg.door", {tr(theme().key), tile_label_}));
    }

    // ---- input events

    // Every key and click: anything that goes wrong becomes a message, never the end of the app.
    void handle(const SDL_Event& e, bool& quit)
    {
        try
        {
            handle_event(e, quit);
        }
        catch (const std::exception& ex)
        {
            message(trf("status.error", {ex.what()}));
        }
    }

    void handle_event(const SDL_Event& e, bool& quit)
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
            // Holding a model, the mouse turns the model instead of you: it is in your hands.
            if (in_hand_ && in_hand_->model)
            {
                model_spin_ += e.motion.xrel * 0.008f;
                model_tilt_ = std::clamp(model_tilt_ + e.motion.yrel * 0.008f * (invert_y_ ? -1.0f : 1.0f), -1.5f, 1.5f);
            }
            else if (!in_hand_)
            {
                cam_.yaw += e.motion.xrel * look_;
                cam_.pitch = std::clamp(cam_.pitch - e.motion.yrel * look_ * (invert_y_ ? -1.0f : 1.0f), -1.45f, 1.45f);
            }
        }
        if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
        {
            if (!SDL_GetWindowRelativeMouseMode(window_)) SDL_SetWindowRelativeMouseMode(window_, true);
            else if (e.button.button == SDL_BUTTON_LEFT) take_or_return();
        }
        if (e.type == SDL_EVENT_MOUSE_WHEEL)
        {
            // Touchpads send fractions of a notch: a tile per whole notch, however it arrives.
            wheel_ += e.wheel.y;
            const int notches = int(wheel_);
            if (notches != 0)
            {
                wheel_ -= float(notches);
                jump_tiles(notches);
            }
        }
        if (e.type != SDL_EVENT_KEY_DOWN) return;
        // Held keys repeat only for what is meant to repeat (jumps, zoom, pages); a held E would
        // otherwise take a book and put it back again and again.
        if (e.key.repeat)
            switch (e.key.key)
            {
            case SDLK_PAGEUP: case SDLK_PAGEDOWN: case SDLK_LEFTBRACKET: case SDLK_RIGHTBRACKET:
            case SDLK_MINUS: case SDLK_KP_MINUS: case SDLK_EQUALS: case SDLK_KP_PLUS: case SDLK_N: case SDLK_B: break;
            default: return;
            }
        switch (e.key.key)
        {
        case SDLK_ESCAPE:
            if (in_hand_) drop_in_hand();
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
        case SDLK_N:
            if (in_hand_ && in_hand_->parts) turn_page(1);
            else step_trail(1);
            break;
        case SDLK_B:
            if (in_hand_ && in_hand_->parts) turn_page(-1);
            else step_trail(-1);
            break;
        case SDLK_A:
            if (in_hand_ && in_hand_->model) model_spin_ -= 0.15f;
            break;
        case SDLK_D:
            if (in_hand_ && in_hand_->model) model_spin_ += 0.15f;
            break;
        case SDLK_C:
            // The item page's tabs: the thing itself, then what it costs to name it.
            if (in_hand_) hand_tab_ = (hand_tab_ + 1) % 2;
            break;
        case SDLK_R:
            if (in_hand_ && in_hand_->model) { model_spin_ = 0.6f; model_tilt_ = 0.35f; }
            break;
        case SDLK_P:
            if (in_hand_ && !on_books() && line().kind == LineKind::Audio)
            {
                const std::string err = synth_.play(in_hand_->unit);
                message(err.empty() ? tr("msg.playing") : err);
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
            drop_in_hand();
            message(tr("msg.home"));
            break;
        default: break;
        }
    }

    void turn_page(int dir)
    {
        const int n = int(books_->pages());
        if (n == 0) return;
        book_page_ = std::clamp(book_page_ + dir, 0, n - 1);
    }

    void drop_in_hand()
    {
        in_hand_.reset();
        synth_.stop();
    }

    void take_or_return()
    {
        if (in_hand_) { drop_in_hand(); return; }
        take_hovered();
    }

    void take_hovered()
    {
        if (!hover_) return;
        const Book& b = book(hover_->tile, hover_->slot());
        if (b.empty) return;
        in_hand_ = b;
        hand_tab_ = 0;
        book_page_ = 0;
        TileIndex t = tile_;
        t += hover_->tile;
        in_hand_where_ = trf("hand.where", {short_big(t.magnitude, t.negative), std::to_string(hover_->slot())});
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
        where += ", " + filter_status() + ", tile " + tile_label_ + ", x " + std::to_string(cam_.pos.x) + ", ";
        if (first.empty) where += "first slot empty (padding)";
        else
        {
            where += percent(first.fraction) + " along, first book " + short_address(first.hex);
            if (first.parts) where += " \"" + ascii(utf8_encode(line().space.text_of(first.parts->title))) + "\"";
            else if (first.model)
            {
                const auto fs = model_space_->faces_of(*first.model);
                std::vector<bool> seen(model_space_->vertices(), false);
                uint32_t used = 0, degenerate = 0;
                for (const auto& f : fs)
                {
                    seen[f.a] = seen[f.b] = seen[f.c] = true;
                    if (f.a == f.b || f.b == f.c || f.a == f.c) ++degenerate;
                }
                for (bool b : seen)
                    if (b) ++used;
                where += " " + std::to_string(used) + "/" + std::to_string(model_space_->vertices()) + " vertices used, " +
                         std::to_string(degenerate) + " degenerate faces";
            }
            else if (line().kind == LineKind::Text) where += " \"" + ascii(utf8_encode(line().space.text_of(first.unit))) + "\"";
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
        ++portal_frame_;
        const Theme& th = theme();
        SDL_SetRenderDrawColor(r_, th.bg.r, th.bg.g, th.bg.b, 255);
        SDL_RenderClear(r_);
        SDL_SetRenderDrawBlendMode(r_, SDL_BLENDMODE_BLEND);

        // Nothing stands on the binary line's shelves, so there is nothing to look at or take.
        hover_ = on_binary() ? std::nullopt : pick_book(cam_.pos, cam_.forward(), 0, 5.0f, sizes_vary());
        if (hover_ && effective_mode() == FilterMode::Hide && !book(hover_->tile, hover_->slot()).passes) hover_.reset();

        constexpr int kBack = kCacheBack, kAhead = kCacheAhead;
        // Only tiles that can appear on screen are drawn (usually about half of them).
        bool visible[kBack + kAhead + 1];
        for (int t = -kBack; t <= kAhead; ++t)
        {
            const Vec3 shift{0, 0, t * kTile};
            visible[t + kBack] = cam_.box_visible(tile_lo_ + shift, tile_hi_ + shift);
        }
        // Real Graphics: the line's models, where it has them; the rest stays wireframe.
        fps_tris_ = 0;
        const Models* md = real_graphics_ ? &models() : nullptr;
        const bool real_hall = md && md->hallway, real_cases = md && md->bookshelf, real_books = md && md->book,
                   real_marker = md && md->marker;
        bool models_drawn = false;
        if (md && (real_hall || real_cases || real_books || real_marker))
        {
            draw_models(*md, visible, kBack, kAhead, w, h);
            models_drawn = true;
        }
        // The portals are drawn over that image, so they test against its depth (see draw_portal).
        const float* depth = models_drawn ? models_batch_.depth() : nullptr;
        // Doors: solid black. Start lines: checkered, where a loop of this line begins.
        // Far tile first, so a nearer door's black face covers the one behind it.
        for (int t = kAhead; t >= -kBack; --t)
        {
            if (!visible[t + kBack]) continue;
            const float z0 = t * kTile;
            for (float sx : {-1.0f, 1.0f})
            {
                // The binary line has one wall, so it has one door: nothing to draw on the side
                // the floor ends at.
                if (on_binary() && sx == drop_sign()) continue;
                const float x = sx * kHalfWidth;
                const std::vector<Vec3> door = {{x, 0, z0 + kDoorStart}, {x, 0, z0 + kDoorEnd}, {x, kDoorTop, z0 + kDoorEnd}, {x, kDoorTop, z0 + kDoorStart}};
                if (!real_hall) fill(door, SDL_Color{0, 0, 0, 255});
                // A door leads to the next line on the left wall and the previous one on the
                // right (cross()), so each portal takes the colour of the line behind it.
                if (door_portals_)
                    draw_portal(door, theme_of(sx < 0 ? (li_ + 1) % kLines : (li_ + kLines - 1) % kLines),
                                depth, models_batch_.width(), models_batch_.height());
            }
            if (!real_marker && offset_loop_tile(t).is_zero())
            {
                draw_start_line(z0);
                if (all_start(t)) draw_start_line(z0 + 1.0f); // every line starts here: a double flag
            }
        }
        // The book you are looking at.
        if (hover_ && !book(hover_->tile, hover_->slot()).empty)
        {
            Vec3 f[4];
            book_face(float(hover_->tile) * kTile, hover_->side, hover_->row, hover_->col, f, sizes_vary());
            SDL_Color c = th.edge;
            c.a = 110;
            fill({f[0], f[1], f[2], f[3]}, c);
        }
        // Every edge, faded towards the background with distance. Padding slots have no book.
        // (The buckets are kept between frames, so their memory is reused.)
        std::vector<std::vector<SDL_FPoint>>& buckets = edge_buckets_;
        buckets.resize(kBuckets);
        for (auto& b : buckets) b.clear();
        // `faint` is the least fade an edge gets (0 = full strength, 1 = background).
        auto add = [&](const Segment& s, float z0, float faint = 0) {
            const auto p = cam_.project_segment({s.a.x, s.a.y, s.a.z + z0}, {s.b.x, s.b.y, s.b.z + z0});
            if (!p) return;
            const float fade = std::clamp(std::max((p->second - 10.0f) / 45.0f, faint), 0.0f, 1.0f);
            auto& b = buckets[size_t(std::min(kBuckets - 1, int(fade * kBuckets)))];
            b.push_back({p->first.first.x, p->first.first.y});
            b.push_back({p->first.second.x, p->first.second.y});
        };
        for (int t = -kBack; t <= kAhead; ++t)
        {
            if (!visible[t + kBack]) continue;
            const float z0 = t * kTile;
            static const std::vector<Segment> none;
            const int b = binary_shelf_;
            const std::vector<Segment>& edges =
                on_binary() ? (real_hall ? (real_cases ? none : bin_case_[b]) : (real_cases ? bin_hall_[b] : bin_tile_[b]))
                            : (real_hall ? (real_cases ? none : case_geometry_) : (real_cases ? hall_geometry_ : tile_geometry_));
            for (const Segment& s : edges) add(s, z0);
            // The binary line's open side: the short wall on the last edge of the floor. Under
            // Real Graphics it is a solid model instead (draw_models), so it is not drawn twice.
            if (on_binary() && !real_hall)
                for (const Segment& s : edge_geometry_[b]) add(s, z0);
            const uint32_t books = real_books ? 0 : books_in_tile(t);
            const FilterMode fm = effective_mode();
            for (uint32_t k = 0; k < books; ++k)
            {
                // Mark: books that fail the filters are drawn faint, so survivors stand out.
                // Hide: they are left out.
                float dim = 0;
                if (fm == FilterMode::Mark || fm == FilterMode::Hide)
                    if (!book(t, k).passes)
                    {
                        if (fm == FilterMode::Hide) continue;
                        dim = 0.8f;
                    }
                for (const Segment& s : book_geometry_[sizes_vary()][k]) add(s, z0, dim);
            }
        }
        // The binary line: the rain falling off its edge. Only there -- the other six lines are
        // bounded by it, they do not contain it.
        if (on_binary())
        {
            render_rain();
            draw_binary_edge(visible, kBack, kAhead, real_hall);
        }
        // The models line: the rendered face of every crate in view, and a few more rendered.
        if (on_models()) draw_crate_faces(visible, kBack, kAhead);
        if (edge_glow_ && !md) draw_glow(buckets);
        for (int i = 0; i < kBuckets; ++i)
        {
            const SDL_Color c = mix(th.edge, th.bg, float(i) / kBuckets);
            SDL_SetRenderDrawColor(r_, c.r, c.g, c.b, 255);
            const auto& b = buckets[size_t(i)];
            for (size_t k = 0; k + 1 < b.size(); k += 2) SDL_RenderLine(r_, b[k].x, b[k].y, b[k + 1].x, b[k + 1].y);
        }
        draw_hud(w, h);
    }

    // ---- Real Graphics

    static const char* media_name(Media m)
    {
        switch (m)
        {
        case Media::Image: return "image";
        case Media::Audio: return "audio";
        case Media::Video: return "video";
        case Media::Books: return "books";
        case Media::Models: return "models";
        default: return "pages";
        }
    }

    // The current line's models, loaded the first time they are needed.
    const Models& models()
    {
        Models& m = models_[li_];
        if (!m.loaded)
        {
            const std::string medium = media_name(media());
            m.hallway = load_model("hallway", medium);
            m.bookshelf = load_model("bookshelf", medium);
            m.book = load_model("book", medium);
            if (m.book) m.book_far = facing_x(*m.book);
            m.marker = load_model("marker", medium);
            if (li_ == kBinaryLine && m.hallway)
                for (int k = 0; k < 2; ++k)
                {
                    const float sign = k == 0 ? 1.0f : -1.0f;
                    m.half[k] = half_x(*m.hallway, sign, kHalfWidth - 0.3f);
                    const float x = sign * kEdgeRail;
                    m.rail[k] = box_mesh({std::min(x, x - sign * 0.12f), 0, 0},
                                         {std::max(x, x - sign * 0.12f), kEdgeRailTop, kTile}, kRailColour);
                }
            m.loaded = true;
            std::cerr << "real graphics for the " << medium << " line:";
            for (const auto& [name, mesh] : {std::pair{"hallway", m.hallway}, {"bookshelf", m.bookshelf}, {"book", m.book}, {"marker", m.marker}})
                std::cerr << " " << name << "=" << (mesh ? std::filesystem::path(mesh->source).filename().string() : std::string("wireframe"));
            std::cerr << "\n";
        }
        return m;
    }

    // Every visible tile's models, drawn with a depth buffer (see MeshBatch in mesh.hpp).
    void draw_models(const Models& md, const bool* visible, int back, int ahead, int w, int h)
    {
        const Theme& th = theme();
        models_batch_.begin(cam_, th.bg, w, h);
        const bool varied = sizes_vary();
        const FilterMode fm = effective_mode();
        for (int t = -back; t <= ahead; ++t)
        {
            if (!visible[t + back]) continue;
            const float z0 = float(t) * kTile;
            // The binary line is the same tile of corridor with one side taken out, and a short
            // wall standing where it went. Everything else about it is an ordinary tile.
            const int bs = binary_shelf_;
            if (on_binary() && md.half[bs]) models_batch_.add(*md.half[bs], {{0, 0, z0}});
            else if (md.hallway) models_batch_.add(*md.hallway, {{0, 0, z0}});
            if (on_binary() && md.rail[bs]) models_batch_.add(*md.rail[bs], {{0, 0, z0}});
            if (md.marker && offset_loop_tile(t).is_zero())
            {
                // Pulled a little nearer than the floor it lies on, so the floor never shows through.
                Placement at{{0, 0, z0}};
                at.depth_bias = 0.002f;
                models_batch_.add(*md.marker, at);
                at.offset.z += 1.0f; // every line starts here: a second strip
                if (all_start(t)) models_batch_.add(*md.marker, at);
            }
            // Tiles more than one away from the camera's are far enough for the simpler models.
            const bool near = t >= -1 && t <= 1;
            if (md.bookshelf)
            {
                // One wall, one bookcase, on the binary line; both walls everywhere else.
                if (!on_binary() || bs == 0) models_batch_.add(*md.bookshelf, {{0, 0, z0}});
                if (!on_binary() || bs == 1) models_batch_.add(*md.bookshelf, {{0, 0, z0}, 1.0f, 1.0f, true});
            }
            if (!md.book) continue;
            const Mesh& book_mesh = near ? *md.book : *md.book_far;
            const uint32_t books = books_in_tile(t);
            for (uint32_t k = 0; k < books; ++k)
            {
                float dim = 0;
                if (fm == FilterMode::Mark || fm == FilterMode::Hide)
                    if (!book(t, k).passes)
                    {
                        if (fm == FilterMode::Hide) continue;
                        dim = 0.8f;
                    }
                const BookSlot b = BookSlot::of(0, k);
                const float y0 = kRowTop - float(b.row + 1) * kRowHeight + 0.02f;
                const float zc = z0 + float(b.col) * book_pitch() + book_pitch() * 0.5f;
                const bool right = b.side == Side::Right;
                const float sx = right ? 1.0f : -1.0f;
                // Skip books that cannot be on screen (their slot's box).
                if (!cam_.box_visible({right ? kCaseFront - 0.4f : -kHalfWidth, y0, zc - book_pitch() * 0.5f},
                                      {right ? kHalfWidth : -kCaseFront + 0.4f, y0 + 0.55f, zc + book_pitch() * 0.5f}))
                    continue;
                // Everything on a shelf is scaled to its slot, keeping its proportions, so more
                // to a tile means smaller items rather than squashed ones. On top of that, pages,
                // pictures and books take their slot's own height, where records and tapes do not
                // -- a record is one size whatever else changes (world.hpp: media_sizes_vary).
                const float tall = varied ? book_height(b.row, b.col, true) / (kUniformBookHeight * shelf_scale()) : 1.0f;
                models_batch_.add(book_mesh, {{sx * kCaseFront, y0, zc}, shelf_scale(), tall, right, dim});
            }
        }
        models_batch_.draw(r_);
        fps_tris_ = models_batch_.drawn();
    }

    // Geometry Edge Glow: under each edge, a soft band in the edge's colour that fades to nothing
    // on both sides, in two layers (wide and faint, narrow and brighter). Ordinary alpha blending,
    // so it brightens light edges on dark lines and darkens dark edges on light ones (books).
    // Far edges, already faded towards the background, get a thinner and fainter glow. All of it
    // is drawn in a few batched calls, one per distance bucket.
    void draw_glow(const std::vector<std::vector<SDL_FPoint>>& buckets)
    {
        const Theme& th = theme();
        const int n = int(buckets.size());
        std::vector<SDL_Vertex> v;
        std::vector<int> idx;
        for (int i = 0; i < n; ++i)
        {
            const auto& b = buckets[size_t(i)];
            if (b.empty()) continue;
            const float near = 1.0f - float(i) / float(n);
            const SDL_Color c = mix(th.edge, th.bg, float(i) / float(n));
            v.clear();
            idx.clear();
            v.reserve(b.size() * 6);
            idx.reserve(b.size() * 12);
            for (const auto& [width, alpha] : {std::pair{10.0f, 0.22f}, std::pair{4.0f, 0.45f}})
            {
                const float w = width * (0.35f + 0.65f * near);
                const SDL_FColor core{c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, alpha * (0.4f + 0.6f * near)};
                const SDL_FColor clear{core.r, core.g, core.b, 0.0f};
                for (size_t k = 0; k + 1 < b.size(); k += 2)
                {
                    const SDL_FPoint a = b[k], e = b[k + 1];
                    float dx = e.x - a.x, dy = e.y - a.y;
                    const float len = std::sqrt(dx * dx + dy * dy);
                    if (len < 0.5f) continue;
                    dx /= len;
                    dy /= len;
                    // The band runs a little past each end, so corners glow too.
                    const SDL_FPoint a2{a.x - dx * w * 0.5f, a.y - dy * w * 0.5f}, e2{e.x + dx * w * 0.5f, e.y + dy * w * 0.5f};
                    const float nx = -dy * w, ny = dx * w;
                    const int base = int(v.size());
                    v.push_back({{a2.x + nx, a2.y + ny}, clear, {0, 0}});
                    v.push_back({{e2.x + nx, e2.y + ny}, clear, {0, 0}});
                    v.push_back({{a2.x, a2.y}, core, {0, 0}});
                    v.push_back({{e2.x, e2.y}, core, {0, 0}});
                    v.push_back({{a2.x - nx, a2.y - ny}, clear, {0, 0}});
                    v.push_back({{e2.x - nx, e2.y - ny}, clear, {0, 0}});
                    for (int t : {0, 1, 2, 1, 3, 2, 2, 3, 4, 3, 5, 4}) idx.push_back(base + t);
                }
            }
            if (!v.empty()) SDL_RenderGeometry(r_, nullptr, v.data(), int(v.size()), idx.data(), int(idx.size()));
        }
    }

    // A checkered strip across the floor at z0: the start (and end) of a loop of the current line.
    // Where every line starts together there are two strips, a metre apart.
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

    // ---- Door Portals
    //
    // A doorway is 99.99% un-sieved entropy seen edge on, so it is drawn as entropy: procedural
    // data noise in the colour of the line it leads to, fading to black at the frame so it reads
    // as a field held inside the door rather than a hole in the wall.
    //
    // The noise is an integer hash of the screen pixel and the frame — no trigonometry, no
    // random number generator, a handful of instructions per pixel — rasterised straight into a
    // small buffer covering the door's part of the screen and uploaded as one texture. Cost is
    // the doors' area in pixels, so a door across the corridor costs almost nothing.

    // One noise cell is this many screen pixels square. Two keeps the grain visible on a big
    // display, reads as data rather than television snow, and costs a quarter of per-pixel noise.
    static constexpr int kGrain = 2;
    // Smoothstep as 256 steps, so the fade costs a lookup rather than three multiplies.
    static const std::array<uint32_t, 256>& smooth_table()
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
    static float noise_at(int x, int y, uint32_t seed)
    {
        uint32_t h = uint32_t(x) * 0x9E3779B1u ^ uint32_t(y) * 0x85EBCA77u ^ seed * 0x27D4EB2Fu;
        h ^= h >> 15;
        h *= 0x2545F491u;
        h ^= h >> 13;
        return float(h >> 24);
    }

    // One octave of the portal's cloud: value noise on a lattice `size` cells apart, read along a
    // row. The two nodes on the right become the two on the left as the row crosses into the next
    // node, so a row of cells costs two hashes per node rather than four per cell.
    struct Octave
    {
        // Quintic easing: its slope is zero at both ends, so neighbouring cells of the lattice
        // blend without the crease that makes plain smoothstep look like a grid of squares.
        static float ease(float t) { return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f); }

        // The easing is the same handful of values over and over (one per cell of a node), so it
        // is worked out once, and the lattice is stepped rather than divided into.
        void init(int lattice, uint32_t s)
        {
            size = std::min(lattice, int(eased_.size()));
            seed = s;
            for (int i = 0; i < size; ++i) eased_[size_t(i)] = ease(float(i) / float(size));
        }
        // `yq` is the row in 1/256 of a cell, so the cloud can drift by less than a whole cell.
        void row(uint32_t yq)
        {
            const uint32_t node = uint32_t(size) * 256;
            gy_ = int(yq / node);
            ty_ = ease(float(yq % node) * (1.0f / float(node)));
        }
        // The first cell of a row: the only division either octave does per row.
        void start(int cx)
        {
            gx_ = cx / size;
            sub_ = cx - gx_ * size;
            h00_ = noise_at(gx_, gy_, seed);
            h01_ = noise_at(gx_, gy_ + 1, seed);
            h10_ = noise_at(gx_ + 1, gy_, seed);
            h11_ = noise_at(gx_ + 1, gy_ + 1, seed);
        }
        float value() const
        {
            const float tx = eased_[size_t(sub_)];
            const float top = h00_ + (h10_ - h00_) * tx, bot = h01_ + (h11_ - h01_) * tx;
            return top + (bot - top) * ty_;
        }
        // On to the next cell; crossing into the next node, the two values on the right become
        // the two on the left, so a row costs two hashes per node rather than four per cell.
        void step()
        {
            if (++sub_ < size) return;
            sub_ = 0;
            ++gx_;
            h00_ = h10_;
            h01_ = h11_;
            h10_ = noise_at(gx_ + 1, gy_, seed);
            h11_ = noise_at(gx_ + 1, gy_ + 1, seed);
        }

        int size = 6;
        uint32_t seed = 0;

    private:
        int gy_ = 0, gx_ = 0, sub_ = 0;
        float ty_ = 0, h00_ = 0, h10_ = 0, h01_ = 0, h11_ = 0;
        std::array<float, 32> eased_{};
    };

    // The line's own colour for its portal: whichever of its two is the brighter, so every line
    // reads (white for PAGES, cyan for IMAGE, amber for AUDIO, yellow for VIDEO, grey for BOOKS).
    static SDL_Color portal_colour(const Theme& th)
    {
        auto luma = [](SDL_Color c) { return 2 * int(c.r) + 5 * int(c.g) + int(c.b); };
        return luma(th.edge) >= luma(th.bg) ? th.edge : th.bg;
    }

    // `depth` is the models' depth buffer when Real Graphics drew this frame, else null: the
    // portal is drawn over that image, so without it a door far down the corridor would show
    // through the bookcases between you and it.
    void draw_portal(const std::vector<Vec3>& quad, const Theme& dest, const float* depth, int dw, int dh)
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
            if (portal_.tex) SDL_DestroyTexture(portal_.tex);
            portal_.w = std::max(portal_.w, nw);
            portal_.h = std::max(portal_.h, nh);
            portal_.tex = SDL_CreateTexture(r_, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, portal_.w, portal_.h);
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

    // Text cut to fit `width` pixels at `scale` (8 pixels per character at scale 1).
    static std::string fit(const std::string& s, float width, float scale)
    {
        const size_t cols = size_t(std::max(4.0f, width / (8 * scale)));
        return text_cells(s) <= cols ? s : fit_cells(s, cols - 2) + "..";
    }

    // Text in the language's font (8 pixels per cell at scale 1; see font.hpp).
    void text(float x, float y, const std::string& s, float scale, SDL_Color c) { draw_text(r_, x, y, s, scale, c); }

    // The compass. Every line is a loop, so the corridor is a set of concentric circles: binary
    // outermost, then the six it bounds, then binary again innermost, because binary wraps around
    // the outside of the others and is met from either end. A needle runs from the middle out to
    // the ring you are standing on, at the angle you stand at; every other ring carries a mark at
    // its own angle, because each line loops at its own rate and they only agree at zero. Zero is
    // at the top, where every loop starts and finishes.
    //
    // This reads the same corridor position the rest of the hallway does, so nothing here decides
    // anything: it is the readout drawn round instead of along.
    static SDL_Color ring_ink(const Theme& t, SDL_Color on)
    {
        auto lum = [](SDL_Color c) { return (c.r * 3 + c.g * 6 + c.b) / 10; };
        const int b = lum(on);
        const SDL_Color pick = std::abs(lum(t.edge) - b) >= std::abs(lum(t.bg) - b) ? t.edge : t.bg;
        if (std::abs(lum(pick) - b) >= 40) return pick;
        // Both of a line's colours sit too close to the panel behind it: lift it until it reads.
        const float k = b < 128 ? 1.0f : -1.0f;
        auto f = [k](Uint8 v) { return Uint8(std::clamp(float(v) + k * 90.0f, 0.0f, 255.0f)); };
        return {f(pick.r), f(pick.g), f(pick.b), 255};
    }

    // Where line i stands in its own loop, 0 at the start of a copy and approaching 1 at its end.
    //
    // Measured in units rather than in tiles, so it is the same number the readout writes as a
    // percentage. The two differ whenever a loop does not fill its last tile: three tiles into a
    // six-tile loop of 729 units is not half way along it, it is 384/729 of the way.
    double line_fraction(int i) const
    {
        const BigUint& at = all_loop_tiles_[size_t(i)];
        if (at.is_zero()) return 0.0;
        const BigUint units = units_of(i);
        if (units.is_zero()) return 0.0;
        BigUint first = at; // the first unit of the tile you are in
        first <<= sieve::books_per_tile_bits();
        const double f = std::pow(10.0, first.log10_approx() - units.log10_approx());
        return std::clamp(f, 0.0, 1.0);
    }

    void draw_compass(float W, float H)
    {
        const Theme& th = theme();
        const float r_out = 92, pad = 15, row = 20;
        const float bw = 2 * r_out + 2 * pad, bh = bw + row;
        const float bx = W - bw - 10, by = H - bh - 30;
        panel(bx, by, bw, bh);
        const float cx = bx + bw / 2, cy = by + pad + r_out;
        // Rings from the outside in: binary, the six in door order, binary again. The picture is
        // the corridor read from one edge to the other, bent into circles.
        const int kRings = kLines + 1;
        const float step = (r_out - 12) / float(kRings - 1);
        for (int k = 0; k < kRings; ++k)
        {
            const int line = k == 0 || k == kRings - 1 ? kBinaryLine : k - 1;
            const float rad = r_out - float(k) * step;
            const bool here = line == li_ && (line != kBinaryLine || k == 0);
            const SDL_Color c = ring_ink(theme_of(line), th.bg);
            SDL_SetRenderDrawColor(r_, c.r, c.g, c.b, here ? 255 : 190);
            const int segs = std::max(32, int(rad * 1.4f));
            for (int j = 0; j < segs; ++j)
            {
                const float a0 = float(j) / segs * 6.2831853f, a1 = float(j + 1) / segs * 6.2831853f;
                auto arc = [&](float d) {
                    SDL_RenderLine(r_, cx + (rad + d) * std::sin(a0), cy - (rad + d) * std::cos(a0),
                                   cx + (rad + d) * std::sin(a1), cy - (rad + d) * std::cos(a1));
                };
                arc(0);
                if (here) { arc(-1); arc(1); }
            }
            // Where that line stands in its own loop. They only agree at zero.
            if (line == kBinaryLine) continue;
            const float a = float(line_fraction(line)) * 6.2831853f;
            SDL_SetRenderDrawColor(r_, c.r, c.g, c.b, 255);
            const SDL_FRect dot{cx + rad * std::sin(a) - 2, cy - rad * std::cos(a) - 2, 5, 5};
            SDL_RenderFillRect(r_, &dot);
        }
        const SDL_Color ink = th.edge;
        // Zero at the top, where every loop starts and finishes.
        SDL_SetRenderDrawColor(r_, ink.r, ink.g, ink.b, 255);
        SDL_RenderLine(r_, cx, cy - r_out - 2, cx, cy - r_out - 11);
        text(cx + 6, cy - r_out - 15, tr("hud.zero"), 1, ink);
        // The needle. It runs the whole radius so the bearing is easy to read off, and is drawn
        // bright as far as the ring you are standing on, faint beyond it.
        const int mine = on_binary() ? 0 : li_ + 1;
        const float rad = r_out - float(mine) * step;
        const float a = on_binary() ? 0.0f : float(line_fraction(li_)) * 6.2831853f;
        const float sn = std::sin(a), cs = std::cos(a);
        SDL_SetRenderDrawColor(r_, ink.r, ink.g, ink.b, 80);
        SDL_RenderLine(r_, cx + rad * sn, cy - rad * cs, cx + (r_out + 2) * sn, cy - (r_out + 2) * cs);
        SDL_SetRenderDrawColor(r_, ink.r, ink.g, ink.b, 255);
        for (float d : {-0.5f, 0.5f}) SDL_RenderLine(r_, cx + d, cy, cx + d + rad * sn, cy - rad * cs);
        const SDL_FRect at{cx + rad * sn - 3, cy - rad * cs - 3, 7, 7};
        SDL_RenderFillRect(r_, &at);
        // And the same bearing written out, to whatever precision Settings > Graphics asks for.
        const std::string deg = on_binary() ? tr("hud.no_angle") : degrees(line_fraction(li_), angle_decimals_);
        text(cx - text_width(deg, 1) / 2, by + bh - row + 4, deg, 1, ink);
    }

    // `alpha`: the readout's panels let a little of the corridor through, but the item page is
    // something you stop and read, so it is drawn solid.
    void panel(float x, float y, float w, float h, Uint8 alpha = 230)
    {
        const Theme& th = theme();
        SDL_SetRenderDrawColor(r_, th.bg.r, th.bg.g, th.bg.b, alpha);
        const SDL_FRect box{x, y, w, h};
        SDL_RenderFillRect(r_, &box);
        SDL_SetRenderDrawColor(r_, th.edge.r, th.edge.g, th.edge.b, 255);
        SDL_RenderRect(r_, &box);
    }

    void draw_pixels(const Space::Digits& unit, float x, float y, float size, int frame)
    {
        draw_pixels(unit, on_books() ? lines_[1].image : line().image, x, y, size, frame);
    }
    void draw_pixels(const Space::Digits& unit, const ImageFormat& f, float x, float y, float size, int frame)
    {
        const float cell = size / float(std::max(f.width, f.height));
        const size_t base = size_t(frame) * f.width * f.height;
        // One texture, scaled with nearest-neighbour sampling (one rectangle per pixel costs a draw
        // call per pixel); per-pixel rectangles only if the texture cannot be made. The picture in
        // front of you and the one in hand rarely change, so their textures are kept between frames.
        static_assert(sizeof(Rgb) == 3, "Rgb must be packed RGB24");
        PictureCache& c = picture_[&f == &lines_[1].image && on_books() ? 1 : 0];
        const bool same = c.tex && c.unit == unit && c.frame == frame && c.w == f.width && c.h == f.height;
        if (!same)
        {
            if (c.tex && (c.w != f.width || c.h != f.height))
            {
                SDL_DestroyTexture(c.tex);
                c.tex = nullptr;
            }
            if (!c.tex) c.tex = SDL_CreateTexture(r_, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STATIC, int(f.width), int(f.height));
            if (c.tex)
            {
                const auto px = render_image(unit, f);
                SDL_UpdateTexture(c.tex, nullptr, px.data() + base, int(f.width * 3));
                SDL_SetTextureScaleMode(c.tex, SDL_SCALEMODE_NEAREST);
                c.unit = unit;
                c.frame = frame;
                c.w = f.width;
                c.h = f.height;
            }
        }
        if (c.tex)
        {
            const SDL_FRect dst{x, y, f.width * cell, f.height * cell};
            SDL_RenderTexture(r_, c.tex, nullptr, &dst);
        }
        else
        {
        const auto px = render_image(unit, f);
        for (uint32_t py = 0; py < f.height; ++py)
            for (uint32_t pxi = 0; pxi < f.width; ++pxi)
            {
                const Rgb c = px[base + size_t(py) * f.width + pxi];
                SDL_SetRenderDrawColor(r_, c.r, c.g, c.b, 255);
                const SDL_FRect cellr{x + pxi * cell, y + py * cell, cell, cell};
                SDL_RenderFillRect(r_, &cellr);
            }
        }
        const Theme& th = theme();
        SDL_SetRenderDrawColor(r_, th.edge.r, th.edge.g, th.edge.b, 255);
        const SDL_FRect border{x - 1, y - 1, f.width * cell + 2, f.height * cell + 2};
        SDL_RenderRect(r_, &border);
    }

    std::string one_line_preview(const Space::Digits& u)
    {
        if (on_binary()) return ""; // its shelves stand empty
        if (on_models()) return ""; // a model is drawn, not written out: see draw_model
        if (line().kind == LineKind::Text) return "\"" + ascii(utf8_encode(line().space.text_of(u))) + "\"";
        if (line().kind == LineKind::Audio) return notes_to_notation(u);
        return "";
    }

    void draw_hud(int w, int h)
    {
        const Theme& th = theme();
        const SDL_Color ink = th.edge;
        const float W = float(w), H = float(h);

        // FPS counter: the average over the last half second, and its slowest frame.
        if (fps_counter_)
        {
            const Uint64 now = SDL_GetTicksNS();
            if (fps_last_)
            {
                ++fps_frames_;
                fps_worst_now_ = std::max(fps_worst_now_, double(now - fps_last_) / 1e6);
            }
            else fps_since_ = now;
            fps_last_ = now;
            if (now - fps_since_ >= 500000000ull && fps_frames_ > 0)
            {
                fps_ms_ = double(now - fps_since_) / 1e6 / fps_frames_;
                fps_shown_ = 1000.0 / fps_ms_;
                fps_worst_ = fps_worst_now_;
                fps_frames_ = 0;
                fps_worst_now_ = 0;
                fps_since_ = now;
            }
            char a[32], b[32], c[32];
            std::snprintf(a, sizeof a, "%.0f", fps_shown_);
            std::snprintf(b, sizeof b, "%.1f", fps_ms_);
            std::snprintf(c, sizeof c, "%.1f", fps_worst_);
            std::string line = trf("hud.fps", {a, b, c});
            if (fps_tris_) line += "   " + trf("hud.fps.triangles", {std::to_string(fps_tris_)});
            const float tw = text_width(line, 1);
            panel(W - tw - 22, 48, tw + 16, 18);
            text(W - tw - 14, 53, line, 1, ink);
        }

        // Crosshair.
        SDL_SetRenderDrawColor(r_, ink.r, ink.g, ink.b, 255);
        SDL_RenderLine(r_, W / 2 - 8, H / 2, W / 2 + 8, H / 2);
        SDL_RenderLine(r_, W / 2, H / 2 - 8, W / 2, H / 2 + 8);

        // Top bar: where you are.
        panel(-1, -1, W + 2, 44);
        const Book& first = book(0, 0);
        const std::string where =
            on_binary() ? trf("hud.line", {tr(th.key)}) + "   " + tr("hud.infinite") + "   " + tr("hud.tile") + " " + tile_label_
                        : trf("hud.line", {tr(th.key)}) + "   " + ordering_name() +
                              (guided_on() ? "  " + trf("hud.zoom", {std::to_string(zoom_)}) : std::string()) + "   " + tr("hud.tile") + " " +
                              tile_label_ +
                              (first.empty ? "   " + tr("hud.padding") : "   " + trf("hud.along", {percent(first.fraction)}));
        text(10, 7, fit(where, W - 20, 2), 2, ink);
        const std::string per_tile = "   " + trf("hud.per_tile", {std::to_string(sieve::books_per_tile())});
        const std::string loop = on_binary() ? tr("hud.no_loop")
                                             : trf("hud.loop", {loop_label_}) + (loop_.fills_whole_tiles()
                                                                                     ? std::string()
                                                                                     : " " + trf("hud.loop.padding", {std::to_string(loop_.padding())}));
        text(10, 28, fit((on_books() ? books_->id() : on_models() ? model_space_->id() : on_binary() ? tr("hud.binary_id") : line().space.id()) + (guided_on() ? "   " + trf("hud.model", {line().model_id}) : std::string()) + "   " +
                         loop + per_tile + "   " + filter_status() + "   " +
                         (on_binary() ? trf("hud.door_one", {tr(theme_of(binary_shelf_ == 0 ? (li_ + 1) % kLines : (li_ + kLines - 1) % kLines).key)})
                                      : trf("hud.doors", {tr(theme_of((li_ + 1) % kLines).key), tr(theme_of((li_ + kLines - 1) % kLines).key)})),
                         W - 20, 1),
             1, ink);

        // The book you are looking at.
        if (hover_ && !in_hand_)
        {
            const Book& bk = book(hover_->tile, hover_->slot());
            const float ph = on_books() || line().kind == LineKind::Image || line().kind == LineKind::Video ? 150 : 96;
            panel(10, H - ph - 44, std::min(W - 20, 900.0f), ph);
            const std::string label = trf(hover_->side == Side::Left ? "hud.slot.left" : "hud.slot.right",
                                          {std::to_string(hover_->row + 1), std::to_string(hover_->col + 1), std::to_string(hover_->slot())});
            float y = H - ph - 36;
            text(20, y, label, 2, ink);
            y += 22;
            if (bk.empty)
                text(20, y, tr("hud.empty_slot"), 1, ink);
            else
            {
                const Space::Digits& u = bk.unit;
                if (bk.guided)
                {
                    char b1[16], b2[16];
                    std::snprintf(b1, sizeof b1, "%.2f", double(bk.bits) / double(u.size()));
                    std::snprintf(b2, sizeof b2, "%.2f", std::log2(double(line().space.base())));
                    text(20, y, trf("hud.point", {short_address(bk.hex), std::to_string(bk.bits), b1, b2}), 1, ink);
                }
                else text(20, y, trf(bk.survivor ? "hud.compact_address" : "hud.address", {short_address(bk.hex)}), 1, ink);
                y += 12;
                std::string verdict;
                if (bk.survivor) verdict = trf("hud.survivor", {short_big(bk.survivor_number)}) + "   ";
                else if (has_filters()) verdict = (bk.passes ? tr("hud.passes") : trf("hud.fails", {bk.failed_by})) + "   ";
                text(20, y, verdict + trf("hud.along_loop", {percent(bk.fraction)}), 1, ink);
                y += 16;
                if (bk.parts)
                {
                    // A book: its cover and its title.
                    draw_pixels(bk.parts->cover, 20, y, 60, 0);
                    const std::string title = utf8_encode(line().space.text_of(bk.parts->title));
                    const auto rows = wrap_words(title, size_t(std::max(20.0f, (std::min(W - 20, 900.0f) - 110) / 16)));
                    for (size_t r = 0; r < rows.size() && r < 3; ++r) text(96, y + float(r) * 20, rows[r], 2, ink);
                }
                else if (bk.model) text(20, y, model_line_summary(*bk.model), 2, ink);
                else if (line().kind == LineKind::Image || line().kind == LineKind::Video) draw_pixels(u, 20, y, 60, 0);
                else text(20, y, wrap(one_line_preview(u), size_t(std::max(20.0f, (std::min(W - 20, 900.0f) - 40) / 16)))[0], 2, ink);
            }
        }

        if (in_hand_) draw_in_hand(W, H);

        // Messages, input box and help.
        if (!message_.empty() && SDL_GetTicks() < message_until_) text(12, H - 36, message_, 1, ink);
        if (input_ != Input::None)
        {
            const std::string prompt = input_ == Input::Warp
                                           ? tr(on_books() ? "prompt.warp.book"
                                                : line().kind == LineKind::Image || line().kind == LineKind::Video ? "prompt.warp.picture"
                                                : line().kind == LineKind::Audio                                   ? "prompt.warp.notes"
                                                                                                                   : "prompt.warp.text")
                                           : tr("prompt.goto");
            panel(10, H / 2 + 40, W - 20, 34);
            text(20, H / 2 + 49, prompt + text_ + ((SDL_GetTicks() / 400) % 2 ? "_" : " "), 2, ink);
        }
        text(12, H - 18,
             tr("hud.keys1") + (guided_on() ? tr("hud.keys.zoom") : std::string()) + tr("hud.keys2") +
                 tr(in_hand_ && in_hand_->parts ? "hud.keys.page" : "hud.keys.trail") + tr("hud.keys3"),
             1, ink);
        // Last, so nothing else in the readout is drawn over it.
        draw_compass(float(W), float(H));
    }

    // ---- the Binary Edge
    //
    // Down each outer side of the corridor runs a half-hallway (world.hpp): an empty bookcase
    // against the inner wall, a walkway, a short wall, and then the floor ends. Off that edge
    // falls rain: columns of characters drawn from every Unicode block Sieve knows
    // (sieve/alphabet.hpp), which is the whole of what could be written and has not been. It is
    // green because that is what this is. The shelves facing it are empty because nothing out
    // there has been catalogued yet, and cataloguing it is a job for people.
    //
    // The rain is drawn into a texture, then mapped onto the curtain hanging off each tile's
    // edge with the same perspective grid the crate faces use, so it lies in the world rather
    // than facing the camera. Four tiles' worth sit side by side in the texture and each tile
    // and band takes one of them, so nothing repeats as you walk.
    // (theme.hpp: kEdgeInk, the one colour the edge is drawn in, here and on the setup map.)

    static constexpr int kRainCell = 16;   // one character cell in the rain texture
    static constexpr int kRainCols = 28;   // characters across one tile of wall
    static constexpr int kRainRows = 22;   // and down it
    static constexpr int kRainPanels = 4;  // side by side, so neighbouring tiles differ

    // The characters the rain can fall as: code points from every block, kept only if the font
    // can actually draw them. With Sieve's own small font that is Latin and Greek; with Unifont
    // (tools/fetch_unifont.py) it is most of what Unicode has.
    const std::vector<char32_t>& rain_glyphs()
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

    // One cell of one column. A column is either characters or, now and then, the surrogate pair
    // of an astral code point written out: the code point, then its high and low halves, which
    // is the forward pass and, read back up the column, the backward one. Nothing here is
    // addressed or stored; the edge is a view of what has not been catalogued.
    std::string rain_cell(uint32_t col, uint32_t row, uint32_t seed) const
    {
        const uint32_t h = hash3(col, row, seed);
        return std::string(1, "0123456789ABCDEF"[h & 15]);
    }

    static uint32_t hash3(uint32_t a, uint32_t b, uint32_t c)
    {
        uint32_t h = a * 0x9E3779B1u ^ b * 0x85EBCA77u ^ c * 0xC2B2AE3Du;
        h ^= h >> 15;
        h *= 0x2545F491u;
        h ^= h >> 13;
        return h;
    }

    // The rain, into its own texture. Redrawn every frame: it is a few hundred glyphs.
    void render_rain()
    {
        const int w = kRainCols * kRainCell * kRainPanels, h = kRainRows * kRainCell;
        if (!rain_tex_)
        {
            rain_tex_ = SDL_CreateTexture(r_, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, w, h);
            if (!rain_tex_) return;
            SDL_SetTextureBlendMode(rain_tex_, SDL_BLENDMODE_BLEND);
            SDL_SetTextureScaleMode(rain_tex_, SDL_SCALEMODE_LINEAR);
        }
        // Rain is a curtain, not an animation anyone reads: redrawing it every third frame
        // looks the same and costs a third as much.
        if (rain_tex_ && rain_at_ != 0 && portal_frame_ - rain_at_ < 3) return;
        rain_at_ = portal_frame_;
        SDL_Texture* const was = SDL_GetRenderTarget(r_);
        if (!SDL_SetRenderTarget(r_, rain_tex_)) return;
        SDL_SetRenderDrawColor(r_, 0, 0, 0, 0);
        SDL_RenderClear(r_);
        const std::vector<char32_t>& pool = rain_glyphs();
        const SDL_Color ink = kEdgeInk;
        const uint32_t frame = portal_frame_;
        for (int p = 0; p < kRainPanels; ++p)
            for (int c = 0; c < kRainCols; ++c)
            {
                const uint32_t col = uint32_t(p * kRainCols + c);
                const uint32_t seed = hash3(col, 0, 0x5EED);
                // Each column falls at its own speed and has its own length and its own start.
                const uint32_t speed = 3 + (seed & 7);
                const int len = 7 + int((seed >> 4) & 15);
                // One in six columns writes a surrogate pair instead of characters.
                const bool pair = (seed >> 9) % 6 == 0;
                const int head = int(((frame * speed) / 16 + (seed >> 12)) % uint32_t(kRainRows + len));
                for (int k = 0; k < len; ++k)
                {
                    const int row = head - k;
                    if (row < 0 || row >= kRainRows) continue;
                    // The head is bright and the tail fades, as rain does.
                    const float t = 1.0f - float(k) / float(len);
                    const float b = 0.35f + 0.65f * t;
                    SDL_Color ch = k == 0 ? SDL_Color{255, 255, 255, 255}
                                          : SDL_Color{Uint8(float(ink.r) * b), Uint8(float(ink.g) * b),
                                                      Uint8(float(ink.b) * b), Uint8(90.0f + 165.0f * t)};
                    std::string cell;
                    if (pair)
                    {
                        // An astral code point and the two halves it is written as in UTF-16.
                        const char32_t cp = 0x10000 + (hash3(col, uint32_t(row) / 3, 0xA57A) % 0xFFFF);
                        const uint32_t v = uint32_t(cp) - 0x10000;
                        const uint32_t hi = 0xD800 + (v >> 10), lo = 0xDC00 + (v & 0x3FF);
                        static const char* const kHex = "0123456789ABCDEF";
                        const uint32_t which = uint32_t(row) % 3 == 0 ? uint32_t(cp) : uint32_t(row) % 3 == 1 ? hi : lo;
                        cell = std::string(1, kHex[(which >> (4 * (3 - uint32_t(row) % 4))) & 15]);
                    }
                    else
                    {
                        const char32_t cp = pool[hash3(col, uint32_t(row), frame / 24) % pool.size()];
                        cell = utf8_encode(std::u32string(1, cp));
                    }
                    draw_text(r_, float(col * kRainCell), float(row * kRainCell), cell, 2, ch);
                }
            }
        SDL_SetRenderTarget(r_, was);
    }

    // The rain, falling off the edge. It hangs in the opening itself -- the full height of the
    // space, from the ceiling down to the floor -- and what falls past the floor is behind the
    // floor, so there is nothing to draw down there. Each tile takes two panels of the texture
    // side by side, which keeps the glyphs about square and stops the corridor repeating.
    static constexpr int kRainSlices = 2;

    void draw_binary_edge(const bool* visible, int back, int ahead, bool real)
    {
        if (!rain_tex_) return;
        std::vector<SDL_Vertex> verts;
        const float sx = drop_sign(), x = sx * kEdgeRail;
        const float slice = kTile / kRainSlices;
        for (int t = -back; t <= ahead; ++t)
        {
            if (!visible[t + back]) continue;
            const float z0 = float(t) * kTile;
            for (int k = 0; k < kRainSlices; ++k)
            {
                const int panel = int(((t * kRainSlices + k) % kRainPanels + kRainPanels) % kRainPanels);
                const float u0 = float(panel) / kRainPanels, u1 = float(panel + 1) / kRainPanels;
                const float za = z0 + float(k) * slice, zb = za + slice;
                // Wound so the rain reads the same way round on both sides.
                const Vec3 quad[4] = {{x, kHeight, sx < 0 ? za : zb},
                                      {x, kHeight, sx < 0 ? zb : za},
                                      {x, 0, sx < 0 ? zb : za},
                                      {x, 0, sx < 0 ? za : zb}};
                draw_face_image(rain_tex_, quad, verts, u0, u1);
            }
        }
        // The short wall stands in front of the bottom of that curtain, and the rain is blitted
        // over the models rather than depth-tested against them, so its two faces are laid back
        // over the top, near tile last. Above the wall the opening is clear and the rain shows;
        // below it, the rain is behind the wall, as it should be.
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

    // ---- crate faces
    //
    // On the models line every slot holds the same crate, because a mesh cannot be read at a
    // hundred and twenty-eight to a tile. Instead, the crate you are looking at has its model
    // rendered to a small flat image and printed on its front, and then its neighbours do the
    // same, a few a frame, spreading outward along the shelf until the cache is full. The cache
    // holds whatever the budget in Settings > Graphics allows and drops the least recently seen.

    // How big one of those pictures is drawn, in pixels square: Pre-rendered Image Tile Cache
    // Size in the setup menu's MODELS section. Bigger is a sharper crate face and four times the
    // memory each time it doubles, so it trades against how many the cache holds.
    int crate_px_ = 64;
    size_t crate_bytes() const { return size_t(crate_px_) * size_t(crate_px_) * 4; }
    static constexpr int kCratesPerFrame = 3;             // rendered anew each frame, at most

    // The model of one crate, drawn small: faces back to front, shaded by depth, on nothing.
    void render_crate_face(const ModelSpace::Parts& p, std::vector<uint32_t>& px) const
    {
        const int n = crate_px_;
        px.assign(size_t(n) * size_t(n), 0u);
        const auto verts = model_space_->mesh_of(p);
        const auto faces = model_space_->faces_of(p);
        const Theme& th = theme_of(kModelsLine);
        const float ca = std::cos(model_spin_), sa = std::sin(model_spin_);
        const float ct = std::cos(model_tilt_), st = std::sin(model_tilt_);
        const float half = float(n) * 0.5f, r = float(n) * 0.30f;
        auto project = [&](const ModelSpace::Vertex& v) {
            const float x = v.x * ca + v.z * sa, z = -v.x * sa + v.z * ca;
            const float y = v.y * ct - z * st, depth = v.y * st + z * ct;
            const float k = 1.0f / (2.4f - depth * 0.45f);
            return std::array<float, 3>{half + x * r * k * 2.4f, half - y * r * k * 2.4f, depth};
        };
        std::vector<std::pair<float, size_t>> order;
        order.reserve(faces.size());
        for (size_t i = 0; i < faces.size(); ++i)
        {
            const auto& f = faces[i];
            if (f.a == f.b || f.b == f.c || f.a == f.c) continue; // a degenerate face has no face
            order.emplace_back((project(verts[f.a])[2] + project(verts[f.b])[2] + project(verts[f.c])[2]) / 3.0f, i);
        }
        std::sort(order.begin(), order.end());
        for (const auto& [depth, i] : order)
        {
            const auto& f = faces[i];
            const auto a = project(verts[f.a]), b = project(verts[f.b]), c = project(verts[f.c]);
            const float t = std::clamp(0.30f + depth * 0.55f, 0.10f, 1.0f);
            const uint32_t argb = 0xFF000000u | (uint32_t(float(th.edge.r) * t) << 16) |
                                  (uint32_t(float(th.edge.g) * t) << 8) | uint32_t(float(th.edge.b) * t);
            fill_triangle(px, n, a, b, c, argb);
        }
    }

    // A flat triangle into the crate's little image. Small and rare enough not to need more.
    static void fill_triangle(std::vector<uint32_t>& px, int n, const std::array<float, 3>& a, const std::array<float, 3>& b,
                              const std::array<float, 3>& c, uint32_t argb)
    {
        const float minx = std::min({a[0], b[0], c[0]}), maxx = std::max({a[0], b[0], c[0]});
        const float miny = std::min({a[1], b[1], c[1]}), maxy = std::max({a[1], b[1], c[1]});
        const int x0 = std::max(0, int(minx)), x1 = std::min(n - 1, int(maxx) + 1);
        const int y0 = std::max(0, int(miny)), y1 = std::min(n - 1, int(maxy) + 1);
        const float area = (b[0] - a[0]) * (c[1] - a[1]) - (c[0] - a[0]) * (b[1] - a[1]);
        if (std::fabs(area) < 1e-6f) return;
        const float inv = 1.0f / area;
        for (int y = y0; y <= y1; ++y)
            for (int x = x0; x <= x1; ++x)
            {
                const float fx = float(x) + 0.5f, fy = float(y) + 0.5f;
                const float w0 = ((b[0] - a[0]) * (fy - a[1]) - (fx - a[0]) * (b[1] - a[1])) * inv;
                const float w1 = ((fx - a[0]) * (c[1] - a[1]) - (c[0] - a[0]) * (fy - a[1])) * inv;
                if (w0 < 0 || w1 < 0 || w0 + w1 > 1) continue;
                px[size_t(y) * size_t(n) + size_t(x)] = argb;
            }
    }

    void clear_crate_faces()
    {
        for (auto& [key, cf] : crate_)
            if (cf.tex) SDL_DestroyTexture(cf.tex);
        crate_.clear();
    }

    // How many images the budget allows: the slider is megabytes, which is what costs.
    size_t crate_capacity() const { return std::max<size_t>(8, size_t(crate_budget_mb_) * 1024 * 1024 / crate_bytes()); }

    // The image for one crate, if it is already rendered. `allow` lets this frame render it.
    SDL_Texture* crate_face(int64_t dt, uint32_t slot, bool allow, std::vector<uint32_t>& scratch)
    {
        const int64_t key = dt * int64_t(sieve::books_per_tile()) + slot;
        if (auto it = crate_.find(key); it != crate_.end())
        {
            it->second.used = portal_frame_;
            return it->second.tex;
        }
        if (!allow) return nullptr;
        const Book& b = book(dt, slot);
        if (b.empty || !b.model) return nullptr;
        render_crate_face(*b.model, scratch);
        SDL_Texture* tex = SDL_CreateTexture(r_, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, crate_px_, crate_px_);
        if (!tex) return nullptr;
        SDL_UpdateTexture(tex, nullptr, scratch.data(), crate_px_ * 4);
        SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
        SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_LINEAR);
        // Room for it: the crate seen longest ago goes first.
        while (crate_.size() >= crate_capacity())
        {
            auto oldest = crate_.begin();
            for (auto it = crate_.begin(); it != crate_.end(); ++it)
                if (it->second.used < oldest->second.used) oldest = it;
            if (oldest->second.tex) SDL_DestroyTexture(oldest->second.tex);
            crate_.erase(oldest);
        }
        crate_.emplace(key, CrateFace{tex, portal_frame_});
        return tex;
    }

    // Prints the rendered images on the crates in view, and renders a few more, spreading out
    // from the crate under the crosshair (or from the nearest one ahead when you look at nothing).
    void draw_crate_faces(const bool* visible, int back, int ahead)
    {
        int64_t seed_tile = 0;
        uint32_t seed_slot = sieve::books_per_tile() / 2;
        if (hover_)
        {
            seed_tile = hover_->tile;
            seed_slot = hover_->slot();
        }
        // The crates in view, nearest the seed first, so the field fills outward as you look.
        struct Want { int64_t order; int64_t dt; uint32_t slot; };
        std::vector<Want> want;
        for (int t = -back; t <= ahead; ++t)
        {
            if (!visible[t + back]) continue;
            const uint32_t books = books_in_tile(t);
            for (uint32_t k = 0; k < books; ++k)
            {
                const int64_t dtile = t - seed_tile, dslot = int64_t(k) - int64_t(seed_slot);
                want.push_back({std::llabs(dtile) * 256 + std::llabs(dslot), t, k});
            }
        }
        std::sort(want.begin(), want.end(), [](const Want& a, const Want& b) { return a.order < b.order; });
        int budget = kCratesPerFrame;
        std::vector<SDL_Vertex> verts;
        for (const Want& w : want)
        {
            SDL_Texture* tex = crate_face(w.dt, w.slot, budget > 0, crate_scratch_);
            if (!tex)
            {
                if (budget > 0) --budget; // a slot with nothing to draw still costs its turn
                continue;
            }
            const BookSlot bs = BookSlot::of(w.dt, w.slot);
            Vec3 f[4];
            book_face(float(w.dt) * kTile, bs.side, bs.row, bs.col, f, sizes_vary());
            draw_face_image(tex, f, verts);
        }
    }

    // One crate's image on its front face. The quad is split into a grid and every grid point is
    // projected, so the picture keeps its perspective instead of skewing across two triangles.
    void draw_face_image(SDL_Texture* tex, const Vec3 quad[4], std::vector<SDL_Vertex>& verts, float uu0 = 0,
                         float uu1 = 1)
    {
        constexpr int kGrid = 2; // cells per side
        Vec3 corner[4];
        for (int i = 0; i < 4; ++i)
        {
            corner[i] = cam_.to_camera(quad[i]);
            if (corner[i].z < cam_.near_z + 0.01f) return; // partly behind you: leave it be
        }
        Point2 p[kGrid + 1][kGrid + 1];
        for (int i = 0; i <= kGrid; ++i)
            for (int j = 0; j <= kGrid; ++j)
            {
                const float u = float(j) / kGrid, v = float(i) / kGrid;
                // Bilinear in camera space, then projected: exact perspective at every grid point.
                const Vec3 top = corner[0] + (corner[1] - corner[0]) * u;
                const Vec3 bot = corner[3] + (corner[2] - corner[3]) * u;
                p[i][j] = cam_.project_camera(top + (bot - top) * v);
            }
        const SDL_FColor white{1, 1, 1, 1};
        verts.clear();
        for (int i = 0; i < kGrid; ++i)
            for (int j = 0; j < kGrid; ++j)
            {
                const float u0 = uu0 + (uu1 - uu0) * float(j) / kGrid;
                const float u1 = uu0 + (uu1 - uu0) * float(j + 1) / kGrid;
                const float v0 = float(i) / kGrid, v1 = float(i + 1) / kGrid;
                const SDL_Vertex a{{p[i][j].x, p[i][j].y}, white, {u0, v0}};
                const SDL_Vertex b{{p[i][j + 1].x, p[i][j + 1].y}, white, {u1, v0}};
                const SDL_Vertex c{{p[i + 1][j + 1].x, p[i + 1][j + 1].y}, white, {u1, v1}};
                const SDL_Vertex d{{p[i + 1][j].x, p[i + 1][j].y}, white, {u0, v1}};
                for (const SDL_Vertex& v : {a, b, c, a, c, d}) verts.push_back(v);
            }
        SDL_RenderGeometry(r_, tex, verts.data(), int(verts.size()), nullptr, 0);
    }

    // A model in hand: its wireframe, turned by the mouse or by A and D, and its .obj text beside
    // it. The mesh is small enough (a few dozen triangles) to draw as lines with the painter's
    // algorithm; the shelf copy is a crate, which is what the render cache fills in.
    float draw_model(const ModelSpace::Parts& p, float x, float y, float pw, float bottom)
    {
        const Theme& th = theme();
        const auto verts = model_space_->mesh_of(p);
        const auto faces = model_space_->faces_of(p);
        const float box = std::min(pw * 0.5f, bottom - y - 20);
        if (box < 40) return y;
        const float cx = x + 14 + box * 0.5f, cy = y + box * 0.5f, r = box * 0.34f;
        // Turn about the upright axis, and tip a little so the shape reads as solid.
        const float ca = std::cos(model_spin_), sa = std::sin(model_spin_);
        const float ct = std::cos(model_tilt_), st = std::sin(model_tilt_);
        auto project = [&](const ModelSpace::Vertex& v) {
            const float px = v.x * ca + v.z * sa;
            const float pz = -v.x * sa + v.z * ca;
            const float py = v.y * ct - pz * st;
            const float depth = v.y * st + pz * ct;
            // A gentle perspective, so turning it reads as turning.
            const float k = 1.0f / (2.4f - depth * 0.45f);
            return std::array<float, 3>{cx + px * r * k * 2.4f, cy - py * r * k * 2.4f, depth};
        };
        // Faces back to front, drawn as filled triangles under their own edges.
        std::vector<std::pair<float, size_t>> order;
        order.reserve(faces.size());
        for (size_t i = 0; i < faces.size(); ++i)
        {
            const auto& f = faces[i];
            order.emplace_back((project(verts[f.a])[2] + project(verts[f.b])[2] + project(verts[f.c])[2]) / 3.0f, i);
        }
        std::sort(order.begin(), order.end());
        std::vector<SDL_Vertex> fill;
        for (const auto& [depth, i] : order)
        {
            const auto& f = faces[i];
            if (f.a == f.b || f.b == f.c || f.a == f.c) continue; // a degenerate face has no face
            const auto a3 = project(verts[f.a]), b3 = project(verts[f.b]), c3 = project(verts[f.c]);
            // Nearer faces a little brighter, so the shape has depth without a light.
            const float t = std::clamp(0.35f + depth * 0.5f, 0.12f, 0.85f);
            const SDL_FColor fc{th.edge.r / 255.0f * t, th.edge.g / 255.0f * t, th.edge.b / 255.0f * t, 0.55f};
            fill.clear();
            for (const auto& v3 : {a3, b3, c3}) fill.push_back({{v3[0], v3[1]}, fc, {0, 0}});
            SDL_RenderGeometry(r_, nullptr, fill.data(), 3, nullptr, 0);
            SDL_SetRenderDrawColor(r_, th.edge.r, th.edge.g, th.edge.b, 200);
            SDL_RenderLine(r_, a3[0], a3[1], b3[0], b3[1]);
            SDL_RenderLine(r_, b3[0], b3[1], c3[0], c3[1]);
            SDL_RenderLine(r_, c3[0], c3[1], a3[0], a3[1]);
        }
        // The .obj text of the very same model, beside it: the two lines this object lives on.
        const float tx = x + 14 + box + 20;
        const size_t cols = size_t(std::max(12.0f, (pw - 48 - box) / 8));
        float ty = y;
        uint32_t shown = 0;
        for (const std::string& l : split_lines(model_space_->to_obj(p)))
        {
            if (ty > bottom - 14) { text(tx, ty, "...", 1, th.edge); break; }
            text(tx, ty, fit(l, float(cols) * 8, 1), 1, ++shown <= model_space_->vertices() ? th.edge : mix(th.edge, th.bg, 0.35f));
            ty += 11;
        }
        return std::max(y + box, ty) + 6;
    }

    // One line about a model, for the shelf row and the readout.
    std::string model_line_summary(const ModelSpace::Parts& p) const
    {
        const auto fs = model_space_->faces_of(p);
        std::vector<bool> seen(model_space_->vertices(), false);
        uint32_t used = 0, degenerate = 0;
        for (const auto& f : fs)
        {
            seen[f.a] = seen[f.b] = seen[f.c] = true;
            if (f.a == f.b || f.b == f.c || f.a == f.c) ++degenerate;
        }
        for (bool b : seen)
            if (b) ++used;
        return trf("model.summary", {std::to_string(used), std::to_string(model_space_->vertices()),
                                     std::to_string(model_space_->face_count() - degenerate),
                                     std::to_string(model_space_->face_count())});
    }

    static std::vector<std::string> split_lines(const std::string& s)
    {
        std::vector<std::string> out;
        size_t start = 0;
        for (size_t i = 0; i <= s.size(); ++i)
            if (i == s.size() || s[i] == '\n')
            {
                if (i > start) out.push_back(s.substr(start, i - start));
                start = i + 1;
            }
        return out;
    }

    void draw_in_hand(float W, float H)
    {
        const Theme& th = theme();
        const SDL_Color ink = th.edge;
        const Book& bk = *in_hand_;
        const Space::Digits& u = bk.unit;
        const float pw = std::min(W - 40, 1000.0f), ph = std::min(H - 120, 640.0f);
        const float x = (W - pw) / 2, y = 50;
        panel(x, y, pw, ph, 255);
        float cy = y + 12;
        text(x + 14, cy, fit(tr("hand.title") + "   " + in_hand_where_ + "   " + trf("hud.along", {percent(bk.fraction)}), pw - 28, 2), 2, ink);
        cy += 28;
        // Two tabs: the thing itself, and what it costs to name it. C moves between them.
        {
            float tx = x + 14;
            for (int t = 0; t < 2; ++t)
            {
                const std::string label = tr(t == 0 ? "hand.tab.item" : "hand.tab.cost");
                const float tw = text_width(label, 1) + 16;
                if (t == hand_tab_)
                {
                    SDL_SetRenderDrawColor(r_, ink.r, ink.g, ink.b, 60);
                    const SDL_FRect box{tx, cy - 3, tw, 16};
                    SDL_RenderFillRect(r_, &box);
                }
                text(tx + 8, cy, label, 1, ink);
                tx += tw + 6;
            }
            cy += 22;
        }
        if (hand_tab_ == 1) { draw_cost(bk, x, cy, pw, y + ph); return; }
        const size_t cols2 = size_t((pw - 28) / 16), cols1 = size_t((pw - 28) / 8);
        if (bk.model) cy = draw_model(*bk.model, x, cy, pw, y + ph - 110);
        else if (bk.parts) cy = draw_book(*bk.parts, x, cy, pw, y + ph - 110);
        else switch (line().kind)
        {
        case LineKind::Text:
            for (const auto& l : wrap(utf8_encode(line().space.text_of(u)), cols2))
            {
                text(x + 14, cy, l, 2, ink);
                cy += 20;
                if (cy > y + ph - 120) break;
            }
            break;
        case LineKind::Audio:
            for (const auto& l : wrap(notes_to_notation(u), cols2))
            {
                if (cy > y + ph - 150) { text(x + 14, cy, "...", 2, ink); cy += 20; break; }
                text(x + 14, cy, l, 2, ink);
                cy += 20;
            }
            text(x + 14, cy + 6, tr("hand.play"), 1, ink);
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
                text(x + 24 + size, cy, trf("hand.frame", {std::to_string(frame + 1), std::to_string(frames)}), 1, ink);
            cy += size + 10;
            break;
        }
        }
        cy = std::max(cy, y + ph - 110);
        const std::string kind = bk.survivor ? "hand.compact_address" : "hand.address";
        text(x + 14, cy,
             bk.guided ? trf(kind + ".guided", {std::to_string(bk.bits)}) : trf(kind, {tr(std::string("ordering.") + to_string(mode_))}), 1, ink);
        cy += 12;
        int shown = 0;
        for (const auto& l : wrap(bk.guided ? bk.own_hex : bk.hex, cols1))
        {
            if (++shown > 6) { text(x + 14, cy, "...", 1, ink); break; }
            text(x + 14, cy, l, 1, ink);
            cy += 10;
        }
        text(x + 14, y + ph - 16, tr(bk.parts ? "hand.keys.book" : bk.model ? "hand.keys.model" : "hand.keys"), 1, ink);
    }

    // COST: what it costs to name the thing in your hand.
    //
    // There is one number underneath all of this -- the unit's index, somewhere in [0, N) -- and
    // every row is that same number written a different way. The point of showing them together
    // is that the first three are the same length: an address in a bijection is the content, not
    // a handle on it. Only the guided ordering is shorter, and only when the content is likely
    // under the model, which is why its percentage doubles as a measure of how text-like the
    // thing in your hand is.
    void draw_cost(const Book& bk, float x, float cy, float pw, float bottom)
    {
        const Theme& th = theme();
        const SDL_Color ink = th.edge, dim = mix(th.edge, th.bg, 0.45f);
        const BigUint& n = on_books() ? books_->size() : on_models() ? model_space_->size() : line().space.size();
        const double bits = n.log10_approx() * 3.321928094887362;
        auto row = [&](const std::string& what, double b, const std::string& written, const std::string& note) {
            text(x + 14, cy, what, 1, ink);
            text(x + 260, cy, trf("cost.bits", {fixed(b, 0)}), 1, ink);
            text(x + 380, cy, written, 1, ink);
            if (bits > 0) text(x + 500, cy, trf("cost.percent", {fixed(b / bits * 100.0, 1)}), 1, b < bits * 0.995 ? ink : dim);
            if (!note.empty()) text(x + 580, cy, note, 1, dim);
            cy += 14;
        };
        auto digits_in = [](double b, double per) { return std::to_string(int(std::ceil(b / per))); };

        text(x + 14, cy, tr("cost.head"), 1, dim);
        cy += 18;
        row(tr("cost.unit"), bits, trf("cost.chars", {digits_in(bits, 4)}), tr("cost.unit.note"));
        cy += 6;
        row(tr("ordering.positional"), bits, trf("cost.chars", {digits_in(bits, 4)}), tr("cost.same"));
        row(tr("ordering.scrambled"), bits, trf("cost.chars", {digits_in(bits, 4)}), tr("cost.shuffled"));
        // The guided length for this unit, whatever ordering you are walking in. It exists only
        // on a text line with a model behind it.
        if (!on_books() && !on_models() && !on_binary() && line().guided)
        {
            try
            {
                const auto code = line().guided->code(bk.unit);
                row(tr("ordering.guided"), double(code.bits), trf("cost.chars", {std::to_string(code.hex.size())}),
                    double(code.bits) < bits * 0.9 ? tr("cost.likely") : tr("cost.unlikely"));
            }
            catch (const std::exception&)
            {
            }
        }
        cy += 10;
        // The other half of a written-down key: the shape that gives the address its meaning.
        const std::string spec = (on_books() ? books_->id() : on_models() ? model_space_->id() : line().space.id());
        text(x + 14, cy, tr("cost.spec"), 1, dim);
        cy += 14;
        for (const auto& l : wrap(spec, size_t((pw - 28) / 8)))
        {
            text(x + 14, cy, l, 1, ink);
            cy += 12;
        }
        text(x + 14, cy, trf("cost.spec.len", {std::to_string(spec.size())}), 1, dim);
        cy += 20;
        // How long the address is in each of the ways it could be written. Sieve writes hex; the
        // others are here because the choice is open and this is the measurement that settles it.
        text(x + 14, cy, tr("cost.written"), 1, dim);
        cy += 14;
        for (const auto& [name, per] : {std::pair<const char*, double>{"hex", 4.0}, {"base32", 5.0}, {"base64", 6.0},
                                        {"base85", 6.409390936137702}})
        {
            text(x + 14, cy, name, 1, ink);
            text(x + 260, cy, trf("cost.chars", {digits_in(bits, per)}), 1, ink);
            cy += 12;
        }
        text(x + 14, bottom - 16, tr("hand.keys.cost"), 1, ink);
    }

    // A book open in hand: the cover beside the title, then the current page. Returns the height used.
    float draw_book(const BookSpace::Parts& p, float x, float cy, float pw, float bottom)
    {
        const SDL_Color ink = theme().edge;
        const float cover = 170;
        draw_pixels(p.cover, lines_[1].image, x + 14, cy, cover, 0);
        const float tx = x + 14 + cover + 20;
        const size_t tcols = size_t(std::max(10.0f, (x + pw - 14 - tx) / 16));
        text(tx, cy, tr("hand.title_label"), 1, ink);
        float ty = cy + 14;
        const auto title = wrap_words(utf8_encode(line().space.text_of(p.title)), tcols);
        for (size_t r = 0; r < title.size() && r < 6; ++r, ty += 20) text(tx, ty, title[r], 2, ink);
        if (title.size() > 6) text(tx, ty, "...", 2, ink);
        cy += cover + 14;
        const uint32_t n = books_->pages();
        if (n == 0)
        {
            text(x + 14, cy, tr("hand.no_pages"), 1, ink);
            return cy + 14;
        }
        text(x + 14, cy, trf("hand.page", {std::to_string(book_page_ + 1), std::to_string(n)}), 1, ink);
        cy += 14;
        // The page at the largest scale that fits.
        const std::string page = utf8_encode(line().space.text_of(p.pages[size_t(book_page_)]));
        for (float scale : {2.0f, 1.0f})
        {
            const size_t cols = size_t((pw - 28) / (8 * scale));
            const auto rows = wrap_words(page, cols);
            const float lh = 10 * scale;
            if (scale > 1 && cy + rows.size() * lh > bottom) continue;
            for (const auto& r : rows)
            {
                if (cy + lh > bottom) { text(x + 14, cy, "...", scale, ink); cy += lh; break; }
                text(x + 14, cy, r, scale, ink);
                cy += lh;
            }
            break;
        }
        return cy;
    }

    bool menu_requested() const { return menu_requested_; }
    ~Hallway() { release_textures(); }
    // Frees textures that belong to the renderer; call before destroying it.
    void release_textures()
    {
        models_batch_.release();
        clear_crate_faces();
        if (rain_tex_)
        {
            SDL_DestroyTexture(rain_tex_);
            rain_tex_ = nullptr;
            rain_at_ = 0;
        }
        if (portal_.tex)
        {
            SDL_DestroyTexture(portal_.tex);
            portal_.tex = nullptr;
            portal_.w = portal_.h = 0;
        }
        for (PictureCache& c : picture_)
            if (c.tex)
            {
                SDL_DestroyTexture(c.tex);
                c.tex = nullptr;
            }
    }
    // From the main menu's settings: mouse look, and the graphics options (Geometry Edge Glow and
    // Real Graphics are recorded here for the renderer; both are off by default).
    void set_controls(int sensitivity_percent, bool invert_y)
    {
        look_ = 0.0025f * float(sensitivity_percent) / 100.0f;
        invert_y_ = invert_y;
    }
    void set_fps_counter(bool on) { fps_counter_ = on; }
    void set_angle_decimals(int d) { angle_decimals_ = std::clamp(d, 0, 8); }
    // How big a crate's picture is drawn (the setup menu's MODELS section). Changing it throws
    // the cache away, because every picture in it is the wrong size now.
    void set_crate_px(uint32_t px)
    {
        const int n = int(std::clamp<uint32_t>(px, 16, 512));
        if (n == crate_px_) return;
        crate_px_ = n;
        clear_crate_faces();
    }
    void set_model_cache(int megabytes)
    {
        crate_budget_mb_ = uint32_t(std::clamp(megabytes, 8, 512));
        while (crate_.size() > crate_capacity())
        {
            auto oldest = crate_.begin();
            for (auto it = crate_.begin(); it != crate_.end(); ++it)
                if (it->second.used < oldest->second.used) oldest = it;
            if (oldest->second.tex) SDL_DestroyTexture(oldest->second.tex);
            crate_.erase(oldest);
        }
    }
    void set_graphics(bool edge_glow, bool real_graphics, bool door_portals)
    {
        edge_glow_ = edge_glow;
        real_graphics_ = real_graphics;
        door_portals_ = door_portals;
        for (Models& m : models_) m = Models{}; // reloaded on first use (picks up edited files)
    }
    void put_back() { in_hand_.reset(); }
    // The item page: 0 the thing itself, 1 what it costs to name it. Always back to the thing
    // when something new is picked up -- the page is about what is in your hands now.
    int hand_tab_ = 0;

private:
    // The loop tile of the tile dt away from yours. Only needs a full modulo for small loops;
    // for big ones a step of a few tiles wraps at most once.
    BigUint offset_loop_tile(int64_t dt) const { return offset_loop_tile(loop_, loop_tile_, dt); }

    // True if tile dt away is the start of a loop on all four lines at once.
    bool all_start(int64_t dt) const
    {
        // Binary is left out: it has no loop to start, so it would mark every tile.
        for (int i = 0; i < kBinaryLine; ++i)
            if (!offset_loop_tile(all_loops_[i], all_loop_tiles_[i], dt).is_zero()) return false;
        return true;
    }

    static BigUint offset_loop_tile(const LineLoop& loop, const BigUint& loop_tile, int64_t dt)
    {
        const BigUint& tiles = loop.tiles();
        if (tiles.bit_length() <= 62)
        {
            auto u64 = [](const BigUint& v) {
                BigUint hi = v;
                hi >>= 32;
                return (uint64_t(hi.low_bits(32)) << 32) | v.low_bits(32);
            };
            const int64_t n = int64_t(u64(tiles));
            const int64_t cur = int64_t(u64(loop_tile));
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
        if (on_binary()) return 0; // the shelves stand empty: nothing out there is catalogued yet
        if (loop_.fills_whole_tiles()) return sieve::books_per_tile();
        BigUint lt = offset_loop_tile(dt);
        lt.add_small(1);
        return lt == loop_.tiles() ? sieve::books_per_tile() - loop_.padding() : sieve::books_per_tile();
    }

    // The books of one tile, slot by slot (4 edges each), drawn separately so padding can be bare.
    // `varied`: heights vary from slot to slot; else every book is the same size (audio, video).
    static std::vector<std::array<Segment, 4>> build_books(bool varied)
    {
        std::vector<std::array<Segment, 4>> out(sieve::books_per_tile());
        for (uint32_t k = 0; k < uint32_t(sieve::books_per_tile()); ++k)
        {
            const BookSlot b = BookSlot::of(0, k);
            Vec3 f[4];
            book_face(0, b.side, b.row, b.col, f, varied);
            out[k] = {Segment{f[0], f[1]}, Segment{f[1], f[2]}, Segment{f[2], f[3]}, Segment{f[3], f[0]}};
        }
        return out;
    }

    SDL_Window* window_;
    SDL_Renderer* r_;
    std::vector<Line> lines_;
    std::vector<Segment> tile_geometry_, hall_geometry_, case_geometry_; // all, hallway only, bookcases only
    // The same three for the binary line, which has one wall, and its open side: [0] shelves on
    // the left and the drop on the right, [1] the mirror of that.
    std::vector<Segment> bin_tile_[2], bin_hall_[2], bin_case_[2], edge_geometry_[2];
    int binary_shelf_ = 0;
    Models models_[kLines]; // Real Graphics, per line
    MeshBatch models_batch_;
    // The last picture drawn in each of the two places one can appear (on the shelf you are
    // looking at, and in your hand), so an unchanged picture is not rebuilt every frame.
    struct PictureCache
    {
        SDL_Texture* tex = nullptr;
        Space::Digits unit;
        int frame = -1;
        uint32_t w = 0, h = 0;
    };
    PictureCache picture_[2];
    std::vector<std::array<Segment, 4>> book_geometry_[2]; // [0] uniform, [1] varied heights
    Vec3 tile_lo_, tile_hi_;
    Camera cam_;
    int li_ = 0;
    AddressMode mode_ = AddressMode::Positional;
    bool guided_ = false; // guided ordering selected (applies on lines with a model)
    uint32_t zoom_ = 20;  // guided: books are 2^-zoom_ of the line apart, so the loop has 2^zoom_ books
    TileIndex tile_;      // the corridor tile you are in, shared by every line
    std::string tile_label_, loop_label_; // the readout's short forms of tile_ and the loop length
    LineLoop loop_{BigUint(1)};
    BigUint loop_tile_;   // tile_ mod loop_.tiles()
    LineLoop all_loops_[kLines] = {LineLoop(BigUint(1)), LineLoop(BigUint(1)), LineLoop(BigUint(1)), LineLoop(BigUint(1)),
                                   LineLoop(BigUint(1)), LineLoop(BigUint(1)), LineLoop(BigUint(1))};
    BigUint all_loop_tiles_[kLines];
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
    float look_ = 0.0025f; // radians per pixel of mouse movement
    float wheel_ = 0;      // mouse wheel movement not yet turned into whole tiles
    std::string filter_status_;
    std::vector<std::vector<SDL_FPoint>> edge_buckets_;
    bool invert_y_ = false;
    bool edge_glow_ = false, real_graphics_ = false;
    // Door Portals: one streaming texture, as big as the largest door drawn so far, and the
    // buffer the noise is rasterised into. The frame number drives the noise.
    struct PortalCache { SDL_Texture* tex = nullptr; int w = 0, h = 0; std::vector<uint32_t> px; };
    PortalCache portal_;
    bool door_portals_ = false;
    // A model in hand turns about the upright axis; the mouse or A and D drive it.
    float model_spin_ = 0.6f, model_tilt_ = 0.35f;
    // The crates whose models have been rendered to an image, and when each was last in view.
    struct CrateFace
    {
        SDL_Texture* tex = nullptr;
        uint32_t used = 0;
    };
    // The Binary Edge: the rain's texture, and the characters the current font can draw.
    SDL_Texture* rain_tex_ = nullptr;
    std::vector<char32_t> rain_pool_;
    std::string rain_font_;
    uint32_t rain_at_ = 0; // the frame the rain was last drawn

    std::unordered_map<int64_t, CrateFace> crate_;
    std::vector<uint32_t> crate_scratch_;
    uint32_t crate_budget_mb_ = 64;
    uint32_t portal_frame_ = 0;
    // FPS counter: frames and time since the shown figures were last updated (twice a second).
    bool fps_counter_ = false;
    int angle_decimals_ = 1; // Settings > Graphics: decimal places on the compass's degree readout
    Uint64 fps_since_ = 0, fps_last_ = 0;
    int fps_frames_ = 0;
    double fps_shown_ = 0, fps_ms_ = 0, fps_worst_ = 0, fps_worst_now_ = 0;
    size_t fps_tris_ = 0;
    FilterStack stacks_[kLines]; // the books line's are in book_stacks_
    BookStacks book_stacks_;
    std::unique_ptr<BookSieve> book_sieve_; // null if the books' filters failed to build
    std::unique_ptr<CompactLine> compact_[kLines]; // survivors in every ordering, where the stack can rank
    FilterMode modes_[kLines] = {FilterMode::Off, FilterMode::Off, FilterMode::Off,
                                 FilterMode::Off, FilterMode::Off, FilterMode::Off};
    std::unique_ptr<BookSpace> books_; // the books line
    std::unique_ptr<ModelSpace> model_space_; // the models line (Models above is Real Graphics)
    int book_page_ = 0;                // the page open in a book in hand
    Synth synth_;
};

// ---------------------------------------------------------------- options

const char* kUsage =
    "hallway - walk the Sieve's five lines in 3D\n\n"
    "Usage: hallway [options]\n\n"
    "Lines (the same meaning as in the sieve tool):\n"
    "  --line pages|image|audio|video|books|models|binary  line to start in (default pages)\n"
    "  --length L          text: characters per book (default 32)\n"
    "  --alphabet ID       text: lower27 (default), babel29, ascii95\n"
    "  --canon v2|v1       text: warp rules (default v2)\n"
    "  --image-width W  --image-height H  --image-palette ID    image line (10, 10, mono)\n"
    "  --notes N           audio: notes per book (default 16)\n"
    "  --video-width W  --video-height H  --video-frames F  --video-palette ID   video (5, 5, 8, mono)\n"
    "  --book-pages N      books: pages per book (default 4); a book is a cover (an image of the\n"
    "                      image line), a title and N pages (pages of the pages line)\n"
    "  --key K             scramble key (default sieve)\n"
    "  --mode positional|scrambled|guided   starting ordering (default positional)\n"
    "  --model ID|PATH|none  text: model for the guided ordering (default: the alphabet's default)\n"
    "  --zoom D            guided: books 2^-D of the line apart (after --warp/--goto set it)\n\n"
    "Start:\n"
    "  --warp INPUT        warp on start (text, notes, a picture file for image/video, a .book file for books)\n"
    "  --goto ADDR|P%|@T   go to an address, a percentage, or corridor tile T on start\n\n"
    "Menu:\n"
    "  The main menu opens first: Start Sieve, Settings (graphics, controls, language; saved to\n"
    "  sieve-hallway.ini) and Exit Sieve. Start Sieve opens the setup menu (Esc: back to the\n"
    "  main menu): adjust every line's state space and see the five lines as a map.\n"
    "  The magnifying glass beside a line's title (or F) opens its filters: tick filters, set\n"
    "  their parameters, and choose the mode: off, mark (failures faint), hide (failures left\n"
    "  out) or compact (only survivors, packed together, in every ordering). Saved to the\n"
    "  --filters file. In compact mode, G takes a compact address, as the books show it.\n"
    "  --no-menu           go straight into the hallway (F1 opens the setup menu from the hallway)\n"
    "  --settings PATH     application settings (default: sieve-hallway.ini next to the executable)\n"
    "  --language CODE     menu language for this run (a file in the lang folder, e.g. en)\n"
    "  --menu              with --screenshot: a picture of the setup menu (--press keys go to it)\n"
    "  --main-menu         with --screenshot: a picture of the main menu (--press keys go to it)\n"
    "  --filters PATH      filter settings (default: sieve-filters.ini next to the executable)\n\n"
    "Screenshots (for documentation and testing):\n"
    "  --screenshot PATH   render one frame to a PNG and exit\n"
    "  --size WxH          window size (default 1280x720)\n"
    "  --pose X,Z,YAW,PITCH  camera position and angles in degrees\n"
    "  --tile N            then move N tiles along the corridor\n"
    "  --take              take the book you are looking at off the shelf\n"
    "  --walk DX,DZ;...    walk these distances in metres first (doors work as when walking)\n"
    "  --press K,K,...     then press these keys (e.g. M,M,-,Shift+=), printing where you are\n"
    "  --edge-glow         draw with Geometry Edge Glow (or --settings a file that has it on)\n"
    "  --real-graphics     draw with Real Graphics: the models in the meshes folder\n"
    "  --door-portals      fill the doorways with procedural data noise (Real Graphics turns this on)\n"
    "  --model-cache MB    memory for the models line's rendered crate faces (8-512, default 64)\n"
    "  --model-tile PX     how big each of those pictures is drawn, square (16-512, default 64)\n"
    "  --items-per-wall N  units on one tile of the corridor (128 or 256; changes no address,\n"
    "                      only the tile and slot that name a unit's place in the corridor)\n"
    "  --fps-counter       show the FPS counter\n"
    "  --bench N           before the screenshot, time N frames and print the frame rate\n\n"
    "Controls: WASD move, mouse look, Shift run, E or click take a book, T warp, G go to,\n"
    "M switch ordering (positional, scrambled, guided), - and = zoom out/in (guided; Shift: 8x),\n"
    "wheel/PgUp/PgDn/[ ] jump 1/1000/1000000 tiles, Home to corridor tile 0 (every line's start line),\n"
    "N/B next/previous unit of a warped trail (or page of a book in hand), P play an audio book, F1 the setup menu, Tab free the mouse,\n"
    "Esc close or free the mouse, Ctrl+Q quit.\n\n"
    "All five lines share one corridor, 128 books per tile. Each line repeats along it; a\n"
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
std::unique_ptr<Hallway> make_hallway(SDL_Window* window, SDL_Renderer* renderer, const Args& a, bool scripted,
                                      const FilterConfig& filters)
{
    // The corridor's tile size, before any line's loop is worked out from it. It changes no
    // address: only the tile and slot that name a unit's place in the corridor (corridor.hpp).
    if (a.has("items-per-wall")) sieve::set_books_per_tile(a.get_u32("items-per-wall", 128));
    std::vector<Line> lines = make_lines(a);
    int start_line = 0;
    if (a.get("line") == "books") start_line = kBooksLine;
    else if (a.get("line") == "models") start_line = kModelsLine;
    else if (a.get("line") == "binary") start_line = kBinaryLine;
    else
    {
        const LineKind wanted = line_from_string(a.get("line", "text")); // "pages" is the text line
        for (int i = 0; i < 4; ++i)
            if (wanted == kLineOrder[i]) start_line = i;
    }
    const bool text_has_model = lines[0].guided != nullptr;
    // Books and models are not made of one alphabet, so neither starts with the text greeting.
    const LineKind start_kind = start_line == kBooksLine || start_line == kModelsLine || start_line == kBinaryLine
                                    ? LineKind::Image
                                    : lines[size_t(start_line)].kind;

    const Hallway::ModelShape shape{a.get_positive("vertices", 8), a.get_positive("faces", 12), a.get_positive("coords", 16)};
    auto hall = std::make_unique<Hallway>(window, renderer, std::move(lines), filters,
                                          a.has("book-pages") ? a.get_u32("book-pages", 4) : 4, shape);
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
        // The greeting is Latin letters, so on an alphabet that cannot hold them (Greek, kana,
        // hieroglyphs) nothing survives canonicalisation: start halfway along instead.
        if (!hall->warp("welcome to the sieve")) hall->go_to("50%");
        hall->put_back();
    }
    else
    {
        // Halfway along, and on the shelf: going to an address hands you the unit, and holding
        // one holds you still, so starting a line holding a record would start it unable to move.
        hall->go_to("50%");
        hall->put_back();
    }
    if (!scripted) return hall;

    if (a.has("zoom")) hall->zoom_to(a.get_positive("zoom", 20));
    if (a.has("tile"))
    {
        const std::string t = a.get("tile");
        const bool neg = !t.empty() && t[0] == '-';
        const int64_t n = int64_t(parse_whole(neg ? t.substr(1) : t, "--tile", uint64_t(1) << 62));
        hall->move_tiles(neg ? -n : n);
    }
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
    // The application's settings (sieve-hallway.ini, or --settings PATH). On the first start the
    // resolution is chosen from the display. Screenshots keep --size (default 1280x720) and the
    // default settings, so they come out the same on every machine.
    const std::filesystem::path app_path = a.has("settings") ? std::filesystem::path(a.get("settings")) : AppSettings::default_path();
    AppSettings app = shot ? AppSettings{} : AppSettings::load(app_path);
    if (shot && a.has("settings")) app = AppSettings::load(app_path);
    const DisplayInfo display = detect_display();
    if (!shot && app.resolution.w <= 0)
    {
        app.resolution = default_resolution(display);
        app.save(app_path);
    }
    if (!shot && !a.has("size")) { w = app.resolution.w; h = app.resolution.h; }
    if (shot && app.resolution.w <= 0) app.resolution = {w, h}; // the screenshot's own size
    set_language(a.has("language") ? a.get("language") : app.language);
    SDL_Window* window = SDL_CreateWindow("Sieve - hallway", w, h, SDL_WINDOW_RESIZABLE | (shot ? SDL_WINDOW_HIDDEN : 0));
    if (!window) throw std::runtime_error(std::string("cannot open a window: ") + SDL_GetError());
    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
    if (!renderer) throw std::runtime_error(std::string("cannot create a renderer: ") + SDL_GetError());
    SDL_SetRenderVSync(renderer, app.vsync ? 1 : 0);
    if (!shot && !a.has("size") && app.fullscreen) apply_video(window, app);
    auto finish = [&] {
        release_fonts(); // glyph textures belong to the renderer
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 0;
    };

    Settings settings = Settings::from_args(a);
    // Filter settings: --filters PATH, or sieve-filters.ini next to the executable.
    const std::string filters_path = a.has("filters") ? a.get("filters") : FilterConfig::default_path().string();
    if (a.has("filters") && !std::filesystem::exists(filters_path))
        std::cerr << "note: " << filters_path << " does not exist yet: no filters ticked (the menu saves your choices there)\n";
    FilterConfig filters = FilterConfig::load(filters_path);
    if (shot && a.has("main-menu"))
    {
        // A picture of the main menu; --press keys go to it (e.g. Down,Enter for Settings).
        MainMenu mm(window, renderer, app, a.has("settings") ? app_path : std::filesystem::path(), display);
        if (a.has("press"))
            for (const auto& [key, mod] : parse_presses(a.get("press"))) mm.press(key, mod);
        mm.render();
        if (!save_render(renderer, a.get("screenshot"))) throw std::runtime_error(std::string("screenshot failed: ") + SDL_GetError());
        std::cout << "main menu: language " << language_code() << ", font " << font_name() << "\n";
        std::cout << "saved " << a.get("screenshot") << "\n";
        return finish();
    }
    if (shot && a.has("menu"))
    {
        // A picture of the setup menu (for documentation and testing); --press keys go to the menu.
        Menu menu(window, renderer, settings, filters, filters_path);
        if (a.has("press"))
            for (const auto& [key, mod] : parse_presses(a.get("press"))) menu.press(key, mod);
        menu.render();
        if (!save_render(renderer, a.get("screenshot"))) throw std::runtime_error(std::string("screenshot failed: ") + SDL_GetError());
        std::cout << "saved " << a.get("screenshot") << "\n";
        return finish();
    }
    if (shot)
    {
        auto hall = make_hallway(window, renderer, a, true, filters);
        {
            const bool glow = app.edge_glow || a.has("edge-glow"), real = app.real_graphics || a.has("real-graphics");
            // --real-graphics brings Door Portals with it, as turning it on in the menu does. The
            // saved setting is taken as it stands: a settings file that turns portals off with
            // Real Graphics on means exactly that, since nothing may turn them off but you.
            const bool portals = app.door_portals || a.has("real-graphics") || a.has("door-portals");
            hall->set_graphics(glow && !real, real, portals);
            hall->set_model_cache(a.has("model-cache") ? int(a.get_u32("model-cache", 64)) : app.model_cache_mb);
            hall->set_angle_decimals(app.angle_decimals);
            hall->set_crate_px(a.get_u32("model-tile", 64));
            hall->set_fps_counter(app.fps_counter || a.has("fps-counter"));
            // On stderr: stdout is where the readout goes, which scripts read line by line.
            std::cerr << "graphics: edge glow " << (glow && !real ? "on" : "off") << ", real graphics " << (real ? "on" : "off")
                      << ", door portals " << (portals ? "on" : "off")
                      << ", fps counter " << (app.fps_counter || a.has("fps-counter") ? "on" : "off") << "\n";
        }
        if (a.has("bench"))
        {
            // Frame timing: render N frames (turning slowly, so nothing is cached between them).
            const uint32_t n = a.get_positive("bench", 60);
            hall->render();
            const Uint64 t0 = SDL_GetTicksNS();
            for (uint32_t i = 0; i < n; ++i)
            {
                hall->camera().yaw += 0.002f;
                hall->render();
                SDL_RenderPresent(renderer); // includes drawing the batched geometry
            }
            const double ms = double(SDL_GetTicksNS() - t0) / 1e6 / n;
            std::cout << "bench: " << n << " frames, " << ms << " ms per frame (" << 1000.0 / ms << " fps), renderer "
                      << SDL_GetRendererName(renderer) << "\n";
        }
        hall->render(); // computes what you are looking at
        if (a.has("take")) hall->take_hovered();
        hall->render();
        if (!save_render(renderer, a.get("screenshot"))) throw std::runtime_error(std::string("screenshot failed: ") + SDL_GetError());
        std::cout << "saved " << a.get("screenshot") << "\n";
        hall->release_textures();
        hall.reset(); // before SDL goes (its audio stream, its textures)
        return finish();
    }

    // Interactive: the main menu, then the setup menu, then the hallway (--no-menu goes straight
    // in). F1 in the hallway returns to the setup menu, and Esc there to the main menu.
    bool show_menu = !a.has("no-menu");
    bool show_main = show_menu;
    bool first = true;
    while (true)
    {
        if (show_main)
        {
            MainMenu mm(window, renderer, app, app_path, display);
            if (mm.run() == MainMenu::Result::Quit) break;
            show_main = false;
        }
        if (show_menu)
        {
            Menu menu(window, renderer, settings, filters, filters_path);
            const Menu::Result r = menu.run();
            if (r == Menu::Result::Quit) break;
            settings = menu.settings();
            filters = menu.filters();
            if (r == Menu::Result::Back)
            {
                show_main = true;
                continue;
            }
        }
        Args ha = a;
        if (show_menu) settings.apply(ha);
        if (!first)
            for (const char* k : {"warp", "goto", "zoom", "tile", "pose", "walk", "press"}) ha.opts.erase(k);
        auto hall = make_hallway(window, renderer, ha, first, filters);
        hall->set_controls(app.mouse_sensitivity, app.invert_mouse_y);
        hall->set_graphics(app.edge_glow, app.real_graphics, app.door_portals);
        hall->set_model_cache(app.model_cache_mb);
        hall->set_angle_decimals(app.angle_decimals);
        hall->set_crate_px(ha.get_u32("model-tile", 64));
        hall->set_fps_counter(app.fps_counter);
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
