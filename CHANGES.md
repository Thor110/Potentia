# Working through the plan (29 October 2026)

Unzip over the repository root. This goes on top of the earlier zips of today. Committed locally (not
pushed). Code changed: rebuild everything, then run the unit suite and CI steps 03, 10 and 11 (all
run here: see the end).

Nothing here changes a `.sieve` made from the same files with the same settings: the docs folder
packs to the same `.sieve`, byte for byte, and the rope's 100 cases and the unit suite's pinned
codes are as before.

## 1. Making large files faster

- **The weighing no longer does the packer's work again.** When a file is read, its ways are now
  kept for the run, keyed by its SHA-256 and settings. The weighing then takes them as found.
  - Kept: the LZMA2 estimate of the whole file, the rope's code and the pieces.
  - It holds at most a quarter of the free memory, up to 2 GiB, and lets go when the reading ends.
  - The weighing also counts its own address's digits from the bytes. Before, it made the address
    as a number and wrote it in hex: two billion digits for enwik9, only to count them.
  - On a 12 MB file in the File Locator, the weighing went from 6.76 s to 0.07 s, and the whole run
    from 140.3 s to about 129.6 s, with the same `.sieve`.
  - On enwik9 the weighing's 734 s should mostly go. That needs your next run to show.
- **Mixing: profiled, left as it is.** Per bit it updates 14 counters and does 14 stretches, plus
  two dot products and two trainings over 24 inputs. All of it is SSE2 already, and the tables are
  prefetched a nibble ahead. I found nothing that wouldn't change a code. On large files the limit
  is memory, not instructions.
- **The rope's decoding:** a short segment finds each symbol by halving instead of building a
  65,536-entry table. On a text in 1,024 columns, decoding went from 0.08–0.11 s to 0.06 s.

## 3. Where a folder's bytes go (Keep a log)

The log now has one line per file of a folder: what LZMA2, the rope, the pieces and mixing each came
to, and which was taken. After those come each group's size mixed together against apart, and the
lzma2 stream's. Run WoWRevived with the box ticked and it will show where the 0.20% is.

## 5. Files that hold much the same (IDEAS §19.106): measured, for you to decide

Both routes work within v31 and need no new version. Both make the `.sieve` smaller for such
folders, which means changing what the packer makes, so neither is built yet.

- **Files carried as they are** already share content in the lzma2 stream, while the stream's
  dictionary reaches back over the first file. It does up to 64 MiB, the packer's cap.
  - Past that, a 30 MB pair costs 30.9 MB with a dictionary shorter than the distance, and 16.6 MB
    with one longer.
  - MAPS.WoW and MAPS-Larger.WoW (160 MB each) would want 192 MiB or more. The format allows up
    to 4 GiB.
- **Files that are mixed:** two 3 MB files mixed as one group are 44% smaller than each mixed alone.
  - The mixer's match model finds the second file in the first.
  - The packer groups only files up to 8 MiB.
  - A 160 MB pair as one group would install at about half the speed, since a group decodes on one
    thread.

First, which way MAPS.WoW is carried: the log now says.

## 6. Left from the review (`docs/REVIEW.md`)

- **Installing** refuses up front a file whose decoding would need more memory than is free, and
  says how much. Before, a few bytes of code could claim gigabytes.
- **Dialogs:** each screen now holds a `DialogOwner` (`client/dialogs.hpp`), and its dialogs name
  it. An answer arriving after the screen has gone is dropped. That could happen on Linux, where
  the system's dialog doesn't block the window.
- **The web host** keeps the `.sieve` it last read, up to 128 MiB of contents, so the pictures of a
  page don't each decode it again. Three pictures from a 30 MB one took 1.211 s, then 0.059 s,
  then 0.056 s.
- **The oracle** refuses entries out of order. Turning that on found a mismatch between Sieve and
  SPECIFICATIONS, below.

**One for you to decide:** where an archive is carried as its contents, Sieve lists them in the
archive's own order. In step 11's LHA test, `empty` comes before `dir1/`. SPECIFICATIONS says
entries are sorted by their paths' bytes. Files like that already exist, so the oracle checks no
order there. Either the specification says so, or the packer sorts them. Sorting would change those
`.sieve` files, not their format.

## 7. The File Locator at 1024×768

The tables now stop above the buttons, so in a short window nothing overlaps; the tables lose their
last lines instead. "Make an installer program" still runs off the right edge at that width, as
before; it can still be clicked.

## Checked here

Linux, GCC, a Release build of everything, with no warnings:

- the unit suite: 154,218 checks, 0 failures;
- CI steps 03 (11 s) and 10 (609 s) passed;
- step 11 (1,551 s) passed;
- the oracle reads the docs folder back with its new checks;
- every changed client and tool file compiles for Windows with MinGW (`-fsyntax-only`).

## Files
- `Sieve/client/chain.cpp`
- `Sieve/client/chat.cpp`
- `Sieve/client/cipher_screen.cpp`
- `Sieve/client/cipher_screen.hpp`
- `Sieve/client/dialogs.hpp`
- `Sieve/client/file_locator.cpp`
- `Sieve/client/file_locator.hpp`
- `Sieve/client/hallway.hpp`
- `Sieve/client/harness.cpp`
- `Sieve/client/harness.hpp`
- `Sieve/client/item_save.cpp`
- `Sieve/client/node_graph.cpp`
- `Sieve/core/src/weights_rans.cpp`
- `Sieve/docs/HANDOFF.md`
- `Sieve/docs/IDEAS.md`
- `Sieve/docs/MANUAL.md`
- `Sieve/docs/REVIEW.md`
- `Sieve/reference/sieve_ref.py`
- `Sieve/tools/cli/locate.cpp`
- `Sieve/tools/cli/pack.cpp`
- `Sieve/tools/cli/pack.hpp`
- `Sieve/tools/cli/web_host.cpp`
- `Sieve/tools/cli/weigh.cpp`
