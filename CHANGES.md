# The shortcut seed's rerun with three more pieces (commit 6388894)

Nothing in Sieve itself changed: a tool, a results table and notes. Unzip over the repository.

## The rerun (`results/shortcut-seed-pieces-2026-11-02.md` and `.tsv`)

The same 24 files as before, with three more pieces: your skip-and-insert, the file read by
columns, and palette (the values a file uses, said once).

| File (first 32 KiB) | Before | Now | Taken | xz | Mixing |
| :--- | ---: | ---: | :--- | ---: | ---: |
| Don Quixote | 12,262 | 12,019 | palette | 11,727 | 9,988 |
| IDEAS.md | 13,685 | 13,556 | palette | 13,311 | 11,529 |
| pack.cpp | 10,492 | 10,379 | palette | 10,189 | 9,102 |
| README.md | 4,291 | 4,245 | palette | 4,195 | 3,708 |
| the teapot | 6,553 | 6,511 | palette | 5,860 | 4,767 |
| designed gradient | 13 | 13 | left | 401 | 464 |
| designed checker | 20 | 20 | none | 83 | 30 |

- **Palette:** the best transform on every text and code file, 0.4% to 2.0% smaller. Text and code
  are now within 1% to 2.5% of xz, with mixing still ahead.
- **Skip-and-insert:** used in three recipes, none the best. **Columns:** never the best. Neither
  shape fits these files, as expected, so both are kept for the layouts they describe.

## The prototype's search (`tools/shortcut_seed.py`)

- **Slow on repetitive files:** it ran on without end on the designed checker, because every
  distance was carried at every place. Only the distances just looked at are carried now.
- **Skip-and-insert matching plain runs:** a skip-and-insert whose inserted byte is the count's own
  next value is a plain run, so it's left to `run`.
- **Speed:** the checker now takes 31.7 s, with the same results, and every recipe is still decoded
  and checked.
- **The table** has a skip-and-insert column now.

## Notes (IDEAS §19.111)

- The rerun's results.
- The viewer opens ordinary pictures too, with a "Sieve" button that converts them (decoding speed
  first).
