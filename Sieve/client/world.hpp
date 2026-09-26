// Sieve hallway — the corridor tile: bookcases, doors and books, and picking a book by ray.
//
// One tile is drawn once and repeated along +z forever. Each tile holds kBooksPerTile consecutive
// book slots (128, a power of two; see sieve/corridor.hpp): first the left wall, then the right
// wall; on each wall shelf by shelf from the top, and along each shelf in +z order. So the
// address always increases as you walk forward.
//
//   z:  0 ........ 6.0 | 6.4 door 7.6 | 8.0 (next tile)
//       bookcase         black door
#pragma once

#include "camera.hpp"

#include "sieve/corridor.hpp"

#include <cstdint>
#include <optional>

namespace hallway {

inline constexpr float kHalfWidth = 2.0f;   // walls at x = -2 and +2
inline constexpr float kHeight = 3.0f;
inline constexpr float kCaseFront = 1.65f;  // bookcase faces at x = -1.65 and +1.65
inline constexpr float kTile = 8.0f;
inline constexpr float kShelfEnd = 6.0f;    // bookcase spans z 0..6 of each tile
inline constexpr float kDoorStart = 6.4f, kDoorEnd = 7.6f, kDoorTop = 2.2f;
inline constexpr int kRows = 4;
inline constexpr float kRowTop = 2.65f, kRowHeight = 0.55f;
// What stands in a slot is scaled to the slot, uniformly. Scaling only along the shelf would
// squash a record or a cassette; a record is round whatever else changes, so everything that
// goes on a shelf -- a book, a sheet, a canvas, a cassette, a crate -- keeps its proportions and
// simply gets smaller. A sixteen-column shelf is the shape everything was drawn for, so that is
// the scale of one; a thirty-two column shelf is half of it. Never larger than one: a wider slot
// is left as air, because an item grown past the row it stands in would burst out of the case.
inline constexpr float kBookWidth = 0.28f;  // at a scale of one
inline constexpr float kBasePitch = kShelfEnd / 16;
// The shelves hold exactly one corridor tile, split between the two walls and then between four
// rows, so how many stand along a row follows from the corridor's tile size (sieve/corridor.hpp)
// rather than being fixed here. At 128 to a tile that is 16 to a row, as it always was; at 256
// it is 32, and they are packed closer together along the same shelf.
inline int books_per_wall() { return int(sieve::books_per_tile() / 2); }
inline int cols() { return books_per_wall() / kRows; }
inline float book_pitch() { return kShelfEnd / float(cols()); }
inline float shelf_scale() { return book_pitch() < kBasePitch ? book_pitch() / kBasePitch : 1.0f; }
inline float book_width() { return kBookWidth * shelf_scale(); }
inline constexpr float kWalkLimit = 1.45f;  // how close to a wall you may walk, except in a doorway

// The binary line: the seventh line, and the one the other six are bounded by.
//
//     binary | pages  image  audio  video  books  models | binary
//
// It is exactly the same space as any other tile of corridor — the same width, the same height,
// the same bookcase — with one difference: it has only one side. One wall carries the shelves; in
// place of the other the floor simply ends, at a short wall no higher than your waist, and past
// that there is nothing. Green rain falls off that edge for ever.
//
// It is where the looping stops. The six lines used to wrap into each other through the doors;
// now they start and finish here. Its one wall carries its one door, and which side of it you see
// the drop on depends on which end of the corridor you walked out of, because it is one line met
// from either end. Nothing out there is addressed, ordered or filtered, and the shelves stand
// empty because cataloguing what is in the drop is a job for people.
inline constexpr float kEdgeRail = kHalfWidth;  // the floor ends where the missing wall would be
inline constexpr float kEdgeRailTop = 0.95f;    // how high the short wall standing on it is
inline constexpr float kEdgeDrop = 26.0f;       // how far the rain falls past the floor

enum class Side { Left = 0, Right = 1 };

struct BookSlot
{
    int64_t tile; // tile, relative to the tile the player is in
    Side side;
    int row, col;
    uint32_t slot() const { return uint32_t(int(side) * books_per_wall() + row * cols() + col); } // within the tile
    static BookSlot of(int64_t tile, uint32_t slot)
    {
        return {tile, slot < uint32_t(books_per_wall()) ? Side::Left : Side::Right,
                int(slot % uint32_t(books_per_wall())) / cols(), int(slot) % cols()};
    }
};

// The kinds of media on the shelves, one per line.
enum class Media { Pages, Image, Audio, Video, Books, Models };

// Whether a line's books vary in size from slot to slot. Pages, pictures (canvases) and books do;
// audio (a record), video (a tape) and models (a crate, all the same box until you open it) must
// not, since the real things come in one size. This is the one switch for it: the wireframe
// follows it, and so must any Real Graphics models.
constexpr bool media_sizes_vary(Media m) { return m != Media::Audio && m != Media::Video && m != Media::Models; }

// Height of a book's spine: varies a little per slot so shelves do not look like a grid, unless
// `varied` is false (then every book is kUniformBookHeight).
inline constexpr float kUniformBookHeight = 0.40f;
float book_height(int row, int col, bool varied = true);

// The four corners of a book's spine (the face towards the corridor) in world space, for a tile
// whose start is at z = tile_z.
void book_face(float tile_z, Side side, int row, int col, Vec3 out[4], bool varied = true);

// The book the ray hits within `reach` metres, if any. `cam_z` is local to the player's tile and
// `player_tile` is that tile's index.
std::optional<BookSlot> pick_book(Vec3 origin, Vec3 dir, int64_t player_tile, float reach, bool varied = true);

} // namespace hallway
