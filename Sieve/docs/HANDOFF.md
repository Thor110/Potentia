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

## The nine lines

    binary | image  pages  books  audio  tracks  video  movies  models | binary

Defined once, in door order, in `client/dimensions.hpp` (`kDimensions`): `kLines = 9`, with
`binary` last (a static_assert holds it there). Each part stands before what is composed of it,
and image, a part of nearly all of them, first. The middle lines used to loop into each other
through the doors; they no longer do. They start and finish at the binary line, which is why it is
listed at both ends of the map. A door's number (`li`) is only where a line stands; what it is,
and everything saved per line, goes by `Media`.

The binary line is **one line met from either end** — one ordinary tile of corridor, the same
width and height and bookcase as any other, with one side missing. Where the other wall would
stand the floor ends at a waist-high wall (`kEdgeRail`, `kEdgeRailTop` in `client/world.hpp`) and
green rain falls past it for ever. The room is always the same way round: shelves and door on the
line's left, the drop on its right. It used to be mirrored depending on which end you came in
from, which made it two rooms; since 27 September you are **turned round** instead when you come
in from models (`cross()`), so the two halves of the ring run in opposite directions, as the two
ends of one line do. Its one door leads back to where you came from (`binary_from_`, the first
line, image, when you start there), and its sign says so. A scripted `--walk` is in the walker's frame, so a door
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

