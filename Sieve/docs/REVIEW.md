# Code review

A sweep over `core/`, `client/`, `tools/`, `tests/` and `reference/` looking for defects, for
optimisations with evidence behind them, and for places where the code does not explain itself.
What was fixed is listed first; what was found and left is listed after, so nothing is lost.

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

1. **The site guard can be argued with by the page it guards.** `guard.css` hides a page until it
   is checked, but the page's scripts run meanwhile, and they share its DOM and its
   `sessionStorage`. A changed page can set `sieve-unchecked:<its URL>` itself and reload, and
   `guard.js` then skips the check entirely (not only for a page missing from the table, which is
   what "Show it anyway" is for); it can also put its own content back after the notice replaces
   it. The buttons are fixed above; the rest is a question of design. Keeping "shown anyway" in
   the extension (the background, per tab) rather than the page's storage closes the first; the
   second needs the page stopped before its scripts run, which a content script cannot do.
2. **A listing or a table larger than 1 MB.** The host sends a `.sieve`'s listing, and a site's
   table, as one message, and Chrome refuses a message from a host over 1 MB, closing the port.
   Measured: 2,000 files named `pages/page-N.html` came to 234,017 bytes, 117 a file, so about
   9,000 such files (fewer with longer names). Sending them in parts, as a file's bytes already
   are, needs `background.js` and the host changed together.
3. **`stb_image_write`'s settings are globals.** `stb_png` sets and restores them under a lock;
   `encode_png` reads them without it, so a PNG written while a v25 PNG is being made again on
   another thread can be written at that one's level and filter. Still a valid PNG, of other bytes.
4. **Still open from the first pass:** items 3, 5 and 7 below, and the last two of 6, as they were
   (item 2 was done on 24 October 2026, as `symbol-entropy-v2` and `model-information-v2`).
5. **Smaller:** `sieve web-host` recognises `moz-extension://` as its first argument, but Firefox
   gives the manifest's path first and the install scripts register Chrome, Chromium and Edge only;
   `install_host.sh` writes the program's path into JSON unescaped (a `"` or `\` in it breaks the
   file); and the harness's trial measurement runs on a detached thread that can still be running
   when the program exits.

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
5. **Filter versions are spelled out by hand** (`core/src/filters/text_m1.cpp:657-712`). Each
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
   - `read --around` has no test and no CI step anywhere, and it holds the fiddliest arithmetic in
     the tool: compact wraparound, guided zoom snapping, positional stepping.
7. **`--zoom 0`** is rejected by `read --at` (`get_positive`) and accepted by `read --around`
   (`get_u32`), for the same option with the same meaning. Pick one.
8. *Done.* `hallway.cpp` was 3,532 lines and five unrelated subsystems; it is now split by
   subsystem, as `docs/HANDOFF.md` describes.
9. **Three clang warnings**, in code this pass did not touch: unused private fields `f_` and
   `pairs_` at `core/src/filters/media.cpp:198-199`, and a variable `chars` set but never used at
   `tools/sieve_cli.cpp:1234`. GCC does not report them, which is why the tree is warning-free
   there. Each wants a look at whether the value was meant to be used rather than simply deleting.
