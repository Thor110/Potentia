# Handoff

This file exists so a fresh conversation can pick Sieve up cold. It is not a specification and
not a change log: `docs/SPECIFICATIONS.md` is the specification, `README.md` is the tour, and
`docs/IDEAS.md` is the list of what is not built yet. This is the briefing that sits between
them — what the project is, the rules it is held to, how to build and check it, and where the
work stopped.

Concept and architecture by Edward James Gordon (**thor110**).

## What Sieve is

Every possible text, picture, melody, animation and model of a fixed size exists whether or not
anyone has written it. Sieve gives each one an address, and sifts the space so that the tiny
fraction of it that means anything can be found. A line is a state space of one kind of unit at
one size; an address is the unit's exact position in that line, written as an integer.

The hallway is that space made walkable: a corridor of bookcases where each slot is one unit,
in address order, so walking forward counts upwards.

## Standing rules

These are not preferences. Work that breaks one of them is wrong.

- **No LLMs anywhere in the engine.** Ordering and prediction use frequency statistics from a
  pinned corpus, nothing more. The Python side of the project is an oracle, not a model.
- **State spaces are dynamic and unbounded.** Nothing in the design may assume a ceiling on a
  line's length, a unit's size, an alphabet's size, or the width of an address.
- **Exact integer arithmetic only.** `BigUint` carries every address. Floating point may compute
  what is drawn on screen and nothing that is addressed.
- **Filters and alphabets are versioned, never edited.** A change of behaviour is a new id
  (`guided-ac-v1` → `guided-ac-v2`). An address written down last year must still resolve.
- **A written address is coupled to the setup that produced it.** The setup specification — line,
  alphabet, length, ordering, key, filters — is part of the address's meaning, and is what the
  reader must be given alongside it.
- **The Python oracle stays independent of the C++.** `reference/sieve_ref.py` is written from the
  specification, not from the C++, and generates the conformance vectors the C++ must reproduce
  bit for bit. Never "fix" the oracle to agree with the engine.
- **Addressing is not compression.** In a bijection the address *is* the content, so there is no
  saving to be had by splitting an address into room and item, or by any other re-coordinating.
  Only a genuinely guided ordering is smaller, because a guided ordering *is* a compressor.
  §0 of `docs/IDEAS.md` sets this out; a proposal that forgets it is the commonest mistake.
- **Purely functional.** Nothing is stored. The engine sieves the state space; it does not keep a
  database of what it found.

## The seven lines

    binary | pages  image  audio  video  books  models | binary

`kLines = 7`, with `binary` at index 6. The six middle lines used to loop into each other through
the doors; they no longer do. They start and finish at the binary line, which is why it is listed
at both ends of the map.

The binary line is **one line met from either end** — one ordinary tile of corridor, the same
width and height and bookcase as any other, with one side missing. Where the other wall would
stand the floor ends at a waist-high wall (`kEdgeRail`, `kEdgeRailTop` in `client/world.hpp`) and
green rain falls past it for ever. `binary_shelf_` decides which wall carries the shelves, since
which side the drop is on depends on which end you walked out of. Nothing out there is addressed,
ordered or filtered, and the shelves stand empty: cataloguing what is in the drop is a job for
people.

Its alphabet is `bytes256` — U+0000 to U+00FF in byte order — and its canonicalisation is
`canon-bytes-v1`, the identity. A file of N bytes is one unit of a length-N line, and its
positional address **is** its hex dump. This is the demonstration that the addressing is honest:

```sh
printf '\xde\xad\xbe\xef' > four.bin
sieve warp --alphabet bytes256 --length 4 --file four.bin            # positional address: deadbeef
sieve read --alphabet bytes256 --length 4 --mode positional deadbeef --out back.bin
sieve read --alphabet bytes256 --length 20000 --mode positional --address-file big.hex --out back.bin
```

`--address-file` exists because a 200,000-digit address does not fit in a command line.

## Geometry that is a setting, not a constant

