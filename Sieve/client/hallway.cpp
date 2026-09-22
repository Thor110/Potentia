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
#include "sieve/utf8.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
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
// The corridor's lines: the four above, then books (made of pages and a picture).
constexpr int kLines = 5;
constexpr int kBooksLine = 4;
const Theme& theme_of(int li) { return li == kBooksLine ? kBooksTheme : kThemes[li]; }

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
    Hallway(SDL_Window* window, SDL_Renderer* renderer, std::vector<Line> lines, const FilterConfig& filters, uint32_t book_pages)
        : window_(window), r_(renderer), lines_(std::move(lines)), tile_geometry_(build_tile()), book_geometry_(build_books())
    {
        // The books line: a cover from the image line, a title and book_pages pages from the pages line.
        books_ = std::make_unique<BookSpace>(lines_[1].space, lines_[0].space, book_pages);
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
    const Line& line() const { return lines_[size_t(on_books() ? 0 : li_)]; }
    bool on_books() const { return li_ == kBooksLine; }
    const Theme& theme() const { return theme_of(li_); }
    Camera& camera() { return cam_; }
    bool guided_on() const { return !on_books() && guided_ && line().guided != nullptr; }
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
        // Keep the books already worked out: they are the same books, d tiles closer.
        std::unordered_map<int64_t, Book> shifted;
        const int64_t by = d * int64_t(kBooksPerTile);
        if (d > -16 && d < 16)
            for (auto& [key, b] : cache_) shifted.emplace(key - by, std::move(b));
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
    bool has_filters() const { return on_books() ? book_sieve_ && !book_sieve_->empty() : !stack().empty(); }
    std::string filter_status() const
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
        loop_ = LineLoop(units_of(li_));
        loop_tile_ = loop_.loop_tile(tile_);
        // Every line's loop too, for the double flag where all four start together.
        for (int i = 0; i < kLines; ++i)
        {
            all_loops_[i] = LineLoop(units_of(i));
            all_loop_tiles_[i] = all_loops_[i].loop_tile(tile_);
        }
        cache_.clear();
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
        std::optional<BookSpace::Parts> parts; // the books line: cover, title and pages
    };

    const Book& book(int64_t dt, uint32_t slot)
    {
        const int64_t key = dt * int64_t(kBooksPerTile) + slot;
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
            if (!on_books() && !b.survivor && !stack().empty())
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
            in_hand_where_ = trf("hand.where", {tile_label_, std::to_string(slot)});
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
        message(trf("msg.jumped", {std::to_string(n), std::to_string(n * int64_t(kBooksPerTile))}));
    }

    // A door keeps your corridor position and changes the line reading it: left wall to the next
    // line, right wall to the previous one. You come in through the opposite wall's door.
    void cross(Side side)
    {
        const int to = side == Side::Left ? (li_ + 1) % kLines : (li_ + kLines - 1) % kLines;
        cam_.pos.x = (kHalfWidth - 0.1f) * (side == Side::Left ? 1.0f : -1.0f);
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
            cam_.yaw += e.motion.xrel * look_;
            cam_.pitch = std::clamp(cam_.pitch - e.motion.yrel * look_ * (invert_y_ ? -1.0f : 1.0f), -1.45f, 1.45f);
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
        const Theme& th = theme();
        SDL_SetRenderDrawColor(r_, th.bg.r, th.bg.g, th.bg.b, 255);
        SDL_RenderClear(r_);
        SDL_SetRenderDrawBlendMode(r_, SDL_BLENDMODE_BLEND);

        hover_ = pick_book(cam_.pos, cam_.forward(), 0, 5.0f);
        if (hover_ && effective_mode() == FilterMode::Hide && !book(hover_->tile, hover_->slot()).passes) hover_.reset();

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
            for (const Segment& s : tile_geometry_) add(s, z0);
            const uint32_t books = books_in_tile(t);
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
                for (const Segment& s : book_geometry_[k]) add(s, z0, dim);
            }
        }
        if (edge_glow_) draw_glow(buckets);
        for (int i = 0; i < kBuckets; ++i)
        {
            const SDL_Color c = mix(th.edge, th.bg, float(i) / kBuckets);
            SDL_SetRenderDrawColor(r_, c.r, c.g, c.b, 255);
            const auto& b = buckets[size_t(i)];
            for (size_t k = 0; k + 1 < b.size(); k += 2) SDL_RenderLine(r_, b[k].x, b[k].y, b[k + 1].x, b[k + 1].y);
        }
        draw_hud(w, h);
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

    // Text cut to fit `width` pixels at `scale` (8 pixels per character at scale 1).
    static std::string fit(const std::string& s, float width, float scale)
    {
        const size_t cols = size_t(std::max(4.0f, width / (8 * scale)));
        return text_cells(s) <= cols ? s : fit_cells(s, cols - 2) + "..";
    }

    // Text in the language's font (8 pixels per cell at scale 1; see font.hpp).
    void text(float x, float y, const std::string& s, float scale, SDL_Color c) { draw_text(r_, x, y, s, scale, c); }

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
        draw_pixels(unit, on_books() ? lines_[1].image : line().image, x, y, size, frame);
    }
    void draw_pixels(const Space::Digits& unit, const ImageFormat& f, float x, float y, float size, int frame)
    {
        const auto px = render_image(unit, f);
        const float cell = size / float(std::max(f.width, f.height));
        const size_t base = size_t(frame) * f.width * f.height;
        // One texture, scaled with nearest-neighbour sampling (one rectangle per pixel costs a draw
        // call per pixel); per-pixel rectangles only if the texture cannot be made.
        static_assert(sizeof(Rgb) == 3, "Rgb must be packed RGB24");
        if (SDL_Texture* tex = SDL_CreateTexture(r_, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STATIC, int(f.width), int(f.height)))
        {
            SDL_UpdateTexture(tex, nullptr, px.data() + base, int(f.width * 3));
            SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_NEAREST);
            const SDL_FRect dst{x, y, f.width * cell, f.height * cell};
            SDL_RenderTexture(r_, tex, nullptr, &dst);
            SDL_DestroyTexture(tex);
        }
        else
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
        const std::string where = trf("hud.line", {tr(th.key)}) + "   " + ordering_name() +
                                  (guided_on() ? "  " + trf("hud.zoom", {std::to_string(zoom_)}) : std::string()) + "   " + tr("hud.tile") + " " +
                                  tile_label_ +
                                  (first.empty ? "   " + tr("hud.padding") : "   " + trf("hud.along", {percent(first.fraction)}));
        text(10, 7, fit(where, W - 20, 2), 2, ink);
        const std::string loop = trf("hud.loop", {loop_label_}) +
                                 (loop_.fills_whole_tiles() ? std::string() : " " + trf("hud.loop.padding", {std::to_string(loop_.padding())}));
        text(10, 28, fit((on_books() ? books_->id() : line().space.id()) + (guided_on() ? "   " + trf("hud.model", {line().model_id}) : std::string()) + "   " +
                         loop + "   " + filter_status() + "   " +
                         trf("hud.doors", {tr(theme_of((li_ + 1) % kLines).key), tr(theme_of((li_ + kLines - 1) % kLines).key)}), W - 20, 1),
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
                    const std::string title = ascii(utf8_encode(line().space.text_of(bk.parts->title)));
                    const auto rows = wrap_words(title, size_t(std::max(20.0f, (std::min(W - 20, 900.0f) - 110) / 16)));
                    for (size_t r = 0; r < rows.size() && r < 3; ++r) text(96, y + float(r) * 20, rows[r], 2, ink);
                }
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
        text(x + 14, cy, fit(tr("hand.title") + "   " + in_hand_where_ + "   " + trf("hud.along", {percent(bk.fraction)}), pw - 28, 2), 2, ink);
        cy += 28;
        const size_t cols2 = size_t((pw - 28) / 16), cols1 = size_t((pw - 28) / 8);
        if (bk.parts) cy = draw_book(*bk.parts, x, cy, pw, y + ph - 110);
        else switch (line().kind)
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
        text(x + 14, y + ph - 16, tr(bk.parts ? "hand.keys.book" : "hand.keys"), 1, ink);
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
        const auto title = wrap_words(ascii(utf8_encode(line().space.text_of(p.title))), tcols);
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
        const std::string page = ascii(utf8_encode(line().space.text_of(p.pages[size_t(book_page_)])));
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
    // From the main menu's settings: mouse look, and the graphics options (Geometry Edge Glow and
    // Real Graphics are recorded here for the renderer; both are off by default).
    void set_controls(int sensitivity_percent, bool invert_y)
    {
        look_ = 0.0025f * float(sensitivity_percent) / 100.0f;
        invert_y_ = invert_y;
    }
    void set_graphics(bool edge_glow, bool real_graphics)
    {
        edge_glow_ = edge_glow;
        real_graphics_ = real_graphics;
    }
    void put_back() { in_hand_.reset(); }

