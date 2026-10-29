# Code review

A sweep over `core/`, `client/`, `tools/`, `tests/` and `reference/` looking for defects, for
optimisations with evidence behind them, and for places where the code does not explain itself.
What was fixed is listed first; what was found and left is listed after, so nothing is lost.

## The third pass (29 October 2026)

Five reviewers at once, each on one part (the core; packing and installing; the web host, site and
command line; the hallway and the installer; the oracle and CI), told to report and not to change
anything; each finding then checked here before it was fixed. Nothing below changes a `.sieve`, an
address or a coded stream: the unit suite's pinned codes and the rope's 100 cases (every version's
ways, coded and read back) come out as before.

### Fixed

- **A `.sieve` or a map could name where it is put.** A manifest's root was never checked, and
  `sieve-install` puts a `.sieve` in a folder of its root's name: a root of `/somewhere` or `..` put
  it there whatever folder was chosen (tried: an absolute root installed into that path). A map's
  `root` line did the same for `sieve map` (tried: a map wrote a file of its choosing outside the
  folder). Both are refused now, as any path in the listing already was.
- **A disk full was a success.** A file is written as it closes, and a small one's bytes go to the
  disk only then, after the check: installing onto an 8 KB disk said "installed 4 files ... every
  one checked" and left two of them empty, and `sieve locate --installer` left a `.sieve` cut short,
  exit 0. Every file Sieve writes is now closed before it is checked, and a `.sieve` or installer
  program that fails is taken away again; an install that fails is undone. Tried again on the
  same disk: both refused, nothing left behind.
- **Sizes that wrap round.** A `pieces` way's pieces were summed without a bound, so 1,850 of them
  could wrap the sum to the file's size and ask the rope for gigabytes, or read past the lzma2
  stream; the streams' sizes in the head were added the same way. Each is checked against what is
  left now (the reviewer's crafted files are refused with the head's own message).
- **The web host.** A page could make a reply larger than the 1 MB Chrome allows (a name or a path
  echoed back), or send a message over 64 MiB, either of which ended the host for every tab; echoed
  text is cut to 4 KB, and a message too large is read past and refused (tried: 70 MiB, then a
  request after it, answered). Listing a `.sieve` of many files summed every file's offset again
  for each (100,000 files: 19.9 s); once now.
- **The extension.** A failed fetch named its status to the page, which could so learn what is on
  servers it cannot reach itself (an intranet's addresses); one answer for every failure now. A
  `.sieve` too large for the host is refused before it is sent. A `%` in an address no longer stops
  the viewer or the guard. The remembered sites and the guard's registration change one at a time
  (two tabs at once could lose a site, or register the guard twice, which Chrome refuses). The
  viewer's pictures from one `.sieve` take its bytes as fetched for the first, for a minute.
- **`site build` and `site check`** copied the rest of a page for each script or style in it: a
  4 MB page of 4,000 scripts took 35.5 s. Linear now.
- **Memory.** Extract read an archive's contents whole with no limit, so a few kilobytes could
  claim gigabytes (a decompression bomb); each reader now stops where what it holds outgrows the
  memory free. Installing held each file twice (copied out of the contents) and, before that, the
  streams and the files and the contents at once; each is let go when done with. The packer's files
  at once are given memory for the largest file's LZMA2 estimate, not only for a mixer. Model
  files (`sieve tensors --unpack`) are read in pieces, not given the memory their header claims
  first (110 bytes asked for 2.3 GB).
- **The installer.** Its address bar: Escape while typing leaves the field, not the installer;
  AltGr (Ctrl+Alt on Windows) types its letter rather than selecting everything; a path pasted in
  quotes ("Copy as path") is the path. A folder chosen in a dialog left open is used only on the
  Ready screen. A second try starts its progress from nothing. An archive of another kind is read
  at once (400 MB: 4.6 s by the byte before).
- **The File Locator's buttons** wrap onto a second row in a narrow window (at 1280 wide the new
  "Installer program from a .sieve" was cut short; at 1024 it was off the screen). A folder chosen
  while it is busy is kept until it is free, not dropped, and its status is set under its lock.
- **The training harness:** "Go to" on a run whose result has no final address said so rather than
  ending the program; a palette of 256 colours no longer gives the levels a range of nothing.
