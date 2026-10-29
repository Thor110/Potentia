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
- **The `.sieve` format is frozen at `sieve-manifest-v31`** (Edward, 27 October 2026: "Yes, it is
  time I stopped pushing more manifest versions, that was just caused by a succession of ideas for
  filters that cascaded into many manifest versions"). Packer improvements that need no new format
  are free to land; anything that would need v32 waits until it is worth a version on its own.
- **Purely functional.** Nothing is stored. The engine sieves the state space; it does not keep a
  database of what it found.

## The eleven lines

    binary | image  pages  books  audio  tracks  video  movies  models  worlds  ai | binary

Defined once, in door order, in `client/dimensions.hpp` (`kDimensions`): `kLines = 11`, with
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
| `core/src/aispace.cpp`, `core/src/llm.cpp`, `core/src/training.cpp` | The AI line (`aispace-v1`), the engine that runs its models, and `training-v1`, training them in pinned integer arithmetic. |
| `client/harness.cpp`, `tools/cli/train.cpp`, `tools/cli/train_cmd.cpp` | The AI Training Harness (main menu), training runs (load, train, record, pack) and `sieve ai-train`. |
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

**Who runs what** (Edward, 12 October 2026): Edward builds Debug and Release on Windows and runs
the whole suite for every package (both SHA-256 paths, 152,668 checks), then tries the change in
the program. In a conversation, run the unit suite once and the CI lines the change adds or
touches; leave the whole hallway step to GitHub's CI and to Edward. Make a check stricter where a
change needs it, not by running everything twice.

**More of the testing on Edward's side** (13 October 2026): he would rather run more tests himself,
of every kind, than have them run here, as long as they are easy to run. `tools\run_tests.bat`
runs the unit tests of every Windows build there is, then every conformance vector through the
oracle (`tools/check_vectors.py`, the same checks as CI), and says what passed. Add to it rather
than to the time spent here.

**This conversation has not been handed off** (Edward, 13 October 2026): the work so far has been
one long session, by choice. Whoever continues it keeps to the same principles: exact integer
arithmetic for anything addressed, versioned rules that are never edited, an oracle written from
the specification, the counting argument respected, measurements before claims, and every result
stated as it was found.

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
- **The interface is even and distinguished** (Edward, 13 October 2026, IDEAS §19.44): rows line
  up, controls in a row share their height, a symbol sits in the exact middle of its button (drawn
  to the pixel where the font would put it off-centre), and the corridors' wireframe style carries
  into the panels.
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

**The File Locator** (`client/file_locator.{hpp,cpp}`, class FileLocator): the tool's own locator
and comparison in a window, opened from the hallway's pause menu or the main menu. What it needs
from where it is opened is a FileLocator::Host: the lines and filters to weigh under (called on the
worker), the ink, and, in the world only, the BINARY length and a walk to a file (Hallway::
locator_host(), hallway.cpp). With no walk given (the main menu, app_main.cpp run_locator, which
makes the lines from the setup menu's settings when the first weighing starts), Go to it and Enter
are not offered, and Use the tailored filters saves them for the next hallway (IDEAS §16.2). Choose a file or folder with the system's pickers (F, D) or drop one
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
the four budget bars says "Memory Limit Change Detected : Press X to re-optimise all dimensions.".
Else, while any line's tally is on a worker (counting_lines, from pending_) or X is weighing
(calculating()), it says "Calculating Dimension... (PAGES, BOOKS)"; and "Calculating Dimensions..."
is said in red at the top, centred between the title and the budget, and ENTER THE HALLWAY is greyed
and refused until it is done (IDEAS §16.5; a scripted Return there waits for the counts, as a person
would). The line under the bars is always kept, so nothing below it moves; the
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
  README's mode list (full, and excluded, which it had left out); IDEAS §17 (§15 when it was made), a new section of
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

### The File Locator from the main menu, the calculating banner, the degrees sample (10 October 2026)
Items 16.2, 16.5 and 16.9 of Edward's list of 10 October (IDEAS §16).
- **The File Locator, standalone** (`client/file_locator.{hpp,cpp}`, class FileLocator): moved out
  of Hallway. What it needs from where it is opened is a `FileLocator::Host`: `lines` (the lines
  and filters to weigh under, called on the worker), `ink`, and, in the world only, `binary_bytes`
  (the BINARY length now) and `go` (a walk to a file); `use_filters` and `closed`. The hallway's
  host is `Hallway::locator_host()` (hallway.cpp, with `walk_to_file`, moved there); its pause menu
  and `--locate`/`--install` work as before. `fit_text` (font.hpp) is the hallway's `fit`, shared.
- **From the main menu:** File Locator, under Settings and above the Filter Designer
  (MainMenu::Result::Locator; Esc on the main screen now points at the last row, Exit, by count).
  app_main's `run_locator` makes the lines from the setup menu's settings (`make_lines`) when the
  first weighing starts, offers no Go to it (nor Enter, nor the BINARY length row), and saves
  tailored filters for the next hallway (`loc.filters_kept`). `--main-menu --locate PATH
  [--tailored] [--press KEYS] --screenshot` draws it and prints whether it is open and offers Go.
- **Calculating, where it is seen:** `Menu::calculating()` (a tally on a worker, or X weighing).
  The setup menu says "Calculating Dimensions..." in red at the top, centred between the title and
  the budget: words that never change, at the largest size the window has room for (2, 1.5 or 1),
  so it keeps its size and place while the lines finish; the menu's narrowest width allows for it.
  Which lines, as before, under the bars. ENTER THE HALLWAY is greyed and refused until it is done;
  a scripted Return there (`Menu::press`) waits for the counts first, as a person would.
  `--menu --busy` draws the menu before the counts are in.
- **The degrees sample** (`Hallway::sample_degrees`, item_save.cpp; `--sample-degrees DIR
  [--sample-out FILE]`): for every dimension and every whole degree, the first unit at or past the
  bearing (ceil(d * units / 360), the navigator's rule, in the ordering in use), saved in
  `DIR/<id>/` by `item_file` (what J reads: a picture at one pixel a pixel; F saves pictures at
  16), a binary file as its bytes, named by its title as F names it (`item_file`'s name,
  `binary_file_name`; " (2)" where two share one), the report saying which file each degree gave
  ("<id> <ddd> <file>"); then DIR as Sieve instructions (`add_packed`, v4). Defaults: 3,240 items,
  566,236 bytes, a 391,724-byte `.sieve`, in about 3.5 s (Release). A file is the item's unit and
  its name the title; a cover is not kept (Edward, 10 October: the pictures are for looks). At
  whole degrees the units repeat (every 40 degrees on pages, image, models, binary and books, every
  72 on audio, tracks, video and movies; the leading part of d/360 is the title), so only 225
  contents differ. On the lines without a cover the titles tell the items apart; on those with one
  the cover leads the address, so the titles repeat too (45 pairs a line, found with worlds added;
  the " (2)" names them apart). The pages item at 90 degrees is titled
  `ftft...ft` and reads `ftft...fu`: a quarter of the line in base 27 repeats.
- **Checked:** CI (hallway step: the main-menu locator offers no Go and Enter does not close it;
  the sample twice, identical; installed by `sieve install`, identical; X, Tab, a bearing and Enter
  on pages and models, then the item taken and saved, equal to the sample's file), all run here;
  screenshots of the main menu, both locators and the setup menu while counting (1920 and 1240 wide).

### The settings as a slide-in panel (10 October 2026)
Item 16.6 of Edward's list. The setup menu's settings, GLOBAL down to ENTER THE HALLWAY, are a
panel (menu.hpp `panel_`, `step_panel()`, `toggle_panel()`): drawn `-(1 - panel_) * kMapX` to the
left of their place, sliding over 180 ms (`kPanelSlideMs`), while the map's left edge goes from
`kMapX` to just past the SETTINGS tab (`kPanelTabW`), so with the panel out the map takes the
width. Tab, or a click on the SETTINGS tab (drawn while the panel is mostly out), toggles it by
hand. While `calculating()` has lasted 200 ms (`kPanelCalcNs`, as CALCULATING waits), it slides out
by itself, once a calculating (`calc_acted_`, so Tab can bring it back meanwhile), and back in when
the counting ends if that is what slid it out (`panel_auto_`). Moving through the settings (the
arrows, PgUp/PgDn, Enter) with it closed by hand brings it back; the keys act on the settings
either way. The footers, the over-budget line and the calculating banner are not part of it. A
scripted key snaps it to where it is going (`Menu::press`), so `--menu --press Tab` draws it out.
The first counts when the menu opens slide it out too, if they take longer than 200 ms (a Debug
build, a large stack).

### Worlds (10 October 2026)
Item 16.7 of Edward's list, first version. Edward chose: each slot a model and its placement (no
texture yet), placement as a grid cell and one of 24 turns, 4 models a world by default, with a
cover and a title.
- **Core** (`core/include/sieve/worldspace.hpp`, `core/src/worldspace.cpp`, SPECIFICATIONS §12.0):
  `WorldSpace` numbers cover, title, then each slot (its model's positional index on the models
  line, its cell x, y, z, its turn) as one mixed-radix number; scrambled through shuffle-sha256-v1
  keyed with the models line's key, domain its id. `turns()` is `turns-24-v1` (permutation, then
  signs, determinant +1). `mesh_of` and `to_obj` place every model exactly (multiples of 1/C).
  `WorldSieve`: cover and title stacks and the models line's `ModelSieve` for each slot, counted
  and ranked exactly (`world-compact-v1`). A world space is about `N * (bits of a model + log2(G^3 * 24))`
  bits: 1,122 at the defaults.
- **Oracle and tests:** `sieve_ref.py world-vectors` (48 vectors and the turns table) and
  `world-obj` (a world's `.obj` from its slots), written apart from the C++; `test_world_vectors`
  checks addresses both ways, the turns and the `.obj` hashes; `test_world_sieve` checks the
  sieve against brute force (every 101st world of a small space, with and without
  canonical-mesh-v1).
- **The tool:** `sieve world` (shape and size; `--compose SLOTS [--title] [--cover]` for
  addresses, survivor number and compact addresses under `[worlds]`; `--read ADDR [--compact]
  [--mode] [--out]` for the `.obj`), with the hallway's option names (`--world-models`,
  `--world-grid`, the models' and cover's and title's).
- **Filters config:** `WorldFilters` (`[worlds]`, `[worlds.cover]`, `[worlds.title]`,
  `[worlds.models]`), `build_world_stacks`.
- **The hallway:** `Media::Worlds` (appended to the enum, so saved per-medium lists only grow),
  door after MODELS, models' clay with black edges. `world_space_`, `world_stacks_`,
  `world_sieve_`; `Book::world`. Shelves: a crate whose face is the whole world drawn as a model
  is (`world_mesh`: the world scaled by 1/G; `render_mesh_face`, which `render_model_face` now
  calls). In hand: `draw_world` (through `draw_mesh`, which `draw_model` now calls), turned as a
  model is. F and J: the `.obj` (`item_file`); the viewer shows it; the vault judges it as a
  model's `.obj`. The ordering toggle keeps the world in front of you. T (warp) refuses with a
  message: there is no form to warp in from yet. Real Graphics: `book-worlds.obj` (a copy of
  `book-models.obj`) and a `[worlds]` section in `faces.ini`.
- **The setup menu:** WORLDS rows (models per world, grid), its map column, FIND MY LIMITS (models
  per world grows, as units per track does), and its filters window with COVER, TITLE and MODELS
  parts (`has_parts` and `parts_cfg` now cover worlds; the models part counts through
  `ModelSieve`, `worlds_stack_info`).
- **The degrees sample** now has ten dimensions: 3,600 items, 1,055,836 bytes, a 436,788-byte
  `.sieve`; worlds repeat every 72 degrees, as the other compositions do.
- **Checked:** unit tests (world vectors and sieve); the oracle's vectors regenerated and
  identical; CI's model step (`sieve world` compose, read, the oracle's `.obj`, the scrambled
  address giving the same world) and hallway step (the world at a composed address saved with F is
  the tool's `.obj`; the WORLDS line draws), all run here; screenshots of the shelves, a world in
  hand, the setup menu and the worlds filters window; compact worlds under
  `distinct-indices-v1` count the same in the hallway and the tool.
- **Not yet:** textures and UVs (IDEAS §14), warping a world in (from an `.obj` with groups, or a
  record), filters on a model's place (for example, no two models in one cell).

### The settings panel scrolls (10 October 2026)
- **Why:** more settings and dimensions are coming, and the menu was being scaled down to show
  every row at once (at 1920 x 1080 it was already drawn at 95%).
- **The list:** the settings (every row before FIND MY LIMITS) are laid out first (`at`, from
  `kListTop`, `kRowH` a row and `kHeadH` more for a section's heading) and drawn in the room above
  the three actions, clipped to it, `list_scroll_` px down. The actions stand `kActionsGap` below
  the list, at least `kActionsUp` above the foot and at most `kActionsLow` (clear of the line of
  status); a longer list scrolls (`list_scroll_max_`).
- **Moving it:** the wheel over the panel scrolls three rows a notch; a row newly chosen with the
  keys is brought into view with its heading (`list_row_shown_`). A thin bar at the panel's right
  edge shows where the room is in the list.
- **The highlight:** centred on its row (4 px above and below the text); it was 3 above and 5 below,
  so the chosen row crowded the one after it.
- **Size:** `kMinH` is now GLOBAL's heading and rows, the gap and the actions, so 1280 x 720 draws
  at full size. `kMinW` now takes the map's columns at their narrowest (`kMapPitchMin`, 80 px,
  which was a bare number); at 1280 wide the last BINARY column had run off the edge.
- **Checked:** screenshots at 1920 x 1080 and 1280 x 720, at the top of the list and at BINARY.

### Item models for worlds and AI; entering a world, noted (10 October 2026)
- **Meshes:** `book-ai.obj` and `book-ai.mtl`, copies of the models line's crate (as
  `book-worlds.*` already were), and an `[ai]` section in `faces.ini`, ready for the AI dimension.
  The line's id is assumed to be `ai`; if the AI dimension takes another id, rename the two files,
  their `mtllib` line and the section. `templates/README.md` now lists worlds and AI.
- **IDEAS §14:** Edward's idea of an ENTER button on worlds (ghost mode), a games dimension built
  from worlds, models, UVs and code manifests, and translation layers, written down and weighed.

### SmolLM2-360M-Instruct taken apart (10 October 2026)
- **The model:** `HuggingFaceTB/SmolLM2-360M-Instruct`, commit `a10cc15...` (Apache 2.0), downloaded
  through the session's allowed domains (`huggingface.co`, `hf.co`, and `us.aws.cdn.hf.co`, where
  the large file is redirected). `model.safetensors`: 723,674,912 bytes, SHA-256 `e6bffe74...f86e`,
  as Hugging Face gives it. Not in the repository.
- **Findings** (IDEAS §15, "The first model, taken apart"):
  - **The start** (32,672 bytes) is rebuilt byte for byte from `config.json`.
  - **The weights:** 361,821,120 bf16 values. The high byte carries 2.72 bits, the low byte 7.85 bits,
    and neighbours tell almost nothing.
  - **Lossless:** coded by frequency, 476 MB (66%); xz -6, 512.9 MB (71%).
  - **The tokenizer:** 2.1 MB (436 KB under xz).
- **Next:** the dissection in `sieve` (safetensors reader, header rebuild, per-tensor statistics),
  then the weight prior (§16.8b), then the AI dimension.

### `sieve tensors`: a model file taken apart (10 October 2026)
- **Core:**
  - `sieve/json.hpp`: a small strict JSON reader and compact writer, keeping members in order and
    escaping as serde_json does;
  - `sieve/safetensors.hpp`:
    - `read_header` and `parse_header`, which check that the offsets hold the shapes, with no
      gaps and no overlaps;
    - `layout_start` and `is_canonical`, which are `safetensors-layout-v1`;
    - `llama_tensors` from a `config.json`;
    - `StatsBuilder` for BF16 and F16.
  - SPECIFICATIONS §12.0a.
- **The tool:** `sieve tensors` (`tools/cli/tensors.*`): the size, the SHA-256 and the two parts in
  one pass, whether the start is the layout's, `--config` (does the config name the file's
  tensors; alone, the start a model of that shape has), `--stats` by kind of tensor, `--tsv`
  per tensor, and `--start-out`. `--stats` and `--tsv` are new switches (`args.hpp`).
- **The oracle:** `tensors` (the start rebuilt and the `--tsv` table, written apart) and
  `tensors-fixture` (a small Llama model of a config's shape, seeded values, laid out by the
  oracle's own layout code). `tests/llama_tiny_config.json` is that shape for tests.
- **Checked:**
  - **On SmolLM2-360M-Instruct:**
    - the hash is right;
    - the start rebuilt from the tensors and from `config.json` is byte for byte the file's;
    - `--tsv` is identical to the oracle's on all 290 tensors;
    - coded by frequency, 475,928,934 bytes;
    - 4.4 s for the file (the oracle takes 49 s).
  - **Unit tests:** JSON, the layout (including the pinned SHA-256 of SmolLM2's start built
    from its config alone), bad headers, and the statistics; 149,802 checks, 0 failures.
  - **CI:** the stand-in model, its start rebuilt from its config, and the oracle's table.
- **Next:** the weight prior (IDEAS §16.8b), measured against these figures; then the AI
  dimension (§15).

### `sieve-weights-v1`: the weights coded under a prior (10 October 2026)
- **Core:** `sieve/weights.hpp`:
  - `quantise`, a kind's table: `1 + floor(c (2^24 - k) / N)`, the rest to the most frequent value;
  - `encode` and `decode`, rANS with a 64-bit state, 32-bit words and 24-bit frequencies. Decoding
    finds a slot's value from a 65,536-entry index of where each block of 256 slots begins;
  - `pack` and `unpack`, the stream (SPECIFICATIONS §12.0b), ending with the file's SHA-256, which
    `unpack` checks.
- **Why tables by kind:** measured on SmolLM2, a table for each tensor costs 4.6 MB to store and
  saves 0.7 MB. Row and column scales were measured too, and save 0.3 MB, so v1 leaves them out.
- **The tool:** `sieve tensors FILE --pack OUT` (read back and checked before it reports) and
  `--unpack FILE --out OUT`.
- **The oracle:** `weights-pack` and `weights-unpack`, written apart. Their streams equal the
  tool's byte for byte (on the stand-in model in CI), and the oracle's unpack rebuilds SmolLM2
  from the tool's pack (3 min 42 s in Python).
- **SmolLM2-360M-Instruct:** 476,729,540 bytes (65.9%): the start 32,675, the 11 tables 117,878,
  the coded tensors 476,578,938. Pack (with its check) 39 s, unpack 12 s, byte for byte.
- **Tests:**
  - **Unit tests:** the table rule, coding and back (skewed values, one value, none, a value its
    table lacks), and a whole small model with an F32 tensor kept as it is, packed and rebuilt;
    damaged, short and over-long streams are refused. 149,840 checks, 0 failures.
  - **CI:** the stand-in model packed by both, the files compared, and each unpacked by the
    other.
- **Next:** the AI dimension (IDEAS §15). Better priors are later versions, measured against this
  one.

### `sieve chat`: the language model run by Sieve itself (10 October 2026)
- **Edward's choice:** no Ollama; the engine is Sieve's own (IDEAS §16.8). SPECIFICATIONS §12.0c
  sets it out.
- **Core:**
  - `sieve/unicode_classes.hpp`: letters, numbers and white space; the table is generated for
    Unicode 14.0 by `tools/gen_unicode_classes.py`.
  - `sieve/llm_tokenizer.hpp`: byte-level BPE from tokenizer.json, with added tokens, Digits,
    the GPT-2 pattern written as a scanner, merges by rank, and decoding.
  - `sieve/llm.hpp`:
    - `Model`, the Llama forward pass, with BF16 weights kept as stored and 32-bit arithmetic;
    - a key and value cache per layer;
    - a small thread pool that shares out rows, each row summed in one fixed order, so the result
      is the same for any thread count;
    - `Sampler` (temperature, top-p, top-k, repeat penalty, splitmix64);
    - `Chat` (ChatML, whole UTF-8 characters streamed).
- **The tool:** `sieve chat`:
  - `--prompt`, or a conversation from standard input;
  - `--encode` and `--encode-lines`;
  - `--logits` and `--logits-out`.
  - How long it read and wrote goes to standard error.
- **The oracle:**
  - `llm-encode-lines`: a tokenizer written apart, using Python's unicodedata and GPT-2's
    merge-all loop;
  - `llm-logits`: the forward pass in 64-bit floats, the whole sequence at once, with
    `--compare` against the tool's logits;
  - `llm-tokenizer-fixture`: a 300-token BPE learnt from a fixed text, as
    `tests/llama_tiny_tokenizer.json`;
  - `llm-corpus`: texts to tokenize.
- **Checked against the real thing:**
  - **Tokenizer:** the tool's and the oracle's tokens equal Hugging Face's `tokenizers` on 5,736
    texts.
  - **Forward pass:** its logits are within 5.4e-5 of the official ONNX export run by ONNX
    Runtime (`onnx/model.onnx`, float32, same commit), on a 42-token chat prompt; KL 2e-11.
    PyTorch could not be installed here (its download host is blocked); ONNX Runtime came from PyPI.
  - **SmolLM2:** finds " Paris" after "The capital of France is", and holds a conversation at
    about 9 tokens a second on four cores; loading takes 1 to 3 s.
- **Tests:**
  - **Unit tests:** the Unicode classes, the byte characters, the stand-in tokenizer (pieces,
    round trips, added tokens), and a small model written to a file (the same logits with one
    thread and with four, after a reset, sampling seeded and greedy, a chat that streams);
    150,402 checks, 0 failures.
  - **CI:** the stand-in tokenizer regenerated and compared, the tokens of the oracle's corpus,
    the logits within 1e-3 of the oracle's, and a reply.
- **Next:** the AI dimension in the hallway: the room, the items, and the chat viewer on this engine.

### The AI line (10 October 2026)
- **Edward's choices:** an item is a model (every model of one shape, numbered by its weights),
  and the default shape is Micro. SPECIFICATIONS §12.0d sets it out; IDEAS §15 ("Another reading")
  and §16.8 say where it stands.
- **Core:**
  - `sieve/aispace.hpp`: `AiShape` (layers, width, heads, bits; `check()`) and `AiSpace`: the
    weights' digits as the address (positional), shuffle-sha256-v1 over the line (scrambled), the
    digits' values, the model's `config.json`, tensors, F32 `model.safetensors` and byte
    `tokenizer.json`, and a file back to its digits (exactly, or not at all).
  - `sieve/llm.hpp`: a `Model` built from tensors in memory (no file), `continue_text` (no chat
    format, an optional stop token) and `Chat::last_reply_tokens()`.
- **The tool:**
  - `sieve ai`: the line's size; `--read`, `--bearing` or `--browse` run each model and print what
    it says; `--out` writes its files; `--warp` takes a file back to its address.
  - `sieve chat --raw`: continue a text with no chat format. The reply's statistics now read "read
    N tokens in X s; wrote M in Y s, Z a second".
- **The oracle:** `AiSpaceRef`, `ai-vectors` (`tests/vectors_ai_v1.tsv`, 30 rows: three shapes,
  both orders, the bearings 0, 13, 137, 246 and 359) and `ai-read` (a model's files);
  `llm-logits` now reads F32 too. The files are the tool's byte for byte; the logits agree to 1.5e-6.
- **The hallway:**
  - `Media::Ai`, appended; its door after WORLDS; deep violet with lilac edges. The AI line has
    no filters and no guided order (`effective_mode` is off); `Book::ai` holds the digits.
  - **Shelves:** a crate (`book-ai.obj`) whose face is what the model says from a newline, the
    bytes that are not printable drawn as `·` (`ai_says`, `printable`, cached in `ai_said_`).
  - **In hand:** `draw_ai`: the shape, what it says, and the conversation. Enter opens `SAY > `
    (`Input::Talk`); `talk()` runs the model on what was said and a newline, and it answers until
    it writes a newline (at most 80 bytes), seeded from the SHA-256 of the conversation. When its
    512 positions are full it starts again.
  - F saves `sieve-ai-<12 hex digits>.safetensors`; J finds it on the binary line; the viewer has a
    WEIGHTS tab. T refuses (`msg.warp.ai`), pointing to `sieve ai --warp`.
  - **The setup menu:** AI rows for layers, width (in steps of twice the heads), heads (divisors
    that keep the head size even) and bits; FIND MY LIMITS grows the layers. The map ignores a bar
    more than 8 times the median when it sets its scale and draws that one broken
    (`kMapOutlier`, `map.scale.broken`), as the AI line's 32,768 bits otherwise flattened the rest.
  - `--talk TEXT` (after `--take`) prints the conversation, for scripts and CI.
- **The degrees sample** has eleven dimensions: 3,960 items, 13,321,756 bytes, a 482,495-byte
  `.sieve`.
- **Checked:** unit tests (the vectors, round trips, addresses off the line refused, bad shapes, the
  values, a model talking): 150,848 checks, 0 failures. CI's new steps, all run here: `sieve ai` at
  246 degrees against the oracle's files in both orders, warp, the logits against the oracle,
  `chat --raw`, the vectors regenerated; in the hallway, the model at 137 degrees saved with F is the
  tool's file, a conversation, the setup menu, the doors (WORLDS to AI to BINARY), the sample and
  the corridor walk (nine doors from pages to binary, and back). Screenshots of the shelves, a model
  in hand, the WEIGHTS viewer, the setup menu's AI rows and the broken bar.
- **Not yet:** filters and a guided order for the line; warping in from the hallway; addresses of
  text under a model (the pinned arithmetic, IDEAS §15).

### The AI Training Harness, and training on the record (11 October 2026)
- **Edward's ideas:** a training harness that records exactly what a model learnt from and in what
  order, so that anyone can train it again and get the same model (the thesis's recorded training
  trajectories); an "AI Training Harness" on the main menu; and, later, no `.map` at all, the
  `.sieve` doing its work from manifest v5 (IDEAS §16.3). A run is therefore a folder packed as an
  ordinary v4 `.sieve`, not a map, and needs no new format.
- **Core, `training-v1`** (`sieve/training.hpp`, SPECIFICATIONS §12.0e):
  - every number a 64-bit integer, Q16 activations and gradients, Q32 master weights; every
    rounding down, every stored value held to ±2^23; 2^x, sine and cosine as fixed polynomials,
    the square root exact, all constants in the specification;
  - the Llama forward pass and its backward pass, written out in that arithmetic; the
    quantisation in the loop (the forward pass on the line's values, the master weights taking the
    gradient), so every checkpoint is a model of the AI line;
  - SGD with momentum; integer sums, so the batch's windows go to threads and the result does not
    change (checked: one thread and three agree);
  - `Recipe` (training.ini), `order.txt` (`sieve-order-v1`, every window written out) with
    `make_order`, `Result` (result.txt: checkpoints with their addresses, the final model's
    SHA-256).
  - Tuned: learning rate 0.0005, momentum 0.9, 4 windows of 65 bytes a step. A Micro model goes
    from 11.9 to 2.9 bits a byte on *Die Universalbibliothek* in about five seconds.
- **The oracle:** `TrainerRef`, written from §12.0e alone; `train-vectors`
  (`tests/vectors_training_v1.tsv`: nine steps on three shapes, and the functions at a few
  points) and `train-replay` (a run folder trained again, its result.txt written). It matched the
  C++ on the first run, step for step, and on two recorded runs byte for byte, every checkpoint
  address and the model's hash included.
- **The tool:** `sieve ai-train` (the name `sieve train` was taken by the guided models):
  `--record CORPUS --out FOLDER` (and `--sieve`, `--model-out`, `--address-out`), or a run named to
  train again and check (exit 1 if not reproduced). `tools/cli/train.*` (in `sieve_compare`, which
  the hallway links) holds the runs; `train_cmd.cpp` the command.
- **The hallway:** the main menu's sixth item, `Harness` (`client/harness.*`): the runs in
  `training/` beside the programs; Enter trains on a worker (progress, the bits-a-byte curve, the
  verdict, what the model says); N records from a typed path and trains; P packs; G goes into the
  hallway on the AI line in the run's shape with the model in hand (`Hallway::go_to_ai`, skipping
  the setup menu once). Scripting: `--harness [--training-dir DIR] [--script KEYS,=text]`.
- **Shipped:** `data/training/universalbibliothek.sieve` (79 KB: the run, trained, its result
  included), copied to `training/` by the build and the release. Training it again on Windows is
  the first check that MSVC's integers give what GCC's did.
- **Checked:** unit tests (the vectors, threads, the files read back): 150,974 checks, 0 failures.
  CI's new steps, all run here: a tiny run recorded, trained again by the oracle to the same
  result.txt, again from its `.sieve` with one thread (reproduced), with windows of two steps
  swapped (not reproduced; within a step the order cannot matter, the gradients being summed), the
  shipped run reproduced; the harness reproducing the shipped run, recording and training a run,
  and going to its model; the vectors regenerated. Screenshots of the main menu, the harness after
  training, and the model in hand.
- **Not yet:** learning-rate schedules and other optimisers (each a new version); training wider
  models faster (the arithmetic is plain loops); runs with several models (a curriculum); maps
  folded into manifest v5.

### Curricula, and runs that carry only what they need (11 October 2026)
- **Edward's decisions:** maps stay maps, and the manifest gains nothing for training (a file
  carries only what it needs), so the plan to fold maps into manifest v5 is set aside (IDEAS
  §16.3, corrected). Runs are made by the harness (or `sieve ai-train`), and other `.sieve` files
  carry nothing of training's. Edward's idea of recording the file order as item numbers in the
  instructions' list is noted there for a far later manifest. And the harness puts a new run's
  files in order: a curriculum.
