# List O: O4 made, O7 measured (list O done)

## What changed

- **O4, a recording kept as its text** (`client/replay.hpp`, `client/app_main.cpp`).
  - A recording being made (F12, or `--record`) now keeps each frame as the line it is saved as, appended
    as the frame is taken (`Recording`). Before, it kept every input event whole, at 128 bytes each.
  - 30 minutes at 60 frames a second, with a 1,000 Hz mouse (17 moves a frame), held 451.9 MB before.
    It holds 49.8 MB now: 30.2 MB of text, and room the string keeps for growing.
  - The text is the same either way.
  - The recording is still packed each time the hallway is left, so a crash or a menu visit leaves the
    file on disk.
  - Playing back is unchanged: a recording is read as before.
- **O7, the trial mixers on a large file** (a measurement, nothing changed). enwik8 (100,000,000 bytes)
  was packed twice:
  - by the build as it is: peak 3,020 MiB, 317 s;
  - by one whose trial history touches every page, as before review 1: peak 3,018 MiB, 336 s.

  The `.sieve` was the same (24,768,406 bytes). Review 1's untouched pages save nothing at the peak here:
  the peak comes later, from the whole file's mixing. The change stays, as it costs nothing.

## How O4 was shown the same

- **The same text:** a 30-minute recording was built both ways from the same frames, and the text matched.
- **A live recording:** one was made in the running hallway, stopped and written. It reads back, replays,
  and is written again the same, byte for byte.
- **CI step 10:** its recordings are replayed and written again the same, and the step passes.

## Also, noted from today's messages

- **X1 and X2 split** (REVIEW-ROUND):
  - X1: the fractal line as every ruleset of the grammar, generated, with the charted fractals as its first
    filter (exclude, include, only) and the index book still naming them.
  - X2: then as many known fractals charted as can be tracked down, spider webs among them. An orb web's
    spiral needs a step that shrinks, so the grammar will need widening for it.
- **Fractals as data in other dimensions** (IDEAS 19.235, in your words): each dimension's ways of reading
  a ruleset as its kind of item, so fractals can be filtered out of it.
- **HANDOFF:** two standing questions for anything new.
  - It becomes its own line, added dynamically.
  - It says what its fractal ways are.
- **OPTIMISATIONS.md:** empty now. Every entry is made or measured.

## Files to delete by hand

None.