- **The core's input checks:** a learning rate of `0.` and 64 zeros divided by zero; a window of
  more than 513 bytes read past the rotation tables; a base above 2^31 wrapped in the scramble
  (no line has one: every address is as it was); a number in a filter plugin overflowed before its
  check, and a plugin's expression could nest deep enough to end the program (256 now).
- **Smaller:** ffmpeg's pipes are not handed to another ffmpeg started meanwhile (one would wait
  for the other); a RAR's temporary copy is removed when a try fails; a RAR header's times cannot
  overflow; a map's node numbers are read whole (4294967296 was 0); the crawler caps what it reads
  and keeps watching after a pass that fails.
- **Filter versions spelled by hand** (the first pass's item 5): each filter's `make` finds its
  settings by its own name, captured where it is set, so a version raised changes both.
- **The oracle** refuses a path that leaves the folder (it wrote `../../escaped.txt` two folders up),
  a v30 group with a file of another way in its middle, a `lines K` naming a file that is not there
  or not carried raw or x86, a mask with lines past its file's, a tab in a `line` record, a line
  of the listing it does not know (a first field, a count of fields, a size such as `+6`), a path
  listed twice (or a file and a folder of one name), a stream with bytes after its end, and a
  dictionary byte a stream cannot have (254 only for the lzma2 stream, 255 not for the x86 stream
  or the body). A file not as written ends with a message, not a Python traceback (none of the
  reviewer's 35 crafted files now does). It says where files are written checked by their sizes
  only (where it cannot make one, it cannot check the tree). Its listing and skeletons are built in
  place (80,000 records took 25.8 s).
- **CI.** Thirteen checks written `! grep -q` could never fail (bash's `set -e` passes over a `!`),
  nor could the first test of a `test a && test b` line (eleven of them); two were `assert` lines
  that were always true, and one counted a hallway's output without noticing it had crashed. All
  fail as they should now. Step 11's web server is stopped however the step ends, and a port
  already taken is said rather than crawled. `ci/run.sh 1` ran steps 01, 10, 11 and 12 (by "1" in
  their names); a number now picks one step. The build has a time limit (180 minutes; a run takes
  about 70).
- **Speed:** the rope's segments have tables of the 256 byte values, not 65,536 (a 1.4 MB text in
  1,024 columns: coded in 0.02 s, not 0.14; decoded in 0.06, not 0.17; the same codes). LZMA2's
  estimate of a file runs beside its rope search (32 MB of random bytes: 22.9 s to 21.7; the
  estimate itself is most of it, and no faster without another number).

### Found and left

1. **The oracle still accepts entries out of order** (SPECIFICATIONS: sorted by their paths' bytes,
   but for v31's members, which follow their archive), text manifests of v20 and after (which exist
   in bytes only), and a `.sieve` with leading zero bytes (as `sieve install` does). None makes it
   accept a wrong file as right, since the tree is checked.
2. **A mixed file's size is taken from the instructions before its code is read** (`mix4_file`
   allocates the file's size first), so a tiny code claiming 4 GB asks for that much before it is
   refused. The web host's limit covers it there; `sieve install` has none.
3. **Dialog callbacks** (the harness, the File Locator, the node graph) can arrive after their
   screen has closed where the system's dialog is not modal (Linux's portal); on Windows the
   window is disabled meanwhile.
4. **A gzip member past 4 GiB** is not read as one (zlib's sizes are 32-bit); it is carried as it is.
5. **The viewer** still has Sieve decode the whole `.sieve` for each picture of a page in it (the
   bytes are fetched once now): a request naming several paths would do it once.

## The second pass (27 October 2026)

A fresh session, reading from the handoff down, with most of its time on what reads files from
someone else: `.sieve` instructions (installing, the web plugin's host), archives (Extract), model
files, and the browser extension. Checked by the unit suite (77,110 checks, 0 failures, a clang
Release build of the core and the `sieve` tool), the web host and v22 blocks of CI step 11 run
alone (they passed), and each fix below tried against crafted input before and after.

### Fixed

**`tools/cli/locate.cpp` — a RAR's recipe was trusted on installing.** A v22 `.sieve` names, for a
RAR made again, the switches to give `rar`, the archive's path, its folder, and every member's
path. None was checked: the switches went to `rar`'s command line as they were (`-df` deletes what
it archives), the members' paths had their attributes and times set (`chmod`, `utimensat`) before
anything was verified, and the paths could be absolute or climb out with `..`. Now, when the
instructions are read: the switches must be ones `rar::read_headers` itself writes
(`rar::switches_known`: `-m`, `-md…k`, `-s`, `-k`, `-htb`, `-ow`, `-tsm-`, `-tsm1`, `-tsc`,
`-tsa`, `-qo±`), and the archive, its folder and its members are held to the same rule as every
file's path (`leaves_folder`, which the file entries' check became). Tried: `-df`, `-ilog…`,
`../../etc/passwd`, `/etc/passwd`, `../evil.rar` and a folder of `/tmp` are each refused with a
line saying why; a recipe of `-m3 -md4096k -s -qo-` still installs.

**`tools/cli/web_host.cpp`, `tools/cli/locate.cpp` — the web host's 256 MiB limit came after the
decoding.** The limit was checked on the files once the instructions had been read, but reading
them allocates (and fills with zeros) every stream and file at the size the instructions declare.
The link's hash only proves the file is the one the page chose, and a page asks for a decode with
no click (`<img src="….sieve" integrity=…>`), so a `.sieve` of a few hundred bytes could have the
host make gigabytes. `DecodeLimit` (locate.hpp) now holds, for the length of a read, the most any
stream or all the files together may declare, checked before anything that size is made; the host
sets it to its limit. Elsewhere it is unset and nothing changes. Tried: a listing declaring an
8,000,000,000-byte file, and one declaring a stream of that size, are both refused at once; a
`.sieve` made by `sieve locate` still decodes and lists.

**`tools/cli/web_host.cpp` — the vault did not cover the browser.** Installing and extracting
check every file against the vault; the host handed decoded bytes to the browser unchecked. The
file it returns is now checked as Extract checks one (`vault::check_bytes`). Tried with a `.sieve`
holding the built-in test bytes: refused, "withheld by the vault".

**`tools/cli/web_host.cpp` — replies that were not UTF-8.** Paths and error messages can carry a
`.sieve`'s own bytes, and Chrome closes a native host that sends text that is not UTF-8, failing
every request still waiting. Every string the host sends now has each byte that does not begin a
well-formed sequence replaced by U+FFFD (the base64 parts, which are ASCII, are left alone). The
JSON writer itself is unchanged, since the safetensors layout is written through it.

**`tools/cli/pack.cpp`, `unpack_archive` — a failed Extract deleted the user's own files.** With
Replace ticked, a file already there was overwritten and then, if a later entry failed (the vault,
a full disk) or the extraction was cancelled, removed with the rest, so it was lost outright.
Installing already kept such files (`existed`); Extract now does the same. Tried with a zip whose
second entry is the vault's test bytes: before, the first file was gone; after, it is there.

**`web/extension/notice.js` — the page could press the notice's buttons.** The notice is built in
the page's own document, and `button.click()` from the page's scripts runs a listener, so a changed
site's own script could press "Trust the new table" and have its table remembered. Each button now
acts only on a click the browser marks as the reader's (`event.isTrusted`). The buttons themselves
stay as Edward decided.

**`core/src/safetensors.cpp` — a name given twice could pass.** Duplicate names were looked for
between tensors next to each other in the data's order, so `a`, `b`, `a` passed. They are now
compared by name. Two checks added to `tests/test_core.cpp`, one of which the old code passes.

**Warnings.** The tree was not warning-free at `-Wall -Wextra -Wpedantic` with GCC 13 or clang 18:
seventeen "missing field initializer" warnings on brace-initialised `PackedStream` and
`UnpackedArchive` values (now constructors, so no call site changed), an unused `sat()` in
`training.cpp` and an unused copy of `kL` in `weights.cpp` (the one `weights_rans.cpp` uses is the
real one). Neither unused definition could have been meant to be used: `training-v1` and
`sieve-weights-v1` are pinned, and using either would change their results. Both compilers are now
silent on the core and the tool.

### Found and left

1. *Done, 27 October 2026 (Edward: "Both of those fixes are necessary").* **The site guard could be
   argued with by the page it guards.** "Show it anyway" was kept in the page's own
   `sessionStorage`, where its scripts could set it, and `guard.js` then skipped the check
   entirely. It is now kept by the background, per tab (`chrome.storage.session`, forgotten when the
   tab closes), set only by the notice's button, and honoured only for a page the table does not
   name. What is left, as `web/README.md` already says: a page's scripts may run before the check
   ends, and can fight the notice once it is shown; a content script cannot stop them first.
   Found while trying it: **the guard hashed its own cached copy of the page.** Its `fetch` used the
   extension's HTTP cache, which the browser keeps apart from the page's, so on a server sending
   `Cache-Control: max-age=600` (GitHub Pages does) a page changed on the server was "checked"
   against the copy cached earlier while the tab showed the changed bytes (measured: the tab had
   3,684 bytes, the changed page). The fetch now revalidates (`cache: "no-cache"`), and the same
   case is refused. Tried in Chromium, headless, with Sieve as the host: `web/README.md`.
2. *Done, 27 October 2026.* **A listing or a table larger than 1 MB.** Both now come in parts, as a
   file's bytes do: a head saying how many, then the JSON's UTF-8 bytes as base64. Measured before:
   a `.sieve` of 10,000 files gave one message of 1,178,012 bytes; after, two parts, and the viewer
   lists all 10,000. CI step 11 checks it (`check_host.py` refuses any message over 1 MB).
3. *Done, 28 October 2026.* **`stb_image_write`'s settings are globals.** `stb_png` sets and restores them under a lock;
   `encode_png` reads them without it, so a PNG written while a v25 PNG is being made again on
   another thread can be written at that one's level and filter. Still a valid PNG, of other bytes.
   Reachable since v30, when an install began decoding files at once. Now `encode_png` takes the
   same lock (`g_stb_write`, `tools/cli/image_io.cpp`). Measured with a stress test (four threads
   making v25 PNGs at level 1 while four make an item's PNG, 1,200 of them): before, 1,199 came out
   with other bytes; after, none.
4. **Still open from the first pass:** items 3, 5 and 7 below, and the last two of 6, as they were
   (item 2 was done on 24 October 2026, as `symbol-entropy-v2` and `model-information-v2`).
5. **Smaller:** `sieve web-host` recognises `moz-extension://` as its first argument, but Firefox
   gives the manifest's path first and the install scripts register Chrome, Chromium and Edge only
   (open: whether Firefox is wanted is Edward's call); `install_host.sh` writes the program's path
   into JSON unescaped (a `"` or `\` in it breaks the file); and the harness's trial measurement runs
   on a detached thread that can still be running when the program exits.
   *Done, 28 October 2026:* `install_host.sh` escapes a backslash and a double quote, and refuses a
   path with a control character (tried with a folder named `we"ird\dir`: the JSON reads back to
   the path, and a path with a tab is refused). The trial thread holds copies of what it is given;
   the one shared thing it reads, the training code's rotation tables (`rope_for`), is now made once
   and never destroyed, so it cannot be read after the program's statics are torn down.

## The first pass

### Fixed in this pass

**`client/hallway.cpp`, `draw_face_image`** — pictures on items warped as the camera moved close
to them: SDL textures a triangle affinely, and each picture was cut into only 2x2 cells, so up
close and at a slant the texture slid across the face. The cell count now follows the face's
depth ratio (about 2% of depth change per cell) and its size on screen (48 px a cell), from 2 to
32 a side; every cell corner is placed by the true projection.

**`client/menu.cpp`** — RESET EVERY SHAPE left the picture cache where FIND MY LIMITS had put it,
so a reset did not undo a search; it now restores the default. The setup menu's actions ran into
the footer once the title-length row was added, at 720 pixels high; the menu's minimum drawing
height is now 780, scaled down to fit a smaller window.

**`core/src/filters/audio.cpp`** — the musical `Key` walked off the end of `kTonics` on an
unrecognised tonic, and dereferenced a null scale pointer on an unrecognised scale. Neither can
happen through the tool, because the parameter's list of choices is checked first, but a filter is
also constructible straight from code. Both are now bounded and throw `std::invalid_argument`.

**`core/src/corridor.cpp`, `core/include/sieve/corridor.hpp`** — the corridor's tile size was
documented as "set once at start-up, before any `LineLoop` is built, and never afterwards" with
nothing enforcing it. A `LineLoop` divides its length by the tile size when it is built and keeps
the answer, so a later change left its tile count measured in one size and its slots in another.
The first `LineLoop` built now latches the setting and a later change throws `std::logic_error`.
Checked empirically afterwards: the hallway still starts at 128 and at 256, the address and the
percentage along the line are unchanged between them, and the tile number halves.

**`client/menu.cpp`** — the filter-mode cycle scanned a four-element list for the current mode with
no bound, so a mode read back from a hand-edited settings file would have run off the end. Bounded,
and anything unrecognised is treated as Off.

**`client/hallway.cpp`** — two pieces of per-frame work that never changed between frames:

- The compass worked out how far along each of the seven lines you stand on every frame, which is a
  big-integer shift and two logarithms per line. It is now computed in `refresh_labels()`, beside
  the tile and loop labels that were already cached for exactly this reason.
- A survivor's number was converted from a `BigUint` to a decimal string on every frame the slot
  was hovered — a number that can run to thousands of digits, for an answer that never changes.
  It is now formatted once, in `book()`, and kept on the `Book`.

**`tools/sieve_cli.cpp`** — `--address-file` stripped only trailing whitespace, so a hex address
that an editor or `fold` had wrapped failed with a bare "invalid hex". All whitespace is now
ignored, since a line break in the middle of an address does not make it a different address.

**`tools/cli/help.cpp`** — `read --address-file` was undocumented, and because the unknown-option
warning is generated by scanning the help text, using the option printed a warning saying it would
be ignored when in fact it was honoured. Documented, in the usage line and the option list. The
`--at` entry also still said slots ran 0-127, which stopped being true when the tile size became a
setting.

**Comments.** Every `client/*.cpp` file now opens with a paragraph saying what it is for and why it
works the way it does — `mesh.cpp`, `menu.cpp`, `font.cpp`, `strings.cpp`, `app_settings.cpp` and
`main_menu.cpp` had none, which on a 474-line software rasteriser and a 1,337-line menu was the
most conspicuous gap in the project. The trie lookups in `text_m1.cpp` now say why they scan rather
than binary-search, and `all_start()` now says what it actually tests and how rarely it is true.

**`core/src/biguint.cpp` — base conversion, multiplication and division for long operands.** This
was the first item under "Found and left" below. Splitting a number in halves by a power of the
base is only faster than peeling a limb at a time if the multiplication and division under it are,
so all three changed together: Karatsuba multiplication from 32 limbs, division by a Newton
reciprocal with a Barrett quotient for long divisors, and conversion by halves over a ladder of
powers `base^(k·2^j)` whose reciprocals are worked out once per base and kept per thread. Every
estimate is followed by an exact correction, and the corrections are capped well above what the
mathematics allows, so broken arithmetic underneath throws instead of looping. At 152,000 bits,
on the same machine before and after:

| | before | after | |
| :--- | ---: | ---: | ---: |
| multiply | 3.40 ms | 1.01 ms | 3.4× |
| divmod (2n by n) | 7.73 ms | 4.80 ms | 1.6× |
| `to_digits(27)` | 18.20 ms | 2.90 ms | 6.3× |
| `from_digits(27)` | 2.02 ms | 1.14 ms | 1.8× |
| `to_decimal` | 14.36 ms | 3.31 ms | 4.3× |
| `pow(27, n)` | 2.00 ms | 0.72 ms | 2.8× |

At a million bits `to_digits(27)` went from 849 ms to 98 ms, and through the tool, reading back a
200,000-character page from its scrambled address went from 0.99 s to 0.27 s. The addresses and
texts are identical before and after. Nothing is slower: a single division only works out a
reciprocal when its lengths repay it, and below that it is Knuth's as before.

A new vector file, `tests/vectors_biguint_large_v1.tsv`, pins the arithmetic at the lengths where
the new algorithms run, since the existing vectors stop at 4,755 bits. It was checked by breaking
the code on purpose: a dropped Karatsuba carry fails nine vectors and throws, a misplaced split in
the conversion throws, and the CI step that regenerates vectors from the oracle now includes both
BigUint files, which it did not before.

**`core/src/bookspace.cpp`, `core/src/modelspace.cpp`** — item 4 below. The three duplicated
helpers are gone rather than moved: a book is its cover's digits followed by one run of page
digits, and a model its coordinates' digits followed by its faces', so each packs and unpacks with
`BigUint::from_digits`, `to_digits`, `pow` and one multiplication or division, and both now get
the fast conversions for free. `BigUint::pow` takes a 64-bit exponent for the purpose.

### Found and left

In rough order of value. These are the work of a session each, not of a paragraph.

1. *Fixed; see above.* `BigUint::to_digits` and `to_decimal` were O(n²).
2. **Alphabet-relative entropy baselines** — §2.1 of `IDEAS.md`. A correctness bug, not a
   performance one.
3. **`GuidedLine::table` allocates four vectors per symbol** (`core/src/guided.cpp:57-90`). It is
   called once per position from both `interval()` and `unit_at()`, so a unit of length L costs 4L
   allocations, while the caller already reuses its `buf` across the same loop. Hoist `live`, `f`,
   `rem` and `order` into scratch buffers sized once and cleared per call. Worth measuring first:
   the allocator may already be making this cheap.
4. *Fixed; see above.* Three identical helpers in two files.
5. *Done, 29 October 2026 (the third pass).* **Filter versions are spelled out by hand** (`core/src/filters/text_m1.cpp:657-712`). Each
   `FilterSpec::make` calls `find_filter("clean-v1")` with the version as a literal, while the
   version also lives in the spec's own field. They agree today, and nothing checks that they
   still agree after someone bumps one of them — which is precisely the versioning workflow the
   file's own header describes as the way to evolve a filter. Pass the spec into `make` and build
   the name from it.
6. **Test gaps**, in the order they matter:
   - *Fixed.* `canon-bytes-v1` and `bytes256` had no oracle vectors. The oracle now has both,
     written from §12.1a, and `tests/vectors_bytes_v1.tsv` pins how files are cut into units and
     each unit's positional and scrambled address, including files text canonicalisation would
     change, and UTF-8 and bytes that are not UTF-8.
   - *Found by those vectors; decided.* §12.1a says `canon-bytes-v1` is used for "any alphabet
     holding every byte value", but `holds_all_bytes` in `core/src/alphabet.cpp` accepts only an
     alphabet that is exactly U+0000-U+00FF, so a stacked `u+0000-u+00ff+greek` is refused. Either
     the specification narrows to "the alphabet of exactly the byte values", or the engine widens
     to any alphabet whose first 256 symbols are the bytes. Widening changes no existing address,
     since it only admits alphabets that are refused today. Decided: the specification stays as
     written, because the direction is for everything to consolidate on the binary line, and the
     engine widens when anchors start to need it. The vectors leave the case out until then.
   - The corridor's tile size is never exercised at anything but its default of 128. Because of the
     latch added above, a test of 2 or 4096 must set it before any line is built, so it wants a
     test binary of its own or a check placed at the very top of the run.
   - *Done, 29 October 2026, but for compact:* `read --around` has no test and no CI step anywhere,
     and it holds the fiddliest arithmetic in the tool: compact wraparound, guided zoom snapping,
     positional stepping. CI step 02 now checks positional neighbours (and their wrapping round the
     line's ends), a guided shelf zoomed out, and a zoom of 0 refused. Compact wants ticked filters
     (a filter file of its own), and is left. (The help's own example of it, `--compact --around 3
     --zoom 30 8`, is refused without one: "--compact needs ticked filters".)
7. *Done, 28 October 2026.* **`--zoom 0`** is rejected by `read --at` (`get_positive`) and accepted by `read --around`
   (`get_u32`), for the same option with the same meaning. Pick one. Both refuse it now (a zoom of 0
   puts the whole line in one book).
8. *Done.* `hallway.cpp` was 3,532 lines and five unrelated subsystems; it is now split by
   subsystem, as `docs/HANDOFF.md` describes.
9. **Three clang warnings**, in code this pass did not touch: unused private fields `f_` and
   `pairs_` at `core/src/filters/media.cpp:198-199`, and a variable `chars` set but never used at
   `tools/sieve_cli.cpp:1234`. GCC does not report them, which is why the tree is warning-free
   there. Each wants a look at whether the value was meant to be used rather than simply deleting.
