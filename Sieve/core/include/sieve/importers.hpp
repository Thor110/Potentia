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
// Its skeletons (skins) and animations are counted, not carried: a held model has no bones yet.
#pragma once

#include "sieve/modelspace.hpp"

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

// True if a URI may be read beside the file: a relative path that stays below the file's
// folder (no "..", no drive or root, no scheme). The importers' callers check this before
// loading, so a model cannot reach elsewhere on the disk.
bool import_uri_is_local(std::string_view uri);
// A URI's %XX escapes decoded (glTF URIs are percent-encoded).
std::string import_uri_decoded(std::string_view uri);

} // namespace sieve
