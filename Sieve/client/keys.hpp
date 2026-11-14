// Sieve hallway — every key in one registry (IDEAS 19.199; Edward, 13 November 2026: "when we add new key
// press handlers to new systems, the system itself has a primary tag and it can funnel/filter itself
// upstream to the controls listings ... so that all keys are adjustable through and through").
//
// A system names each thing a key does where the system is written, as a KeyAction: its section (the
// system's tag, a path such as "personal dimension/minimap"), its id within the section, the language key
// of its name, and its default keys. It then asks the registry whether an event is that action, or
// whether that action's keys are held, rather than testing key codes itself. So every action is listed,
// by section, on Settings > Controls > Keys, where any of them can be given other keys; and the footers
// that name keys read them from here, so they show the keys as they are set.
//
// Keys are written as SDL names them ("E", "Return", "Keypad +"), with "Shift+", "Ctrl+" or "Alt+" before
// one that needs them, several apart by commas: "W,Up". A change is kept in the settings file under
// [keys] as "SECTION/ID = KEYS" (only where it differs from the default), and read back at start.
#pragma once

#include <SDL3/SDL.h>

#include <map>
#include <string>
#include <utility>
#include <vector>

namespace hallway {

struct KeyChord
{
    SDL_Keycode key = SDLK_UNKNOWN;
    SDL_Keymod mod = SDL_KMOD_NONE; // only Shift, Ctrl and Alt (either side) are told apart
};

struct KeyAction
{
    std::string section;  // e.g. "personal dimension/minimap"
    std::string id;       // e.g. "zoom_in"; with the section, its name in the settings file
    std::string name_key; // the language key of what it does
    std::string defaults; // its default keys, as written above
};

// "Shift+R" and the like: one chord, or SDLK_UNKNOWN if it is not a key's name.
KeyChord parse_chord(const std::string& text);
std::vector<KeyChord> parse_chords(const std::string& text);
std::string chord_name(const KeyChord& c);

class Keys
{
public:
    static Keys& get();
    // Declared once, where the system is (a second declaration of the same section and id is ignored).
    // Returns the full id ("SECTION/ID") to ask by.
    const std::string& declare(const KeyAction& a);
    // Whether a key press is this action (its key, and its modifiers exactly).
    bool is(const std::string& full_id, SDL_Keycode key, SDL_Keymod mod) const;
    bool is(const std::string& full_id, const SDL_Event& e) const;
    // Whether any of its keys is held now (for walking and the like; modifiers are not asked).
    bool held(const std::string& full_id, const bool* keyboard) const;
    // Its keys as set, for a footer or help line: "W / Up".
    std::string names(const std::string& full_id) const;
    std::string first(const std::string& full_id) const; // its first key only, for a short footer
    // Every action, in the order declared, and its keys as set.
    struct Row
    {
        std::string full_id, section, name_key, keys;
        bool changed; // differs from its default
    };
    std::vector<Row> listing() const;
    // Set from the settings file ([keys]), or by Settings > Controls > Keys; "" puts back the default.
    void set(const std::string& full_id, const std::string& keys);
    // What differs from the defaults, to be kept in the settings file.
    std::map<std::string, std::string> overrides() const;
    // Two sections whose keys are read on the same screen (the Character Creator's fixed keys and its
    // editing keys), so a key given to one must not be given to the other: checked together by clashes().
    void together(const std::string& section_a, const std::string& section_b);
    // Two actions whose keys are the same, in one section or in two sections said together: the key, and
    // the two actions' full ids (Settings > Controls > Keys names them in red).
    struct Clash
    {
        std::string chord, a, b;
    };
    std::vector<Clash> clashes() const;

private:
    struct Entry
    {
        KeyAction action;
        std::string keys; // as set
        std::vector<KeyChord> chords;
    };
    std::vector<Entry> entries_;
    std::map<std::string, size_t> index_; // full id to entry
    std::map<std::string, std::string> pending_; // set before its system declared it (read from settings first)
    std::vector<std::pair<std::string, std::string>> together_; // sections checked together for clashes
    const Entry* find(const std::string& full_id) const;
};

// The Character Creator's editing keys (IDEAS 19.203): the chosen sphere the next or the one before, a limb
// grown from it or taken away with all beyond it, a joint added after it or it taken out, stretching
// bones on or off, the chosen sphere moved a step up, down, left or right as seen, and the last edit
// undone. Declared after the personal dimension's (personal_world.cpp), so they are listed last.
namespace creator_keys {
extern const std::string& kNextSphere;
extern const std::string& kPreviousSphere;
extern const std::string& kGrow;
extern const std::string& kRemoveLimb;
extern const std::string& kAddJoint;
extern const std::string& kRemoveJoint;
extern const std::string& kStretch;
extern const std::string& kNudgeUp;
extern const std::string& kNudgeDown;
extern const std::string& kNudgeLeft;
extern const std::string& kNudgeRight;
extern const std::string& kUndo;
// The Creator's own keys besides (IDEAS 19.199: every key on the registry), read by the CHARACTER page
// too: play or pause the animation, take it off, turn the view, tilt it, the view as it opens, a seed at
// random (on the held-model row, a model at random), the next character of the seed walk and ten on, the
// chosen sphere's held model taken away, and the files (save, a model, a skeleton, open a character).
extern const std::string& kPlay;
extern const std::string& kTakeOff;
extern const std::string& kTurnLeft;
extern const std::string& kTurnRight;
extern const std::string& kTiltUp;
extern const std::string& kTiltDown;
extern const std::string& kViewHome;
extern const std::string& kRandom;
extern const std::string& kNext;
extern const std::string& kNextTen;
extern const std::string& kEmptySphere;
extern const std::string& kSave;
extern const std::string& kImportModel;
extern const std::string& kSkeleton;
extern const std::string& kOpen;
} // namespace creator_keys

} // namespace hallway
