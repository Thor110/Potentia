# Stop, and the best so far, in the File Locator (22 October 2026)

## Built
- **Stop (keep the best so far)**, a button and the S key, while the File Locator works. It ends
  every step that isn't needed for a result, keeping the best found by then:
  - LZMA2's trial settings and its further passes, and 7-Zip's encoder;
  - cutting the file into pieces;
  - the rope's search;
  - the comparison with zip and 7z (shown as "not measured");
  - tailoring and the weighing.

  One LZMA2 pass is always finished, so whatever it stops with installs. The status says so while
  it finishes that pass.
- **What it's doing, and the best so far:** a line above the estimate, in forms such as "the lzma2
  stream (S bytes): best so far B bytes; packing it with 7-Zip's encoder", "the rope's search: N
  ways tried, best so far B bytes of S (L8 W1 S1 P1 K0)", and "cutting it into pieces: moving
  boundary J of N".
- **Saving writes what was made when the file was read**, with nothing packed again, so the result
  you stopped at is the one saved. It only packs again if you changed the search's time or the
  archives box since. This also takes several hours off saving a very large file.
- Saving no longer copies the whole result in memory (it did, three copies of a 1 GB file).

## Measured
- The 40 MB tar, stopped after 90 s: finished at 180 s (the LZMA2 pass under way, and the
  address), saved in 0.3 s, 27,868,382 bytes against 27,832,122 unstopped, installed byte for byte.

## Also
- `sieve/stop.hpp` (core): the stop flag and the progress line, for the thread doing the work.
- Scripted runs: `--stop-after S`, `--locate-save FILE`. A CI check saves from the File Locator,
  and saves a stopped run, and installs both.

**Checked:** unit tests, every workflow step (6 to 16) locally, and a libc++ build.

**Docs:** IDEAS §19.97, README (File Locator), HANDOFF, VOICES.

To apply: copy `.github/` and `Sieve/` over the repository's top folder. New files:
`Sieve/core/include/sieve/stop.hpp`, `Sieve/core/src/stop.cpp`.
