#include "world.hpp"

#include <cmath>

namespace hallway {

float book_height(int row, int col, bool varied)
{
    if (!varied) return kUniformBookHeight * shelf_scale();
    uint32_t h = uint32_t(row) * 2654435761u ^ uint32_t(col) * 40503u;
    h ^= h >> 13;
    h *= 0x5bd1e995u;
    h ^= h >> 15;
    return (0.34f + 0.12f * float(h % 1000u) / 1000.0f) * shelf_scale(); // 0.34 .. 0.46 m at a scale of one
}

void book_face(float tile_z, Side side, int row, int col, Vec3 out[4], bool varied)
{
    const float x = side == Side::Left ? -kCaseFront : kCaseFront;
    const float y0 = kRowTop - (row + 1) * kRowHeight + 0.02f; // sits on the shelf board
    const float y1 = y0 + book_height(row, col, varied);
    const float z0 = tile_z + col * book_pitch() + (book_pitch() - book_width()) * 0.5f;
    const float z1 = z0 + book_width();
    out[0] = {x, y0, z0};
    out[1] = {x, y0, z1};
    out[2] = {x, y1, z1};
    out[3] = {x, y1, z0};
}

std::optional<BookSlot> pick_book(Vec3 origin, Vec3 dir, int64_t player_tile, float reach, bool varied)
{
    if (std::fabs(dir.x) < 1e-6f) return std::nullopt;
    const Side side = dir.x < 0 ? Side::Left : Side::Right;
    const float plane = side == Side::Left ? -kCaseFront : kCaseFront;
    const float t = (plane - origin.x) / dir.x;
    if (t <= 0 || t > reach) return std::nullopt;
    const Vec3 hit = origin + dir * t;

    const float tile_f = std::floor(hit.z / kTile);
    const float zin = hit.z - tile_f * kTile;
    if (zin < 0 || zin >= kShelfEnd) return std::nullopt;
    const int col = int(zin / book_pitch());
    const float within = zin - col * book_pitch() - (book_pitch() - book_width()) * 0.5f;
    if (within < 0 || within > book_width()) return std::nullopt;

    const float from_top = kRowTop - hit.y;
    if (from_top < 0) return std::nullopt;
    const int row = int(from_top / kRowHeight);
    if (row < 0 || row >= kRows || col < 0 || col >= cols()) return std::nullopt;
    const float y0 = kRowTop - (row + 1) * kRowHeight + 0.02f;
    if (hit.y < y0 || hit.y > y0 + book_height(row, col, varied)) return std::nullopt;
    return BookSlot{player_tile + int64_t(tile_f), side, row, col};
}

} // namespace hallway
