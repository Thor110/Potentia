// Sieve hallway — Real Graphics: the models that replace the wireframe when the option is on.
//
// Four models, one set per line (see data/meshes/templates/README.md):
//   hallway    one 8 m tile of corridor, doors included
//   bookshelf  the left wall's bookcase (the right wall's is its mirror image)
//   book       one slot's book, spine towards the corridor, 0.40 m tall
//   marker     the checkered start/finish strip on the floor
// For line M the file is <model>-<M>.obj in the meshes folder (data/meshes while developing,
// meshes/ next to the executable); if it is missing, templates/<model>.obj; if that is missing
// too, that part stays wireframe. Only what Wavefront .obj/.mtl files need for flat colour is
// read: v, vn, f (any polygon, split into a fan), mtllib, usemtl, newmtl and Kd.
//
// Drawing: SDL's renderer has no depth buffer, so every visible triangle is shaded, sorted far to
// near and drawn in one batch (the painter's algorithm). Faces turned away from the camera are
// skipped, and colour fades into the line's background with distance, like the wireframe.
#pragma once

#include "camera.hpp"

#include <SDL3/SDL.h>

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace hallway {

struct MeshTri
{
    Vec3 p[3];
    Vec3 n;         // unit normal, from the face's winding or its vertex normals
    SDL_FColor kd;  // its material's colour
};

struct Mesh
{
    std::vector<MeshTri> tris;
    Vec3 lo{1e9f, 1e9f, 1e9f}, hi{-1e9f, -1e9f, -1e9f}; // bounds
    std::string source;                                  // the file it came from
};

// Loads an .obj (and the .mtl it names). Returns null if the file cannot be read or has no faces.
std::shared_ptr<const Mesh> load_obj(const std::filesystem::path& path);

// The model for a line: <model>-<medium>.obj, else the template, else null (stays wireframe).
std::shared_ptr<const Mesh> load_model(const std::string& model, const std::string& medium);

// The mesh cut into slabs `pitch` metres long along Z (at z = k * pitch), so that sorting far to
// near works where parts overlap on screen: a 6 m shelf board sorts slot by slot against the
// books standing on it. Only faces that cross a slab boundary are cut, and only there.
std::shared_ptr<const Mesh> slice_z(const Mesh& mesh, float pitch);

// A cheap stand-in for a distant copy: only the faces turned towards +X (a book's spine, the
// side you see from the corridor, for the left wall) and up (its top).
std::shared_ptr<const Mesh> facing_x(const Mesh& mesh);
std::filesystem::path mesh_folder();

// Where and how one copy of a mesh is drawn: scaled in Y, mirrored in X or not, then moved.
struct Placement
{
    Vec3 offset;
    float scale_y = 1;
    bool mirror_x = false;
    float dim = 0; // 0 = full colour, 1 = the background (like the wireframe's mark mode)
};

// Collects triangles from many meshes, then draws them sorted far to near. The hallway, the
// markers and the shelves with their books go in three batches, drawn in that order: from inside
// the corridor its floor, walls and ceiling can never hide anything, and a marker lies on the floor.
class MeshBatch
{
public:
    void begin(const Camera& cam, SDL_Color background, int width, int height);
    void add(const Mesh& mesh, const Placement& at);
    void draw(SDL_Renderer* r);
    size_t triangles() const { return tris_.size(); }
    size_t drawn() const { return drawn_; } // triangles in the last draw

private:
    struct Tri
    {
        float depth;
        SDL_Vertex v[3];
    };
    void emit(const std::vector<Point2>& poly, float depth, SDL_FColor c);
    const Camera* cam_ = nullptr;
    SDL_FColor bg_{};
    float w_ = 1, h_ = 1;
    std::vector<Tri> tris_;
    std::vector<std::pair<float, uint32_t>> order_; // depth, index: sorting these is cheaper than the triangles
    std::vector<SDL_Vertex> verts_;
    size_t drawn_ = 0;
};

} // namespace hallway