- **Run format 2** (SPECIFICATIONS §12.0e):
  - `training.ini` holds the order as a rule: `order = order-v1`, `reading` (`in-order`,
    `shuffled-within-files`, `shuffled`), `window`, `epochs`, `seed`, and a `file =` line for
    each corpus file in the order read. `order.txt` is written only with `order = order.txt`
    (`--write-order`), for orders the rule cannot say.
  - `result.txt` is `sieve-training-result-v2`: each checkpoint by the SHA-256 of its digits,
    its address only with `checkpoint-addresses = yes`; the final model's address and SHA-256 as
    before.
  - The first runs' format (no `order` line, a v1 result) still trains and is checked
    (`tests/training_run_v1`, by the tool and the oracle in CI).
- **Core:** `Reading`, `make_order(..., reading)`, the recipe's new lines, `Result` v1 and v2.
  `tools/cli/train.*`: the order made when a run is loaded; `corpus_files()` and
  `record_training_run(files, ...)` take the files in the order to read them (`TrainingFile`; the
  name `CorpusFile` was taken by the guided models' corpus). `sieve ai-train`: `--reading`,
  `--file-order`, `--checkpoint-addresses`, `--write-order`.
- **The harness:** N, then a path, lists the corpus's files: Up/Down choose, Shift+Up/Down move
  (or the buttons), M the reading, Left/Right the epochs (Shift: by 10), Enter records and trains.
  A run's page shows how it reads and its files in the order read. The curve is now about 200
  points across the run, not one a checkpoint.
- **The shipped run** was recorded again in format 2: the same order (the rule is the one the
  first recorder used), so the very same final model (`b3e5b752...`), now 14.5 KB instead of 79 KB:
  the order is 10 lines of the recipe instead of 391 KB of text (20 KB packed), and the result
  lists hashes instead of addresses (50 KB packed before).
- **Checked:** unit tests (the readings, the recipe's lines, both results read back): 150,992
  checks, 0 failures. CI's training steps, all run here: each reading with a chosen file order,
  trained again by the oracle to the same result; a written-out order with checkpoint addresses,
  reproduced from its `.sieve` and not reproduced with windows of two steps swapped; the
  first-format fixture by the tool and the oracle; the shipped run; in the harness, a run recorded
  from a folder with its second file moved first and read in order. Screenshots of the curriculum
  list and of the run it made.

### The harness's folder dialog and model settings (11 October 2026)
- **Edward's asks:** a folder chosen rather than a path typed, no single-file corpora, and models
  as adjustable as the state spaces are.
- **A corpus is a folder:** `corpus_files()` refuses a file (the CLI's `--record` too); the
  harness's N opens the system's folder dialog (`SDL_ShowOpenFolderDialog`, as the File Locator
  does), a path still typed in its place for scripts (`set_scripted`).
- **The new run's page** (Tab between its two parts): the settings (layers, width in steps of two
  heads, heads that keep the head size even, bits, learning rate on 1-2-5 steps, windows a step,
  window size, epochs, reading), starting from the AI line's shape as the setup menu has it
  (`ai_shape_of(Settings)` in `app_main.cpp`); and the files in reading order.
- **What it will take, shown as the settings change:** `estimate_training()` (weights, windows,
  steps, threads, memory per thread and in all, the result's size) and `measure_training_step()`
  (a step trained on the first windows, timed, on a detached thread so a large shape's minute never
  holds the window); memory past half the computer's (`SDL_GetSystemRAM`) in red and not measured.
- **The learning rate suggested:** `suggested_learning_rate()`, 0.008 / width × 256 / (batch ×
  (window − 1)) rounded down to 1-2-5, no lower than 0.00002, from a sweep on the shipped text
  (best: 0.0005 at width 16, 0.0002 at 32, 0.0001 at 64, 0.00005 at 128; layers mattering little;
  width 128 at 0.0005 went from 4.1 to 9.2 bits a byte, worse than guessing). The harness and
  `sieve ai-train` use it unless a rate is given; Micro's is still 0.0005, so the shipped run is
  unchanged. Near 1/65536 the page and the tool say training-v1's floor is near (IDEAS §15, larger
  models).
- **Measured:** 8 layers of width 1024: 134,479,872 weights, a step 41.8 s here, about 29 h for
  2,475 steps, 7.7 GB, a result of 129 MB.
- **Checked:** CI's training steps (a file refused as a corpus; the folder runs as before) and
  the harness's (a setting changed by keys, the width 16 to 20 and the rate following to 0.0002),
  all run here; screenshots of the settings at width 28 and at 8 layers of width 1024.
- **Next (agreed order):** threads within a window and faster loops; resuming; training-v2 for
  the precision wide and deep models need.

### Training: faster, resumable, and training-v2 (11 October 2026)
- **Edward's agreed order:** faster training, then resuming long runs, then training-v2.
- **Faster** (`core/src/training.cpp`; results bit for bit as before):
  - activations and weights' values kept as 32-bit integers (every stored value is held to ±2^23),
    products summed in 64 bits;
  - each layer's matrices taken whole (`forward`, `backward`, `gradient`), a row of weights read
    once against every position;
  - a persistent `Pool`: a small model gives each window a thread (each its own gradient, summed
    after), a large one (2^20 weights or more) gives each window every thread, its rows (forward,
    gradient) or positions (backward) shared out so no two threads add to one place; attention in
    two passes (queries, then keys and values) for the same reason;
  - AVX2 integer kernels (`dot`, `axpy`: `_mm256_mul_epi32`, 64-bit lanes), chosen at start-up as
    SHA-256's SHA-NI path is (CPUID and XGETBV); `use_vector_kernels()` switches them off for the
    tests, which run the vectors both ways.
  - One bug on the way, caught by the reproduced runs before anything else: a product of two
    32-bit values taken in 32 bits in attention's backward pass (`P[u] * hold(...)`), which
    overflowed; every other product was audited to have a 64-bit operand.
  - Measured (4 cores): 8 layers of width 1024, a step 41.8 s to 12.2 s; width 128, a pass 4.9 s
    to 2.2 s; width 64, 1.4 s to 0.8 s; Micro unchanged (its time is in the softmax, not the
    matrices).
- **Resuming:** `Trainer::masters()`, `velocities()`, `restore()`; `sieve-training-state-v1`
  (`tools/cli/train.cpp`: text lines with the recipe's and order's SHA-256, the step, the score
  since the last checkpoint and the result so far; then the masters and momenta, 8 bytes each)
  saved at every checkpoint and when stopped, as `RUN.resume` beside the run (`state_file_of`),
  never in it, written whole and then renamed. `sieve ai-train --resume`, `--stop-after N`; the
  harness keeps it always: Esc stops and keeps it, the page says where, Enter carries on, S starts
  over. A run's result parser now accepts the empty `final` of a result so far.
- **training-v2** (SPECIFICATIONS §12.0e): `loss-scale-bits = k` (0 to 7); the output gradient
  times 2^k; the rate held to 16 + k bits; the update `(v · η) >> (16 + 2k)` with `fx::mul_shift`,
  an exact product of 128 bits from 32-bit halves (no `__int128`, which MSVC lacks); limits 2^(43+k)
  and 2^(46+k). k = 0 is v1 exactly. New runs are v2 (`--training-v1` for the first version); the
  loss scale is suggested (`suggested_loss_scale`: 6 for a width of 64 or more or 4 layers or
  more) and the learning rate's floor follows it (8 parts in 2^(16+k)). The oracle: `TrainerRef`'s
  `shift`, `train-vectors --v2` (`tests/vectors_training_v2.tsv`, ten steps, one rate below v1's
  floor), `train-replay` of either version. The measurements and the slip (16 + k at first) are in
  IDEAS §15, larger models.
- **The harness:** a loss scale row (suggested, as the rate is); the stopped state, Carry on and
  Start over.
- **Checked:** unit tests (both kernel paths, threads, the v2 vectors, `mul_shift` past 64 bits);
  CI's training steps, all run here (a v2 run replayed by the oracle; a run stopped twice and
  carried on, equal to the oracle's replay; the earlier steps); the shipped run and the first
  format's fixture reproduced after every change.

### The AI line's filters (11 October 2026)
- **Edward's direction:** seek the noise; set aside structure (runs, repeats, stepping), which
  trained weights do not have. Then the symmetries, one model of each class. SPECIFICATIONS §12.0f.
- **Measured first:** models trained from the shipped text at seven shapes (1 to 8 bits, 1 to 4
  layers, width 16 to 64): the longest run in a row 12 (1 bit), 9 (2), 5 (4), 3 (8); no row a
  ramp; at 1 bit two rows repeated a block of 6 or 7 digits (chance, in rows of 16 binary digits).
  The defaults (`max_run = 16`, `period = 4`, ramps on) pass them all.
- **Core:**
  - `FilterLine::ai` (the space; `cli::ai_filter_line`), and `Filter::survivors()` for a filter that
    counts without ranking.
  - `core/src/filters/weights.cpp`: `RowRule` (one row's survivors from a prefix: runs by a table,
    periods by Möbius and runs round a circle, ramps), `RowsRanker` (the product over rows,
    mixed radix in address order; state walk for the guided coder), `weight-rows-v1` (soft),
    `canonical-model-v1` (hard).
  - `AiSpace::canonical_of`, `is_canonical`, `canonical_count` (exact, product trees, worked out
    once), `canonical_log2_count`. Members keyed by SHA-256 ("canonical-model-v1" and the digits),
    not by their digits: by digits, the first row of every sorted group came out in order and the
    trained models' canonical forms failed `weight-rows-v1`.
  - `not-a-file-v1`, `not-a-pattern-v1` and `symbol-entropy-v1`, which apply to any line, are not
    offered on the AI line: the first two keep tables that grow as the square of the length
    (not-a-file ran out of memory at width 256), and the row filter does their work per row.
- **CLI:** `sieve filters --line ai` (each filter's share, exact; the stack's survivors);
  `sieve ai` prints each model's verdicts and, under a stack that ranks, its compact addresses;
  `--canonical`, `--address-file`, `--address-out` for every reading, `--filters`.
- **Hallway:** the `[ai]` section; modes, compact (survivors closed up, compact addresses, survivor
  numbers), mark, hide, excluded; the status line; the setup menu's magnifying glass on the AI
  column, the overlay with live counts (to a line of 2^26 bits; canonical alone from its log past
  2^20). The harness's G: where the filters set the trained model aside and its canonical form
  passes, it goes there ("as its class's canonical form").
- **Oracle:** `WeightRowRef` counts by enumerating every structured row (not Möbius), `ai_canonical`,
  `ai_canonical_count`; `ai-filter-vectors` (`tests/vectors_ai_filters_v1.tsv`), `ai-canonical`.
- **Checked:** unit tests (brute force over every row of 8 digits of 4 values at five settings;
  the ranker's own rank and unrank against the generic walk; the logits of a model and its
  canonical form agree; the oracle's vectors); the CI lines, all run here; the hallway in compact,
  mark and hide, the overlay, and G, by screenshot.
- **Open:** ranking canonical-model-v1 (a walk that keeps which members are still tied); the
  language filter and the layers around anchors (IDEAS §18.2).

### Language and layers on the AI line (11 October 2026)
- **Edward's choice:** the language filter and the layers round anchors (IDEAS §18.2), after the
  AI line's filters. SPECIFICATIONS §12.0f, "Filters that name a run".
- **Core:**
  - `training::evaluate_model`: a model's score on windows, the forward pass alone, one thread
    (Trainer::evaluate without a trainer); the tests check the two agree.
  - `ModelRun` and `set_model_run_loader` / `model_run` (filter.hpp): runs named by filters,
    loaded by a function the programs set, kept once loaded.
  - `language-v1` (judges: `1000 · prob_sum ≥ min_permille · n · 2^16`) and `near-model-v1`
    (`NearRanker`: state = place · (k + 1) + changes used; `ways(r, b)` cached) in
    `core/src/filters/weights.cpp`.
- **CLI:** `model_run_of`, `use_training_runs` (tools/cli/train.cpp; the dirs are passed in, since
  sieve_compare cannot call sieve_lines' `install_dir`); `sieve ai` prints `speaks` and `layer`;
  `--exact` is now a flag.
- **Hallway:** `use_training_runs` at start (the harness's folder, then data/training); the top
  bar names the language and the anchor.
- **Measured:** random Micro models 2 to 7.5 thousandths on the run's text, its model 275.7, hence
  `min_permille = 20`; the speakers round the model reach out to about a quarter of its digits
  changed (IDEAS §18.5, the table).
- **Oracle:** `NearModelRef`, `ai-run-filter-vectors` (`tests/vectors_ai_run_filters_v1.tsv`),
  the language rows from `TrainerRef.window`.
- **Not done, and why:** ranking canonical-model-v1. Its SHA-256 keys cannot be counted under a
  prefix; see IDEAS §15 for the choice this leaves.

### Manifest v5, stage 1, and the judge-only tab (12 October 2026)
- **JUDGE-ONLY FILTERS** (Edward): `FilterSpec::judge_only` (canonical-model-v1, language-v1),
  `tab_of` gives it tab 3 (`kFilterTabs = 4`, menu.hpp), the tick-all keys leave it alone, and
  `sieve filters` marks it.
- **Navigator random buttons** (Edward): `nav_random(any_line)` and `nav_reset_layout` in
  navigator.cpp; R and Shift+R, and two buttons top right.
- **sieve-manifest-v5** (SPECIFICATIONS §12.2; Edward chose stage 1: `sieve install` and the
  hallway install it, `sieve-install` refuses it with a word):
  - `locate.*`: `Manifest::items`, `lines`, `tree`; `tree_sha256`; `set_item_maker` (the hook the
    programs fill; without it a v5 with items is refused); `item K ADDR` ways in `unpack_v4`,
    and a derived file's base may be an item; install_tree takes `-` for an item's SHA-256.
  - `pack.*`: `add_packed(m, root, report, ItemFinder*, item_hashes)`; items marked first, left
    out of the streams, lines renumbered by first use; no v3 fallback with items.
  - `items.*` (sieve_lines): `item_lines` (pages of --alphabet and --page-length, image, video,
    audio at their defaults), `line_of_options`, `item_finder` (item_of), `use_item_maker`
    (made once per line, checked by its space id). `sieve` and the hallway call it at start.
  - `sieve locate --items [--item-hashes]`; sieve-install prints its error with `--yes`.
  - Oracle: `unpack` reads v5, makes pages itself, `--items-from` for the rest, checks the tree.
- **Next for v5:** ranges; items of the other lines (their file code is in the client:
  item_save.cpp, to move into a shared library first); the File Locator making v5.


### figure-v1: shapes and figures (12 October 2026)
- **What** (Edward, IDEAS §19.1): the ink (every colour but the background) makes exactly
  `shapes` connected shapes, `connect` four or eight, `border` any, clear or standing.
- **Where:** `core/src/filters/figure.cpp`. `step` is the frontier; `figure_dfa2` builds and
  minimises the two-symbol automaton (cached for the process, as the menus ask often); `widen`
  reads it over every colour; `InkRanker` counts it on rgb24; `figure_passes` judges by flood fill.
- **Counts** at any height (no row counter); the width bounds it (the frontier grows with it).
- **Oracle:** `figure-vectors` (`tests/vectors_figure_v1.tsv`, about 4 minutes): brute force on
  small lines, a positional frontier walk at 10 by 10. `test_figure_vectors` in test_core.cpp.
- **Next:** an area setting (how much of the picture the ink covers, which costs a counter in the
  state), and a column-runs counterpart to row-runs for upright figures; then the diffusion model
  (IDEAS §19.2), trained on figures found this way.

### diffusion-v1: pictures made by undoing noise (12 October 2026)
- **What** (Edward, IDEAS §19.2; SPECIFICATIONS §12.0g): a denoiser of the AI line's shape, trained
  in training-v1's arithmetic; `sieve diffuse --record / RUN / --make`.
- **training.cpp:** `run_window` takes an `Example` (inputs, a target a position or -1, causal or
  not); windows of text are made into causal examples (`run_batch`), so the language model trains
  exactly as before (both shipped runs reproduce). `Trainer::step` and `evaluate` on examples;
  `Trainer::apply` is the update both use; `model_logits` for samplers.
- **diffusion.{hpp,cpp}:** `Setup`, `SplitMix`, `draw`, `make_example`, `step`, `make_picture`.
- **tools/cli/diffuse.cpp:** the run folder (diffusion.ini, pictures.txt, result.txt); pictures
  drawn from the image line's stack (two splitmix64 draws as a 128-bit number, mod the count).
- **Oracle:** `TrainerRef.example` (`window` calls it), `update`; `SplitMixRef`, `df_*`;
  `diffusion-vectors` (`tests/vectors_diffusion_v1.tsv`), `diffuse-replay --run`.
- **Measured** (IDEAS §19.2): settings that learn are rate 0.0001 and loss scale 2^4; 2 layers of
  width 32 make a figure (figure-v1) one time in ten after 15,000 steps.
- **Next:** larger and longer runs; the hallway (a model's picture on its face, J noise to
  picture); aispace-v2 naming the architecture; a shipped run.

### The metrics list (12 October 2026)
- **What** (Edward, IDEAS §19.12): every line's share removed, as the setup menu shows it, in a
  text file with each release.
- **Where:** `Menu::metrics_text` (menu.cpp) waits on each line's `stack_info` from the menu's
  workers and writes the list (binary first, then door order; names in title case, two letters or
  fewer kept as they are; tiny exponents to two significant figures), then each line's size.
  `hallway --metrics FILE [--filters PATH]` (app_main.cpp) runs it before SDL starts: no display.
  `parts_stack_info` now records `kept_log10` as the other tallies do.
- **Reference:** `data/metrics/reference-filters.ini`, the hard filters that count together, its
  SHA-256 in the list's header. make_release.py step 8b, by the build's hallway (the staged folder
  is left as checked); `metrics.txt` is published and in the notes.
- **Next:** the tally into tools/cli, for a `sieve metrics` (and a unit test of the list).

### Picture models in the AI Training Harness (12 October 2026)
- **What** (Edward): a small diffusion test beside the text runs in the harness.
- **tools/cli/diffusion_run.{hpp,cpp}** (in sieve_compare, beside train.cpp): `DiffusionRecipe`,
  `DiffusionRun`, `load_diffusion_run`, `record_diffusion_run`, `draw_pictures`,
  `train_diffusion_run` (progress and cancel), `final_address_of`. `sieve diffuse` now uses them;
  its runs reproduce as before (checked against an earlier run, and by the oracle).
- **client/harness.{hpp,cpp}:** `PictureSource` (the setup menu's image shape and the filters file,
  from app_main); picture runs listed (`Entry::pictures`); `open_pictures` and
  `change_picture_setting` (the settings page, `kPictureRows`); `start_picture_training` and
  `poll_pictures` (`djob_`); `make_pictures` (six from noise) and `render_pictures`. D opens it; a
  second new row in the list. A stopped picture run starts again (no saved state yet).
- **Not yet:** resuming a picture run; packing one as .sieve; the hallway showing a picture model's
  pictures on its shelf face.

### Menus in one order; Go to it from the main menu (12 October 2026)
- **Main menu** (main_menu.cpp): Start, File Locator, Media Player (new: `Result::Media`), Filter
  Designer, AI Training Harness, Settings, Exit. CI's "Down,Return" to Settings is now five Downs.
- **Pause menu** (pause_menu.cpp): adds Filter Designer and AI Training Harness
  (`Request::Designer`, `Request::Harness`, handled in app_main's in-game loop beside Settings;
  the harness's GO TO IT builds a hallway on the AI line, `harness_went`). The panel is as tall as
  its rows.
- **File Locator from the main menu** offers Go to it again: `run_locator(..., &go)` sets
  `Host::go` and `binary_bytes` (the setup menu's), app_main builds the hallway and calls
  `Hallway::go_to_located`. CI: "open, Go to it offered".
- **Media Player from the main menu:** the hallway is built with `open_media_from_main()`; Esc there
  is `Request::MainMenu` (`media_from_main_`), cleared once a melody is gone to.

### The shipped picture model (12 October 2026)
- `data/training/figures-2x32/` (diffusion.ini, pictures.txt 404 KB, result.txt): the run the
  harness screenshots and IDEAS §19.2 measure. Made with `sieve diffuse --record figs --filters
  figures.ini --ai-layers 2 --ai-width 32 --steps 15000 --checkpoint-every 1500` (figures.ini: the
  image line's figure-v1 connect = four, row-runs-v1 changes = 2). Reproduced exactly by the
  current build (7 minutes here). CI checks only that its final model makes the six pictures the
  harness shows, by address; training it again is by hand.

### The chat window (12 October 2026)
- **What** (Edward, IDEAS §19.13): a built-in window for talking to a model of the AI line, with a
  plus button for files and, multimodal, figures drawn with a picture model, so a text model and a
  picture model are one interface.
- **client/chat.cpp** (Hallway methods; state in hallway.hpp's chat section): Enter on a model in
  hand opens it (`open_chat`), replacing the old `SAY > ` prompt (`Input::Talk` is gone). The
  conversation is `chat_` (entries: you, it, a note, a file card, a picture card) beside the text
  transcript `talk_`, which the hand panel and `--talk` still print, and which seeds the replies as
  before (`talk()` moved here from hallway.cpp, unchanged in what it says).
  - **Files** (`chat_attach`; + or Ctrl+O, the system dialog, several at once): read and weighed on
    `busy()` by `cli::weigh_files` under `locator_host().lines()`, the File Locator's own lines;
    the vault refuses as everywhere. The model is given the first bytes, at most half its
    positions (`talk_give`: no reply; when they and room for a message and a reply do not fit, it
    starts again and says so). Card buttons: GO TO IT (`walk_to_file`), J (`open_as_kind`), SAVE.
  - **Figures** (`chat_draw`; DRAW or Ctrl+D): `cli::picture_models_in(cli::training_run_dirs())`
    lists the trained diffusion runs (the harness's folder, then data/training; Ctrl+P changes),
    `cli::load_picture_model` loads one, and figure k is `nth_picture(k)`. The model is given its
    rows (a character a cell). GO TO IT: `go_to_picture` when this hallway's image line has the
    picture's size and palette, else `Request::GoToPicture` (app_main builds a hallway with that
    image line and goes there). SAVE writes a PNG at one pixel a cell.
  - Keys: Ctrl+G, Ctrl+J and Ctrl+S press the newest card's buttons; Page Up and Page Down or the
    wheel scroll; Esc closes. The panel dims the hallway round it.
- **tools/cli/diffusion_run:** `nth_picture` (the k-th noise from splitmix64 seeded 1, seed k),
  `PictureModel`, `load_picture_model`, `picture_models_in`, `image_line_of`. The harness's
  `make_pictures` and `sieve diffuse --make` use them, so all three make the same figures.
  **tools/cli/train:** `training_run_dirs()` returns what `use_training_runs` was given.
- **Scripted runs:** `--chat` (open it), `--chat-attach FILE`, `--chat-draw N`, then `--talk`, the
  conversation printed; `--chat-press KEYS` after them. CI: a file, two figures (pictures 1 and 2,
  by address) and a message; Ctrl+G on a figure holds it on IMAGE; Ctrl+J on a text file opens it
  on PAGES.
- **Not yet:** the model naming places in its replies, turned into cards (needs a model that follows
  a format: a published one through `sieve chat`); published models in the window; a picture model
  in hand on the AI line (its shape differs from the line's).

### CHAT as a tab; notes 19.21 to 19.25 (12 October 2026)
- **What** (Edward): the chat as its own tab, so the item's other tabs stay available.
- **The tab:** `hand_tabs()` counts one more with a model of the AI line in hand, and CHAT is the
  last (`chat_tab()`). Enter, or C round to it, opens it (`open_chat`); while it shows, the keys are
  its box's (`chat_open_`); Esc goes back to ITEM (`close_chat`). `draw_chat(x, top, w, bottom)`
  draws in the hand panel's area. `chat_poll()`, every frame from `render()`, takes the dialogs'
  choices and gives the keys back if the tab is no longer showing. The compass is not drawn over
  the tab (it covered SEND).
- **IDEAS:** §19.21 conversations among the pages and books; §19.22 an unfoldable language
  network; §19.23 limits of reduction, measured and kept (a ledger, proposed as a round); §19.24
  the predictive side as addressing, and usefulness as a metric; §19.25 a model's probabilities
  over every line, and models together.

### Fixes after the CHAT tab (12 October 2026)
- **CHAT's buttons took the window's coordinates** for the renderer's. Where the two differ (a
  scaled display), a click could land on another button. Edward's GO TO IT on the Vitruvian Man
  card opened it with J instead: the 949×1300 JPG fitted to the 10×10 mono image line, which comes
  out white (reproduced here, the same tile). Clicks now go through
  `SDL_RenderCoordinatesFromWindow`, as the navigator, pause menu and media player do.
- **Shadowed names** (MSVC C4456 and C4457 in music.cpp, and the rest that GCC's -Wshadow=local
  finds) renamed.
- **The figures run's pictures are recorded, not hand-made:** drawn again from the image line's
  stack (figure-v1 connect four, row-runs-v1 changes 2, seed 42), they are pictures.txt byte for
  byte; CI now checks it.

### CHAT cards show what a file is (12 October 2026)
- **What** (Edward): a small thumbnail with a file's card, so it is clear what was given.
- **client/chat.cpp:** `chat_attach` works out, on the same `busy()` job as the weighing:
  - **a picture or video** (PNG, JPG, GIF, BMP; MP4, AVI and WEBP when ffmpeg is there): its first
    frame through `cli::read_media_frames`. The vault checks it (`withheld_picture`), as every
    picture shown, and it is kept at most 96 pixels on its longer side (`small_copy`, an average of
    the pixels each covers);
  - **a text:** its first four lines, as their own characters (`text_line`; a line that is not
    UTF-8, or is cut mid-character, falls back to `printable`);
  - **anything else:** its first 24 bytes in hex.

  The thumbnail's texture is made the first time the card is drawn (`chat_thumbs_`, through
  `gpu::create`, so it is counted) and let go with the conversation or the hallway's textures
  (`chat_clear_thumbs`). Pictures smaller than 96 pixels are drawn in sharp cells.
- **Scripted runs:** `--chat-attach` takes several files apart by ';'. CI attaches a JPG, a text cut
  mid-character, a text and a MIDI file.
- **IDEAS §19.26:** bigger models and ComfyUI, with a proposed order.
- **A slip of mine, caught here:** the client build directory had been broken by a second build
  directory sharing its SDL download, and the chat-fixes round's hallway CI lines ran on the hallway
  built before it. Built again and run again on this round's hallway: they pass.

### Models given as files (12 October 2026)
- **What** (Edward): a text or picture model plugged in as a file, as an option beside the AI
  line's shape in the setup menu.
- **tools/cli/model_folder.{hpp,cpp}** (sieve_compare): `is_model_folder` and `load_model_folder`
  (config.json, model.safetensors, tokenizer.json, as `sieve chat --model` reads them; `chatml`
  when the tokenizer has ChatML's tokens).
- **AppSettings `[models]`:** `text_model` and `picture_model` (paths), saved by the setup menu. An
  empty value with only a comment after it now reads as empty.
- **The setup menu** (menu.cpp): the AI section has six rows, `setup_rows_of(Media::Ai)` 6.
  - Row 5, CHAT's model: Enter or Right opens the system's folder dialog (`choose_model_folder`,
    handed over through `folder_picked_` and `poll_model_folder`); Left clears.
  - Row 6, DRAW's picture model: Left and Right go through the trained runs of
    `picture_models_in(training_run_dirs())`, stored as absolute paths; Enter chooses a folder.
  - Each row's value says when the folder is not a model folder.
- **The hallway:** `set_model_files` (app_main, after every build and after the in-game setup
  menu; `--text-model DIR` and `--picture-model DIR` in scripted runs).
  - **CHAT:** `talk()` goes to `talk_file()` while `talking_to_file()`. The model loads on first
    use (`busy` LOADING) and answers on `busy` THINKING: a ChatML model through `llm::Chat` (system
    "You are a helpful assistant."), any other by `continue_text` to a newline, at most 200 tokens,
    seeded from the conversation's SHA-256 as before. When the conversation would not fit its
    positions it starts again and says so.
  - **What it reads:** files and figures given to it wait in `file_context_` for the next message
    (`give_file_model`: a text's first 1,500 bytes, cut at a whole character; else a tag naming
    the file's kind and size). The cards say so (`with_next`).
  - **Ctrl+M** turns between the model file and the model in hand. Its replies are labelled
    "IT (folder)" and drawn as UTF-8 text.
  - **DRAW:** `chat_find_models` puts the setting's run first among the trained runs (or as the
    first, when it lies elsewhere) and chooses it.
- **CI:** the tiny model (from the oracle's step) answers through `--text-model`; DRAW with
  `--picture-model`; the setup menu's picture row saved in `[models]`.
- **IDEAS:** §19.27 files from known generators and their seeds; §19.28 a universe in a seed.
- **Not yet:** quantised weights (a model of billions of weights in f32 needs its size in memory
  several times over); a picture model other than diffusion-v1's runs (ComfyUI, IDEAS §19.26).

### A sound model: melodies (12 October 2026)
- **What** (Edward): an audio, track or "sound" model beside the text and picture models, without
  ComfyUI; commands such as a random track or picture, for the person first.
- **diffusion-v1 on the audio line** (tools/cli/diffusion_run, SPECIFICATIONS §12.0g):
  - `DiffusionRecipe` has `line`, `symbols` and `audio()`; a melody is a picture one cell high,
    `length` cells of the note set's base (notes104: 104 colours).
  - `pictures_text`/`parse_pictures` write a cell a base-36 digit, or numbers apart by spaces when
    there are more than 36 colours. `unit_space_of(recipe)` gives the run's space.
  - `sieve diffuse --line audio` (with the note set's options; `--levels` is the run's, not
    notes3's) records a run from the `[audio]` stack. `--make` writes each melody's notation and
    its address; `--judge FILTERS` counts what each filter keeps of the made units and of their
    noises.
  - The oracle's `df_replay` replays audio runs (notes104, or the notes2 set named in `symbols`).
- **data/training/melodies-2x64:** 2 layers, width 64, on the default `[audio]` stack's
  survivors. Measured (§12.0g): of 300 made, 300 in key, 300 end on the tonic, 3 to 5 pass the
  whole stack (noise: none); the rests rule is the one it keeps less often than noise.
- **The harness** (harness.cpp) lists melody runs and shows made melodies in notation.
- **CHAT** (chat.cpp): `ChatEntry` has `symbols` and `title`; `melody()` when symbols are set,
  `heading()` names it. A melody card draws a piano roll; `ChatAct::Play` plays it through
  `music()`; GO TO IT is `go_to_melody` (`Request::GoToMelody`, on which `handle()` also leaves
  CHAT); SAVE writes MIDI. `chat_show_unit` is shared by DRAW and the commands.
- **Commands** (`chat_command`, caught at the top of `talk()`, so `--talk "/random melody"` works):
  `/random picture|image|melody` draws `random_below(count)` and unranks it from the line's stack
  where it counts, else uniform cells; `/help`; anything else says it is unknown.
- **The shipped figures measurement** in §12.0g was 29 of 300; `--judge` gives 22, 24 and 27 for
  three noise seeds, and the text now says so.
- **CI:** a tiny melody run recorded, reproduced and replayed by the oracle; `--make` and
  `--judge` on it; the shipped melody run's first three addresses; CHAT's melody card, `/random`,
  and Ctrl+G onto the audio line.
- **IDEAS:** §19.29 reconstruction and probes; §19.30 flavours and categories of categories;
  §19.31 the sound model; §19.32 commands; §19.33 a voice from a model of the vocal tract.
- **Found: line art fits to white.** CHAT's J on Vitruvian.jpg gave an all-white picture on a
  10x10 mono image line: `canon-image-v1` working as specified, each cell's area average light
  parchment, nearest to white. Fixed by `canon-image-v2` (the entry below).
- **Not yet:** recording a melody run from the harness; a track model; a model of sound itself
  (pcm); commands for the models to use.

### Notes 19.34 to 19.39 (12 October 2026)
- **IDEAS only, no code:**
  - §19.34 the Encryption Engine (recipes as a line; what can and can't be told from noise);
  - §19.35 which rules have small automata, and a superset automaton with a judge where none does;
  - §19.36 superoptimisation: only so many ways to write code;
  - §19.37 the limit of rules, and constrained decoding across every line;
  - §19.38 a map of trained models, and models strung into pipelines;
  - §19.39 the release: paid as *State Space: Near Zero*, free to build as *The Sieve of Potentia*.
- **GAME.md** points to §19.39.
- **Who runs what:** above, under "Build, test, package".
- **Next, when chosen:** a `build_release.bat` that needs only Visual Studio (§19.39).

### canon-image-v2, and the hallway as a Windows program (12 October 2026)
- **What** (Edward): GO TO IT / J on Vitruvian.jpg still found a purely white picture; bring the
  window back after finding a file; no console window behind the hallway.
- **canon-image-v2** (core/image, SPECIFICATIONS §6.2), the default:
  - *The rule:* v1 plus a levels step. Unless every resampled pixel is already a palette colour,
    the luminance range between the twentieth darkest and twentieth lightest is stretched to
    0..255.
  - *Where it lives:* `ImageFormat::fit` (`Fit::Levels` by default, `Fit::Average` for v1) and
    `canon_version()`, which the reports print. It is not in the line's id, since it decides only
    where a file lands.
  - *Chosen by:* `sieve --fit levels|average` on the image and video lines, and the hallway's
    `--image-fit` and `--video-fit`.
  - *Exact palette pictures are left alone,* so saved items come back as themselves, and rgb24 is
    unchanged.
  - *Checked by:* the oracle's `levels_v2` and `image-v2-vectors` (`tests/vectors_image_v2.tsv`,
    32 cases, 17 different from v1); the unit tests (both vector files, and a direct ink-on-paper
    case); and CI (Vitruvian under both rules, an ega16 picture at three times the size coming
    back under both, the oracle's v2 vectors).
  - *Measured:* Vitruvian.jpg at 10x10 mono is a silhouette (v1: all white); at 64x64 the head,
    arms, legs and circle are there.
- **Dialogs:** `client/dialogs.hpp` wraps SDL's three dialogs. When one closes, its callback runs,
  then `SDL_RunOnMainThread` raises the window it belonged to. Every dialog in the client and the
  installer goes through it.
- **No console window:** `hallway` is built `WIN32`.
  - `attach_parent_console()` (client/win_console) attaches to a command prompt's console when
    started from one; it does nothing when the output is already redirected (CI, scripted runs).
  - Not tried on Windows here (no Windows machine). On Linux both changes compile to the same
    behaviour as before.

### The system prompt, FULL SIZE, an even CHAT row, and Build or Buy (13 October 2026)
- **The system prompt, `sieve-system-v1`** (docs/SYSTEM-PROMPT.md), a specification like the
  manifest's:
  - *The file:* `data/prompts/sieve-system-v1.txt`, 3,370 bytes, its SHA-256 pinned in chat.cpp
    and checked by the unit tests. It is never edited; a change is v2.
  - *What a model is given:* the text, then "# The place, now", in the format the spec gives (line
    and space id, ordering, filters, the model in hand, which model it is).
  - *When:* given to a ChatML model file when a conversation starts. A model with too few
    positions is given the one-line prompt, and CHAT says so; so is a missing file, and a changed
    file is used but noted.
  - *Seen with:* `/prompt` in CHAT and `hallway --print-system-prompt`. CI compares the printed
    text with the file, byte for byte.
  - *Shipped:* the build copies `data/prompts` next to the hallway, and make_release's FOLDERS
    include it.
  - *Found here:* the tiny model's conversation broke with the full prompt (too few positions),
    and stayed silent about it. That is now the one-line fallback, with a note.
  - `Hallway::space_id()` is the HUD's space id, now shared with the HUD and COST.
- **FULL SIZE** (`ChatAct::Full`, F on the newest card), on picture file cards (PNG, JPG, GIF,
  BMP):
  - *What it does:* decodes the first frame and fits it to an rgb24 line of its own size, where
    every pixel is exact (`canonicalise_frame`). It then goes there directly, or through
    `Request::GoToPicture` as DRAW's GO TO IT does.
  - *Measured:* Vitruvian.jpg is a 949x1300 rgb24 line, 1,233,700 positions. A room of such
    pictures took about a minute here (1,024 items at 57 ms, half of it the vault), on workers.
  - *Not tried here:* the scripted run follows no requests, so the rebuild path is checked only
    by its being DRAW's.
- **CHAT's row:** the box is at least a button's height, and +, DRAW and SEND share its bottom
  edge. A one-line box is exactly their height; the + is two filled bars.
- **The web page:** `Pages/release.html` (Build or Buy), linked from `index.html`. It has the
  PayPal link Edward gave, the three steps, and exact BigInt counters. Checked in headless
  Chromium at 1280 and 520 pixels wide (headless Chromium lays out no narrower than 500).
- **Testing on Edward's side:** `tools/check_vectors.py` and `tools/run_tests.bat` (above, under
  "Build, test, package").
- **IDEAS:** §19.40 a file, its full size and its variants; §19.41 other developers' oddities;
  §19.42 the web page; §19.43 all the AIs folding in on themselves; §19.44 keeping the interface
  even.
- **GAME.md:** the one-sentence description, and the philosophy as the lore.

### CHAIN, and the notes to wrap it all up (13 October 2026)
- **CHAIN** (client/chain.cpp, SPECIFICATIONS §12.0h, IDEAS §19.46):
  - *The tab:* `chain_tab()` is `hand_tabs() - 2`, so C reaches it before CHAT takes the keys.
    `hand_tabs()` adds two for a model of the AI line.
  - *Its keys:* `chain_key()` takes them in `handle_event` before walking's.
  - *Running:* `/chain WORDS` is `chain_run()`.
    - Text stages use a fresh model each time (the model in hand's, or a run's via
      `model_run_of`, cached), or the model file through `file_reply()`.
    - Picture and melody stages draw their noise from splitmix64 seeded by the stage's SHA-256
      seed and make it with `diffusion::make_picture`.
    - *Found here:* drawing the k-th noise for a large k was far too slow, since it walks every
      noise before it.
  - *The file:* `sieve-chain-v1`, saved and opened with dialogs. Scripted runs use `--chain`,
    `--chain-file`, `--chain-press` and `--chain-save`.
  - *Refactors:* `talk_file` is now `load_file_model()` plus `file_reply()`; `unit_rows()` is
    shared; `chat_show_unit()` returns false when the vault withholds.
  - *CI:* the tab edited by keys and saved; the four-stage chain run (each stage checked to have
    run); a picture-and-melody chain checked address for address.
- **IDEAS §19.45 to §19.53:**
  - stories in the noise, and one equation;
  - CHAIN;
  - an engine that could make every game;
  - the documentation inside the game;
  - the completion line on the main menu, kept in sync with the website;
  - metrics for every build, charted;
  - a decompilation engine;
  - the Encryption Engine as format-preserving encryption;
  - the plan in order of importance (§19.53).
- **GAME.md:** stories in the noise, and the engine that could make every game.
- **Next:** the Encryption Engine, stage 1 (§19.52, first in §19.53).

### Voices, disclosure, what the numbers mean, and notes 19.54 to 19.66 (13 October 2026)
- **`docs/VOICES.md`:** the corpus of attributed voices for the halls, one a line (`- **WHO** (DATE,
  KIND): TEXT`).
  - *Who is in it:* Edward's, Gemini's and Claude's lines so far.
  - *Gemini's two messages:* kept whole, with notes on their figures. Several were not exact: a
    day of pages is 1.36 × 10^-39 %, the printed stack is 7.2 × 10^14 universe widths, and chains
    are not lossless.
  - *Claude's version* is left out of the copy, as everywhere in the repository.
- **The website:**
  - *Disclosure:* an AI disclosure on every page, fuller on Build or Buy. Edward's own multi-year
    theory, Gemini's help with concepts and the website, and Claude's with the code and
    documentation; what the game's own models are trained on.
  - *Build or Buy:* gains "What The Numbers Mean" (a day; everyone at once; the pages printed;
    gained and lost; the audio page's tracks from the same epoch) and "Running Out Of Original
    Things To Say" (3.5 × 10^-64 %).
  - *How it stays exact:* every constant is named in the script, with its source.
- **IDEAS §19.54 to §19.66:**
  - mapping the madness;
  - the voices of the halls;
  - a chain as a state space, and steering it;
  - Lean as a filter;
  - reading every document (PDF, OCR, layout);
  - a corpus of every language;
  - bodies, light and scale (skeletons, impostors, levels of detail);
  - what the numbers mean;
  - the Universal Exchange Rate in bits;
  - a currency for every person, identity, a wallet and shared computing;
  - the originality counter;
  - unfold or collapse;
  - disclosure everywhere.
- **No code changed this round.**

### Folding everything, and the disclosure first (13 October 2026)
- **Website:**
  - *Disclosure first:* the AI disclosure is at the top of every page. On Build or Buy it comes
    first and the buy and download buttons last (Edward: it always has to be front and centre).
  - *"Folding Everything" on Build or Buy:*
    - 10^9868 bits written out against one line folded;
    - Lloyd's 10^90 bits and 10^120 operations;
    - 1.5 × 10^89 background photons;
    - the 4.6% reachable share and the stars crossing out of reach this year, with the Orbital
      Simulation page's constants and formula, so the pages agree.
- **IDEAS §19.67** folding everything. **VOICES.md:** "There are so many ways to map the data.
  We'll get there." (Edward), and two notes of Claude's.
- **No code changed.**

### The Encryption Engine, stage 1, and the voices on the website (13 October 2026)
- **`cipher-v1`** (SPECIFICATIONS §12.0i; `core/include/sieve/cipher.hpp`, `core/src/cipher.cpp`):
  - recipes of `reverse`, `caesar`, `xor`, `vigenere` and `feistel` layers, the last being the
    binary line's keyed scramble (format-preserving);
  - `break_weak`, which ranks every caesar shift, one-byte XOR and column-fitted repeating key by
    English score less 1500 for each key byte beyond the first.
- **`sieve cipher`** (`tools/cli/cipher_cmd.cpp`): `--encrypt`, `--decrypt`, `--break` and
  `--catalogue`. Its input option is `--in-hex`, since `--hex` was already a switch.
- **Oracle:** `sieve_ref.py cipher-vectors` writes `tests/vectors_cipher_v1.tsv` (55 encryptions,
  15 guesses); `test_core` checks them, with round trips and refused recipes. 76,549 checks, 0
  failures. CI diffs the oracle and runs the CLI there and back, a broken XOR, an unbroken feistel
  and the catalogue. `tools/check_vectors.py` includes it.
- **Website:** a random voice from VOICES.md at the very bottom of Build or Buy, under the
  buttons, with a "Read aloud" button (IDEAS §19.68).
- **Next:** the Encryption Engine screen in the game.

### Build or Buy, every build on the record, and how rare a voice is (13 October 2026)
- **Build or Buy:**
  - the page is "Build or Buy" everywhere (title, the index's link, the docs);
  - its buttons run Build, Download, then Buy, at the bottom;
  - the line at the top says the game is the same either way and that the page will not try to
    sell it (IDEAS §19.69).
- **Metrics for every build** (IDEAS §19.50, §19.69):
  - `tools/log_metrics.py` writes `metrics/builds/<UTC time>.txt` (`sieve-metrics-v1`) and
    rewrites `metrics/index.tsv` from every record;
  - `run_tests.bat` keeps each unit run's output and runs the logger once everything has passed,
    with the hallway's metrics list under the reference filters when a hallway is built;
  - the website's "Every Build, On The Record" reads the index: the time since the last build, the
    latest figures and a chart for each measure.
  - No record is committed yet: the first will be the first full Windows run's.
- **Voices:**
  - three more of Edward's in VOICES.md;
  - under the website's voice, its odds by the length of the list, and how many of them this
    visitor has met (IDEAS §19.70; a hall of voices in the game is noted there).

### The characters model, every release on the record, Specs vs Stats (13 October 2026)
- **`data/training/type-2x32`:** the characters model, the fourth entry in the model list
  (IDEAS §19.72, §19.73). Its pictures are written by `tools/build_character_pictures.py`. The
  list is alphabetical (`picture_models_in`), so naming it after `melodies` keeps
  `figures-2x32` the default and leaves the menu's CI steps as they were.
- **`make_release` step 8c:** runs and times the build's unit suite, then `log_metrics.py`
  writes a `release` record with `unit_seconds` and the hallway's metrics list. `--no-record`
  skips it.
- **`log_metrics.py`:**
  - `--set KEY=VALUE`;
  - the machine as read from the system (`cpu`, `logical_processors`, `memory_gb`, `gpu` on
    Windows), never its name;
  - `machine_profile` when `metrics/machines/*.txt` (`sieve-machine-v1`) has the same `cpu`;
  - empty values left out of a record.
- **`metrics/machines/edward-1.txt`:** Edward's machine in his words, listed in `list.txt`.
- **`Pages/specs.html`, Specs vs Stats** (linked from Build or Buy's builds section):
  - machines, the leaderboard by checks a second, and machines working together;
  - Landauer limits for Earth's sunlight, and a Dyson sphere in a category of its own;
  - how to add a machine.
- **VOICES.md:**
  - three more of Edward's, two marked "a point about everything" (a new marker, explained in the
    file's format);
  - Claude's answer to "can we fold all video games into one?".
- **IDEAS:** §19.71 to §19.75 (accessibility, with the quick-time-event grid noted and not wanted).

### Notes: characters and figures as filters and keys, and contributors' voices (13 October 2026)
- **IDEAS:**
  - §19.76: characters and figures as filters (a `glyph-v1` filter, cross-line pages) and as
    keys (a picture's 100 cells as a cipher key, a model and noise as a key);
  - §19.77: contributors' voices, unlimited within reason, a respectful amount asked, nothing
    guaranteed. The same words are in VOICES.md.
- **VOICES.md:** "This, models across all dimensions?" (Edward).
- **No code changed.**

### The Encryption Engine's screen, a quick start (13 October 2026)
- **`client/cipher_screen.{hpp,cpp}`:** class `CipherScreen`.
  - Rows: the input (text or hex), INPUT IS, the recipe, and DO (encrypt, decrypt or break).
  - The output is worked out again on every change.
  - Tab puts the output in as the input and flips encrypt and decrypt; Ctrl+V pastes, Ctrl+C
    copies the hex.
  - Strings: `cipher.*` in `data/lang/en.txt`.
- **Menus:**
  - main menu `cipher` (`MainMenu::Result::Cipher`) and pause menu `cipher`
    (`Hallway::Request::Cipher`), both placed after Settings so every CI step that counts Downs to
    an existing row still lands on it;
  - app_main runs the screen and comes back to the menu, or to the paused hallway.
- **Scripting:** `hallway --cipher --script "Down,=text,Return" --screenshot F` prints
  `cipher: <status>`, the output lines and `output: <hex>`. CI checks the opening recipe there
  and back, an XOR broken, and the main menu's new row.
- **Built and checked here** with the client (SDL_VIDEO_DRIVER=offscreen).

### Every figure with its inverse (13 October 2026)
- **Build or Buy:** each counter now shows what is left beside what is done (the pictures, pages
  and models still to go, the sentences still unsaid, the rule's share of the bits written out).
- **The Orbital Simulation:** "% Still Reachable" under "% Unreachable".
- **IDEAS §19.78:** the rule, what has an inverse and what has none (the audio page's tracks, the
  time since a build), and why.
- **Widths at 390 pixels, already like this before:** the space page is 804 pixels wide and Build
  or Buy 394. Both are older than this change, and left as they were.

### Metrics Over Time (13 October 2026)
- **`Pages/metrics.html`:** a line chart a measure from `Sieve/metrics/index.tsv` (the local copy,
  else GitHub's raw copy).
  - Filters: machine (`edward-1` by default) and releases or every build.
  - Hover gives the nearest build; each chart has a table view.
  - Charts are drawn at their container's width, so the text stays 11 pixels on a phone.
- **Its button:** on Build or Buy, between "Every Build, On The Record" and "Build It Or Buy It".
- **IDEAS §19.50** records it as built.

### A once-over across Sieve, Potentia, the website and the docs (13 October 2026)
- **Healthy:**
  - the engine, tests and client build with no warnings, and the unit suite passes (76,549
    checks, 0 failures, here);
  - every Python tool compiles, and the CI file parses;
  - every string the client asks for is in `en.txt`;
  - every page of the site loads with its AI disclosure, no script errors and no broken local
    links.
- **Fixed:**
  - *Phone width:* the front page and the audio, sieve, quest and space pages pushed sideways on a
    390-pixel screen, because titles and footers could not wrap (the space page reached 804
    pixels). Build or Buy was 4 pixels over, from a long run of zeros. Each page now has the
    phone-only wrap rule the newer pages had; desktop layout is unchanged.
  - *Duplicate idea number:* IDEAS had two entries numbered 5.12. The address navigator is now
    5.11a; the one reference to §5.12 means the junction, which keeps it.
  - *The corpus manifest:* `.gitignore`'s `corpus/` also hid
    `data/models/corpus/gutenberg-nltk.tsv`, the manifest `tools/fetch_corpus.py` checks the
    download against, so it has never been committed. It is now excepted. If Edward's copy
    exists, it shows up to commit; if not, it has to be made again from the 15 NLTK books the
    pinned model was trained on.
- **Found, not changed:**
  - *The CI never runs on GitHub.* `build.yml` is at `Sieve/.github/workflows/`, but GitHub runs
    only `.github/workflows/` at the repository's root; its only runs are the website's Pages
    builds. Its checks have been run by hand, line by line, in these sessions. Moving it to the
    root needs every step to run in `Sieve/` (a `defaults: run: working-directory: Sieve`), and
    then a first run to find what has drifted.
  - *Model names:* "Claude Opus 5.5" appears in Edward's own acknowledgements (both READMEs,
    GAME.md, the anchors' copy, `Art/credit.txt`), as he asked; kept as written.
  - *`favicon.ico`:* the site has none, so browsers log one harmless 404 on the front page.

### Notes from playing it, and the ideas after (17 October 2026)
- **IDEAS §19.79, things that get in the way:**
  - ffmpeg warnings in red, with where it looked; a Settings row to choose ffmpeg;
  - the Encryption Engine taking the item in hand;
  - controls flowing between windows;
  - the window still not returning to the front after a file is opened;
  - CHAT's tabs answering the mouse;
  - saving chat logs and editing the context window;
  - a help tab;
  - bumping the version (still 0.13.1).
- **IDEAS §19.80, one less if:** a tab with a use belongs on every line; CHAT on every line, about
  the item in hand.
- **IDEAS §19.81:** folding as a second temperature, the models' own maps, models as filters,
  pictures boiled down to titles.
- **IDEAS §19.82:** the Encryption Engine over every file (`.sieve` recipes, keys split by secret
  sharing), and what filtering encrypted files out can and cannot honestly do.
- **IDEAS §19.83:** one model at the centre of every model.
- **IDEAS §19.84:** a website gallery, and why it needs `gallery.json`. `Images/Sieve` has no
  top-level images now.
- **IDEAS §19.85:** a skeleton system beside the UV system.
- **VOICES.md:** three of Edward's (two rules, one quote).
- **No code changed.**

### Next: `sieve-hallway.ini` written with every entry it can hold (17 October 2026)
This is the next task to tackle, ahead of the rest of §19.79. Edward asked for it so that a player
can see every setting in one file and set it there.
- **What happens now:** `AppSettings::save` (`client/app_settings.cpp`) writes `[graphics]`,
  `[world]`, `[controls]`, `[models]` and `[language]`. The file is written on the first start
  (when it has no resolution) and whenever Settings or the setup menu changes something
  (`menu.cpp`, the `app_->save` call). So an older file that already has a resolution does not gain
  new keys until something is changed in a menu.
- **What is missing:** the settings that come only from the command line (`Settings::from_args` in
  `client/menu.cpp`), among them `ffmpeg`, `video-fps`, the AI line's shape, `items-per-wall`,
  `title-length` and `binary-length`. None of these can be set in the file. Check which of them
  the setup menu already saves elsewhere (a setup's `.sieve`, `sieve-filters.ini`) before moving
  any, so that each setting has one home.
- **What to build:**
  - Every entry the file can hold is written, each with its value and a one-line comment. An empty
    one is written empty, never left out.
  - The file is rewritten at start when a key is missing, so that an older file gains new keys
    (it should keep the values it has).
  - A `[paths]` section with `ffmpeg =` (the program itself). `AppSettings::load` reads it and the
    hallway passes it to `set_ffmpeg_path`; `--ffmpeg` on the command line still wins.
  - A unit or CI check that a file saved from the defaults, loaded again and saved again is the
    same, byte for byte.
- **Correction:** I told Edward on 17 October that `ffmpeg = C:/ffmpeg/bin/ffmpeg.exe` in
  `sieve-hallway.ini` would work. It does not: nothing reads that key yet. What works today is
  `--ffmpeg PATH`, the `SIEVE_FFMPEG` environment variable, `ffmpeg.exe` beside the program, or
  `C:\ffmpeg\bin` on `PATH`. IDEAS §19.79 and the README said the same wrong thing and are
  corrected.

### `sieve-hallway.ini` holds every setting (17 October 2026)
The task above, built. The handoff asked to check first where the setup menu's settings were
already saved: nowhere. `sieve-filters.ini` holds only the filters, and the `.sieve` files are
items, favourites and training runs, so every line's shape started from the defaults (or the
command line) on every start. `sieve-hallway.ini` is now their one home.
- **`[setup]`:** every setting of the setup menu, all 54 of them (`client/setup.hpp`, the
  `Settings` struct, moved out of `menu.hpp` so the settings file can hold one). The keys are the
  hallway's option names with `_` for `-` (`items_per_wall`, `ai_width`, `binary_length`), in the
  menu's order, GLOBAL first. Their table, with the comments, is `kSetupKeys` in `menu.cpp`, beside
  `from_args` and `apply`; it was checked against `apply` by hand here, and a setting added to
  `Settings` needs a row there too. `model` is written `default` or `none`.
- **`[paths]`:** `ffmpeg =`, the program itself. The hallway passes it to `set_ffmpeg_path`;
  `--ffmpeg` still wins. What Edward was told on 17 October (above) is now true.
- **Every entry is written,** each with its value and a comment in one column, and an empty one
  is written empty (`AppSettings::rows`, `text`). The [graphics] entries that had no comment
  have one.
- **An older file is rewritten at start** when it lacks an entry (`AppSettings::complete`),
  keeping its values. Not in scripted pictures, which never write the file unless given one.
- **Reading `[setup]`:** each entry goes through `from_args`, so a number is checked as the option
  is. A word is checked against what the lines can be made with (`named_parts_ok`: the alphabet,
  the palettes, the note names, the modes, the fits). A value that fails leaves the setting as it
  was, so a mistyped line cannot stop the hallway from starting.
- **When `[setup]` is saved:** whenever the setup menu leaves with its settings changed (into the
  hallway, back to the main menu, or quitting), and from F1 in the hallway on ENTER. It is saved
  as the menu had it, options and all, so `--length 64` given once and then a change in the menu
  keeps 64. A scripted `--menu` picture saves only into a `--settings` file.
- **Runs without the menu:** `--no-menu` now builds from `[setup]` as well, when it says anything
  the defaults do not. When it says nothing new, the run is built from its options alone, exactly
  as before, so the CI's pictures are unchanged.
- **`--write-settings OUT`:** writes the file (from `--settings PATH`, else the defaults) without
  a window and stops. CI writes it from the defaults, again from that file, and compares the two
  byte for byte. It also checks that every entry has a comment, that an older file gains its
  missing entries and keeps its values, and that the setup menu's first row lands in `[setup]`.
  One older CI line, which failed if the Graphics-written file contained "binary" at all, now
  looks only for a binary-edge setting, since the file holds `binary_length`.
- **Checked here:** every client file that includes the changed headers compiles without a
  warning (`-Wall -Wextra -Wpedantic`, against SDL 3.2.30's headers). A scratch program built on
  the real `app_settings.cpp`, `from_args`, `apply` and `[setup]` code, with only `sieve_core`
  built, passed all ten of its checks: the defaults written, loaded and written again are the same
  byte for byte, and an older file keeps its values, gains its missing entries, leaves bad values
  at their defaults and reads `ffmpeg`. Not run here: the full hallway build and the new CI lines,
  which need the hallway itself.
- **Docs:** README (the settings paragraph and where ffmpeg is looked for), SPECIFICATIONS §5.3.1,
  IDEAS §19.79.

### 0.13.25, and most of what gets in the way (17 October 2026)
IDEAS §19.79, in the order of the plan agreed with Edward.
- **The version is 0.13.25** (Edward's choice; `CMakeLists.txt`, `vcpkg.json`, and the release
  tool's examples). The READMEs' "released (v0.13.1)" stays until a release is made.
- **Settings > Programs** (`client/main_menu.cpp`, a fourth Settings page after Language, so
  the CI's key sequences keep their rows): one row, ffmpeg. It shows the version found, or "not
  found" in red with where it looked under it; Enter chooses the program with the system's file
  window, Left clears it. It saves `[paths] ffmpeg` and calls `set_ffmpeg_path` at once, so it
  wins over `--ffmpeg` for the rest of the run, being chosen later. Worked out when the page
  opens and when it changes, not every frame, since finding ffmpeg looks through the PATH.
- **Where ffmpeg was looked for** (`cli::ffmpeg_looked`, `tools/cli/media_decode.cpp`):
  `SIEVE_FFMPEG`'s value or "not set", the folder beside Sieve, and how many folders the PATH
  has. Every "not found" message now says it, and names `[paths]` and Settings > Programs too.
- **Red warnings** (`Hallway::warn`): the messages that say something was stopped (a warp, a
  go-to, a save, J, a chain or anchor that would not open) are red on a dark plate, as
  [Null Title] is, so they read on the red lines too, and stay 10 seconds rather than 6. A video
  or WebP with no ffmpeg now says that, and where it looked, instead of "no line holds that kind".
- **The tabs answer the mouse** (`click_hand_tab`, `select_hand_tab`): a click on any tab of
  the item page goes to it, with the mouse free, in CHAT too. C and a click go through the same
  function, so CHAT opens and closes the same way either way.
- **HELP on every item** (`draw_help`, hud.cpp; `hand.help.*` in en.txt): after ITEM, COST,
  SORT and META, and before CHAIN and CHAT so that C reaches it before CHAT takes the keys. Its
  rows are the keys that work on what is in hand, chosen by its line (pages to turn, a melody to
  play, a model to turn or talk to, the binary line's J), read from the key handler, not guessed.
  The tab count moved up by one (`hand_tabs`); no CI sequence presses C far enough to notice.
- **Chat logs** (`chat_log_text`, `chat_save_log`, chat.cpp): Ctrl+L or `/log [PATH]`. The
  version and the time, the line and the model's whole address, then who said what and each card
  as a line (a file's size, kind and weighing; a picture's or melody's address). Without a path,
  `chat-logs/chat-YYYY-MM-DD-HHMMSS.txt` beside the program.
- **The window raise on Windows** (`client/window_front.cpp`, in both programs): from the
  dialog's callback, `AttachThreadInput` to the foreground's thread, `BringWindowToTop`,
  `SetForegroundWindow`, then SDL's raise as before. Not compiled here (no Windows headers in
  this container); Edward's build compiled it, and the window now comes back to the front.
- **CI added:** the log (its header, the model's address, a file card, the command itself), and
  a picture of HELP.
- **Checked here:** every changed file compiles warning-free against SDL's headers, but for the
  Windows half of `window_front.cpp`. Not run here: the hallway build, the unit suite, CI.
- **Tested by Edward (17 October 2026):** Debug and Release built, the tests run, a release build
  made as usual, and each change tried in the program: all of it works, the window raise too,
  and the settings work of the entry before this one.
- **Left of §19.79:** editing the context window, controls that flow between windows (wants a
  short design first: which windows, and what "the item and the keys" mean in each), and the
  Encryption Engine's item-in-hand button (stage 2).

### The rest of §19.79: the engine takes items and files, and the context window (17 October 2026)
- **Edward's calls first:**
  - *Controls between windows* is mostly the ethos now: most things hand off to most things
    already, the Encryption Engine being the exception (fixed below). He will keep an eye out.
  - *CHAT on every line* (§19.80) is cancelled for now: he is not sure it fits. IDEAS §17.2
    records it; the rest of §19.80 stands.
- **The Encryption Engine with files and items** (`client/cipher_screen.*`):
  - **I** on the item page opens the engine with the item in hand as its input, its bytes as F
    would save them (`item_file`, or the file's own on the binary line). It has to be a key of
    the page: Esc puts the item down before the pause menu opens. The vault withholds here as
    everywhere.
  - The INPUT row names the file and shows its first 24 bytes; typing there, or Backspace, goes
    back to typed input; Left/Right on INPUT IS too. Ctrl+I takes the item again, Ctrl+O takes a
    file. TAB puts an output of more than 4,096 bytes in as bytes rather than as hex.
  - Ctrl+S saves the output; **GO TO IT** (Ctrl+G) puts you on the binary line in front of it
    (`Hallway::go_to_bytes`; from the main menu, through the same `pending_file` as the File
    Locator's Go to it). A row of four buttons does the same with the mouse.
  - BREAK reads at most the first 64 KB of a long input, and says so.
  - Scripted: `--cipher-in FILE`, `--cipher-out FILE`. CI checks that the engine's output on a
    file is the same bytes as `sieve cipher --in --out` makes of it.
- **The context window** (`chat.cpp`): `/context` shows what the model is given; **Ctrl+E**
  puts it in the box to edit; Enter gives it, Esc or Ctrl+E leaves it as it was.
  - *The model in hand:* `talk_given_` holds every byte it has been fed since it started, kept to
    its position after each reply (the last byte written at the length limit is never fed). A
    byte that is not text shows as `\xNN` and a backslash as `\x5c`, so the edit is exact
    (checked here over every byte value). Enter starts the model again and feeds it exactly the
    box, cut to leave room for a reply.
  - *A model file:* its system prompt (`file_system_`); Enter starts the conversation again with
    it. A model changed in the setup menu forgets it.
  - The box only types at its end, so Ctrl+C copies it to edit elsewhere, Ctrl+V pastes it back,
    and Ctrl+Backspace clears it.
- **HELP** has a row for I.
- **Checked here:** every changed file compiles warning-free against SDL's headers. Not run here:
  the build, the unit suite, CI. No CI line drives Ctrl+E yet: it needs keys pressed after
  `--talk`, which the scripting does not order.

### I opens the engine; the AI line's figure; larger models evaluated (17 October 2026)
- **Edward's test of the last batch:** Debug, Release, the tests and the release package all
  pass, and the context window works. One fault: I with an item in hand did not open the engine.
- **Fixed:** the I key set the request but not `quit`, and the hallway's frame loop hands over
  to another screen only when the key handler sets both (as the pause menu's rows do). It sets
  `quit` now.
- **The AI line's figure** (Edward asked whether 2^32768 was right, expecting a parameter in it):
  it is right. 8,192 weights at the defaults, the embedding's 4,096 and the one layer's 4,096, at 4
  bits each. The setup menu (and so `--metrics`) now writes it `16^8192 = ~10^9864.2`,
  values^positions as the other lines are written. The `sieve ai` tool's own `2^32768` line is
  unchanged (CI checks it).
- **Larger models, evaluated** (IDEAS §19.86): twelve published models, read from their
  `config.json` and safetensors headers by range request (no weights downloaded): SmolLM2 three
  sizes, TinyLlama, Llama 3.2 1B, Qwen2.5 two sizes, Phi-3-mini, Gemma 2 2B, Mistral 7B, GPT-2 and
  Mamba. What each needs beyond the engine, and a plan in six stages: inspect without loading,
  memory-map rather than copy, families as versioned data, GGUF's quantised blocks read in place,
  tokenizers, and other families only when wanted. It also says what plugging a published model
  into the AI line would honestly mean (an `aispace-v2` whose digits are the stored values).
- **Proposed next:** stages 1 and 2 with Llama 3's rope scaling, Qwen2's biases and Phi-3's fused
  tensors. Waiting for Edward's word.
- **Checked here:** the two changed client files compile warning-free. Not run here: the build,
  the unit suite, CI.

### F16 kept as stored, widening a setting; the AI figure with its parameters (17 October 2026)
- **Edward's test:** built, tested and the repository updated with the last batch; the AI line's
  figure still read `16^8192` in the release's metrics, without its parameters showing.
- **The AI figure** now reads `(2^B)^(256H+16LH^2), B=4 L=1 H=16: 16^8192 = ~10^9864.2`: the
  formula, the parameters it takes, and the count. The map shows the part before " = ".
- **F16 weights** (Edward: do they have to be widened?): no. `llm.cpp`'s `Matrix` keeps F16 as
  stored in `f16`, turned into floats as it is used through a table of all 65,536 values
  (`half_table`, exact), as BF16 already was through a shift. **`widen_weights`** (core
  `llm::set_widen_weights`) widens both to F32 as they load instead: `[models] widen_weights` in
  `sieve-hallway.ini`, `--widen-weights` on the hallway and `sieve chat`. Off by default.
  - *Unit tests:* the stand-in model, BF16 widened against stored, and an F16 copy of it stored
    against widened: the same logits to the bit, and the widened weights exactly twice the bytes.
    153,106 checks, 0 failures, here.
  - *Measured here* (one thread, a model of 27.4 million weights): stored 54.9 MB, F16 64 tokens a
    second, BF16 48; widened 109.7 MB, F16 78 to 94, BF16 65 to 85. Memory against speed, the
    numbers unchanged.
- **Packages** now have `Sieve/` at the top and `CHANGES.md` beside it, as Edward asked.
- **Checked here:** the unit suite (core build), and every changed client and tool file compiles
  warning-free. Not run here: the hallway build, CI.

### What each ordering saves, in the metrics list (17 October 2026)
- **Edward's test of the last batch:** Debug, Release, the tests and the release package, and
  the repository updated: all good.
- **His request:** a metric of the bit reduction, at the least, each ordering can be expected to
  give. The honest answer is in the list's new last part, "Orderings" (`Menu::ordering_metrics`):
  - *Positional and scrambled:* 0 bits for every unit on every line (each a one-to-one numbering of
    the whole line; §0's counting argument).
  - *Compact:* exactly log2(line / survivors) bits off every survivor, from each line's count: the
    one guaranteed saving.
  - *Guided* (pages): no minimum. The bound over every page, from the model's tables
    (`CharModel::frequency_range`, new: the least and most any symbol has in any table, 1 and
    65,510 for the pinned model, which a unit test checks): 0.0 to 514.0 bits at 32 characters,
    against 152.2 positional. And measured on a text the model was not trained on,
    `data/metrics/reference-text.txt` (the opening of *A Tale of Two Cities*, copied from
    `tests/example_book_source.txt`; Dickens is not among the 15 training books): 29 pages,
    63.2 bits a page on average, 88.9 shorter, 1.98 bits a character against 4.75. The oracle
    (`guided_code`, its own tables) gives the same figures, and a scratch program on the core
    gives them too.
- **`--metrics-text FILE`** names the text; by default it is found in `data/metrics`, beside the
  program, or in the source tree. `make_release.py` and `log_metrics.py` pass it, as they pass the
  reference filters. CI checks the new lines' shape.
- **Checked here:** the unit suite, 153,108 checks, 0 failures; the changed client file compiles
  warning-free. Not run here: the hallway build, CI.

### The build error fixed; a full build here; the trained model on the record (17 October 2026)
- **Edward's build failed:** `'executable_dir': is not a member of 'sieve::cli'` in `app_main.cpp`
  (the `--metrics-text` lookup). It needed `cli/dictionaries.hpp`. The last batch was checked by
  compiling the changed files, and `app_main.cpp` was changed after that check. So this time
  **the whole hallway was built here** (Release, SDL3 from source, no warnings) and run.
- **Run on that build:** every CI line added this session (the settings file's round trip and
  older file, the setup menu's `[setup]`, Graphics' file, the engine on a file against
  `sieve cipher`, HELP, the chat log, the metrics list and its new parts): all pass.
- **The trained model, on the record** (Edward: "an insane reduction"; the aim is to reduce files
  to their limit, and to keep the figures unbiased: both in VOICES.md). The metrics list's last
  part, `Menu::record_metrics`, weighs every trained run of the AI line in the training folders,
  live, on every build:
  - universalbibliothek: address 32,768 bits; `model.safetensors` 272,576; deflate 58,256; LZMA2
    44,608. The address is 12.0% of the file and 73.5% of LZMA2's.
  - A model drawn at random from the same line (mt19937, seed 1): address 32,768; file 272,576;
    deflate 59,600; LZMA2 45,056. So the address's advantage is knowing the line's shape (the
    file keeps 4-bit weights as 32-bit floats), not the training. Training is what makes this
    model one worth naming, and the compressors find a little more in it.
  - The line's id, which the reader must be given (360 bits), is shown with it.
  - CI pins the address and file figures; the compressors' depend on the zlib and liblzma linked.
- **Checked here:** the full build; the unit suite as before; the CI lines above.

### `sieve-manifest-v6`, minimum viable places; the search's time (17 October 2026)
- **Asked:** the manifest to use the minimum viable address; and in the File Locator, how long to
  search for a shorter address for each file and for the whole run, each moving the other.
- **`sieve-manifest-v6`** (`--items` now; `--items --v5` the older one; SPECIFICATIONS §12.2):
  - Each item's place as raw bytes, the fewest its line's places need (`line K <width> ...`); the
    way is `item K` and the places are a third stream, `stream items`, carried raw or as LZMA2,
    whichever is smaller.
  - Per line, its items are kept as places only where that is smaller than its files packed. The
    first try made §16.9's sample 1.87 MB, because its 360 near-identical AI models' places were
    carried raw; with the places stream carried as LZMA2 when smaller, they pack to almost nothing
    and stay places. (This entry first said they stay files; corrected in the v7 entry below.)
  - Models of the AI line are items (their file must be exactly what the line writes).
  - Measured here: the sample 391,033 bytes (v5 443,973, v4 482,496); the universalbibliothek
    model 4,443 (v4 5,775; Edward's 7z of the file: 5,698). Both installed by `sieve install` and
    by the oracle; `sieve-install` refuses v6 as it does v5.
  - `--v5` and `--widen-weights` were missing from `is_flag`, so each swallowed the next argument.
- **The search's time** (`cli::WeighBudget`; `--search-per-file`, `--search-total` on `sieve
  locate --weigh` and on the hallway's scripted `--locate`):
  - A line's search has the file's time for each of its items, capped at what is left of the
    run's. Tailoring gives its first 70% to filters alone and the rest to stacks, and keeps the
    best found when stopped.
  - The table says when a line stopped, found nothing by then, or never began; the `search:` line
    gives what was set and what it took. One step is not cut short, so a tiny time can be overrun
    (0.001 s a file took 0.19 s on the four-file CI folder).
  - The File Locator has a row above Tailor: − and + for a file and for the run, or `[` and `]`
    (Shift for the run). The one last set leads; the other is worked out from the files counted,
    again whenever a new folder is read.
- **Not done:** the times are not saved in `sieve-hallway.ini` (one session only). v6's items are
  still positional places; carrying a tailored stack's shorter address in the manifest is the
  next step that would tie the search to the instructions.
- **Checked here:** the full build (no warnings), the unit suite, the CI lines for v6 and for the
  search's time, and the File Locator's row in pictures.

### `sieve-manifest-v7` and `digit-histogram-v1` (17 October 2026)
- **Asked:** make a start on the next reductions (the text around a place, and a shorter place).
- **Where the model's 4,486 bytes went (v6):** its place, 4,096 bytes (8,192 digits of 4 bits),
  and about 390 of listing, the file's name twice among it. The search's time changed nothing:
  the weighing used it, the instructions did not.
- **What can shorten the place, measured:** the AI line's filters cannot, losslessly
  (`weight-rows` sets aside about 10^-11.8 of the line; `canonical-model` gives an equivalent
  model, not the file; `near-model` needs the model as its anchor). The digits are close to even:
  3.97 bits each over the model, 3.66 to 3.99 within a tensor. Coded under counts that learn per
  tensor, 4,012 to 4,027 bytes depending on the prior; Laplace's rule, 4,020.
- **`digit-histogram-v1`** (`core/include/sieve/digit_histogram.hpp`, SPECIFICATIONS §12.2): each
  segment's histogram ranked among all histograms, then its digits ranked among the rows with that
  histogram; exactly Laplace's code, in integers. The model: 4,020 bytes, about 0.03 s each way.
  The oracle decodes it its own way (`histogram_digits`). Unit tests: every row of 5 digits of 3
  values, all numbers different and back; random even and uneven models.
- **`sieve-manifest-v7`** (`--items` now; `--v6`, `--v5` the older ones):
  - nothing the rest gives: counts, empty streams, a single file's root, options the id fixes
    (an option is dropped when the line made without it has the same id), every file's own SHA-256
    (the tree's covers them; `--file-hashes` keeps them); items as `i K path`;
  - the listing carried as LZMA2 where smaller (`listing <packed> <bytes> <dictionary>` as line 2);
  - a line's places coded where the codes, as carried, are smaller than the raw places as
    carried. First built comparing them before compression: the sample's 360 near-identical
    models then went to files (405,002 bytes), since their codes are far-apart numbers that do
    not compress, while their raw places do.
- **Measured here:** the model `sieve-ai-16432c2a3953.safetensors` 4,257 bytes (v6 4,486, v4
  5,818; Edward's 7z 5,698); the CI's copy as `ub_one/model.safetensors` 4,253 (v6 4,443). The
  degree sample 9,618 (v6 391,033); for comparison, `tar` + `xz -9e` of the folder 28,152, and 7z's
  LZMA2 of its files' bytes alone, no names, 9,226 (no 7z here to make a real `.7z`).
- **A correction:** the v6 entry above, SPECIFICATIONS, and last round's CHANGES said the
  sample's AI models stay files under v6. They are carried as places (their raw places pack to
  almost nothing). Corrected in SPECIFICATIONS and the CI comment.
- **Tailored addresses, measured:** `sieve locate degrees --weigh --tailored --search-total 1800`
  found no filter stack that shortens any line's places (it took 49.8 s, not stopped by its time):
  the sample's items are spread evenly round each line. So carrying tailored addresses in the
  instructions would not shorten this sample; it would where items cluster (the CI's two melodies:
  864 and 800 bits as their own addresses, 36 and 32 under the tailored audio filters).
- **Not done:** the File Locator's weighing does not weigh the AI line (a model shows as its own
  address on the binary line), though its instructions carry it; tailored addresses in the
  instructions.
- **Checked here:** the full build (no warnings), the unit suite, the CI lines for v5, v6 and v7
  on the sample and the model, the oracle on each, `sieve-install` refusing v7.

### The plan from here, and the tightened rope (19 October 2026)
- **Plan agreed in outline** (Edward: reductions first, after this idea was recorded):
  1. Firm ground: the File Locator's weighing to weigh the AI line as the instructions carry it;
     the search's times saved in `sieve-hallway.ini`; CI to the repository root (waits on
     Edward's word).
  2. A fixed benchmark set, every reduction measured on it. Edward would rather not crowd the
     repository with large files, so its files must be small, or fetched or made by the tests
     rather than stored. Undecided which published model, if any.
  3. Reductions, in the order expected to pay: published models carried by `sieve-weights-v1`
     in the instructions; file names given by their items' titles; tailored addresses (v8), where
     items cluster.
  4. Features, in IDEAS order.
- **New idea recorded:** IDEAS §19.87, the tightened rope filter (keep bytes and propagate noise,
  searched together), from Edward's notes, his diagram and his paper *Rope Around The Clock*.
  Five questions for him are listed there; the proposed first step is a measurement (a wave
  fitted to a file's values, subtracted, the residuals coded by their histogram) before any
  filter. His notes ask that the AI conversation around the idea weigh less than his own words
  (VOICES).
- **No code changed.**

### The tightened rope built: `rope-wave-v1` and `sieve-manifest-v8` (19 October 2026)
- **Asked:** a filter across all dimensions, from Edward's idea (IDEAS §19.87), everything
  understood and noted first. His answers are recorded there in full.
- **Measured first (Python, before building):** one wave through the five kept points over a
  whole file never helped (worse than no wave on every file tried); propagation did. On raw
  pixels the third phase (value before + line before − diagonal) beat LZMA2.
- **`rope-wave-v1`** (`core/include/sieve/rope_wave.hpp`, SPECIFICATIONS §12.2): parameters
  L (8/16/32-bit values), W (line), S (step), P (phases 0–3, Edward's), K (the kept values and the
  rope through them); what is left coded per byte plane by `sieve-weights-v1`'s tables and rANS,
  whose functions moved to `core/src/weights_rans.cpp` so that `sieve-install` links them without
  the safetensors code (it went 2,308,920 → 2,140,984 bytes; the rope and its coder are 52 KB of
  compiled code). `rope_search` pulls tight within a time. `digit-histogram-v1` was not used for
  it: its exact numbering is quadratic.
- **`sieve rope`** (search, code, check, beside LZMA2; `--params`, `--decode`); the oracle's
  `rope-unpack`.
- **`sieve-manifest-v8`**: way `rope <params>`, a fourth stream; tried on every file carried as
  it is of 512 bytes or more with no copy in the folder, kept where smaller than LZMA2 alone (preset
  6). `--search-per-file` is its time (the File Locator passes its time for a file). v7 and v8 are
  now made for folders with no items too. `--v7` for no rope.
- **Measured here:**
  - the Vitruvian picture, grey pixels: 832,146 (LZMA2 865,400);
  - in colour at 475 × 650: 494,774 (LZMA2 623,717);
  - a noisy made sine: 4,583 (LZMA2 5,324);
  - text: no gain;
  - a mixed folder: v8 1,341,514, v7 1,504,516.
- **Not done:** other lines' places coded by the rope; the File Locator's weighing showing it;
  wider width candidates (a colour picture's row is not among the three best lags).
- **Checked here:** the full build, the unit suite (new tests: every parameter on six files,
  refusals, the search), the CI lines (`sieve rope`, v8 with sieve install, the oracle and
  sieve-install, and the earlier v5 to v7 lines again).

### The rope in the weighing; the comparison table's hex row (19 October 2026)
- **The comparison table:** "address, written in hex" is 23 characters, one more than the name
  column, so its row sat one space to the right (Edward). The column is 24 now
  (`tools/cli/compare.cpp`).
- **The rope in the weighing** (`tools/cli/weigh.cpp`): every file's code under the wave its
  search pulls tight to is a way to name it, weighed at 8 bits a byte of the code, the parameters'
  text and the file's size as a varint; the rope is a line of its own in the table, its shape its
  name (`rope-wave-v1`, 96 bits, paid once). Its searches take the time for a file and come out of
  the run's. The File Locator shows it in its table; the `search:` line now shows the rope's time
  too. The mixed folder weighs at 61.4% of its own addresses, every file's best way the rope (the
  text's being the rope with no wave: its bytes coded by how often each occurs).
- **Checked here:** the full build, the unit suite, the weighing's CI lines (unchanged: on those
  four small files the rope is not the best way), a new one for the rope, and the File Locator's.

### The first search's time, the dialogs in fullscreen (19 October 2026)
- **Edward asked:** with no time limit the default, does the File Locator's setting reach the
  first search? It did not: the search row was drawn only once a file was read, so the first
  search always had no limit, and the rope's search ran twice (the weighing, then the
  instructions).
  - The row is drawn before anything is chosen (and after a failure).
  - The time is kept in `sieve-hallway.ini` `[locator]` (`search_seconds`, `search_for = file |
    run`), loaded by both File Locators (the main menu's, the world's:
    `FileLocator::remember_search`), saved when the player changes it.
  - `rope_search` keeps its answers for the run, by the file's SHA-256 and the time given: the
    mixed folder's weighing and instructions together took 5.5 s, each about 5 s before.
- **Edward found:** installing from Sieve instructions, the folder dialog seemed to lock up, and
  the hallway minimised at every dialog. Not reproducible here (no Windows dialogs on Linux);
  three changes, each for a likely cause:
  - `SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS` off: a fullscreen window was minimised whenever a
    dialog took the focus.
  - An exclusive fullscreen window is set to the desktop's fullscreen while a dialog is open and
    given its mode back when the last one closes (`client/dialogs.hpp`): an exclusive window can
    sit over a dialog, which is modal, so the hallway looked locked.
  - The installer's first dialog ("which instructions") no longer raises the window when it
    closes, since the folder's dialog follows at once and the raise could land on top of it.
  Edward to confirm on Windows.

### `rope-wave-v2`: columns; `sieve-manifest-v9` (19 October 2026)
- **From Edward's stress test:** pi to a million digits as text, 1,111,103 bytes: 7z 512,745,
  `.sieve` 490,473 (rebuilt byte for byte). The digits' floor is 415,241 bytes; the gap was the
  layout. His file could not be fetched here (the site is blocked); a copy was made with mpmath and
  laid out as his size suggests (groups of ten, ten a line, CRLF: 1,110,004 bytes).
- **`rope-wave-v2`** (`C1`): what is left counted per column (position mod W); a segment of one
  byte value has no stream. The copy: 420,025 bytes (v1's best 491,890; LZMA2 486,077). The colour
  picture: 468,078 (was 494,774). `sieve-manifest-v9` when a file uses it; v8 refuses `C1`.
- **The search's widths:** the copy's line length (111) was not found at first: random digits make
  the scores noisy, and 111's multiples score as well. Now: the common divisor most often shared by
  the best lags first; each best lag taken down to its smallest divisor within 2%; multiples of a
  width taken left out; four widths; the shared divisor only where it matches within 10% of the
  best lag (else a picture's lags near its width share 2, and 2 blocked the width itself). The
  noisy CI sine now beats LZMA2 too (10,213 against 10,295, a line of nine of its cycles).
- **Edward asked** whether the File Locator shows the wave: it does (the weighing's "how").
- **Fixed in IDEAS:** last round's note that the weighing shows the rope had not landed (an edit
  that half-applied); it is there now.
- **Checked here:** the full build, the unit suite (new: columns in the round trips, a digit
  table, parsing), the rope CI lines and new ones for v2 and v9 (sieve install, the oracle,
  sieve-install), the earlier instruction lines again.

### `rope-wave-v3`: columns by line; `sieve-manifest-v10` (19 October 2026)
- **Edward found** v2 made his `pi6.txt` no smaller (490,473, the same as before). His file,
  uploaded here: lines of 55 and 54 bytes, a blank line after every twenty, a form feed in every
  tenth (100 of them), so any fixed width drifts a byte at each form feed. v2 had run (the file is
  v9) and rightly kept v1's answer.
- **`rope-wave-v3`** (`C2`, with `L8 S1 P0 K0` only): a byte's column is the bytes since the last
  line feed, held below W; each column's count is written before its segment, since a reader learns
  the columns only as it makes the file. His file: 421,306 bytes (v1 490,293; LZMA2 505,978; floor
  415,241); as instructions 421,491 (`sieve-manifest-v10`), 7z 512,745. The search tries it for any
  file with line feeds, W its longest line (at most 4,096).
- **Checked here:** the full build, the unit suite (new: C2 parsing and refusals, round trips, a
  table that drifts, the search choosing it), CI lines for v3 and v10 with all three readers, and
  the earlier lines again.

### `rope-wave-v4`: columns sharing tables; `sieve-manifest-v11` (19 October 2026)
- **Edward chose** shared column tables as the next step. On a benchmark script for the testers:
  "I feel like handing an automated script that tells them the right answer is the wrong move" —
  none is planned.
- **`rope-wave-v4`** (` G1` after C1 or C2): each column's group number first, then a segment for
  each group (C2: its count first). The coder groups greedily by estimated size; at most 256
  groups. `pi6.txt` 418,927 bytes (v3 421,306), instructions 419,115; a 200,000-digit table 83,225
  (v2 87,967, floor 83,048); grey picture 831,022 (v1 832,146).
- **`--v10`, `--v9`, `--v8`** make the older instructions (the search limited to the rope's ways
  they have), so CI still makes and checks v9 and v10 now that the search prefers v4.
- **What is left of pi6's gap** is layout (IDEAS §19.87): 2,500 bytes for the last column's
  alternating space and line feed, about 900 for blank lines and form feeds. Proposed: the byte
  above as context.
- **Checked here:** the full build, the unit suite (new: G1 parsing and refusals, round trips with
  groups, a group number out of order refused, the search choosing v4), the rope CI lines with
  v4 and v11 and v9, v10 still made with the new flags, all three readers each time.
- **Found running the hallway step's CI here** (it has never run on GitHub, the workflow not
  being at the root):
  - the filter check for `clean-data-v1` matched the retired clean-v1's note too ("replaced by
    clean-data-v1"); it now matches the filter's own row. Fixed.
  - the CHAIN tab check (`--chain-press "Down,N,Right,Right"`, written 13 October) saved
    `draw type-2x32` where it expected `hand`. **Fixed 20 October:** the training run `type-2x32`
    came later the same day ("character specs"), adding a stage to the default chain and a choice
    to the tab's list, so two steps of Right no longer wrapped round to `hand`. The check now
    presses Right three times and expects five stages (`hand` still among them); the whole step
    passes here.

### `sieve-manifest-v12`: less around a file's bytes (20 October 2026)
- **Edward found** a PNG's `.sieve` larger than the PNG. Measured: a PNG at zlib's strongest has
  nothing left for LZMA2 or the rope, and the instructions added 184 bytes (noise, 270,448) or
  205 (a photo, 715,596): 168 of listing (19 an empty `stream items 0 0 0`) and 16 of LZMA2's
  framing on bytes it could not shrink.
- **`sieve-manifest-v12`:** no line for an empty items or rope stream; the lzma2 stream carried as
  it is (`stream lzma2 B B 0`) where LZMA2 makes it no smaller. Made only where one applies;
  `--v11` makes v11. Both PNGs now 145 bytes over the file; `pi6.txt` 419,096 (v11 419,115). The
  rest is the version, the stream's line, the tree's SHA-256, the file's line and `end`.
- **His two photos** (IDEAS §19.88): the 7z's `.sieve` beat the JPEG's on both, the two gains
  nearly adding up; noted, not measured further.
- **Also fixed:** a v4 to v11 reader now refuses a stream carried at dictionary 0 instead of
  failing inside LZMA2.
- **Checked here:** the full build, the unit suite, the hallway CI step whole (new: a file of
  noise carried as it is, v12 against v11, all three readers, v12's lines refused when called
  v11; v11 still made with `--v11`).

### `sieve-manifest-v13`: files cut into pieces (20 October 2026)
- **Why:** Edward's photos packed smaller as `.7z.sieve` than as `.jpg.sieve`. Measured on his
  originals: the phone's metadata at the start (17,733 and 25,652 bytes) is LZMA2's (to 4,678 and
  12,536); the picture is the rope's (LZMA2 gains nothing on it). Chaining the two (the rope on
  LZMA2's output) lost on a test photo, LZMA2 having scrambled the bytes' spread.
- **`sieve-manifest-v13`:** a way `pieces z… r… rope <parameters>`; z pieces in the lzma2 stream
  at the file's turn, r pieces joined and coded by the rope. The cut is found from the bytes (4 KB
  blocks, then each boundary moved in 64-byte steps); used only where smaller than either way
  alone. `--v12` makes v12. His photos: 4,111,592 and 4,558,132 (were 4,122,470 and 4,569,466;
  7z 4,127,434 and 4,575,201); about 45 s each with no search limit.
- **Agreed next:** "Optimise Archive Formats", one box, off by default (IDEAS §19.88).
- **Edward's uploads:** the two photos were measured only; nothing of them is in the repository.
- **Checked here:** the full build, the unit suite, the hallway CI step whole (new: a file of text
  then unevenly spread bytes, cut within a block of where the text ends, smaller than v12 and xz,
  all three readers, refused when called v12).

### `sieve-manifest-v14`: Optimise Archive Formats (20 October 2026)
- **Edward asked** for one box, off by default (VOICES), and noted another for later: size and
  decoding time as one setting, for `.sieve` files served on the internet (IDEAS §19.89; set
  aside the same day, §17.3).
- **Built:** `--optimise-archives` and the File Locator's "Optimise Archive Formats" box (key A,
  kept in `sieve-hallway.ini` `[locator] optimise_archives`). xz remade exactly (liblzma's
  streaming encoder; the single-call one writes block sizes xz does not); 7z as its contents
  (the installer's decoder, now also in `sieve_compare`, with `read_7z` reading into memory);
  each only where clearly smaller. `pi6.txt.xz` 507,092 → 419,117; `pi6.7z` 507,182 → 419,128.
- **The remake depends on the compressor:** instructions made by one build remake on another
  only where its liblzma writes the same; installing checks it, and says so where not. A release
  built with the fetched liblzma (`SIEVE_FETCH_COMPRESSION`) pins it.
- **Not yet:** gzip (GNU gzip is not zlib), zip, multi-block xz.
- **New file:** `tests/optimise_fixture.7z` (10,513 bytes), for CI: no 7z tool is assumed there.
- **Checked here:** the full build, the unit suite, the hallway CI step whole (new: an xz remade,
  the 7z as contents, nothing changed with the box off, sieve-install refusing, v13 refused).

### IDEAS: size and decoding time set aside; proofread (20 October 2026)
- **Edward deprecated** the size-and-decoding-time setting for now (IDEAS §19.89, struck through;
  §17.3 says why): as he recalls, decoding takes fractions of a second, while the seconds go on
  loading and working in the hallway. Deprecated ideas are to be looked back over in the far
  future.
- **IDEAS proofread:** across the whole file by script (every section it refers to exists; no
  American spellings; no doubled words), and §19.87 to §19.89 read through: §19.87's "Next
  (proposed)" step, done since, now says so and lists what is still open; three smaller fixes.
- **No code changed.**

### The weighing weighs every way; `rope-wave-v5` and `sieve-manifest-v15` (21 October 2026)
- **Edward asked** for all five next steps (weighing, the byte above, a binary listing, more
  archive formats, CI at the root), and that Optimise Archive Formats aim, in the long run, at
  every archive format there is.
- **The weighing:** `file_ways` (pack.hpp) gives a file's three ways (LZMA2 alone, the rope,
  the pieces) with their sizes; the packer carries the smallest and the weighing weighs all three
  (new pseudo-lines `lzma2` and `pieces`). Edward's first photo now weighs 32,891,480 bits
  (4,111,435 bytes) against instructions of 4,111,599 (was 4,122,311). Not yet: archives weighed
  as carried with the box ticked (contents make one file many).
- **`rope-wave-v5`** (` A1` after `C2 G1`): each byte counted by its column and the byte above;
  pairs take the next listed group number when first met. `pi6.txt` 416,967 (v4 418,927),
  instructions 417,139 (were 419,096). `sieve-manifest-v15`; `--v14` and `--v13` make the older.
- **CI:** two checks read `sieve locate`'s report from standard output without `--manifest`
  (where it goes to standard error); fixed.
- **Checked here:** the full build, the unit suite (153,230 checks, 0 failures; new: A1 parsing,
  round trips, the search choosing v5 and v4 under `newest` 4, refusals), the hallway CI step whole.

### `sieve-manifest-v16`: the listing's head in bytes (21 October 2026)
- **Built:** "sieve", 16, flags, the root, the streams (varints and LZMA2's dictionary byte), the
  tree's SHA-256 as 32 bytes, then the body (v15's `line`, `u` and entry lines, packed as LZMA2
  where smaller). Read back into v15's text, so every reader path is v15's. Made wherever `--items`
  makes v7 or later; `--v15` keeps the text. Oracle: `v16_text`.
- **Measured:** a 1,597-byte script 661 → 586 (head 142 → 67); `pi6.txt` 417,139 → 417,061; the
  trained model 4,257 → 4,185 (folder 4,176); the 64-character pages 422 → 345.
- **CI:** the v7, v12 and v13 checks that read or relabel a listing's text now make it with
  `--v15`; new: v16 on the degree sample and the model, and a v16 head with an unknown flag refused.
- **Still open:** the entries in bytes too (IDEAS §19.90).

### Optimise Archive Formats: gzip and zip; `sieve-manifest-v17` (21 October 2026)
- **Edward's aim:** every archive format there is, in the long run (VOICES; IDEAS §19.88 has the
  tiers).
- **Built:** a gzip remade exactly where zlib gives its deflate back (`gz <level> <memory level>
  <header hex> <size>`, v17, head byte 17), else carried as what it holds (`.tgz` as `.tar`); a
  zip as its contents (stored and deflated entries, CRC-checked; refused, so left as it is, if
  encrypted, another method, zip64 or a name that leaves its folder). `--v16` makes v16.
- **Measured first:** a tar needs no unpacking (the pieces split it: 425,701 as it is, 425,597 for
  its two files). `pi6.txt` as a gzip 546,741 → 417,128 (remade); GNU gzip -9's 546,777 → 417,128
  (remade too); a zip 546,827 → 417,107; a `.tar.gz` 557,241 → 425,767.
- **Fixed:** an xz or gzip located on its own and carried as what it holds now takes its new name
  as the root (before, the archive's name stayed the root: installing would have made a folder).
- **CI:** the archive check gains a Python gzip (remade) and a zip (contents), and v17 called v16
  refused.

### CI's first runs on GitHub: three fixes (21 October 2026)
- **Linux:** SDL found no X11 or Wayland development files on GitHub's runner, so configuring
  stopped. The workflow now installs them (`apt-get`) before configuring.
- **macOS:** `std::min` was given a `size_t` and a `uint64_t` in `biguint.cpp`; on macOS those are
  different types, so it did not compile. Now `std::min<size_t>`. A check with `uint64_t` made a
  distinct type compiled all 124 core, client and tool files and found no other.
- **Windows:** 18 training checks failed because Git on GitHub's Windows runner turns LF into CRLF
  on checkout, and `tests/training_corpus.txt` was not among the files `.gitattributes` protects,
  so the corpus the vectors were made from changed. Reproduced here (the same 18 checks at the same
  lines with the corpus given CRLF); `.gitattributes` now protects all of `tests/`. A build with
  GCC's undefined-behaviour checker ran the whole suite with none found.
- **Edward's IDEAS note on a file's opposite in noise** is §19.91 (an answer; nothing built).
- **The second run on GitHub** got further and found four more, none of them in the packing:
  - *macOS:* the installer's link options (`--gc-sections`, `-s`) are GNU's; Apple's linker takes
    `-dead_strip`.
  - *Windows:* `sieve world --read … --out FILE` built the path from `a.get("out").begin()` and
    `a.get("out").end()`, two different copies of the string (undefined behaviour). GCC got away
    with it; Microsoft's library threw "string too long". Now one string. No other line of that
    form calls a function that returns a copy.
  - *Every system:* GitHub runs a step with `bash -eo pipefail`, so the check that a reordered run
    is "NOT reproduced" (which exits with an error, as it should) failed the step; it now tolerates
    that exit. Two checks wrote the AI line's filters in the wrong order (Sieve lists them in its
    registry's order). The model step starts the hallway, so it now has the hallway step's
    no-screen settings (`SDL_VIDEO_DRIVER=offscreen`, `SDL_RENDER_DRIVER=software`).
  - Every multi-line step now names the line it stopped at (`::error::the step stopped at its line
    N`), so a failure on GitHub points to its command.
  - These steps had never run before: the workflow ran only once it moved to the repository root.
    Run here the way GitHub runs them (`bash -eo pipefail`, steps 6 to 16, Linux) before handing over.

### LZMA2's settings chosen per stream (21 October 2026)
- **Why:** Edward sieved 137 JPEGs from his pictures folder one by one; 21 came out larger than
  7z's (by 15 to 393 bytes), all small (the largest 364 KB), none larger than the picture. Sieve's
  LZMA2 always used preset 9 extreme's lc 3, pb 2 (made for text); a JPEG has no alignment, and
  pb 0 with lc 0 or 4 was 113 to 614 bytes smaller on each of the eight he sent.
- **Built:** v17's packing tries a few lc/lp/pb on each stream's first MiB, then the best of them
  on the whole stream at two match lengths, and keeps the smallest (the old stream among them).
  No new version: LZMA2 carries lc/lp/pb in its own stream, and every reader takes them from there.
  `--v16` and older are made as they were. Measured on a varied set (SPECIFICATIONS §12.2): every
  file smaller or the same; five of the eight JPEGs now smaller than 7z's. LZMA2's time about
  triples.
- **Still larger than 7z:** K2_00000 (3 bytes), K2_00001 (53), TheBigBus (204). 7-Zip's own LZMA
  encoder parses better than liblzma's; the repository has only its decoders. Adding its encoder
  (`LzmaEnc.c`, `Lzma2Enc.c`, `LzFind.c`) as another candidate is the next step there; measure
  first.
- **CI:** a check that the oracle's source as one file is smaller than `--v16` makes it, read back
  by both readers. The setup menu's reset check now compares the saved settings, not screenshots
  (the menu shows a time measured on each run, which a busy machine changes).
- **Edward's photos stay out of the repository;** the eight JPEGs were only measured.

### CI on GitHub: where the three systems stand (21 October 2026)
- **Linux:** every step green, on every run since the second fixes.
- **Windows:** green up to the oracle step, which stopped at its pcm vectors: the oracle made the
  float WAV of `vectors_pcm_v1.tsv` from `math.sin`, the platform's C library's, and Windows' gave
  one sample a different last bit. The oracle now computes that sine exactly (`_sin_exact`,
  `Decimal`, one rounding), the same bits everywhere; glibc's were already correctly rounded on all
  forty, so the committed vectors are unchanged. The rest of that step has not yet run on Windows.
- **macOS:** the build stopped linking `sieve-install`: SDL's static library is Objective-C on
  Apple's frameworks, which the hand-made `sieve_sdl_small` target did not name. It now names the
  Objective-C runtime, iconv and the frameworks, and `-dead_strip_dylibs` drops the unused ones.
  Not testable here (no Mac); the next run shows it.
- **The run after (21 October 2026)** got further on all three, and found three more:
  - *Linux:* the setup menu's Find my limits grew the AI line's layers past aispace-v1's 64 on
    GitHub's larger machine, and the hallway refused its own settings ("layers from 1 to 64"). A
    bug in the hallway, not the check: growing now stops at a line's own limit where its format has
    one (the AI line's layers, 64). Shown here with a far larger budget (`--unit-time 60000
    --item-memory 90`): 64, and no error. The check now also reads that the saved layers are 1 to 64.
  - *macOS:* it builds now (the frameworks). The model step's hallway could not open a window:
    on a Mac, SDL's offscreen driver still asks for OpenGL. That step uses SDL's dummy driver on
    macOS (it draws in memory too; nothing there compares pictures), offscreen elsewhere.
  - *Windows:* past the pcm vectors, it stopped at `sieve map fixture.map | cmp - fixture.map`:
    printed to the console, Windows turned each line feed of the map into CR LF. `sieve map` now
    prints the map's own bytes on every system. No other check compares Sieve's console output
    byte for byte.
- **Round five (21 October 2026):** Linux and Windows green on every step. macOS failed in the
  filters step: "an instrument is a General MIDI program, 0 to 127" for a notes3 line. The cause,
  a bug on every system: `make_notes3_set` filled the instruments with
  `instruments.assign(voices, instruments[0])`, a reference into the vector it refills, which the
  standard forbids for `assign`. GNU's library copes; Apple's (libc++) frees first and reads
  garbage. Shown here with clang and libc++: `{0}` assigned that way reads 1,627,873,494. Now a
  copy first. No other `assign` takes its own element (`push_back` and `insert` must cope, and do).
- **A libc++ build here:** Sieve built with clang and libc++ (as on a Mac): unit suite 0
  failures, and every workflow step (6 to 16) passes on it. Worth doing whenever macOS fails again:
  `CC=clang CXX=clang++ cmake … -DCMAKE_CXX_FLAGS=-stdlib=libc++ -DCMAKE_EXE_LINKER_FLAGS=-stdlib=libc++`
  (needs `libc++-dev`).
- **Round six:** macOS reached the oracle step (Linux and Windows green). A check compared
  `$(wc -c < FILE)` as text; macOS's `wc` pads its count with spaces. The three such checks now
  compare numbers (`-eq`). The workflow's macOS steps have no other GNU-only tools (`stat -c`,
  `sed -i`, `sha256sum`, `timeout`, `\t` in `sed` and the like; checked).
- **Round seven:** macOS reached the oracle step's line 270: the check copied Sieve's own
  program into a folder and expected it in the x86 stream. Sieve puts ELF and PE programs there,
  not a Mac's Mach-O (and GitHub's Macs are ARM). On macOS the check's program is now those bytes
  behind an ELF's first four, so the x86 stream is still tried and read back there; Linux and
  Windows use the program as before. Shown here with such a stand-in: one program in the x86
  stream, and both readers give the folder back.

### 7-Zip's encoder, install times, a web dictionary weighed, maps and instructions (22 October 2026)
- **7-Zip's own LZMA2 encoder** (LZMA SDK, 7-Zip 26.03, as the decoders; `third_party/lzma`,
  `LzmaEnc`, `Lzma2Enc`, `LzFind`, `LzFindOpt`, `LzHash.h`, single-threaded) is one more candidate
  for each stream of the newest version, at the settings already chosen, kept where smaller
  (SPECIFICATIONS §12.2). Its own library (`sieve_lzma_enc`), linked by the packer only, so the
  installers are as before. Measured: the program 2,227 bytes smaller, a log 273, one JPEG 11; the
  rest unchanged. 7-Zip's whole grid of settings was tried first: no better than this one pass, at
  four times the time; its deeper match search (`mc`) gained 17 bytes on the program.
- **Install times:** `sieve install` ends with `time      decoded in D s, checked and written in W s`;
  the hallway's File Locator says the same after installing (`install_times_text`, locate.cpp).
  A CI check reads the line.
- **The web dictionary** (IDEAS §19.92) was measured and not built: level with brotli on pages
  of 20 KB and more, behind on small pages. The pages were a 2014 snapshot (the websites were not
  reachable from the session).
- **Maps and instructions** (IDEAS §19.93): the 3D node graph stores nothing in a map (it lays one
  out afresh), so a map could be a `.sieve` with a flagged graph part. Edward chose it; built the
  same day (the entry below).
- **Edward's sync:** this session's git could not be reset or merged to Edward's `origin/main`
  (refused as destructive); the code was the same, only his build metrics and root `CHANGES.md`
  differed. Start the next session from his `main`.

### Maps as Sieve instructions, `sieve-manifest-v18` (22 October 2026)
- **The format** (SPECIFICATIONS §12.2, §12.3): v17 with a graph part, the map's text as `g<TAB>`
  lines after the entries; head byte 18 with flag 64, which only v18 may set and v18 must. The
  held files are the instructions' own files; nodes that only name a file are text alone.
- **Code:** `tools/cli/map_sieve.*` (new): `map_of_instructions`, `read_map_file`,
  `is_old_map_file`, `write_map_instructions` (stages the held files in a temporary folder named
  as the root, walks and packs it with the graph part set first). `Map::parse` can stop after the
  text (`held_bytes_follow = false`). `locate.*` reads and writes the graph part.
- **`sieve map`** writes `.sieve` only (`--out X.map` is refused, naming IDEAS 19.93), reads a
  folder, files, a `.sieve` (with or without a graph part) or an older `.map`, and prints the
  text. **`sieve install`** of a map puts back its held files and says how many it names that it
  does not carry.
- **The hallway** (`client/node_graph.cpp`): `maps/` lists `.sieve` and `.map`; a change is saved
  as `.sieve` (an older `.map` is written beside it as `.sieve`, the `.map` left alone); New
  map..., export and the open dialog use `.sieve`; instructions without a graph part are shown as
  the map of their tree, read-only (`tree_only`).
- **The release:** `make_release` publishes `source.sieve` (Edward's name) in place of
  `sieve.map`, and checks it installs the source back. `--map-with` used an undefined `source`
  and would have stopped with an error; it now holds the source again, as the published map does.
- **The oracle:** `unpack` reads v18 and a `.sieve` file as it is (its address bytes turned back
  into the manifest by the closed form); `graph FILE` prints the graph part; `map … --text`
  prints a map's text alone. It still writes the older map files, for the check that they are read.
- **CI:** the map checks now make `.sieve` and compare the text with the oracle's, install the
  held files with both readers, read a `.sieve` without a graph part as a map, and convert an
  older `.map` (the oracle's) to the same text. The hallway's anchor checks use `.sieve`.

### Releases at the newest version, maps with a search limit (22 October 2026)
- **`make_release` made v4** for `sieve.sieve` and the installer program (no `--items`); it
  passes `--items` now. This release's folder: v4 7,939,263 bytes, newest 7,819,304, 7z
  8,414,101. Run here end to end (635 s): `sieve-setup` installs the folder byte for byte, no
  release file is an item.
- **Maps** give the rope 10 s for each held file (`kMapSearchSeconds`, `sieve map
  --search-per-file`, 0: none). The source's 7z took 19.5 minutes before, for nothing.
- **IDEAS §19.94** (grids along the line: measured, break-even), **§19.95** (this), **§19.96**
  (the degrees sample 8,784 bytes, the starting page 162, as `.sieve`).
- **Next to measure:** BCJ2 for programs (7z beats Sieve on a folder that is mostly the program);
  the hallway's `--sample-degrees` at the newest version (its CI check compares against v4).

### The File Locator's time estimate, pieces cut faster (22 October 2026)
- **The estimate** (`client/file_locator.*`, IDEAS §19.97): 16 MiB and more, the reading's work
  timed on 1 and 2 MiB taken in eight stretches across the file, drawn out to the size, the
  search's most added; "took X (estimated Y)" after. Measured 15 to 20% short on two 40 MB files.
  Scripted runs (`--main-menu --locate`) print the status line ("file locator: took ...").
- **Pieces** (`tools/cli/pack.cpp`, `pieces_of`): the boundary steps weigh LZMA2 over a fixed
  window of 64 KiB, not the whole run; the same instructions on all 16 files of the varied set and
  on a 40 MB tar, the tar packed in 1,245 s (2,053 s).
- **Stop and the best so far** (`core sieve/stop.hpp`, a thread-local flag and progress sink;
  checked in `pack.cpp`, `rope_wave.cpp`, `compare.cpp`, `tailor.cpp`, `weigh.cpp`): the File
  Locator's Stop (S) keeps the best so far; saving writes the instructions made when read.
  Scripted `--stop-after S`, `--locate-save FILE`; a CI check installs a stopped result.
- **Force stop:** the stop flag is an `atomic<int>` (1 stop, 2 force; `force_requested()`);
  `encode_lzma2` streams a MiB at a time and returns nothing when forced (`abortable`), the SDK
  encoder breaks off through `ICompressProgress`, `pack_stream`'s first pass is abortable only for
  the x86 and lzma2 streams and the quick estimates, with the fallbacks in `add_packed`.
- **The graph of the best so far** (`FileLocator::draw_graph`): `sieve::progress(what, best, of)`
  carries a step's new best; the locator draws it onto a target texture as it comes (no values
  kept but each step's first), squeezing left past its time and up past its depth.
- **IDEAS §19.98** (recurring values, scale repeats, a locked random byte, the standing watch),
  **§19.99** (threads for the search and the packer).

### 0.13.75 (22 October 2026)
- **The version is 0.13.75** (Edward: 0.13.25 was manifest v7's): `CMakeLists.txt`, `vcpkg.json`,
  and the release tool's examples (`make_release.py`, `SIEVE-INSTALL-USAGE.md`).
- **The web decoder plugin** is planned in IDEAS §19.92 (Edward's notes, and what follows from
  them: native messaging, the `.sieve`'s own hash in an `integrity` attribute, a site map as the
  root of a site's hashes since links that go round cannot hold each other's, single files shown).

### The web plugin, phase 1 (23 October 2026)
- **`sieve web-host`** (`tools/cli/web_host.*`; `sieve_cli.cpp` starts it for an argument
  `chrome-extension://...`): native messaging; refuses before decoding unless the `.sieve`'s
  SHA-256 is the link's; one file only, no items, remakes, archives as contents, or maps.
- **`web/`**: `extension/` (Manifest V3; background, content script, viewer; id
  `bnigebolckfoapimidlcmbilcbhjbmjg`, fixed by the public key in `manifest.json`, whose private
  half was not kept: an unpacked extension needs none), `host/` (install scripts, `check_host.py`),
  `test-site/` (`make_site.py` builds `site/`, which is not committed), `README.md`.
- **Tested** in Chromium here (Playwright, headless, the host's JSON in the profile's own
  `NativeMessagingHosts`, which a profile folder of its own needs on Linux). The CI runs
  `check_host.py`, `make_site.py` and `node --check`.
- **Next:** the site's map of every page's hash and the harness that writes links' hashes with it
  (built since: `sieve site`, below); the crawler; a `.sieve` of many files as a listing;
  WebAssembly.
- **On the website (23 October 2026):** `Pages/secure.html` (Sieve Secure, a button on
  `index.html`) gives the setup in four steps and links to the test site built into
  `Pages/secure` (`make_site.py --out Pages/secure --home ../secure.html`, 600 KB). The test site
  is styled as the website, its many-file `.sieve` is the pages' sources (3 KB, not the pictures),
  and its plain pages say when the extension is not running (the content script marks
  `<html data-sieve-extension>`). Tested in Chromium served from the repository's top folder.
- **`web/test-site/serve_site.bat`:** step 4 in a double-click (finds Python as `make_release.bat`).
- **Edward's live test (23 October 2026):** the pages said the extension was not running while
  showing its pictures; the mark came from the content script at `document_idle`, after the page's
  1.5 s look on a slow connection. Now `mark.js` marks at `document_start`, and the pages look
  after `load` (and take the note away if the mark comes later); shown with every request held 2 s.
  The viewer's bar gives the times (fetched and checked, Sieve's own decoding from the host's
  `seconds`, the hand-over, in all, and a page's own pictures).
- **Still shown for Edward after that:** the pictures decoded (content.js running) but the page was
  never marked, which fits `mark.js` finding no `<html>` element yet at `document_start` (Chrome:
  "before any other DOM is constructed"; headless Chromium here had one). `mark.js` now waits for
  it, and `content.js` marks again; shown with a `mark.js` that marks nothing and every request
  held 1 s: the note stays hidden on both pages. Not confirmed on Edward's Chrome yet.
- **Confirmed by Edward (23 October 2026):** everything works as expected.

### The site harness, `sieve site` (23 October 2026)
- **`tools/cli/site.*`, `cmd_site` in `sieve_cli.cpp`** (IDEAS §19.92): `build SRC --out OUT
  [--sieve-pages] [--plain LIST] [--sieve LIST] [--search-per-file S]` and `check DIR`. Any site with
  plain links; pictures (and pages) made into `.sieve` files (the newest version, no items), links
  changed to them with their `integrity`, pages made after those they name (pages naming each other
  refused, named), `site.sieve` (a v18 map of every file) written last and its SHA-256 printed.
  `data-sieve-keep` leaves a tag as written and unchecked.
- **The test site:** `src/` has plain links now (`pictures/*.jpg`, `about.html`, `notes.html`);
  `make_site.py` stages it (`{{home}}`, `pages.sieve`, the tampered picture made as the build makes
  Vitruvian Man, `{{tampered}}`), runs `sieve site build ... --sieve-pages --plain gallery.html`, then
  `sieve site check`. The built pictures moved from `images/` to `pictures/`. The wrong-hash link
  states an empty file's hash now (it named About's, which the source cannot know before the build).
- **The CI** checks the test site with `sieve site check` (3 kept as written), a file changed after
  the build caught, two pages naming each other refused, and the same two built with one plain.
- **`Pages/secure`** rebuilt with it.
- **Tried on the Potentia website, and made careful (23 October 2026; Edward asked how it does
  with PHP and scripts):** only a file a read tag names is made; a file named elsewhere (CSS,
  scripts, PHP's code, other attributes, by its whole name) is kept beside its `.sieve`
  (`--keep-originals` keeps all); pages with a script stay plain; `.php` pages are read and kept,
  their code untouched; loops are found first and shown with a `--plain` list; a failed build
  leaves nothing; progress per file. The extension (`content.js`, `viewer.js`) takes away
  `srcset` and `<picture>` sources, which showed the unchecked file. CI: a mixed site (CSS, a
  script, PHP, an unlinked picture).

### The documentation brought up to date (23 October 2026)
- **Edward:** the full readout of every command and option belongs in `sieve` itself, and the
  README points to `sieve help`. Checked against the code: `sieve help` already named every option
  every command reads; what was out of date was facts, now fixed:
  - `sift`'s default dictionary (scowl-en-60-names, with the -names lists), and its CSV columns;
  - `locate --installer` writes v4 on its own, the newest (v16, v17) with `--items`, v3 with `--v3`;
  - `install` reads every version from v2 to v18; `lines` lists every line and alphabet; the
    overview groups the commands by what they are for, and says every line;
  - `version` prints every pinned rule (the pcm and notes3 rules, the other lines, the instructions,
    the rope, records, the vault, training), from the code's own constants;
  - `mesh` and `world` name `--short`; `dicts`, `bind`, `tensors`, `chat` and `cipher` carry the
    README's measurements.
- **The hallway's `--help`** names `--vertices`, `--faces`, `--coords` and `--limits-focus`, every
  line, and the tile size as a setting. **`sieve-install --help`** is new (`client/win_console.cpp`
  now linked into it, for the console).
- **`tools/check_help.py`** (in the CI): every option the code reads is on its command's help page,
  every command has one, and every hallway option is in its usage text.
- **README:** the Commands section is a quick start, `warp`'s output and a table of the commands,
  pointing to `sieve help`. What it held beyond options moved: the File Locator and the
  every-degree demonstration to the hallway section, the installer formats' history to
  `docs/SIEVE-INSTALL-USAGE.md` (*The formats, as they grew*), `measure`'s figures to M3 results.
  Layout, key terms and the corridor's tile size brought up to date; SPECIFICATIONS' corridor too.
- **The repository's README:** eleven lines, version 0.13.75, recorded training and the web
  extension among what is built.
- **Edward's next step:** new screenshots for the README (he is moving the old ones out).
- **`sieve locate --installer` and `--program` make the newest by default** (Edward, 23 October 2026);
  `--v4` makes the plain installer, `--items` is accepted and changes nothing. CI lines that
  expect v4 pass `--v4`. Not yet tested (Edward's usage limit): build, run the CI.

### The right-click menu (24 October 2026)
- **`context/`** (Edward: "the same way I can right click a 7zip and extract here or extract to a
  folder of the files name"): `install_context.bat [sieve-install.exe]` adds a Sieve submenu to
  `.sieve` files for this user (`HKCU\Software\Classes\SystemFileAssociations\.sieve\shell\Sieve`,
  `SubCommands`, two verbs), `uninstall_context.bat` removes it, `README.md`.
- **`sieve-install --extract-here` / `--extract-folder`:** the destination from the file's own
  place (its folder, or a folder of its stem); the window opens ready, so it shows the progress
  and Replace can be ticked. Shown here with `--yes`: both put the files where they should be.
- **Noted, not built:** a "Sieve File/s" entry on any file or folder to make `.sieve` files
  (command line or a window of its own: not decided).
- **Not yet:** tried in Explorer (Windows); the release does not ship `context/` yet.
- **`sieve-install` is the full installer** (Edward, 24 October 2026: "the self extracting
  installers should carry the full capabilities of sieve.exe"): linked with `sieve_lines` and
  `sieve_compare`, it calls `use_item_maker()`, so items of Sieve's lines are made again and
  archives remade exactly, for installer programs and the right-click menu alike. Linux Release:
  2,227,000 → 5,209,160 bytes. Shown: a picture item and a remade gzip, by `sieve-install --yes`
  and by an installer program, byte for byte. The CI's four "sieve-install refuses" checks now
  check that it installs every file. Not yet run: the full CI.
- **Extract works as an archiver's (Edward, 24 October 2026):** `--extract-here` and
  `--extract-folder` start at once and close the window when done (or cancelled); a failure stays
  open, saying why, with **Back** to the first screen (Replace, another folder). Shown: the first
  extract closes by itself, a second into the same folder stays open.

### Step 1 begun: making `.sieve` measured, and threads (24 October 2026)
- **`tools/bench/bench_make.py SIEVE [--set full] [--search S,...] [--only NAME,...]`:** a fixed set
  from the repository's own files, timed by the packer's stages (`--timings` names them now:
  `pack.*` in `tools/cli/pack.cpp`); writes results/bench-make-<set>-<date>.tsv and .md (a row at a
  time). results/ holds the first runs (quick set, and the 52 MB recording).
- **Threads in the packer** (`for_each_index`, `pack_threads`, `passes_at_once` in pack.cpp;
  `sieve::current_stop()` so workers take on the caller's Stop): cutting into pieces (blocks,
  boundary steps), LZMA2's trial and passes. The same instructions at any count (shown on every
  file of the quick set; the CI compares 1 and 4 threads on Vitruvian Man). `SIEVE_PACK_THREADS`.
  Figures in IDEAS §19.99. Not yet: the File Locator's estimate checked against this.
- **The rope's search, ordered by the file (24 October 2026; IDEAS §19.99):** text has its line
  waves early; bytes the first waves cannot shrink by 2% are searched no further;
  `--search-every-wave` / `[locator] search_every_wave` searches every wave. `RopeSearch` records
  `best_at`, `best_trial` and `took`; `sieve rope` prints them. Quick set at 1 s: 67.0 → 30.5 s, the
  word list 10.86% smaller, nothing larger. The estimate measured (30% low on the recording); not
  changed yet.
- **The File Locator, step by step** (IDEAS §19.97): `estimate.*` and `locator.*` timings; the cut
  into pieces kept by SHA-256 (`pieces_of` in pack.cpp), so weighing does not cut again (the 52 MB
  recording 11 min 43 s → 8 min 28 s); samples packed with the whole's dictionary (`SampleAsWhole`).
  The estimate still a third low; next, refine it once the instructions are made.
- **The estimate, a quarter more and refined as it goes** (`refine_estimate`, `est_steps_` in
  file_locator.*): per-step lines from the samples; after each step the rest rescaled by the done
  steps' ratio. The recording: 7:38 first, 8:01 refined, 8:14 taken; the repeated text 1:54, 5:06,
  4:48. `loc.estimate.refined`, `loc.took.refined` in data/lang/en.txt.
- **The pieces' boundaries, a twelfth of the time** (`pieces_of_uncached` in pack.cpp, IDEAS
  §19.99): steps of 512 bytes then 64 near the best, over 16 KiB (was every 64 over 64 KiB); the same
  bytes on every file measured; the recording's boundaries 164 s → 13 s. `pack.pieces.blocks` and
  `pack.pieces.boundaries` in `--timings` and in bench_make's table.
- **A text's search ends after its own waves** (IDEAS §19.99): measured on 127 files, the same bytes
  as every wave on all of them, 3.8 times faster with no limit; the texts of the quick set about
  twice as fast at 1 s. Fixed-width lines also try their columns at that width.
- **`tools/bench/bench_make.bat`** (Edward: his `python` and `py` fail at a prompt but work from
  `make_release.bat`): finds Python as that does, passes the Release `sieve.exe` (or the `.exe`
  named first), and keeps the window open.

### Step 3: the long-open filter defects, IDEAS §2.1 to §2.3 (24 October 2026)
- **`symbol-entropy-v2`, `model-information-v2`** (core/src/filters/statistics.cpp): bounds in
  thousandths of log2 min(symbols, n); a unit ending in two or more SPACEs judged on what comes
  before them (padding alone fails). Retired as v1 (judge only), so tailoring and every existing
  stack are unchanged; v1 says "replaced by" v2. Oracle (`f_symbol_entropy_v2`,
  `f_model_information_v2`), 288 verdict vectors and 24 black-and-white ranks; unit tests.
- **`scowl-en-{35,60,80}-names-v2`** (data/dictionaries, `build_dictionary.py --names-v2`): without
  all-caps entries and one- or two-letter entries the plain list lacks (but a, I). The default
  since 24 October 2026 (Edward's choice), with **`function-words-v2`** (data/filters, by
  `build_text_plugins.py`, which writes v1 unchanged beside it; CI compares it with the oracle).
- Figures in IDEAS §2.1 to §2.3.

### Step 4: the web, phase 2, begun (24 October 2026)
- **`--served-at PATH`** (`SiteOptions::served_at`, `served_path`/`target_of` in tools/cli/site.cpp;
  `SiteCheck::unread`): links from the server's root read as the site's own under PATH.
- **Listings** (`web_open`, `WebOpened`, `WebFile` in tools/cli/web_host.*; `web_decode` is
  `web_open` with no path, and still refuses many files, as `site check` expects): decode answers
  a listing, or one file by `path`. Extension: background passes `path`; viewer `showListing`,
  `viewerFor`, `pathIn`; content.js refuses a listing as a picture. `check_host.py --listing`,
  `--path P FILE`.
- Figures and what was seen in IDEAS §19.92 ("Phase 2 begun").
- **The site's table** (25 October 2026, Edward's design): `index.sieve` (`kTable` in site.cpp;
  `site.sieve` still read), `<link rel="sieve-table">` in plain pages, `resource_of` (scripts and
  stylesheets given `integrity`); host op `table` (`web_table`); extension `guard.js`, `guard.css`,
  `notice.js`, `options.*`, background `guard`/`trust`/`forget`/`sites` and `registerGuard`
  (permissions `storage`, `scripting`); also a fix: a host reply with no `parts` (hello, table)
  never resolved. **`web/crawl_site.py`/`.bat`**. CI: the table, the host's reading of it, and the
  crawler against the test site served locally. Left: PHP and scripts' own fetches. Decided
  (Edward): the remembered table and "Trust the new table" stay; no signing, ever ("I don't trust
  myself or any human being alive to keep a key safe").

### The READMEs rewritten, a benchmark, and a plan for showing Sieve (27 October 2026)

Edward: "it is very difficult for people to get into and takes me a long time to explain to
people ... Help me present the project better through the readmes and help me put together a plan
to show off its utility." Both READMEs now lead with what works today and how to try it; the
vision moved whole to `VISION.md` at the top, and the Sieve README's reference sections whole to
`docs/MANUAL.md`; the old READMEs are kept as `Previous/READMEv2.md` and
`Previous/SIEVE-READMEv1.md`. `tools/bench/compare.py` measures Sieve against 7-Zip, xz, zstd,
brotli, bzip2, gzip and zip on any folder; its first table (this repository's `Sieve` source tree)
is in `README.md`. `docs/SHOWCASE-PLAN.md` is the plan: the format frozen, the benchmark on public
data, the ISO test set, six short demonstrations, and who to show what. Found on the way: a
committed `lt/` folder (a language-test leftover), to remove (removed 28 October 2026).

### A second code review (27 October 2026)

A fresh session read the project from this file down and reviewed it, mostly where Sieve reads
files from someone else. `docs/REVIEW.md` ("The second pass") has what was fixed and what was left,
with how each was tried. In short: a RAR's recipe (its `rar` switches and paths) is checked when
instructions are read; the web host holds decoding to its limit before anything is made, checks
the vault, and sends only UTF-8; a failed Extract no longer deletes files it wrote over; the site
notice's buttons answer only the reader's own clicks; a safetensors name given twice is refused;
and the core and the `sieve` tool are warning-free under GCC 13 and clang 18 (the hallway was not
built here). No format, version or address changed. The
plan below is unchanged.

Then the two left open that Edward asked for (27 October 2026, "Both of those fixes are
necessary"): "Show it anyway" kept by the extension per tab, not in the page's storage; and a
listing or a site's table sent in parts, as a file is (a `.sieve` of 10,000 files had made one
message of 1,178,012 bytes; CI step 11 now checks one). Trying them in Chromium found a third: the
guard's fetch of the page could come from the extension's own cache, and so pass a page changed on
the server; it now revalidates. Details in `docs/REVIEW.md` and `web/README.md`.

### The documentation read through, during the format's pause (28 October 2026)

Edward: "let the manifest v31 sit for a while before pressing forwards with new manifest versions
... read through ideas and the general documentation base and make suggestions and edits along the
way as necessary." No code, format or figure changed: documentation and one script's notes, with `lt/` removed.
- **IDEAS.md** has a guide at its top: how to read it (the file grew in the order things were said,
  so an entry's later notes may undo its first paragraph), where the built, set-aside and framing
  parts are, the ideas that would need a new manifest version and so wait, and the work that needs
  none. Entries found built or answered and marked so: §4.1, §4.3 (partly), §4.4, §4.5, §4.6
  (no signing), the licence (§8, §11), §10's portal signs, warp by content, warp-to-start, CI on
  three systems and `Previous/`, §11's names list, and §9.4 (now in Sieve's README). §16.3's
  "folding maps in: set aside" and §19.53's plan are marked superseded (§19.93; this file's plans).
- **SIEVE-INSTALL-USAGE.md** said the newest version was v16 and that releases were v4; it says
  v31, and its one paragraph of every version's history is now a table. `make_release.py`'s notes
  said v4 too (the code has passed `--items` since 22 October).
- **MANUAL.md** and **web/README.md** no longer call maps v18 or `source.sieve` a sealed map.
- **`Sieve/lt/`** removed (a language-test leftover, already in `.gitignore`).
- **SHOWCASE-PLAN.md:** where it stands, and a running order for Edward's tutorial video.
- **The first public benchmark:** enwik8 on Edward's machine, 21,675,870 bytes against 7z's
  26,080,445 (−16.89%), made in 109.3 s and unpacked in 148.7 s (Proof/STATISTICS.md).
- **IDEAS §19.103:** pieces weighed by the ways that code them, Edward's question; the first step
  needs no new version and is to be measured.

**The plan above, read against the pause.** Of the 27 October plan, step 2 (the web dictionary)
can be gathered, learnt and measured now, but would ship only as a new version if it beats
brotli; step 3 (JPEG made again from its coefficients) and step 5's sound model and zlib PNGs are
new versions, and wait. Step 4 (mixing faster when packing, "Create .sieve") needs none where the
bytes made are the same, and suits the pause, as do the benchmark on public data (SHOWCASE-PLAN)
and a short `.sieve` v31 specification for independent decoders.

### The plan from here (29 October 2026), while the format rests at v31

Edward: "Make a plan moving forward for anything else we can do without moving to a new manifest
version." The plan of 28 October below is done but for its measuring (2), the release (5), the posts
(6) and the game (7), which carry on here. Nothing below changes a `.sieve` a build makes from the
same files with the same settings, nor what any `.sieve` installs to. In order:

1. **Making large files faster, the same bytes out.**
   - *The weighing does the packer's work again.* Reading a file packs it (`file_ways`: LZMA2's
     estimate of the whole, the rope's search and code, the pieces), then weighs it, and the
     weighing calls `file_ways` on the same bytes. Only the rope's search is cached (by the file's
     SHA-256 and its settings); LZMA2's estimate of the whole and the rope's code of it are made
     twice. On enwik9 the weighing's own search took 734 s. A cache of `file_ways` by the same key,
     held within a memory budget, should take most of that off. To measure on a 100 MB cut of
     enwik9 before and after, with the log.
   - *Mixing's speed.* Unpacking enwik9 optimised for size took 1,137.9 s (0.88 MB a second), and
     mixing codes as slowly. The SSE2 sums took 13.9% of the instructions off; next, a profile of
     mix-v4 on enwik8 for its memory (each context's table row as it is fetched) and its counters,
     each change held to the same codes by the pinned SHA-256s.
   - *The rope's decoding:* the per-segment slot table is 65,536 entries whatever the symbols; for
     the byte values, 256 starts would do (the third review's measure: 0.06 s left on a text of
     1,024 columns, from 0.17).
2. **Measuring, on Edward's machine, with Keep a log ticked.** enwik8 and enwik9 again (each way's
   whole time, its last new best, the weighing's search, and the save); Silesia both ways, file by
   file (SHOWCASE-PLAN); the ladder of cuts (`tools/bench/ladder.py`, with 7-Zip itself). For each,
   zip and 7z as the same right-click makes them, as for enwik9, and at their strongest beside.
3. **Where Sieve loses to 7z (WoWRevived, +0.20%).** First, the log gains a table of each file: its
   size, the way chosen, what that way costs, and LZMA2's estimate. Then Edward's run says which
   files the 0.20% is in (likely the films and music, already packed), and whether the packer's
   choice there (carried as it is, or in the lzma2 stream beside others) can be made better within
   v31.
4. **The estimate (IDEAS §19.97),** measured again against the logs' whole times now that both are
   kept: on enwik8, enwik9 and the varied set; its shares learned per kind of file if they differ.
5. **Files that hold much the same (IDEAS §19.106),** the part that needs no version: whether two
   such files placed one after the other in the lzma2 stream, with a dictionary that holds the first,
   come out smaller than each carried on its own. Measured on a pair made here first (a folder of
   pictures and the same with a few changed), then on MAPS.WoW and MAPS-Larger.WoW on Edward's
   machine. Where mixing carries the files, it does not help; the delta way waits for the version.
6. **Left from the third review** (REVIEW, "Found and left"): a mixed file's claimed size checked
   against the memory before it is decoded (`sieve install` has no limit); dialog callbacks that can
   come after their screen has closed (Linux); the viewer's pictures decoded in one request (the web
   host's own protocol, not the format); the oracle refusing entries out of order.
7. **The File Locator at 1024×768:** its lower rows run into the status line (as before the third
   review); they want to scroll, or the boxes to sit beside the search row.
8. **Then, as before:** the release (0.13.77) with the measured tables and the three uses, the
   posts one audience at a time (SHOWCASE-PLAN §5), "Create .sieve" in the right-click menu once
   making is faster and its estimate holds (§19.100), and the game as a test bed when its engine runs
   (§19.104).

*Done the same day, by item:*
- 1. The weighing's repeat: `file_ways` keeps what it found for the run by the file's SHA-256 and
  settings (a quarter of the memory free, at most 2 GiB; let go when a reading ends). A 12 MB file
  in the File Locator: the weighing 6.76 s to 0.07, the whole 140.3 s to about 129.6, the same
  `.sieve`; its own address's length is counted from the bytes, not made as a number and its hex.
  Mixing: profiled (callgrind, a 128 KB text coded and decoded): per bit, 14 counters updated, 14
  stretches, two dot products and two trainings over 24 inputs, all in SSE2 already; nothing found
  that would not change a code, and on large files the bound is memory, which is prefetched a nibble
  ahead already. Left as it is. The rope's decoding: a segment of fewer than 16,384 values finds
  each symbol by halving instead of building a table of 65,536 (0.08 to 0.11 s down to 0.06 on the
  text of 1,024 columns; the rope's 100 cases the same).
- 3. Keep a log now says, for each file of a folder, what LZMA2, the rope, the pieces and mixing each
  came to and which was taken, then each group's mixed and apart sizes, and the lzma2 stream's.
- 5. Measured (IDEAS §19.106): both ways work within v31; the choice is Edward's.
- 6. All but the gzip past 4 GiB (REVIEW, the third pass, "Found and left").
- 7. The File Locator's tables stop above its buttons in a short window (1024x768: nothing
  overlaps now; the tables lose their last lines instead).
Left for Edward's machine: 2 and 4, and 3's run on WoWRevived.

What waits for the next version is IDEAS's list "Waiting for the next format version".

### The plan from here (28 October 2026), while the format rests at v31

Edward: "we should take a look through everything again and plan for what is next." Read from
this file, IDEAS (its guide at the top), REVIEW, SHOWCASE-PLAN and the statistics. Nothing here
needs a new manifest version; what does is gathered in IDEAS's list, for when the pause ends. In
order:

1. **Correctness, small and first.**
   - *The PNG writer's settings (REVIEW, second pass, item 3).* `stb_png` sets
     `stbi_write_png_compression_level` and `stbi_write_force_png_filter` under a lock to make a v25
     PNG again; `encode_png` (`tools/cli/image_io.cpp`), which makes an item's PNG, reads them without
     it. Since v30 an install decodes files at once on several threads, so an item's PNG made beside
     a v25 PNG can come out at the other's level: other bytes, refused by its SHA-256 (the install
     fails safe, but fails). The fix: `encode_png` takes the same lock. Not yet seen to happen;
     reachable by reading the code.
   - *REVIEW's smaller items:* `install_host.sh` writes the program's path into JSON unescaped; the
     harness's trial thread can outlive the program; Firefox (`moz-extension://`) is half supported
     (Edward to say whether Firefox is wanted).
   - *Done the same day,* but Firefox: the lock shared (a stress test: 1,199 of 1,200 item PNGs
     wrong before, none after), the path escaped, the trial's shared tables never destroyed, and
     `read --around --zoom 0` refused as `--at` refuses it (REVIEW, both passes). Left: REVIEW's
     first pass items 3 (measure first), 5 (filter versions written by hand, about twenty filters)
     and 6's last two (tests for the tile size and `read --around`).
   - *29 October 2026: a third review,* five reviewers at once, each finding checked before it was
     fixed (REVIEW, "The third pass"): a manifest's or a map's root could put files outside the
     folder chosen; a disk full was reported as a success; sizes in a `.sieve` could wrap round; the
     web host could be ended by one page; thirteen CI checks could never fail. Item 5 done (each
     filter finds its settings by its own name) and `read --around` tested in step 02. Edward's
     notes of the same day built (IDEAS §19.107), and his note on files that hold much the same
     (`MAPS.WoW` and `MAPS-Larger.WoW`) written up for the next version (IDEAS §19.106).
2. **The evidence, on Edward's machine** (STATISTICS has the tables ready):
   - enwik9, optimised for size and prioritised for decoding, beside zip and 7z (PPMd's published
     183,964,915 already there);
   - Silesia, the same two ways, file by file and in all (it is mixed files, where Sieve should do
     best; the published tables list each file);
   - the lower edge of where Sieve wins: one text, one picture and one program cut at 1, 4, 16, 64,
     256 and 1,024 KB, each packed by Sieve and 7z: `tools/bench/ladder.py FILE ... --sieve PATH
     [--decoding] [--out results/ladder-NAME.md]` (written the same day; tried here on GAME.md with
     py7zr standing in for 7-Zip: Sieve 13% smaller at 1 KB, 22% at 4 KB, so for text the edge may
     be well under a megabyte; Edward's run with 7-Zip itself is the measure).
   Then "where it shines" goes in the READMEs with the numbers.
3. **The packer, measured before kept** (the same format, other choices):
   - pieces weighed by the ways that code them (IDEAS §19.103); *measured the same day: the
     `.sieve` is the same size under every weighing, since mixing the whole file beats every cut;
     not kept, and what remains of it (mixing as a kind of piece) waits for a new version;*
   - making faster: the ways tried for a file (LZMA2's passes, the rope's search, mixing's trial)
     run at once where the result is the same at any thread count (IDEAS §19.99); *done the same
     day for a folder: its files worked on several at once, the same bytes at any thread count
     (`docs`, 33 files: 40.8 s → 20.1 s); a single file's time is now mixing's own;*
   - mixing faster with the same codes (IDEAS §19.102, *done the same day*: the mixers' sums and
     training in SSE2, exactly; 13.9% fewer instructions, every code the same);
   - the estimate's error, measured again on Edward's machine now the search shares are learned
     (IDEAS §19.97).
4. **For others to build on:** a short `.sieve` v31 specification on its own, enough for an
   independent decoder (SHOWCASE-PLAN §1), checked against the oracle's `unpack`. *Written the same
   day:* `docs/SIEVE-FORMAT.md`, the outer layer, head, body, streams, ways and check on one page,
   each part pointing to its rules in §12.2 and its function in the oracle; tested by a decoder
   written from the page alone (its coders borrowed from the oracle), which read three of Sieve's
   own files back byte for byte, tree and all: a folder at v30 (a program in the x86 stream, five
   files mixed together in a group) and two single files at v20.
5. **A release** (0.13.76, set 28 October 2026 to keep it apart from the last release) once 1 and 2 are done: the notes leading with the benchmark
   tables and the three uses (smaller archives, right-click extract, installers that check).
6. **Then the posts,** one audience at a time (SHOWCASE-PLAN §5), Edward's video among them.
7. **A game as a test bed** (Edward, 28 October 2026; IDEAS §19.104): once his *Alien Trilogy*
   engine (another Claude session's work) runs normally, its small files sieved, and decoded or
   streamed into a cache as the game asks for them. Measured first with a cache filled at start-up,
   which needs no new version.

**The macOS emails (28 October 2026).** Edward had been told by email that the macOS build was
failing. In the last twenty runs it failed twice (runs 68 and 80), each time the same way: the
macOS job was never given a runner (no step ran) and was cancelled about 15 minutes later, while
Windows and Linux passed. Both were pushes of documentation only, with green macOS runs either side
(run 81 passed on all three). So it is GitHub's macOS runners not being had, not Sieve's code; a run
takes about an hour and pushes often come every 15 to 30 minutes, so macOS jobs queue. The version
is 0.13.76 from the same day, to keep the next build apart from the published release. Edward
then asked for a new push to cancel the run still going, on all three systems: the workflow's
`concurrency` group does it, so only the latest commit of a branch is tested.

**WoWRevived, the first large real test (28 October 2026).** Edward's release (1.43 GB, mostly films
and music) came to 0.20% over 7z, and its installer went to 12 GB of memory. Found and fixed the
same day (IDEAS §19.105): a `.sieve` read and made through a number the file's size, and the rope's
coder holding every byte as 64 bits; the same bytes, installing in a fifth of the memory and making
in a third. Its figures are in
Proof/STATISTICS.md.

Left for later, by Edward's word or the pause: "Create .sieve" in the right-click menu (after 3's
speed), the web dictionary's learning (it may be gathered and measured now, shipped only as a new
version), and the overhead on small files (a new version; Edward: not vital now).

### For the next session (27 October 2026): where the work stopped

This session ran from mix-v2 (`sieve-manifest-v26`) to `sieve-manifest-v31`; its entries follow,
newest first. Edward closed it here, the session having grown too long.

**State.** Edward's repository has v31 (`3e7fecb`, "sieve manifest v31": every `.sieve` at the
newest, the File Locator's estimate counting the search as used, Compressed formats temporary and
permanent, and v31's archive members) and the Proof files made again. Delivered after it, as a
zip, and to be found in his next commit: `source.sieve` as a plain `.sieve` of the source folder
(`make_release.py` step 7; his release: 47,299,016 bytes, the anchors folder being most of it, which
he is content to leave). CI was green on his machine for v31.

**The plan Edward agreed (27 October 2026), and where each step is.** He asked where the next gains
would come from:
1. *Mix the shared stream.* Done, and further: v29 (the stream mixed), v30 (files mixed together in
   groups of up to 8 MiB, decoded at once), v31 (an archive's files carried one by one).
2. **The web dictionary: next.** Edward is choosing the outside pages it is made from (he was still
   looking into sources when the session closed; ask him). Where it stands: on 22 October a
   dictionary learnt from a 2014 snapshot of 90 sites, used as LZMA2's preset dictionary, came to
   0.5% below brotli on unseen pages but 6.6% above it on Edward's own small pages (IDEAS §19.92);
   since then mixing with a word list (mix-v3/v4, `D60`) carries pages of 4 KiB and more, and
   `release.html` came to 8,215 bytes against brotli's 8,147, while `sieve.html` (under 4 KiB, so
   not mixed) is 1,519 against 1,121. The steps: gather today's pages from the sources Edward
   chooses (their licences allowing it; Edward's own pages kept for testing, never for learning);
   learn from them (a) a preset dictionary for LZMA2 and (b) a web vocabulary for mixing's word
   list, or a mixing model primed on them; try mixing on pages below 4 KiB with it; measure on
   pages it never saw and on Edward's pages, against brotli, as on 22 October. A new version only
   if it beats brotli.
3. JPEG recompressed losslessly (its coefficients carried and coded better, the JPEG made again
   exactly), for the photos and pictures Sieve now carries as they are.
4. Mixing in parallel when packing (the groups already are; a single large file is not), and
   "Create .sieve" in the right-click menu (§19.100).
5. Then: a model for sound in mixing; zlib's own PNGs made again by zlib (no preflate difference);
   CI's time.

**Open from 27 October.**
- The archive format as a number in the listing (IDEAS §19.88), for v31's members and the rest.
- Mixing's word list with how often each word is used (SCOWL has none; a frequency list would be a
  new pinned file).
- The estimate's learned shares (`[locator] search_share_*`) start at -1 and are set by Edward's
  first runs; how close the next first estimate comes is unmeasured on his machine.
- A rebuilt site's `index.sieve` has new bytes (the newest instructions), so the extension says a
  remembered site's table changed, once.
- `source.sieve` no longer names `sieve.exe` and `sieve.sieve`; SHA256SUMS.txt and the release
  notes do.
- Tried and set aside today, with their figures: IDEAS §17.4 (square and circle wave filters: the
  triangle stays best), §17.5 (the map holding the 7z), §17.6 (a smaller `sieve-install`), §17.7.

**Edward's rules this session held to** (with "How Edward works" at the end of this file):
- Deliver each update as a zip of the changed files with a root `CHANGES.md`.
- Never push: commit locally, and answer each push reminder only with "Not pushing, at Edward's
  instruction." Sync with `git fetch origin main && git reset origin/main && git checkout -- .`
  whenever he says he has updated the repository.
- British English; no figure that was not measured.
- A new manifest version for each format change, every older one still read.
- Run only the CI steps a change touches (he runs the whole CI).
- Leave `docs/VOICES.md` alone.
- Never measure his anchors folder (`data/anchors/edward-james-gordon-thor110/`).
- Nothing from his personal photos goes in the repository, nor a certified register entry he once
  uploaded.
- Test pictures from the repository only.
- `pi6.txt` and his WoW game files are never committed (the WoW files are for measuring only).
- His screenshots are 1920 by 1080.

### `source.sieve`: the source folder's own instructions (27 October 2026)

Edward: `source.sieve` came out at 49,244,484 bytes with v31, as at v29 and v30. It was still a
sealed map holding the source's 7z, and v31 could only do better where Sieve made that 7z again
byte for byte, which it did not on his machine (with a stand-in for the anchors folder here, 58.5
MB of random bytes, it did: 66,614,143 to 64,742,836, little, the stand-in being incompressible).
What he expected, and asked for: a plain .sieve of the source folder. `make_release.py` step 7 now
runs `sieve locate <the committed tree> --installer source.sieve --items` on the same tree the 7z is
made from, checks it installs back to that tree file for file, and publishes `sieve-source.7z`
beside it as the baseline (five files with `metrics.txt`). `--map-with` is gone (there is no map
to make again), and so is `-mmt=1`. Measured on the source without the anchors folder: 6,185,486
bytes (v30), against the 7z's 8,006,568; step 7 took 4 min 26 s here. `source.sieve` no longer
names `sieve.exe` and `sieve.sieve`: SHA256SUMS.txt and the release notes do.

### Every .sieve at the newest, the estimate, temporary and permanent: `sieve-manifest-v31` (27 October 2026)

Edward's three: a .sieve always the latest, the estimate counted as used, and the permanent and
temporary choice (IDEAS §19.88) that was never built.
- **Newest everywhere:** `add_packed_newest` (items.cpp) for maps (`map_sieve.cpp`, temporary only,
  so a held archive comes back byte for byte), sites (`site.cpp`, no items), training runs
  (`train.cpp`), the hallway's saves (`item_save.cpp`) and music favourites (`music.cpp`, staged to
  a temporary folder; read with `installable_manifest`, so v3 ones still read). `kNewestRope`
  (pack.hpp) replaces the literal levels. `sieve map` reports the version it wrote (from the head).
  CI: a map of two 9,000-byte texts is v30 (mixed together); a site's page is mix-v4. Step 10's
  degree sample is now smaller than v5, so its check compares v5 against an explicit `--v4`.
- **Estimate:** `rope_search_spent()` (rope_wave) counts the searches' seconds taken and given;
  the File Locator learns the share of packing's and weighing's limits used (wall time past the
  sampled work, steps 1 to 3 giving the speed ratio), keeps it in `[locator] search_share_packing`
  / `_weighing` (-1 until measured), and ticks the estimate once a second while packing. Under the
  graph: `estimate: X` while it works, `took: Y` after, beside `Z across`.
- **v31:** `skm <members> <skeleton> <way>`; members `<archive>/<n>` flagged `ManifestEntry::member`
  at parse, joined after every other file (and after derivations) and made again through the `sk`
  remaker; kept in `Manifest::parse`'s result (the packer's read-back compares contents),
  left out of the tree's SHA-256, and stripped by `manifest_of_installer`/`installable_manifest`
  (`without_members`), which every installer and the map read through. Packer: `as_members` for
  7z (read_7z) and zip (zip_entries) where the data after the skeleton is exactly their files in
  order. `set_compressed_modes(temporary, permanent)` (pack.hpp) is the run's choice; the File
  Locator's two boxes set it per job; `sieve locate` from the flags. `mixed_estimate` in `worth`.
- **Measured** (on the source without the anchors folder, which the release's source holds, so not the release's own `source.sieve`,
  about 49 MB at v29 and v30 by Edward's): the 7z (single-threaded) 8,005,603 to 6,189,674; a map holding it 6,189,841,
  installed in 47 s (the 7z made again); a 7z p7zip made on four threads is not made again (so
  `make_release.py` passed `-mmt=1` for a while; dropped the same day, when `source.sieve` became the
  source tree's own instructions, below). The small docs 7z: v30 1,081,922 (not unpacked by LZMA2's
  measure), v31 976,356.
- **Oracle:** `skm`, members, tree without them; a stored zip made again fully (CI's case).

### Groups of files mixed together: `sieve-manifest-v30` (27 October 2026)

The next step after v29 (Edward: "Make a start on the next step").
- **Format:** ways `mixt <mix-v4 parameters> T<total>` (a group's first file; its code next in the
  rope stream, varint then code) and `mixn` (the group's other files, in the listing's order, each
  taking the next `size` bytes). `Manifest::v30`; head byte 30; `kMixGroupsManifestVersion`.
- **Reader (`unpack_v4`):** a `mixt` pushes one `Deferred` job (`mix4_file`) writing to the group's
  own bytes (`Deferred::out`), so the groups decode at once with the files mixed alone
  (`decode_deferred`). Files in groups are `late`: their slices are handed out after decoding, then
  their remakes run. Refused: a group not used up exactly, `mixn` with none open, another way inside
  a group.
- **Packer:** files `raw` or mixed alone (not pictures), non-empty, and not already compressed (LZMA2
  of the first 256 KiB at least 95% of it), in listing order, cut at **8 MiB**; groups mixed on
  `for_each_index` with as many at once as memory holds (about 300 MB each); kept where code + way
  + varint beat the members apart; groups of one skipped. Replaces v29's together path when on.
- **Why 8 MiB (measured, data folder, four threads):** v29 3,160,426 / 12.4 s; 4 MiB 3,154,427 /
  9.9 s; 8 MiB 3,100,643 / 14.1 s; 16 MiB 3,099,798 / 28.6 s. A group decodes on one thread
  (~0.6 MB/s), so the largest sets the install time. Packing: 94 s, 100 s, 108 s, 140 s. It's the
  packer's choice: no format change to move it.
- **Switches:** `rope_newest` 13 (`--v29` for 12; the File Locator passes 13; `items.hpp` default 13);
  Prioritise Decoding Speed turns it off. Report: `groups    N file(s) mixed together in G group(s)`.
- **Oracle:** `mixt`/`mixn` branches, its group checked used up at the end. **CI:** step 11's v29
  block pins `--v29`; a v30 block (the same five files); version ranges widened to 30.

### Files mixed together: `sieve-manifest-v29` (27 October 2026)

Step 1 of the plan Edward saw (mixing the shared stream), and what it turned into.
- **Format:** the lzma2 stream may be mixed (mix-v4): `PackedStream::mixed` holds its parameters
  (`B22`, `B22 D60`); text `stream lzma2 P B 0 mix4 <parameters>`; in bytes the dictionary byte
  0xFE, then a varint and the parameters. `unpack_stream` mixes it back (the word list through
  `mix_words()`). Reader checks: only the lzma2 stream, never a picture's parameters.
- **What it is used for:** in real instructions that stream is small (every file over 4 KiB is mixed
  on its own: tools' stream 79 KB of 1.36 MB), so mixing it alone gained 0.3% to 0.6%. The gain is
  in mixing **together** the files mixed one by one: just before the streams are built, those not
  carried as pictures are tried in the stream with the rest (`stream_of(true)`, the same order the
  stream is built in), mixed once; where that beats their codes plus LZMA2's estimate of the stream
  without them, they become `raw` and that code is used (`premixed`), kept only if smaller than
  LZMA2 of the stream. Else the stream alone is weighed (4 KiB to 16 MiB, the 3% start test).
- **Measured:** tools 349,689 to 260,529; client 336,403 to 247,045; docs 1,045,190 to 973,631;
  tests 401,156 to 396,337. Packing about 25% longer; installing slower where files were decoded at
  once (docs 1.1 s to 4.2 s): one stream decodes in one piece. Held to 16 MiB (so not the release).
- **Switches:** `rope_newest` 12 (`--v28` for 11; the File Locator passes 12); Prioritise Decoding
  Speed turns it off. Report: `stream    the files carried together mixed by mix-v4 (...)`.
- **Oracle:** reads 0xFE and ` mix4 `, and mixes the stream back with its `mix4_file` (slow in Python:
  CI's fixture is five small files). **CI:** step 11's v28 block pins `--v28`; a v29 block.
- **Next (IDEAS §19.102):** several mixed streams decoded at once (no 16 MiB limit, install as quick
  as before), and already-compressed files kept out of them.

### mix-v4: the SSE stages retired (`sse-v1`), `sieve-manifest-v28` (27 October 2026)

- **Why (Edward's call):** after the SSE rows were prefetched, the two stages cost 9% to 29% of
  decoding for 0.07% to 2.3% of the size (the first stage alone kept most of it). Edward: retire both
  as one filter with its figures kept. They are in FILTERS-CONFLICTS' retired list (a stage of
  mixing, not a line filter: one place for every retired thing), beside the code in `mix.cpp`
  (`Apm`, marked RETIRED), and in SPECIFICATIONS §12.2.
- **Core:** `Model2` takes `sse` (true for mix-v2 and mix-v3, which must still be read so); mix-v4
  (`Mix4Params`: `B22`, `B22 I949x1`, `B22 D60`; `mix4_code`/`mix4_file`) runs without, its SSE
  tables not even made. mix-v2's and v3's codes unchanged (their pins).
- **v28:** a way `mix4 ...`, tried in v2's and v3's place by the same tests; `rope_newest` 11
  (`--v27` for 10; the File Locator passes 11). `sieve mix` makes mix-v4; `--v2` gives v2 or, with
  `--words`, v3. Report: `mix-v4[, P as picture(s)][, W with a word list]`.
- **Measured as instructions:** Don Quixote 533,148 bytes (v27 527,837), installed in 5.7 to 6.0 s
  (v27 6.8 to 7.2 s); the ten PNGs 217,170 (v26/v27 210,046), on one thread 24.6 s (27.4 s), on four
  9.1 to 9.7 s (9.6 to 10.0 s): four mixings at once are held back by the memory's speed instead.
- **Oracle:** `mix4_file` (`mix-unpack --v4`), mix-v2's decoder with `sse=False`; reads v28.
  **Unit test:** `test_mix4`, three codes pinned. **CI:** step 11's v26 and v27 blocks pin `--v2`
  and `--v26`/`--v27`; a v28 block (a text with the list and a guessed picture in one folder; Sieve,
  the oracle, sieve-install). A folder's report goes to standard error: `2>&1` there.

### mix-v3: a word list as a model (Edward's grammar), `sieve-manifest-v27` (27 October 2026)

Edward's "grammar as a model", measured three ways in a scratchpad copy of mix-v2 (environment
switches), every file decoded back:
- **Grammar as contexts, not taken:** Moby's parts of speech (`moby-pos-v1.tsv`) of the last two
  words with the word so far; the last word's with the byte before; the last word with the word so
  far. 0.15% to 0.55% on Don Quixote, and larger on small texts (the orders already see it).
- **The word list as a prediction, taken (mix-v3):** from the word so far, the letters SCOWL has
  next (and the end), each by its count of words, give each bit a probability: an input of its own,
  and its place in the word a context. Don Quixote's first 800 KB 191,190 to 187,650; a 3 KB text
  1,303 to 1,212; a word list 41,770 to 27,138; a PDF 0.45% larger, so the packer codes a text's
  start both ways and keeps the smaller. Sizes 35, 60 and 80 within 0.2%: 60 (`scowl-en-60`).
- **Core:** `MixDictionary` (a trie with each node's counts summed in byte order, so a bit's counts
  are two look-ups, the same numbers as summing each letter), `Mix3Params` (`B22 D60`),
  `mix3_code`/`mix3_file`; mix-v2's model with the list given (one context more, one input more).
  mix-v1's and mix-v2's codes unchanged (their pins).
- **Where the list comes from:** a hook (`set_mix_words`, `mix_words()` in `locate.hpp`), set by
  `use_item_maker()` (sieve, the hallway and sieve-install all call it) to `mix_word_list`
  (`items.cpp`): the registry's file, its SHA-256 checked, made ready once a run. A reader without
  it says to use Sieve.
- **v27:** a way `mix3 B<bits> D60`; `rope_newest` 10 (`--v26` for 9; the File Locator passes 10).
  `sieve mix --words 60`. Report: `mix-v3 (with a word list)`, or how many by each version.
- **Measured as instructions:** Don Quixote 527,837 bytes (v26 532,501); a folder of a word list,
  two texts, the teapot and Don Quixote's start, 72,692 (v26 88,341).
- **Oracle:** its own trie and mix-v3 (`mix-unpack --words 60`), counting letter by letter; matched
  on its first run, and reads v27. **Unit test:** `test_mix3` (a small list made in the test), one
  code pinned. **CI:** step 11's v26 block pins `--v26` for its text; a v27 block (Sieve, the
  oracle, sieve-install).
- **Mixing's 4 KiB limit:** a text under it is not mixed, though mix-v3 is much smaller there (a 3 KB
  text 1,212 bytes; LZMA2 about 1,500). See CHANGES for the measurement and Edward's call.

### Prioritise Decoding Speed, and decoding faster (27 October 2026)

Edward: a box for decoding speed over footprint (for comparisons, measuring and recording, and where
install time matters), and decoding made faster where it can be.
- **The box** (File Locator, key P; `[locator] prioritise_decoding`; `--prioritise-decoding` for
  sieve and the hallway): the packer tries no mixing (`sieve::prioritise_decoding()`, in
  `core/mix.*`, beside the rope's every-wave switch). Measured decoding: the rope 15 to 20 MB/s,
  LZMA2 faster, mixing 0.3 to 1 MB/s. The ten PNGs: 247,692 bytes installed in 0.6 s (v26, mixing:
  210,046 in 28 s). The report's line: `decoding  prioritised: mixing not tried on N file(s)`.
  No format change. The File Locator remakes cached instructions when either newer box changes.
- **The boxes wrap** to a second row where the window is too narrow (`search_row` returns the last
  row's y; Tailor goes under it). Screenshots at 1920 by 1080 (Edward's size) fit on one row.
- **Installing decodes files at once:** the rope's and mixing's files are deferred while the
  listing is read, then decoded on `pack_threads()` threads (SIEVE_PACK_THREADS; now declared in
  `pack.hpp` with `memory_free()`), no more begun while those under way hold half the memory free;
  remakes after, in order; a failure reported as the first file's. The same bytes at any count. The
  ten PNGs: 28.3 s to 9.1 s on 4 cores.
- **Mixing faster, the same codes** (the pinned SHA-256s unchanged): counters moved by a reciprocal
  multiplication in place of a division (checked exact for every pair it meets), the tables aligned
  to 64 bytes a slot and each slot prefetched, and the SSE stages' rows prefetched before mixing
  (their contexts are known then): decoding best of five, Don Quixote's first 800 KB 2.35 to 2.16 s,
  the Vitruvian picture 3.97 to 3.27 s, the screenshot unchanged. Measured and not kept: a fixed
  input count of 18 (a picture's 15 padded): no faster, and the screenshot 20% slower.
- **Where the rest of the time is:** without the SSE stages mixing runs about 40% faster (they cost
  0.07% on the Vitruvian, about 1.7% on text): a format change, for a later version if wanted.

### mix-v2: mixing existing methods in a new way, `sieve-manifest-v26` (27 October 2026)

Edward's IDEAS §19.102, its "Next" taken up. Every part measured on the repository's own files
(scratchpad prototype first), taken only where it paid; the rest is in IDEAS as not taken.
- **mix-v2 (`core/mix.*`, `sieve mix` now makes it; `--v1` for mix-v1):** a match model (what LZ
  does, and v1 lacked), the column and field (tables), sparse contexts, two mixers, two SSE stages,
  counters in slots of 16 a half-byte. For a **picture** (parameters `B22 I<w>x<c>`, `sieve mix
  --picture WxC`), Edward's triangle and its neighbours replace the column and sparse contexts.
  About 0.3 MB/s both ways, as v1; about 240 MB at B22 (v1 128 MB).
- **v26:** a way `mix2 B<bits>[ I<w>x<c>]`, carried as v24's `mix`. The packer tries mix-v2 in
  mix-v1's place (`rope_newest` 9; `--v25` gives v1 back). A picture's shape: the rope's best wave
  where it reads the file as one (L8, P2 to P4, W a multiple of S, two lines or more), else
  `picture_shape` guesses from the bytes (non-text only; the step 1 to 4 with the least difference,
  then a row of 8 to 16,384 pixels) and the start is coded both ways, the smaller kept. The guess
  exists because the rope's search misses the width of small pictures (64 rows of the Vitruvian
  centre: it chose W1).
- **The File Locator** passes 9 now (three places in `client/file_locator.cpp`).
- **Measured:** Don Quixote 532,168 bytes (v1 565,596); the Vitruvian picture 727,842 (rope-wave-v6
  762,766); a screenshot's pixels 62,925 (LZMA2 93,936). Sieve instructions: Don Quixote 532,501
  (v25 565,664); the ten PNGs 210,046 (v25 247,692).
- **Install time, the trade-off (told to Edward):** Don Quixote installs in 8.4 s (v25 11.7 s:
  mix-v2 decodes faster than v1); the ten PNGs in 28.3 s against v25's 0.75 s (their 24.8 MB of
  pixels, carried by the rope and LZMA2 in v25, are decoded by mixing at about 1 MB/s). Packing
  the same (271 s, v25 262 s). The packer still keeps the smallest; weighing install time too would
  be a small change there, if Edward wants it.
- **Not taken:** LOCO-I's bias correction for the rope (0.4% on the Vitruvian; the screenshot twice
  as large), a dictionary read first, two-rate counters, 2^23 tables (IDEAS §19.102).
- **Oracle:** its own mix-v2 from SPECIFICATIONS §12.2 (`mix-unpack --v2`); it matched Sieve's codes
  on its first run (text, grey and RGB pictures), and reads v26.
- **Unit test:** `test_mix2`, two codes' SHA-256 pinned (a text and a 60 by 40 RGB picture):
  Windows and macOS must make the same bytes (check step 01 there). A check that v2 beats v1 on the
  synthetic words was dropped: on words drawn at random (nothing ever repeats) v2's faster counters
  lose a little to v1's, as expected.
- **CI:** step 11's v23, v24 and v25 blocks pin their versions with `--v25` (mix-v2 wins on their
  files otherwise); a v26 block (Don Quixote's start, and 64 rows of the Vitruvian centre as a
  guessed picture), Sieve and the oracle.

### Optimise Compressed Formats, and Try every way (26 October 2026)

- **The box renamed (Edward):** "Optimise Archive Formats" is "Optimise Compressed Formats" now (a
  PNG is not an archive; the mechanism, a known compression undone and made again exactly, is the
  same). The ini key `optimise_archives` and `--optimise-archives` stay, the new flag
  `--optimise-compressed` beside it. Older entries below keep the old name.
- **Try every way:** a box beside it in the File Locator (key E; [locator] search_every_wave,
  `--search-every-wave`): the search's shortcuts off: the rope's early stops (on text, on bytes
  already packed) and mixing's limits (4 KiB to 16 MiB, its test on the first 256 KiB). Off by
  default, and not implied by no time limit (Edward and Claude: an unconstrained search finishes
  anyway, and the shortcuts skip only what never won where measured; with them off every plain
  locate, CI and the release would take far longer). It never makes instructions larger.

### Towards the minimum address: v23, v24 and v25 (26 October 2026)

Edward's roadmap after the filter ideas (IDEAS §19.102): the triangle, mixing, generators. All three
built, each a new instructions version, the older ones read as before.
- **v23, rope-wave-v6 (Edward's triangle):** `P4` (the median edge detector: the value before, the
  line before, the corner) and ` T1` (what is left coded in nine buckets by how busy the triangle
  is). The search tries both at its widths. Vitruvian raw 762,766 bytes (before, 831,022).
  `--v22` keeps to rope-wave-v5. Oracle: its own rope-wave-v6. Fixture: `tests/rope/`
  (`vitruvian-centre-256.gray`, raw, so no JPEG decoder's version can change it).
- **v24, mix-v1 (`core/mix.*`, `sieve mix`):** bitwise context mixing, integers only; a way
  `mix B<bits>`, its code in the rope stream. Tried on files of 4 KiB to 16 MiB whose first 256 KiB
  mixed beat LZMA2's estimate by 3%. Don Quixote 565,664 bytes of instructions (v23 683,195);
  0.4 MB/s both ways. `--v23` turns it off. Oracle: its own mix-v1 (`mix-unpack`). A unit test
  pins one code's SHA-256 (Windows and macOS must make the same bytes: check step 01 there).
- **v25, PNGs (Optimise Archive Formats):** stb_image_write's own PNGs as their pixels (recipe
  `stb <level> <filter> <w> <h> <c> <size>`, format 19), any other PNG by its skeleton with a new
  segment, kind 5, a run of IDAT chunks (CRCs worked out again). Ten repository PNGs 657,221 →
  247,695 bytes. `--v24` turns it off. Oracle: reads both, names them not made again.
- **The newest switch inside the packer:** `rope_newest` now runs to 8: 6 the rope's v6, 7 and
  mixing, 8 and PNGs (`add_packed` clamps it to 5 wherever the newest instructions are not made).
- **Fixed on the way:** the oracle's `instructions_bytes` read a v22-or-later manifest file (head in
  bytes) as a .sieve address; it now knows heads 16 to 25.
- **CI:** step 11's v23, v24 and v25 blocks; step 10's rope checks allow rope-wave-v6. Steps 01, 10
  and 11 run here (Linux).

### A RAR made again by the machine's own rar: `sieve-manifest-v22` (25 October 2026)

- **What:** with Optimise Archive Formats, a RAR 5 that the packing machine's `rar` gives back byte
  for byte is carried as its contents with a recipe: `ur` (version, SHA-256, how, switches) and one
  `um` line a member (attributes, times in ns, path) after its `u` line. Installing makes it again
  where the same `rar` is found and checks it; else its contents and a **warning**.
- **Notes from one place (Edward: "if the CLI is the one reporting errors or warnings, hallway can
  always detect and present them"):** `install_tree` returns `InstallNote`s; `sieve install` prints
  them (`note` / `warning`), `sieve-install` shows them on its last screen and pops warnings up
  (`SDL_ShowSimpleMessageBox`; to the error output with `--yes`), the File Locator adds them to its
  status and pops warnings up on the next frame. The v14 "carried as its contents" note moved there.
- **Code:** `tools/cli/rar_remake.hpp/.cpp` (RAR 5 headers read after RARLAB's published
  description, the switches they give, finding `rar` and its version, setting times and
  attributes, running it in a folder); the packer's RAR branch in `pack.cpp` (before libarchive's
  contents); `finish_archives` in `locate.cpp`; `--v21`.
- **Tested:** `ci/stand_in_rar.py`, a RAR 5 writer of stored files, plays `rar` in step 11 (Linux
  and macOS): alone and in a folder, with and without a `rar`, by `sieve-install` and the oracle,
  and `--v21`. **Not measured:** the real `rar` (none in the cloud session: rarlab.com refused).
  Edward to try: `tools\rar_check.bat` (or `py -3 tools\rar_check.py [MY.rar ...]`) makes RARs
  from the repository's files with his rar at 14 settings (each level, solid, BLAKE2, quick open,
  creation times, times to the second, a dictionary, files alone, one file), adds any given, and
  for each packs it (with SIEVE_RAR_TRACE: each try and how many of its first bytes came out the
  same), installs it with rar, with none (the warning) and with sieve-install, then prints a table
  and writes `out\rar-check\rar-check.log`. Run here with the stand-in: 8 of the 8 it can make.

### Extract any archive, from the right-click menu (25 October 2026)

- **What:** `sieve-install` extracts every archive kind Sieve reads, not only `.sieve` files: 7z,
  zip, xz, gzip, bzip2, zstd, lz4, lzip, lzma, brotli, and through libarchive RAR 4 and 5, cab, ISO,
  LHA, cpio, ar, WARC and tar. One layer at a time, as 7-Zip's (Edward: a `.tar.gz` gives its
  `.tar`; a `.sieve` is never changed by this). Extract here keeps the archive's folders; Extract
  to a folder of its name leaves out its one top folder. Links in a tar are left out and said so.
- **One list (Edward: "dynamically access archive file formats that we add to the list in the
  future"):** `archive_kinds()` in `tools/cli/pack.cpp`; `sieve-install --archive-extensions`
  prints its names; `context/install_context.bat` registers the Sieve submenu on each of them, so a
  kind added to the table (and to `read_any_archive`) reaches the menu when the script is run
  again. `uninstall_context.bat` removes Sieve from every kind of file it is on.
- **Code:** `read_any_archive`, `leave_out_top_folder`, `unpack_archive` (pack.hpp/.cpp; the
  writer refuses files already there, checks each with the vault and removes what it wrote on a
  cancel or failure); `zip_entries` and `libarchive_entries` gained an extract mode (top folder
  kept; tar and zip through libarchive; a name libarchive cannot convert in the "C" locale taken as
  held where it is UTF-8). The packer's own use of them is unchanged.
- **CI:** `ci/extract_check.py` (step 10): every kind made from the repository's own files and
  libarchive's test archives, extracted unattended and compared.
- **The WoW work** is notes only, in `engine/` (Edward: out of the main architecture; nothing of
  the game's kept).
- **Then:** RAR made again by the machine's own `rar.exe` (`sieve-manifest-v22`, below).

### 7z, zip and gzip made again exactly: `sieve-manifest-v21` (25 October 2026)
Edward: "we should delay the filter ideas until we deal with 7z, zip and rar exactly".
- **preflate** (Apache 2.0, pinned to commit 609eefa, its last; `third_party_licenses/preflate`) for
  deflate whoever made it; **the LZMA SDK's encoder** (already in `third_party/lzma`) for 7-Zip's
  LZMA and LZMA2, and its x86 filter. One recipe, `sk`: a skeleton (bytes kept as they are, and
  where each stream goes) then the data; SPECIFICATIONS §12.2.
- **Windows builds** (first time, 25 October 2026, fixed the same day): libarchive's zlib checks
  answered beforehand, its regex search off, an empty build type given one, and `mode_t` (POSIX)
  replaced; Edward's Debug and Release builds and full CI then passed.
- **The oracle** cannot make preflate's or the SDK's streams again: it reads each skeleton whole, makes
  again those of bytes and stored data alone, names the rest, and then leaves the tree unchecked.
- **CI step 11:** a gzip zlib's levels do not give back, a zip of zlib's deflate and one stored, and
  three small archives 7-Zip made (`tests/archives`: LZMA2, LZMA, 7-Zip's deflate); `--v20` now keeps
  the v20 block's two-member gzip as contents.
- **RAR:** measured, not built (IDEAS §19.88): the machine's own `rar` gives an archive back only
  where it is the same version, with the same switches, order and nanosecond times. Edward: note it
  and move on; archive optimising is a pleasantry, not core.
- **Windows, the same day:** a second configure took the cache entries left for libarchive as a
  system zlib (now forgotten before Sieve looks); and preflate's headers use `std::result_of`, which
  C++20 removed and MSVC enforces, so preflate is built as C++17 behind `tools/cli/preflate_glue.*`.

### Every archive format we can read: `sieve-manifest-v20` (25 October 2026)
Edward: "all of the above for now, every step forward is a step in the right direction and even the
small savings will add up".
- **New libraries, each with its licence in `third_party_licenses/`:** lz4 1.10.0 and brotli 1.2.1
  (built from their C files, as bzip2), and libarchive 3.8.10 (its own CMake, nothing but the
  library, zlib its only dependency: Sieve's own, the fetched one on Windows, found through a copy
  of its header). libarchive's `COPYING` and a `NOTICES.txt` of every notice in the files built.
- **Remade exactly:** lz4, brotli (a file named `.br`) and `.lzma` files. **As their contents:** lzip;
  RAR 4 and 5, cab, ISO, LHA, cpio, ar, WARC (libarchive); several lz4 frames, gzip members or xz
  streams. Links, encrypted entries, names in a code page and multi-volume sets are refused.
- **The body in bytes** (IDEAS §19.90), with Edward's format numbers: v20 is the newest wherever the
  head is in bytes; `--v19` makes the older; maps stay v18.
- **Never larger than v19:** the first measurement found the body in bytes 1 or 2 bytes larger on
  listings of near-alike lines (the trained model's file, 4,187 against 4,185; 64-character pages,
  340 against 339). Two changes: a fifth record for `line` lines (four bytes shorter each than as
  text), and the head's flag 128, the body as text after all, the packer keeping whichever is
  smaller. Now 4,182 and 339. The CI checks both, and that the oracle reads them.
- **`SIEVE_ARCHIVES_ALWAYS=1`** (environment, for testing): every archive read is carried as its
  contents whether or not that is smaller, so CI step 11 checks each reader on libarchive's own small
  test archives (`tests/archives`, BSD, with a README).
- **A fetched-zlib build checked here** (`-DSIEVE_FETCH_COMPRESSION=ON`, as Windows builds): libarchive
  found `ZLIB::ZLIBSTATIC`, and an MSZIP cab read.
- **Corrected:** I had said the rope's remaining gap on `pi6.txt` was mostly its tables; they are 76
  bytes. The gap is the shared tables' grouping (IDEAS §19.87).
- **Not yet:** encoders to remake 7z, zip and RAR; deflate by other than zlib; the temporary and
  permanent choice for game formats (IDEAS §19.88).

### Third-party archive decoders: bzip2 and zstd, `sieve-manifest-v19` (25 October 2026)
Edward: "make a start on including third party archive format decoders, making sure we always
include any licenses or readmes that might be necessary for them".
- **Built:** libbzip2 1.0.8 and libzstd 1.5.7, fetched by CMake at those tags (never the system's),
  linked into `sieve_compare`. With Optimise Archive Formats, a `.bz2` and a `.zst` are remade
  exactly where they can be (`bz2` and `zst` ways) or carried as what they hold (`u` formats `bz2`,
  `zst`), written as `sieve-manifest-v19` (head byte 19), only where some file needs it.
  SPECIFICATIONS §12.2, IDEAS §19.88.
- **zstd remakes only at its own version:** a `zst` way names libzstd's version, and a Sieve with
  another refuses it and says which it needs. Moving to a newer zstd means files made before can no
  longer be remade by the newer build; keep 1.5.7 pinned unless that is weighed.
- **Licences:** `third_party_licenses/bzip2/LICENSE` and `zstd/LICENSE`, verbatim, rows in its
  README (zstd taken under BSD, not GPLv2). The rule from now on: every decoder taken brings its
  licence and any notice or readme it asks for.
- **Oracle:** remakes with Python's `bz2` and `zstandard` 0.25.0 (libzstd 1.5.7); CI step 10
  installs it with pip and checks a v19 three ways (installed, oracle, `sieve-install`).
- **CI fix found on the way:** `ci/lib.sh` now sets `set -E`, so a failure inside a function such as
  `q` stops the step; before it the v17 block's expected lines (wrongly `v14`) failed silently.
- **Small format savings wait for the next manifest version** (Edward, 25 October 2026): the
  listing's entries in bytes (§19.90) was not added to v19, which came first; it joined v20. (The
  rope's tables, measured since, are 76 bytes on `pi6.txt`: nothing to build; IDEAS §19.87.)
- **Run:** CI steps 10 and 01 alone here; the full CI is Edward's.

### VOICES.md: leave it alone (Edward, 25 October 2026)
`docs/VOICES.md` is the corpus of lines for the game's AIs to speak, not a record of what Edward
says about the work. Claude had been adding nearly all of Edward's messages there (22 to 25 October);
Edward has asked that Claude no longer change it at all. Quotes about the work go into IDEAS and
HANDOFF only. A note at the top of VOICES.md says so. Which of the lines added since then to keep,
Edward decides himself, when he has time (some fit, such as the "living web of ranges and states"
one): nothing is to be removed or tidied there by Claude.

### Step 7 of the plan: the CI in parts (Edward, 24 October 2026; built 25 October)
- **`ci/steps/NN-name.sh`**, one a step (01 unit tests to 12 the pinned model), moved word for word
  out of `.github/workflows/build.yml`, which now calls them (95 lines, from 2,299). `ci/lib.sh`:
  `python` falls back to `py -3`; `sieve_ci_no_display` for the hallway's steps.
- **`ci/run.sh [STEP...]`** (and `ci/run.bat`, through Git's bash): the steps asked for, in a fresh
  copy of the Sieve folder with the build copied in as `build/`; a log each, a failure's last lines
  shown and its copy kept. `--list`, `--build DIR`, `--keep`. `ci/README.md` says which step to run
  for which change. Run here only the steps a change touches (Edward runs the full CI himself).
- **Every step runs alone:** each was run alone, in its own copy; the one that needed an earlier
  step's file (10, `cube.obj` from 04) now makes it itself.
- **Found by it:** the default dictionary's change (24 October) had broken 03, 09 and 10, whose
  figures were the old list's. Edward: "checks should check the new default, old defaults have
  been technically been checked". They now name `scowl-en-60-names-v2` (so a later default cannot
  move them unseen), with the new list's figures: 03's count, survivor number and survivor
  1,000,000 recomputed by the oracle's `WordsRank` and the same; 09's book count is exactly 2^100
  (the cover) × the title's count (words at 64) × the pages' (words at 1,200), each from the same
  ranker (the oracle runs out of memory counting 1,200 letters of 88,886 words, so not by it).
- **Backwards compatibility (Edward, 25 October 2026):** "maintaining backwards compatibility
  until that point is ideal", the point being a system near perfect; most of it will be thrown away
  later ("when we get to making an OS, all the old work will be thrown away I would imagine"), and
  the application has no users yet that he knows of.

### Step 2: the release carries the web extension and the right-click menu (24 October 2026)
- **`tools/make_release.py`** (`EXTRAS`, `WEB_NOTE`): the release folder gets `context\` (its README
  and the two scripts) and `web\extension`, `web\host\install_host.bat`, `uninstall_host.bat`,
  `install_host.sh`, and a `web\README.txt` written for an installation (the repository's
  web/README.md is about the test site and the build). The test site stays in the repository. The
  release notes' Getting started names both.
- **The scripts find the installation's programs:** `install_context.bat` tries `..\tools\sieve-install.exe`
  first, `install_host.bat` and `install_host.sh` `..\..\tools\sieve(.exe)`; then the build, as before.
- **"This installation"** (`kInstalled`, client/node_graph.cpp) maps `context` and `web`, and
  `prompts`, which releases carried but it had left out.
- Measured on Linux (`make_release.py --skip-build`): every check passed; the extras add 44,895 bytes
  to sieve.sieve (8,164,254 → 8,209,149) for 64,098 bytes of files; `install_host.sh` from inside the
  release registered its `tools/sieve`, and `check_host.py` decoded through it. Not yet: the same on
  Windows (Edward), and an actual release (Edward: once the plan is done).

### The plan from here (24 October 2026)
Edward asked for the next most important steps, from the documents. In order:
1. **Making `.sieve` files, measured and made faster** (IDEAS §19.97, §19.99, §19.100). Everything
   that makes instructions (the File Locator, `sieve locate`, `sieve site`, a future Create .sieve)
   waits on how long it takes. A fixed benchmark set and a table of seconds per stage and per kind
   of file; the estimate's bias (15 to 20% low) corrected from it; then threads for the pieces'
   blocks and boundaries, LZMA2's full passes and the rope's ways, the result the same at any
   thread count, the count from the cores and the memory free.
2. **A release** with what has been built since 0.13.75 was set (the full installer, the
   right-click menu, `sieve site`, the extension, the newest installers by default): `context/` and
   `web/` in the release, `sieve-install`'s size measured on Windows, and the release notes.
3. **The long-open correctness defects** (IDEAS §2.1 to §2.3): entropy against each alphabet's own
   baseline, padding not counted as content, short all-caps tokens kept out of the cased
   dictionary. Each a new filter version, the old kept.
4. **The web, phase 2** (web/README.md, Next): the extension reading `site.sieve`, so pages that
   link to one another are checked against one hash; `sieve site check` against a server (the
   crawler); a `.sieve` of many files shown as a listing; links from the server's root.
5. **The README's screenshots** (Edward's, with the old pictures moved out; Edward, 25 October 2026:
   "Leave step5 to me, I need to take some time to do that and present it myself").
6. **Then, from the open list below:** the listing's entries in binary (§19.90), the rope's last
   gap on `pi6.txt` (§19.87), archives not yet optimised (§19.88), larger models (§19.86), the
   Encryption Engine's stage 2 (§19.76), and the WebAssembly decoder (§19.92).
7. **The CI in parts** (Edward, 24 October 2026): a script and folder for each step, so each can
   be run alone, as a change needs it, and the full CI kept for checking everything at once.

### For the next session (20 October 2026)
- **The website gallery (IDEAS §19.84) waits.** Edward moved every picture out of `Images/Sieve`'s
  top level on purpose and will sort them later, so the Build or Buy preview has nothing to show
  yet.
- **CI is green on Linux, Windows and macOS (22 October 2026),** at the repository root
  (`.github/workflows/build.yml`, its steps in `Sieve/`). The rounds that got it there are in the
  entries above (21 October). When a run fails, the log names the step's line
  (`::error::the step stopped at its line N`); for macOS, build here with clang and libc++ first
  (the command is in "A libc++ build here" above).
- **Still open, in IDEAS order:**
  - §19.79 is done, but voice in CHAT;
  - §19.80 (tabs on every line, CHAT left out for now: §17.2), §19.81 to §19.83 (models), §19.82
    (`.sieve` recipes and keys split by secret sharing) and §19.85 (skeletons);
  - the rest of Encryption Engine stage 2: a layer keyed by a picture (§19.76), the catalogue on
    the screen;
  - larger models plugged in (IDEAS §19.86), stages 1 and 2 first;
  - the characters model's sampling experiments (§19.73: argmax in parallel per level is the
    likely reason no character comes out recognisable; untested);
  - V5S2;
  - the melody tasks SM1 to SM4.
- **Open from the compression work (19 and 20 October 2026), measured where it says so:**
  - **The rope's last gap on `pi6.txt`** (IDEAS §19.87): the byte above is done (`rope-wave-v5`,
    417,139); what is left is mostly the blank lines, one after every twenty lines.
  - **The listing in binary:** the head done (v16, 142 → 67 bytes on a small script); the
    entries still text (IDEAS §19.90).
  - **The weighing** (`--weigh`, the File Locator's table): LZMA2 and v13's pieces weighed since
    21 October; v14's archives with the box ticked not yet (contents make one file many).
  - **Archives not yet optimised:** 7z, zip and RAR remade exactly (Sieve has no encoders for them), multi-volume sets, and
    deflate not made by zlib remade exactly (the preflate route); IDEAS §19.88.
  - **The rope for the other lines' places**, and V5S2 above.
  - **Websites** (IDEAS §19.92): a shared web dictionary for LZMA2, measured against brotli on
    pages from outside this site, before any plugin; and the SHA-256 as a verifiable layer (a
    link carrying its page's hash; signatures would be a new version).
  - **7-Zip's LZMA encoder** as one more candidate for each stream (three of Edward's JPEGs still
    3 to 204 bytes over 7z); measure first.
- **How Edward works:**
  - Hand work over as a zip with `Sieve/` at its top (no `files/` folder) and `CHANGES.md` beside
    it (Edward, 17 October 2026); root website files go in, beside `Sieve/`, when they change.
  - Never push: Edward reviews in GitHub Desktop and commits. A cloud session's push is refused
    anyway (403), and the stop hook's reminders to push can be ignored; commit locally if useful,
    and deliver the zip, which is what he wants (17 October 2026).
  - Sync first with `git fetch origin main && git reset origin/main && git checkout -- .`, keeping
    the root `CHANGES.md` (it is Edward's).
  - British English and exact wording; no figure that was not measured.
  - Each metric is given with its inverse where the inverse can be known (§19.78).
  - Edward's quotes go in `docs/VOICES.md`.
  - The website's AI disclosure stays at the top of every page; the page is "Build or Buy", and it
    never sells.
  - Edward runs Debug, Release, `tools\run_tests.bat` and `make_release`; run the unit suite once
    here, plus any CI lines a change adds. Compile every changed file after the last edit, and when
    a change touches the hallway, build it whole before handing over (17 October 2026: a missing
    include reached Edward's build).
