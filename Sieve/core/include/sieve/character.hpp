// Sieve — characters: a seed and a few settings unfolding a tree of spheres (IDEAS §19.147 to §19.149).
//
// "sieve-character-v1". A character is a tree of spheres, each a centre, a radius and a parent: the
// spheres of influence, each able to hold anything (a model of the models line, a shape, nothing)
// in later versions. The tree is the skeleton, and it is not placed bone by bone: the settings and
// the seed unfold it, so the same seed and settings give the same character on every machine, and a
// character is named by them. Every number is an integer (lengths in 1/1024 of a unit), so nothing
// depends on a machine's floating point.
//
// The draws: the k-th 32-bit number is the first four bytes, little-endian, of
//     SHA-256("sieve-character-v1" | seed as 8 bytes little-endian | k as 4 bytes little-endian),
// k counting from 0 in the order the steps below take them.
//
// The unfolding:
//   1. The body (sphere 0) at the origin, radius 384 + d % 256.
//   2. The head, when asked for: radius body * (50 + d % 26) / 100, straight up from the body, its
//      centre the two radii apart.
//   3. The limbs, each from a direction of the 26 of a cube's neighbours (x, y, z each -1, 0 or 1,
//      not all 0, in that order of x, then y, then z), never straight up:
//        none        each limb draws its own: d % 25 of the 25 directions that are not up;
//        bilateral   limbs come in pairs: the first of a pair draws d % 8 of the 8 directions with
//                    x = +1 other than (1, 1, 0), (y, z) in the order (-1,-1), (-1,0), (-1,1), (0,-1),
//                    (0,0), (0,1), (1,-1), (1,1); the second is its mirror (x = -1); an odd last
//                    limb goes forward or straight down (d % 2: 1 forward, (0, 0, 1); 0 down) and
//                    stays on the plane x = 0: its x never bends, and its branches' drawn
//                    directions have x set to 0 (straight down where that leaves nothing);
//        radial      limb i of L takes the horizontal direction (y = 0) i * 8 / L of the 8 in the
//                    order (1,0,0), (1,0,1), (0,0,1), (-1,0,1), (-1,0,0), (-1,0,-1), (0,0,-1),
//                    (1,0,-1).
//      A limb's first joint sits on the body's surface along its direction, radius body * (35 + d %
//      21) / 100. Each next joint: one draw; when d % 4 = 0 the direction bends (one of its three
//      components, chosen by (d / 4) % 3, steps by +1 or -1 by (d / 12) % 2, kept within -1..1 and
//      never all 0); the joint moves two radii along the direction (a unit direction scaled by
//      1024 / sqrt(number of nonzero components): 1024, 724, 591); its radius is the last times (75
//      + (d / 24) % 18) / 100, never under 8. A mirrored limb makes no draws of its own: it is its
//      pair with x negated.
//   4. Branches: from each limb's last joint, B branches of ceil(J / 2) joints each, unfolding as a
//      limb from that joint (its first direction drawn as in "none", its first joint one radius out),
//      mirrored with its limb.
//
// The text: the first line names the version and the settings; then one line a sphere, in order:
//     s INDEX PARENT X Y Z R KIND    (KIND: body, head, joint; PARENT -1 for the body)
// The character's identity is the SHA-256 of that text.
//
// "sieve-character-v2" (IDEAS 19.148, step 6): a v1 character whose spheres hold models. Its text is
// v1's with the version on the first line read v2, and after the spheres one line a held model:
//     h SPHERE MODEL    (MODEL: a model of the models line "models/V8/F12/C16/key=sieve/modelspace-v1",
//                        by its positional address in hex, 51 digits; at most one a sphere, in order)
// The model's coordinates, in (-1, 1), are its sphere's radius. A character that holds nothing is
// written as v1, so every v1 character keeps its text and its identity; v1 files read as before.
//
// "sieve-character-v3" (IDEAS 19.170): a character whose spheres are given rather than unfolded from
// a seed: an imported skeleton (a glTF skin's joints, sieve/importers.hpp), or any tree made by hand.
// Its first line is "sieve-character-v3 spheres=N"; then its N sphere lines as above, each one's
// parent before it (the body, sphere 0, has parent -1 and no other sphere has), every radius at
// least 1 and every coordinate within 2^28; then its held models' lines, if it holds any, as in v2.
// It has no seed and no settings; its identity is the SHA-256 of its text, as for every version.
//
// Any version may also be written on one line with ';' between its lines (the settings file keeps a
// character so); it reads the same and has the same identity.
#pragma once

