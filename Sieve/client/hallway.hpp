// Sieve hallway -- the Hallway class and what its parts share.
//
// The hallway is one class, because everything in it -- the corridor, the lines, where you stand,
// what you hold -- is one piece of state. Its drawing is split by subsystem into files of their
// own, which share this declaration: hallway.cpp (the corridor, the lines, movement, input and
// the frame), hud.cpp (everything drawn in screen space: the compass, the panels, the readout and
// the item page), door_portal.cpp (the noise in the doorways), binary_edge.cpp (the rain and the
// drop), item_faces.cpp (the pictures on the items' fronts and their render workers), and
// app_main.cpp (options, menus and the event loop).
//
// Every line shares one endless corridor lined with bookcases; every book is one unit and the
// address increases as you walk forward. Each line repeats along the corridor with its own
// period, and a checkered start line marks where each repeat begins. Doors in the walls lead to
// the next line (left wall) or the previous line (right wall) at the same corridor position, so
// every door lines up with a door in every other line. One tile of geometry is built once and
// repeated.
//
// In the guided ordering (text, with a model) each book is a point on the entropy-ordered line,
// 2^-zoom apart, showing the unit whose stretch of the line contains it. Probable text owns
// long stretches, so at any zoom the shelves are mostly readable; zooming out (-) shows the
// likeliest continuations, zooming in (=) the fine structure around one unit.

#pragma once

#include <SDL3/SDL.h>

#include "display.hpp"
#include "app_settings.hpp"
#include "camera.hpp"
#include "font.hpp"
#include "main_menu.hpp"
#include "menu.hpp"
#include "mesh.hpp"
#include "strings.hpp"
#include "synth.hpp"
#include "theme.hpp"
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
#include "cli/locate.hpp"
#include "sieve/binaryspace.hpp"
#include "sieve/titledspace.hpp"
#include "sieve/utf8.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <climits>
#include <cmath>
#include <condition_variable>
#include <cstdio>
#include <deque>
#include <filesystem>
#include <fstream>
#include <functional>
#include <future>
#include <iostream>
#include <iterator>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>


