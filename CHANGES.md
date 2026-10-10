# Worlds: models placed in a world

Against `23dee32` (titles panel). Item 7 of your list (IDEAS §16.7), as you chose it: each slot a
model and its placement, placement as a grid cell and one of 24 turns, 4 models a world by
default, with a cover and a title.

## What a world is (`worldspace-v1`, SPECIFICATIONS §12.0)
- **The parts:**
  - a cover (a picture of the image line);
  - a title (as every titled line has);
  - N models of the models line (**models per world**, 4).
- **Each model's place:** a cell of a grid of G cells along each axis (**grid**, 8), and one of the
  24 rotations that take a cube onto itself.
- **One number:** cover first, then each slot's model, cell and turn; scrambled as the other lines
  are.
- **Size:** at the defaults a world's address is 1,122 bits (about 10^337 worlds).
- **Exact coordinates:** a cell is one model wide, so every coordinate stays exact and a world's
  `.obj` text is the same on every machine.
- **The turns (`turns-24-v1`):** a fixed, documented order; turn 0 is unturned.
- **Not in this version:** textures, which wait for UVs (IDEAS §14).

## Filters (`[worlds]`)
- **Parts:** the cover, the title, and each slot's model judged by the models line's own
  filters. A model's place has none.
- **Counting:** exact, so worlds compact like the other lines. The hallway and the tool agree on
  the survivors (checked with distinct-indices-v1 ticked).

## In the hallway
- **The door:** WORLDS stands after MODELS, in models' clay with black edges.
- **On the shelf:** a crate with the whole world drawn on its front under its title.
- **In hand:** it turns as a model does (A, D, the mouse, R), with its `.obj` beside it.
- **F, J, viewer:** F saves the `.obj`, J finds it on the binary line, and the viewer shows it.
- **T (warp):** says there is nothing to warp in from yet. Go to an address (G), or find one with
  `sieve world --compose`.
- **The setup menu:**
  - **Rows:** WORLDS has two (models per world, grid).
  - **Map:** a column of its own.
  - **FIND MY LIMITS:** grows the models per world.
  - **Filters window:** COVER, TITLE and MODELS parts, with each filter's share.
- **Real Graphics:** `book-worlds.obj`, a copy of the models crate, and a `[worlds]` section in
  `faces.ini`.

## In the tool: `sieve world`
- **`sieve world`:** the shape and its size.
- **`--compose "MODEL:x.y.z.turn|..." [--title TEXT] [--cover PICTURE]`:** a world's
  positional and scrambled addresses, and its survivor number and compact addresses under
  `[worlds]`.
- **`--read ADDR [--mode scrambled] [--compact] [--out world.obj]`:** the world's `.obj`.
- **Option names:** the hallway's (`--world-models`, `--world-grid`, and the models', cover's
  and title's).

## The degrees sample
- **Now ten dimensions:** 3,600 items, 1,055,836 bytes, a `.sieve` of 436,788 bytes. Worlds repeat
  every 72 degrees, as the other compositions do.

## A correction to the last batch
- **Duplicate titles in the degrees sample:** I wrote that the default shapes have no two items
  with one title. That is wrong for the lines with a cover: books, audio, tracks, video, movies
  and now worlds.
- **Why:** there the cover leads the address, so at whole degrees the titles repeat as well, 45
  pairs on each of those lines. The " (2)" suffix already named those files apart.
- **Fixed:** the notes in IDEAS and HANDOFF now say so.

## Checked here (Linux)
- **Unit tests:** 48 world vectors against the oracle, the turns table, `.obj` hashes, and the
  world sieve against brute force.
- **The oracle:** `sieve_ref.py world-vectors` and `world-obj`, written apart from the C++. Its
  vectors are regenerated and identical, and its `.obj` matches the tool's byte for byte.
- **CI steps, run here:**
  - the model step: compose, read back, the oracle's `.obj`, and the scrambled address giving the
    same world;
  - the hallway step: the world at a composed address saved with F is the tool's `.obj`, byte for
    byte;
  - the menu step;
  - CI's checks of the corridor's order, updated for the new door: models' left door leads to
    worlds and worlds' to binary, and the walk end to end takes eight doors.
- **Screenshots:** the WORLDS shelves, a world in hand, the setup menu with WORLDS, and its
  filters window.
- **Not checked:** I have not built on Windows.

## Not yet
- **Textures and UVs** (IDEAS §14).
- **Warping a world in.**
- **Filters on a model's place** (no two models in one cell, for example).
