# Pre-release review (4 November 2026)

Commits 727467e (code) and 3e9a351 (docs), local and not pushed, on top of your a843c87 / 5a4a5bb line.

Six reviews ran in parallel: the changes since the v33 freeze, the reader against hostile files, duplicated code, speed, outdated documents, and what's vital before release. I checked every finding myself before changing anything.

## Vital before release (all fixed)

- **Windows CI was red.** Step 11:1098 expected a group total (T1536) that changes when a Windows checkout gives the web pages CRLF line endings. The check now accepts any total.
- **The source archive had no licence.** `make_release.py` now puts `../LICENSE` into `sieve-source.7z` and `source.sieve`.
- **The right-click menu's README described old behaviour.** It now explains that files already present are asked about (Yes / No / Yes to all / No to all / Cancel).

## The reader (installing a .sieve from anyone)

- **A stream can no longer fake its size.**
  - Before: a 30 KB file claiming its LZMA2 stream held 6 GB made Sieve allocate 6 GB of zeros (5.7 GB peak memory) before refusing it.
  - Now: the size is read from the stream's own chunk headers first. The file is refused at 12 MB.
- **Files too big for memory are refused up front.**
  - Before: a 171-byte file that honestly declares a 3 GB file installed at 5.7 GB peak memory.
  - Now: anything needing more than half the available memory is refused before anything is made.
- **A remade RAR sets safe file modes.** It never sets setuid, setgid or the sticky bit, even though the modes come from the file.
- **Remade archives budget their compressor's memory.** The parallel remakes now count each compressor's own memory (xz about 700 MiB), so they no longer start all at once and run the machine out.
- **New checks in CI step 11** cover both memory cases.

## Other fixes

- `--v32` makes v32's bytes again: the middle-of-file rope probe added after the freeze now applies to v33 only.
- Three screens (the setup menu, main menu and media player) opened file windows without naming themselves as owner. An answer arriving after the screen closed could have reached a destroyed screen.
- **Manual Navigation:**
  - The window no longer freezes while a long exact check is being dropped.
  - A log started partway through a walk now clears the redo steps it can't replay.
- The Filter Designer now reads `.sfilter` files saved with CRLF line endings.
- `--in-hex` checks every digit.
- Three column paddings can no longer underflow.
- `sieve version` names v33 as the newest; it said v31.
- The RAR helper finds Sieve's own folder on macOS.
- `hallway --help` lists all nine main menu items.
- Manual Navigation now says "1 decimal place(s)".
- **CI step 4:**
  - Its "same book" check had never checked anything: `book` was undefined there, so both sides were empty.
  - Made real, it showed the expectation was wrong: tiles of 128 and 256 begin at different books, by design.
  - It now checks that each tile begins at a multiple of its size.

## Speed (every .sieve byte-identical)

Compared with the previous build on 21 runs (7 inputs, 3 settings each), and every output was identical:
- When a file's own start test already mixed the whole file, that code is reused instead of mixing again. `core/src` went from 12.7 s to 9.9 s, and MANUAL.md from 2.4 s to 1.8 s.
- A text's four base64 weighings run at once: xml went from 26.1 s to 23.7 s.

Larger gains were found but left for after the release (IDEAS §19.120):
- Running the LZMA2 first pass alongside mixing: dickens 49.4 s → 36.5 s in a prototype. It needs a proper redesign first.
- The installer's checks are still single-threaded.

## Docs

- 26 outdated statements corrected across MANUAL, SIEVE-INSTALL-USAGE, NEXT-VERSION, SHOWCASE-PLAN, ROADMAP, FEATURES, FORMATS, the READMEs, VISION, STATISTICS, SPECIFICATIONS, ODDITIES, IDEAS headings and HANDOFF's current state.
- The third-party licence rows are corrected.
- The README gains the training command that works in an installed release.
- IDEAS §19.120 records what the review found but deliberately left for after the release: speed, Unicode names, Windows reserved names, and the duplicate-code merges.

## Tested

- Unit suite: 154,318 checks, 0 failures.
- CI steps 4, 10 and 11: all pass here. Step 10 ran on the build before the speed changes; steps 4 and 11 on the final build.