private:
    // The loop tile of the tile dt away from yours. Only needs a full modulo for small loops;
    // for big ones a step of a few tiles wraps at most once.
    BigUint offset_loop_tile(int64_t dt) const { return offset_loop_tile(loop_, loop_tile_, dt); }

    // True if tile dt away is the start of a loop on all four lines at once.
    bool all_start(int64_t dt) const
    {
        for (int i = 0; i < kLines; ++i)
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
    std::string tile_label_, loop_label_; // the readout's short forms of tile_ and the loop length
    LineLoop loop_{BigUint(1)};
    BigUint loop_tile_;   // tile_ mod loop_.tiles()
    LineLoop all_loops_[kLines] = {LineLoop(BigUint(1)), LineLoop(BigUint(1)), LineLoop(BigUint(1)), LineLoop(BigUint(1)), LineLoop(BigUint(1))};
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
    bool invert_y_ = false;
    bool edge_glow_ = false, real_graphics_ = false;
    FilterStack stacks_[kLines]; // the books line's are in book_stacks_
    BookStacks book_stacks_;
    std::unique_ptr<BookSieve> book_sieve_; // null if the books' filters failed to build
    std::unique_ptr<CompactLine> compact_[kLines]; // survivors in every ordering, where the stack can rank
    FilterMode modes_[kLines] = {FilterMode::Off, FilterMode::Off, FilterMode::Off, FilterMode::Off, FilterMode::Off};
    std::unique_ptr<BookSpace> books_; // the books line
    int book_page_ = 0;                // the page open in a book in hand
    Synth synth_;
};

