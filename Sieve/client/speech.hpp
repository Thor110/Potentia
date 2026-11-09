// Sieve hallway -- reading the menus aloud (IDEAS 19.74, 6 November 2026): the item chosen in a
// menu spoken when it changes, as a screen reader would, by the system's own speech (Windows'
// SAPI voice, which every Windows has). Off unless Settings, Controls turns it on. Elsewhere there
// is no voice yet, and nothing is said.
#pragma once

#include <string>

namespace hallway {

void set_read_aloud(bool on);
bool read_aloud();
bool speech_available(); // a voice to speak with (Windows)

// Says `text` (UTF-8) unless it is what was said last: so a menu can call it every frame with the
// item chosen, and it speaks only when the choice (or its value) changes. Whatever is being said
// is cut short by the next.
void announce(const std::string& text);

// Forgets what was said last, so the next announce speaks even the same words: called when a menu
// opens or changes screen, where the item chosen may be the one chosen before (the review of 6
// November 2026: the pause menu reopened on Resume was silent).
void forget_announced();

// A line as it is shown, as it should be said: a tick box ("[x] name", "[ ] name") as "name, on" or
// "name, off", and the spaces that line up columns as one.
std::string spoken(const std::string& shown);

// What was said last (empty if nothing since it was forgotten): scripted runs print it, since a
// machine without a voice says nothing (--read-aloud).
std::string last_announced();

// Text to Voice (Edward, 9 November 2026): the AI's replies in the CHAT tab spoken aloud, by the
// system's voice where there is one (Windows); Sieve's own voice model is to speak them once one is
// trained. Off unless Settings, Controls turns it on. Each reply cuts short the one before.
void set_text_to_voice(bool on);
bool text_to_voice();
void speak_reply(const std::string& text);
// The reply spoken last (empty if none): scripted runs print it, as they print what is read aloud.
std::string last_voiced();

} // namespace hallway
