# Counting memory and merge cache are settings (relative to origin/main 515b816)

- `this-round-only.patch` applies on top of the first review batch (review-batch-1).
- `files/` and `full-vs-origin-main.patch` include that batch too, since it isn't on origin/main
  yet.

## What changed
Two new GLOBAL rows under FILTER MEMORY. Both are saved in `[world]` of the application's settings.

**COUNTING MEMORY** (row 12): the share of installed memory the setup menu's counts may take at
once.
- 50% at first, from 5 to 100. Left and Right step by 5; PgUp and PgDn step by 25.
- The row shows how many counts that allows on this machine with your filter memory setting, for
  example "50% of 15.7 GB (3 at once)". It's still never more than one per core, less one for
  drawing.
- Before this, the 50% was fixed in the code.

**MERGE CACHE** (row 13): the share of the filter memory that keeps the plugins' merged automata
between counts.
- 50% at first, from 0 to 100. 0 keeps none.
- The row shows the size, for example "50% (keeps 256 MB)".
- Lowering it clears what no longer fits.

**Common to both:**
- Neither changes any count, only how fast the counts arrive. So they take effect at once, with no
  red line and no X.
- Flags: `--counting-memory PCT` and `--merge-cache PCT` on the hallway, and `--merge-cache PCT` on
  the `sieve` tool.
- The line rows moved down two (`kFirstLineRow` is 14), and CI's row numbers moved with them.

## Files
- **Core:** `core/include/sieve/plugin.hpp` and `core/src/plugin.cpp` (merge_cache_share /
  set_merge_cache_share), `core/src/filter.cpp`.
- **Client:** `client/app_settings.*`, `client/app_main.cpp` (flags, applied at start-up),
  `client/menu.*` (the rows, set_counting_share, counting_slots), `data/lang/en.txt`.
- **sieve tool:** `tools/sieve_cli.cpp`, `tools/cli/help.cpp`.
- **Docs:** `README.md`, `docs/HANDOFF.md`.
- **CI** (`.github/workflows/build.yml`): a check that both rows save (55 and 40 after a step each),
  and the moved row numbers.
- **Tests** (`tests/test_core.cpp`): the merge cache share stays within 0 to 1, and with none kept
  the count is the same.

## Screenshot
`rows.png`: the two rows under FILTER MEMORY, set to 55% and 40%.

## Checked
- tests/test_core.cpp: 28,127 checks, 0 failures.
- Run locally, both CI steps pass: the models and setup menu step (including the new row checks)
  and the whole hallway step.
