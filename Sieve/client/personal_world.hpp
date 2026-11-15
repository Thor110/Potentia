// Sieve hallway — your personal dimension as a world of its own (IDEAS 19.187; Edward, 13 November
// 2026: "it should go through to an empty 3D world, the default starting world should be a grassy hill
// with blue sky and clouds").
//
// The main menu's Personal Dimensions opens it: a rolling grassy hill under a blue sky with white
// clouds drifting, and your characters standing on the hill, yours (the one you play) in front of you
// and those in your store round the top: objects of the world like any other (below). The feeling of that famous desktop, made from Sieve's own
// shapes, not a copy of the photograph: everything drawn from the world's seed (0, the default) as a
// line's units are, so the same seed is the same world on every machine.
//   - The hill: a height field, the sum of a few broad swells whose places, sizes and heights come
//     from the seed, highest near the middle, falling away to a far plain; the grass shaded by the
//     slope against the sun, lighter on the crests, with a faint variation of its own.
//   - The sky: from deep blue overhead to pale near the horizon.
//   - The clouds: soft white shapes, each a few overlapping discs, at heights and places from the
//     seed, drifting slowly with the wind.
// You walk it with WASD and the mouse (or the arrows), Shift to run, your eyes 1.7 m above the grass.
// Esc opens its pause menu (pause_rows.hpp), whose Leave row goes back.
//
// It is a space to make your own (Edward: "the personal dimension should have the ability to place
// objects from a catalog in the world space"): Tab opens the catalogue, Up/Down choose, Enter places
// one on the ground before you, turned to face you, and Backspace takes away the one nearest you
// (within eight metres). The catalogue is Sieve's own shelf items, one for each line (Edward: "the
// default placeable objects can just be the sieve books for shelves"), at their size on the shelves,
// read from data/objects/shelf-items.sieve: preserved there, apart from the meshes folder that Real
// Graphics lets anyone change ("they also need sieving and preserving somewhere aside from the
// overridable ones"). Every object is drawn in the world's 3D space with the hill, the nearer before
// the farther. What is placed is kept as text beside the settings (worlds/personal.world;
// SPECIFICATIONS 12.0r): "sieve-personal-world-v1" while everything stands on the ground, its first
// line, then "seed N", then one line an object, "place KIND X Z YAW" (KIND a catalogue id, metres and
// radians, %.9g), in the order they were placed; "sieve-personal-world-v2" as soon as anything is more,
// "place KIND X Y Z YAW PITCH ROLL fixed|physics" (KIND also fav:SHA, char:you or char:SHA), with
// "characters placed" once your characters are moved, "day SECONDS" once the day's length is set
// (IDEAS 19.222), "ground X Z DH" for each cell corner the ground shaper has moved (IDEAS 19.209: the
// change, not the whole grid), "model SHA" for each model of your own in its catalogue (placed as
// model:SHA), "music ..." for music of its own (IDEAS 19.220, set in the Media Player), and every line it does not read kept and written back. It is read back when the world opens (a line it knows
// but cannot read is left out, and said).
#pragma once

#include "sieve/image.hpp" // RgbaImage

#include <SDL3/SDL.h>

#include "sieve/character.hpp"
#include "sieve/worldspace.hpp"
#include "mesh.hpp"
#include "character_look.hpp"
#include "slide_panel.hpp"
#include "favourites.hpp"
#include "dialogs.hpp"

#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace hallway {

