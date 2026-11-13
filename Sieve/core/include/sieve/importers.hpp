// Sieve — importers: existing models brought into Sieve, one format at a time (IDEAS 19.159).
//
// Every importer reads one existing format into the same plain form, a mesh of triangles
// (ModelSpace::AnyMesh), and says what else the file held that the mesh does not carry: skins,
// animations, primitives that are not triangles, compression it cannot undo. Nothing is dropped
// silently; a format's roof is stated, and what lies above it is named.
//
// The formats, each in import_formats():
//   obj   Wavefront .obj text: v and f lines (ModelSpace::read_any_obj)
//   gltf  glTF 2.0 JSON, buffers embedded as data: URIs or beside it in files
//   glb   glTF 2.0 binary: the same JSON and its buffer in one file
//
// A glTF's meshes are placed as its default scene places them (each node's matrix, or its
// translation, rotation and scale, down the tree), so a model made of parts comes in assembled.
//
// Its first skin comes in as a skeleton (IDEAS 19.170): its joints at their rest positions, each
// with the nearest joint above it as its parent, parents first; skeleton_character() makes that a
// character of given spheres (sieve-character-v3). Its animations are sampled at 30 frames a second
// into each joint's turn from rest in every frame, and imported_animation() makes one of them a unit
// of the animation line (sieve/animation.hpp). Turns are what that line holds: a joint's movement
// (translation) and growth (scale) are counted and named as left out, and so are cubic splines'
// tangents (their keys are played as straight lines between them). The roof is the animation line's:
// 64 joints, 300 frames (ten seconds).
#pragma once

#include "sieve/modelspace.hpp"
#include "sieve/character.hpp"
#include "sieve/animation.hpp"

#include <array>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace sieve {

struct ImportFormat
{
    std::string_view id;         // "obj", "gltf", "glb"
    std::string_view name;       // "Wavefront OBJ"
    std::string_view extensions; // "obj"; "gltf"; "glb" (for file dialogs: ';'-separated)
    std::string_view carries;    // what of the file comes in
    std::string_view leaves;     // what does not, yet
};
const std::vector<ImportFormat>& import_formats();

struct ImportedModel
{
    std::string format;    // an ImportFormat id
    ModelSpace::AnyMesh mesh;
    // What the file held, counted (glTF; an .obj counts only its mesh).
    size_t meshes = 0, primitives = 0, nodes = 0;
    size_t skins = 0, joints = 0, animations = 0, channels = 0, keyframes = 0;
    // The first skin's joints (glTF), parents first; positions in metres, as the file's rest pose
    // places them.
    struct Joint
    {
        std::string name;
        int32_t parent = -1; // an index into skeleton; -1 for the first (the body)
        double x = 0, y = 0, z = 0;
    };
    std::vector<Joint> skeleton;
    // Each animation, sampled: frames * skeleton.size() turns from rest, quaternions (x, y, z, w).
    struct Sampled
    {
        std::string name;
        double seconds = 0;
        uint32_t frames = 0;
        std::vector<std::array<double, 4>> turns;
        // Where each joint stands in each frame (metres, as turns), and whether a translation channel
        // moved anything: then the animation carries moves as well (sieve-animation-v2).
        std::vector<std::array<double, 3>> places;
        bool moved = false;
    };
    std::vector<Sampled> sampled;
    // Each thing the file held that the mesh does not carry, said in a few words
    // ("2 skins of 38 joints, not carried: a held model has no bones yet").
    std::vector<std::string> left_out;
};

// Reads a file beside the one being imported, by the URI the file names (a glTF's external
// buffers). Return nothing if it cannot or may not be read; the import then fails, naming it.
using ImportLoader = std::function<std::optional<std::string>(const std::string& uri)>;

// glTF 2.0, as JSON (.gltf) or binary (.glb), told apart by the first bytes. Throws
// std::invalid_argument, saying what is wrong, on a file it cannot read: not glTF 2.0, a buffer
// it cannot reach, an accessor outside its buffer, or a required extension it does not know
// (Draco or meshopt compression).
ImportedModel read_gltf(std::string_view bytes, const ImportLoader& load = {});

// Any format above, told apart by its content: "glTF" at the start is a .glb, a JSON object
// with an "asset" member is a .gltf, anything else is read as an .obj.
ImportedModel read_model(std::string_view bytes, const ImportLoader& load = {});

// The skeleton as a character of given spheres (sieve-character-v3), 4096 units a metre, its body at
// the origin: each joint a sphere, its radius a quarter of the way to its nearest neighbour in the
// tree (the body's a third of its farthest child's), a joint named like "head" a head. Throws
// std::invalid_argument if the file has no skeleton.
Character skeleton_character(const ImportedModel& m);
// One of the sampled animations as a unit of the animation line, its bones the skeleton's joints in
// the same order. Throws std::invalid_argument if there is no such animation.
Animation imported_animation(const ImportedModel& m, size_t which = 0);

// True if a URI may be read beside the file: a relative path that stays below the file's
// folder (no "..", no drive or root, no scheme). The importers' callers check this before
// loading, so a model cannot reach elsewhere on the disk.
bool import_uri_is_local(std::string_view uri);
// A URI's %XX escapes decoded (glTF URIs are percent-encoded).
std::string import_uri_decoded(std::string_view uri);

} // namespace sieve
