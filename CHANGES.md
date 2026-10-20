# Changes: shared column tables, `rope-wave-v4` and `sieve-manifest-v11` (19 October 2026)

Copy `Sieve/` over the repository's `Sieve/`. No new files.

## What it does

v3 gave each of your pi6.txt's 56 columns its own table and stream, though about fifty of them
hold the same thing: digits spread evenly. Now columns that hold alike share one table and one
stream (written ` G1` after `C1` or `C2`, e.g. `L8 W56 S1 P0 K0 C2 G1`).

- The code begins with each column's group number, then has one segment per group.
- The coder chooses the groups, and the code says which they are. Each column joins the group it
  costs least to add to, or starts its own where that's cheaper. At most 256 groups.
- Readers refuse group numbers out of order.

Instructions that use it are `sieve-manifest-v11`. Older readers refuse such a way.

## Measured here

| file | before | now |
|---|---|---|
| pi6.txt, rope code | 421,306 (v3) | **418,927** |
| pi6.txt as Sieve instructions | 421,491 | **419,115** |
| CI digit table (200,000 digits, lines of 111) | 87,967 (v2) | **83,225**, its floor 83,048 |
| grey picture | 832,146 (v1) | **831,022**, the first time columns have won on it |
| colour picture | 468,078 (v2) | unchanged |

For pi6.txt, all three readers gave the file back byte for byte: `sieve install`, `sieve-install`
and the oracle.

## Where pi6.txt's remaining gap is

It's now 3,874 bytes over the floor, and it's in the layout, not the tables. Added up column by
column, the file's own entropy is 418,604 bytes, just 323 under the code. Of the rest:

- **2,500 bytes:** the last column, a space on one line and a line feed on the next. Counting by
  column can't see that it alternates.
- **About 900:** the blank lines and form feeds.

Proposed next step: take each byte's context from the byte above it (the same column on the line
before), which would see the alternation. That's in IDEAS §19.87.

## Also

- **`--v10`, `--v9`, `--v8`** (with `--items`) make the older instructions, the search limited to
  the rope's ways those versions have. CI uses them, so v9 and v10 are still made and checked now
  that the search prefers v4.
- **A CI fix:** the filter check for `clean-data-v1` also matched the retired clean-v1's note
  ("replaced by clean-data-v1"), so it failed. It now matches only the filter's own row.
- **A failing CI check, not fixed:** the CHAIN tab check (`--chain-press "Down,N,Right,Right"`,
  written 13 October) now saves `draw type-2x32` where it expects `hand`. The chain tab changed on
  17 October, and this round didn't touch it. It needs a look at what those keys should do now.
  Neither failure showed up before, because the CI has never run on GitHub (it isn't at the
  repository root).
- **Your quote** about the benchmark script is in VOICES.

## Checked here

- The full build, with no warnings.
- The unit suite: 153,210 checks, 0 failures. New checks cover G1 parsing and refusals, round trips
  with groups, an out-of-order group number refused, and the search choosing v4.
- The rope CI lines: v4 with v11 instructions, plus v9 and v10 made with the new flags, each with
  all three readers.
- The rest of the hallway step, apart from the chain check above.

**Docs:** SPECIFICATIONS §12.2, IDEAS §19.87, README, HANDOFF, VOICES, help.
