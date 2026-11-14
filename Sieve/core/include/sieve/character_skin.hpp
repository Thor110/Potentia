// Sieve — a character's skin (IDEAS 19.225): one surface wrapped over its spheres, smooth where they
// meet, the mesh scaling system's stage after the voxels. The one way every space makes it (the
// Character Creator, the CHARACTER page, your personal dimension and the hallway), so the skin is the
// same wherever it is seen.
//
// The spheres are taken as a field: at any point, each sphere's distance from its surface (negative
// inside), the distances joined by a smooth minimum taken in the spheres' order. Where two spheres are
// further apart than the blend, the field is the nearer one's distance, so the skin lies on the sphere;
// where they come within the blend of each other, the smooth minimum fills the crease between them, the
// wider the blend the fuller (0 is the plain union, every crease kept). The skin is where the field is
// nought, made into triangles by surface nets: the field worked out at the corners of a grid, one vertex
// in every cell the surface passes through (the average of where it crosses the cell's edges), and one
// four-sided face, split in two, across every grid edge it crosses. Each vertex keeps the way the field
// rises there (its normal, for lighting) and the sphere nearest it (for its colour). Everything is in the
// character's own units (sieve::Sphere's), as the voxels are, for the space that draws it to place.
#pragma once

#include "sieve/character.hpp"

#include <cstdint>
#include <vector>

namespace sieve {

struct SkinVertex
{
    float p[3];          // where it is
    float n[3];          // its normal: the way the field rises there, outwards, of length one
    uint32_t sphere = 0; // the sphere whose surface is nearest it (for its colour)
};

struct SkinMesh
{
    std::vector<SkinVertex> vertices;
    std::vector<uint32_t> tris; // three vertices a triangle, wound anticlockwise seen from outside
    // The same surface as the four-sided faces surface nets makes it (IDEAS 19.214's quads): four vertices
    // a face, wound as the triangles are, each face the two triangles at the same place in `tris` (face q
    // is triangles 2q and 2q + 1), so a drawing of quads and one of triangles are of one surface.
    std::vector<uint32_t> quads;
    uint32_t grid[3] = {0, 0, 0}; // the grid's corners along x, y and z, for a readout
    size_t triangles() const { return tris.size() / 3; }
    size_t quad_count() const { return quads.size() / 4; }
};

// The blend in the character's units: `percent` of its spheres' mean radius, so one setting gives every
// character a skin as tight for its size (Settings > Graphics > Character Voxels, Skin Blend). The radii
// do not change as a character moves, so its posed spheres give the same as its spheres at rest.
float skin_blend(const std::vector<Sphere>& spheres, uint32_t percent);

// The skin's cell: the voxels' cell (sieve::voxel_cell_of's, so the skin is as fine as the voxels by the
// same rule) halved `halvings` times (Skin Detail, 0 to 3), and doubled again while the widest side of
// the spheres' box would be more than kSkinMostCells cells, so that a fine lattice cannot ask for a grid
// too large to make in a frame.
inline constexpr uint32_t kSkinMostCells = 160;
struct SkinCell
{
    float cell = 1;
    bool coarser = false; // made coarser than asked, to stay within the limit
};
SkinCell skin_cell(const std::vector<Sphere>& spheres, float voxel_cell, uint32_t halvings);

// The skin over these spheres at this cell and blend (both in the character's units). The grid's corners
// lie at whole cells from the character's centre, so as it moves the grid holds still on it and the skin
// does not shimmer. Empty for no spheres or no cell.
SkinMesh skin_mesh(const std::vector<Sphere>& spheres, float cell, float blend);

// Each vertex's normal averaged across the faces round it (IDEAS 19.214's smooth drawing): the sum of
// its triangles' normals, each as long as twice its triangle's area so a large face counts for more, made
// of length one; three floats a vertex. The field's own slope (SkinVertex::n) says where the spheres'
// surface faces; this says where the skin as made faces, so the light follows the faces that are drawn
// (on one sphere the field's slope is the nearer to the sphere's true normal: the unit tests measure
// both). A vertex no triangle uses keeps its field normal.
std::vector<float> skin_smooth_normals(const SkinMesh& skin);

} // namespace sieve
