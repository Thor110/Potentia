# Sieve: the trailer

One commit on top of 8fc8e3d.

## The film

`potentia-sieve-trailer.mp4`: 10 minutes 35 seconds, 1920 x 1080, 30 frames a second, in eight chapters,
each opening on a title card:

0. Cold open: a walk down the pages line, wireframe off, and the title.
1. Why: exact records, the Museum and the Gallery, the shortest description (Kolmogorov complexity,
   and that it cannot be computed), honesty about addresses, and what it is for: historical
   preservation, human unification, long-term archiving.
2. The address: warp (T), the item tabs (C), the orderings (M), go to (G), the navigator (X), Home,
   running and jumping (PgUp, ]).
3. Walking the dimensions: one walk through every door, pages to binary, each line's size on
   screen; the same corridors with wireframe off; edge glow; the fractal shelves.
4. Holding things: Sieve's icon warped onto the image line and opened in the viewer, a book, a
   melody, a world, J to the binary line, the fractal index, CHAT, CHAIN, a model turned.
5. Filters: the setup menu, a line's filters, the details, OPTIMISE FILTERS and what it keeps on
   every line, then the pages and image lines walked in compact mode; the Filter Designer, versions.
6. Files: the File Locator; the help pages packed, installed and a damaged copy refused, measured
   by the build itself (65,185 bytes; zip 30,029; 7z 21,204; Sieve 16,478); the benchmark table;
   the two decoders, and where it loses.
7. Everything else: pause menu, node graph, Manual Navigation, Encryption Engine, Media Player,
   rings, Character Creator, personal dimension (day, dusk, ground shaper), Training Harness,
   Replays, System Viewer, Documentation, Settings.
8. Dynamic, modular, procedural; the close; the end card, "by Edward James Gordon (Thor110)".

The music is melodies of the audio line, drawn at random from the survivors of the music player's
own default stacks (WORLD under the walks, MENUS under the cards and tools) and played with its
voices; `music.txt` lists each one by its rank.

## In the repository

- **The hallway:** `--replay-video OUT.mp4` (with `--replay` and `--screenshot`), `--replay-fps N`.
  Every frame of a recording is drawn and piped to ffmpeg; MANUAL's Replays paragraph says so.
- **tools/trailer:** the whole film as scripts (README.md says how to run it): `trailer.py` (the
  running order, every caption and card), `walker.py` (recordings written by hand), `shoot.py`,
  `stills.py`, `cards.py`, `music.py`, `cube.obj`. Run on your machine it makes the same film, drawn
  by your graphics card.

## Checks

- CI steps 01 and 10 pass; the help, help-library and settings checks pass.

## Files to delete by hand

None.
