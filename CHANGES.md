# The small tasks, an archive's order, and the right-click design (29 October 2026)

Unzip over the repository root, on top of your "sieve planned work" commit. This zip includes the
archive-order zip sent earlier, so apply just this one. Committed locally as two commits (not
pushed). Code changed: rebuild everything, then run the unit suite and CI steps 10 and 11 (all run
here: see the end).

## The small tasks

- **The File Locator's save buttons wrap** (`Sieve/client/file_locator.cpp`). At 1024 wide, "Make
  an installer program..." ran off the right edge. It now goes on a second row, and the tables stop
  above to make room. Checked by screenshots at 1024×768 and 1280×720.
- **The oracle's last two refusals** (`Sieve/reference/sieve_ref.py`):
  - a text manifest of v22 or later, since those exist in bytes only;
  - a `.sieve` that begins with a zero byte, since a number's bytes have none. `sieve install`
    still takes one, as it always has.
- **gzip members past 4 GiB** (`Sieve/tools/cli/pack.cpp`). zlib counts in 32 bits, so a member
  past 4 GiB was cut at 4 GiB and its CRC-32 taken over a length cut the same way. The input is
  now given a gigabyte at a time, and the CRC-32 taken the same way.
  - A small gzip still comes back byte for byte.
  - One past 4 GiB wasn't tried here, for lack of memory.

Every item of REVIEW's third pass is now done.

## An archive's order (the zip sent earlier)

- No checkbox. Every format Sieve makes again keeps its own order in its recipe, never in the
  listing's order (SPECIFICATIONS, "An archive's order").
- The specification and SIEVE-FORMAT.md name the two exceptions to "sorted": v31's members, and an
  archive's contents carried as its contents.
- The oracle checks the order of everything else.

## The right-click design (`Sieve/docs/IDEAS.md` §19.100)

- **Create .sieve** opens the File Locator alone, at the hallway's default start-up settings, and
  saves `NAME.sieve` beside what was chosen.
- **Other compressed formats** lists what Sieve can write, to mirror Extract:
  - gz, xz, lzma, bz2, zst, lz4 and br, by its own compressors;
  - 7z, zip, tar, cpio, iso, ar and warc, by libarchive, whose writers are already in the build.
- Not rar (no free compressor exists), nor lz, cab or lha.
- To be built after making is faster and the estimate is checked against your logs.

## Checked here

Linux, GCC, a Release build of everything, with no warnings:

- the unit suite: 154,218 checks, 0 failures;
- CI step 10 (the hallway and the File Locator, 649 s) and step 11 (the oracle, archives and the
  web host, 1,727 s) passed.

## Files
- `Sieve/client/file_locator.cpp`
- `Sieve/docs/HANDOFF.md`
- `Sieve/docs/IDEAS.md`
- `Sieve/docs/REVIEW.md`
- `Sieve/docs/SIEVE-FORMAT.md`
- `Sieve/docs/SPECIFICATIONS.md`
- `Sieve/reference/sieve_ref.py`
- `Sieve/tools/cli/pack.cpp`
