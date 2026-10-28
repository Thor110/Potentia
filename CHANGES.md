# Step 1 of the plan: the small correctness fixes (28 October 2026)

Unzip over the repository root. Committed locally (not pushed). Code changed: rebuild, and run the
unit suite and CI steps 04 and 11 (both run here: see below).

- `Sieve/tools/cli/image_io.cpp`: the PNG writer's settings (stb_image_write's globals) are guarded
  by one lock that both writers take. Before, an item's PNG made on one thread while a v25 PNG was
  made again on another could be written at the other's level, other bytes, and refused by its
  SHA-256; reachable since v30, when installs began decoding files at once. A stress test (four
  threads making v25 PNGs at level 1, four making 1,200 item PNGs): before, 1,199 wrong; after, 0.
- `Sieve/web/host/install_host.sh`: the program's path is escaped for JSON (a `\` or `"`), and a path
  with a control character is refused. Tried with a folder named `we"ird\dir` and one with a tab.
- `Sieve/core/src/training.cpp` (`rope_for`), `Sieve/client/harness.hpp`: the training code's shared
  rotation tables are made once and never destroyed, so the hallway's trial measurement, which runs
  on a thread nobody waits for, cannot read them after the program's statics are torn down at exit.
  No number the training makes changes.
- `Sieve/tools/sieve_cli.cpp`: `read --around --zoom 0` is refused, as `read --at --zoom 0` was.
- `Sieve/docs/REVIEW.md`, `Sieve/docs/HANDOFF.md`: what was done, and what is left (Firefox, Edward's
  call; the first pass's items 3, 5 and 6).

Checked here (Linux, GCC, Release with the hallway and `sieve-install` built): the unit suite,
154,218 checks, 0 failures; CI steps 04 (models and training replay, 41 s), 10 (the hallway, the
File Locator and installers, 655 s) and 11 (the oracle, archives and PNGs remade, the web host,
1,810 s), all passed.

## Files
- `Sieve/client/harness.hpp`
- `Sieve/core/src/training.cpp`
- `Sieve/docs/HANDOFF.md`
- `Sieve/docs/REVIEW.md`
- `Sieve/tools/cli/image_io.cpp`
- `Sieve/tools/sieve_cli.cpp`
- `Sieve/web/host/install_host.sh`
