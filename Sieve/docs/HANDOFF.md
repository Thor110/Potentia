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
green rain falls past it for ever. The room is always the same way round: shelves and door on the
line's left, the drop on its right. It used to be mirrored depending on which end you came in
from, which made it two rooms; since 27 September you are **turned round** instead when you come
in from models (`cross()`), so the two halves of the ring run in opposite directions, as the two
ends of one line do. Its one door leads back to where you came from (`binary_from_`, pages when
you start there), and its sign says so. A scripted `--walk` is in the walker's frame, so a door
that turns you round turns the rest of the walk with you.

**Its units (`binary-v1`, since 27 September).** Every file of 0 to N bytes, N the BINARY
length setting (`--binary-length`, default 32), numbered shortest first and then by the bytes read
big-endian, so a file's positional address is its hex dump plus `0101...01`, one `01` for each of
its bytes (`core/include/sieve/binaryspace.hpp`; SPECIFICATIONS §12.1; oracle `binary-vectors`,
128 rows in `tests/vectors_binary_v1.tsv`). Scrambled is shuffle-sha256-v1 over the count, and each
file is titled with a cover, as audio and video are. The files stand on the one wall, so the loop
counts the right wall's slots as empty (`loop_pos()`, `unit_of_pos()` in `hallway.cpp`; the
navigator and go-to take the file's place on the line). T on the binary line takes a path, and
warps to that file's bytes, or else the typed text's own bytes: that is how the release's CLI
executable goes on the shelf as the node graph's first anchor, once N is at least its size. The
item in hand is a hex dump. The item model is `data/meshes/book-binary.obj`, the models line's
crate copied to start from, with a `[binary]` section in `faces.ini`.

Cost: a file's conversions are hex and grow in proportion to N, about 5 ms a file at 272 KB in
positional order; scrambled order is several times that, since the keyed shuffle runs over a
number of 8N bits. The setup menu measures the binary line's time on its own (scrambled, the
slower) rather than with the other lines' base-conversion growth.

**FIND MY LIMITS focus** (GLOBAL, `--limits-focus`: all, binary, pages, image, audio, video,
books, models): grows the one line and leaves the rest as they are. Each line is judged on its
own against the budget, so a line grown alone reaches the same largest shape it would with every
other line at its smallest; pages and image grown together is what "books" and "all" do, since a
book is made of both.

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
| `client/door_portal.cpp`, `client/binary_edge.cpp`, `client/item_faces.cpp` | The doorway noise; the rain and the drop; every item's face (pages, pictures, covers, crates, files) and their render workers. |
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

### The binary line's room is a template

`data/meshes/templates/edge.obj` (from `tools/build_mesh_templates.py`, which now builds each wall
with one function shared by `hallway.obj` and `edge.obj`; the other four templates came out
byte-for-byte the same). The game loads `edge-binary.obj`, then the template, and mirrors it in X
when the shelves are on the right; only when neither exists does it cut `hallway.obj` in half and
add a box for the short wall, as it did before.

The portal titles are clipped by hand where a bookcase stands between you and them (nothing
there is depth-tested): the bookcases are boxes at known places, so the part of a sign behind
one is a single cut along the wall, where the line of sight grazes the bookcase's end.

### The file locator and the manifest creator

`sieve locate FILE` names a file's place on the binary line (positional binary-v1, which does not
depend on the line's length) with its size and SHA-256; `--out` writes the whole address.
`sieve locate FOLDER` walks it and writes a `sieve-manifest-v1` (SPECIFICATIONS §12.2), with the
manifest's own SHA-256 as the tree's identity; `--addresses DIR` writes every file's address as
`DIR/<path>.hex`. `--compare` is the comparison page: original, zip's deflate, 7z's LZMA2 and
the address, each as a share of the original (`tools/cli/compare.*`). The compressors are zlib
and liblzma: the system's if there are any, else fetched and built static (`SIEVE_FETCH_COMPRESSION`
forces that), linked into the tools only. The fixture `tests/manifest_fixture/` (a space in a
folder name, an empty file, a non-ASCII name) is checked in CI against the oracle's `manifest`.

