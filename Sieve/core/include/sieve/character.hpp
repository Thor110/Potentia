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
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace sieve {

inline constexpr const char* kCharacterVersion = "sieve-character-v1";
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
    std::string text() const;     // the canonical text (above)
    std::string id() const;       // SHA-256 of the text, in hex
    // The half-widths of the box holding every sphere, centred on the body (for the hallway to fit it).
    void extent(int32_t& x, int32_t& y, int32_t& z) const;
};

// Unfolds the character of these settings (checked first).
Character unfold_character(const CharacterSettings& s);

// A character read back from its text (a .character file): the first line's settings unfolded,
// and, where sphere lines follow, every one checked against the unfolding. Throws
// std::invalid_argument saying what is wrong (not this version, a setting missing or out of range,
// or spheres that do not match their settings). A file of the first line alone is a character too.
Character read_character(const std::string& text);

const char* symmetry_name(Symmetry s);
// "none", "bilateral" or "radial"; throws std::invalid_argument otherwise.
Symmetry symmetry_from_name(const std::string& name);

} // namespace sieve
