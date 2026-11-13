# The music visualiser's rings (IDEAS 19.175)

Builds on the replay zip.

## Using the rings

- **R** in the Media Player opens them. **R** or **Esc** goes back.
- There are eleven rings, one for each octave, lowest in the middle.
- Each ring has twelve sectors, one per note, coloured round a colour wheel from C (red) through the hues to B.
- A note lights as loud as it is sounding now, then fades by half every 0.35 s. The whole circle turns slowly.
- Every voice the music player is sounding is shown exactly, including the line's character.

## Zooming

- **+ / -** or the wheel zooms in.
- One step in shows the overtones: rings of the first eight harmonics, each lit in the colour of the note it falls on.
- The next step shows each note's own wave, drawn round a circle.

## For pictures

- `--rings NOTES` (MIDI note numbers, for example 48,60,64,67,72) and `--rings-zoom N`.
- The readout names every lit note.

## Noted

- **IDEAS §19.175:** as the music gets richer, the rings grow finer until they merge into one continuous circle: pitch as an angle, octave as a distance from the centre.
- **IDEAS §19.188 and the ROADMAP's final tasks:** the main menu reordered from Start Sieve to Exit Sieve, Settings renamed Potentia Settings, Character Creator and Replays placed in the list, and the CI steps choosing rows by name first.

## Checked

- CI 10 passed:
  - R opens the rings.
  - A C major chord (C3, C4, E4, G4, C5) lights exactly those five notes.
  - The waves zoom works.

## Files

- New: `client/rings.cpp` and `client/rings.hpp`. Re-run CMake.
- No files to delete.
