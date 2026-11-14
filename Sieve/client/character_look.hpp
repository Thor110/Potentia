// Sieve hallway — how characters are drawn (IDEAS 19.214): the one setting ([character] drawing, on
// Settings > Graphics and in the Character Creator alike) and the one way a space seen with perspective
// (your personal dimension, the hallway) draws a character as it says. The Creator and the CHARACTER page
// draw square-on with the same renderer and the same cubes (sieve/character_voxels.hpp).
#pragma once

#include "camera.hpp"
#include "mesh.hpp"
#include "sieve/character.hpp"
#include "sieve/character_skin.hpp"
#include "sieve/character_voxels.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace hallway {

// The drawings, as [character] drawing numbers them. Shaded is the default: every picture taken before
// the choice reached every space was shaded there, so it keeps them as they were. The skin (IDEAS 19.225)
// came fifth, so the four before it keep their numbers, and the skin's quads and its smooth drawing
// (IDEAS 19.214) sixth and seventh after it; kDrawCount is how many there are, for every row that steps
// through them (the Creator's and Settings > Graphics').
enum CharacterDrawing : int { kDrawPixels = 0, kDrawWireframe = 1, kDrawVoxels = 2, kDrawShaded = 3, kDrawSkin = 4, kDrawQuads = 5, kDrawSmooth = 6 };
inline constexpr int kDrawDefault = kDrawShaded;
inline constexpr int kDrawCount = 7;

// The three drawings of the skin: its triangles, its quads, and its triangles lit smoothly.
inline bool skin_drawing(int drawing) { return drawing == kDrawSkin || drawing == kDrawQuads || drawing == kDrawSmooth; }

// Its name for readouts: "pixels", "wireframe", "voxels", "shaded", "skin", "quads" or "smooth".
const char* character_drawing_name(int drawing);

// The drawing a space uses: the one chosen with Real Graphics on, the wireframe with it off (as
// everything else is then drawn).
inline int character_drawing_shown(int chosen, bool real_graphics) { return real_graphics ? chosen : int(kDrawWireframe); }

// How big the voxels are and whether their faces are joined ([character] voxel_size, voxel_base_mm,
// voxel_across and voxel_joined; IDEAS 19.216, 19.215), one for every space as the drawing is.
struct VoxelChoice
{
    sieve::VoxelSizing sizing;
    bool joined = false;
    // The skin's (IDEAS 19.225; [character] skin_blend and skin_detail): how full it fills the creases
    // between spheres, as a percentage of their mean radius, and how many times the voxels' cell is halved
    // for its grid. Used only when the skin is drawn.
    uint32_t skin_blend = 50;
    uint32_t skin_detail = 1;
    // The cell for a character at rest, by the rule chosen (its height in 16 cells by default).
    float cell(const sieve::Character& at_rest) const { return sieve::voxel_cell(at_rest, sizing); }
    bool anchored() const { return sizing.rule == sieve::VoxelSizing::Lattice; }
    // Either differs from what every space drew before the choice (so a readout may say so).
    bool chosen() const { return joined || anchored(); }
};

struct AppSettings;
// The choice as the settings file has it (one reading of the four settings for every space).
VoxelChoice voxel_choice_of(const AppSettings& app);

// What a space says of its characters' drawing in its readout, said only where it differs from what the
// space drew before the choice reached it (so a readout that does not use the choice is as it was):
// empty for shaded with Real Graphics on. For voxels with the lattice or joining chosen, also the cubes'
// triangles before joining (`unjoined`) and the rule.
std::string character_drawing_report(int chosen, bool real_graphics, size_t triangles, const VoxelChoice& voxels = {}, size_t unjoined = 0);

// What one skin cost to make, for a readout (the Creator's): its cell and blend in the character's units,
// its grid, the triangles made and drawn, and the time it took.
struct SkinReport
{
    float cell = 0, blend = 0;
    bool coarser = false;
    uint32_t grid[3] = {0, 0, 0};
    size_t made = 0, drawn = 0; // triangles (for the quads, quads)
    double ms = 0;     // the time it took to make (when it was made, if it was kept from a frame before)
    bool kept = false; // kept from a frame before, so made no time in this one
    double smooth_ms = 0; // the smooth drawing's: the time its normals took to average, in this frame
    // `drawing` says which of the skin's three drawings it was: the skin's line is as it was before the
    // other two came; the quads' counts quads; the smooth drawing's adds the normals' time.
    std::string line(const VoxelChoice& voxels, int drawing = kDrawSkin) const;
};

// The skin over these spheres (posed or at rest), made as the choice says: its cell the voxels' cell
// (`voxel_cell`, VoxelChoice::cell of the character at rest) halved skin_detail times, its blend
// skin_blend percent of the spheres' mean radius. The one way every space makes it, the Creator's
// square-on drawing and add_character_drawing alike. The last few skins made are kept, each with what it
// was made from, so a character standing still (or one seen in several frames in the same pose) is made
// once, not every frame; `report`, where given, says what it cost to make and whether it was kept.
std::shared_ptr<const sieve::SkinMesh> make_skin(const std::vector<sieve::Sphere>& spheres, float voxel_cell, const VoxelChoice& voxels, SkinReport* report = nullptr);
// Said by a space before it draws its characters: how many it draws in this frame, so that make_skin keeps
// at least twice that many skins (never fewer than eight; the number kept only grows) and a character
// standing still is not made again however many are in sight.
void skins_kept_for(size_t characters_in_a_frame);

