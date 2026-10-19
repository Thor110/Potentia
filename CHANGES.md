# Changes: the rope by line, `rope-wave-v3` and `sieve-manifest-v10` (19 October 2026)

Copy `Sieve/` over the repository's `Sieve/`. No new files.

## Why v2 made your pi6.txt no smaller

Your `.sieve` was already made by v2 (it begins `sieve-manifest-v9`). v2 tried columns and correctly
kept the older answer, because your file's layout isn't regular by position:

- lines alternate between 55 bytes (five groups and a trailing space) and 54;
- after every twenty lines there's a blank line, and in every tenth of those a form feed (100 in
  all).

Each form feed shifts everything after it by one byte, so with any fixed line width a given column
drifts between digits and separators across the file.

## The fix: columns counted from the start of each line

`rope-wave-v3`, written ` C2` (`L8 W56 S1 P0 K0 C2`): a byte's column is the number of bytes since
the last line feed. Blank lines and form feeds can't shift it. It's for bytes with no wave (`L8 S1
P0 K0`). The code stores each column's byte count first, because a reader only learns which column
each byte is in as it rebuilds the file.

Your file, measured here:

| | bytes |
|---|---|
| pi6.txt | 1,111,103 |
| 7z (your figure) | 512,745 |
| LZMA2 | 505,978 |
| your previous .sieve | 490,473 |
| **the rope by line** | **421,306** |
| **pi6.txt as Sieve instructions** | **421,491** |
| the digits' floor (1,000,000 × log₂10 ÷ 8) | 415,241 |

That's 18% under 7z, and about 6 KB above the floor. That 6 KB is mostly the 56 column tables,
about 65 bytes each. `sieve install`, the oracle (its own decoder) and the small `sieve-install`
all gave the file back byte for byte.

## Also

- The search tries columns by line on any file with line feeds, with as many columns as its
  longest line (up to 4,096).
- Instructions that use it are `sieve-manifest-v10`. v8 and v9 readers refuse such a way instead
  of misreading it.
- `sieve rope` names the version it used (v1, v2 or v3).

## Checked here

- The full build, with no warnings.
- The unit suite: 153,192 checks, 0 failures. New checks cover C2 parsing and refusals, round trips,
  a table laid out like yours, and the search choosing C2.
- New CI lines for v3 and v10 with all three readers, on a table laid out like your file.
- Every earlier rope, weighing, degree-sample and model CI line again, unchanged.

**Docs:** SPECIFICATIONS §12.2, IDEAS §19.87, README, HANDOFF, help.