namespace hallway::hall {

using namespace sieve;
using namespace sieve::cli;

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
// it. Binary is one line, not two -- it wraps around the outside of the other six. Its room is
// always the same way round, shelves on one side of the line and the drop on the other; coming in
// from models you are turned to face the other way along it (see binary_from_).
constexpr int kLines = 7;
constexpr int kBooksLine = 4, kModelsLine = 5, kBinaryLine = 6;
// The short wall on the binary line's open edge, under Real Graphics: dark, so the drop past it
// is what the eye goes to.
constexpr SDL_FColor kRailColour{0.42f, 0.44f, 0.42f, 1.0f};
// Tiles drawn behind and ahead of the one you are in (and kept in the book cache).
constexpr int kCacheBack = 6, kCacheAhead = 7;
constexpr int kBuckets = 12; // distance fades of the wireframe
inline const Theme& theme_of(int li)
{
    return li == kBooksLine ? kBooksTheme : li == kModelsLine ? kModelsTheme : li == kBinaryLine ? kBinaryTheme : kThemes[li];
}

inline SDL_Color mix(SDL_Color a, SDL_Color b, float t)
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
inline std::vector<Segment> build_tile(bool hallway = true, bool shelves = true, int only = 0)
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
inline std::vector<Segment> build_edge(float sx)
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

inline std::string short_address(const std::string& hex)
{
    if (hex.size() <= 28) return hex;
    return hex.substr(0, 12) + "..." + hex.substr(hex.size() - 12) + " (" + std::to_string(hex.size()) + " digits)";
}

inline std::vector<std::string> wrap(const std::string& s, size_t width)
{
    std::vector<std::string> out;
    for (size_t i = 0; i < s.size(); i += width) out.push_back(s.substr(i, width));
    if (out.empty()) out.push_back("");
    return out;
}

// Greedy word wrap: lines of at most `width` characters, broken at spaces where possible;
// trailing spaces (a page's padding) are dropped.
inline std::vector<std::string> wrap_words(std::string s, size_t width)
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
inline std::string ascii(const std::string& utf8)
{
    std::string out;
    for (char32_t c : utf8_decode(utf8)) out.push_back(c < 0x80 ? char(c) : '?');
    return out;
}

inline std::string fixed(double v, int d)
{
    char b[48];
    std::snprintf(b, sizeof b, "%.*f", d, v);
    return b;
}

inline std::string percent(double f)
{
    char buf[48];
    std::snprintf(buf, sizeof buf, "%.10f%%", f * 100.0);
    return buf;
}

// The same position as an angle. Every line is a loop, so where you stand in it is a bearing:
// 0 degrees is where the loop starts and finishes, and it comes round to 0 again. How many
// decimal places is Angle Precision in Settings > Graphics.
inline std::string degrees(double f, int decimals)
{
    char fmt[16], buf[48];
    std::snprintf(fmt, sizeof fmt, "%%.%df", std::clamp(decimals, 0, 8));
    std::snprintf(buf, sizeof buf, fmt, f * 360.0);
    return std::string(buf) + "\xc2\xb0";
}

// ---------------------------------------------------------------- the hallway

enum class Input { None, Warp, Goto };

// Short form of a (possibly enormous) tile number for the readout.
inline std::string short_number(const std::string& dec)
{
    const bool neg = !dec.empty() && dec[0] == '-';
    const std::string d = neg ? dec.substr(1) : dec;
    if (d.size() <= 16) return dec;
    return (neg ? "-" : "") + d.substr(0, 6) + "..." + d.substr(d.size() - 4) + " (" + std::to_string(d.size()) + " digits)";
}

// Short form of a big number without converting all of it to decimal (which costs time
// quadratic in its length): exact below 20,000 bits; above that the digit count and the leading
// digits come from its logarithm (display only) and the last four digits are exact.
inline std::string short_big(const BigUint& v, bool negative = false)
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
        // Or the edge template (edge.obj), made for the shelves on the left and mirrored for the
        // right; when there is one it is drawn instead of the two above.
        std::shared_ptr<const Mesh> edge;
    };

    // The models line's shape, which is not a Line: V vertices, F triangles, a grid of C steps.
    // And the length of every line's titles (SPECIFICATIONS §11, "Titled lines").
    struct ModelShape
    {
        uint32_t vertices = 8, faces = 12, coords = 16;
        uint32_t title_length = 32;
        uint64_t binary_bytes = 32; // the binary line holds every file up to this many bytes
    };

    Hallway(SDL_Window* window, SDL_Renderer* renderer, std::vector<Line> lines, const FilterConfig& filters, uint32_t book_pages,
            ModelShape shape);

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
    // It is always the left (binary_shelf_ stays 0): the line is one room, the same way round
    // whichever end you meet it from. Coming in from the models end you are turned round instead
    // (cross()), so the drop is still on the line's right, and on your left. The geometry for
    // the other way round ([1] below) is kept, unused, so the wireframe tables stay symmetrical.
    int shelf_side() const { return binary_shelf_; } // 0: shelves left, drop right. 1: the mirror.
    float drop_sign() const { return binary_shelf_ == 0 ? 1.0f : -1.0f; }
    Media media() const;
    bool sizes_vary() const { return media_sizes_vary(media()); }
    const Theme& theme() const { return theme_of(li_); }
    Camera& camera() { return cam_; }
    bool guided_on() const { return !on_books() && !on_models() && !on_binary() && guided_ && line().guided != nullptr; }
    const GuidedLine& guided() const;
    const CompactLine& compact() const { return *compact_[li_]; }
    std::string ordering_name() const { return tr(std::string("ordering.") + (guided_on() ? "guided" : to_string(mode_))); }

    void set_line(int li);
    void set_mode(AddressMode m);
    void enable_guided();
    void move_tiles(int64_t d);

    FilterMode effective_mode(int i) const;
    FilterMode effective_mode() const { return effective_mode(li_); }
    const FilterStack& stack() const { return stacks_[li_]; }
    // The books line compacts when every part with filters can rank its survivors, and some survive.
    bool books_compact() const { return book_sieve_ && book_sieve_->can_rank() && !book_sieve_->count().is_zero(); }
    bool has_filters() const;
    // The filters' part of the readout, worked out when the line or its settings change (the
    // survivor count is a big number, too slow to write out every frame).
    const std::string& filter_status() const { return filter_status_; }
    std::string compute_filter_status() const;
    BigUint units_of(int i) const;

    void refresh_labels();

    void rebase();

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
        // The short form the readout writes, worked out here rather than in the draw path: it is a
        // decimal conversion of a number that may be thousands of digits long, and it never
        // changes once the book is known.
        std::string survivor_label;
        std::optional<BookSpace::Parts> parts;       // the books line: cover, title and pages
        std::optional<ModelSpace::Parts> model;      // the models line: vertices and faces
        // The binary line: a file. Only its place, title and cover are kept on the shelf; its bytes
        // and its address in hex (each as large as the file) are worked out when something asks,
        // for the one item looked at or held (file_of, hex_of below).
        bool is_file = false;
        Space::Digits title, cover;                  // a titled line: its title, and its cover if it has one
    };

    const Book& book(int64_t dt, uint32_t slot);
    // A book's address in hex, and a binary file's bytes: kept on the book where they are small,
    // worked out on demand where they are not (the binary line), remembering the last one asked.
    std::string hex_of(const Book& b);
    const BinarySpace::Bytes& file_of(const Book& b);
    BigUint memo_index_;
    std::string memo_hex_;
    BinarySpace::Bytes memo_file_;
    bool memo_hex_ok_ = false, memo_file_ok_ = false;

