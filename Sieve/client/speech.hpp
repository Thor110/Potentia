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

} // namespace hallway
