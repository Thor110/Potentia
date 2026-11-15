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

// A limb (IDEAS 19.229, the pipeline's first stage): one bone, a sphere and its parent, as a rounded cone
// in the field, so the skin runs along the bone rather than being a row of balls apart where they do not
// touch. The cone runs from the parent's centre to the child's, the child's end as wide as the child's
// sphere, so it is the whole of the child's sphere and the field takes it in that sphere's place (each
// sphere's surface is then in the field once, and the smooth minimum adds nothing where it would only meet
// itself). The parent's end is set back inside the parent's sphere by the blend (by at most half the
// parent's radius), so that where the limb leaves the parent the smooth minimum fills the shoulder as it
// fills any crease, and does not swell the parent where the cone would lie on it. With no blend the cone
// is the two spheres' hull exactly.
struct SkinLimb
{
    uint32_t parent = 0, child = 0; // the spheres it joins
    float a[3] = {0, 0, 0}, ra = 0; // the parent's end: its centre, and the cone's radius there
    float b[3] = {0, 0, 0}, rb = 0; // the child's end: its centre and radius
};

// The limbs of these spheres (one for every sphere with a parent, in the spheres' order) at this blend.
// The stage's switch is whether it is used: skin_mesh with no limbs is the skin of the spheres alone,
// exactly as it was before limbs came.
std::vector<SkinLimb> skin_limbs(const std::vector<Sphere>& spheres, float blend);

// The exact signed distance from p to a limb's rounded cone (negative inside): the distance to the surface
// swept by a sphere moving from one end to the other, its radius running straight from one to the other.
// Where one end's sphere holds the other's, it is that sphere's distance.
float skin_limb_distance(const SkinLimb& limb, const float p[3]);

// The skin over these spheres at this cell and blend (both in the character's units), with these limbs in
// its field (none: the spheres alone). The grid's corners lie at whole cells from the character's centre,
// so as it moves the grid holds still on it and the skin does not shimmer. Empty for no spheres or no cell.
SkinMesh skin_mesh(const std::vector<Sphere>& spheres, float cell, float blend, const std::vector<SkinLimb>& limbs = {});

// Each vertex's normal averaged across the faces round it (IDEAS 19.214's smooth drawing): the sum of
// its triangles' normals, each as long as twice its triangle's area so a large face counts for more, made
// of length one; three floats a vertex. The field's own slope (SkinVertex::n) says where the spheres'
// surface faces; this says where the skin as made faces, so the light follows the faces that are drawn
// (on one sphere the field's slope is the nearer to the sphere's true normal: the unit tests measure
// both). A vertex no triangle uses keeps its field normal.
std::vector<float> skin_smooth_normals(const SkinMesh& skin);

// Smooth plus, the surface (IDEAS 19.229, the pipeline's second stage): the skin's corners moved towards
// the average of their neighbours a round at a time, by Taubin's method, so the bumps where spheres and
// limbs meet are eased without the shape shrinking as plain averaging would shrink it. Each round is two
// steps over every vertex at once: a shrink, each corner moved `lambda` of the way to its neighbours'
// average, then an inflate, moved `mu` of the way (mu negative and larger than lambda, so it moves out a
// little further than the shrink moved in; the two together pass the broad shape and damp the fine
// ripples). A vertex's neighbours are the corners it shares a side of a quad with (the surface nets'
// own grid, four or so each; the triangles' sides where the skin has no quads), so the diagonal each quad
// was split along does not pull harder one way than the other. Only where the vertices are changes: the
// triangles, quads, colours and grid are the skin's, so the mesh stays closed with the same faces. Each
// vertex's normal is then the faces' round it (as skin_smooth_normals gives it), since the field's slope
// no longer says where the moved surface faces. No rounds gives the skin exactly as it came.
inline constexpr float kTaubinShrink = 0.5f;
inline constexpr float kTaubinInflate = -0.53f;
SkinMesh skin_taubin(const SkinMesh& skin, uint32_t rounds, float lambda = kTaubinShrink, float mu = kTaubinInflate);

// The volume a closed skin holds, in the character's units cubed (the sum over its triangles of the
// signed volume each makes with the origin, which for a closed surface wound outwards is what it holds,
// wherever the origin is): how the smoothing is measured not to shrink the shape.
double skin_volume(const SkinMesh& skin);

// Smooth plus, the outline (IDEAS 19.229, the pipeline's third stage): the skin's faces subdivided by
// Catmull and Clark's rule `steps` times, so the outline is round as well as the light (the smoothing
// before it moves the corners, but a silhouette drawn with the skin's own faces still shows their
// edges). Each step puts a point in every face (its corners' average), one on every edge (the average of
// its two ends and the points of the faces either side of it) and moves every corner to (F + 2R + (n - 3)P)
// / n, where P is where it stood, F the average of the points of the n faces round it and R of the middles
// of its n edges; each face of k corners then becomes k quads, one at each corner. So quads stay quads,
// four for each, and a skin given as triangles alone (with no quads) is taken by the rule's general form,
// each triangle three quads at the first step. An edge between one face is an outline: its point is its
// middle, and an end with two such edges moves as a curve does (six eighths of itself and one of each
// neighbour). Each new quad is split into two triangles along its shorter diagonal and kept as skin_mesh
// keeps them (face q the triangles 2q and 2q + 1); each new corner's sphere is its own for the old
// corners, its two ends' for an edge's point where they share one (else the lower-numbered end's, so
// the choice is the same every time), and the one most of its corners share for a face's point; its normal the faces' round it, as
// skin_smooth_normals gives it. No steps gives the skin exactly as it came.
SkinMesh skin_subdivide(const SkinMesh& skin, uint32_t steps);

// The faces a step makes: each face of k corners becomes k quads. Each step after the first multiplies
// the quads by four.
size_t skin_subdivided_quads(const SkinMesh& skin, uint32_t steps);

// The steps that may be taken of those asked without passing `most_quads` quads: each step multiplies the
// faces by about four, so a fine skin subdivided three times could ask for more than the renderer can draw
// in a frame. The steps taken are the most of those asked whose quads stay within the bound; the readout
// says when fewer were taken than asked.
inline constexpr uint32_t kSkinMostSubdivisionSteps = 3;
inline constexpr size_t kSkinMostSubdividedQuads = 200000;
uint32_t skin_subdivision_steps(const SkinMesh& skin, uint32_t asked, size_t most_quads = kSkinMostSubdividedQuads);

} // namespace sieve