public:
    // Thin: over the budget, only the room you stand in keeps its items and their pictures (the
    // rooms either side borrow your room's pictures, as the far rooms always do). Set when the
    // setup menu is told to go in anyway, or the File Locator goes to a file past the budget.
    void set_thin(bool on);
    bool thin() const { return thin_; }
    // The binary line made long enough for a file (the File Locator's Go to it, past the budget).
    void set_binary_length(uint64_t bytes);

private:
    bool thin_ = false;
    int face_rooms() const { return thin_ ? 0 : kFaceRooms; }

public:

    // The current line's titled space, when its units are titled in the ordering you are walking:
    // every line but books and binary, in positional and scrambled order. Guided order and compact
    // mode still order and count the content alone, so there it is null.
    const TitledSpace* titled_here() const;
    const TitledSpace* titled_of(int i) const;
    // A book's title as text, without the padding it ends in; empty for none or a blank title.
    std::string title_text(const Book& b) const;
    float draw_null_title(float x, float y, float scale); // "[Null Title]"; returns its width
    // Whether this line's items have titles at all (title length above 0, and not guided or compact).
    bool has_titles() const
    {
        const auto* ts = titled_here();
        return ts && ts->title_space().has_value();
    }

    BigUint index_of(const Space::Digits& unit);

    void go_to_unit(const Space::Digits& unit, bool open);

    void go_to_book(const BookSpace::Parts& p, bool open);
    void go_to_file(const BinarySpace::Bytes& f, bool open, const Space::Digits* title = nullptr, const Space::Digits* cover = nullptr);

    BookSpace::Parts parts_of_record(const std::string& path);

    void place(const BigUint& index, bool open);

    void face(uint32_t slot);

    void message(const std::string& m);

    const Book* reference_book();

    void cycle_ordering();

    void zoom_to(uint32_t d) { zoom_by(int(d) - int(zoom_)); }

    void zoom_by(int delta);

    bool warp(const std::string& input);

    bool go_to(std::string input);

    void step_trail(int dir);

    void update(float dt, const bool* keys);

    // How far left and right you may walk. Normally a wall on either side; on the binary line the
    // drop side has no wall, so you can walk out onto the walkway as far as the short wall, and
    // there is no door out there -- its one door is in its one wall of shelves.
    float walk_lo() const { return on_binary() && drop_sign() < 0 ? -(kEdgeRail - 0.35f) : -kWalkLimit; }
    float walk_hi() const { return on_binary() && drop_sign() > 0 ? kEdgeRail - 0.35f : kWalkLimit; }

    void move_by(Vec3 d);

    void jump_tiles(int64_t n);

    void cross(Side side);

    void handle(const SDL_Event& e, bool& quit);

    void handle_event(const SDL_Event& e, bool& quit);

    void turn_page(int dir);

    void drop_in_hand();

    void take_or_return();

    void take_hovered();

    void open_input(Input which);
    void close_input();

    std::string status();

    void render();

    static const char* media_name(Media m);

    const Models& models();

    void draw_models(const Models& md, const bool* visible, int back, int ahead, int w, int h);

    void draw_glow(const std::vector<std::vector<SDL_FPoint>>& buckets);

    void draw_start_line(float z0);

    void fill(const std::vector<Vec3>& quad, SDL_Color c);

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
    static const std::array<uint32_t, 256>& smooth_table();

    static float noise_at(int x, int y, uint32_t seed);

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

    static SDL_Color portal_colour(const Theme& th);

    void draw_portal(const std::vector<Vec3>& quad, const Theme& dest, const float* depth, int dw, int dh);

    static std::string fit(const std::string& s, float width, float scale);

    // Text in the language's font (8 pixels per cell at scale 1; see font.hpp).
    void text(float x, float y, const std::string& s, float scale, SDL_Color c) { draw_text(r_, x, y, s, scale, c); }

    static SDL_Color ring_ink(const Theme& t, SDL_Color on);

    // Where line i stands in its own loop, 0 at the start of a copy and approaching 1 at its end.
    //
    // Measured in units rather than in tiles, so it is the same number the readout writes as a
    // percentage. The two differ whenever a loop does not fill its last tile: three tiles into a
    // six-tile loop of 729 units is not half way along it, it is 384/729 of the way.
    //
    // Cached by refresh_labels(); this is the working out behind the cache.
    double line_fraction(int i) const { return line_fraction_[size_t(i)]; }

    double compute_line_fraction(int i) const;

    void draw_compass(float W, float H);

    void panel(float x, float y, float w, float h, Uint8 alpha = 230);

    void draw_pixels(const Space::Digits& unit, float x, float y, float size, int frame);
    void draw_pixels(const Space::Digits& unit, const ImageFormat& f, float x, float y, float size, int frame);

    std::string one_line_preview(const Space::Digits& u);
    static std::string binary_preview(const BinarySpace::Bytes& f, size_t most);
    float draw_file(const BinarySpace::Bytes& f, float x, float cy, float pw, float bottom);

    void draw_hud(int w, int h);

    // ---- the Binary Edge
    //
    // Down each outer side of the corridor runs a half-hallway (world.hpp): an empty bookcase
    // against the inner wall, a walkway, a short wall, and then the floor ends. Off that edge
    // falls rain: columns of characters drawn from every Unicode block Sieve knows
    // (sieve/alphabet.hpp), which is the whole of what could be written and has not been. It is
    // green because that is what this is. The shelves facing it are empty because nothing out
    // there has been catalogued yet, and cataloguing it is a job for people.
    //
    // The rain is streaks of small characters falling through the open air beyond the edge.
    // Each streak stands at its own distance from the edge, anywhere out to kRainReach metres, the
    // way the books on a shelf each have their own height, so the rain has depth rather than
    // hanging in one sheet. Everything about a streak -- where it stands, how fast it falls, how
    // long it is, whether it is falling at all this time round -- is a hash of where it is and of
    // the clock, so nothing is stored and nothing swims as you walk. Most stand near the edge and
    // fewer further out. Near streaks are written in characters; far ones, too small to read, are
    // drawn as a line.
    // (theme.hpp: kEdgeInk, the one colour the edge is drawn in, here and on the setup map.)

    static constexpr float kRainReach = 50.0f;   // metres beyond the edge the furthest streak stands
    static constexpr float kRainGlyph = 0.07f;   // metres, one character of a streak
    static constexpr float kRainSpacing = 0.06f; // metres along the corridor between streaks
    static constexpr int kRainSparse = 2;        // a streak falls one time in this many
    static constexpr float kRainSpeed = 2.5f;    // metres a second, give or take half: slow enough to read

    const std::vector<char32_t>& rain_glyphs();

    static uint32_t hash3(uint32_t a, uint32_t b, uint32_t c);

    bool visible_band(float x, float& lo, float& hi) const;

    void draw_rain(int back, int ahead);

    void draw_binary_edge(const bool* visible, int back, int ahead, bool real);

    // ---- crate faces
    //
    // On the models line every slot holds the same crate, because a mesh cannot be read at a
    // hundred and twenty-eight to a tile. Instead, the crate you are looking at has its model
    // rendered to a small flat image and printed on its front, and then its neighbours do the
    // same, spreading outward along the shelf, for as long each frame as kFaceMsPerFrame allows.
    //
    // Only the room you are in and kFaceRooms either side of it get faces of their own: three
    // rooms is 384 crates at 128 a tile and 768 at 256, where the corridor in view can hold far
    // more. Past them a crate wears the face already rendered for the same slot in your room, so
    // the distance is filled with a stand-in that becomes the real face as you walk up to it.
    // Rendering only what the cache can keep is what lets it settle: when it tried to keep every
    // crate in view, a cache smaller than the view dropped faces you were looking at and drew
    // them again, every frame, for ever.

    // How wide those displays are drawn: the display size setting (GLOBAL in the setup menu),
    // or wider on a line whose items carry text, as wide as its letters need (display.hpp).
    // Bigger is sharper and four times the memory each time it doubles, so it trades against how
    // many the cache holds.
    int face_px_ = 64;   // the setting
    int letters_px_ = 8; // the letter size the width is chosen for; smaller letters are dashes
    int line_px_ = 64;   // what the current line's displays are drawn at
    int display_px_here() const;
    // A display is line_px_ wide and as tall as the item's front is, in proportion.
    int face_w() const { return line_px_; }
    int face_h() const { return std::max(1, int(std::lround(float(face_w()) * face_aspect_))); }
    size_t face_bytes() const { return size_t(face_w()) * size_t(face_h()) * 4; }
    float face_aspect_ = 0.40f / 0.28f; // height over width of the current line's picture
    static constexpr int kFaceRooms = 1;          // rooms either side of yours with faces of their own
    static constexpr double kFaceMsPerFrame = 4.0; // time each frame may spend rendering faces

    static void render_model_face(const ModelSpace& space, const ModelSpace::Parts& p, int n, float spin, float tilt,
                                  SDL_Color edge, std::vector<uint32_t>& px);

    static void fill_triangle(std::vector<uint32_t>& px, int n, const std::array<float, 3>& a, const std::array<float, 3>& b,
                              const std::array<float, 3>& c, uint32_t argb);

    // ---- the render workers
    //
    // A crate face at 512 pixels with a few thousand triangles takes tens of milliseconds to
    // draw, which no frame can spare, so the drawing is done by a few worker threads and the
    // frame only uploads what they have finished. A job is keyed by the crate's place counted
    // from where the corridor started (face_shift_), so a face finished after you have walked
    // on still lands on the right crate; and every job carries the generation it was asked for
    // in, so a face finished after the field was cleared (a new face size, a jump) is dropped.
    // A job is what to draw, as a painter: a function that fills a w x h ARGB picture from a
    // copy of the item it was made for, so the workers touch nothing the frame might change.
    using Painter = std::function<void(std::vector<uint32_t>& px, int w, int h)>;
    struct FaceJob
    {
        int64_t place;
        uint64_t generation;
        int w, h;
        Painter paint;
        bool sharp = false; // a close-up display (below), not the ordinary one
    };
    struct FaceDone
    {
        int64_t place;
        uint64_t generation;
        int w, h;
        std::vector<uint32_t> pixels;
        bool sharp = false;
    };
    // The painter for the item in a slot on the current line, or none if it has no picture.
    Painter face_painter(const Book& b) const;
    // Where the picture goes on the current line's items (faces.ini), read once per medium.
    const FaceRect& face_rect();
    std::unordered_map<std::string, FaceRect> face_rects_;

    void start_face_workers();

    void stop_face_workers();

    void clear_faces();

    size_t face_capacity() const;

    void make_face_room();

    void collect_faces(Uint64 until);

    SDL_Texture* item_face(int64_t dt, uint32_t slot, bool ask, bool& asked);

    void draw_item_faces(const bool* visible, int back, int ahead);

    void draw_face_image(SDL_Texture* tex, const Vec3 quad[4], std::vector<SDL_Vertex>& verts, float uu0 = 0,
                         float uu1 = 1);

    float draw_model(const ModelSpace::Parts& p, float x, float y, float pw, float bottom);

    std::string model_line_summary(const ModelSpace::Parts& p) const;

    static std::vector<std::string> split_lines(const std::string& s);

    void draw_in_hand(float W, float H);

    void draw_cost(const Book& bk, float x, float cy, float pw, float bottom);

    float draw_book(const BookSpace::Parts& p, float x, float cy, float pw, float bottom);

    bool menu_requested() const { return menu_requested_; }
    void clear_menu_request() { menu_requested_ = false; } // back from the setup menu without a new hallway
    ~Hallway();
    void release_textures();
    void set_controls(int sensitivity_percent, bool invert_y);
    void set_fps_counter(bool on) { fps_counter_ = on; }
    void set_angle_decimals(int d) { angle_decimals_ = std::clamp(d, 0, 8); }
    void set_face_px(uint32_t px);
    // The close-up display size (0 turns close-ups off); see sharp_.
    void set_closeup_px(uint32_t px)
    {
        const int v = px == 0 ? 0 : int(std::clamp(px, 64u, uint32_t(kMaxDisplayPx)));
        if (v == closeup_px_) return;
        closeup_px_ = v;
        clear_faces();
    }
    // The smallest letters drawn on item pictures, in picture pixels: a title or a page that
    // would need smaller ones is drawn as short bars instead. Changing it redraws the pictures.
    void set_letters_px(uint32_t px)
    {
        const int v = int(std::clamp(px, 1u, 64u));
        if (v == letters_px_) return;
        // The widths are chosen for it too (display_px_here), so every display is drawn again.
        letters_px_ = v;
        clear_faces();
    }
    void set_model_cache(int megabytes);
    void set_graphics(bool edge_glow, bool real_graphics, bool door_portals);
    void put_back() { in_hand_.reset(); }
    // The item page: 0 the thing itself, 1 what it costs to name it. Always back to the thing
    // when something new is picked up -- the page is about what is in your hands now.
    int hand_tab_ = 0;

