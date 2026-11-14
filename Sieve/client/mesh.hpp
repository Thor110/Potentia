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
// Drawing: see MeshBatch, a small depth-buffered rasteriser (SDL's renderer keeps no depth).
#pragma once

#include "camera.hpp"
#include "sieve/image.hpp" // RgbaImage: a picture on a face

#include <SDL3/SDL.h>

#include <cstdint>
#include <filesystem>
#include <functional>
#include <istream>
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
// The same from text already in memory (a member of a .sieve, say): `mtl_text` gives the text of a
// material library it names, empty if there is none. `source` is what the mesh says it came from.
std::shared_ptr<const Mesh> read_obj(std::istream& in, const std::string& source, const std::function<std::string(const std::string&)>& mtl_text);

// The model for a line: <model>-<medium>.obj, else the template, else null (stays wireframe).
std::shared_ptr<const Mesh> load_model(const std::string& model, const std::string& medium);

// Where the pre-rendered picture goes on a line's item, in the item model's own metres: the front
// faces the corridor at x = 0, y is up from the shelf board, and the picture is centred across the
// slot, half_width either side. Read from faces.ini beside the meshes, one section per medium;
// anything missing is the whole 0.28 x 0.40 m slot.
struct FaceRect
{
    float bottom = 0.0f, top = 0.40f, half_width = 0.14f;
    float aspect() const { return (top - bottom) / (2.0f * half_width); } // height over width
};
FaceRect load_face_rect(const std::string& medium);

// A cheap stand-in for a distant copy: only the faces turned towards +X (a book's spine, the
// side you see from the corridor, for the left wall) and up (its top).
std::shared_ptr<const Mesh> facing_x(const Mesh& mesh);

// A corridor mesh with one side removed, for the binary line's one-sided tile: `sign` is which
// side goes, `at` how far out a triangle must stand to count as being in that wall.
std::shared_ptr<const Mesh> half_x(const Mesh& mesh, float sign, float at);

// A solid box (the short wall on the binary line's open edge).
std::shared_ptr<const Mesh> box_mesh(Vec3 lo, Vec3 hi, SDL_FColor kd);
std::filesystem::path mesh_folder();

// Where and how one copy of a mesh is drawn: scaled all over, then in Y, mirrored in X or not,
// then moved. `scale` keeps a thing's proportions -- it is how an item is fitted to a smaller
// slot when a tile holds more of them -- where `scale_y` deliberately does not, since a book's
// height varies from slot to slot but its width and depth do not.
struct Placement
{
    Vec3 offset;
    float scale = 1;
    float scale_y = 1;
    bool mirror_x = false;
    float dim = 0;        // 0 = full colour, 1 = the background (like the wireframe's mark mode)
    float depth_bias = 0; // pulls it this fraction nearer, so a decal on a surface wins (the marker)
};

// A depth-buffered rasteriser for the models. SDL's renderer draws triangles but keeps no depth,
// so sorting them far to near can go wrong where parts are close (a book and the board it stands
// on). This draws them itself, keeping each pixel's nearest depth, into an image that is then
// shown with one texture. Faces turned away from the camera are skipped; every face has one
// colour, lit from the eye and fading into the background with distance like the wireframe.
// The screen is cut into bands drawn by worker threads.
class MeshBatch
{
public:
    MeshBatch();
    ~MeshBatch();
    MeshBatch(const MeshBatch&) = delete;
    MeshBatch& operator=(const MeshBatch&) = delete;

    void begin(const Camera& cam, SDL_Color background, int width, int height);
    // As begin, but what is drawn is laid over what is on the screen already (the ground and sky of
    // your dimension): where nothing is drawn, the image is clear (IDEAS 19.207: one renderer for
    // every space).
    void begin_overlay(const Camera& cam, int width, int height);
    void add(const Mesh& mesh, const Placement& at);
    // One face, already placed in the world and coloured by its owner (who lights it as it likes):
    // clipped to the near plane, depth-tested at every pixel. Whether it faces the eye is the
    // owner's to say (a two-sided face is simply always given).
    void add_face(const Vec3 world[3], uint32_t argb);
    // A picture on a flat four-cornered face (corners top left, top right, bottom right, bottom
    // left as seen), its pixels read with perspective. Left out where any corner is behind the eye.
    // The picture must outlive draw().
    void add_picture(const Vec3 world[4], const sieve::RgbaImage& picture);
    // A line, drawn where nothing nearer covers it (a wireframe's edges over faces filled black).
    void add_edge(Vec3 a, Vec3 b, uint32_t argb);
    // Rasterises everything added and draws it over the whole render target.
    void draw(SDL_Renderer* r);
    size_t drawn() const { return drawn_; } // triangles in the last draw
    // The depth the last draw() left behind: 1 / camera depth for each pixel, row by row, larger
    // being nearer. Anything drawn over the models afterwards (the door portals) tests against
    // it so the models still hide what is behind them. Null before the first draw.
    const float* depth() const { return depth_.empty() ? nullptr : depth_.data(); }
    int width() const { return w_; }
    int height() const { return h_; }
    // Frees the texture; call before destroying the renderer.
    void release();

private:
    struct Tri
    {
        float x[3], y[3];  // screen position of each corner
        float A, B, C;     // 1 / camera depth over the screen: A x + B y + C (it is planar there)
        uint32_t colour;   // ARGB
        float ymin, ymax;
        const sieve::RgbaImage* picture = nullptr; // a picture's face: u/z and v/z planar too
        float Au = 0, Bu = 0, Cu = 0, Av = 0, Bv = 0, Cv = 0;
    };
    struct Edge
    {
        float x0, y0, x1, y1; // on the screen
        float n0, n1;         // 1 / camera depth at each end
        uint32_t colour;
    };
    void push(const Vec3* cam3, uint32_t colour, float bias, const float (*uv)[2] = nullptr, const sieve::RgbaImage* picture = nullptr);
    void clip_push(const Vec3* cam3, uint32_t colour);
    std::vector<Edge> edges_;
    bool overlay_ = false;
    void raster_band(int y0, int y1);

    const Camera* cam_ = nullptr;
    SDL_FColor bg_{};
    uint32_t bg_argb_ = 0xFF000000u;
    int w_ = 1, h_ = 1;
    std::vector<Tri> tris_;
    std::vector<uint32_t> colour_; // the image the bands draw into, uploaded in one go
    uint32_t* pixels_ = nullptr;   // colour_.data() while the bands are running
    int pitch_ = 0;                // pixels per row there
    std::vector<float> depth_;
    SDL_Texture* texture_ = nullptr;
    SDL_Renderer* texture_owner_ = nullptr;
    int tex_w_ = 0, tex_h_ = 0;
    size_t drawn_ = 0;
    struct Pool;
    std::unique_ptr<Pool> pool_;
};

} // namespace hallway
