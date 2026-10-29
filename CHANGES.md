# A question for each file already there (29 October 2026)

Unzip over the repository root. This goes on top of the earlier zips of today. Committed locally (not
pushed). Code changed: rebuild everything, then run the unit suite and CI steps 10 and 11 (all run
here: see the end).

Nothing here changes what a `.sieve` holds or how one is made: only how `sieve-install` writes.

## What you asked

Before, extracting where files were already there wrote nothing and said which file was there; you
then went Back, ticked Replace, and everything was written over.

Now, with Replace not ticked, `sieve-install` asks about each file already there, one at a time,
before anything is written:

- It shows "Already there (2 of 3):", the file's path, its size there now and the size the
  installer gives.
- **Yes** replaces it; **No** leaves it as it is; **Yes to all** and **No to all** answer it and the
  rest; **Cancel** writes nothing.
- Keys: Y, N, A (yes to all), Shift+N (no to all), Escape (cancel). Enter answers nothing, so a key
  pressed for something else doesn't write over a file.
- From the right-click menu, Cancel closes the window, as a cancel does while unpacking. Opened
  normally, Cancel goes back to choose again.
- Ticking Replace is Yes to all before anything is asked.
- When it's done, the window says how many files were left as they were.

This works the same for a Sieve installer, a 7z carried in one, and every other archive it extracts
(zip, RAR, tar and the rest). For a RAR that is made again, it asks about the RAR by its own name,
not its pieces.

Unattended, `--yes` still refuses files already there, as before. `--replace` (new) is the box
ticked. `--answers LETTERS` answers the questions in turn, for testing: y, n, a (yes to all),
x (no to all), c (cancel).

## One change in behaviour, in the 7z writer

A cancel or a failure while unpacking a 7z no longer deletes a file that it wrote over (Replace
ticked); it removes only what it made. The Sieve installer and the other archives already worked
that way. Deleting it would have lost the file outright, the old and the new.

## Checked here

Linux, GCC, a Release build of everything, with no warnings.

- A script installed into a folder that already held three of the fixture's files, once with each
  of seven answers, for a Sieve installer, a 7z one and a zip (21 runs):
  - `nyn`, `x` and `ya`: the files answered No were left as they were, the rest written.
  - Unanswered, `c` and `--yes`: refused, with nothing written.
  - `--replace`: everything written over.
  - All 21 came out as expected.
- A screenshot of the question at 760×440: nothing overlaps.
- Fixed while there: the "To" line ran off the right edge with a long path; it now shows the
  path's last 48 characters.
- CI step 10 has a new check: one file replaced, one left as it was, then Replace over the lot.
- The unit suite (step 01, 296 s) and CI steps 10 (607 s) and 11 (1,702 s) passed.
- Every changed client and tool file compiles for Windows with MinGW (`-fsyntax-only`).

## Files
- `Sieve/ci/steps/10-hallway.sh`
- `Sieve/client/installer_main.cpp`
- `Sieve/client/unpack_7z.cpp`
- `Sieve/client/unpack_7z.hpp`
- `Sieve/docs/HANDOFF.md`
- `Sieve/docs/SIEVE-INSTALL-USAGE.md`
- `Sieve/tools/cli/locate.cpp`
- `Sieve/tools/cli/locate.hpp`
- `Sieve/tools/cli/pack.cpp`
- `Sieve/tools/cli/pack.hpp`
