# The last items that need no new version (29 October 2026)

Unzip over the repository root, after the small-tasks zip. This includes the last-look zip, so apply
just this one. Committed locally as two commits (not pushed). Code changed: rebuild everything, then
run the unit suite and CI steps 01, 08, 10 and 11 (all run here: see the end).

Nothing here changes a `.sieve` or an address.

## Done

- **The cipher breaker** (`core/src/cipher.cpp`) scores each guess and lets its text go. Only the
  few it shows are decrypted again. Before, it kept a whole decrypted copy of every guess, at least
  512 of them. Same guesses, same order.
- **Thread pools** (`core/src/training.cpp`, `core/src/llm.cpp`): an error in a worker thread
  ended the program. Now every part finishes and the first error is passed on.
- **CI:**
  - Step 08 shows a GitHub warning where ffmpeg is missing. Before, it said so only inside its log.
  - `pip_add` (`ci/lib.sh`), used by steps 10 and 11, installs into a virtual environment of the
    step's own where the machine's Python refuses. Debian's, Ubuntu's and Homebrew's do. Tried by
    making pip refuse (`PIP_REQUIRE_VIRTUALENV=1`).
- **The guided line** (`core/src/guided.cpp`) reuses its four working lists on each thread. They
  were made afresh for every symbol of every position. A guided compact shelf with words-v2 ticked
  went from 0.67–0.74 s to 0.59–0.60 s, with the same shelf.
- **The corridor's tile size is tested** at its two ends (`tests/test_tiles.cpp`). ctest runs it at
  2 (160 checks) and 4096 (12,858 checks). It checks:
  - the tile count and padding;
  - each unit's tile and slot, and back again;
  - the loop's wrap-around;
  - bad sizes refused, and the latch (a different size refused once a line is built).

## Measured and left

- **Threads for the weighing and the tailoring:** on the docs folder they took 0.18 s together, of
  a 28.5 s reading. The packer, which is threaded already, is the rest.
- **Links from a server's root** were done on 24 October (`--served-at`). The viewer leaves a root
  link inside a `.sieve` alone on purpose: it can't know which server it means.

## Your question: extracting where files are already there

- **Now:** nothing is written, and it says which file is there. "Replace files that are already
  there" then replaces them all.
- **A prompt** would be one question, since every clash is known before anything is written:
  - the files already there, listed;
  - then Replace them, Keep mine and write the rest, or Cancel.
- Written into HANDOFF, not built yet.

## Checked here

Linux, GCC, a Release build of everything, with no warnings:

- CI step 01, the unit suite and the two tile tests (339 s);
- CI steps 08 (2 s), 10 (635 s) and 11 (1,622 s).

All passed.

## Files
- `Sieve/CMakeLists.txt`
- `Sieve/ci/lib.sh`
- `Sieve/ci/steps/08-ffmpeg.sh`
- `Sieve/ci/steps/10-hallway.sh`
- `Sieve/ci/steps/11-oracle.sh`
- `Sieve/core/src/cipher.cpp`
- `Sieve/core/src/guided.cpp`
- `Sieve/core/src/llm.cpp`
- `Sieve/core/src/training.cpp`
- `Sieve/docs/HANDOFF.md`
- `Sieve/docs/REVIEW.md`
- `Sieve/tests/test_tiles.cpp`