#include "sieve/format_stats.hpp"

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace sieve {

inline constexpr const char* kCharacterVersion = "sieve-character-v1";
inline constexpr const char* kCharacterVersion2 = "sieve-character-v2";
inline constexpr const char* kCharacterVersion3 = "sieve-character-v3";
// The models a sphere may hold: the models line at its default shape (eight vertices, twelve faces,
// sixteen coordinates), positional.
inline constexpr uint32_t kHeldVertices = 8, kHeldFaces = 12, kHeldCoords = 16;
// The central metric (IDEAS 19.153): 4096 of a character's units are a metre, so every character has
// a size in the world and the hallway can be scaled to it; the default character, about 0.9 m
// across, fits the doors as built.
inline constexpr int32_t kCharacterUnitsPerMetre = 4096;

enum class Symmetry : uint8_t { None = 0, Bilateral = 1, Radial = 2 };

struct CharacterSettings
{
    uint64_t seed = 0;
    uint32_t limbs = 4;     // 0 to 16
    uint32_t joints = 3;    // per limb, 1 to 12
    uint32_t branches = 0;  // per limb, at its end, 0 to 3
    Symmetry symmetry = Symmetry::Bilateral;
    bool head = true;
    // Throws std::invalid_argument naming the setting out of range.
    void check() const;
};

struct Sphere
{
    int32_t parent = -1;
    int32_t x = 0, y = 0, z = 0; // in 1/1024 of a unit
    int32_t r = 0;
    enum Kind : uint8_t { Body = 0, Head = 1, Joint = 2 } kind = Joint;
};

struct Character
{
    CharacterSettings settings;
    std::vector<Sphere> spheres;
    // Models held (v2): sphere index -> the model's positional address on the models line, in hex.
    std::vector<std::pair<uint32_t, std::string>> holds;
    // v3: the spheres are given, not unfolded from the settings (which are then unused).
    bool given = false;
    std::string text() const;     // the canonical text (above): v1, v2 where it holds models, v3 where given
    std::string one_line() const; // the text with ';' between its lines (for the settings file)
    // Puts a model in a sphere (an empty address takes it out), checked: the sphere must exist and
    // the address be a model of the held models' line. Throws std::invalid_argument otherwise.
    void hold(uint32_t sphere, const std::string& hex);
    const std::string* held(uint32_t sphere) const;
    std::string id() const;       // SHA-256 of the text, in hex
    // The half-widths of the box holding every sphere, centred on the body (for the hallway to fit it).
    void extent(int32_t& x, int32_t& y, int32_t& z) const;
};

// Unfolds the character of these settings (checked first).
Character unfold_character(const CharacterSettings& s);

// A species ("sieve-species-v1", IDEAS 19.152): the settings with the seed left free, named by the
// SHA-256 of "sieve-species-v1 limbs=L joints=J branches=B symmetry=S head=H" in hex. Every seed of a
// species is one of its members, so a species is a line of characters walked by seed.
std::string species_text(const CharacterSettings& s);
std::string species_id(const CharacterSettings& s);

// The seed walk ("sieve-character-walk-v1", IDEAS 19.148 step 7): from one character's settings, the
// next: d = SHA-256("sieve-character-walk-v1" | the settings' line as text); its first 8 bytes,
// little-endian, are the next seed, and its next bytes draw the next settings: limbs = d[8] % 9,
// joints = 1 + d[9] % 6, branches = d[10] % 3, symmetry = d[11] % 3 (none, bilateral, radial),
// head = d[12] % 4 != 0. So a walk is a line of characters, the same from the same start on every
// machine, and any step can be retraced from the one before.
CharacterSettings walk_character(const CharacterSettings& from, uint32_t steps = 1);

// A character of given spheres (v3), checked as read_character checks one. Throws
// std::invalid_argument saying what is wrong.
Character given_character(std::vector<Sphere> spheres);

// A character read back from its text (a .character file): the first line's settings unfolded,
// and, where sphere lines follow, every one checked against the unfolding. Throws
// std::invalid_argument saying what is wrong (not this version, a setting missing or out of range,
// or spheres that do not match their settings). A file of the first line alone is a character too.
Character read_character(const std::string& text);

// A held model's edges, each a pair of points in (-1, 1) (times its sphere's radius to draw it).
struct HeldEdge
{
    float a[3], b[3];
};
std::vector<HeldEdge> held_model_edges(const std::string& hex);
// The model n steps along the held models' line from `hex` (wrapping round), or the first model
// when `hex` is empty; and a model chosen by a 64-bit number (any model as likely as any other,
// up to the number's reach).
std::string held_model_step(const std::string& hex, int64_t n);
std::string held_model_from(uint64_t r);

// Importing an existing model into a sphere (IDEAS 19.159, ".obj" first): any Wavefront .obj, brought
// under the held models' roof (8 vertices, 12 faces). 8 vertices are kept spread across the whole shape
// (the first the farthest from the centre of its bounds, each next the farthest from those kept), the
// 12 largest faces kept with each corner moved to its nearest kept vertex (faces that close up are left
// out), and the result fitted to the line's grid. Deterministic: the same file, the same model.
struct HeldImport
{
    std::string hex;                       // the held model's address
    size_t vertices_in = 0, faces_in = 0;  // what the file had
    size_t faces_kept = 0;                 // faces that survived the reduction (at most 12)
    std::string format;                    // the importer that read it ("obj", "gltf", "glb")
    std::vector<std::string> left_out;     // what the file held beyond the mesh (importers.hpp)
    size_t joints = 0, animations = 0;     // its skeleton and animations, as read (for its figures)
};
HeldImport import_obj_for_hold(std::string_view obj_text);
// Any format the importers read (sieve/importers.hpp: .obj, .gltf, .glb), told apart by its content;
// `load` reads a glTF's buffers beside it. The mesh is brought under the roof as above.
HeldImport import_model_for_hold(std::string_view bytes, const std::function<std::optional<std::string>(const std::string&)>& load = {});
// The same from a model already read (a skeleton taken by Ctrl+K); with no mesh in it, its figures
// alone (no hex).
struct ImportedModel;
HeldImport held_import_of(const ImportedModel& m);
// Stage 4 of the catalogue: what an import kept, as a record (sieve-format-stat-v1) for its
// importer's entry, "model-" + its format: `sieve import` and the CHARACTER page's Ctrl+I and Ctrl+K
// alike. "whole" when nothing was left out and every vertex and face kept.
FormatStat import_format_stat(const HeldImport& h, const std::string& machine);

const char* symmetry_name(Symmetry s);
// "none", "bilateral" or "radial"; throws std::invalid_argument otherwise.
Symmetry symmetry_from_name(const std::string& name);

} // namespace sieve
