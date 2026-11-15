# List V: the viewers (V1 and V2)

Unzip at the root of the repository (this zip's folders are `Sieve/` and `REVIEW-ROUND.md`, with no
extra `Potentia/` folder above them).

**Files to delete by hand:** none from this zip. If not done already, delete the folder `Potentia/` at
the root of the repository: it was left there by the first R7–R9 zip (every file one folder too deep),
and the corrected R7–R9 zip put the same files in their right places.

## V1. Sound and model files open in the viewers made for them (IDEAS 19.200)

On the binary line, a file picked up and opened (Z) now goes to:

- **Sound** (WAV and FLAC by their kind; AIFF, Wave64, .au and CAF by their first bytes): its waveform,
  a band a channel, drawn as the audio line draws a unit. **Space** plays it, through the music player
  where there is one (the music fades under it), and again stops it; closing the viewer stops it.
- **MIDI**: its notes, each voice on its own lines, read as the audio line reads a MIDI file, at the
  tempo the file sets first. Space plays them. A chord's other notes and tracks past the fifteenth are
  left out, and the foot of the screen says how many.
- **Models** (.obj, glTF with its buffers inside, .glb): its **3D** view, turned with the arrows or WASD,
  its text the next tab where it is text.

A sound of more than 2^26 samples (every channel counted) is refused before it is decoded, a FLAC by
what its STREAMINFO says, so a few bytes claiming hours cannot fill the memory; a glTF whose buffers
are files beside it says it cannot be read from its bytes. Neither is flagged in `viewers-needed.txt`.
MP3 and Ogg have no decoder in the hallway yet, so they are still flagged.

Code: `client/viewer.cpp`, `client/hallway.hpp`; `tools/cli/flac_wav.*` gains `sound_samples`, and its
FLAC decoder a limit on the samples it may hold.

## V2. The Documentation (IDEAS 19.201)

A new main-menu row, **Documentation**, before Potentia Settings: Sieve's help library. CONTENTS,
INDEX and SEARCH on the left, the page on the right; links followed in place (Tab to a link, Enter),
Backspace back through the pages read, typing searches. Its keys are on Settings > Controls > Keys
under "The Documentation", and the keys the pages name read as you have set them.

The library is data: `Sieve/data/help/contents.txt` and one file a page, 25 pages in six categories,
written for the person using Sieve from the manual and the menus' own words.
`python tools/check_library.py` (run in CI) checks every page, link and key named against the code.

Code: `client/help_library.*` (new), `client/main_menu.*`, `client/app_main.cpp` (`--docs PAGE`),
`client/personal_world.cpp` and `client/keys.hpp` (its keys), `CMakeLists.txt` and
`tools/make_release.py` (the help folder beside the program).

## Checked

- CI: all 12 steps pass (01 to 09 and 11 to 12 on the changes; step 10 again after its fix below).
  The first run of step 10 failed at the new check that Space plays a WAV: the CI machine has no sound
  card, so it said "audio unavailable". The two play checks now play into SDL's dummy audio device.
- New CI checks in step 10: each kind of file opened and played, the FLAC refusal, the MP3 still
  flagged, the Documentation's link, back, index, search and missing page, its footer's width, and
  the main menu's new row.
- `tools/check_help.py`: 0 missing. Settings check: 141 settings match.

## Ideas written down (docs/IDEAS.md)

- A priority mark for new entries (*low*, *medium*, *high*), described at the top of IDEAS.
- 19.231 The hash viewer (*priority: low*; in REVIEW-ROUND's Later list, not list V), with the
  measurements: hashes carry no trace of their file's place.
- 19.232 Pages of few values left to the binary line (2^k symbols), the partial filter for runs on two
  symbols, the bits' other order, and four candidate filters for sound itself.
- 19.233 State spaces unfolding as formats are added: formats as branches, endless parameters as axes,
  meanings as a layer of tags, a hyperbolic layout, colour by branch.
- 19.234 Towards the shortest program: Kolmogorov complexity, time-bounded search, recipes, routes to
  a file in bits, and everyone's favourites as one index (1.66 trillion, about 41 bits a route).

## To test (REVIEW-ROUND.md, items 48 to 51)

48 sound files, 49 MIDI files, 50 model files, 51 the Documentation.
