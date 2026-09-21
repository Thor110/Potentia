// Sieve hallway — the corridor tile: bookcases, doors and books, and picking a book by ray.
//
// One tile is drawn once and repeated along +z forever. Each tile holds kBooksPerTile consecutive
// units: first the left wall, then the right wall; on each wall shelf by shelf from the top, and
// along each shelf in +z order. So the address always increases as you walk forward.
//
//   z:  0 ........ 6.0 | 6.4 door 7.6 | 8.0 (next tile)
//       bookcase         black door
#pragma once

#include "camera.hpp"

#include <cstdint>
#include <optional>

namespace hallway {

inline constexpr float kHalfWidth = 2.0f;   // walls at x = -2 and +2
inline constexpr float kHeight = 3.0f;
inline constexpr float kCaseFront = 1.65f;  // bookcase faces at x = -1.65 and +1.65
inline constexpr float kTile = 8.0f;
inline constexpr float kShelfEnd = 6.0f;    // bookcase spans z 0..6 of each tile
inline constexpr float kDoorStart = 6.4f, kDoorEnd = 7.6f, kDoorTop = 2.2f;
inline constexpr int kRows = 5;
inline constexpr int kCols = 20;
inline constexpr float kRowTop = 2.65f, kRowHeight = 0.48f;
inline constexpr float kBookPitch = kShelfEnd / kCols, kBookWidth = 0.26f;
inline constexpr int kBooksPerWall = kRows * kCols;
inline constexpr int kBooksPerTile = 2 * kBooksPerWall;
inline constexpr float kWalkLimit = 1.45f;  // how close to a wall you may walk, except in a doorway

enum class Side { Left = 0, Right = 1 };

struct BookSlot
{
    int64_t tile; // tile index, relative to the walk's base tile
    Side side;
    int row, col;
    int64_t offset() const { return tile * kBooksPerTile + int64_t(side) * kBooksPerWall + row * kCols + col; }
};

// Height of a book's spine: varies a little per slot so shelves do not look like a grid.
float book_height(int row, int col);

// The four corners of a book's spine (the face towards the corridor) in world space, for a tile
// whose start is at z = tile_z.
void book_face(float tile_z, Side side, int row, int col, Vec3 out[4]);

// The book the ray hits within `reach` metres, if any. `cam_z` is local to the player's tile and
// `player_tile` is that tile's index.
std::optional<BookSlot> pick_book(Vec3 origin, Vec3 dir, int64_t player_tile, float reach);

} // namespace hallway
