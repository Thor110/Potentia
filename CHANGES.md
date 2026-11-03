# v33 reopened: web pages mixed with the MDN dictionary (3 November 2026)

This zip also holds the earlier web dictionary measurement (results and script), in case you haven't
applied that one yet. Every file is relative to your current main.

## What's new

- **`W1`, a new setting for mixing.** Before mixing a web text (HTML, CSS, JavaScript), the model
  first reads `data/dictionaries/web-mdn-2026-11-03-64k.bin`, so the page starts with what it
  learnt. The dictionary is 65,536 bytes, learnt from MDN's CC0 example repositories. Its SHA-256
  is part of the format, so it can never change. It is tried on single files, groups of files mixed
  together, and the mixed stream, and kept only where it is smaller.
- **Small pages.** Web texts from 128 bytes to 4 KiB are now mixed too. Before, mixing skipped
  anything under 4 KiB.
- **Which files count as web text.** A file needs tags at least every 256 bytes, or braces with
  statements at least every 512 bytes, and no `#include`. Without that density rule, nine of the
  repository's Markdown files were tried with the dictionary for nothing, and docs/ took 4.6 s
  longer for the same bytes.
- **Who can read it.** Sieve, sieve-install, the hallway and the Python decoder (oracle) all read
  it, and refuse it if the dictionary is missing or differs. `--v32` makes exactly what v32 made.
- **A bug found and fixed while building it.** Two pages that v32 mixed as one group came out
  larger at first, 710 bytes against 692. One page primed alone had beaten the unprimed group.
  Groups and the stream are primed too now, which brings it to 509.

## Measured

Each page's whole `.sieve` against brotli at quality 11. None of these pages were learnt from.

| Pages | brotli | Sieve before | Sieve now | Smaller than brotli (before → now) |
|---|---|---|---|---|
| 258 from 2014 | 4,194,084 | 3,955,146 | 3,826,134 (−8.8%) | 221 → 253 |
| Your 11 | 32,518 | 33,911 | 29,171 (−10.3%) | 2 → 11 |
| MDN learning-area, 154 | 71,656 | 100,888 | 66,148 (−7.7%) | 2 → 113 |

- No page came out larger than before.
- Your site packed as one folder: 20,192 bytes at v32, 19,421 now.
- The cost: about 0.1 s for each file or group mixed this way, both when packing and when
  installing.

## Files

- **Core:** `core/include/sieve/mix.hpp` and `core/src/mix.cpp` (the `W1` setting and priming).
- **Sieve:**
  - `tools/cli/items.cpp` and `.hpp` (loading the dictionary and checking it);
  - `tools/cli/pack.cpp` (weighing it);
  - `locate.cpp`, `help.cpp` and `sieve_cli.cpp` (messages).
- **Python decoder:** `reference/sieve_ref.py`.
- **Tests:**
  - `tests/test_core.cpp`. Unit suite: 154,308 checks, 0 failures.
  - `ci/steps/11-oracle.sh`: a new check of a test-site page on its own and with a second page,
    each smaller than at v32 and installed exactly by Sieve, sieve-install and the oracle. It passes
    when run on its own; the full CI hasn't been run on this build.
- **The dictionary:** `data/dictionaries/web-mdn-2026-11-03-64k.bin`, and its README section (where
  it came from: the MDN repositories and commits).
- **Docs:**
  - SPECIFICATIONS §12.2 v33, SIEVE-FORMAT.md, METHODS.md, FEATURES.md, NEXT-VERSION.md, HANDOFF.md;
  - `results/web-dictionary-2026-11-03.md` and `tools/measure_web_dictionary.py`.

## Still to do

- Your full CI on Windows, Linux and macOS.
- (3) STATISTICS.md and the Proof samples measured again at v33.
