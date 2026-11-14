// Sieve — a character's voxels (IDEAS 19.214): the one way its spheres become cubes, for every space
// that draws a character as voxels (the Character Creator, the CHARACTER page, your personal dimension
// and the hallway), so the cubes are the same wherever they are seen.
//
// The cubes lie on a grid fixed to the character (they turn with it). A cell is filled where its middle
// lies in a sphere, and belongs to the sphere it lies deepest in, so the spheres run together into one
// blocky body. Only the faces between a filled cell and an empty one are given: a face inside the body
// can never be seen. Everything is in the character's own units (sieve::Sphere's), for the space that
// draws it to place, turn and light as it draws everything else.
#pragma once

#include "sieve/character.hpp"

#include <cstdint>
#include <vector>

namespace sieve {

// The six ways a face can face, in the order the faces of one cell are given: +x, -x, +y, -y, +z, -z.
inline constexpr int kVoxelDirs[6][3] = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};

struct VoxelFace
{
    uint32_t sphere = 0; // the sphere its cell belongs to (for its colour)
    uint8_t dir = 0;     // which way it faces: kVoxelDirs[dir]
    float corner[4][3];  // its corners, in order round it
    float mid[3];        // the middle of its cell (for its depth, as the Creator colours by depth)
    int32_t cell[3];     // its cell on the grid of the call that gave it (x, y, z), for joining faces
};

// One cell size for every character, so the cubes (and the pixels' squares) are alike from one character
// to the next (Edward, 14 November 2026: "square sizes need normalising"): its height in 16 cells, from
// the character at rest (so the cells hold still while it moves). Never less than one unit, and never so
// small that its widest side is more than kVoxelLatticeMostCells cells (below).
float voxel_cell(const Character& at_rest);

// How big a voxel is (IDEAS 19.216), [character] voxel_size and the rows beside it: today's rule (the
// character's height in 16 cells, above) or one lattice for the whole world, whose cells are powers of
// two of a base size, each character taking the coarsest level at which its thinnest part (its smallest
// sphere, across) is still `across` cells wide. On the lattice every character's cells are a doubling or
// a halving of every other's, and its grid is anchored at its own centre, so cells of one size line up.
struct VoxelSizing
{
    enum Rule : uint8_t { Height = 0, Lattice = 1 } rule = Height;
    uint32_t base_mm = 10; // the lattice's finest cell, in millimetres (1 to 100)
    uint32_t across = 2;   // the thinnest part in at least this many cells (1 to 8)
};
// So that a character cannot ask for a grid too large to hold, the lattice steps one level coarser while
// the character's widest side would be more than this many cells (one very small sphere), and the height
// rule's cell grows to fit it (a character far wider than it is tall).
inline constexpr uint32_t kVoxelLatticeMostCells = 256;

struct VoxelCell
{
    float cell = 1;      // in the character's units
    int level = 0;       // on the lattice: the cell is base_mm times 2^level (0 for the height rule)
    bool coarser = false; // made coarser than its rule asks (its thinnest part, or its height), to stay within the limit
};
VoxelCell voxel_cell_of(const Character& at_rest, const VoxelSizing& sizing);
inline float voxel_cell(const Character& at_rest, const VoxelSizing& sizing) { return voxel_cell_of(at_rest, sizing).cell; }

// Every face that can be seen of the spheres' cubes, cells taken z, then y, then x, and each cell's faces
// in kVoxelDirs' order (so a space that draws them in this order draws them as every other does). The
// grid starts at the spheres' lowest corner; `anchored` (the lattice) starts it instead at the multiple
// of the cell just below, so its cells lie at whole cells from the character's centre.
std::vector<VoxelFace> voxel_faces(const std::vector<Sphere>& spheres, float cell, bool anchored = false);

// Greedy meshing (IDEAS 19.215): faces that lie in one plane, face the same way and share a colour joined
// into as few rectangles as cover them: each layer of the grid swept row by row, a rectangle grown along
// its row as far as the faces match, then over as many following rows as match along its whole width.
// `faces` are any of one voxel_faces call's faces (those a space will draw), `colours` one for each (equal
// numbers may join). A rectangle's corners are its corner faces' own, in the same order round it, so it
// covers exactly the faces it stands for; `face` is the index of its first (for its sphere and colour).
struct VoxelQuad
{
    uint32_t face = 0;
    uint8_t dir = 0;
    float corner[4][3];
    uint32_t cells = 0; // how many faces it stands for
};
std::vector<VoxelQuad> join_voxel_faces(const std::vector<VoxelFace>& faces, const std::vector<uint32_t>& colours);

} // namespace sieve
