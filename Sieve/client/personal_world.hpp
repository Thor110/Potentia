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
#include "slide_panel.hpp"
#include "favourites.hpp"

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
    enum class Result { Back, Quit, Open }; // Open: a favourite opened (O), to be read on its own line (opened())
    const Favourite* opened() const { return opened_.empty() ? nullptr : favourite(opened_); }
    Result run();
    // For pictures and tests: a key as if pressed and held for `seconds`, the frame drawn, and where
    // you stand.
    void hold(SDL_Scancode key, float seconds);
    void press(SDL_Keycode key, SDL_Keymod mod = SDL_KMOD_NONE); // the world's keys, as if typed
    void look(float pitch_degrees) { pitch_ = pitch_degrees * 3.14159265f / 180.0f; } // (for pictures and tests)
    void render();
    void settle_panels() { cat_panel_.settle(), place_panel_.settle(); } // (a picture: the panels where they are going)
    // Real Graphics off: the characters drawn as the wireframe (rings and bones), as everywhere else.
    void set_wireframe(bool on) { wireframe_ = on; }
    // Its features (IDEAS 19.197), as Personal Dimensions > Features has them: the map and the compass.
    void set_features(bool map, bool compass) { show_map_ = map, show_compass_ = compass; }
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
        float x = 0, y = 0, z = 0;           // where it stands (y up; its foot, the model's y = 0)
        float yaw = 0, pitch = 0, roll = 0;  // its turns, in that order (sieve-personal-world-v2)
        bool physics = false;                // dropped: falls, rests and can be knocked; else placed
        float vx = 0, vy = 0, vz = 0;        // (physics: its velocity, metres a second)
        bool resting = true;
    };
    std::vector<Placed> placed_;
    std::vector<std::string> kept_lines_; // a v2 file's lines this build does not know, written back unchanged
    int held_ = -1;                       // the object in your hand (picked up to the cursor), or none
    Placed held_from_;                    // where it was, for Esc (put back)
    float physics_left_ = 0;              // time not yet stepped by the fixed physics step
    bool unsaved_rest_ = false;           // something came to rest since the last save
    // The object under the crosshair, within reach, or -1.
    int aimed() const;
    float ray_objects(const float o[3], const float d[3], float reach, int skip, int* which) const;
    // Surfaces facing up, for resting things on things (a shelf board, a lid, the grass).
    float surface_below(float x, float top, float z, int skip) const;
    bool surface_along(float x, float y, float z, float dx, float dy, float dz, float reach, int skip, float& t) const;
    // Physics, in fixed steps of 1/120 s, so the same drop rests in the same place on every machine.
    void physics_step(float dt, float move_x, float move_z);
    // An object's shape as its turns make it: the drop from its origin to its lowest point, and how
    // far it reaches across (for knocks and aiming).
    void extent_of(const Placed& p, float& low, float& high, float& across) const;
    std::array<float, 3> middle_of(const Placed& p) const; // its model's box's middle, in the world
    std::filesystem::path file_;
    // The panels slide in from the left as the setup menu's do (slide_panel.hpp), one at a time: the
    // catalogue, and while something is in your hand, Place (its exact place and turns).
    SlidePanel cat_panel_, place_panel_;
    int place_row_ = 0;       // Place: the row chosen (x, y, z, yaw, pitch, roll, then "place it")
    std::string place_edit_;  // Place: a number being typed into the chosen row
    void place_key(SDL_Keycode k, SDL_Keymod mod);
    int catalogue_row_ = 0;
    std::string note_;
    void load();
    void save();
    void key(SDL_Keycode k, SDL_Keymod mod = SDL_KMOD_NONE);
    void place_held();
    void drop_held();
    void put_back_held();
    // The map's zoom (IDEAS 19.199's first keys): 0 the whole world, each step in halves what it shows.
    static constexpr int kMapZooms = 5;
    int map_zoom_ = 0;
public:
    float map_across() const { return 2 * 320.0f / float(1 << map_zoom_); } // (metres the map shows across)
private:
    // An object's faces, in the world: triangles (three points each) and their colour.
    void faces_of(const Placed& p, std::vector<std::pair<std::array<std::array<float, 3>, 3>, SDL_FColor>>& out) const;
    // Favourites (IDEAS 19.190): this dimension's, read beside its world file, in the catalogue after
    // Sieve's shelf items as "fav:SHA" and drawn as their line's shelf item.
    std::vector<Favourite> favourites_;
    std::vector<std::string> rows_;  // the catalogue as shown: the shelf items' ids, then "fav:SHA" each
    std::string opened_;             // the favourite O opened, by its SHA
    const Favourite* favourite(const std::string& sha) const;
    std::string mesh_kind(const std::string& kind) const; // the shelf item a kind is drawn as
    std::string label_of(const std::string& kind) const;  // what it is called on the screen
    bool project(float x, float y, float z, float& sx, float& sy, float& depth) const;
    SDL_Window* window_;
    SDL_Renderer* r_;
    std::vector<sieve::Character> characters_;
    uint64_t seed_;
    std::vector<Swell> swells_;
    std::vector<Cloud> clouds_;
    float px_ = 0, pz_ = -60, yaw_ = 0, pitch_ = -0.05f; // where you stand, and which way you look
    bool wireframe_ = false;
    bool show_map_ = true, show_compass_ = true;
    void draw_map();
    void draw_compass();
    // Whether you may pick things up and place them here (an author) or only open them (a visitor):
    // every world is yours alone until identity exists (IDEAS 19.178), so always an author for now.
    bool author_ = true;
    float wind_ = 0;                                     // how far the clouds have drifted
    float W_ = 1280, H_ = 720;
};

} // namespace hallway