Corridor tile size is a **runtime setting**: `sieve::books_per_tile()` / `set_books_per_tile(n)`
(a power of two, 2 to 4096, default 128), exposed in the hallway's setup menu as GLOBAL →
*number of items per wall*. It changes no address whatsoever — only the (tile, slot) coordinate
an address is displayed at. Proven empirically: at 128 and at 256 the same book is the same
`85.9326950016% along` with the same address, and the tile number exactly halves.

What stands in a slot is scaled **uniformly** by `shelf_scale()`, never squashed along the shelf:
a record is round whatever else changes. A sixteen-column shelf is the scale of one; a
thirty-two-column shelf is half of it; never larger than one.

## Where things are

| Path | What it is |
| :--- | :--- |
| `core/` | The engine. No dependencies, C++20. |
| `core/src/biguint.cpp` | Exact big integers, limbs of 2^64. Everything rests on this. |
| `core/src/corridor.cpp` | Address → (tile, slot). Holds the tile-size setting. |
| `core/src/guided.cpp` | `guided-ac-v1`, the entropy-ordered addressing. |
| `core/src/filter.cpp`, `core/src/filters/` | The filtration stack. |
| `client/hallway.hpp` | The `Hallway` class and the helpers its parts share (namespace `hallway::hall`). |
| `client/hallway.cpp` | The corridor itself: lines, position, filters, movement, input, the frame, Real Graphics. |
| `client/hud.cpp` | Everything in screen space: the compass, the panels, the readout, the item page and COST tab. |
| `client/door_portal.cpp`, `client/binary_edge.cpp`, `client/crate_faces.cpp` | The doorway noise; the rain and the drop; the models line's crate faces and their render workers. |
| `client/app_main.cpp` | Options, the menus, the screenshot and scripting paths, the event loop, `main`. |
| `client/menu.cpp` | The setup menu, `find_limits()`, the budget model. |
| `client/mesh.cpp` | The software rasteriser behind Real Graphics. |
| `tools/sieve_cli.cpp` | The `sieve` command. |
| `reference/sieve_ref.py` | The independent oracle and the vector generator. |
| `tests/` | Core tests and the committed conformance vectors. |
| `docs/IDEAS.md` | Everything not built yet, triaged. Read §0 first. |

## Build, test, package

```sh
cmake --preset linux-release
cmake --build --preset linux-release
ctest --preset linux-release
```

Without presets: `cmake -S . -B build && cmake --build build --config Release && ctest --test-dir build -C Release`.
`-DSIEVE_BUILD_CLIENT=OFF` skips the hallway (and so SDL3). `-DSIEVE_BIGUINT_PORTABLE` builds
`BigUint` without a 128-bit integer type; CI checks that path on Linux.

**Tool time is the scarce resource, so say before spending it.** In a conversation, a full build
(SDL3 is fetched and compiled from source on a fresh folder), extra build folders, mutation runs and
CI steps add up to most of an hour, while the core suite itself runs in under a minute. Before any
change that needs the test suite run, or anything heavier, say what running it will cost and ask,
so that it can be skipped, timed, or run locally on Edward's machine (Windows 10) instead. The
cheapest check is a core-only build (`-DSIEVE_BUILD_CLIENT=OFF`) and one run of `sieve_tests`.
Prefer changes that need less of it.

The hallway can be driven headless for checks — `--take` a screenshot, `--press` a key sequence —
and note that `--take` runs after `--press`, so anything that needs a frame first must render one.

Regenerating vectors (only when the specification changes, never to make a test pass):

```sh
python3 reference/sieve_ref.py biguint-vectors > tests/vectors_biguint_v1.tsv
python3 reference/sieve_ref.py biguint-large-vectors > tests/vectors_biguint_large_v1.tsv
python3 reference/sieve_ref.py bytes-vectors > tests/vectors_bytes_v1.tsv
python3 reference/sieve_ref.py titled-vectors > tests/vectors_titled_v1.tsv
```