// ---------------------------------------------------------------- options

const char* kUsage =
    "hallway - walk the Sieve's five lines in 3D\n\n"
    "Usage: hallway [options]\n\n"
    "Lines (the same meaning as in the sieve tool):\n"
    "  --line pages|image|audio|video|books  line to start in (default pages; text also works)\n"
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
    "  --edge-glow         draw with Geometry Edge Glow (or --settings a file that has it on)\n\n"
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
    std::vector<Line> lines = make_lines(a);
    int start_line = 0;
    if (a.get("line") == "books") start_line = kBooksLine;
    else
    {
        const LineKind wanted = line_from_string(a.get("line", "text")); // "pages" is the text line
        for (int i = 0; i < 4; ++i)
            if (wanted == kLineOrder[i]) start_line = i;
    }
    const bool text_has_model = lines[0].guided != nullptr;
    const LineKind start_kind = start_line == kBooksLine ? LineKind::Image : lines[size_t(start_line)].kind;

    auto hall = std::make_unique<Hallway>(window, renderer, std::move(lines), filters, a.has("book-pages") ? a.get_u32("book-pages", 4) : 4);
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
    SDL_SetRenderVSync(renderer, 1);
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
        hall->set_graphics(app.edge_glow || a.has("edge-glow"), app.real_graphics);
        hall->render(); // computes what you are looking at
        if (a.has("take")) hall->take_hovered();
        hall->render();
        if (!save_render(renderer, a.get("screenshot"))) throw std::runtime_error(std::string("screenshot failed: ") + SDL_GetError());
        std::cout << "saved " << a.get("screenshot") << "\n";
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
        hall->set_graphics(app.edge_glow, app.real_graphics);
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
