# Step 7 (the CI in parts), the first two items of step 6, and notes (25 October 2026)

## A correction first
The last zip's CHANGES.md said the extension's remembering of a site's table was "as you described
it". It wasn't: that was my suggestion. You've since decided to keep it, with "Trust the new
table", and no signing. That's recorded in IDEAS §19.92, HANDOFF and `web/README.md`.

## Step 7: the CI in parts
- **`Sieve/ci/steps/`:** one script a step, from `01-unit-tests` to `12-pinned-model`. They were
  moved word for word out of `.github/workflows/build.yml`, which now just calls them (95 lines,
  from 2,299). GitHub runs the same checks, in the same order, as before.
- **`ci/run.sh`** (and **`ci/run.bat`**, which uses Git for Windows' bash):
  - `ci\run.bat` alone runs every step (it skips the Linux-only ones on Windows);
  - `ci\run.bat 03 10` or `ci\run.bat web oracle` picks steps, by number or by a word of the name;
  - `ci\run.bat --list` lists the steps.

  It works in a fresh copy of the Sieve folder, with `out\build\x64-Release` copied in (or
  `--build DIR`). Each step gets a log; a failed step shows its last lines, and its copy is kept.
  Python is `python` where that works, else `py -3`.
- **`ci/README.md`:** which step to run for which change.
- **Every step runs alone:** I ran each one alone, in its own copy. The only one that needed an
  earlier step's file (10, needing 04's `cube.obj`) now makes it itself.

## What running the steps alone found: the CI was failing
Steps 03, 09 and 10 were failing since the default dictionary changed (24 October): their figures
were the old list's. I hadn't run the full CI after that change. As you said, the checks now test
the new default, and name `scowl-en-60-names-v2` explicitly, so a later default can't move them
unseen. The new figures were checked:
- **Step 03:** the count at 32 letters (543,295,627,560,114,847,147,360,598), the survivor number of
  "it was the best of times", and survivor 1,000,000 ("a a a ... sat ya"). The Python oracle's own
  ranker computed all three, with the same results.
- **Step 09:** the book count is exactly 2^100 (the cover) × the title's count × the pages' count,
  each from that ranker. The oracle itself ran out of memory counting 1,200 letters of 88,886
  words, so this one isn't the oracle's.
- **Steps 03, 09, 10:** the addresses that depend on the stack's id (scrambled) and the guided zoom
  (58, as step 03's 58 bits) were taken from the engine. The steps read the addresses back.

All three pass now, run alone here.

## Step 6: the first two items, measured and not built
- **The listing's entries in bytes (IDEAS §19.90):**
  - the release folder (146 files): the listing packs to 1,706 bytes as text and 1,598 in bytes,
    so 108 bytes saved of 8.2 MB;
  - a single small file would gain about 7 bytes.

  Not worth a new manifest version.
- **The rope's last gap on `pi6.txt` (IDEAS §19.87):** counted by column and the byte above, the
  model is only 435 bytes (0.10%) over the floor. 1,291 of the 1,726 bytes over the floor are the
  code's own tables. Nothing worth a new rope version.
- **Next in step 6, archives (bzip2, zstd, and xz and gzip in several parts):** these need
  third-party decoders, a new manifest version and oracle support. I'd like your go-ahead on the
  dependencies first.

## Notes
- **IDEAS §19.101:** the crawler as a card, a hardware layer, noted for the far future.
- **VOICES.md:** a note at the top says I lost sight of its purpose, and that I'll leave the file
  alone. HANDOFF says so too, and that which lines to keep is yours to decide.
- **HANDOFF:** step 5 is yours. It also records your view on backwards compatibility: keep it until
  the system is near perfect.

## Tested here
- Every CI step alone: 01 to 04 and 06 to 11 pass. 05 and 12 weren't run here: 05 builds a second
  copy, and 12 downloads its corpus.
- Not run as one full CI, as you run that.

## To apply
Copy `Sieve/` and `.github/` over. Nothing to rebuild for the CI itself.