Measured here (and note: xz's liblzma, built inside the project, does not pass on its header
folder, so CMake adds it to `sieve_compare` by hand; a system `lzma.h` hid that on Linux): the `sieve` executable (1.86 MB) is 44% under deflate, 34% under LZMA2, and 100%
as an address; `data/meshes` 31%, 15% and 100%, and its manifest 2.2 KB.

**The installer** (Edward's design): a v3 manifest carries the folder structure and then every
file's raw bytes, one after another in the listing's order; the installer is that manifest's own
address, one number (`sieve locate FOLDER --installer OUT.sieve`), and `sieve install OUT.sieve
--to FOLDER` puts the tree back, checking every file before writing any. It was first built on v2
(every file's address in hex in the listing), which made an installer twice the tree's size:
Edward's first real test, a folder holding the 5,056,573-byte `Sieve.zip`, gave a 10,113,286-byte
installer. On v3 the same folder gives 5,056,712 bytes, the zip and its 139-byte listing, the
least possible without compressing first. v2 stays as `--with-addresses` (a listing to read), and
a v2 installer still installs. With `--compare` the installer's manifest is compressed too. The
fixture round-trips in CI, v2 and v3 both, each manifest the oracle's to the byte.

**Installer files are raw bytes** (`NAME.sieve`, `--hex` for hex): the manifest's address as a
number in base 256, exactly the manifest's size, readable as the manifest shifted up by one per
byte. The item page's COST tab and `--compare` both show "as raw bytes" beside hex.

**`sieve-install`** (`client/installer_main.cpp`) is the installer program and the trimmed build:
the core, the locator's install code and SDL3, nothing else (SDL's own debug font, so no data
files); a windowed program on Windows. It opens the `.sieve` given on its command line, dropped on
it, or lying beside it alone; shows what it installs, where (your Documents/<name> by default, a
text field and the system's folder dialog), and an option to replace existing files; then checks
every file, writes them with a progress bar, and on Cancel removes what it wrote, folders
included (`install_tree` in `tools/cli/locate.cpp`, shared with `sieve install`). `--to`,
`--yes` and `--screenshot FILE.bmp` run it unattended; CI does, on the Linux runner.

**Installer programs** (one file to hand to someone): `sieve locate FOLDER --program OUT`, or the
locator's "Make an installer..." when the name is not `.sieve`, copies the `sieve-install` beside
the tool and attaches the installer to its end: `<program> <installer, raw> <length, 8 bytes LE>
"sieve-attached-1"`. Systems run a program from its start and ignore what follows it, so it runs
as sieve-install, which first looks at its own end (`own_executable`, `attached_address`), then
at its command line, then beside itself. A program that already has one attached has it
replaced. `sieve install` reads programs too.

**sieve-install's own SDL** (`SIEVE_SMALL_INSTALLER`, on by default): every installer program
carries sieve-install, so it is built on a second SDL of its own (ExternalProject `sdl3_small`,
from the same source, or fetched when SDL came from vcpkg or the system): static, MinSizeRel
(Debug in a Debug build, so the runtimes match), without audio, joystick, haptic, HIDAPI,
sensor, camera, power, GPU, Vulkan or OpenGL, keeping a window, the 2D renderer and the dialogs;
on GCC/Clang with sections and `--gc-sections -s`. Linux: 3.69 MB to 1.51 MB. Edward's Windows
sieve-install was 7.29 MB before it. It adds about a minute to the first build. Edward's Windows Release build after it: 4.94 MB (from
7.29 MB). Proper procedure (compress first, then make the installer) and the plan to move to
platform installer front ends are in `docs/SIEVE-INSTALL-USAGE.md`. Off, it links
the hallway's SDL (static if there is a static target; with only a shared one the program also
needs `SDL3.dll`, and `--program` says so). Edward's TEST folder: a 3.69 MB sieve-install (Linux) + 5,056,712 + 24 =
8,750,408 bytes. Not yet: an icon, code signing (Windows SmartScreen warns about unsigned programs
from the internet), or checking the attached installer against a checksum before reading it
(install_tree checks every file anyway).