CI is `.github/workflows/build.yml`: fourteen named steps across Windows, Linux and macOS. The
full sweep takes about twenty-five minutes, so run targeted steps during development and the
whole thing only for a release. The workflow's `run: |` blocks are fragile in one specific way —
a multi-line `python -c "` block starting at column 0 ends the YAML scalar — so validate with
`yaml.safe_load` after editing it.

Packaging the build for delivery:

```sh
cd /home/claude/Sieve && rm -f *.png *.ini *.bin *.hex *.book *.model *.obj \
  obj.txt objcrlf.txt positional.txt scrambled.txt unbound.txt reference/book.out 2>/dev/null; rm -rf lt
cd /home/claude && rm -rf Sieve/reference/__pycache__ Sieve/tools/__pycache__ Sieve.zip && \
zip -qr Sieve.zip Sieve -x 'Sieve/build/*' 'Sieve/build-*/*' 'Sieve/out/*' 'Sieve/.git/*' \
  'Sieve/corpus/*' 'Sieve/data/fonts/unifont.hex' '*/__pycache__/*'
```

## House style

- Comments are prose in full sentences, British English, explaining *why*. No bullet lists in
  code comments, no restating what the line plainly does.
- Each file opens with a paragraph saying what it is for and how it fits.
- Warning-free at `/W4` and `-Wall -Wextra -Wpedantic`.
- The oracle mirrors the specification's vocabulary, so the two can be read side by side.
- The code is meant, in the end, to document itself, and the program to explain itself to the
  person using it. Documents outside the code should shrink over time rather than grow: when
  something can be said in the file it describes, or in the program's own help, it goes there.

## Where the work stopped

Built and green as of this handoff: the binary line and its edge, the compass HUD measured in
units, the limits finder, `bytes256` end to end, the COST tab on the item page, the 64-bit
`BigUint` with Karatsuba multiplication, reciprocal division and conversion by halves,
items-per-wall as a global setting, uniform shelf scaling, and the filters-overlay crash fix. The
core suite is 16,270 checks, 0 failures, run once with each SHA-256 implementation, and passes
under `SIEVE_BIGUINT_PORTABLE` too.

`BigUint` at 152,000 bits (a pages-line address) now measures multiply 1.01 ms, divmod 4.80 ms,
`to_digits(27)` 2.90 ms and `to_decimal` 3.31 ms; before-and-after figures are in
`docs/REVIEW.md`. Its thresholds are measured, not critical, and listed at the top of
`core/src/biguint.cpp`.

A code review was run over the whole tree at the same time as this handoff was written:
`docs/REVIEW.md` lists what it fixed and, more usefully, what it found and left.

### Titled lines

Every line but books now gives each unit a title, and audio and video a cover as well, as books
have (SPECIFICATIONS §11, "Titled lines"; `core/include/sieve/titledspace.hpp`, id `titled-v1`).
A titled unit is its cover, its title and its content packed into one number,
`(cover * |title| + title) * |content| + content`, so neighbours in positional order differ in
their content and the line is `|cover| * |title|` times the size it was. Titles use the pages
line's alphabet; covers are pictures of the image line. The title length is a GLOBAL setting in
the setup menu (`--title-length`, default 32). **A title length of 0 means no title**: the line
is then the bare line again, and on a line without a cover its positional addresses are the
bare line's own. Books are unchanged: they already carry a title page and a cover.

This changes the hallway's addresses. There is no release, so nothing published is lost. The
`sieve` tool's lines are still bare, so the tool and the hallway agree only at title length 0,
and the CI checks that compare them pass `--title-length 0` to the hallway. Guided and compact
orderings still address the content alone. The oracle writes `tests/vectors_titled_v1.tsv` (84
rows, five shapes with titles and two without), and the core suite checks every row both ways.

Two fixes came with it. The setup menu grew a row and its actions ran into the footer at 720
pixels high, so the menu is now drawn at 1240x780 at least (760 then; 780 since the letters row) and scaled down to fit. And RESET
EVERY SHAPE now puts the picture cache back as well, since FIND MY LIMITS sets it.

