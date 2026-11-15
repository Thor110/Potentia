// Sieve hallway — every key in one registry (IDEAS 19.199; Edward, 13 November 2026: "when we add new key
// press handlers to new systems, the system itself has a primary tag and it can funnel/filter itself
// upstream to the controls listings ... so that all keys are adjustable through and through").
//
// The registry itself (actions declared by section, asked by what they do, their keys kept as [keys]
// overrides, clashes found) is the core's, sieve/key_registry.hpp, where the unit tests reach it. Here it
// is given SDL's key names ("E", "Return", "Keypad +"), and asked of SDL's key events and of the
// keyboard's held keys. Every system's actions are listed, by section, on Settings > Controls > Keys,
// where any of them can be given other keys; the footers that name keys read them from here (or through
// "{key:SECTION/ID|shown}" in a language string, trf and trk), so they show the keys as they are set.
#pragma once

#include "sieve/key_registry.hpp"

#include <SDL3/SDL.h>

#include <string>
#include <vector>

namespace hallway {

// (the window-free part is the core's, sieve/key_registry.hpp, so the unit tests check it)
using sieve::chord_name;
using sieve::KeyAction;
using sieve::KeyChord;
using sieve::parse_chord;
using sieve::parse_chords;

// The registry with SDL's key names, its key events and the keyboard's held keys.
class Keys : public sieve::KeyRegistry
{
public:
    // The one registry of the program (its key names SDL's from the first call on).
    static Keys& get();
    using sieve::KeyRegistry::is;
    bool is(const std::string& full_id, const SDL_Event& e) const;
    // The same with its modifiers held and perhaps others too (Shift held with Left is still Left, as the
    // menus' keys always were, where Shift changes by ten), and of a screen's actions the one a key press
    // is (see the core's which()): both "" or false for anything but a key press.
    using sieve::KeyRegistry::within;
    bool within(const std::string& full_id, const SDL_Event& e) const;
    using sieve::KeyRegistry::which;
    std::string which(const std::vector<std::string>& full_ids, const SDL_Event& e) const;
    // Whether any of its keys is held now (for walking and the like; modifiers are not asked).
    bool held(const std::string& full_id, const bool* keyboard) const;
    // Whether any key of a section, or of the sections under it ("setup" holds "setup.side"), differs
    // from its default: a readout names a screen's keys as set only then, so with the defaults it is as
    // it was.
    bool changed_in(const std::string& section) const;
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

// The hallway's own keys (IDEAS 19.199), declared after the Creator's (personal_world.cpp): walking (held),
// whatever you hold, with empty hands, holding something, and typing in the warp and go-to boxes.
namespace hallway_keys {
extern const std::string& kForward;
extern const std::string& kBack;
extern const std::string& kLeft;
extern const std::string& kRight;
extern const std::string& kTurnLeft;
extern const std::string& kTurnRight;
extern const std::string& kRun;
extern const std::string& kTake;
extern const std::string& kWarp;
extern const std::string& kGoTo;
extern const std::string& kNavigator;
extern const std::string& kGraph;
extern const std::string& kOrdering;
extern const std::string& kZoomOut;
extern const std::string& kZoomIn;
extern const std::string& kZoomOutFar;
extern const std::string& kZoomInFar;
extern const std::string& kThousandOn;
extern const std::string& kThousandBack;
extern const std::string& kHome;
extern const std::string& kMouse;
extern const std::string& kMenu;
extern const std::string& kQuit;
extern const std::string& kTrailNext;
extern const std::string& kTrailBack;
extern const std::string& kMillionOn;
extern const std::string& kMillionBack;
extern const std::string& kNextPage;
extern const std::string& kPreviousPage;
extern const std::string& kSpinLeft;
extern const std::string& kSpinRight;
extern const std::string& kNextTab;
extern const std::string& kUpright;
extern const std::string& kPlay;
extern const std::string& kNextMap;
extern const std::string& kPreviousMap;
extern const std::string& kAnchor;
extern const std::string& kSave;
extern const std::string& kItsFile;
extern const std::string& kCipher;
extern const std::string& kTailor;
extern const std::string& kFurther;
extern const std::string& kUse;
extern const std::string& kFavourite;
extern const std::string& kView;
extern const std::string& kTypedGo;
extern const std::string& kTypedErase;
extern const std::string& kTypedPaste;
} // namespace hallway_keys

// The menus' navigation, one shared section every list reads (IDEAS 19.199, put on the registry on 14
// November 2026), so a change to it is a change on every list that reads it: a row up or down, a setting
// less or more, choose, Space (tick or change, where a list takes it), back (Esc), the next area (Tab) and
// a page up or down. Declared after the hallway's (personal_world.cpp), as the systems below are.
namespace menu_keys {
extern const std::string& kUp;
extern const std::string& kDown;
extern const std::string& kLeft;
extern const std::string& kRight;
extern const std::string& kChoose;
extern const std::string& kToggle;
extern const std::string& kBack;
extern const std::string& kNext;
extern const std::string& kPageUp;
extern const std::string& kPageDown;
} // namespace menu_keys

// The setup menu's own keys (IDEAS 19.199), besides the menus' above and the Character Creator's: the
// list's (the alphabets, a line's filters and details, a character of the key taken back, the first and
// last alphabet), the filters window's three toggles, the right-hand panel's (the Character Creator, the
// tabs, YOURS' playground and keep) and the catalogue's (SYSTEMS and the System Viewer).
namespace setup_keys {
extern const std::string& kAlphabets;
extern const std::string& kFilters;
extern const std::string& kDetails;
extern const std::string& kErase;
extern const std::string& kFirst;
extern const std::string& kLast;
extern const std::string& kThisTab;
extern const std::string& kEveryLine;
extern const std::string& kBothTabs;
extern const std::string& kCreator;
extern const std::string& kPreviousTab;
extern const std::string& kNextTab;
extern const std::string& kPlayground;
extern const std::string& kKeep;
extern const std::string& kDoorBack;
extern const std::string& kRoot;
extern const std::string& kOtherSystem;
extern const std::string& kOntoLine;
} // namespace setup_keys

// The main menu's and Settings' keys (IDEAS 19.199), besides the menus': back as Esc is (Backspace), and a
// drop-down list's first and last rows. (Settings > Controls > Keys' own Enter, Backspace and Esc on a key's
// row stay fixed, so a key can always be given back.)
namespace main_keys {
extern const std::string& kBack;
extern const std::string& kFirst;
extern const std::string& kLast;
} // namespace main_keys

// The Encryption Engine's keys (IDEAS 19.199), besides the menus' (Tab there puts the output in as the
// input): a character taken back (Ctrl: the row cleared), the item in hand, a file, save, go to it, paste
// and copy.
namespace cipher_keys {
extern const std::string& kErase;
extern const std::string& kItem;
extern const std::string& kFile;
extern const std::string& kSave;
extern const std::string& kGo;
extern const std::string& kPaste;
extern const std::string& kCopy;
} // namespace cipher_keys

// The Documentation's keys (IDEAS 19.201), besides the menus' (Up, Down, Left, Right, Enter, Tab, PgUp,
// PgDn, Esc; Tab moving to the next tab on the list, or the next link on the page): the tab or the link
// before, a search, back to the page read before, and the first and the last row or line.
namespace help_keys {
extern const std::string& kPrevious;
extern const std::string& kSearch;
extern const std::string& kHistory;
extern const std::string& kFirst;
extern const std::string& kLast;
} // namespace help_keys

// The CHAIN tab's keys (IDEAS 19.199), asked before the item page's (a key none of them is goes on to the
// page): a stage up or down, moved up or down, its model the one before or the next, steered by the
// filters, one added after it or taken away, run from CHAT, and a chain saved or opened.
namespace chain_keys {
extern const std::string& kUp;
extern const std::string& kDown;
extern const std::string& kMoveUp;
extern const std::string& kMoveDown;
extern const std::string& kPreviousModel;
extern const std::string& kNextModel;
extern const std::string& kSteer;
extern const std::string& kAdd;
extern const std::string& kRemove;
extern const std::string& kRun;
extern const std::string& kSave;
extern const std::string& kOpen;
} // namespace chain_keys

// The Media Player's keys (IDEAS 19.199) and its rings': W, A, S and D beside the arrows, as they were.
namespace media_keys {
extern const std::string& kUp;
extern const std::string& kDown;
extern const std::string& kLess;
extern const std::string& kMore;
extern const std::string& kChoose;
extern const std::string& kNextColumn;
extern const std::string& kBack;
extern const std::string& kRings;
extern const std::string& kGoTo;
extern const std::string& kSave;
extern const std::string& kFavourite;
extern const std::string& kNext;
extern const std::string& kMelody;
extern const std::string& kRingsClose;
extern const std::string& kRingsZoomIn;
extern const std::string& kRingsZoomOut;
} // namespace media_keys

} // namespace hallway
