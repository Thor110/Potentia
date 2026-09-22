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
inline constexpr int kCols = 16;
inline constexpr float kRowTop = 2.65f, kRowHeight = 0.55f;
inline constexpr float kBookPitch = kShelfEnd / kCols, kBookWidth = 0.28f;
inline constexpr int kBooksPerWall = kRows * kCols;
using sieve::kBooksPerTile; // 128, defined by the corridor
// The shelves must hold exactly the corridor's tile, which must be a power of two, so that
// power-of-two line sizes fill whole tiles and their loops nest (sieve/corridor.hpp).
static_assert(2 * kBooksPerWall == int(kBooksPerTile), "the shelves must hold exactly one corridor tile");
static_assert((kBooksPerTile & (kBooksPerTile - 1)) == 0, "books per tile must be a power of two");
inline constexpr float kWalkLimit = 1.45f;  // how close to a wall you may walk, except in a doorway

enum class Side { Left = 0, Right = 1 };

struct BookSlot
{
    int64_t tile; // tile, relative to the tile the player is in
    Side side;
    int row, col;
    uint32_t slot() const { return uint32_t(int(side) * kBooksPerWall + row * kCols + col); } // 0..127 within the tile
    static BookSlot of(int64_t tile, uint32_t slot)
    {
        return {tile, slot < uint32_t(kBooksPerWall) ? Side::Left : Side::Right, int(slot % kBooksPerWall) / kCols, int(slot % kCols)};
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