private:
    // The loop tile of the tile dt away from yours. Only needs a full modulo for small loops;
    // for big ones a step of a few tiles wraps at most once.
    BigUint offset_loop_tile(int64_t dt) const { return offset_loop_tile(loop_, loop_tile_, dt); }

    bool all_start(int64_t dt) const;

    static BigUint offset_loop_tile(const LineLoop& loop, const BigUint& loop_tile, int64_t dt);

    uint32_t books_in_tile(int64_t dt) const;

    static std::vector<std::array<Segment, 4>> build_books(bool varied);

    SDL_Window* window_;
    SDL_Renderer* r_;
    std::vector<Line> lines_;
    std::vector<Segment> tile_geometry_, hall_geometry_, case_geometry_; // all, hallway only, bookcases only
    // The same three for the binary line, which has one wall, and its open side: [0] shelves on
    // the left and the drop on the right, [1] the mirror of that.
    std::vector<Segment> bin_tile_[2], bin_hall_[2], bin_case_[2], edge_geometry_[2];
    int binary_shelf_ = 0;
    // The line you came into binary from, which its one door leads back to and its sign names:
    // pages, or models. Pages when you start there.
    int binary_from_ = 0;
    // Where a door in the wall at sx (-1 left, +1 right) leads.
    int door_to(float sx) const
    {
        if (on_binary()) return binary_from_;
        return sx < 0 ? (li_ + 1) % kLines : (li_ + kLines - 1) % kLines;
    }
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
    double line_fraction_[kLines] = {}; // how far along each line you stand; see refresh_labels()
    std::unordered_map<int64_t, Book> cache_;
    std::optional<BookSlot> hover_;
    std::optional<Book> in_hand_;
    std::string in_hand_where_;
    std::vector<Space::Digits> trail_;
    size_t trail_index_ = 0;
    Input input_ = Input::None;
    std::vector<Point2> grid_; // draw_face_image's cell corners, kept between calls

    // ---- the File Locator (file_locator.cpp): the pause menu's, `sieve locate` in a window