**FIND MY LIMITS focus** (GLOBAL, `--limits-focus`: all, or any one line): grows the one line and
leaves the rest as they are. Each line is judged on its own against the budget, so a line grown
alone reaches the same largest shape it would with every other line at its smallest; pages and
image grown together is what "books" and "all" do, since a book is made of both, and audio
(video) grows only while a one-unit track (movie) still fits, before the track (movie) takes the
units left.

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
| `core/src/composition.cpp` | `composition-v1`: tracks and movies, a cover, a title and N units; their filters, each unit alone and the units joined. |
| `client/dimensions.hpp` | The dimensions, each defined once, in door order: identity, names, colours, unit line, music mode. Reorder the doors here. |
| `client/hallway.hpp` | The `Hallway` class and the helpers its parts share (namespace `hallway::hall`). |
| `client/hallway.cpp` | The corridor itself: lines, position, filters, movement, input, the frame, Real Graphics. |
| `client/hud.cpp` | Everything in screen space: the compass, the panels, the readout, the item page and COST tab. |
| `client/door_portal.cpp`, `client/binary_edge.cpp`, `client/item_faces.cpp` | The doorway noise; the rain and the drop; every item's face (pages, pictures, covers, crates, files) and their render workers. |
| `client/app_main.cpp` | Options, the menus, the screenshot and scripting paths, the event loop, `main`. |
| `client/menu.cpp` | The setup menu, `find_limits()`, the budget model. |
| `tools/cli/pack.cpp`, `tools/cli/locate.cpp` | Installers: the manifest versions, packing (v4) and unpacking (the LZMA SDK), installing. |
| `client/tailoring.cpp`, `tools/cli/tailor.cpp`, `tools/cli/weigh.cpp` | COST's K and Return, the search behind them and `sieve tailor` (a line's filters tailored to items, by their shortest routes), and files weighed against their own addresses (`sieve locate --weigh`, the File Locator). |
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
core suite was then 16,270 checks, 0 failures, run once with each SHA-256 implementation, and
passes under `SIEVE_BIGUINT_PORTABLE` too. (It is 36,415 now; what came since is in the dated
entries at the end.)

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
8 px, the font's own size), rounded up to a power of two, at most the widest at which the rooms
with pictures all fit the display cache (`client/display.hpp` widest_display_px, 1024 until
4 October 2026; shared by the hallway and the setup menu's budget, so the two agree). A 410-letter page is 256 px
wide at the defaults and reads as a page; images, audio and video stay at the setting because
their titles fit. Letters that still come out smaller are dashes. A title's letters are capped at
a sixteenth of the width so a large display is not all title.

**Close-ups** are a level of detail on top (GLOBAL **close-up display size**, `--close-up`: off,
**screen**, the default, for the screen's width rounded up to a power of two, or a power of two from 256
to the renderer's widest texture). An item whose display is drawn more than 1.25 times wider on screen
than it has pixels is drawn again at the power of two that covers its width on screen, from twice
the line's width up to the setting, by the same render workers, ahead of the ordinary queue. The
ordinary display stands in until it arrives. As many are kept as the graphics memory left beside
the world and the display cache holds at the close-up size (no more than a room's items); one
unused for two seconds is dropped. The budget counts them at what the screen can show of them,
under four screens' worth and twice that for the ones kept (display.hpp closeup_bytes_most):
63 MB at 1920 x 1080.

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
apply to every line and sit in GLOBAL; the display size goes up to the renderer's widest texture
and the cache as far as the graphics memory has room (1024 px and 4 GB until 4 October 2026). `--settle N` draws N frames standing still before a
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
(3) a full test harness run on Windows; (4) the version: 0.13.1 (set in CMakeLists.txt and
vcpkg.json); (5) the release build, with tools/make_release.py --version 0.13.1.

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

**Excluded mode** (Edward, 28 September 2026). A fifth display mode, `excluded` (`FilterMode::Excluded`,
`sieve-filters.ini` `mode = excluded`): hide turned round, so the units a stack sets aside can be
walked and checked, each with the filter that failed it. Drawing, picking and item pictures skip
survivors (hallway.cpp, item_faces.cpp); the setup menu cycles five modes; the top bar shows
"(N excluded)" where the stack ranks (the line's size less the survivors; for books, the books
line's size less the surviving books), and `sieve filters` prints an `excluded` line (exact). No
core change and no new address scheme. A compact excluded mode (excluded units packed together, numbered
k = 0 .. total - survivors - 1) is possible with the same rankers, unranking digit by digit with
`base^r - completions(state, r)` in place of `completions`, but it would be a new versioned
compact scheme (and a shuffle domain for scrambled), so it waits. Named "excluded" by Edward (first built as "rejects", renamed the same night, before any release).

**Filters on every column** (Edward, 28 September 2026). The setup menu's map has a magnifying
glass on all eight columns; both binary columns open one overlay (6), since binary is one line met
at both ends (`Menu::overlay_of_column`). F on the models rows opens the models overlay (5), on
the binary row the binary one. `FilterConfig` gains `models` and `binary` (`[models]`, `[binary]`
in `sieve-filters.ini`); no filters are registered for either yet, so their overlays show the mode
and "no filters for this line yet; filter plugins will add them". CI's F-row check covers both.

**F in the setup menu** (Edward, 28 September 2026). On rows that belong to no line (GLOBAL and
the three at the foot), F opens the filters of the line you will start on (the "line" row; pages
by default); F again closes them. CI checks both (pages by default, models once the line row is
turned five times). The filter-plugin proposal is `docs/FILTER-PLUGINS.md` (draft, for review).

**The vault** (Edward, 28 September 2026; docs/VAULT.md). Hashes of content Sieve refuses to show,
emit or pass on; not a filter, no setting turns it off. `tools/cli/vault.*` (in `sieve_locate`);
`sieve-vault-v1` files hold only hashes; built-in entries compiled in (the test entry, SHA-256 of
"sieve vault test"); `vault/` beside the programs (build and release copy `data/vault/test.vault`)
adds to them; a broken vault file fails closed. Checked at: walk/locate/install (including the
installer's 7z), map held anchors, warp input, CLI output (`preview`, `read`, `browse`, `unbind`,
`mesh`), hallway items (`Book::withheld`, `vault_withholds`, `file_withheld`, `refuse_if_withheld`),
F. `sieve vault [FILE...] [--parse]`. PDQ (pictures) is next. Edward is weighing a custom licence
clause forbidding its removal (AGPL lets recipients drop added restrictions); undecided.
Text is out of the vault's reach (Edward, same night): pages, books and titles are never withheld;
files (the binary line, and a byte-alphabet line), pictures (PDQ, next), melodies and models are.
The docs say plainly that an address cannot be made secret (it is neighbour plus or minus one, and
it is the content): the vault only makes sure Sieve is not what hands it over.
Then (same night) Edward's point that a file can be written out as text, even on lower27 ("c7 a9
9a 7c", a-p nibbles, spelled digits), so the vault is addressed at every level Sieve can see: text
is checked as a file written out, through `vault-decoders-v1` (tools/cli/vault_decode.*: its own
bytes, hex, base64, base32, ascii85, decimal, nibbles, spelled, binary), on every unit, page, title
and book (pages as one); never by what it says. `sieve vault --written FILE`. So "sieve vault test"
as text is now withheld again, as the file it is. Next: PDQ (pictures, and decoded bytes that are
images), then `chunk` entries for files split across units.
PDQ built (same night): Meta's reference C++ vendored unedited in third_party/pdq (ThreatExchange
ec3671b, BSD), wrapped by tools/cli/pdq_hash.*; vault format sieve-vault-v2 adds `v pdq <hex>`
entries (v1 still read); match rule pdq-match-v1 (quality >= 50, any of 8 orientations within 31
bits). Checked on every picture drawn (image units, video frames, covers) and every file that is a
picture (stb formats, each frame), so also bytes decoded from text. Agrees bit for bit with
Meta's pdqhash on PNGs (JPEGs within 2 bits: stb and libjpeg decode slightly differently). A second
built-in test entry: the test picture (`sieve vault --test-picture`). The G message no longer
overwrites "withheld". Misses, documented: heavy crops, few-colour palettes, quality < 50. Next:
`chunk` entries; then the filter plugins (step 1 needs the full test harness: ask first).
`--timings` (same night; Edward's 5 s stall on the setup menu turned out to be VS2022 and other
programs in the background, not Sieve): tools/cli/timings.* in sieve_locate, a table per phase
(runs, total, mean, slowest) written at exit to stderr, and by the hallway to sieve-timings.txt
beside its settings. Phases: sieve <command>, hallway.build, hallway.item(.filters/.vault),
hallway.face, hallway.frame.update/render/present, menu.main.frame, menu.setup.frame,
menu.budget.measure(_binary), menu.survivors(.books), vault.load/bytes/decoders/pdq. Off costs one
atomic read. First reading: on the image line nearly all of an item's cost is PDQ (0.11 ms of 0.13).
Worth doing later, measured with this: the item work on the face workers' pool (it is pure, per
address; an item is shown only once its vault check is done).
Chunks built (same night): core/include/sieve/chunks.hpp, `cdc-v1` (gear hash, top 8 bits zero,
64..1024 bytes, gear[i] = SHA-256("cdc-v1 gear"+byte i)[:8] big-endian), with a streaming Chunker;
the oracle's independent `chunks`/`chunk-vectors` and tests/vectors_chunks_v1.tsv (C++ checks them,
also fed in pieces; CI regenerates and diffs). Vault format sieve-vault-v3 adds `v chunks
h,h,...` (one file per entry); `chunk-match-v1` = two distinct chunks of one entry (one for a
one-chunk file), uniform chunks never count. Checked in withheld_bytes, check_known, and
check_file (streamed, 1 MB blocks). `sieve vault --chunks FILE... [--common FOLDER]` writes a
vault file; `--test-file` writes the third built-in test (listed by chunks only). Measured catch
rate on random data: 2 KB 98%, 1.5 KB 86%, 1 KB ~65%, 600 B 22% (the ~600 B I first told Edward
was optimistic). Edward's point: chunking is general (filters, maps), so it is in the core;
VAULT.md section 8. Next: the filter plugins (step 1 needs the full test harness: ask first).
A located file is now read once (vault::hash_checked_file: SHA-256 and chunks from the same 1 MB
blocks; no threads needed), replacing sha256_file_hex + check_file in locate.cpp. Noted for the
multithreading work: the hallway's item work (content, filters, vault) on the face workers' pool;
the menu's survivor counts and budget test off the main thread. Chunk sizes as a parameter:
a named family (cdc-v1/min-avg-max), VAULT.md section 8, with the plugins.
Filter plugins, step 1 (and 2) built (same night; full test harness run locally, all Linux CI steps
pass except the model rebuild, which needs the network): core/include/sieve/dfa.hpp (canonical
minimise, intersect, DfaRanker), core/include/sieve/plugin.hpp (sieve-filter-v1 table form with
`param NAME int DEF MIN MAX`, {expressions}, for/done loops, @digit symbols; registry merged into
find_filter/filters_for; a stack of plugins ranks via the product), tools/cli/plugins.* (filters/
folder, refusals listed), `sieve filters --plugin/--plugins`, oracle `plugin` (own parser, Hopcroft
with a sink), data/filters: clean-data-v1, max-run-data-v1, key-data-v1. Edward: parameters must be
adjustable in plugins (done, int only in v1); `.sfilter` kept; Moby later. Next: the token form
(step 3), then the menu's plugin details (step 4).
Prerequisites, tabs, relations (same night, full harness run again, all pass): `requires NAME-vN
[k=v ...]` header lines (FilterSpec::prerequisites; loader refuses a missing/refused/self/badly
pinned prerequisite); cli::tick_filter (ticks the closure with pinned settings, keeps existing
settings), cli::prerequisite_notes, build_stack includes prerequisites; setup menu filters window:
BUILT-IN / CUSTOM tabs (otab_), info rows (author, origin, file, requires, refused files, notes);
dfa::subset and `sieve filters --plugin F --relations`; data/filters/tidy-data-v1 (requires example).
Oracle reports `requires` too. Planned: the filter designer from the main menu (FILTER-PLUGINS.md
section 12): entry node = prerequisites with + per entry (settings node) and + to add; the filter
list as reference; blocks not states; live test panel; relation check on save; then Moby.
Step 1 of Edward's order (word sets + toggle all) built, full harness run, all pass: Z/C in the
filters window (this tab / both tabs; ticks with prerequisites, or unticks if all on); token form
(`tokens separator`, `edges whole|cut`, `set NAME dict:ID|dict:{param}|list:FILE`, `follow`,
`first`, `last`; `param NAME dict DEFAULT`, FilterParam::registry); TokenCompiler (trie + reading
masks, generalized suffix automaton for cut); compile cache; oracle token_nfa_dfa (NFA + subset
construction) and a proper Hopcroft (203 s -> 9 s); words-data-v1, window-data-v1;
tests/plugins/toy-grammar(-cut)-v1 with lists. Fixed on the way: the menu's text parameters with a
fixed list (key-v1's tonic, scale) cycled through the registry instead of their own choices.
Next: 2 the designer (FILTER-PLUGINS.md section 12), 3 Moby (built by hand, not in the designer).
Step 2, the filter designer, built (full harness run, all pass): main menu item after Settings
(Result::Designer; Esc returns to the main menu). client/designer_model.* (Doc <-> .sfilter with
layout comments; test() compiles from a scratch folder, counts, 5 samples; judge() explains
failures, table errors in editor line numbers; relations(); save(): version never overwritten,
lists never changed underneath, add_plugin registers at once) and client/designer.* (nodes rebuilt
each frame, fields walked by keys, mouse pick and title drag, auto column layout until a node is
placed, follow arrows routed round the side, multi-line editor, worker for tests and relations).
Core: plugin registry is a deque with add_plugin. Scripts: `hallway --designer --design F --script
"keys,=text" --design-out F`, Ctrl+ in --press. Next: step 3, Moby (by hand).
Designer progress window (Edward: compiling was slow in his debug build and it was not obvious
what was happening): centred window while a test/relations job runs over 200 ms (step from
design::Progress, fed by compile_plugin's new `step` callback, elapsed time, spinner), then 2 s of
the outcome; Esc hides it; Save while busy waits (pending_save_) instead of freezing. Minimise is
now Hopcroft on flat arrays (0.95 s -> 0.43 s on 418k states, release); oracle comparison of every
plugin identical, full harness passes. `hallway --designer --busy` screenshots it mid-job.
Designer tests only on request (Edward): nothing compiles on opening or editing; F5 / TEST NOW
(test panel item 1) requests it; Save, relations and judging request it when stale (result_key_ vs
current_key(): the file without layout, plus the length), with pending_save_ / pending_relations_
done after; Save refuses a filter that does not compile. CI: `hallway --designer --timings` shows no
designer.test run.

Step 3, the Moby grammar filters (by hand, as planned; FILTER-PLUGINS.md section 13,
data/filters/moby-pos-v1.md). tools/moby_pos.py converts Moby Part-of-Speech II (public domain,
Grady Ward's grant of January 2001; third_party_licenses/Moby) to data/filters/moby-pos-v1.tsv and
makes moby-inflections-v1.tsv and moby-names-v1.tsv from it and SCOWL en-80. Token form: `set NAME
tags:FILE+FILE:TAGS[-EXCLUDED]`; a word is judged by the tags of all its lines together (every
file, every spelling folding to it), found when "quickly the of" passed because Moby's "OF" is a
noun. moby-grammar-v1 (lenient) and moby-grammar-strict-v1: follow/first/last chosen by a greedy
search (real minus weighted shuffled) on three NLTK Gutenberg books, measured with the compiled
plugins on 2,790 held-out sentences: 83.5% real / 42.8% shuffled / 41.4% random words (lenient),
58.8 / 18.4 / 17.1 (strict). 258,024 words; 287,267 and 215,179 minimal states; about 6 s to
compile cold (release). Oracle: its NFA subset construction passes ten million states and does
not fit in 8 GB, so `plugin --lazy` (subsets made as reached) and `--judge FILE` (engine: `sieve
filters --plugin F --judge FILE`, # lines are comments); identical to the engine at length 8 and
on 200 held-out sentences, and at length 6 in CI. The oracle's minimiser and token NFA now use
integer states and flat arrays (same results, far less memory). tests/plugins/toy-tags(-cut)-v1
with tags-a.tsv/tags-b.tsv (CI, engine vs oracle), unit tests for both grammars (compiled once per
run), CI expects 8 loaded plugins. Designer: opens and tests filters with tagged lists (read where
the file is, copied to the scratch folder once; Save copies them beside the saved filter).
Limits: imperatives fail (no sentence may start with a verb); a word in no list fails its sentence;
no punctuation.
Background music and the Media Player (Edward's design; no plugins or listening samples first:
tuned by ear later). client/music.* : MusicPlayer, one SDL audio stream with a callback, mixing
three channels (MENUS, WORLD, the melody in hand) synthesised sample by sample (voices soft, sine,
triangle, square; tempo; a feedback echo), gains fading over 1.5 s; the inactive mode's channel
waits where it was. Tracks: a worker picks one per mode (count the mode's stack, uniform random
rank below the count, unrank; no filters: any unit; judge-only stack: up to 5000 draws; vault-
withheld units skipped), the first at once, later ones after the mode's gap. Defaults: menus
key-v1 A minor, sine, 66 bpm; world key-v1 C major-pentatonic, soft, 72 bpm, more echo. Recent
(10, chosen tracks only), favourites as sieve-favourites.sieve (v3 manifest: NNN-mode.mid +
favourites.tsv; installs with sieve install), settings and recent in sieve-music.ini beside the
hallway's settings. present(r) draws the NOW PLAYING box on every screen; music_mode() is set by
each screen (menus) and per hallway frame (Hallway::in_menu()). P plays through the mixer (music
ducks). client/media_player.cpp: pause menu item after File Locator; three columns (recent,
controls with the mode's filters and params, favourites), actions play / go to (same length:
audio line + go_to_unit; else Request::GoToTrack, app_main rebuilds with notes = track length) /
save MIDI / favourite. --no-music; --media-player gives scripted runs a silent player. Checked
headless with SDL's disk audio driver (levels, ducking, crossfade), favourites round trip and
install. CI: Media Player screenshot. The test harness does not cover sound.
The NOW PLAYING box takes the colours of where you are (Edward): the line's background and edges in the hallway, paused or not (music_colours() each frame), white on black in the menus (music_colours_default()).
Each line's character (Edward): WORLD plays every line in its own mode (MusicSettings::modes, one per
line, default image Lydian, pages Ionian, audio Mixolydian, video Dorian, books Aeolian, models
Phrygian, binary Locrian) by moving each note from its step of key-v1's scale (read as its
seven-note parent: pentatonics and blues as major/minor) to the same step of the mode on the same
tonic, plus optionally the key round the circle of fifths (fifths, off by default); a per-pitch
shift table on the WORLD channel, recomputed on set_line and settings changes, taken up at the next
note. World default scale is now major (the pentatonic lacks steps 4 and 7). Hallway::set_line calls
music_line() (colours + line), as does the hallway loop on (re)entry, instead of every frame.
Tracks record their line: WORLD [BINARY] in the lists, world/binary in sieve-music.ini and
favourites.tsv (older lines read as before), NNN-world-binary.mid.
Melody filters, Part 1 of Edward's plan (FILTER-PLUGINS.md section 14; full harness run). The
format sieve-filter-v2 (v1 untouched): comparisons, && || !, min max abs, if/else/fi blocks,
`param NAME choice DEFAULT A,B,C`, constants BASE and (note lines) PITCHES DURATIONS LOW, and
`symbols notes*`. Engine (Expr v2 grammar, parser, Compiler if/else, choice as index) and oracle
(its own) agree; the oracle comparison caught an oracle bug (if reused the loop bound j), the unit
tests an engine one (choice params without a description refused). Plugins: key-data-v2,
melody-leap-v1, melody-lengths-v1, melody-rests-v1, melody-ending-v1, melody-range-v1, and
tests/plugins/toy-v2-v1. Player defaults are stacks of them (they count, so tracks by rank);
falls back to key-v1 without the plugins; line character reads key-data-v2 or key-v1. Designer:
format field, choice params (kind int -> dict -> choice), notes* tested on notes104. CI: 14
plugins loaded; oracle diffs for all six at four lengths. IDEAS §12: cross-line filtering
(Edward), loudness per note (later), book format v2 for notes2, rules between voices.
Next: Part 2, the notes2 family (range C3-C6 default up to C2-C7, eight durations, up to 4
voices filtered one by one).
Part 2, the notes2 family (SPECIFICATIONS §3.2; full harness run). core/audio: NoteSet (notes104
described too, legacy: its functions and MIDI unchanged), make_note_set / note_set_of / note_name,
canon-notes-v2 (s e e. q q. h h. w, // voices, nearest duration with ties to the longer, octave
moves, voices padded to equal runs), notation with " // ", format-1 MIDI with a track per voice.
filter.cpp: FilterStack on a note line of V voices builds its filters for one voice's line and
judges each voice; VoicesRanker (count^V, mixed-radix rank/unrank, packed 3+16+45-bit states);
voices_ranker() exported. plugin.cpp: PITCHES/DURATIONS/LOW from the set. lines: --note-set
notes2 --low --high --durations --voices (length per voice); warp, preview, vault and MIDI by set;
books refuse notes2 sections. Oracle: its own notes2 (NoteSet2, canon_notes2, notation, MIDI),
notes2-vectors -> tests/vectors_notes2_v1.tsv (12 cases), plugin --note-set/--voices; 108
engine-vs-oracle plugin comparisons identical. Tests: vectors, round trip, notes104 equivalence,
refusals, a two-voice stack exhaustively against brute force (26^4 units). Hallway: Settings
note_set/note_low/note_high/note_durations/voices (setup menu audio rows 18-23; rows below moved
by 5, CI pins updated), in-hand notation by set, mixer and fallback synth play every voice,
Media Player note-set rows, tracks/recent/favourites carry their set (ini: notes = ..., recent
lines name a notes2 set before the notation; favourites.tsv a fifth column), go to a track
rebuilds the hallway at its set. Known limits: melody-lengths-v1 and melody-ending-v1 are
notes104 only (their duration choices); books hold notes104 only.
One line filtered by another (FILTER-PLUGINS.md section 15; full harness run). Edward's idea,
with the arithmetic agreed first: exclusion (not another line's content) removes only the share
another line takes up, which is tiny; requirement multiplies shares. Built as four parts.
(1) core/filekind: file-kinds-v1, the hallway's kind table moved to core with MID added
(Hallway::file_type delegates); KindCounter counts and ranks files of chosen kinds on binary-v1 at
any length (heads of <= 16 bytes through a small automaton; longer files are heads times
256^(L-16)); BinarySieve is the binary line's stack, with compact positional/scrambled.
binary-kind-v1 (kinds, keep) is registered for FilterLine kind "binary"; symbol-entropy-v1 no
longer applies there. (2) core/written: not-written-v1 for text lines. Each vault reading but
ascii85 is a machine walked into an automaton, and their union O is minimised. The binary
reading is walked by symbol class (first, other, whitespace) as Bn (872 states), and a stack's
count is |P| - |P&O| - |P&Bn| + |P&O&Bn|. The joint terms are walked with the two symbols named,
cut to small per-pair automata once both are known; a joint budget falls back to judge-only.
FilterStack combines it with plugins; make_dfa_filter is exported; dfa gained unite and
complement. (3) Oracle: kind-vectors (583 rows) and written-vectors (585 rows, about 3 minutes):
its own machines, its own closed form for |Bn|, and the overlap walked pair by pair. The engine
matched all of it, and brute force at small lengths in development (43 million units on
3-symbol alphabets at length 16). (4) CLI: sieve filters prints shares as powers of ten;
--line binary lists the filter, stack and every kind's share (--exact); sieve check --line
binary --file F; locate prints the kind. Hallway: binary filters in the setup menu (overlay 6,
survivor bar), the modes on the binary line (compact closes up the survivors, titles blank), J
between an item and its file (item_file / open_as_kind in item_save.cpp; cli::unit_file and
image_io encode_png factored out of save_unit), and midi_to_notation in core/audio so a MIDI file
opens as a melody (300 round trips in the tests). Measured: pages (lower27, 32) set aside
10^-4.23, mostly base32 (any letters are base32); binary TXT about 1 in 7 at 16+ bytes, which is
why TXT clusters are easy to find.
The setup menu's filter overlay shows a live tally beside its title (Edward): "amount of content
filtered", the exact share of the line's units the ticked filters remove (filtered_text in
menu.cpp: truncated, decimals until past the leading 9s or 0s plus one, at most twelve; the small
side as a power of ten), from StackInfo::filtered, computed with the survivor count (books: the
parts' product). A note that "display mode: off" leaves every unit shelved was offered and
declined for now.
Every line filtered by every other (Edward's table; FILTER-PLUGINS §16). Four filters, each
exact on its own. not-a-file-v1 (core/src/filters/crossline.cpp; every line but binary): the
unit's number as a binary-v1 index; KindCounter::count_before(x) gives the signed files below x
from the head alone, so survivors below x are x - E(x), completions S(hi) - S(lo), unrank by
binary search. It is not an automaton, so with other ranking filters the stack judges only.
not-other-line-v1 (written.cpp NotesMachine/ObjMachine; text but bytes256; forms all|notes|obj)
and not-packed-v1 (BitsMachine; image/video with 2/4/16/256 colours) are DFA filters
(make_dfa_filter) and combine with plugins, each other and not-written-v1. not-an-item-v1
(binary; items pages|melodies|pictures|models|all): pages are a KindCounter::Pattern counted
exactly (|K| - |K&P|, rank by pattern_below); the rest are judges supplied by the application
(cli::binary_items in filter_config: PNG round trip through canonicalise_image and unit_file,
MIDI through midi_to_notation and notes_to_midi, .obj through from_obj/to_obj), which make
BinarySieve::can_rank false with a blocker (hallway falls back to hide and shows why; menu shows
not countable). BinarySieve::first_failure_of(file) judges whole files; needs_file() when items
are asked for. The models line has no filter stack yet. sieve filters/check --line binary take
--page-length. Oracle cross-vectors (115 rows, ~10 s): its own count_before, notation and .obj
machines, packed closed form, pages walked through its head automaton; all match. Tests: 48,300
checks. Measured: pages not-a-file 10^-4.38; ascii95 not-other-line 10^-32.93 (none on lower27:
no digits); image not-packed 10^-4.34, not-a-file 10^-5.54; audio not-a-file 10^-5.30; binary
40 bytes with ascii95/20 pages (see FILTER-PLUGINS §16 for the figures). GAME.md gained the credits replay/graph and the emulator-logo idea.
F no longer appends a line feed to a page saved from an alphabet without one (Edward: it should
never have been there). cli::unit_file writes exactly the page's text on every alphabet, so a page
and its file are one to one and J lands on the 32-byte file of a 32-letter page. No address
changes (a page's comes from its symbols, a file's from its bytes); older 33-byte saves keep their
addresses and still open. not-an-item-v1's pages pattern (page_pattern, the tests, the oracle's
pages_pattern) was corrected in place, before any release, with Edward's agreement.
not-written-v1's "trimmed with a line feed" reading is unchanged (versioned).
The models line has a filter stack (Edward's table, last row). core ModelSieve
(modelsieve.hpp/.cpp): judges a model by its positional index (a mixed-radix number), holds
not-a-file-v1 via NumberFiles (x - E(x), unrank by halving), compact positional/scrambled
(shuffle keyed with the key, domain = stack id). not-a-file-v1's applies now includes kind
"models" (its make throws there: ModelSieve sieves it); symbol-entropy-v1 no longer offered on
models. cli: models_filter_line, build_model_sieve; sieve filters --line models; sieve mesh --warp
prints the stack's verdict and compact addresses. Hallway: model_sieve_, effective_mode, status,
units_of, shelves (compact: model_at, blank titles, compact hex; else failed_by), G parses compact
hex. Menu: overlay 5 lists the filters, stack_info(5) counts, ticked count on the map. Oracle
cross-vectors gained models/model-unit rows (125 rows); tests brute-force the 3/1/2 shape through
both orders. Default models (8/12/16) set aside 10^-5.54. The tiers of the models line's own
filters (SPECIFICATIONS §12) are still to come; ModelSieve accepts only not-a-file-v1 so far.
The models line's own filters, tiers 1 and 2 (core/src/filters/models.cpp specs; MeshRules in
modelsieve.cpp): distinct-vertices-v1, distinct-indices-v1, every-vertex-used-v1. Vertices and
faces are separate, so kept = (vertex strings kept) x (face strings kept) and rank = vrank *
fcount + frank, which is the positional order. Vertices: falling factorial of C^3, a vertex's
place = points left below it. Faces: completions(m unused, k named in the face, faces left) =
sum_j (-1)^j C(m,j) tail(V-j,k) (face ways(V-j))^left, tables of powers and binomials; ranking
budget 3F*V*(V+1) <= 4e6 (else counts, judges only). not-a-file with the rules: judged, blocker
shown (the open problem, item 1 of Edward's list, to be discussed next). Oracle: brute force of
the 3/2/2 shape (373,248 models, all seven combinations) and its own face walk over (used set,
face so far) at three larger shapes; 218 cross rows, all matching. Defaults: distinct-indices
keeps 10^-2.20, all three 10^-2.21.
Notes cleared (Edward's loose files): GAME.md gained "Start Game" (only if the game needs a predefined
state space); FILTER-PLUGINS §17 records hard/soft filter categories, titles that are not words
(model-information-v1 is the existing tool; entropy fails on short titles) and a proposed
not-a-pattern-v1 (periods and ramps, which symbol-entropy passes). Nothing built.
not-a-pattern-v1 (core/src/filters/pattern.cpp; Edward's ramp idea): repeats of a block of up to
`period` values (default 16) and ramps (value k = a + k*d mod B^w), values of width 1/2/4/8
digits, big/little. Exact ranker (PatternRanker, prefix-interned states; rank/unrank walk with
PatternRules::excluded_after): repeats by Mobius/Mertens over the smallest block (Fine-Wilf),
ramps B^(2w), both = ramps with j*d = 0 (steps list D). Oracle: direct judge, brute force of ten
small lines, inclusion-exclusion over period sets and Euler phi at full size; 60 rows. Defaults
set aside: lower27/32 10^-22.89, bytes256/32 10^-38.53, image 10^-24.99, audio 10^-16.13. Not in
v1: float ramps, binary line, curves. filters_for counts in the tests rose to 14 (text), 5 (image).
Start-up and menu stalls (Edward: Z/C and Start Sieve "locked up" in all builds, after C ticked
every filter). Causes and fixes: (1) the two Moby grammars compile in ~9 s each (Release) and were
compiled on the menu's drawing thread and at every launch: compiled automata are now also kept on
disk (set_plugin_cache_dir, executable_dir()/cache, sieve-dfa-cache-v1: sparse transitions plus a
SHA-256 of the file, verified on load, written via rename; plugin_compiled() asks). (2) The menu
counted every stack on the drawing thread: Menu::resolve now runs each line's count on a worker
(shows "counting..."); the workers outlive the menu and finish_filter_warmup() joins them before
the hallway is built and before main returns (statics). Scripted menu screenshots wait for them.
(3) Counting tables were built for every filter at every build, even in hide mode (a Moby grammar
at 32 letters is ~700 MB): Filter::can_rank() says whether one would exist, PluginFilter and the
word filters (LazyRanker) build theirs on first use, FilterStack settles compact lazily
(settle()), and the hallway asks only for lines in compact mode. (4) BinarySieve with pages:
pattern_below rebuilt the pattern table each call (15.5 s start): the table is kept
(pages_table_), 1.0 s. Measured, everything ticked: 23 s / 1 GB -> 2.6 s / 594 MB (cold compile
once, then cached). Menu: X toggles every filter on both tabs of every line (Edward's wording in
the footer).
Follow-up (Edward: VS Debug/Release stuck at the loading screen, packaged Release fine; no tally
on MODELS FILTERS). The tally was switched off for overlay 5 by an old condition: removed. The
compile cache moved from next to each executable to one per-user folder shared by every build
(cli::plugin_cache_dir: SIEVE_CACHE, else %LOCALAPPDATA%\Sieve\cache, $XDG_CACHE_HOME/sieve or
~/.cache/sieve, else next to the executable), so Debug reuses what Release compiled rather than
compiling for minutes and never finishing. compile_plugin holds a per-key lock, so two threads
never compile the same automaton at once (the second waits and takes it from the cache). The
hallway build no longer waits for the menu's counting workers; at exit, if one is still running,
the program flushes and leaves with _Exit (waiting would hold it open, and statics would go
first). Note for Visual Studio: start with Ctrl+F5 (without debugging) to compare timings; a
debugger slows heavy allocation.
Filter conflicts (Edward: "the safe thing", auto-untick; docs/FILTERS-CONFLICTS.md). FilterSpec
gained counts_as (automaton, written, own, arithmetic, model-rule, or "" for judge-only), and
filter_conflict(a, b) returns "", "merge" ("filters need merging": both automata underneath, not
yet merged) or "conflict" ("conflicting filters": not-a-file / not-a-pattern). Implication never
conflicts. cli::tick_filter_by_hand (menu, media player) unticks conflicts; build_stack and loading
never do. Z/C/X skip filters that clash with one already ticked (first in list order wins). The
selected filter shows its conflicts in red (filters.conflict.merge / .hard). Judge-only filters
(max-run, symbol-entropy, model-information) are left out: Edward's group 3, next. Next build step:
the word filters, neighbour-agreement and key-v1 hand over their automata so "merge" pairs combine
(with an early state limit so an oversized merge gives up fast).
max-run-v1 is an automaton now (statistics.cpp max_run_dfa: states start, SPACE, and (symbol, run
1..R); a symbol one past max_run is dead; minimised; make_dfa_filter with the same provenance, so
stack ids are unchanged; the old judge remains when the table would pass 2^24 entries). counts_as
"automaton": it merges with the custom filters and not-written, compacts on its own, and "needs
merging" with the word filters. Oracle: max-run rows in cross-vectors, counted by its own (last
symbol, run) walk; tests: every unit of three short lines judged, counted and ranked, and a merged
stack with not-written compacts. Two older tests that used max-run as their judge-only example now
use symbol-entropy.
Retired filters (Edward): FilterSpec::retired, set on symbol-entropy-v1 and model-information-v1
(judge only). The filters window has three tabs (tab_of: built-in, custom, retired); retired ones
tick only by hand or Z on their tab. Toggle-all fixed: it unticks when anything in reach is ticked
(with clashes skipped, "all ticked" was never reached, so it never toggled off), ticks newer
versions first and arithmetic filters last, and ticks title-v1 only on the books' title part.
Footer: "both main pages". sieve filters marks retired filters. Edward's measured kept shares with
everything countable ticked: binary 10^-4.34, pages 10^-18.32, image 10^-2.32, audio 10^-3.37,
video removes 10^-4.34, books 10^-17.23, models 10^-2.97.
The October 2026 filters (Edward's go-ahead after a review of the stack; FILTER-PLUGINS.md §19).
Plugins: melody-metre-v1, melody-ambitus-v1, melody-gapfill-v1; letter-pairs-v1, letter-triples-v1,
word-cost-v1, function-words-v1 (+ function-words-v1.tsv), sentence-shape-v1, babel-punctuation-v1,
generated by tools/build_text_plugins.py (--check in CI) from scowl-en-80-names and -60-names;
tools/build_anchor_plugins.py (near / contains) with examples in tests/plugins. Built in:
palette-size-v1 and row-runs-v1 (core/src/filters/picture.cpp, own rankers, arithmetic rank on any
palette), canonical-mesh-v1 (CanonicalMesh in modelsieve.cpp; `sieve mesh --warp F --canonical`,
canonical_mesh()), utf8-valid-v1 (Utf8Counter in filekind.cpp, a BinarySieve filter that needs the
whole file), FilterSpec::category (hard/soft, one table in builtin.cpp, shown by sieve filters). The
new built-ins are registered last, so existing filters keep their places in every list. Oracle:
cross-vectors gained canonical rows (old rows unchanged; it now takes about 45 s), picture-vectors
and utf8-vectors are new; the choice-name rule (letters, digits, # and -) is now in §14 and the
oracle. Core suite passes; the client builds. Not built, with reasons, at the end of §19.
LOCATING, and the hang walking to a long file (Edward, 2 October 2026: Windows thought the hallway
had crashed, and a click or Alt-Tab could leave it minimised). Measured on a 5 MB file: the walk
itself took 0.6 s, but in thin mode book() built the rooms either side of yours and then threw
them away, so both were rebuilt on every frame (64 files of about 40 ms each, per room per frame);
and each file's head built a 10-million-character "0101..." string to parse. Now: in thin mode
the neighbouring rooms stand bare and are never worked out; file_head makes 0101...01 by a shift
and one small division, kept for the last length (40 ms to 15 ms an item); and Hallway::busy()
runs the long part of a walk (the line made long enough, the file's place, its room and the hex
of the file in hand, warm_room()) on a worker while the main thread keeps the window answering
and, past 200 ms, draws a red LOCATING with counting dots, as CALCULATING does. Input meanwhile is
dropped, a quit is kept for afterwards. What touches the renderer (clear_faces, the line change)
happens on the main thread first; the job must not. Used by the File Locator's Go to it, the node
graph's walk, J to a long file, and T with a path on the binary line. Scripted runs: `--thin` no
longer turns thin off after a --press walk made it thin. Timings phase `hallway.walk`.
Looking around a room of long files (Edward, 2 October 2026: hallway.exe located, then lag as the
pointer crosses its neighbours). Each new file under the pointer cost, on the main thread, its
bytes from its number, the vault over them, and its whole address in hex for the panel's short
form: about 100 ms a file of 5 MB here. Now, with nothing shown differently: BigUint::from_bytes
and to_bytes (eight bytes to a limb) replace the hex text BinarySpace went through both ways, and
0101...01 is made from its byte pattern (a file to its place 102 ms to 13 ms, back 56 ms to 22
ms); the panel and the readout shorten a file's address from the number itself (short_hex_of: a
shift and the low limb, the same text as short_address(hex_of(b))); each item keeps the vault's
verdict on its file (Book::file_vault), so looking back costs nothing; and vault_ahead() checks
your room's files on a worker as soon as the room is built (binary line of 64 KB or more), middle
of the wall first, keyed by slot and room generation (room_gen_, bumped whenever the cache is
cleared or moved), so a file is usually checked before you look at it. The worker reads only the
files' places (BinarySpace::file_at_place, static, so a line rebuilt meanwhile cannot dangle) and
the vault, which is read-only once loaded. Timings phases hallway.file.bytes, .vault, .hex.
Angle Precision to 20 places (Edward, 2 October 2026; kMaxAngleDecimals in app_settings.hpp, used
by the settings file, the setup menu and the hallway). The compass bearing was a double (about 16
figures, from log10 approximations), so places past the tenth or so were noise; it is now exact,
floor(first unit of your tile x 360 x 10^d / units) in integers (exact_degrees in hallway.hpp,
shared with the navigator's bearing field), worked out in refresh_labels() when you move, not per
frame. "359." and 20 places is 25 characters, 200 px under a 214 px compass. Scripted runs apply
Angle Precision before --press keys (make_hallway's angle_decimals), as a person's would be.
Stuck at LOCATING with every filter maxed (Edward, 2 October 2026: all display modes compact,
every filter at its most, hallway.exe walked to from the node graph). The cause was
BinarySieve::unrank with not-an-item-v1: it binary-searched the survivor number across every page
it excludes (27^32, about 2^152, on an 8.5 MB line), so about 152 whole-file unranks of 140 ms each
per item, for the 64 items of the room: it never finished. Now, exactly and with the same files:
files are ordered shortest first and every page is page_bytes long, so a survivor past the
page-length block has every page below it, and its file is the counter's unrank of k + pages_in_
(the search remains for survivors of page length or shorter). KindCounter::shorter_than is public
for it. Then the room: find_room_files() works out the left wall's files on every core (an atomic
slot index, std::thread per hardware thread) before book() picks them up (found_, keyed by slot
and room_gen_, emptied after warm_room): compact, each found by its survivor number (file_at,
head, place); otherwise its title, head and place and, where a filter needs the whole file
(utf8-valid-v1, not-an-item-v1), its verdict. The room's first unit and its file are worked out
once per room (room_unit, file_place; room_first_, room_first_file_), not once per item. The
helpers in filekind.cpp make their numbers from bytes, not hex text; BigUint::mod_small gives a
remainder without a copy (short_big). Timings phases hallway.walk.length, .place, .room, .find.
Measured here (4 cores, hallway.exe of 8.5 MB, graph walk): binary-kind-v1 compact 9.9 s to 5.1
s; not-an-item-v1 compact never to 5.7 s; utf8-valid-v1 (it cannot rank that long, so it hides)
7.0 s to 3.0 s; all three 7.0 s to 2.9 s. On fewer cores the room takes proportionally longer.
busy() still cannot cancel a job that is running.
Thin stayed on for every line after a walk to a long file (Edward, 2 October 2026: titles gone
from most items, no record covers, tapes only drawn when stepped up to). A walk past the budget
(the File Locator, the node graph, J to a long file) set thin for the whole hallway, so on every
other line the rooms either side stood bare and only your own room had pictures (face_rooms 0).
Now set_thin(on, line) records the one line it is for (thin_line_; -1 for every line), and thin()
asks whether it applies to the line you are on: a walk makes only the binary line thin, while the
setup menu's go-in-anyway and --thin still make every line thin, as before.
Filter list round (Edward, 2 October 2026: utf8-valid broke binary's percentage; X ticked
neighbour-agreement instead of row-runs on video; a Filtered: X% beside each filter; start merging
and retiring; a note on optimise all and COST).
- utf8-valid-v1 is counted with binary-kind-v1 and not-an-item-v1's pages: KindCounter::utf8_count
  walks the head automaton and UTF-8's side by side over the first 16 bytes, then multiplies by
  UTF-8's completions (Utf8Counter::tail_sums from its table). Past the table (about 7.4 KB) the same
  walk runs on Scaled (a double and a power of two) with Utf8Counter::tail_estimate (a matrix power
  of [[M,0],[M,I]], 18x18, rescaled after each product), so 8.5 MB takes microseconds. BinarySieve
  gains can_count / count_exact / survivors / survivors_log10, apart from can_rank (compact still
  needs utf8-valid alone). The menu shows the exact share, or `~` and the estimate
  (filtered_estimate), and two new footers (status.survivors_no_compact, survivors_estimated).
  Oracle: utf8-joint rows in vectors_utf8_v1.tsv (224, written by utf8_joint, whose UTF-8 states are
  Python's own incremental decoder's held-back bytes, grouped by their whole future, since the
  decoder holds back ED A0 and refuses it a byte later); asserted against every file of up to 2
  bytes for every kind set. Tests: those vectors, every file of up to 2 bytes judged against the
  count for every kind set, controls and two page patterns, every kind = utf8-valid's own count,
  and estimate = exact (to 1e-9 in log10) at 20, 300 and 3000 bytes.
- Each filter row shows Filtered: X%, its share alone with its settings as they stand (and what
  ticking it ticks too): Menu::stack_job is the line tally's key and work, now shared; filter_share
  runs one per filter on the workers and keeps it (shares_). StackInfo::kept_log10 carries the share
  as a number.
- X (and Z, C): of two filters that clash, the one that removes more alone is kept (then arithmetic
  last, then newer versions). The shares are counted on the workers first (seconds: the books'
  pages with the grammars); toggle_all_filters starts them and poll_toggle (each frame) ticks when
  they are in, the footer saying so meanwhile; a scripted key waits for it. Unticking is at once.
  Result at defaults: row-runs-v1 on image and video; the text automata (function-words first).
- Retired, as replaced by an automaton (FilterSpec::replaced_by, in the list and `sieve filters`):
  clean-v1 by clean-data-v1, clean-v2 by the new clean-data-v2 (a table: clean-data-v1 and a
  padding state), key-v1 and key-data-v1 by key-data-v2. Plugins are retired by name and SHA-256
  (plugin_spec). Checked: equal counts and sampled survivors both ways; clean-data-v2 also
  exhaustively to 4 symbols, against the oracle's engine at 5 lengths (added to CI's plugin loop),
  and key-data-v2 on all 72 keys. What is left, and what blocks each: FILTERS-CONFLICTS.md, "Merging
  and retiring: what is left". The optimise-all and COST note: IDEAS.md §12.
Variable length addressing (Edward, 2 October 2026: a COST that works out the shortest route to a
file from what we know, since a file could land exactly on a bearing). core corridor.hpp:
bearing_of (the compass's reading, as exact_degrees), unit_at_bearing (the navigator's landing,
ceil(A * units / (360 * 10^d)), round to 0 past the end), and shortest_path(v, units, max_decimals):
the position in hex without leading zeros, or the best of, for d = 0..20, the bearing 0, the
bearings just below and above the unit, and each width's (1, 2, 3 whole degrees) nearest and ends,
each followed by the shorter walk round the loop. Exact against brute force over every bearing and
walk on loops of 1 to 19683 units (44,010 units); past 4096 bits the far bearings are first
estimated from leading bits and skipped when they cannot win (40 long loops agree with the
exhaustive search). 8.5 MB takes about 2.4 s, so the COST row (hud.cpp draw_cost) asks
Hallway::shortest_path_of, a worker never waited for (it finishes the unit it has, then takes
the one now asked for), showing "working out..." meanwhile. Seen: a page near the start of the
pages line names in 38 chars of 77; the page at 90.1 degrees is "90.1"; hallway.exe saves one
leading zero.
not-written-v1 refused to count with the plugins (Edward, 2 October 2026: X left pages and books
"not countable": not-written-v1's tables over the budget). written_ranker sized P-and-O (the units
the plugins keep that a reading writes as a file) as P's states times (1 + O's states / 64): for
every text plugin at 32 characters, 236,034 x 41.5 states, 13 GB. Built, it has 1,559 states (most
readings die within a few symbols of a page; with words-data-v1 alone, 6,834). Now written_ranker
minimises P, checks P's table, builds P-and-O with intersect's new max_pairs cap (as many pairs as
could still fit, times 4 for the minimising), and checks the real sizes; WrittenRanker takes the
built automata. X's whole pages stack counts exactly at 32 (not-written takes 736 pages from it) and
compacts. Where P alone is over the budget the message says so ("the plugins' combined table"),
which is what stops the books' pages part (128 characters, 2.1 GB of table) with or without
not-written. Measured for what to do about that: the same count without the table (two rows of
counts, walked length by length) is exact in 1.7 s at 32, 12.8 s at 128 and 114 s at 512, in a few
tens of MB; it would give the tallies, the Filtered shares and X's weighing, not compact. Tests:
words-data-v1 (scowl-en-60) and not-written at 32 ranks, below words-data's own count, and survivors
drawn by number pass both and come back to their number; intersect's cap throws early.
The map's books bar, and what "the budget" was (Edward, 3 October 2026). The books theme is grey
with black edges, so on the black setup menu its frame (and any survivors bar, drawn in the edge
colour) vanished and the bar read as a plain grey block; a line whose edges are dark (luminance under
60) now has its two colours swapped on the map: a black body in a grey frame, survivors in grey.
With X's stack books still shows no survivors bar, because its pages part (128 characters) cannot be
counted. The "budget" in "over the budget at this length" was kPluginTableBudget, a fixed 512 MB on
one counting table, not the setup menu's memory budget: over_table_limit (plugin.hpp) now says what
the table would need and against what ("the plugins' combined table needs 2.0 GB of memory at this
length, over the 512 MB limit on a counting table"), in the stack, not-written, utf8-valid and
`sieve` messages. The filters window's footer wraps to two lines so the reason is not cut off.
The filter memory (Edward, 3 October 2026: a setting, not a hardcoded limit, at the foot of GLOBAL).
plugin.hpp: filter_memory() / set_filter_memory() (an atomic, 512 MB until set) replace every fixed
memory limit on counting: kPluginTableBudget (plugin tables, the plugins merged, not-written with
them, the designer's check), Utf8Counter's kMaxTableBits (now Utf8Counter::table_bytes against it),
not-written's walk memo (kJointBudget: now filter_memory / 179 bytes an entry, 3,000,000 at 512 MB)
and text_m1's kMaxRankLength (20,000 characters: now the words/window rankers' two rows of counts,
m1_table_bytes, against it; about 21,000 at 512 MB). memory_text and over_table_limit name the
setting in every refusal. Filter::table_bytes / FilterStack::table_bytes / BinarySieve::table_bytes
report what a count needs (or would), recorded where the stack picks its ranker, so nothing is built
to ask. Client: AppSettings::filter_memory_mb ([world], 64..1048576), applied at start-up
(--filter-memory MB too, and on the `sieve` tool); the setup menu's row 11 (kFilterMemoryRow; the
line rows moved down one, kFirstLineRow 12, and CI's row numbers with them), in 256 MB steps; the
tallies' cache keys carry it, so they count again when it changes; and a fourth budget bar, "filter
memory (largest count)", the most any line needs (stack_info's table_bytes; the books' largest part)
against it. Tests: each kind of table counts with the setting just above its need and judges just
below, naming it. Seen: X's pages stack at 128 characters needs 2.0 GB; with 3 GB set it counts
exactly (28 s) and books gains its survivors bar.
The filter memory, counted only on X (Edward, 3 October 2026: re-counting on every step of the
setting stuttered, and X is what asks for a count). The row now only saves the setting
(AppSettings::filter_memory_mb); sieve::filter_memory, the limit the tallies are counted with (and
their cache keys carry), changes only on X (toggle_all_filters with EveryLine: it applies the setting,
unticks everything in reach and ticks it all again with the clashes weighed) or on going into the
hallway (app_main sets it before building). memory_pending() is the two differing: a red line under
the four budget bars says "Memory Limit Change Detected : Press X to re-optimise all dimensions."; else,
while any line's tally is on a worker (counting_lines, from pending_) or X is weighing, it says
"Calculating Dimension... (PAGES, BOOKS)". The line is always kept, so nothing below it moves; the
budget's spacing is tighter so ENTER THE HALLWAY stays clear of the footer. X now works on the setup
screen too (not on the key's row, where it is a letter). The row and the filter-memory bar show the
setting, which is what X will count with.
The first batch of the code review (3 October 2026).
- **Counting pool:** the menu's counts go through a bounded pool (menu.cpp Workers: a queue,
  counting_slots() threads; a job whose settings went stale before it started is skipped).
- **Vault pictures:** checked on workers (hallway.cpp vault_queue / vault_collect: an item is held
  back as withheld, Book::vault_pending, until its verdict is in; withheld() decides it on the spot
  for anything that takes, saves or describes it; settle_vault() before a screenshot).
- **Item page:** hex_of returns a reference and caches a compact survivor's address too.
- **line_units():** returns a reference.
- **Plugin merge:** the plugins' automata are merged smallest first and kept (filter.cpp
  merge_plugins: by line and provenance, one merge per key at a time; not-written's ranker takes it
  as minimal).
- **not-written-v1's state limit:** written_max_states(base), from the filter memory (a refusal is
  retried when the memory grows).
- **canonical-mesh-v1 with every-vertex-used-v1:** ranks while its 2^V-row table fits the filter
  memory (16 vertices at 512 MB; ModelSieve::table_bytes feeds the budget bar).
- **symbol-entropy-v1's 2,048:** stays. It is a time limit (one rank is about L^3: 1 s at 2,048,
  66 s at 8,192), not a memory one.

Then the two shares became settings (Edward: "it just sticks to the ethos of the project").
- **COUNTING MEMORY** (row 12, AppSettings::counting_memory_pct, 5..100, set_counting_share):
  installed memory the counts may take at once, each up to the filter memory.
- **MERGE CACHE** (row 13, AppSettings::merge_cache_pct, 0..100, sieve::set_merge_cache_share):
  the filter memory kept for merged automata.
- **Effect:** both apply at once (they change no count). kFirstLineRow is 14, and CI's row numbers
  moved with it.
The second batch (4 October 2026).
- **Tables summed in place:** BigUint::add_mul_small (this += x * m) in every counting table's
  sums: DfaRanker, not-written's Bn, the kind counter, the word filters, two picture tables. The
  236,034-state table at 32 characters builds in 1.0 s, not 2.2 s.
- **COST's guided row:** kept on the Book (guided_code, guided_by).
- **Measured and left alone:**
  - E6 and E7: the menu's per-frame keys and sizes. A menu frame is 2-3 ms in Release, and taking
    the share lookups out changed nothing.
  - E10: not-written's per-step lock, which is never contended.
- **Tables shared** (filter.cpp shared_table):
  - A merged automaton's table, by merge key and length, is built once while any stack uses it. A
    second stack waits and uses the same one, through a weak_ptr.
  - It is kept between counts in the merge cache's share when it fits (the 308 MB pages table does
    not fit the default 256 MB).
  - WrittenRanker holds P's table as a shared_ptr and reads P from it (DfaRanker::dfa()).
  - Merges are keyed by the line alone, so stacks that ticked the same plugins in another order
    share too.
- **Two test bugs fixed:**
  - A comparison whose two sides changed the filter memory: MSVC evaluated the right side first.
  - A rule-cache test that assumed one run of the suite: it runs twice where the CPU has SHA
    instructions.
Packed tables (4 October 2026: "every last byte of saving will compound").
- **DfaRanker's table is packed:** per row, one block of limbs and where each state's number
  starts, not a BigUint each. The 236,034-state table at 32 characters holds 98 MB (it held
  358 MB) and builds in 0.4 s (1.0 s). table_bytes follows the packed size, still an overestimate:
  160 MB against 98 MB measured; the pages stack at 128 characters is estimated at 1.4 GB, not
  2.0 GB.
- **The automaton is held by shared_ptr:** a DfaRanker shares it with the merge cache and the
  compile cache (compile_plugin_shared hands out an aliasing pointer to the kept compile), so no
  stack copies a plugin's automaton any more.
- **DfaRanker(longer, length) serves a shorter length on a longer table**, with nothing built
  (rows 0..r answer every length up to r).
- **Merges are kept by the line's kind and symbols, not its length:** an automaton filter's
  automaton never depends on the length, as the compile cache already assumed. So the books' pages
  at 128 characters reuse the pages line's merge, and shared_table finds the shortest table of the
  same merge at the asked length or longer.
- **utf8-valid (Utf8Counter):**
  - below_[s][x][t] counts the bytes under x leading from s to t, so the table, rank and unrank
    sum over at most 9 states, not 256 bytes, and unrank finds the byte by halving.
  - At 4,000 bytes: build 1,694 ms to 74 ms, rank 225 ms to 3 ms, unrank 192 ms to 13 ms.
  - Its table_bytes counts numbers at half the longest, as they are on average (78 MB estimated,
    76 MB held; it said 138 MB), so it counts files about a third longer in the same memory.
- **X with every filter:** 10 s to 5.8 s, with the same counts (the screenshot after X differs only
  in the filter memory bar's 1.4 GB).
The remaining tables (4 October 2026).
- **PackedRows (sieve/packed.hpp):** rows of exact numbers packed, one block of limbs a row, built
  number by number into a reused BigUint. DfaRanker's table and not-written's Bn table use it.
- **Bn needed it:** written_ranker's budget counted Bn at DfaRanker::table_bytes, which became the
  packed size, while Bn still held a BigUint per number. That was an underestimate, about 150 MB
  against a fraction of that on a 3,000-character page.
- **The word filters' estimate (text_m1 m1_table_bytes):** counts at half the longest, as utf8's.
  At 512 MB they rank about 30,000 characters, not 21,000. Measured at 16,000 characters, it is
  still well above the real size: clean 87 MB held against 146 MB estimated; words' rows about
  48 MB against 291 MB, since words-only text has fewer units than 27 a letter. A growth rate
  measured on a short prefix would make it tighter.
- **Left alone, measured small:**
  - The picture tables (colours by pixels in scope).
  - The kind and pattern tables (a 16-byte head).
  - The models line's (vertices by faces).
  - not-written's joint memo: at most 41,000 entries in X's counts, about 7 MB, against its budget
    of 3,000,000.
The word filters' estimate made exact, and the time budget (4 October 2026).
- **text_m1's m1_table_bytes:**
  - One row, not two: each ranker keeps one, clean's c2, words' A or window's B.
  - Each count is shrink_to_fit, so it holds exactly its limbs, and the row is (L + 1) x 56 + g L (L + 1) / 16 bytes.
  - g is log2(27) first. Where that does not fit, m1_growth measures it on a 512-character table,
    once per filter, padding and dictionary: the growth from 256 to 512 characters, plus 2%.
  - Measured against mallinfo2: clean 256 MB held against 257 MB estimated at 30,000 characters,
    and words and window 153 MB against 156 MB. So at 512 MB clean ranks about 42,000 characters,
    words and window about 54,000 (it was 21,000).
- **The time budget:** sieve::unit_time_ms (plugin.hpp, 50 ms until set), the setup menu's TIME
  BUDGET (row 14, AppSettings::unit_time_ms, 5..60000; kFirstLineRow is 15), and --unit-time MS.
- **The menu side** (machine_budget):
  - The time bar and FIND MY LIMITS follow the setting at once (set_time_budget).
  - The growth (kGrowth 1.6 before) is measured at start-up from base-27 conversions at 10,000 and
    40,000 characters.
- **symbol-entropy-v1 on two symbols** (statistics.cpp binary_rank_limit):
  - Its 2,048 limit followed neither memory nor time. The rank is timed at 256 and 512 symbols
    (growth at least the cube); the length that fits the budget is then timed itself and brought
    in until it fits, and kept per budget.
  - Here: 960 symbols at 50 ms (the unrank there takes 50 ms), 1,498 at 200 ms, 2,588 at 1,000 ms.
  - Its walk's memory is held to the filter memory too.
- **Applied like the filter memory:** since what can rank follows it, X or going in applies it
  (time_pending, a red line in its own words), and the tallies' keys carry it.
- **symbol-entropy's limit is held to the filter memory first:** nothing is timed past that
  (a budget set huge once had the check time a rank at ten million symbols).
- **The fixed numbers that now follow the machine:**
  - **The item cache:** cached_units() is (kTilesKept + 2) x books_per_tile, the tiles kept and one
    more either side. It was 4096. kCacheBack and kCacheAhead moved to menu.hpp.
  - **Too large:** too_large_bits() is an address that alone fills the item cache's memory (a
    quarter of installed memory, at three eighths of a byte a bit). It was 8e9.
  - **Faces:** face_ms_per_frame() is a quarter of a frame at the display's refresh rate (4 ms at
    60 Hz before). The face workers are a core each but the one that draws, with no cap of 6.
  - **Large files:** large_file_bytes() is a file whose bytes take a quarter of the time budget,
    from the binary measurement. It was 65,536 in two places: vault_ahead and thin on taking a file.
- **Left as they are, for a decision:**
  - The world's graphics allowance (512 MB).
  - The rooms drawn and kept (6 back, 7 ahead) and rooms with faces (1).
  - The display width cap (1024 px): raised to the renderer's limit, a long page's display could
    be 16,384 px wide, 1 GB each.
The four left for decision (4 October 2026).
- **D. The item cache's share (GLOBAL ITEM MEMORY, row 15; kFirstLineRow is 16):**
  - item_memory_pct, 25 at first, 5 to 90 in steps of 5 (PgUp/PgDn 25), saved in [world];
    --item-memory PCT.
  - cache_bytes_here() is installed memory x the share, so the item cache, too_large_bits() and
    everything sized from it follow it. CI's line rows moved down one, with a check the row saves.
- **A. The world's graphics, measured (client/gpu_memory.hpp):**
  - Every texture is made and destroyed through gpu::create and gpu::destroy, which count their
    bytes as World or Pictures (item_faces.cpp's).
  - Menu::world_graphics_mb() replaces the 512 MB allowance: the renderer's three frames, plus the
    larger of what the world will make (Real Graphics' frame, the door portals at kPortalGrain,
    the seven signs, two picture-sized frames) and what it holds now.
  - About 37 MB at 1080p.
- **B. View distance (Settings > Graphics):**
  - VIEW DISTANCE (view_rooms, 2..64, 7 at first) is the rooms drawn and kept either side of yours, as many
    behind as ahead (it was 6 back and 7 ahead).
  - PICTURE DISTANCE (picture_rooms, 0..8, 1 at first) is the rooms with pictures either side
    of yours.
  - Both are saved in [graphics]; --view-rooms N, --picture-rooms N.
  - menu.hpp's set_view, view_rooms, tiles_kept and picture_rooms replace
    kCacheBack, kCacheAhead, kTilesKept and kFaceRooms.
  - The item cache, the line cache estimate, the render window and FIND MY LIMITS' display cache
    all follow them.
- **C. The widest display follows the display cache (display.hpp widest_display_px):**
  - It is the widest power of two at which every picture of the rooms with pictures fits the
    display cache, no wider than the renderer's widest texture (gpu::max_texture_px). It holds
    back only the letters' widening, never the display size setting.
  - kMaxDisplayPx (1024) is gone. The display size and close-up rows go up to the renderer's
    widest texture, and the display cache as far as the graphics memory has room (4 GB before).
  - FIND MY LIMITS sizes the cache for the widths the letters ask (display_mb(false)), and the
    display cache row says what they would take.
  - So a long page at a small cache is drawn narrower, with every item pictured, where before a
    few items had wide pictures and the rest stand-ins. Close-ups still draw the nearest wide.
- **Close-ups follow the screen and the memory (display.hpp closeup_width, closeup_count):**
  - The close-up size's default is **screen** (kCloseUpScreen, saved as "screen"): the screen's
    width rounded up to a power of two, 2048 at 1920 x 1080 and 4096 at 4K. It was 1024. A close-up
    is drawn at the width it shows, so pixels past the screen's are never seen.
  - The row goes off, screen, then 256 up to the renderer's widest texture.
  - How many are kept (kSharpMax, 24) is as many at that width as the graphics memory left beside
    the world (gpu_memory's count and the renderer's frames) and the display cache holds, at least
    one and at most a room's items. Hallway::set_graphics_memory gives the hallway the setting.
  - The menu counts them at the lesser of that and closeup_bytes_most (eight screens' worth), and
    keeps one close-up's room when it sizes the display cache.
  - A page reads only where it shows wide enough on screen for its letters (about 660 px for 4,000
    characters at 8 px letters); past that, it needs the item in hand, scrolled.
The item viewer (4 October 2026).
- **client/viewer.cpp:** Z, or a click on the thing in hand (over_hand_view: hand_view_rect_, where
  draw_in_hand drew it; the pointer, or the crosshair when the mouse is held), opens the thing over
  the whole window.
  - Pages: page_columns and page_rows lay it out as paint_text does, a character to a square cell
    in the pages display's shape less its margins.
  - Pictures and films: render_image's pixels, sampled to one streaming texture of the view's size
    whenever the view moves, so the work is the view's pixels whatever the picture's size or zoom.
  - Books: the open page (N and B turn it). Tracks: notes_to_notation, wrapped. Models: to_obj.
    Files: a hex dump worked out a row at a time from file_of's bytes.
- **Drawing:** text a row at a time, only the rows and columns in view. Below the letters on items
  size a cell is drawn as bars for its words, as on the item, at most a row to a pixel and runs
  closer than a pixel joined, so a page of any length costs about a screen's worth. render()
  draws only the viewer while it is open (the corridor is hidden behind it), 16.7 ms a frame at a
  30,000-character page on the software renderer, where the corridor and the item page took 36 ms.
- **Keys:** the wheel, Shift and the wheel, Ctrl and the wheel (zoom about the pointer), dragging,
  arrows or WASD, PgUp/PgDn, Home/End, + and - (Shift: twice), 0 fit, 1 one to one, N/B, Space,
  Esc or Z to close. A key's modifiers are read from the event, so --press can script them.
- **--press Click:** a left click in the middle of the window, where the crosshair is.
- **CI:** the hallway step opens a 4,000-character page by click (52 columns of 77 rows), zooms it
  with Z and Shift+=, and checks that Esc closes it with the page still in hand.
The viewer's views (4 October 2026).
- **ViewKind (hallway.hpp):** Raw (the thing itself), Picture (face_painter's picture, drawn again),
  Cover (bk.cover or a book's cover, at the image line's pixels) and Title (title_text, or a book's
  title page, laid out with page_rows). open_viewer lists the ones the item has; view_show(i)
  builds one. Buttons along the top (view_buttons_, hit-tested on a click) and Tab / Shift+Tab.
- **Picture:** view_picture_px is the larger of display_px's width for its letters and the screen's
  width rounded up to a power of two (closeup_width), held to what fits the display cache
  (widest_display_px for one picture). The painter runs on std::async (view_job_); the view
  says "drawing the picture..." until draw_viewer collects it. The viewer keeps one ARGB buffer
  (view_.px, frame after frame) for pixels, frames, covers and the picture alike.
- **Saving (item_save.cpp):** F or the button calls save_view. Raw goes to save_in_hand as before;
  view_file makes the other views' files (a PNG at one pixel a pixel through cli::encode_png, or
  the title's text) into save_blob_, which item_save_poll writes once the dialog answers. The name
  is the item's own with -picture.png, -cover.png or -title.txt. --save-view PATH does the same
  without the dialog, for CI.
- **display_text_here()** is split out of display_px_here, so the viewer sizes the picture as the
  shelf does.
- **CI:** the hallway step saves a page's picture and a book's cover as PNGs and a book's title as
  text, through --save-view.
Filters, section 1 of the list (4 October 2026).
- **1a. tidy-data-v2** (data/filters): tidy-data-v1 requiring the built-in max-run-v1 (max_run=3)
  in place of max-run-data-v1. max-run-data-v1 and tidy-data-v1 retire (plugin.cpp's retired list,
  by name and SHA-256): the same counts at every length tried.
- **1b. The token form's padding and within** (plugin.cpp, plugin.hpp, the oracle):
  - `padding trailing`: pad_trailing transforms the compiled automaton (each state also remembers
    whether the part before the last separator passes; a second separator after one goes to a
    padding state taking only separators). Still one automaton, so it merges.
  - `within N` (an expression over the parameters): WithinFilter and WithinRanker, the automaton's
    table at n = min(N, L) and every string of separators after, ranked prefix first. Counts on its
    own (spec counts_as "own"; plugin_dfa gives nothing for it, so it is never merged).
  - plugin_within, plugin_separators and make_plugin_filter are public; the CLI's --plugin report
    has a `within` line, and so has the oracle's (padding as two NFA states, within as a product).
  - words-data-v2, window-data-v2 and title-data-v1 equal words-v2, window-v2 and title-v1;
    title-v1 retires. tests/plugins/toy-padding-v1 and toy-padding-whole-v1 (two separators, cut
    and whole edges) are compared with the oracle in CI.
- **1c. Long units:** a token-form plugin of one dict set, no grammar, SPACE between tokens, on
  lower27 gets PluginFilter's fallback where its automaton's table does not fit: word_counting
  (text_m1.cpp), the built-ins' words and window rankers over the same dictionary. words-v1,
  window-v1, words-v2 and window-v2 retire for the -data twins (the same counts to 3,200; 20,000 in
  under a second). Compiled automata are kept by dictionary id, so a test with a dictionary of its
  own gives it an id of its own.
- **1d. Pictures:** palette-size-v1 and row-runs-v1 build an automaton where states x symbols stay
  under 2^24 (picture.cpp row_runs_dfa, palette_dfa), and make_dfa_filter it, so they merge with
  each other, not-packed-v1 and plugins; on rgb24 they keep their own rankers. FilterSpec gained
  counts_as_on(line) (judged at the line's largest settings), and filter_conflict and
  tick_filter_by_hand take the line where it is known (the setup menu passes it).
- **notes2 melody filters:** melody-lengths-v2 and melody-ending-v2 (`symbols notes*`), with
  LENGTH(i) and SIXTEENTHS(k) added to the v2 expressions in the engine and the oracle; the v1s
  retire.
- **Left in section 1:** optimise-all wired into COST (needs a source of anchors per line: a
  decision), ascii85 in not-written, the cross-line filters (pictures as text or files, MIDI as
  something else, pages as models and the reverse), transformed copies, models' third tier and
  an .obj filter, signed files checked past their signatures, titled lines filtered bottom-up.

### Other picture and video formats through ffmpeg (5 October 2026)
- **What:** `tools/cli/media_decode.*` (in sieve_lines).
  - `read_media_frames` hands every frame to a callback.
  - **stb_image first**, as before, for the PNG/JPG/GIF/BMP signatures and anything `image_info` reads, so those addresses never change.
  - **ffmpeg otherwise:** an ffmpeg program run as a separate program, never linked (since 7 October with no shell between: `posix_spawnp`, or `CreateProcessW` on Windows).
- **The ffmpeg call:** `-map 0:v:0 -frames:v N+1 -fps_mode passthrough -sws_flags +accurate_rnd+full_chroma_int+bitexact -flags +bitexact -pix_fmt rgba -c:v pam -f image2pipe -`, with stderr to a temporary file (reported as "ffmpeg reported: ..." when a file is damaged).
- **Streaming:** a reader thread hands frames over, at most two at a time, to `ImageCanoniser` (sieve/image.hpp). That's a frame-at-a-time `canonicalise_image`, which is now built on it.
- **Which ffmpeg:** `--ffmpeg PATH` (CLI, global), the hallway's `ffmpeg` setting, `SIEVE_FFMPEG`, beside the executable, then the `PATH`. The environment is read wide on Windows.
- **Where it's used:** `sieve warp --line image|video --file`, and J on the binary line for MP4, AVI, WEBP and unknown kinds (more than one frame goes to the video line).
- **Measured:**
  - an ffv1 MKV frame lands where the same frame as a PNG does, at rgb24;
  - the 52 MB 1080p anchor video, all 3,105 frames, took 57 s at 308 MB peak (ffmpeg alone takes 30 s here);
  - 5 frames of it take 0.3 s.
- **CI:** a new step. Linux installs ffmpeg if it's missing; other runners skip the ffmpeg half if they have none.
- **Next:** the cross-line filters (pictures as text, audio as text, a still as a video, models and pages), then the locator reading a file on every line.

### Sound itself on the audio line, and saving as other formats (5 October 2026)
- **The `pcm` set** (sieve/sound.hpp, core/src/sound.cpp; SPECIFICATIONS §3.3):
  - id `pcm/RATE/BITS/Cn`;
  - each digit a two's-complement sample;
  - units channel by channel.
- **`canon-pcm-v1`:** `PcmCanoniser`, a block at a time.
  - **Rate:** spans in units of 1/(R·S) seconds reduced by their gcd, sums in int64, so rates up to 2^31 can't overflow.
  - **Rounding:** `round_half_up(a, b) = floor((a + floor(b/2)) / b)`.
- **WAV:** `read_wav`, `wav_format` and `wav_sample` (also used on ffmpeg's piped WAV), and `pcm_to_wav`.
- **Oracle:** `canon_pcm` in exact Fractions, `pcm-vectors` → `tests/vectors_pcm_v1.tsv` (13 cases). The engine matched on the first run. CI diffs them.
- **CLI:**
  - `--note-set pcm --rate --bits --channels`, with `--length` = samples per channel (default: one second);
  - warp `--file` reads WAV itself, else ffmpeg (`read_media_audio`: `-map 0:a:0? -c:a pcm_s32le -f wav`, parsed as it streams);
  - `preview` draws a row of shades per channel;
  - `audio_file()` gives WAV or MIDI, for both the vault and `unit_file`.
- **Hallway:**
  - **Settings:** `samples`, `pcm-rate`, `pcm-bits`, `pcm-channels`. When on pcm, the six audio rows are samples, AUDIO SET (notes104 → notes2 → pcm), sample rate (standard rates; the ini takes any), bits, (notes2) and channels, so no row number moves.
  - **Playback:** P plays through `MusicPlayer::play_sound` (mixed to mono, linear interpolation, the music fading under it) or `Synth::play_sound`.
  - **Viewer:** a SOUND tab.
  - **Warp:** T takes a sound file path.
  - **F and J:** F saves `.wav`. J opens WAV, MP3, OGG and FLAC when the audio line is pcm and MID when it holds notes, and says which set is needed otherwise.
- **Export:** `cli::export_formats` and `export_unit` (lines.hpp).
  - **What's offered:** the line's own format, then the candidates whose encoder `ffmpeg -encoders` lists (`ffmpeg_has_encoder`, asked once per ffmpeg).
  - **How:** `ffmpeg_convert` quotes every argument. Pictures and video go through a PAM sequence (`-f pam_pipe`) and nearest-neighbour scaling; a GIF gets palettegen/paletteuse without dithering; MP4 and WebM get even sizes and yuv420p.
  - **Choosing:** `save_unit` goes by the extension, where an unknown one keeps the old behaviour. The hallway's save dialog lists the formats as filters, and the chosen filter is used when no extension is typed.
- **Not yet:**
  - filters of the pcm set's own, and filters between it and the note sets;
  - open-ended notes between notes2 and pcm;
  - an MP3 without an ID3 tag reads as an unknown kind, so J tries it as a picture;
  - a drawn waveform in place of the text one;
  - a hallway setting for `--fps`.

### Sound: filters, a drawn waveform, video-fps, tagless MP3 (5 October 2026)
- **Channels are judged one at a time** (filter.cpp):
  - `line_voices(FilterLine)` gives a note set's voices, a pcm set's channels, else 1;
  - FilterStack and `sieve filters --plugin` use it;
  - `VoicesRanker` packs its state into bits sized to the line (voice, place, inner) rather than fixed 3/16/45, and raises one voice's count to the voices after by squaring.
  - **Unchanged:** ranks and units are unchanged, since states are internal.
- **core/src/filters/sound.cpp:**
  - `sound-peak-v1`: a one-state automaton to 24 bits; past that `PeakRanker`, a mixed-radix count over the allowed digits in digit order.
  - `sound-step-v1`: an automaton over the last sample when (base+1)·base ≤ 2^24, i.e. up to 11 bits; else judged.
  - `silence-run-v1`: an automaton over the run so far while it fits; else judged.
  - **Oracle:** `sound-vectors`, with a closed form for peak and tables for step and silence, each checked by brute force on small lines, in `tests/vectors_sound_v1.tsv` (69 rows). The engine matched on the first run.
  - **Measured:** at 800 samples, 8-bit mono, the three merged (silence-run at 800) count exactly in about 6 s (10^-102.20). At 8000 the merged table needs 30.8 GB, so the stack judges only.
  - **Judged by hand:** white noise fails step; a loud tone passes all three; a 0.25 s tone padded to 1 s fails silence-run.
- **Waveform:**
  - `sieve::pcm_envelope` gives each column's lowest and highest sample;
  - `Hallway::draw_waveform` draws the item page's bands with lines, each column reaching back to the middle of the one before, so samples join;
  - `waveform_argb` is the viewer's picture: a column a sample up to `view_picture_px`, each band an eighth as tall as wide;
  - the CLI preview uses the envelope too, and the one-line preview puts the channels side by side.
- **video-fps:** an ini key, `Hallway::set_export_fps`, `--video-fps`; F's video saves use it.
- **J:**
  - `cli::looks_like_mpeg_audio` (MPEG audio frame or ADTS header) sends a file of unknown kind to sound first, without touching file-kinds-v1;
  - an MP4, AVI, WebP or unknown file that ffmpeg finds no picture in (an .m4a) is tried as sound when the audio line holds pcm.

### The filter survey, and open-ended notes (6 October 2026)
- **tools/survey/survey.py** drives `sieve filters` for every combination in `grid.tsv` (lists, ranges `lo..hi`, `:step`, `*k`, `key==other`; stacks `each`, `all`, `a-v1+b-v1`, `a-v1[p=1,2]`).
  - **Bounds:** a hardware profile (`profiles.tsv`: ram_gb, cores, filter_memory_mb, time_limit_s, each overridable) bounds each count. Jobs = min(cores, 3/4 of the RAM / filter memory).
  - **Exactness:** exact percentages come from the printed survivor and excluded counts, as Python fractions, with as many decimals as it takes to get past the leading 9s or 0s. Python 3.11's limit on reading long numbers is lifted.
  - **Measurement:** peak memory by `wait4` (Linux and macOS). A row's own failure is recorded as its error.
  - **First run** (average profile, this machine: 4 cores, 15 GB): 471 counts, 402 exact and 69 judged only, 138 s of counting (`results/survey-average-2026-10-05.*`).
- **notes3** (sieve/notes3.hpp, core/src/notes3.cpp; SPECIFICATIONS §3.4):
  - `Notes3Set` and its id; `canonicalise_notes3`, notation, `notes3_to_midi`, `midi_to_notes3`;
  - the oracle's own `canon_notes3`/`notes3_midi` (Fractions), with `notes3-vectors` → `tests/vectors_notes3_v1.tsv` (8 cases). The engine matched on the first run, and a MIDI round trip is tested;
  - **CLI:** `--note-set notes3 --low --high --tpq --longest --levels --voices --tempo --instruments`, and warp reads a MIDI file itself;
  - **Filters:** `line_voices` takes notes3's voices;
  - **Hallway:** `render_notes3` (synth.cpp) makes square tones at the levels and tempo, played through `play_samples`; J opens MIDI on a notes3 line through warp; the setup menu's six audio rows become notes, set (notes104 → notes2 → notes3 → pcm), lowest, highest, lengths (presets of TPQ and longest) and voices; levels, tempo and instruments live in the settings file only.
  - **Not yet:** the melody plugins for notes3's digits (with levels), and drawing it as notes on a staff.

### The dimensions as data (6 October 2026)

The first step of IDEAS §13: every dimension defined once, so that adding one, or reordering the
doors, is an entry in a table rather than a hunt through the client.
- **`client/dimensions.hpp`:** `kDimensions`, in door order. Each entry is the dimension's
  identity (`Media`, moved here from `world.hpp`), its id ("pages"), its `--line` name ("text"),
  the unit line it is, if any, its two colours (moved here from `theme.hpp`) and its default
  WORLD music mode. `kLines`, `kBooksLine`, `kModelsLine`, `kBinaryLine`, `theme_of()`,
  `line_of()`, `line_named()` and `menu_ink()` all come from it. Binary must be last: the
  corridor starts and ends at it, and a static_assert says so.
- **Two numbers, kept apart:** a dimension's door (`li`, where it stands) and its identity
  (`Media`, what it is).
  - **By door:** walking, the map's columns, the HUD's rings, and the key that moves a fifth at
    each door (`fifths_at()`, generated, so it extends to any number of doors).
  - **By identity:** everything configured or saved per dimension. The music settings' `modes`
    list is saved in `Media` order, exactly as before, so existing settings files read the same.
    The filter settings were by kind already.
- **The hallway's unit lines are looked up by kind** (`unit_line(LineKind)`, `line_at(li)`), no
  longer as `lines_[0..3]`, which tied a door's number to the line behind it.
- **The setup menu** takes its start lines, focus list, map columns, section headings and filter
  windows from the table. `menu_ink()` replaces the books-only special case; its output is the
  same for every line.
- **One visible change:** the main menu's rule under the title was five hardcoded colours (pages
  to books; models and binary were never added). It is now a band per door, in each line's
  colour as the setup menu writes its name.
- **Checked:** a copy with the doors reordered as decided (image first, then pages, books, audio,
  video, models, binary) builds with nothing else changed, and its corridor and menus come up in
  that order.

### The setup menu on one screen (6 October 2026)

Every setting stays on screen at once, beside the map, so turning one shows at once what it does to
the bars (IDEAS §13). Laid out for 1920 x 1080, which now holds every row with room for tracks and
movies; a smaller window shows it whole, scaled down, as before.
- **The budget's four bars and status line** moved from under the settings to the top right
  (`draw_budget(x, y)`, `kBudgetW` wide). The map starts below them.
- **"the largest that fit the budget"**, the value beside FIND MY LIMITS, is gone.
- **The values start just past the widest label**, worked out from the rows rather than fixed at
  320 px, so every value fits whole; one too long for its column stops at the map's edge with "..".
  The map's edge is one constant, `kMapX`.
- **The menu's smallest size** is worked out from what it draws: as wide as the subtitle and the
  budget beside it (at least 1240), and as tall as the rows and a heading for GLOBAL and each line.

### Tracks and movies, and the doors in their new order (6 October 2026)

IDEAS §13, steps 2 to 4.
- **Core: `composition-v1`** (`core/include/sieve/composition.hpp`, `core/src/composition.cpp`;
  SPECIFICATIONS §11).
  - `CompositionSpace`: a cover, a title (or none) and N units of a base line, as one mixed-radix
    number, scrambled with shuffle-sha256-v1. The id names the kind ("tracks", "movies").
  - `CompositionSieve`: a stack each for the cover, the title and the units, each unit judged on
    its own, so the count is exact (`Nc * Nt * Nu^N`) and compact works in both orderings
    (`composition-compact-v1`).
  - `join_units` / `split_units`: a track's units as one unit of audio, strand by strand (voices
    or channels), as items are shown, played, saved and warped in.
  - **Checked:** `reference/sieve_ref.py composition-vectors` (an independent implementation) gives
    `tests/vectors_compositions_v1.tsv`, 48 vectors, which CI diffs and the unit tests read; the
    sieve is checked against brute force, with and without filters on each part.
- **Client:**
  - `dimensions.hpp` gains `Media::Tracks` and `Media::Movies` (at the end of the enum, so saved
    music settings still read), `composes` (the base line) and `sizes_vary`; the table is in the
    decided order. (Real Graphics' models follow each line's id: see the next entry.)
  - The hallway holds a `Composition` per door (space, stacks, sieve, joined line). `line_at()` of
    a composition is its **joined line**, the base line N units long, so the item panel, P, the
    viewer and F work as on audio and video. Items, warps (T), addresses (X), M and the status
    line have composition branches.
  - `sieve-filters.ini` gains `[tracks]` and `[movies]` with `.cover`, `.title` and `.units`.
  - The setup menu: units per track and per movie; the rows follow the doors (`setup_rows_of`,
    `setup_row_of` in `menu.hpp`, so the row constants are computed); the filters window treats
    books, tracks and movies alike as lines with parts (`has_parts`, `part_line`,
    `parts_stack_info`); FIND MY LIMITS grows audio and video only while a one-unit track or movie
    still fits, then the units. `--track-units`, `--movie-units`.
  - `menu_ink()` lightens a dark background when the edges are dark too, so TRACKS and MOVIES read
    on black.
- **Found and fixed on the way:** door numbers written as literals that only worked in the old
  order (`find_limits`, the books' count's `resolve(4, ...)`), now named.
- **CI:** the checks that walk through doors or count the menu's rows follow the new order, and new
  checks cover tracks and movies (both orderings, a warp, saving, compact).

### Real Graphics for tracks and movies, and binary at both ends of the title rule (6 October 2026)

- **Models of their own:** `data/meshes/book-tracks.obj` and `book-movies.obj` (with their `.mtl`),
  begun as copies of `book-audio` and `book-video`, so each can be modelled from there. `faces.ini`
  has `[tracks]` and `[movies]` (copies of audio's and video's), and its sections are in door order.
  The table's separate `models` folder field is gone: every line loads `<model>-<id>.obj`, then the
  template.
- **The main menu's rule** is a band per line as the corridor runs, binary at both ends: ten bands.
- **Docs:** `data/meshes/templates/README.md` lists every line's files in door order, binary's room
  model, and which item models began as copies; README's file map lists the nine lines.
- **CI:** tracks and movies each load their own item model under Real Graphics.

### A review of the session's work (6 October 2026)

Everything since `2262e74` read again: the core (composition, notes3, sound and its filters,
image, filter), the CLI (ffmpeg, lines, filter settings) and the client, and the docs against the
code. Fixed:
- **A typed length could fill memory:** a notes3 event of `C4:999999999` on a set with a short
  longest length split into up to a billion rests before anything checked; more than 2^24 pieces
  is now refused, with the event and the lengths named.
- **A failed save could pass for a good one:** `ffmpeg_convert()` judged success by the output
  existing, so saving over a file ffmpeg then failed to write reported success with the old file
  still there. The output is removed first.
- **Exceptions while drawing:** `is_pcm_symbols()`, `is_notes3_symbols()` and `is_note_symbols()`
  parsed the id and caught the failure, and some are asked every frame (`on_sound()`, the item
  panel); a cheap prefix test answers every other id first.
- **One power of a big number:** `BigUint::pow(const BigUint&, n)`, by squaring, replaces four
  hand-written copies (composition, the voices ranker, sound-peak, the menu); the menu's three
  copies of a power modulo the tile are one `tile_pow()`.
- **One temporary file:** `TempFile` (`cli/media_decode.hpp`) replaces the two copies in
  `media_decode.cpp` and `lines.cpp`.
- `CompositionSpace` refuses no units before building its shuffle; notes3 works out the default
  level once a warp, not once an event.
- **Docs:** this file's line overview (nine lines, in door order); SPECIFICATIONS §3 (the nine
  lines) and §5.4 (every line's colours, in door order); README (models "a sixth line"); IDEAS
  (nine lines; §7.12 built as movies); `hallway --help` lists every line for `--line`.

Left, as notes:
- `silence-run-v1` tells the menu "judged" whenever the whole unit's run would not fit an
  automaton, though it counts exactly whenever the chosen run does (4000 samples on an 8000-sample
  line). `counts_as_on` cannot see the filter's settings; it needs them passed to be exact.
- On Windows, ffmpeg runs through `cmd.exe` (`_wpopen`), which expands `%NAME%` in a path when a
  variable of that name exists; a path with two `%` signs could be changed. `CreateProcessW` with
  pipes would avoid the shell altogether.
- `read_media_audio()` on a WAV path reads the whole file before decoding it.
- Composition ranking and the sound-peak ranker divide a long number once a unit or sample, so
  their cost grows with the square of the length; fine at the sizes the menu allows, and
  divide-and-conquer (as `to_digits` does) if they are ever pushed.

### Joined filters, tracks and movies in the `sieve` tool, notes3's melody filters (6 October 2026)

IDEAS §13's open steps, and one fix:
- **A held model no longer redraws the crates.** `face_painter()` drew each crate's picture at the
  held model's turn (`model_spin_`, `model_tilt_`), so a face painted while one was turned came out
  at that angle. Faces are now drawn at the resting turn, `Hallway::kModelSpin` / `kModelTilt`,
  which R also returns to.
- **The joined stack** (`CompositionSieve`, SPECIFICATIONS §11): a fourth stack for tracks and
  movies, judging the units joined (`join_units`, strand by strand) as one unit of their line N
  long, so the line's own filters judge across the seams, as a book's pages are read as one text.
  It counts and ranks when the per-unit stack is empty: the joined unit is then the part (`Nc *
  Nt * Nj`), and survivors follow its order. Both at once judge but do not count. The compact
  domain gains `/<joined id>` only when it has filters, so no address made before changes.
  `[tracks.joined]` / `[movies.joined]` in the settings; `joined_filter_line()` is the line it
  sees; a JOINED part in the setup menu's filters window, counted in the line's survivors.
- **`sieve info|warp|read|filters --line tracks|movies`** (`tools/sieve_cli.cpp`, "tracks and
  movies"): built from the hallway's option names (`--track-units`, `--movie-units`,
  `--title-length`, `--image-width/-height/-palette`), so they give the hallway's addresses. warp
  gives a blank cover and title, as T does; read prints title, cover and the units joined (`--out`,
  `--cover-out`); `--compact` through the composition sieve. `strands_of()` and `joined_line()`
  moved from the hallway into `cli/lines` for both to use.
- **The notes3 family of plugins** (`symbols notes3*`, FILTER-PLUGINS §14): constants `PITCHES`,
  `LOW`, `LEVELS`, `LONGEST`, `TPQ`, and `NOTE(p, k, n)` / `REST(n)` to name symbols, in the engine
  (`plugin.cpp`) and the oracle. Ten plugins in `data/filters`: `key-notes3-v1` and
  `melody-{leap,range,ambitus,rests,gapfill,metre,ending,lengths,loudness}-notes3-v1`. A pitch's
  notes are one run of symbols, so the pitch-only ones name a pitch as a range, which builds the
  default set's ambitus in 2.4 s rather than 16.
- **Checked:** unit tests (the joined stack against brute force with one and two strands, both
  stacks at once, the unchanged domain); CI: the composition addresses pinned in the tool, the tool
  and the hallway saving the same track (plain and compact through the joined stack), joined counts
  on movies, notes3 stacks compact both ways, and the oracle against the engine on every notes3
  plugin.

### The music's melodies, and tracks as a source (6 October 2026)

- **"Melody", not "track", for what the music plays**, now that TRACKS is a line: the Media Player's
  labels (RECENT MELODIES, "A new melody now", "Quiet between melodies" ...), the README, and the
  code (`MusicMelody`, `go_to_melody()`, `Request::GoToMelody`). `sieve-music.ini` writes recent
  entries as `melody =` and still reads `track =`.
- **Draws from: the audio line or the tracks line** (per mode; `source` and `units` in
  `sieve-music.ini`). From the tracks line a melody is a real track: N units of the mode's length,
  blank cover and title, its notes the units joined; the mode's filters judge the whole joined
  track, as a JOINED stack does. `MusicMelody::units` (0 for the audio line) is kept in the recent
  list (`tracks:N`) and in favourites.tsv (a sixth field). Its address is its tracks-line address,
  from the hallway's cover and title shapes (`set_track_shape()`, called when a hallway is built;
  the setup menu's defaults before that). G walks to it on TRACKS, building the hallway again with
  that length and number of units when it differs.
- **A door literal fixed:** "go to" set the line to door 2, which was audio before the doors were
  reordered and is books now; it goes by name (`line_of(LineKind::Audio)`, or tracks).
- **The model in hand** turns back to its resting angle whenever it is put down, so it comes back
  upright; the one path that put an unshelved item in hand without putting the last one down
  (compact, no shelf) now does.
- **The NOW PLAYING box** is 21 px lower and 6 px further right (`y = 73`, right edge at `W - 6`):
  below the FPS counter, flush with its right edge, and clear of the setup menu's budget rows.

### The review's four notes, fixed (7 October 2026)

The four left as notes at the end of the review (above):
- **silence-run "judged" in the menu.** `FilterSpec::counts_as_on` now takes the filter's settings
  (`const FilterValues*`, null for "at the largest the line allows"), and `filter_conflict()` passes
  each filter's (`LineFilters::values_of()`), in the menu, X and `tick_filter_by_hand`. silence-run
  is an automaton wherever its chosen run is small enough, whatever the line's length.
- **No shell for ffmpeg.** `Pipe` (`cli/media_decode.cpp`) runs ffmpeg with its arguments:
  `posix_spawnp`, or `CreateProcessW` with pipes on Windows (stdin NUL, stderr to the temporary
  file, only those handles inherited), so `cmd.exe` never sees a path: no `%NAME%` expansion, and
  a path with a double quote works (it was refused). Checked: Linux, a round trip through MP4 with
  `%PATH%`, quotes, an apostrophe, brackets and a semicolon in the name; Windows compiled with
  MinGW here, not run.
- **WAV files read a block at a time** (`own_wav_file`): what `read_wav()` does, chunk by chunk, the
  same blocks handed on; checked byte for byte against the committed tool.
- **Rankers no longer grow with the square of the length:**
  - **The generic `Ranker::rank` / `unrank`** (every automaton's) took one big addition or
    subtraction for every smaller symbol at every place: on a 16-bit sound (65,536 symbols) a
    second's compact address took hours. Symbols are now taken as runs that lead to the same
    state: rank counts them as plain numbers and makes one product a state; unrank finds its run
    and divides. The same numbers: 8 seconds of 16-bit stereo, compact under sound-peak, in 12 s
    (the committed tool had not finished in 9 minutes), round trips exact.
  - **sound-peak's own ranker** (past what an automaton holds) is one number in base |set|:
    `from_digits` / `to_digits`, which divide and conquer.
  - **Composition ranks** join and split the units' ranks by halves.
  - Checked: unit tests (a five-unit composition against one unit at a time), the committed tool's
    output on tracks and WAV files, and every CI step.

### Records for tracks and movies (7 October 2026)

IDEAS §13's last open step.
- **The record** (`tools/cli/book.*`): `sieve-book-v1`'s sections, `cover`, `title` and `units`;
  `record_composition()` reads one as a composition's parts (missing parts blank), `record_line()`
  says which line a record belongs on from its sections, `composition_record()` writes one (blank
  cover and title left out, so one item has one id) and `line_shape()` gives any line's options
  back as a section's shape.
- **`sieve-book-v2`:** one field more, `notes <symbols id>`, after an audio section's line, for any
  set but `notes104` (its `length` is then per voice or channel); written only when needed, so a
  record without it is still v1. That also answers IDEAS §12's "books of notes2 melodies".
- **The tool:** `bind --line tracks|movies --file ... [--title] [--cover] --out`, `read --line
  tracks|movies ... --record FILE`, `unbind --units FILE` (the units joined). bind and read give
  byte-identical records of the same item.
- **The hallway:** F offers "Sieve record" (`.track`, `.movie`) on a track or movie
  (`composition_record_of()`); J on a record routes by `record_line()` and opens it with
  `go_to_composition()` (go_to_unit now calls it on a composition line), or says which shape it
  needs (`msg.record_shape`).
- **Checked:** CI binds a two-voice notes2 track (v2), reads the same record back by its address,
  unbinds it to MIDI; and in the hallway a track and a movie saved as records, opened with J and
  saved again, are byte for byte the same.

### COST's balance (7 October 2026)

Edward's design (the bar) and the first part of 1e.
- **What it shows:** the cheapest address found for the item in hand against the item as a file,
  as a share of the file: `Neutral : 0%`, `Positive : X%` (longer than the file) or `Negative : X%`
  (shorter). The cheapest of: the address it is held by (`bits` positional or scrambled, the
  compact address `log2(line_units())` for a survivor, `bk.bits` guided), its guided code, and its
  variable length route. So it moves when the route arrives from its worker.
- **The bar** (`hud.cpp draw_balance`): grows outward from the centre line, red for Positive,
  green for Negative, a white border, full width at 100% and held there.
- **The file** (`Book::file_bytes`, kept with the item like `guided_code`): `item_file()`'s size (what
  F saves and J opens), or `file_size` on the binary line; 0 if it cannot be made (no balance then).
- **New rows:** "as a file", and "compact" for a survivor of a compact shelf. Notes are fitted to
  the panel (the route's note ran off it).
- **Strings:** `cost.compact*`, `cost.file*`, `cost.balance.*`.
- **Checked:** Linux build, screenshots of each case: text (Negative, by guided), image and
  binary near a bearing (Negative, by the route), a random binary address (Positive 57.8%, by the
  route, the title making the address longer than the file), a compact image shelf.
- **Still open in 1e:** searching the filters and their values for the cheapest address that keeps
  the item in hand (the anchor), and showing that beside what the filters as they are give.

### Tailoring the filters to the item in hand (7 October 2026)

1e's search, for one anchor: the item in hand.
- **The search** (`tools/cli/tailor.*`, in sieve_lines): `tailor_filters(line, unit, current,
  progress)`.
  - Each filter that can count (not retired; not one that judges only everywhere) is judged
    alone, with its prerequisites (`tick_filter`), first at `current`'s settings, then each
    setting in turn: an integer over a grid of 7 across its range, then between the best value's
    neighbours, until they are adjacent; a text over its choices, or every registered dictionary
    (`load_registry`). A value is kept where the item passes and the count is lowest.
  - The set: from each filter kept (strongest first), a stack grown greedily, a filter added where
    `filter_conflict` allows, the stack still counts and it removes more; a filter already in a
    grown stack seeds none. The best stack wins. Those that clash with everything (the "own"
    counters, title-data and window-data) make stacks of one, and the automata, which merge,
    make another.
  - Progress and cancel through `TailorProgress`.
- **`sieve tailor`**: prints each filter, its settings and bits alone, used or why not, the address
  and the tailored compact address with the survivor number; `--out` writes the settings file.
  `--unit N` picks the unit of the input. Help entry added.
- **The hallway** (`client/tailoring.cpp`): K (any tab: it shows COST) runs the search on
  `tailor_thread_` for the item in hand (pages, image, audio, video; not guided), K again stops it.
  COST's section (`draw_tailor`) shows progress, then the compact address found against the held
  one and the filters with their settings. Return sets `Request::Tailored`: the application saves
  the new `FilterConfig` (the hallway now keeps a copy, `filters_`) to the settings file, builds a
  hallway on the same line, and `hold_on_cost(unit)` puts the item in hand on COST.
  `--tailor PATH` (screenshots): K, waited for, and what Return would apply saved to PATH.
- **Measured:** a melody of 8 notes, 53.6 bits to 16.3 in 2 s (key, lengths, range, ending,
  rests); a 32-character page, 152.2 bits to 67.3 in 30 s (words-data-v2 with scowl-en-35,
  word-cost, letter-triples, max-run).
- **Checked:** CI tailors the melody, checks key-data-v2's scale and the survivor number, warps it
  compact under the file written (`012f3`) and reads it back. In the hallway: the section while
  searching and when done, and the file it saved, opened, with the item compact at 16 bits. The
  Return path in app_main was read, not driven: there is no way to send keys to a live window
  here.

### Full mode, and the rotation transform set aside (7 October 2026)

- **Why:** Return (COST's tailoring) saves compact mode, and compact on a titled line names the
  content alone, so the titles and covers went, and stayed after a restart, as the settings file
  said compact. Edward: "compact should be compact"; some files need titles or file names, some do
  not. So a mode of its own.
- **`FilterMode::Full`** (`filter_config.*`, `full` in the settings file; the menu cycles off,
  mark, hide, compact, full, excluded). The hallway stores it as Compact in `modes_` (so every
  compact rule applies) and keeps `full_[i]`, a `TitledSpace` over the survivors: title and cover
  spaces from `titled_[i]`, content size the survivor count, shape `<symbols>/L<n>/survivors=<stack
  id>`. `full_here(i)`; `titled_of` returns it in compact; `units_of`, the book construction (content
  = survivor number, unit by `CompactLine::unit_at(..., Positional)`), `index_of` (blank title and
  cover) and G's parse use it. Books, tracks and movies: full is compact (their titles are kept
  there); models and binary: compact for now. Guided: the content alone, as in compact.
- **COST:** the row reads "full" (cover, title and survivor number); the readout and the item page
  say "full address". Tailoring keeps a full line full on Return, and its row adds the title and
  cover bits ("full, tailored").
- **Docs:** SPECIFICATIONS §9 (the mode table) and §11 (titled lines in full mode, its id);
  README's mode list (full, and excluded, which it had left out); IDEAS §15, a new section of
  deprecated ideas, holds the rotation transform (was §3.7).
- **Checked:** the audio line full with the tailored melody filters (survivor 4851 with its title
  and cover, a 68-digit address, the shelves titled) and the image line full under
  palette-size-v1.

### The cleanest digits, and files weighed (7 October 2026)

- **The search's target** (`cli/tailor.*`): each anchor's shortest route among the survivors
  (`route_bits`: `corridor.hpp shortest_path`, 20 places; past 2^16 bits, hex digits), not the
  count. Step 1 keeps each filter's best setting by route and its best by count; step 2 grows
  stacks from every kept setting both ways (accept where the count falls; accept where the routes
  shorten), each in its own order, and keeps the best score. Several anchors (`std::vector` of
  units; `--unit all`); `TailorOptions::count_description` adds `description_bits` (mask: one bit a
  countable filter offered; each setting at log2 of its values; dictionaries over the registry;
  free text 8 bits a character). `TailorResult` gains `route_bits`, `description_bits`,
  `count_bits`; `line_bits` is now the anchors' routes unfiltered.
- **`item_of(line, file)`** (`filter_config.*`): the unit whose file is exactly `file`, or nothing;
  binary_items' judges use it (one test of "exactly an item").
- **Weighing** (`cli/weigh.*`, in sieve_lines): `weigh_files(root, files, WeighLines, tailor)`.
  Ways per file in groups (base; "binary, compact"; "<line>"; "<line>, compact"; "<line>,
  tailored"), each group's stack cost and each line's shape (symbols/L<n>, 8 bits a character)
  paid once; groups switched off greedily while the total falls. `weighing_table()`.
- **`sieve locate --weigh [--tailored [--out-filters PATH]] [--page-length N]`**: the lines from the
  line options (`--page-length` for pages, since `--length` is audio's), filters from `--filters`.
- **The File Locator** weighs every analysis with the hallway's unit lines and `filters_` (binary
  length its BINARY setting), shows the table (12 rows), a **Tailor the filters to these files**
  button (re-analyses with tailoring), then **Use the tailored filters** (`Request::Tailored`
  without a unit; `tailored_unit()` is now optional in app_main). `--locate PATH --tailored` for
  screenshots. `handle()` ends the frame loop when a request is Tailored.
- **COST's K section:** "tailored, shortest route", and the stack's description in bits.
- **Checked:** CI (filters step) tailors the melody (survivor 46327 of 204994, route b4f7, compact
  0b4f7, read back) and weighs a folder of two melodies, a picture and a note (530 bits against
  2488); the whole step run here. In the hallway: the locator plain and tailored (screenshots).

### Installers packed: `sieve-manifest-v4` (7 October 2026)

Edward chose compression built into the installer (the release's files are no Sieve items, so
the weighing's ways alone would not have changed its size).
- **The format** (`locate.hpp`, SPECIFICATIONS §12.2): v3's listing, two `stream` lines (x86,
  lzma2: packed size, size, dictionary), each file's way (`x86`, `raw`, `lines K`), and after `end`
  the two raw LZMA2 streams. `Manifest` gains `packed`, `x86`, `lzma2`, `streams` and
  `ManifestEntry::way`; `parse()` unpacks a v4 into `contents` (`unpack_v4`: `Lzma2Decode`, then
  `z7_BranchConvSt_X86_Dec`; masks after every raw file in the lzma2 stream), so `install_tree`
  and everything after it is v3's.
- **One decoder:** `sieve_lzma` (the LZMA SDK, public domain) moved out of the client block and is
  linked into `sieve_locate`, so `sieve`, the hallway and `sieve-install` unpack with the same code.
  `sieve-install` is no larger (it carried the decoder for 7z already).
- **The packer** (`tools/cli/pack.*`, in `sieve_compare` with liblzma): ways chosen (EXE/ELF by
  file kind; a text derived from the larger file carried as it is whose lines hold its lines in
  order, smallest mask, only where the mask packs smaller at preset 6; a file derived from is never
  derived), streams at preset 9e with the smallest dictionary that holds them; the result parsed
  back and compared; v3 kept where v4 is no smaller. `PackReport`, `pack_report_text()`.
- **Where:** `sieve locate --installer/--program` (and `--compare`'s rows) make v4, `--v3` the old;
  the File Locator saves and measures v4; `make_release.py` packs the staged folder (no 7z step;
  `--seven-zip` makes the old 7z-carrying installers for comparing); `sieve-install` and
  `sieve install` install v4 through the same path.
- **The oracle:** `sieve_ref.py unpack MANIFEST [--to DIR]`, with Python's lzma (raw, x86 + LZMA2),
  nothing of Sieve's.
- **Measured:** a release-shaped folder (Linux), 32,887,678 bytes: zip 35.26%, one LZMA2 stream
  21.17%, v4 20.00% (6,577,790 bytes; 258 KB from the x86 filter, the SCOWL masks 180 KB of masks).
  `make_release.py` on Linux (a stand-in 7z for the source archive): `sieve.sieve` 6,613,564 bytes,
  both installers installing back to the release folder byte for byte.
- **Checked:** CI (oracle step: v3 still the oracle's to the byte with `--v3`; a v4 fixture with a
  program and a derived list unpacked by the oracle and by `sieve install`, identical; a damaged
  stream refused with nothing written; hallway step: `sieve-install --yes` on a v4 with a program
  and a derived list), all run here; unit tests.
- **Found on the way:** a file named without a folder (`--locate big.bin`) gave the packer an empty
  root; it now reads from the current folder, and needs none when it has the bytes.
