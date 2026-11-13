// Sieve — actions to animations (IDEAS 19.186): which animation plays each thing a character does.
//
// "sieve-actions-v1". The actions are the game's own, a fixed list in this version, in this order:
//     idle   standing still            walk   moving               run    moving quickly (Shift)
//     jump   off the ground            take   taking something     read   reading what is held
//     wave   a greeting
// A map names, for every one of them, the animation that plays it, by the animation's SHA-256
// (sieve-animation-v1 or v2), or `none`:
//     sieve-actions-v1
//     action idle SHA-256|none
//     action walk SHA-256|none
//     ...                          (every action of the list, once each, in the list's order)
// Each line ends in a line feed; nothing else. Its identity is the SHA-256 of that text, so a change
// (another animation for walking) is a new version. A character carries its map in its save
// (sieve-save-v2), so a custom character brings its actions with it.
//
// Playing: an action with an animation plays it where the animation fits the character, that is
// where it has no more bones than the character has spheres (the rule pose() plays by); an action
// with none, or with one that does not fit or is not in the store, plays nothing, and the player is
// told once ("no animation for walk on this character"), never again for that action, and may choose
// one from the animations that fit.
#pragma once

#include "sieve/animation.hpp"
#include "sieve/character.hpp"

#include <array>
#include <string>
#include <string_view>
#include <vector>

namespace sieve {

inline constexpr const char* kActionsVersion = "sieve-actions-v1";

enum class Action { Idle, Walk, Run, Jump, Take, Read, Wave };
inline constexpr size_t kActionCount = 7;
inline constexpr std::array<const char*, kActionCount> kActionNames = {"idle", "walk", "run", "jump", "take", "read", "wave"};

const char* action_name(Action a);
// The action of that name; throws std::invalid_argument if it is not one of the list.
Action action_from_name(std::string_view name);

struct ActionMap
{
    std::array<std::string, kActionCount> animation; // each action's animation's SHA-256, or empty (none)
    const std::string& of(Action a) const { return animation[size_t(a)]; }
    void set(Action a, std::string sha) { animation[size_t(a)] = std::move(sha); }
    bool empty() const;          // no action has an animation
    std::string text() const;
    std::string id() const;      // SHA-256 of the text
};

// A map read back, checked (its version, every action once in the list's order, each a SHA-256 or
// none). Throws std::invalid_argument saying what is wrong.
ActionMap read_actions(const std::string& text);

// Whether an animation can play on a character: no more bones than it has spheres.
bool animation_fits(const Character& c, const Animation& a);

} // namespace sieve
