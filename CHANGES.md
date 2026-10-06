# The music plays melodies, and can draw them from the tracks line

Against `83093ba` (joined filters). This includes the earlier `hand.zip` (the model comes back
upright, and the NOW PLAYING box moved down 21 px), so apply this one instead of that one.

## What changed

### NOW PLAYING box
It is 21 px lower and 6 px further right (`y = 73`, right edge at `W - 6`). It now sits just below
the FPS counter, flush with its right edge, and clear of the setup menu's memory rows.

### "Melody", not "track", for what the music plays
The NOW PLAYING message already said "a melody", but the Media Player's own labels said "track".
They now say melody: RECENT MELODIES, "A new melody now", "Quiet between melodies", "Play the
highlighted melody", and so on. The same change was made in the README and the code
(`MusicMelody`, `go_to_melody()`, `Request::GoToMelody`), so "track" now means only the TRACKS
line. `sieve-music.ini` writes recent entries as `melody =` and still reads the old `track =`.

### Draws from: the audio line or the tracks line
- **Two new rows per mode (MENUS, WORLD) in the Media Player:**
  - **Draws from:** *the audio line* (as before) or *the tracks line*.
  - **units a track:** 1–64, used only for the tracks line.
- **What a melody from the tracks line is:** a real track. It is N units of the mode's length,
  with a blank cover and title, as T gives a track warped in.
- **Filters:** the mode's filters judge the whole track with its units joined, as a track's JOINED
  stack does, so they judge across the seams. The filters heading says so.
- **Its address** is its tracks-line address. I checked one against `sieve warp --line tracks`:
  they match. Long addresses are shown as their first and last digits around "..", since a
  track's begins with the zeros of its blank cover and title.
- **G** walks to it on TRACKS. If the hallway's audio length, note set or units a track differ,
  the hallway is rebuilt with the track's values.
- **Saved:** a track keeps its unit count in the recent list and in the favourites, and saves as
  `sieve-tracks-<last 12 digits>.mid`.

### A bug found on the way: "Go to its location" went to BOOKS
It set the line to door 2, which was audio before the doors were reordered and is books now. It
now goes by name: audio, or tracks for a track.

### From `hand.zip`, included here
- **The model in hand turns back to its resting angle whenever it is put down,** so it comes back
  upright.
- **One more path now puts the previous item down first:** going to an address that compact mode
  has no shelf for put the new item in hand without putting the last one down.

## Checked
- **Build:** the client builds with no warnings (Linux, GCC 13).
- **CI steps:** the models step and the whole hallway step pass, including the Media Player
  screenshot.
- **Media Player:** switching WORLD to the tracks line, with 5 units, is saved to
  `sieve-music.ini` (`source = tracks`, `units = 5`).
- **Old and new entries:** an old-format recent entry (`track =`) and a new tracks entry
  (`tracks:2`) both load and are listed (`media-player.png`).
- **The address:** the listed track address ends `2de953`, as `sieve warp --line tracks
  --track-units 2` gives for the same notes.
- **Not checked:** a melody actually being drawn from the tracks line and played. Scripted runs
  have no audio device, so the player never draws. Please listen to one in-game.

## Docs
- **README:** the music and Media Player paragraphs.
- **HANDOFF:** a new entry.
- **`client/music.hpp` and `client/media_player.cpp`:** their file comments are rewritten.
