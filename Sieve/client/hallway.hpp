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
#include "music.hpp"
#include "synth.hpp"
#include "dimensions.hpp"
#include "theme.hpp"
#include "world.hpp"

#include "cli/args.hpp"
#include "cli/book.hpp"
#include "cli/map.hpp"
#include "cli/filter_config.hpp"
#include "cli/image_io.hpp"
#include "cli/lines.hpp"

#include "sieve/audio.hpp"
#include "sieve/notes3.hpp"
#include "sieve/sound.hpp"
#include "sieve/booksieve.hpp"
#include "sieve/bookspace.hpp"
#include "sieve/composition.hpp"
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
#include <atomic>
#include <stdexcept>
#include <string>
#include <thread>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>


namespace hallway::hall {

using namespace sieve;
using namespace sieve::cli;

constexpr float kPi = 3.14159265358979f;

// ---------------------------------------------------------------- the lines (dimensions.hpp)

// The unit lines, in the order the hallway holds them: lines_[size_t(kind)]. Which door each stands
// at is kDimensions' business, not this.
constexpr LineKind kLineOrder[4] = {LineKind::Text, LineKind::Image, LineKind::Audio, LineKind::Video};
static_assert(int(LineKind::Text) == 0 && int(LineKind::Image) == 1 && int(LineKind::Audio) == 2 && int(LineKind::Video) == 3);
// (The doors, kLines of them, and kBooksLine, kModelsLine and kBinaryLine, theme_of() and walking
// round them: dimensions.hpp. Binary is one line, not two -- it wraps around the outside of the
// others. Its room is always the same way round, shelves on one side of the line and the drop on
// the other; coming in from the last door before it you are turned to face the other way along it
// (see binary_from_).)
// The short wall on the binary line's open edge, under Real Graphics: dark, so the drop past it
// is what the eye goes to.
constexpr SDL_FColor kRailColour{0.42f, 0.44f, 0.42f, 1.0f};
// (view_rooms(), the tiles drawn and kept either side of yours: the View Distance setting, menu.hpp.)
constexpr int kBuckets = 12; // distance fades of the wireframe

// An array of N copies of v, for the per-line arrays of things with no default.
template <class T, size_t... I>
std::array<T, sizeof...(I)> filled_(const T& v, std::index_sequence<I...>)
{
    return {{((void)I, v)...}};
}
template <size_t N, class T>
std::array<T, N> filled(const T& v)
{
    return filled_(v, std::make_index_sequence<N>());
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
// An opaque colour as ARGB, as the item pictures are kept.
inline uint32_t argb(SDL_Color c) { return 0xFF000000u | uint32_t(c.r) << 16 | uint32_t(c.g) << 8 | uint32_t(c.b); }

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
// 0 degrees is where the loop starts and finishes, and it comes round to 0 again. How many decimal
// places is Angle Precision in the setup menu. Exact: floor(v * 360 * 10^d / units), written with
// d places, so all of them are right however many there are (a double would run out at about 16
// figures, and the lines are far longer than that).
inline std::string exact_degrees(const BigUint& v, const BigUint& units, int decimals)
{
    const int d = std::clamp(decimals, 0, kMaxAngleDecimals);
    if (units.is_zero()) return d ? "0." + std::string(size_t(d), '0') : "0";
    BigUint scale = BigUint::pow(10, uint64_t(d));
    scale.mul_small(360);
    BigUint q, r;
    BigUint::divmod(BigUint::mul(v, scale), units, q, r);
    std::string digits = q.to_decimal();
    if (d == 0) return digits;
    if (digits.size() <= size_t(d)) digits.insert(0, size_t(d) + 1 - digits.size(), '0');
    return digits.substr(0, digits.size() - size_t(d)) + "." + digits.substr(digits.size() - size_t(d));
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
    char tail[8];
    std::snprintf(tail, sizeof tail, "%04u", v.mod_small(10000)); // (a copy of a number of megabytes cost more than this)
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
        uint32_t track_units = 4, movie_units = 4; // units of audio a track, of video a movie
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
    const Line& line() const { return line_at(li_); }
    // A composition's is its joined line (Composition::joined): the base line, N units long.
    const Line& line_at(int li) const
    {
        if (is_composition(li)) return *comps_[size_t(li)].joined;
        return unit_line(kDimensions[li].unit.value_or(LineKind::Text));
    }
    // A unit line, wherever its door is.
    const Line& unit_line(LineKind k) const { return lines_[size_t(k)]; }
    bool on_books() const { return li_ == kBooksLine; }
    // Tracks or movies: a composition of another line's units (sieve/composition.hpp).
    bool on_composition() const { return is_composition(li_); }
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
        // Its size and its first bytes (up to 16), worked out with its title from the top of the
        // number, without converting the whole file: what the shelf and the hover panel show, and
        // what file_type() reads its kind from.
        uint64_t file_size = 0;
        std::vector<uint8_t> head;
        // Its place on binary-v1 itself (positional, no title), whatever the ordering or filters:
        // what its bytes are worked out from (file_of).
        std::optional<BigUint> content;
        Space::Digits title, cover;                  // a titled line: its title, and its cover if it has one
        // The vault (tools/cli/vault.hpp): withheld content keeps its place but is never shown,
        // taken, saved or passed on. A file's is worked out with its bytes (file_withheld). An item
        // with pictures (an image, a video, a cover) has its verdict worked out on a worker
        // (vault_pictures below) and is held back, as if withheld, until it is in: `vault_pending`.
        // Anything that would take, save or describe it decides it on the spot (withheld()).
        mutable bool withheld = false;
        mutable bool vault_pending = false;
        // COST's guided row: the unit's guided address under `guided_by`, worked out the first time
        // the tab shows it (6.5 ms for a page of 3,000 characters, every frame before).
        mutable std::optional<sieve::GuidedLine::Code> guided_code;
        mutable const sieve::GuidedLine* guided_by = nullptr;
        // COST's file row and balance: the size of the item as the file F saves it, worked out the
        // first time the tab shows it (a picture is encoded as a PNG to find it); 0 if it cannot be.
        mutable std::optional<uint64_t> file_bytes;
        // A file's verdict once worked out (-1 not yet, 0 shown, 1 withheld), kept with the item so
        // looking back at a file costs nothing, and its slot when it stands in your room (-1
        // otherwise), where the vault worker (vault_ahead) may have worked it out already.
        mutable int8_t file_vault = -1;
        int32_t room_slot = -1;
    };

    const Book& book(int64_t dt, uint32_t slot);
    // A book's address in hex, and a binary file's bytes: kept on the book where they are small,
    // worked out on demand where they are not (the binary line), remembering the last one asked.
    // The reference holds until another file's is asked for.
    const std::string& hex_of(const Book& b);
    // What kind of file this is, from its first bytes (its signature, as the file itself says):
    // ZIP, PNG, EXE, ... TXT for readable text, EMPTY, or "?" for anything else.
    static std::string file_type(const std::vector<uint8_t>& head, uint64_t size);
    // A filename as a title on the binary line: canonicalised as text is warped, cut to the title
    // length. Blank without titles.
    Space::Digits title_for_name(const std::string& name) const;
    const BinarySpace::Bytes& file_of(const Book& b);
    // A book's address shortened for the panels (its first and last twelve hex digits and how many
    // there are), the same text as short_address(hex_of(b)); for a file, from the number itself,
    // without writing out its whole hex (ten million digits for a file of 5 MB).
    std::string short_hex_of(const Book& b);
    // The vault's verdicts on the files of your room, worked out on a worker as soon as the room
    // is built, so the first look at a file of megabytes does not wait for its bytes and hashes.
    // Results are kept by slot for the room they were worked out for (room_gen_); a room left,
    // rebuilt or remade (moving, a new ordering, a new length) is a new generation.
    void vault_ahead();
    void stop_vault_ahead();
    uint64_t room_gen_ = 0;
    std::mutex vault_mx_;
    std::unordered_map<uint32_t, int8_t> vault_done_; // slot -> verdict, for vault_done_gen_
    uint64_t vault_done_gen_ = ~uint64_t(0), vault_started_gen_ = ~uint64_t(0);
    std::atomic<uint64_t> vault_want_gen_{0};
    std::thread vault_thread_;
    BigUint memo_index_, memo_content_;
    std::string memo_hex_;
    BinarySpace::Bytes memo_file_;
    bool memo_hex_ok_ = false, memo_file_ok_ = false;
    bool memo_hex_survivor_ = false; // memo_hex_ is a survivor's compact address, not its place
    bool memo_withheld_ = false; // the vault's verdict on memo_file_

public:
    // Thin: over the budget, only the room you stand in keeps its items and their pictures (the
    // rooms either side borrow your room's pictures, as the far rooms always do). Set when the
    // setup menu is told to go in anyway (every line), or a walk to a file past the budget (the
    // binary line only: `line`, so the other lines keep their neighbours and pictures).
    void set_thin(bool on, int line = -1);
    bool thin() const { return thin_ && (thin_line_ < 0 || thin_line_ == li_); }
    // The binary line made long enough for a file (the File Locator's Go to it, past the budget).
    void set_binary_length(uint64_t bytes);

private:
    bool thin_ = false;
    int thin_line_ = -1; // the one line thin is for, or -1 for every line
    int face_rooms() const { return thin() ? 0 : picture_rooms(); } // (Picture Distance: menu.hpp)

public:

    // The current line's titled space, when its units are titled in the ordering you are walking:
    // every line but books and binary, in positional and scrambled order. Guided order and compact
    // mode still order and count the content alone, so there it is null.
    const TitledSpace* titled_here() const;
    const TitledSpace* titled_of(int i) const;
    // A book's title as text, without the padding it ends in; empty for none or a blank title.
    std::string title_text(const Book& b) const;
    // The vault: whether an item is withheld (its content, its title, a book's pages, a model's
    // .obj; a file by its bytes, worked out when they are), and putting one down if it is in hand.
    // What it reads of an item is gathered first (VaultCheck), so that the check itself can run
    // off the main thread: it needs nothing of the hallway that can change meanwhile.
    struct VaultCheck
    {
        const Line* line = nullptr;          // the line its unit and pages are read on
        const ImageFormat* covers = nullptr; // how a cover is drawn (the image line's format)
        Space::Digits cover, unit;           // its cover (as a picture), and its content
        std::optional<BookSpace::Parts> parts;
        std::string title, obj;              // a titled unit's title as text; a model's .obj
        bool model = false, file = false;
        // Whether it draws a picture: the part of the vault (PDQ) that takes time.
        bool pictures() const;
        bool verdict() const;
    };
    VaultCheck vault_check(const Book& b) const;
    bool vault_withholds(const Book& b) const { return vault_check(b).verdict(); }
    bool file_withheld(const Book& b);
    bool withheld(const Book& b)
    {
        if (b.vault_pending) // not in from the worker yet: decided now, for this one item
        {
            try
            {
                b.withheld = vault_withholds(b);
            }
            catch (const std::exception&)
            {
                b.withheld = true; // failed closed, as everywhere in the vault
            }
            b.vault_pending = false;
        }
        return b.withheld || (b.is_file && file_withheld(b));
    }
    // The vault's checks of items with pictures, on workers (one per core but the one that draws):
    // queued by book() as each item is built, and their verdicts put on the items by
    // vault_collect() each frame, for the room they were asked for (room_gen_). A picture costs
    // about 0.13 ms (PDQ), a video unit eight of them: on the main thread that was most of a new
    // room's time (600 ms for a room of videos).
    struct VaultJob
    {
        int64_t key;  // the item's place in cache_
        uint64_t gen; // room_gen_ when it was asked
        VaultCheck check;
    };
    void vault_queue(int64_t key, VaultCheck check);
    void vault_collect();
    void stop_vault_pictures();
    std::mutex vault_pic_mx_;
    std::condition_variable vault_pic_cv_;
    std::deque<VaultJob> vault_pic_jobs_;                          // (vault_pic_mx_)
    std::vector<std::tuple<int64_t, uint64_t, bool>> vault_pic_done_; // key, gen, verdict (vault_pic_mx_)
    int vault_pic_running_ = 0;                                    // jobs being checked (vault_pic_mx_)
    bool vault_pic_stop_ = false;                                  // (vault_pic_mx_)
    std::atomic<uint64_t> vault_pic_gen_{0};                       // room_gen_, for skipping old rooms' jobs
    std::vector<std::thread> vault_pic_threads_;
    // Waits for every vault check asked so far and puts its verdict on its item (a screenshot,
    // so that what it shows does not depend on how fast the workers were).
    void settle_vault();
    bool refuse_if_withheld();
    bool refused_ = false; // set when refuse_if_withheld puts something down, so its message stands
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
    void go_to_composition(const CompositionSpace::Parts& p, bool open); // a track or movie, on its line
    std::string record_ext() const;                           // ".track" or ".movie"
    std::string composition_record_of(const Book& bk) const; // a track or movie in hand, as a record
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
    static constexpr int kGrain = kPortalGrain; // (world.hpp)
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
    // Its size and first bytes; f may be only the head of a file of `size` bytes (the default: all of it).
    static std::string binary_preview(const BinarySpace::Bytes& f, size_t most, uint64_t size = UINT64_MAX);
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
    // same, spreading outward along the shelf, for as long each frame as face_ms_per_frame() allows.
    //
    // Only the room you are in and picture_rooms() either side of it (the Picture Distance setting,
    // 1 at first) get faces of their own: three
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
    DisplayText display_text_here() const; // what the displays on this line carry
    // A display is line_px_ wide and as tall as the item's front is, in proportion.
    int face_w() const { return line_px_; }
    int face_h() const { return std::max(1, int(std::lround(float(face_w()) * face_aspect_))); }
    size_t face_bytes() const { return size_t(face_w()) * size_t(face_h()) * 4; }
    float face_aspect_ = 0.40f / 0.28f; // height over width of the current line's picture
    int texture_px_ = 1024;               // the renderer's widest texture (gpu::max_texture_px)
    // The time each frame may spend taking in rendered faces: a quarter of a frame at the
    // display's refresh rate (4 ms at 60 Hz, 1.7 ms at 144), the rest left for drawing the world.
    double face_ms_per_frame() const;

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
    // An audio unit as text: a note set's notation, or a pcm unit's waveform, each channel a row of
    // `columns` shades (cli::pcm_preview).
    std::string audio_text(const Space::Digits& u, uint32_t columns) const;
    bool on_sound() const; // on the audio line, holding a pcm set (sound itself)
    // A pcm unit's waveform: each channel a band of h / channels, each column the line from its
    // lowest to its highest sample (sieve::pcm_envelope), over a dimmer line at silence.
    void draw_waveform(const Space::Digits& u, float x, float y, float w, float h, SDL_Color ink);
    std::vector<uint32_t> waveform_argb(const Space::Digits& u, uint32_t w, uint32_t h) const;

    void draw_in_hand(float W, float H);

    void draw_cost(const Book& bk, float x, float cy, float pw, float bottom);
    float draw_balance(double best, double file_bits, const std::string& by, float x, float cy, float pw);
    // Variable length addressing (COST): the shortest route found to a unit (corridor.hpp
    // shortest_path), worked out on a worker, since on a line of long files it takes seconds. The
    // worker is never waited for: it finishes the unit it was given, and the next one asked for is
    // started after it. The answer, or null while it is being worked out.
    const sieve::ShortestPath* shortest_path_of(const BigUint& v, const BigUint& units);
    std::thread vla_thread_;
    std::atomic<bool> vla_busy_{false};
    std::mutex vla_mx_;
    BigUint vla_v_, vla_units_;                       // what the answer below is for (vla_mx_)
    std::optional<sieve::ShortestPath> vla_done_;     // (vla_mx_)
    uint64_t vla_done_gen_ = 0, vla_shown_gen_ = 0;   // which answer each is (done: vla_mx_)
    std::optional<sieve::ShortestPath> vla_shown_;    // a copy for the frames, made once per answer

    float draw_book(const BookSpace::Parts& p, float x, float cy, float pw, float bottom);

    bool menu_requested() const { return menu_requested_; }
    void clear_menu_request() { menu_requested_ = false; } // back from the setup menu without a new hallway
    // Back from the setup menu to this hallway: to the pause menu if Restart Sieve opened it
    // (true: the pointer is then free), else to walking.
    bool back_from_menu()
    {
        menu_requested_ = false;
        if (!restart_from_pause_) return false;
        restart_from_pause_ = false;
        open_pause();
        return true;
    }
    ~Hallway();
    void release_textures();
    void set_controls(int sensitivity_percent, bool invert_y);
    void set_fps_counter(bool on) { fps_counter_ = on; }
    void set_angle_decimals(int d)
    {
        angle_decimals_ = std::clamp(d, 0, kMaxAngleDecimals);
        refresh_labels(); // the bearing's text is worked out to this many places
    }
    void set_face_px(uint32_t px);
    // The close-up display size (0 turns close-ups off, kCloseUpScreen follows the screen); see
    // sharp_.
    void set_closeup_px(uint32_t px)
    {
        const uint32_t v = px == 0 || px == kCloseUpScreen ? px : std::clamp<uint32_t>(px, 64, uint32_t(texture_px_));
        if (v == closeup_setting_) return;
        closeup_setting_ = v;
        clear_faces();
    }
    // The graphics card's memory (Settings > Graphics), which the close-ups have what is left of.
    void set_graphics_memory(int gb) { graphics_bytes_ = double(std::max(gb, 1)) * 1073741824.0; }
    // The smallest letters drawn on item pictures, in picture pixels: a title or a page that
    // would need smaller ones is drawn as short bars instead. Changing it redraws the pictures.
    // A video saved as a video (GIF, MP4, WebM through ffmpeg): frames a second (the "video-fps" setting).
    void set_export_fps(uint32_t fps) { export_fps_ = std::max(1u, fps); }
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
    // the first door or the last before binary. The first when you start there.
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
    std::array<LineLoop, kLines> all_loops_ = filled<kLines>(LineLoop(BigUint(1)));
    BigUint all_loop_tiles_[kLines]; // where you last stood on each line (the current: where you are)
    // The doors you came through since you last moved, so that going straight back returns
    // exactly: each time, you came from `to_tile` of `to_line` to `on_tile` of `on_line`.
    struct DoorBack
    {
        int to_line = 0;
        BigUint to_tile;
        int on_line = 0;
        BigUint on_tile;
    };
    std::vector<DoorBack> door_back_;
    double line_fraction_[kLines] = {}; // how far along each line you stand; see refresh_labels()
    std::string bearing_text_;          // the line you are on, as an exact bearing (refresh_labels())
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
    void install_now(const std::string& from, const std::string& to) { open_locator(); locator_install(from, to, true); }
    // From the system dialogs' callbacks (any thread): what was chosen, handed to the next frame.
    void locator_picked(const std::string& path);
    void locator_save_to(const std::string& path);
    void locator_install_picked(int step, const std::string& path); // 0 the installer, 1 where
    void locator_install(const std::string& from, const std::string& to, bool sync = false);

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
    std::optional<std::pair<int, std::string>> loc_pending_install_;
    std::string loc_install_src_; // the installer chosen, while its folder is being chosen
    int loc_save_what_ = 0;
    bool loc_go_armed_ = false; // Go to it pressed once for a file past the BINARY length
    float loc_mx_pos_ = -1, loc_my_pos_ = -1;
    std::vector<std::pair<SDL_FRect, std::string>> loc_buttons_;
    // Onto the binary line, in front of a file you have (the locator's Go to it, a map node's):
    // its name as its title. `past`: longer than the BINARY length, so the line is made to fit.
    void walk_to_file(const std::vector<uint8_t>& bytes, const std::string& name, bool past);
    // Work too long for one frame (a walk to a file of megabytes), done on a worker while this
    // thread keeps the window answering and, past a fifth of a second, says so: a red word with
    // its dots counting, as CALCULATING does while a hallway is built. Input that arrives
    // meanwhile is dropped (a click storm on a window that is busy would otherwise land later),
    // and a request to quit is kept for afterwards. The job must not touch the renderer.
    void busy(const std::string& word, const std::function<void()>& job);
    // The room you stand in worked out ahead of drawing it, and the file in hand's bytes and hex,
    // so the first frame after a long walk does not do it.
    void warm_room();
    // The binary line: your room's files worked out side by side on every core during a walk (each
    // is independent, and one of megabytes takes a tenth of a second or more), then picked up by
    // book(). Compact: each found by its survivor number. Otherwise: its title, head and place,
    // and, where a filter needs the whole file to judge it, its verdict. Only what the shelf keeps.
    struct FoundFile
    {
        uint64_t size = 0;
        std::vector<uint8_t> head;
        BigUint content;
        Space::Digits title;      // (not compact) its title
        std::string failed_by;    // (not compact) the first filter it fails, where the stack needs the
        bool judged = false;      // whole file to say (judged then)
    };
    std::unordered_map<uint32_t, FoundFile> found_; // slot -> its file, for found_gen_
    uint64_t found_gen_ = ~uint64_t(0);
    void find_room_files();
    // The first unit of the tile you stand in, and the binary line's first file there (the left
    // wall's first slot), kept for the room: every item of the room is one of these plus its slot,
    // which on a line of files of megabytes saves a few shifts of numbers that size per item.
    std::optional<BigUint> room_first_, room_first_file_;
    uint64_t room_first_gen_ = ~uint64_t(0);
    std::optional<BigUint> room_unit(uint32_t slot); // the unit at a slot of your room, or none past the line
    // A binary item's file by its place in the corridor: unit_of_pos(pos), or, in your room, the
    // room's first file plus the slot (the same number: the left wall's slots run on from it).
    BigUint file_place(int64_t dt, uint32_t slot, const BigUint& pos);

    // ---- the node graph (node_graph.cpp): maps of verified anchors, O or the pause menu, and
    // the item page's SORT tab
public:
    void open_graph();
    // Scripted (--map PATH): a map file or a folder, loaded (or made) at once and chosen; with the
    // viewer open if `open`, its layout settled so a screenshot shows it laid out.
    void graph_map_now(const std::string& path, bool open);
    // From the system dialogs' callbacks (any thread).
    void graph_picked(int what, const std::string& path);

private:
    struct GraphMap
    {
        std::string title;            // as the list shows it
        std::filesystem::path file;   // the .map it was read from (empty: made here)
        std::filesystem::path root;   // where its root folder is on this computer
        bool installation = false;    // "This installation": made from the program's own folder
        bool ready = false;           // loaded or made (a map being made is not yet)
        std::string error;
        cli::Map map;
        std::vector<std::vector<uint32_t>> adj;  // neighbours, either way along an edge
        std::vector<uint8_t> present;            // a file node's file is here, at its size
        std::vector<int8_t> verified;            // -1 not yet checked, 0 no, 1 its SHA-256 matches
        std::unordered_map<std::string, uint32_t> by_sha; // a file's SHA-256 to its (first) node
        std::vector<Vec3> pos, vel;              // the layout
        int steps = 0;                           // layout steps taken
        void prepare();                          // the above, from the map
        bool read_only() const;                  // This installation, and sealed maps (the release map)
    };
    void graph_list();                  // what the dropdown offers, found once
    void graph_select(int i);
    GraphMap* graph_current();
    bool graph_step(GraphMap& g, int budget); // a few layout steps; false once settled
    // The graph drawn into `area`, turned by yaw and pitch, around `focus` (or the whole map);
    // returns the node under (mx, my), or -1. `lone`: only a single point, for an item in no map.
    int draw_graph(GraphMap* g, SDL_FRect area, float yaw, float pitch, float zoom, int focus, bool lone, float mx, float my);
    void graph_event(const SDL_Event& e);
    void draw_graph_view(float W, float H);
    void draw_sort(const Book& bk, float x, float cy, float pw, float bottom);
    int sort_node(const Book& bk);      // the item in hand as a node of the current map, or -1
    void graph_go(int node);
    void graph_export(const std::string& to);
    void graph_make(const std::string& folder, bool sync = false);
    void graph_poll(); // what the worker and the dialogs have handed over
    void graph_save(GraphMap& g);
    void graph_add_anchor(const Book& bk); // V: the file in hand, to the chosen map (or out of it)
    // F: the item in hand saved as a file (item_save.cpp). The dialog's answer arrives on the
    // system's thread (item_save_chosen) and is written at the next frame (item_save_poll).
public:
    void item_save_chosen(const std::string& path, int filter = -1);
    void save_in_hand_to(const std::string& path);
    void save_view_to(const std::string& path); // F in the viewer, to PATH (--save-view)
private:
    void save_in_hand();
    // J: between an item and its file. On any other line, the item in hand as the file F saves
    // (a picture at one pixel a pixel) is found on the binary line; on the binary line, a file
    // whose kind is one a line holds (TXT, PNG, JPG, GIF, BMP, MID, BOOK) is opened on that line,
    // fitted to it as T would fit it.
    void jump_kind();
    std::vector<uint8_t> item_file(const Book& bk, std::string& name);
    void open_as_kind(const std::vector<uint8_t>& bytes);
    std::string prepare_save();
    void item_save_poll();
    std::string binary_file_name(const Book& bk, const std::string& sha);
    std::mutex save_mx_;
    std::optional<std::string> save_pending_;
    int save_filter_ = -1;
    uint32_t export_fps_ = cli::kDefaultExportFps; // the dialog's chosen filter: an index into save_formats_, or -1
    // The formats F offers for the item in hand (cli::export_formats: its own, then ffmpeg's), and
    // the dialog's filters made from them (kept while the dialog is open).
    std::vector<cli::ExportFormat> save_formats_;
    std::vector<std::string> save_filter_text_;
    std::vector<SDL_DialogFileFilter> save_filters_;
    std::optional<Book> save_item_;
    std::optional<std::vector<uint8_t>> save_blob_; // or a file made already: a view's picture or text
    std::string save_ext_;
    int hand_tabs();                       // 3, or 4 with META
    void draw_meta(const Book& bk, float x, float cy, float pw, float bottom);
    void graph_remove(int node);
    // The real names of files walked to (the locator, T with a path, a map's node), by SHA-256,
    // so an anchor made of one keeps its name rather than its canonicalised title.
    std::unordered_map<std::string, std::string> walked_names_;
    void close_graph();
    void stop_graph();
    bool graph_open_ = false;
    bool graph_listed_ = false;
    std::vector<GraphMap> graphs_;
    int graph_sel_ = 0;
    float graph_yaw_ = 0.6f, graph_pitch_ = 0.3f, graph_zoom_ = 1.0f;
    float sort_yaw_ = 0.6f, sort_pitch_ = 0.3f;
    int graph_hot_ = -1, graph_pick_ = -1;
    bool graph_drop_ = false, graph_drag_ = false, graph_go_armed_ = false;
    float graph_mx_ = -1, graph_my_ = -1, graph_down_x_ = 0, graph_down_y_ = 0;
    std::vector<std::pair<SDL_FRect, std::string>> graph_buttons_;
    std::string graph_status_;
    // Making a map (walking and hashing a folder) runs on a worker; its result is handed over.
    std::thread graph_worker_;
    std::atomic<bool> graph_busy_{false};
    std::mutex graph_lock_;
    std::optional<GraphMap> graph_made_;
    std::optional<std::pair<int, std::string>> graph_pending_;
    std::unordered_map<std::string, std::string> sort_sha_; // a binary item's index (hex) to its file's SHA-256

    // ---- the pause menu (pause_menu.cpp): Esc, with nothing in your hands
public:
    // What the pause menu has asked the application to open: the settings (after which the same
    // hallway goes on, paused) or the main menu (after which a new one is built).
    // GoToMelody: a melody of the music to walk to, on an audio line (or with tracks of a number
    // of units) other than this one's, so the application builds a hallway that has it and hands
    // it the melody (melody_to_go()).
    enum class Request { None, Settings, MainMenu, GoToMelody };
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

    // ---- the media player (media_player.cpp): the music's settings, its recent melodies and
    // favourites, from the pause menu
public:
    // Whether a menu is over the world (the pause menu or a tool opened from it): the music
    // player's MENUS mode, else WORLD.
    bool in_menu() const { return pause_open_ || nav_open_ || loc_open_ || graph_open_ || media_open_; }
    // Tells the music where you are: the line's colours for the NOW PLAYING box, and the line for
    // WORLD's character. At every change of line, and when the hallway is back from a menu.
    void music_line();
    void open_media_player();
    // Walk to a melody of the music: on the audio line (or the tracks line, for a track), facing
    // it, the item in hand. One of another length or note set than this hallway's audio line, or a
    // track of another number of units than its tracks', cannot be reached here: request() is then
    // GoToMelody.
    void go_to_melody(const MusicMelody& m);
    const std::optional<MusicMelody>& melody_to_go() const { return melody_to_go_; }
    void media_saved(const std::string& path); // the save dialog's choice (any thread)

private:
    struct MediaRow
    {
        enum class Kind { Mode, On, Volume, Source, Units, Length, NoteSet, Low, High, Durations, Voices, Tempo, Voice, Echo, Gap, Character, Fifths, LineMode, Next, Filters, Filter, Param, Play, GoTo, Save, Favourite };
        Kind kind;
        std::string filter, key; // Filter and Param rows
        int line = -1;           // LineMode rows
    };
    std::vector<MediaRow> media_rows() const;
    std::optional<MusicMelody> media_selected() const; // the highlighted entry of the list last used
    void media_change(int dir, bool big, bool& quit);
    void media_act(MediaRow::Kind k, bool& quit);
    void close_media_player();
    void media_event(const SDL_Event& e, bool& quit);
    void draw_media_player(float W, float H);
    bool media_open_ = false, media_had_mouse_ = false;

    // ---- the item viewer (viewer.cpp): Z, or a click on the thing in hand, opens it over the
    // whole window, scrolled both ways and zoomed
    // What the thing can be seen as, the buttons along the top: what it is (its text, pixels, notes,
    // .obj or bytes), the picture on the item, its cover, and its title.
    enum class ViewKind { Raw, Picture, Cover, Title };
    struct ViewDoc
    {
        std::string heading;               // what is open, above it
        std::vector<std::u32string> rows;  // text, as laid out
        sieve::BinarySpace::Bytes bytes;   // a file: its hex dump, worked out a row at a time
        size_t cols = 0, row_count = 0;    // the text's size in characters
        bool picture = false;              // a picture, in pixels
        std::vector<uint32_t> px;          // ARGB, frame after frame (empty while it is being drawn)
        uint32_t pw = 0, ph = 0, frames = 1;
        int frame = 0;
        bool playing = true;
        Uint64 next_frame = 0;
    };
    void open_viewer();
    void view_show(size_t at);
    void view_raw();
    std::string view_label(ViewKind k) const;
    int view_picture_px(double aspect) const;
    void save_view();
    std::optional<std::pair<std::vector<uint8_t>, std::string>> view_file(); // the file F saves from a view: bytes, extension
    void close_viewer();
    bool over_hand_view(const SDL_Event& e) const;
    void viewer_event(const SDL_Event& e, bool& quit);
    void draw_viewer(float W, float H);
    void view_book_page();
    std::u32string view_row(size_t r) const;
    SDL_FRect view_area() const;
    float view_content_w() const;
    float view_content_h() const;
    float view_fit_zoom() const;
    void view_set_zoom(float z);
    void view_zoom_at(float factor, float sx, float sy);
    void view_clamp();
    ViewDoc view_;
    std::vector<ViewKind> view_kinds_;              // the views this thing has
    size_t view_at_ = 0;                            // the one shown
    std::future<std::vector<uint32_t>> view_job_;   // the picture on the item, being drawn
    std::vector<std::pair<SDL_FRect, int>> view_buttons_; // the buttons last drawn: a view, or -1 to save
    bool view_open_ = false, view_had_mouse_ = false, view_drag_ = false, view_dirty_ = true;
    float view_zoom_ = 1, view_x_ = 0, view_y_ = 0; // the zoom, and the point of the thing at the view's top left
    double view_aspect_ = 1.0;                      // a page's shape, height over width
    SDL_Texture* view_tex_ = nullptr;               // a picture, sampled to the view's size
    int view_tex_w_ = 0, view_tex_h_ = 0;
    std::vector<uint32_t> view_px_;
    SDL_FRect hand_view_rect_{};                    // where the thing in hand was drawn on its page
    int media_area_ = 1;          // 0 recent, 1 the controls, 2 favourites
    int media_list_ = 0;          // the list the actions act on: 0 recent, 2 favourites
    int media_row_ = 0, media_sel_[3] = {0, 0, 0}, media_top_ = 0;
    MusicMode media_mode_ = MusicMode::World;
    std::vector<std::pair<SDL_FRect, std::pair<int, int>>> media_rects_; // area, row
    std::mutex media_mx_;
    std::string media_status_;
    std::vector<uint32_t> media_saving_;
    sieve::NoteSet media_saving_set_;
    std::optional<MusicMelody> melody_to_go_;

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
    // The bearing field at the foot of the navigator: an angle typed at the Angle Precision of the
    // setup menu, which moves the digits to the first unit at or past that bearing.
    bool nav_angle_focus_ = false, nav_angle_fresh_ = false;
    std::string nav_angle_;       // as typed (or, before any typing, the position's own bearing)
    SDL_FRect nav_angle_box_{};
    std::string nav_angle_of(const BigUint& v) const; // v's bearing, exactly, to angle_decimals_ places
    void nav_angle_key(SDL_Keycode k);
    void nav_angle_apply();
    float nav_wheel_ = 0;
    // The layout of the last frame drawn, which the pointer is tested against.
    size_t nav_cols_ = 0;
    float nav_x0_ = 0, nav_y0_ = 0;
    int nav_scroll_ = 0, nav_rows_shown_ = 1;
    std::string text_;
    std::string message_;
    Uint64 message_until_ = 0;
    bool menu_requested_ = false;
    bool restart_from_pause_ = false; // the setup menu was opened by the pause menu's Restart
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
    // (kSignPxW by kSignPxH, 1.2 m by 0.4 m at 400 px a metre: world.hpp)
    static constexpr float kSignBottom = 2.35f, kSignTop = 2.75f; // between the door and the ceiling
    SDL_Texture* sign_texture(int line);
    void draw_door_sign(float sx, float z0, std::vector<SDL_Vertex>& verts);
    void release_signs();
    bool door_portals_ = false;
    // A model in hand turns about the upright axis; the mouse or A and D drive it, and R sets it
    // back. Its picture on the shelf (and in the viewer) is always drawn at the resting turn, so
    // turning it in hand draws nothing again.
    static constexpr float kModelSpin = 0.6f, kModelTilt = 0.35f;
    float model_spin_ = kModelSpin, model_tilt_ = kModelTilt;
    // The crates whose models have been rendered to an image, and when each was last in view.
    struct Face
    {
        SDL_Texture* tex = nullptr;
        uint32_t used = 0;
        int w = 0; // a close-up's width; the ordinary displays are all line_px_
    };
    // ---- close-ups: a level of detail for the displays nearest you
    //
    // An ordinary display is drawn once at line_px_ for every item in the rooms with pictures (yours
    // and the Picture Distance either side: three at first). Walk up to an
    // item and its display covers far more of the screen than it has pixels, so the few items
    // that do are drawn again, larger: at the power of two that covers the width they take on
    // screen, from twice line_px_ up to the close-up size setting. The ordinary display stands in
    // until the close-up arrives, and a close-up is dropped once it has gone unused for a couple
    // of seconds or sharp_max() newer ones need its room. They are keyed by place (as the jobs
    // are), which does not change as you walk from room to room.
    std::unordered_map<int64_t, Face> sharp_;
    std::unordered_set<int64_t> sharp_pending_;
    uint32_t closeup_setting_ = kCloseUpScreen; // 0: no close-ups
    double graphics_bytes_ = 4.0 * 1073741824.0; // set_graphics_memory
    int closeup_px() const;  // the widest a close-up is drawn now (display.hpp closeup_width)
    size_t sharp_max() const; // how many are kept (display.hpp closeup_count)
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
    // Tracks and movies, by door (empty at every other door). Each item is shown, played and saved
    // as one unit of `joined`, the base line N units long, its units joined strand by strand
    // (join_units: voice by voice, channel by channel).
    struct Composition
    {
        std::unique_ptr<CompositionSpace> space;
        CompositionStacks stacks;
        std::unique_ptr<CompositionSieve> sieve; // null if its filters failed to build
        std::optional<Line> joined;
        uint32_t strands = 1;
    };
    std::array<Composition, kLines> comps_;
    const Composition& comp() const { return comps_[size_t(li_)]; }
    CompositionSpace::Parts composition_of(const Space::Digits& joined) const;
    std::unique_ptr<CompactLine> compact_[kLines]; // survivors in every ordering, where the stack can rank
    FilterMode modes_[kLines] = {}; // all Off
    std::unique_ptr<BookSpace> books_; // the books line
    std::unique_ptr<ModelSpace> model_space_; // the models line (Models above is Real Graphics)
    // The binary line: every file of 0..N bytes (SPECIFICATIONS §12.1, binary-v1). It has one
    // wall, so its files stand only in the left wall's half of each tile's slots; the loop counts
    // the right wall's slots as empty, and these two convert between a file's place on the line
    // and its place in the loop (the same number on every other line).
    std::unique_ptr<BinarySpace> binary_space_;
    // Its filters (binary-kind-v1, sieve/filekind.hpp), rebuilt when its length changes; null if
    // they failed to build.
    LineFilters binary_filters_;
    std::unique_ptr<BinarySieve> binary_sieve_;
    void rebuild_binary_sieve();
    // The models line's filters (not-a-file-v1), judged on a model's positional index.
    LineFilters model_filters_;
    std::unique_ptr<ModelSieve> model_sieve_;
    void rebuild_model_sieve();
    BigUint loop_pos(const BigUint& unit) const;   // unit index -> loop position
    BigUint unit_of_pos(const BigUint& pos) const; // loop position (left-wall slot) -> unit index
    // How many units the current line holds: its loop's count, except on binary (see above). A
    // reference to the space's own number (megabytes on a long binary line), so callers that only
    // read it copy nothing; it changes when the line, the mode or the filters do.
    const BigUint& line_units() const;
    // Every titled line's space (null for books and binary), built with the lines.
    std::array<std::unique_ptr<TitledSpace>, kLines> titled_;
    int book_page_ = 0;                // the page open in a book in hand
    Synth synth_;
};

} // namespace hallway::hall
