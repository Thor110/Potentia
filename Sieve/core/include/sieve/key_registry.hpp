// Sieve — every key in one registry (IDEAS 19.199; Edward, 13 November 2026: "when we add new key press
// handlers to new systems, the system itself has a primary tag and it can funnel/filter itself upstream to
// the controls listings ... so that all keys are adjustable through and through").
//
// The part of the registry that needs no window, here in the core so the unit tests can check it: chords
// read and written, actions declared by section, asked by what they do, their keys kept as overrides, and
// clashes found. The hallway's client/keys.hpp builds on it, adding SDL's key names, its events and the
// keyboard's held keys; this is the one copy of the rest.
//
// A system names each thing a key does where the system is written, as a KeyAction: its section (the
// system's tag, a path such as "personal.minimap"), its id within the section, the language key of its
// name, and its default keys. It then asks the registry whether a key press is that action rather than
// testing key codes itself. So every action is listed, by section, on Settings > Controls > Keys, where
// any of them can be given other keys; and the footers that name keys read them from here.
//
// Keys are written by their names ("E", "Return", "Keypad +"), with "Shift+", "Ctrl+" or "Alt+" before
// one that needs them, several apart by commas: "W,Up". "Comma" and "Plus" stand for the two keys a
// comma list cannot hold. A change is kept in the settings file under [keys] as "SECTION/ID = KEYS"
// (only where it differs from the default), and read back at start.
#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace sieve {

// The modifiers a chord tells apart, either side of the keyboard: the same bits as SDL's (the client
// checks that they are), so a key event's modifiers can be given as they come.
inline constexpr uint16_t kKeyShift = 0x0003, kKeyCtrl = 0x00C0, kKeyAlt = 0x0300;

struct KeyChord
{
    uint32_t key = 0;  // the key's code (0: none); SDL's codes in the hallway
    uint16_t mod = 0;  // only Shift, Ctrl and Alt (either side) are told apart
};

struct KeyAction
{
    std::string section;  // e.g. "personal.minimap"
    std::string id;       // e.g. "zoom_in"; with the section, its name in the settings file
    std::string name_key; // the language key of what it does
    std::string defaults; // its default keys, as written above
};

// How key names become codes and back. The hallway gives SDL's; without one, a single printable
// character names the key that types it (a letter's code is the small letter, its name the capital, as
// SDL has them), which is all the unit tests and a program without a keyboard need.
struct KeyNaming
{
    std::function<uint32_t(const std::string&)> code; // 0 for a name it does not know
    std::function<std::string(uint32_t)> name;        // "" for a code it does not know
};
void set_key_naming(KeyNaming naming);

// "Shift+R" and the like: one chord, or key 0 if it is not a key's name.
KeyChord parse_chord(const std::string& text);
std::vector<KeyChord> parse_chords(const std::string& text);
std::string chord_name(const KeyChord& c);
// Only Shift, Ctrl and Alt of an event's modifiers, each as both sides (what a chord holds).
uint16_t plain_mods(uint16_t mod);

class KeyRegistry
{
public:
    // Declared once, where the system is (a second declaration of the same section and id is ignored).
    // Returns the full id ("SECTION/ID") to ask by.
    const std::string& declare(const KeyAction& a);
    // Whether a key press is this action (its key, and its modifiers exactly).
    bool is(const std::string& full_id, uint32_t key, uint16_t mod) const;
    // Whether a key press is this action with its modifiers held and perhaps others too (Shift held to
    // run and E pressed is still E). A screen asking this asks is() first over all its actions, so that a
    // chord with more modifiers given to another action is that action.
    bool within(const std::string& full_id, uint32_t key, uint16_t mod) const;
    // Of a screen's actions, the one a key press is: one whose chord it is exactly, else the one with a
    // chord it is within that asks the most modifiers (Shift held and - pressed is "zoom out eight times"
    // before "zoom out"), the first of them on a tie; "" for none.
    std::string which(const std::vector<std::string>& full_ids, uint32_t key, uint16_t mod) const;
    // Its keys as set, for a footer or help line: "W / Up".
    std::string names(const std::string& full_id) const;
    std::string first(const std::string& full_id) const; // its first key only, for a short footer
    // Whether its keys differ from its defaults (false for one not declared).
    bool changed(const std::string& full_id) const;
    // Its chords as set (empty for one not declared), for the keyboard's held keys.
    std::vector<KeyChord> chords(const std::string& full_id) const;
    // A text with its keys filled in: "{key:SECTION/ID|shown}" becomes `shown` while the action has its
    // default keys (so a footer reads as it was written) and its keys as set (names()) once they are
    // changed; "{key1:...}" the same with its first key only. One not declared keeps `shown`.
    std::string keyed(const std::string& text) const;
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

} // namespace sieve