Agreed and still to do: titles in the `sieve` tool (`read`, `warp`, `info`), and titles in guided
and compact orderings. When titled lines get filters, they scan **bottom-up**: content first, then
title, then cover, the reverse of how the unit reads (`docs/IDEAS.md` §5.13).

The item faces: every item on a titled line shows a title band at its top, even when the title is
blank. A blank title reads **[Null Title]** (white brackets, red words, on a dark plate) on the item
and in every window that shows a title, for every line including books (`title.null` in the
language file). The first titles of a line in positional order are blank, and at 50% they are one
letter repeated. The title is set as large as fits in a third of the item, broken
into rows for that. Books, audio and video share one front: title at the top, cover in a frame at
the foot, on the line's colour.

### Displays sized for their letters, and close-ups

An item's display (the picture on its front) used to be one width on every line, the display
size setting. It is now that setting **or wider**, on a line whose items carry text: as wide as
its letters need to come out at the **letters on items** size (GLOBAL, `--item-letters`, default
8 px, the font's own size), rounded up to a power of two, at most 1024 (`client/display.hpp`,
shared by the hallway and the setup menu's budget, so the two agree). A 410-letter page is 256 px
wide at the defaults and reads as a page; images, audio and video stay at the setting because
their titles fit. Letters that still come out smaller are dashes. A title's letters are capped at
a sixteenth of the width so a large display is not all title.

**Close-ups** are a level of detail on top (GLOBAL **close-up display size**, `--close-up`: off,
256, 512 or 1024, the default). An item whose display is drawn more than 1.25 times wider on screen
than it has pixels is drawn again at the power of two that covers its width on screen, from twice
the line's width up to the setting, by the same render workers, ahead of the ordinary queue. The
ordinary display stands in until it arrives. At most 24 are kept; one unused for two seconds is
dropped. The budget allows 24 at the close-up size (about 140 MB at 1024).

The setup menu's minimum height is now worked out from its rows, so it no longer needs raising by
hand when a row is added. The setup menu's line rows are now counted from named constants
(`kFirstLineRow` and the section rows in `menu.hpp`), so a GLOBAL row added later moves them with
one change; the CI row numbers (the model cache row, and the filter rows 7/11/14/15/19) still
have to be moved by hand.

Pictures on items no longer warp up close: see `docs/REVIEW.md`, `draw_face_image`.

### The address navigator

**X** in the hallway opens the whole address of the item you are looking at, full screen, a hex
digit at a time (`client/navigator.cpp`). Left/Right choose a digit (Shift: a row), Up/Down or the
wheel turn it (Shift: eight), the pointer picks a digit and its arrows turn it, 0-9 and a-f type
it, ENTER goes there, Esc or X leaves. Turning digit p moves 16^p units, carrying into the digits
to its left and wrapping round the line. What it edits is the item's position in the loop, the
number `place()` takes, so it works in every ordering; the binary line has none.

### Item pictures

Every line's items now show a picture on their front (`client/item_faces.cpp`): pages their text
(real characters when they are large enough to read, short bars when not), images their picture,
videos their title over their first frame, audio their title over their cover, books their title
over their cover, models their mesh with its title. They are drawn by the render workers for your room and one either side, and rooms further
off wear the picture of the same slot in your room. Where the picture goes on each item is
`data/meshes/faces.ini`, measured from the item models (`data/meshes/book-<medium>.obj`, cleaned
to flat normals). The setup menu's display size and display cache (named "picture" until 27 September)
apply to every line and sit in GLOBAL; the display size goes up to 1024 px and the cache to 4 GB,
since three rooms of 1024 px pictures need about 2.2 GB. `--settle N` draws N frames standing still before a
screenshot, so the pictures have arrived when it is taken.

### The hallway, split

`client/hallway.cpp` was split by subsystem into the files in the table above, with the class in
`client/hallway.hpp`. Every member function with more than one line moved out of the class to the
file of its subsystem; one-line accessors stayed in the class. The move was made by a script, so
nothing was rewritten by hand: the bodies are the same code, and the hallway CI step passes as it
did. `draw_face_image` is shared by the binary edge and the crate faces and stays in
`hallway.cpp` with the rest of the drawing helpers. `SDL_main.h` is included only by
`app_main.cpp`, because on Windows it defines `WinMain` and must be in exactly one file.

### Open, in rough order of value

1. **Alphabet-relative entropy baselines** — §2.1 of `docs/IDEAS.md`. A latent correctness bug,
   not a cosmetic one. It changes what filters decide, so it is new filter ids, and the ids want
   agreeing before the work starts.
2. **The node graph, as a SORT tab** beside ITEM and COST on the item page, and as its own
   screen: the item highlighted in the graph. Edges come from verified anchors, the first being
   the release manifest, which has to carry the keys of the template models anyway. SORT shows
   the shortest route an item has: the nearest anchor and the offset from it, against the full
   address, which is only shorter when the item is near an anchor. The graph is agreed to be a
   separate system from the corridor, not a mode of it.
3. The colour-blindness pass over the line palettes.
4. Portal signage: the doors say nothing about what is through them.
5. The manifest and directory walker (§4 of `docs/IDEAS.md`).
6. The grammar reducer (§1).
7. The word/dword scaling ladder, so that values are not quietly pinned to a byte.

The `bytes256` vectors found the engine narrower than §12.1a: `canon-bytes-v1` accepts only the
exact byte alphabet, where the specification says any alphabet holding every byte. The
specification stays as written and the engine widens when anchors need it (`docs/REVIEW.md` item 6).

**FIND MY LIMITS and the budget.** Entering the hallway at the shapes FIND MY LIMITS used to pick
froze Edward's Windows build for over a minute (on Linux in Release the same shape opened in a
second, so the build type is the likely difference). The finder now works to one budget of three
parts, drawn as bars in the setup menu: the largest line's whole cache of units (positions, and the
address as a number and as hex, for every unit kept) against a quarter of memory; the model image
cache plus an allowance for the world against **Graphics Memory**, a setting in Settings >
Graphics because SDL cannot ask the card; and the time to open one unit, measured on the machine
at start-up, against 50 ms, so a slow build gets smaller limits. ENTER THE HALLWAY stays shut over
budget and the foot of the list says which limit and which line. The hallway is built on a worker
thread while the window shows a red CALCULATING with its dots counting, if it takes more than a
fifth of a second. Crate faces are drawn by a few worker threads (one fewer than the cores, at
most five), filling only the pixels each triangle covers, and every vertex is drawn as a dot so a
model whose vertices have all come out in one place still shows. Still to do, later: read the
graphics memory from the card (DXGI on Windows) rather than asking for it.