// One skin triangle's normal for its lighting (its corners' normals averaged, of length one) and its
// sphere for its colour (the one two or more of its corners share, else its first corner's), in the
// character's own units.
void skin_triangle_look(const sieve::SkinMesh& skin, size_t triangle, float normal[3], uint32_t& sphere);

// One skin quad's normal (its four corners' normals averaged, of length one) and its sphere (the one most
// of its corners share; where none is shared by more than the others, its first corner's), as
// skin_triangle_look gives a triangle's.
void skin_quad_look(const sieve::SkinMesh& skin, size_t quad, float normal[3], uint32_t& sphere);

// Where one space puts a skin and how it lights it, for add_skin_drawing: the one way the skin's three
// drawings are drawn, whether into a batch begun with perspective (your dimension, the hallway) or
// square-on (the Creator and the CHARACTER page).
struct SkinPlacing
{
    bool world = true; // points given to the batch in the world (add_face), else in camera space (add_face_cam)
    // A point and a direction of the character's own, as the batch takes them.
    std::function<Vec3(float, float, float)> point, direction;
    // Whether a face is turned away from the eye: `g` across it (outwards), `centre` its middle.
    std::function<bool(Vec3 g, Vec3 centre)> turned_away;
    // A normal in camera space (x right, y up, z away), for the light: `n` at the point `at`.
    std::function<Vec3(Vec3 at, Vec3 n)> cam_normal;
    // The colour of a sphere's skin lit by Lambert's cosine `lambert`, at the corners given (one for a
    // vertex, three for a triangle, four for a quad), as the space colours its characters.
    std::function<uint32_t(uint32_t sphere, float lambert, const Vec3* corners, int count)> colour;
};

// A skin drawn into the batch as `drawing` says (IDEAS 19.225, 19.214): the skin, each triangle turned
// towards the eye one colour; the quads, each four-sided face surface nets made turned towards the eye one
// colour (as its two triangles) with its four edges drawn darker over it, so the faces read as quads; the
// smooth drawing, the skin's triangles with each vertex lit by its normal averaged across the faces round
// it (sieve::skin_smooth_normals) and coloured as its own sphere, the colours running smoothly across each
// face. Returns the faces given: triangles, or for the quads quads. `smooth_ms`, where given, has the time
// the normals took added to it.
size_t add_skin_drawing(MeshBatch& batch, int drawing, const sieve::SkinMesh& skin, const SkinPlacing& at, double* smooth_ms = nullptr);

// The shaded drawing's light from the upper left, towards the eye, for a normal in camera space (x right,
// y up, z away): Lambert's cosine, nought where it faces away.
inline float skin_lambert(Vec3 cam_normal) { return std::max(0.0f, dot(cam_normal, Vec3{-0.45f, 0.55f, -0.70f})); }

// A line for a readout of the voxels' cell: "80.3 units (its height in 16 cells)" or "81.9 units (the
// lattice: 20 mm, level 1)", with ", made coarser to fit" where the limit stepped in.
std::string voxel_cell_report(const sieve::Character& at_rest, const VoxelChoice& voxels);

// Where a character stands in a space: a point of its own (in its units) to the world, and a direction
// of its own to the world's (a unit vector stays one).
struct CharacterPlacing
{
    std::function<Vec3(float, float, float)> point;
    std::function<Vec3(float, float, float)> direction;
    float metres = 1; // world metres a unit of its own, as it is drawn here
};

// One character's spheres drawn into a batch begun with perspective, as `drawing` says: each sphere's
// look comes from `look_of` (its colour, and for shaded its ambient light and least size), its style set
// here (shaded as given, rings, the pixels' squares, or the cubes in place of the spheres). `cell` is
// the voxels' and pixels' cell in the character's units (VoxelChoice::cell); `voxels` says whether the
// grid is the lattice's and whether faces are joined. The bones are the space's own to draw. Returns the
// triangles given for the cubes (two a face or a joined rectangle) or for the skin (those turned towards
// the eye; for the quads, the quads), 0 for the others; `unjoined`, where given, has added to it the
// triangles the cubes would have been with each face on its own. The skin (IDEAS 19.225), its quads and
// its smooth drawing (IDEAS 19.214, by add_skin_drawing) are made from the spheres as given, so a posed
// character's skin follows its joints; each lit as the shaded spheres are (Lambert's cosine from
// the upper left over the look's ambient light) and coloured as its nearest sphere's look.
size_t add_character_drawing(MeshBatch& batch, const Camera& cam, int drawing, const std::vector<sieve::Sphere>& spheres, float cell,
                             const CharacterPlacing& at, const std::function<MeshBatch::SphereLook(const sieve::Sphere&)>& look_of,
                             const VoxelChoice& voxels = {}, size_t* unjoined = nullptr);

} // namespace hallway
