// Sieve — the animation line: turns per bone per frame (IDEAS 19.159, 19.170).
//
// "sieve-animation-v1". An animation is B bones and F frames at 30 frames a second. In each frame
// each bone has a turn: how far it has turned from its rest pose, in the world's frame, as a
// rotation vector (an axis times an angle in radians), each of its three components one of 256
// steps,
//     component = (d - 128) * pi / 128,    d in 0..255,
// so d = 128 is exactly no turn, and the steps reach from -pi to just under pi. Every animation of B bones and F frames is one address on the line of
// 256^(3BF) of them: its digits in order, frame by frame, bone by bone, x then y then z, written as
// hex (two digits each), positionally. Nothing depends on a machine's floating point but turning
// a digit back into an angle and the angle into a matrix, which only drawing does.
//
// Playing it on a character (a tree of spheres, each sphere a bone): the body stays where it is;
// every other sphere moves with its parent's turn,
//     P(i) = P(parent) + T(parent) * (rest(i) - rest(parent)),
// where T is the parent's turn in that frame (a sphere past the animation's bones turns as its
// parent does). So an animation made for one skeleton plays on any character with as many spheres,
// and a seed-made character can be animated as well as an imported one.
//
// The text: the first line names the version and its shape; then one line a frame:
//     sieve-animation-v1 bones=B frames=F rate=30
//     f INDEX HEX...        (one six-digit hex group a bone: x, y, z)
// Its identity is the SHA-256 of that text. The roof, as built: 64 bones, 300 frames (ten seconds).
//
// "sieve-animation-v2" (13 November 2026): the same, and in each frame each bone a move as well, in
// the character's units (4096 a metre), its three components each one of 2^24 steps,
//     component = d - 8,388,608,    d in 0..16,777,215 (six hex digits),
// so a bone may stand up to 2,048 m from where its parent's turn alone would put it:
//     P(i) = P(parent) + T(parent) * (rest(i) - rest(parent)) + M(i),    P(body) = rest(body) + M(body).
// It carries what a glTF's translation channels do (the body walking forward, a part sliding). The
// text is v1's but for its first word, and each bone's group is its turn's six hex digits then its
// move's eighteen (x, y, z):
//     sieve-animation-v2 bones=B frames=F rate=30
//     f INDEX TURNMOVE...
// and its address the turns' and moves' digits in that order, frame by frame, bone by bone. An
// animation with no move anywhere is written as v1, so every v1 animation keeps its identity, and
// both are read.
#pragma once

#include "sieve/character.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace sieve {

inline constexpr const char* kAnimationVersion = "sieve-animation-v1";
inline constexpr const char* kAnimationVersion2 = "sieve-animation-v2";
inline constexpr int32_t kAnimationMoveMost = (1 << 23) - 1, kAnimationMoveLeast = -(1 << 23);
inline constexpr uint32_t kAnimationRate = 30;
inline constexpr uint32_t kAnimationMaxBones = 64, kAnimationMaxFrames = 300;

struct BoneTurn
{
    uint8_t x = 128, y = 128, z = 128; // 128: no turn
    bool operator==(const BoneTurn&) const = default;
};

// A quaternion (x, y, z, w) as the nearest turn, and a turn as a rotation matrix (row-major 3x3).
BoneTurn turn_of_quaternion(double x, double y, double z, double w);
void turn_matrix(const BoneTurn& t, double m[9]);

struct BoneMove
{
    int32_t x = 0, y = 0, z = 0; // character units; kAnimationMoveLeast to kAnimationMoveMost
    bool operator==(const BoneMove&) const = default;
};

struct Animation
{
    uint32_t bones = 0, frames = 0;
    std::vector<BoneTurn> turns; // frames * bones, frame by frame
    std::vector<BoneMove> moves; // none (v1), or frames * bones as turns (v2)
    const BoneTurn& at(uint32_t frame, uint32_t bone) const { return turns[size_t(frame) * bones + bone]; }
    BoneMove move(uint32_t frame, uint32_t bone) const { return moves.empty() ? BoneMove{} : moves[size_t(frame) * bones + bone]; }
    bool moving() const;         // any move not zero (written as v2)
    std::string text() const;
    std::string id() const;      // SHA-256 of the text, in hex
    std::string address() const; // its place on the line of B bones and F frames, in hex
};

// Reads an animation's text back (v1 or v2), checked (the shape within the roof, one line a frame, every
// group six hex digits). Throws std::invalid_argument saying what is wrong.
Animation read_animation(const std::string& text);

// The spheres' centres in one frame of an animation played on a character (above), in the
// character's units. Frames past the end wrap round.
std::vector<std::array<double, 3>> pose(const Character& c, const Animation& a, uint32_t frame);
// Each sphere's turn in that frame (row-major 3x3): its own, or, past the animation's bones, its
// parent's; what a model held in it turns by.
std::vector<std::array<double, 9>> pose_turns(const Character& c, const Animation& a, uint32_t frame);
// A point of a held model (its sphere's units, -1 to 1) turned by its sphere's turn; unturned with none.
inline std::array<float, 3> turned(const std::array<double, 9>* t, const float v[3])
{
    if (!t) return {v[0], v[1], v[2]};
    const auto& m = *t;
    return {float(m[0] * v[0] + m[1] * v[1] + m[2] * v[2]), float(m[3] * v[0] + m[4] * v[1] + m[5] * v[2]), float(m[6] * v[0] + m[7] * v[1] + m[8] * v[2])};
}

} // namespace sieve