**Pictures on every line's items.** The crate faces became item faces (`client/item_faces.cpp`):
each line's items carry a picture drawn by the workers from a painter the frame hands them --
pages their text (letters where the cells are big enough to read, short bars where not), image
the picture, video its first frame, books a title band over the cover, models the mesh. Audio has
none yet (see below). Where the picture sits on each item is `data/meshes/faces.ini`, measured
from the item models, which are now per medium in `data/meshes/book-<medium>.obj` with flat
normals. `hallway --settle N` draws N frames standing still before a screenshot so the pictures
have arrived.

**Where settings live.** Settings > Graphics is only about drawing. Anything about the generated
world is in the setup menu: Angle Precision is in GLOBAL (row 4) and Model Image Cache in MODELS
(row 22), and both are saved in the `[world]` section of `sieve-hallway.ini`.

Done this session in the hallway: crate faces are rendered for your room and one either side only
(`kCrateRooms`), within a time allowance per frame, and rooms further off wear the face of the
same slot in your room; the Model Image Cache goes to 2048 MB, so three rooms of 512-pixel faces
fit; and the binary edge's rain is no longer a curtain but streaks placed in the world, each at
its own distance beyond the edge out to 50 m (more near than far), written in characters up
close and drawn as lines far off, placed and timed by hashes so nothing is stored, and clipped
to the band of heights that can be seen through the opening at each streak's distance.

`docs/IDEAS.md` §11 lists the questions that never got an answer; those are the ones worth
raising before building anything large.