class PersonalWorld
{
public:
    // `characters`: yours first where `first_is_yours`, then your store's.
    PersonalWorld(SDL_Window* window, SDL_Renderer* renderer, std::vector<sieve::Character> characters, bool first_is_yours, std::filesystem::path world_file = {}, uint64_t seed = 0);
    // The catalogue of objects, by id ("cube", "pillar", ...), in the order shown.
    static const std::vector<std::string>& catalogue();
    // Empty when every shelf item was read from shelf-items.sieve; else what could not be.
    static std::string catalogue_note();
    // Open: a favourite opened (O), to be read on its own line (opened()). System: a row of the pause menu
    // chosen that is another system's (system(): its id, pause_rows.hpp), to be opened by whoever ran this.
    // GoToWorld: the worlds line's Go to it chosen (below), to be gone to in the hallway (worlds_parts()).
    enum class Result { Back, Quit, Open, System, GoToWorld };
    const std::string& system() const { return system_; }
    // The pause menu (Esc): its rows as pause_rows.hpp gives them for your dimension, the hallway's tools
    // too where Personal Dimensions > Features asks (IDEAS 19.196).
    void set_hallway_tools(bool on) { hallway_tools_ = on; }
    bool pause_open() const { return pause_open_; }
    const std::vector<std::string>& pause_list() const { return pause_rows_; } // (its rows while it is in)
    bool pause_select(const std::string& id); // its row of that name chosen (Item:ID presses); false if none
    const Favourite* opened() const { return opened_.empty() ? nullptr : favourite(opened_); }
    // What it last said at the foot of the screen (O on something that does not open, a line left out, ...).
    const std::string& note() const { return note_; }
    Result run();
    // For pictures and tests: a key as if pressed and held for `seconds`, the frame drawn, and where
    // you stand.
    void hold(SDL_Scancode key, float seconds);
    void press(SDL_Keycode key, SDL_Keymod mod = SDL_KMOD_NONE); // the world's keys, as if typed
    void look(float pitch_degrees) { pitch_ = pitch_degrees * 3.14159265f / 180.0f; } // (for pictures and tests)
    void render();
    void settle_panels() { cat_panel_.settle(), place_panel_.settle(), day_panel_.settle(), worlds_panel_.settle(); } // (a picture: the panels where they are going)
    // Wireframe on: the characters drawn as the wireframe (rings and bones), as everywhere else.
    void set_wireframe(bool on) { wireframe_ = on; }
    // How the characters are drawn with Wireframe off (IDEAS 19.214): [character] drawing, every space's
    // one setting (character_look.hpp); shaded by default, as they always were here.
    void set_character_drawing(int drawing) { char_drawing_ = drawing; }
    // How big their voxels are and whether the voxels' faces are joined (IDEAS 19.216, 19.215).
    void set_voxels(const VoxelChoice& v) { voxels_ = v; }
    // (for pictures and tests) what the readout says of it: empty for shaded (as before the choice came).
    std::string drawing_report() const;
    // Its features (IDEAS 19.197), as Personal Dimensions > Features has them: the map and the compass.
    void set_features(bool map, bool compass) { show_map_ = map, show_compass_ = compass; }
    // Turning what you hold (IDEAS 19.221), as Personal Dimensions > Features and Settings > Graphics have
    // it: the rings round it on or off, how far a notch of the wheel turns it (degrees), and the axes'
    // colours, x, y and z (opaque ARGB; AppSettings::axis_colours).
    void set_turning(bool rings, int wheel_step_degrees, const uint32_t axis_argb[3]);
    // The mouse wheel, `notches` of it (forward positive; a touchpad's fractions add up): what you hold
    // turned about the turn last chosen (R, T or G), a step a notch. Nothing while no turn is chosen.
    void wheel(float notches);
    // For the readout: while something is held, the turn chosen, its three turns and its rings; else "".
    std::string turning_report() const;
    // Day and night (IDEAS 19.222), as Personal Dimensions > Features has it: off, the sky as it always
    // was (one sun, fixed, always day); on, the sun crosses the sky once a day of this dimension's own
    // length (its world file's `day` line, kDefaultDayLength seconds without one), and the sky's colours,
    // the light on everything and the stars at night follow it. The time of day is worked out each frame
    // from the clock (seconds since 1 January 1970, UTC) and the length, never added up, so two people in
    // the same dimension see the same sky and nothing runs over however long it is left open.
    static constexpr uint64_t kDefaultDayLength = 1440; // (24 minutes: an hour of the day a minute)
    void set_day_night(bool on) { day_on_ = on; }
    bool day_night() const { return day_on_; }
    // For pictures and tests: the time of day held at `hours` (0 to 24 on the dimension's own clock,
    // 0 midnight, 12 noon, whatever the day's length), not the clock's; below 0, the clock's again.
    void set_time_of_day(double hours) { fixed_phase_ = hours < 0 ? -1.0 : hours / 24.0; }
    uint64_t day_length() const { return day_length_; }
    // A day's length as typed: whole seconds, 1 to 2^64 - 1, digits only. False (and `out` untouched)
    // for anything else, a number past the largest an unsigned 64-bit count holds among them: refused,
    // never wrapped round.
    static bool parse_day_length(const std::string& digits, uint64_t& out);
    // For the readout: while day and night is on, the day's length, the time of day and the sun; else "".
    std::string day_report() const;
    // The author's tools (IDEAS 19.209), the first the ground shaper: its key takes it out, another puts it
    // away; while it is out, the ground where the crosshair meets it is raised, lowered or flattened in a
    // soft round brush of this radius (metres), at most this many metres a second at its middle, less
    // towards its edge. Both start from the settings ([personal] shaper_radius, shaper_strength); the
    // wheel and the keys change the radius for the visit.
    static constexpr int kShaperRadiusMin = 10, kShaperRadiusMax = 80, kShaperStrengthMin = 1, kShaperStrengthMax = 20;
    void set_shaper(int radius_m, int strength_m_s);
    bool shaper_out() const { return tool_out_; }
    // For the readout: while the shaper is out, its radius and strength and where it is aimed; else "".
    std::string shaper_report() const;
    // Favourites' covers in the catalogue (IDEAS 19.190): each favourite's own picture (favourites/SHA.png,
    // the one its front shows once placed) small beside its row, and the chosen one's larger beside the
    // list, so they can be told apart before placing. On by default ([personal] catalogue_covers).
    void set_catalogue_covers(bool on) { covers_on_ = on; }
    // For the readout: while the catalogue is out with a favourite in it, the covers' setting, how many
    // of its favourites have a cover and the chosen row's; else "".
    std::string covers_report() const;
    // Your own objects (IDEAS 19.187's next step): a model (.obj with its .mtl, or a glTF, .gltf or .glb)
    // brought in from a file, kept in the store beside the settings as objects/SHA.sieve (SHA the SHA-256
    // of the .obj it is kept as; SPECIFICATIONS 12.0r), named by a `model SHA` line of the world and
    // listed in the catalogue after your characters as "model:SHA", drawn from its own mesh. The
    // catalogue's import key opens the system's file dialog; --pworld-import FILE brings one in for a run.
    // True where it is now in the catalogue (kept now, or already); note() says what came in or why not.
    bool import_model(const std::string& path);
    // For the readout: your own models in the catalogue, how many could be drawn, and each by its SHA's
    // first twelve digits, its name and its triangles (or why not); "" while there are none.
    std::string models_report() const;
    // Its music (IDEAS 19.220): the world file's `music` line (SPECIFICATIONS 12.0r), set in the Media
    // Player's PERSONAL space, which plays it while the dimension is open; "" where it has none (it then
    // plays WORLD's music, as every dimension did before). Given to the music player as you arrive.
    const std::string& music_line() const { return music_line_; }
    // The line written into a world file as the Media Player changes it ("" takes it away), the rest of
    // the world read and written back as the world itself would write it: "" or why it was not.
    static std::string write_music(const std::filesystem::path& world, const std::string& line);
    // Whether a world file is in its canonical form (the formats' check, IDEAS 19.185): read with
    // nothing left out, and written back the same, byte for byte (written over itself, so given a copy).
    // "" where it is; else why not.
    static std::string round_trip(const std::filesystem::path& world);
    // A world file's `music` line, read without opening the world (the Media Player's PERSONAL as the
    // application starts); "" where it has none or it cannot be read.
    static std::string music_line_of(const std::filesystem::path& world);
    // For the readout: what its music is, where it has some; else "".
    std::string music_report() const;
    // The hallway's own viewer, opened in place over this world (IDEAS 19.208). The viewer belongs to a
    // built hallway (client/viewer.cpp reads a line's items through it), and this world builds none, so
    // whoever runs it lends one: O on a placed favourite asks it to make that favourite's line (in the
    // background, as the hallway's calculating screen does, kept once made), a small "calculating..."
    // stands by the crosshair while it is made and the world goes on, and then the viewer opens over
    // the world; Esc or Z closes it back to the world. Where it cannot be opened so (the line made
    // under other settings than the favourite was kept with, or the making failed), O goes the old way,
    // into the hallway on its line (Result::Open), and the note and the readout say why. With none
    // lent ([personal] open_in_place off), O goes the old way at once.
    class InPlace
    {
    public:
        enum class State { Idle, Making, Open, Failed };
        virtual ~InPlace() = default;
        virtual void begin(const Favourite& f) = 0; // the line made (or the one kept), then the favourite opened
        virtual State poll() = 0;                   // each frame: how it stands (Failed once, then Idle)
        virtual void cancel() = 0;                  // Esc while it is made: not opened when it is (still kept)
        virtual const std::string& why() const = 0; // why it failed
        virtual void event(const SDL_Event& e) = 0; // while Open: the viewer's keys and mouse
        virtual void draw(float W, float H) = 0;    // while Open: the viewer, over the world
        virtual std::string report() const = 0;     // for the readout: the viewer's tab, and whether the line was kept
    };
    void set_in_place(InPlace* v) { in_place_ = v; }
    // For the readout: while a favourite is made or open in place, or after it fell back; else "".
    std::string in_place_report() const;
    bool in_place_making() const { return in_place_state_ == InPlace::State::Making; }
    void in_place_frame(); // its state taken from what was lent (run() each frame; a picture between keys)
    // Your world as one item of the worlds line (IDEAS 19.187's "the world itself as an address on the
    // WORLDS line"; sieve/worldspace.hpp, worldspace-v1). A world of that line is a cover, a title and N
    // slots, each a model of the models line (V vertices on a grid of C steps, F triangles) in one cell of
    // a G x G x G grid, turned by one of the 24 turns that take a cube onto itself. What is placed here
    // is freer than that, so the unit is worked out as the importers work out theirs, saying what is
    // carried and what is left out:
    //   - each object (a shelf item, a favourite, a model of your own) is carried as its box: the eight
    //     corners of its mesh's bounds, in its own turns, on the models line's grid, and its twelve faces
    //     (a models line of fewer than 8 vertices or 12 faces cannot hold a box: then only its place);
    //     more vertices repeat the last corner, more faces are left empty (all three at vertex 0);
    //   - its place is the cell its box's middle falls in, the grid laid over everything carried with
    //     cells as wide as the largest box or the arrangement over G, whichever is wider;
    //   - its turn is the nearest of the 24 to its yaw, pitch and roll;
    //   - in the order placed, the first N; slots left over are model 0 in cell 0, unturned, as
    //     `sieve world --compose` fills them; the cover and the title are left at 0.
    // Left out, and said: your characters (the line holds models, not people), each object's own mesh and
    // colours, which item a favourite is, the objects past the N slots, dropped or placed, the hill, the
    // sky and the day, the music. The hallway's worlds line is lent as a WorldSpace made from the settings
    // (whoever runs this world makes it; without one the key says there is none).
    struct WorldsLine
    {
        std::shared_ptr<const sieve::WorldSpace> space; // null where it could not be made
        sieve::AddressMode mode = sieve::AddressMode::Positional; // the ordering the hallway shows it in
        std::string why;                                          // (why not, where it could not)
    };
    void set_worlds_line(std::function<WorldsLine()> f) { worlds_line_of_ = std::move(f); }
    struct WorldsUnit
    {
        sieve::WorldSpace::Parts parts;
        size_t objects = 0;    // objects placed (not counting your characters)
        size_t carried = 0;    // of them, in the slots
        size_t characters = 0; // your characters standing in the world, left out
        size_t favourites = 0, own = 0, dropped = 0; // of those carried: favourites, models of your own, dropped
        bool boxes = true;     // the models line holds a box (else only the places are carried)
        float cell = 0;        // a cell's width, metres
        float off = 0;         // the farthest a carried box's middle is from its cell's middle, metres
        float turn_off = 0;    // the most a carried turn is from the nearest of the 24, degrees
    };
    // One object as the worlds line sees it: its box's middle in the world, its half-widths in its own
    // frame (metres) and its turn (columns: where its own x, y and z go in the world).
    struct WorldsBox
    {
        float centre[3] = {0, 0, 0}, half[3] = {0, 0, 0};
        float turn[3][3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
    };
    // The unit of those boxes, in that order (the counts of what they are left for the caller).
    static WorldsUnit worlds_unit(const sieve::WorldSpace& ws, const std::vector<WorldsBox>& boxes);
    WorldsUnit worlds_unit(const sieve::WorldSpace& ws) const;
    // The worlds line's Go to it chosen (Enter in its panel): its parts, for the hallway to go to.
    const std::optional<sieve::WorldSpace::Parts>& worlds_parts() const { return worlds_go_; }
    // For the readout: once the panel has been opened, the address and what was carried; else "".
    std::string worlds_report() const;
    std::string report() const;
    float height(float x, float z) const; // the ground's height there, in metres (the seed's, and what is shaped)
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
        // (worked out from its kind and turns, kept until they change: extent_of; and its faces in the
        // world, kept until its place changes too: faces_of. The physics and the crosshair ask for them
        // many times a frame; review 1, 14 November 2026.)
        mutable std::string ext_kind, face_kind;
        mutable float ext_turns[3] = {NAN, NAN, NAN}, ext[3] = {0, 0, 0};
        mutable float face_at[6] = {NAN, NAN, NAN, NAN, NAN, NAN};
        mutable std::vector<std::pair<std::array<std::array<float, 3>, 3>, SDL_FColor>> faces;
    };
    std::vector<Placed> placed_;
    std::vector<std::string> kept_lines_; // a v2 file's lines this build does not know, written back unchanged
    std::string music_line_;              // its `music` line (IDEAS 19.220), or "" for none
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
    void extent_fresh(const Placed& p, float& low, float& high, float& across) const;
    std::filesystem::path file_;
    // The panels slide in from the left as the setup menu's do (slide_panel.hpp), one at a time: the
    // catalogue, and while something is in your hand, Place (its exact place and turns).
    SlidePanel cat_panel_, place_panel_;
    // The pause menu (Esc where nothing is open or held).
    bool pause_open_ = false, hallway_tools_ = false;
    int pause_row_ = 0;
    std::vector<std::string> pause_rows_;
    std::vector<SDL_FRect> pause_rects_;
    std::string system_;        // a system's row chosen (Result::System)
    bool leave_ = false, quit_ = false; // Leave your dimension / Exit Sieve chosen
    void escape();              // Esc: a panel closed, what is held put back, else the pause menu in or out
    void pause_key(SDL_Keycode k);
    void pause_choose(int row);
    void draw_pause();
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
    void pick_up(int which); // into your hand, no turn chosen yet
    // Turning what you hold (IDEAS 19.221): the turn last chosen, 0 the yaw (R, about the vertical: y),
    // 1 the pitch (T, about its side: z), 2 the roll (G, about its front: x), or -1 none since it was
    // picked up; the wheel's step (radians) and its part-notches not yet turned; the rings and colours.
    int turn_ = -1;
    float wheel_step_ = 15.0f * 3.14159265f / 180.0f, wheel_left_ = 0;
    bool turn_rings_ = true;
    uint32_t axis_argb_[3] = {0xFFFF4040u, 0xFF40D040u, 0xFF4080FFu};
    // Where the rings go: the middle of what is held (its box's centre, in the world) and the farthest of
    // its points from there, in metres; false while nothing is held.
    bool held_bounds(float centre[3], float& reach) const;
    void add_rings(); // the rings, given to the renderer with the objects (so the thing held hides their far side)
    // Day and night (IDEAS 19.222): on or off, the day's length (seconds) and whether the file said it
    // (or it was typed here: then it is written back, as a v2 line), the time of day held for a picture
    // (a fraction of the day, or below 0 for the clock's), and the panel where the length is typed.
    bool day_on_ = false, day_written_ = false;
    uint64_t day_length_ = kDefaultDayLength;
    double fixed_phase_ = -1.0;
    SlidePanel day_panel_;
    std::string day_edit_;
    void day_key(SDL_Keycode k, SDL_Keymod mod);
    // The time of day now, as a fraction of the day (0 midnight, 0.25 sunrise, 0.5 noon, 0.75 sunset).
    double day_phase() const;
    // The sky at a time of day: where the sun is (a unit vector, y up), the sky's colours overhead and at
    // the horizon, the colour of the light on things, how much of the day's light there is (0 night, 1
    // day) and how bright the stars are (0 to 1).
    struct Sky
    {
        float sun[3];
        SDL_FColor top, low, light;
        float day, stars;
    };
    Sky sky_at(double phase) const;
    // The stars: directions on the sky (unit vectors, as at midnight's turn of it), from the seed.
    std::vector<std::array<float, 3>> stars_;
    // The map's zoom (IDEAS 19.199's first keys): 0 the whole world, each step in halves what it shows.
    static constexpr int kMapZooms = 5;
    int map_zoom_ = 0;
public:
    float map_across() const { return 2 * 320.0f / float(1 << map_zoom_); } // (metres the map shows across)
private:
    // An object's faces, in the world: triangles (three points each) and their colour.
    using Face = std::pair<std::array<std::array<float, 3>, 3>, SDL_FColor>;
    const std::vector<Face>& faces_of(const Placed& p) const; // (kept on p until it moves or turns)
    // Your own objects: their SHAs in the order the world names them (its `model` lines, then those
    // brought in since), each one's mesh read from the store the first time it is asked for (null where it
    // cannot be: not there, not as named, no faces), stood as every object is (its foot at y = 0, its
    // middle over the place), with the name of the .obj it is kept as, or why not.
    struct OwnModel
    {
        std::shared_ptr<const Mesh> mesh;
        std::string name, why;
    };
    std::vector<std::string> models_;
    mutable std::map<std::string, OwnModel> model_meshes_;
    static bool is_model(const std::string& kind) { return kind.rfind("model:", 0) == 0; }
    const OwnModel& own_model(const std::string& sha) const;
    std::filesystem::path store() const { return file_.parent_path().parent_path(); } // (beside the settings)
    // The mesh any kind is drawn from: a shelf item's, a favourite's line's, your own model's; null for
    // none (a character, a model not in the store).
    const Mesh* mesh_of(const std::string& kind) const;
    // The import key's file dialog: it answers on a thread of its own, so the file chosen waits here
    // until run() takes it; the mouse is given back to the dialog while it is open.
    DialogOwner dialog_owner_{this};
    std::mutex import_mx_;
    std::string import_from_;
    std::atomic<bool> import_open_{false};
    bool import_waiting_ = false; // (the dialog opened and not yet seen to answer: run()'s, on its own thread)
    void import_dialog();
    static void SDLCALL import_chosen(void* self, const char* const* files, int);
    // Favourites (IDEAS 19.190): this dimension's, read beside its world file, in the catalogue after
    // Sieve's shelf items as "fav:SHA" and drawn as their line's shelf item.
    std::vector<Favourite> favourites_;
    std::vector<std::string> rows_;  // the catalogue as shown: the shelf items' ids, "fav:SHA" each, your characters, "model:SHA" each
    std::string opened_;             // the favourite O opened, by its SHA
    // In place (IDEAS 19.208): what lends the viewer (none: the old way), how it stands, the favourite
    // asked for, when it was asked (for the dots by the crosshair) and, for the readout, how the last
    // one ended (closed, not opened, or the old way taken and why).
    InPlace* in_place_ = nullptr;
    InPlace::State in_place_state_ = InPlace::State::Idle;
    std::string in_place_sha_, in_place_last_;
    uint64_t in_place_since_ = 0;
    const Favourite* favourite(const std::string& sha) const;
    std::string mesh_kind(const std::string& kind) const; // the shelf item a kind is drawn as
    std::string label_of(const std::string& kind) const;  // what it is called on the screen
    bool project(float x, float y, float z, float& sx, float& sy, float& depth) const;
    // Your eye as the hallway's Camera (where you stand, your turns, this world's field of view and
    // near plane): the ground, the clouds and the objects are all seen through it.
    Camera eye_camera() const;
    Vec3 eye_space(float x, float y, float z) const; // (the ground's and the clouds' turn into your eye)
    SDL_Window* window_;
    SDL_Renderer* r_;
    // Your characters are objects of the world like any other (Edward, 14 November 2026: "the fixed spot is
    // fine so long as it is an object that can be moved around and exists in the catalog"): in the
    // catalogue after the favourites, as "char:you" (yours, whichever it is now) and "char:SHA" (one of
    // your store's, by its SHA-256), picked up, turned, placed, dropped and taken away as the rest are,
    // and drawn as their spheres. A world that never had them placed by hand (no "characters placed"
    // line) stands them where they always stood: yours ahead of where you begin, the store's round the
    // hilltop; while they stay there, nothing of them is written, so such a world is still v1.
    std::vector<sieve::Character> characters_;
    std::vector<std::string> char_ids_; // (each one's SHA-256, worked out once)
    bool first_is_yours_ = false;
    bool characters_placed_ = false;     // the file said where they are ("characters placed")
    std::vector<Placed> default_chars_;  // where they stand when it does not
    const sieve::Character* character(const std::string& kind) const; // a "char:" kind's, or null (not here)
    static bool is_char(const std::string& kind) { return kind.rfind("char:", 0) == 0; }
    bool chars_at_default() const;
    float char_hit(const Placed& p, const float o[3], const float d[3]) const; // a ray's distance to its spheres, or -1
    uint64_t seed_;
    std::vector<Swell> swells_;
    // The hill's height at every cell corner, and each cell's shade of grass, worked out once from the
    // swells (they never change): the ground is drawn and stood on from these.
    std::vector<float> corner_h_, cell_vary_;
    // The ground shaper (IDEAS 19.209). What is shaped is kept as a change to the height at each cell
    // corner (metres, added to the seed's; the world file's `ground` lines), empty until anything is: so
    // a world never shaped has the seed's ground exactly, worked out as it always was.
    std::vector<float> shaped_;
    float seed_height(float x, float z) const; // the swells' alone
    float shaped_at(float x, float z) const;   // the change there, between the corners as the ground is drawn
    size_t shaped_corners() const;             // corners changed
    bool tool_out_ = false;
    bool shape_keys_wait_ = false; // the shaper's keys shape nothing until let go (one closed the viewer)
    float shaper_radius_ = 20, shaper_strength_ = 4;
    int mouse_shape_ = 0;          // a mouse button held while the shaper is out: 1 raise, 2 lower, 3 flatten
    bool shaped_unsaved_ = false;  // shaped since the last save (saved when the button or key is let go)
    // The stroke under way: where it shapes, and where you stood and looked when it was aimed there.
    bool stroke_on_ = false;
    float stroke_x_ = 0, stroke_z_ = 0;
    std::array<float, 4> stroke_view_{};
    // Where the crosshair meets the ground, within reach; false where it meets only sky.
    bool aim_ground(float& x, float& z) const;
    void shape(int mode, float dt, float cx, float cz);
    void draw_shaper();
    // The worlds line (IDEAS 19.187): what lends it, its panel, what it last worked out (or why not)
    // and its address in the hallway's ordering and the other, and the Go to it chosen.
    std::function<WorldsLine()> worlds_line_of_;
    SlidePanel worlds_panel_;
    bool worlds_asked_ = false;
    std::optional<WorldsUnit> worlds_;
    std::string worlds_why_, worlds_hex_, worlds_other_hex_;
    sieve::AddressMode worlds_mode_ = sieve::AddressMode::Positional;
    uint32_t worlds_grid_ = 0, worlds_slots_ = 0; // (the line's G and N, for the panel and the readout)
    std::optional<sieve::WorldSpace::Parts> worlds_go_;
    void open_worlds();
    void worlds_key(SDL_Keycode k, SDL_Keymod mod);
    void draw_worlds();
    std::vector<Cloud> clouds_;
    float px_ = 0, pz_ = -60, yaw_ = 0, pitch_ = -0.05f; // where you stand, and which way you look
    bool wireframe_ = false;
    int char_drawing_ = 3;   // [character] drawing (character_look.hpp's kDrawShaded)
    size_t char_tris_ = 0;   // the characters' cubes' triangles in the last frame drawn
    size_t char_unjoined_ = 0; // the same with each face on its own (for the readout, where joined)
    VoxelChoice voxels_;       // [character] voxel_size and the rows beside it
    bool show_map_ = true, show_compass_ = true;
    void draw_map();
    void draw_compass();
    // Whether you may pick things up and place them here (an author) or only open them (a visitor):
    // every world is yours alone until identity exists (IDEAS 19.178), so always an author for now.
    bool author_ = true;
    float wind_ = 0;                                     // how far the clouds have drifted
    float W_ = 1280, H_ = 720;
    // The objects placed, drawn by the hallway's renderer (MeshBatch, client/mesh.hpp: a test of depth
    // at every pixel) over the ground (IDEAS 19.207; Edward's testing, 14 November 2026: faces sorted by
    // their middles popped in and out).
    MeshBatch batch_;
    // A favourite's picture (favourites/SHA.png, kept by the hallway as it draws it), read once; null
    // where it has none.
    std::shared_ptr<const sieve::RgbaImage> picture_of(const std::string& sha) const;
    bool picture_quad(const Placed& p, std::array<std::array<float, 3>, 4>& w, std::array<float, 3>& n) const;
    mutable std::map<std::string, std::shared_ptr<const sieve::RgbaImage>> pictures_;
    mutable std::map<std::string, FaceRect> face_rects_; // (faces.ini read once a medium)
    // The catalogue's covers: a texture a favourite, made from its picture the first time its row is
    // drawn (null where it has none) and kept until the dimension is left.
    bool covers_on_ = true;
    SDL_Texture* cover_of(const std::string& sha);
    std::map<std::string, SDL_Texture*> covers_;

public:
    ~PersonalWorld();
    PersonalWorld(const PersonalWorld&) = delete;
    PersonalWorld& operator=(const PersonalWorld&) = delete;
};

} // namespace hallway