**The File Locator in the hallway** (pause menu, `client/file_locator.cpp`): the tool's own locator
and comparison in a window. Choose a file or folder with the system's pickers (F, D) or drop one
on the window. A file shows its size, SHA-256, address and comparison, and "Go to it" puts you on
the binary line with the file in hand (the BINARY length must hold it; it says so if not), or its
address can be saved. A folder shows its manifest's identity and the comparison (its installer's
manifest compressed too), and saves as a manifest or an installer (`.sieve`). The work runs on a
worker thread; `--locate PATH` opens it for scripted runs and CI.

**A file's front.** A binary file has a title and no cover. Its front shows, large, what kind of
file it is, from its own first bytes (`Hallway::file_type`: ZIP, PNG, EXE, PDF, TXT, EMPTY, ? ...),
and its size. Those bytes, up to 16, come from the top of the number with one borrow
(`file_head`, hallway.cpp), not from working the file out. A file reached from a path (the
locator's Go to it, T with a path) has its file name as its title (`title_for_name`). A picture of
one's own choosing is to be a separate layer of metadata (anchors), never part of the address.

**Past the budget.** A binary file on the shelf keeps only its place, title and first bytes; its bytes
and its hex are worked out for the one item looked at or held (`file_of`, `hex_of`, remembering
the last). The setup menu's first Enter over the budget says what going in anyway means, and a
second goes in **thin**: only the room you stand in keeps items and pictures (`set_thin`,
`face_rooms()`); past `kTooLargeBits` the way stays shut. The File Locator's Go to it does the same
for a file longer than the BINARY length: a second press makes the line exactly that long
(`set_binary_length`), positional, and thin. Edward's 5 MB `Sieve.zip`: 2.8 s from the locator to
the file in hand, 340 MB at the most, about 16 frames a second walking (software renderer).

Still open: compressing files before they are addressed (the order of operations, for later);
releasing the locator and manifest creator as a stand-alone tool (IDEAS 4.9). Packaging the release must
carry `third_party_licenses/` (zlib, XZ Utils' 0BSD, SDL, stb, SCOWL, font8x8): there are no
install rules yet.

### The pause menu

Esc in the hallway, with nothing in your hands, pauses (`client/pause_menu.cpp`): Resume Sieve;
Node Graph Viewer and File Locator, listed but greyed, since neither is built yet; Navigation
System, the address navigator (also X); Settings, the main menu's own Settings screens
(`MainMenu::run_settings()`), after which the same hallway goes on, paused, with the new
graphics, controls and language applied; and Exit Sieve, which asks "Return to the main menu?",
Y for the main menu and N to leave Sieve. The hallway does not draw the other menus: it sets
`request()` and ends its loop, and `app_main.cpp` opens them.

The hover panel lays out anything with a cover as a book is: cover on the left, title beside it,
and what the thing is (its notes, its bytes) under the title, all inside the panel. A video's
first frame is no longer drawn beside its cover.

### F1 in game, and FIND MY LIMITS on pages or image

F1 opens the setup menu over a hallway that is kept: Esc there goes straight back to it as it
was, and ENTER THE HALLWAY builds a new one from the new settings (`Menu::set_in_game`, and the
F1 branch in `app_main.cpp`). Opened from the main menu, Esc still goes to the main menu.

FIND MY LIMITS focused on pages or on image cannot take that line to its own limit: the books
line is made of both, and grown alone pages took books to ten times the time allowed. So those
two focuses set books to one page and grow the line while it and a one-page book both still open
in time. A line already over budget from an earlier run is left as it is by a focused run (the
others are untouched); "all" or RESET puts it right.

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
4. ~~Portal signage~~ — built: every doorway has a sign over it naming the line it leads to,
   in that line's colours (`draw_door_sign`, `client/door_portal.cpp`).
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

**The node graph** (`client/node_graph.cpp`, `tools/cli/map.*`, SPECIFICATIONS §12.3): Edward's
design, 27 September 2026. Maps (`sieve-map-v1`) link verified anchors, real files named by their
SHA-256; a map from a folder is its manifest with every folder linked to its contents
(`contains`); relations are free lower-case words, so later ones need no new format. `sieve map`
writes a map, DOT or GraphML, and the oracle writes the same map (CI compares). The viewer (O,
the pause menu's third row) is full screen: dropdown of maps ("This installation", made on a
worker from the programs and data folders beside the hallway; every `maps/*.map`; any opened or
made), 3D Fruchterman–Reingold layout (300 steps, sampled repulsion above 900 nodes, seeded by
node id so it always settles the same), drag / arrows to turn, wheel to zoom, click or Tab to
choose, Enter or double click to walk to a file (its SHA-256 checked first; past the BINARY
length it asks twice, as the locator does: both go through `walk_to_file`). Made maps are saved
into `maps/`. The item page has a third tab, SORT: a binary item whose file's SHA-256 a node of
the current map names is that node (a verified anchor by construction), drawn in its place;
anything else is a lone point. No "whole state space" graph, by design (Edward): it would be a
tangle of noise. `--map PATH` and `--graph` script it. Not yet: nodes that are items on the other
lines (a page, a picture) with their line's setup, relations other than `contains` made from the
viewer, anchors feeding filters, and bundled maps in a release.

**Pause menu returns, and installing from the locator** (27 September 2026). Every tool opened
from the pause menu now comes back to it with Esc: the navigator is drawn and handled over it (its
ENTER goes to the address and leaves the pause), and Restart Sieve's setup menu, on Esc, returns
to this hallway paused (`back_from_menu`, `restart_from_pause_`); F1 itself still returns to
walking. The File Locator has **Save Sieve instructions...** (the `.sieve`) and **Make an installer
program...**, and **Install from Sieve instructions...** (I), which takes a `.sieve` or an
installer program, asks for a folder, and installs on the worker. "Sieve instructions" is
Edward's name for the `.sieve` (27 September 2026). The manifest on its own is no longer saved
from the hallway: it is part of the instructions, for the tools; `sieve locate --manifest` still
writes it.
`installable_manifest` (tools/cli/locate.cpp) is the one reader of all three, used by `sieve
install`, sieve-install and the locator; a v1 manifest is refused (it lists, it does not hold).
`--install FILE --install-to DIR` scripts the locator's install for CI.

**Single files are handled as folders** (Edward, 27 September 2026). A file's Sieve instructions
are a folder's holding just it (`manifest_of_file`: root = the file's name, one entry), so the
locator offers a file the same two saves as a folder (Sieve instructions, installer program;
"Save its address..." as hex is gone from the hallway), `sieve locate FILE --installer/--program`
work, and sieve-install puts a single file straight into Documents rather than a folder of its
name. A bare `.address` was considered and not made: it saves ~130 bytes but has no name and no
check (every number is a valid file). The oracle's `manifest` takes a file too.



**Doors keep the angle; the minimap shows where you last stood** (Edward, 27 September 2026).
Doors used to keep the corridor tile, so a longer line was nearly always at ~0° and a shorter one
wrapped to an unrelated angle. Now `cross()` maps loop tile t of A to floor(t · T_B / T_A) of B
(exact BigUint), and a stack of doors taken since you last moved (`door_back_`) makes stepping
straight back exact, several doors in a row included. `move_tiles` moves only the current line;
`all_loop_tiles_[i]` is where you last stood on line i (0 until visited), which is what the
minimap's dots show. `all_start` is now the current line's loop start (every line starts there
through its doors). CI: 36.25% on pages is 36.25% on images, and back; six doors and back return
to the same book.

**Verified anchors and map v2** (Edward, 27 September 2026). `sieve-map-v2` (tools/cli/map.*):
held nodes (the anchor's bytes after `end`, like Sieve instructions: Edward's idea that a map is
"another .sieve, for mapping"), `sealed yes|no`, and `m` metadata lines (none written yet: room
for item metadata later). A map is v1 unless it needs v2, so folder maps and the oracle's v1
checks are unchanged. The hallway: **V** on any item tab adds the file in hand to the chosen map
(`graph_add_anchor`; files only for now; name = the real file name if walked to from one
(`walked_names_`), else the title plus the kind from its first bytes); [ ] choose the map on any
tab, and the item page shows it; the viewer's **Remove from map** / Delete (`graph_remove`).
`read_only()`: This installation and sealed maps. *(Superseded 27 September 2026, see "No shipped map" below.)* `data/maps/sieve.map` (sealed, only its root
until the first release) is the one map shipped; a `default.map` was tried and dropped (below). **At the first release:** make the source archive, then
`sieve map Sieve-src.7z --name release --seal --out data/maps/sieve.map` and build; the map
naming the published files (`sieve map sieve.exe sieve.sieve --name sieve --seal`) goes
beside the downloads, with its SHA-256 published, since an archive cannot name itself.

**New map, the V toggle, META** (Edward, 27 September 2026). No `default.map`: the node graph's
**New map...** (save dialog in `maps/`, `--new-map PATH` scripted; `sieve map --new NAME`) makes an
empty map, and "This installation" is chosen at the start. **V** toggles: it adds the file in
hand, or removes it if the chosen map names its bytes; the item page's top right says which. A
fourth item tab, **META**, appears only when the item is an anchor of the chosen map with
metadata (`hand_tabs()`, `draw_meta`); `sieve map --meta NODE:key=value` sets it (the oracle
writes the same). Anchors are for the binary line only, by design: Edward, "the other lines are
just for show really; having the binary dimension working is the real win". The superseded
formats and rules (manifest v2 installers, map v1 beside v2, doors keeping the corridor tile)
stay documented as the record.

**The first release** (27 September 2026). Published: three files, `sieve.exe`, `sieve.sieve`
and `sieve.map` (sealed, naming both; Edward's naming, "three identical filenames"; the map is both
the release's map and Sieve's, so `data/maps/release.map` became `sieve.map` too); the version is
in the folder they install, `Sieve-<version>`, and in the notes; GitHub's own source zip covers the source,
so the source's Sieve instructions are optional (`--with-source`, which also makes
`data/maps/sieve.map` name them). `tools/make_release.py` does it in order and checks it (see
SIEVE-INSTALL-USAGE.md); tested end to end on the Linux build here, not yet on Windows. Potentia's
`LICENSE` (one folder up) is copied beside the programs as `potentia-license.txt` by `sieve_data`
on every build (which now also copies `maps/sieve.map`), is listed in "This installation", and a
release refuses to go without it. Before the release, Edward (his order): (1) the licence: decided,
standard AGPLv3 (Potentia's LICENSE, one folder up); (2) icons for the programs and the file types;
(3) a full test harness run on Windows; (4) the version: 0.13.0 (set in CMakeLists.txt and
vcpkg.json); (5) the release build, with tools/make_release.py --version 0.13.0.

**The icon** (Edward's choice, 27 September 2026): the minimap itself, design "C" of three: the
eight rings in door order on a black disc (as the minimap has them on its black panel), pages
thicker, the needle at 0 degrees with every line's mark; transparent outside the disc, so round.
`data/icons/` holds only `sieve.ico` (Edward's: 16 to 256 pixels, each shrunk bicubic from the
256-pixel rendering, which keeps thin lines brighter at 48 and 64 than drawing at those sizes
does) and `sieve.rc.in` (the Windows resource template CMake fills in, so all three programs carry
the icon as resource 1 for Explorer). `tools/make_icon.py` turns the .ico's 64-pixel image into
`client/icon_pixels.h`, the window icon the hallway and sieve-install set at start
(`client/window_icon.hpp`); `--draw DIR` makes the drawing again (SVGs, PNGs, an .ico) into
another folder, for a redesign. The resource is proved on the first Windows build. File-type icons
(.sieve, .map) need registry associations, which an installer that registers types would make;
not done.

**Windows warnings fixed** (27 September 2026, from Edward's first Release build log): MSVC raised
14 warnings GCC here does not: `sscanf` (C4996; now `parse_size` / `parse_floats` in
app_settings.hpp, std::from_chars and strtof), locals shadowing locals (C4456: navigator.cpp,
hallway.cpp, hud.cpp; and two lambda parameters in node_graph.cpp found by GCC's -Wshadow=local,
which is the closest check here), and int-to-float initialisations (C4244: main_menu.cpp, hud.cpp,
menu.cpp). The build itself failed copying `data/maps/release.map`, renamed on Edward's side to
`sieve.map` (now here too). Worth running `-Wshadow=local` in future before sending code, since
MSVC's /W4 checks it and this build does not.

**Release files, final** (Edward, 27 September 2026): three, `sieve.exe`, `sieve.sieve` and
`sieve.map`. The map is sealed, names the other two, and **holds** `sieve-source.7z`: the Sieve
folder's source at the last commit (`git archive --format=tar HEAD:<Sieve's path>`, run from the
repository's top folder since Sieve is a folder in the Potentia repository, then 7-zipped). GitHub's
own source download is the whole Potentia repository. `tools/make_release.bat` runs it all with
defaults (version from CMakeLists.txt; `py -3` first, as a bare `python` on Windows 10/11 can be the
Store's stand-in).

**No shipped map** (Edward, 27 September 2026). `data/maps/sieve.map`, sealed and only its root,
is gone, and `data/maps` with it: it was meant to name a source archive (`--with-source`), and the
published `sieve.map` now holds the source instead. The real release map cannot be shipped inside
the release, since it names `sieve.sieve`, which carries the release (a hash cannot contain
itself). "This installation" is made fresh anyway. So the build (`sieve_data`) and the release
script only make an empty `maps/` folder beside the programs, for the node graph; `--with-source`
is dropped. `sieve map --new NAME --seal` now makes a sealed empty map (it ignored `--seal`
before), which the CI's read-only checks make for themselves instead of using the shipped one.

**The installer unpacks a 7z** (Edward, 27 September 2026). A release installer carried
`sieve.7z` and wrote it as it was (and a folder chosen with Browse gained the file's name as a
folder, `C:\TEST\sieve.7z\sieve.7z`: fixed, a single file goes straight into the folder chosen).
Now `sieve-install`, and only it, recognises a lone 7z by its signature and unpacks it into a
folder named after the file (`sieve.7z` -> `sieve\`), leaving out the archive's one top folder if
it has one (`client/unpack_7z.*`). The archive's SHA-256 is checked on opening; 7z checks each
file's CRC; existing files are refused unless Replace; cancel or failure removes what was written;
Unix modes are kept where the archive has them. The decoder is the LZMA SDK's C code, public domain,
from 7-Zip 26.03, in `third_party/lzma` (18 .c files, `sieve_lzma`, linked into sieve-install
alone); it adds about 80 KB to the trimmed installer (Linux 1.51 -> 1.59 MB). The locator and
`sieve install` are unchanged, byte for byte. CI: `tests/manifest_fixture.7z` through
sieve-install (unpacked, equal to the fixture) and through `sieve install` (the archive itself).
Zip could be done the same way later with zlib's inflate, if wanted.

**Release layout: tools\** (Edward, 27 September 2026). So that people start the right program,
a release puts `hallway` alone at the top and `sieve` and `sieve-install` in `tools\` with a
`README.txt` (`make_release.py`, `TOOLS`/`TOOLS_NOTE`). The build folder stays flat (Visual Studio
runs and the CI's `build/sieve` paths unchanged); the programs look in both places:
`cli::install_dir()` is the program's folder, or the one above it when that is named `tools` and
has `dictionaries` beside it (used for dictionaries, models, the corpus and `sieve-filters.ini`,
which the hallway shares); `installer_program_beside(dir)` also looks in `dir/tools`, so the
hallway's File Locator finds `sieve-install`. "This installation" lists `tools`. The release script
checks that `tools/sieve` finds the data above it (run from `tools/`, since a `data/` folder where
it is run is looked at first); CI builds the layout and makes and runs an installer program from
it. The hallway keeps its name for now (renaming it `Sieve.exe` would clash with the CLI's name in
the docs).

**F: save the item in hand; the highlight fixed** (Edward, 28 September 2026). F on any tab of the
item page (V was taken by anchors) saves the item as a file through the system's save dialog
(`client/item_save.cpp`): text as .txt, image as PNG, video as its frames in one PNG, audio as MIDI
(`cli::save_unit`, as `sieve read --out`), a model as its .obj, a book as text (title, then pages;
no cover), a binary-line file as its bytes, named by `binary_file_name` (shared with V's anchors).
`--save-item PATH` does it without the dialog, for the checks. The item highlight (the book you
look at) was drawn before the item pictures, which covered it on every line but models (whose
picture leaves the front bare), so only the models crate lit up; it is now drawn after them, over
the picture's own rectangle from faces.ini (so the audio item's highlight is its own height, not
the slot's), white where a line's edges are black (books). Picking is narrowed to that rectangle
too (`pick_book`'s bottom/top), so the space above an audio item picks nothing. faces.ini's values
were right (every model's front measured at x = 0, z within ±0.14, audio 0.006-0.286).