public:
    void open_locator();
    void locate_now(const std::string& path); // scripted: open on it, measured at once
    // From the system dialogs' callbacks (any thread): what was chosen, handed to the next frame.
    void locator_picked(const std::string& path);
    void locator_save_to(const std::string& path);

private:
    struct LocatorResult
    {
        enum Kind { None, File, Folder } kind = None;
        std::string path, sha256, hex, table, error;
        std::vector<uint8_t> bytes; // a file
        cli::Manifest manifest;     // a folder
    };
    void close_locator();
    void locator_analyse(const std::string& path, bool sync = false);
    void locator_save(const std::string& to);
    void locator_go();
    void locator_event(const SDL_Event& e);
    void draw_locator(float W, float H);
    void stop_locator();
    bool loc_open_ = false;
    std::atomic<bool> loc_busy_{false};
    std::thread loc_worker_;
    std::mutex loc_mx_;
    LocatorResult loc_result_;
    std::string loc_status_;
    std::optional<std::string> loc_pending_pick_, loc_pending_save_;
    int loc_save_what_ = 0;
    bool loc_go_armed_ = false; // Go to it pressed once for a file past the BINARY length
    float loc_mx_pos_ = -1, loc_my_pos_ = -1;
    std::vector<std::pair<SDL_FRect, std::string>> loc_buttons_;

    // ---- the pause menu (pause_menu.cpp): Esc, with nothing in your hands
