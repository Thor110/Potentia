// Sieve hallway — your personal dimension as a world of its own (IDEAS 19.187; Edward, 13 November
// 2026: "it should go through to an empty 3D world, the default starting world should be a grassy hill
// with blue sky and clouds").
//
// The main menu's Personal Dimensions opens it: a rolling grassy hill under a blue sky with white
// clouds drifting, and your characters standing on the hill, yours (the one you play) in front of you
// and those in your store round the top. The feeling of that famous desktop, made from Sieve's own
// shapes, not a copy of the photograph: everything drawn from the world's seed (0, the default) as a
// line's units are, so the same seed is the same world on every machine.
//   - The hill: a height field, the sum of a few broad swells whose places, sizes and heights come
//     from the seed, highest near the middle, falling away to a far plain; the grass shaded by the
//     slope against the sun, lighter on the crests, with a faint variation of its own.
//   - The sky: from deep blue overhead to pale near the horizon.
//   - The clouds: soft white shapes, each a few overlapping discs, at heights and places from the
//     seed, drifting slowly with the wind.
// You walk it with WASD and the mouse (or the arrows), Shift to run, your eyes 1.7 m above the grass.
// Esc goes back to the main menu.
//
// It is a space to make your own (Edward: "the personal dimension should have the ability to place
// objects from a catalog in the world space"): Tab opens the catalogue, Up/Down choose, Enter places
// one on the ground before you, turned to face you, and Backspace takes away the one nearest you
// (within eight metres). The catalogue is Sieve's own shelf items, one for each line (Edward: "the
// default placeable objects can just be the sieve books for shelves"), at their size on the shelves,
// read from data/objects/shelf-items.sieve: preserved there, apart from the meshes folder that Real
// Graphics lets anyone change ("they also need sieving and preserving somewhere aside from the
// overridable ones"). Every object is drawn in the world's 3D space with the hill, the nearer before
// the farther. What is placed is kept as "sieve-personal-world-v1" text beside the settings
// (worlds/personal.world): its first line, then "seed N", then one line an object, "place KIND X Z
// YAW" (KIND a catalogue id, metres and radians, %.9g), in the order they were placed; it is read
// back when the world opens (a line not so written is left out, and said).
#pragma once

#include <SDL3/SDL.h>

#include "sieve/character.hpp"

#include <array>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace hallway {

class PersonalWorld
{
public:
    PersonalWorld(SDL_Window* window, SDL_Renderer* renderer, std::vector<sieve::Character> characters, std::filesystem::path world_file = {}, uint64_t seed = 0);
    // The catalogue of objects, by id ("cube", "pillar", ...), in the order shown.
    static const std::vector<std::string>& catalogue();
    // Empty when every shelf item was read from shelf-items.sieve; else what could not be.
    static std::string catalogue_note();
    enum class Result { Back, Quit };
    Result run();
    // For pictures and tests: a key as if pressed and held for `seconds`, the frame drawn, and where
    // you stand.
    void hold(SDL_Scancode key, float seconds);
    void press(SDL_Keycode key); // the catalogue's keys (Tab, Up, Down, Enter, Backspace), as if typed
    void render();
    // Real Graphics off: the characters drawn as the wireframe (rings and bones), as everywhere else.
    void set_wireframe(bool on) { wireframe_ = on; }
    std::string report() const;
    float height(float x, float z) const; // the ground's height there, in metres
    float ground(float x, float z) const; // the same as drawn (the cells' flat triangles)

private:
    struct Swell
    {
        float x, z, radius, rise;
    };
    struct Cloud
    {
        float x, y, z, size;
        std::vector<std::array<float, 3>> puffs; // offsets and radius of its discs
    };
    void step(float dt, const bool* keys);
    struct Placed
    {
        std::string kind;
        float x = 0, z = 0, yaw = 0;
    };
    std::vector<Placed> placed_;
    std::filesystem::path file_;
    bool catalogue_open_ = false;
    int catalogue_row_ = 0;
    std::string note_;
    void load();
    void save();
    void key(SDL_Keycode k);
    // An object's faces, in the world: triangles (three points each) and their colour.
    static void faces_of(const Placed& p, float ground, std::vector<std::pair<std::array<std::array<float, 3>, 3>, SDL_FColor>>& out);
    bool project(float x, float y, float z, float& sx, float& sy, float& depth) const;
    SDL_Window* window_;
    SDL_Renderer* r_;
    std::vector<sieve::Character> characters_;
    uint64_t seed_;
    std::vector<Swell> swells_;
    std::vector<Cloud> clouds_;
    float px_ = 0, pz_ = -60, yaw_ = 0, pitch_ = -0.05f; // where you stand, and which way you look
    bool wireframe_ = false;
    float wind_ = 0;                                     // how far the clouds have drifted
    float W_ = 1280, H_ = 720;
};

} // namespace hallway
