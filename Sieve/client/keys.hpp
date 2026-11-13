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
    // Actions in one section whose keys are the same: "personal dimension: E is pick up and open".
    std::vector<std::string> clashes() const;

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
    const Entry* find(const std::string& full_id) const;
};

} // namespace hallway