public:
    // What the pause menu has asked the application to open: the settings (after which the same
    // hallway goes on, paused) or the main menu (after which a new one is built).
    enum class Request { None, Settings, MainMenu };
    Request request() const { return request_; }
    void clear_request() { request_ = Request::None; }
    // After Settings, the new graphics and language: signs are lettered again, the rest is set.
    void settings_changed() { release_signs(); }

private:
    void open_pause();
    void close_pause();
    void pause_choose(int row, bool& quit);
    void pause_event(const SDL_Event& e, bool& quit);
    void draw_pause(float W, float H);
    bool pause_open_ = false, pause_confirm_ = false;
    int pause_row_ = 0;
    Request request_ = Request::None;
    std::vector<SDL_FRect> pause_rects_; // the rows as last drawn, for the pointer

    // ---- the address navigator (navigator.cpp): X opens the whole address, digit by digit
    void open_navigator();
    void close_navigator(bool go);
    void nav_step(size_t d, int dir);
    void nav_set(size_t d, uint32_t v);
    std::pair<size_t, float> nav_cell(size_t d) const;
    std::optional<std::pair<size_t, int>> nav_hit(float x, float y) const;
    void navigator_event(const SDL_Event& e);
    void draw_navigator(float W, float H);
    bool nav_open_ = false, nav_had_mouse_ = false;
    BigUint nav_value_;           // the position being edited, < loop_.units()
    std::string nav_hex_;         // nav_value_ in hex, nav_width_ digits
    size_t nav_width_ = 1, nav_sel_ = 0;
    std::optional<size_t> nav_hover_;
    std::string nav_note_;        // why the last typed digit was refused
    float nav_wheel_ = 0;
    // The layout of the last frame drawn, which the pointer is tested against.
    size_t nav_cols_ = 0;
    float nav_x0_ = 0, nav_y0_ = 0;
    int nav_scroll_ = 0, nav_rows_shown_ = 1;
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
    // Portal titles (door_portal.cpp): one lettered sign per line, over every doorway leading to it.
    struct SignCache { SDL_Texture* tex = nullptr; };
    std::array<SignCache, kLines> signs_{};
    static constexpr int kSignPxW = 480, kSignPxH = 160;          // 1.2 m by 0.4 m, 400 px a metre
    static constexpr float kSignBottom = 2.35f, kSignTop = 2.75f; // between the door and the ceiling
    SDL_Texture* sign_texture(int line);
    void draw_door_sign(float sx, float z0, std::vector<SDL_Vertex>& verts);
    void release_signs();
    bool door_portals_ = false;
    // A model in hand turns about the upright axis; the mouse or A and D drive it.
    float model_spin_ = 0.6f, model_tilt_ = 0.35f;
    // The crates whose models have been rendered to an image, and when each was last in view.
    struct Face
    {
        SDL_Texture* tex = nullptr;
        uint32_t used = 0;
        int w = 0; // a close-up's width; the ordinary displays are all line_px_
    };
    // ---- close-ups: a level of detail for the displays nearest you
    //
    // An ordinary display is drawn once at line_px_ for every item in three rooms. Walk up to an
    // item and its display covers far more of the screen than it has pixels, so the few items
    // that do are drawn again, larger: at the power of two that covers the width they take on
    // screen, from twice line_px_ up to the close-up size setting. The ordinary display stands in
    // until the close-up arrives, and a close-up is dropped once it has gone unused for a couple
    // of seconds or kSharpMax newer ones need its room. They are keyed by place (as the jobs
    // are), which does not change as you walk from room to room.
    std::unordered_map<int64_t, Face> sharp_;
    std::unordered_set<int64_t> sharp_pending_;
    int closeup_px_ = 1024; // 0: no close-ups
    static constexpr size_t kSharpMax = 24;
    float screen_width(const Vec3 f[4]) const; // how many pixels wide a display is drawn, 0 if not
    void drop_stale_sharp();
    // The Binary Edge: the rain's texture, and the characters the current font can draw.
    std::vector<char32_t> rain_pool_;
    std::string rain_font_;

    std::unordered_map<int64_t, Face> faces_;
    // The render workers and what passes between them and the frame (see start_face_workers).
    std::vector<std::thread> face_workers_;
    std::mutex face_mx_;
    std::condition_variable face_cv_;
    std::deque<FaceJob> face_jobs_;
    std::vector<FaceDone> face_done_;
    bool face_stop_ = false;
    uint64_t face_generation_ = 1;
    int64_t face_shift_ = 0;                  // tiles walked since the start, for crate places
    std::unordered_set<int64_t> face_pending_; // places asked for and not yet collected
    uint32_t face_budget_mb_ = 64;
    uint64_t face_warned_ = 0; // the (face size, budget) last warned about, so it is said once
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
    // The binary line: every file of 0..N bytes (SPECIFICATIONS §12.1, binary-v1). It has one
    // wall, so its files stand only in the left wall's half of each tile's slots; the loop counts
    // the right wall's slots as empty, and these two convert between a file's place on the line
    // and its place in the loop (the same number on every other line).
    std::unique_ptr<BinarySpace> binary_space_;
    BigUint loop_pos(const BigUint& unit) const;   // unit index -> loop position
    BigUint unit_of_pos(const BigUint& pos) const; // loop position (left-wall slot) -> unit index
    // How many units the current line holds: its loop's count, except on binary (see above).
    BigUint line_units() const;
    // Every titled line's space (null for books and binary), built with the lines.
    std::array<std::unique_ptr<TitledSpace>, kLines> titled_;
    int book_page_ = 0;                // the page open in a book in hand
    Synth synth_;
};

} // namespace hallway::hall
