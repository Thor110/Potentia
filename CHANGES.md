# Packed counting tables, shared automata, faster utf8-valid (relative to origin/main 41f20f8)

Unzip `files/` over the Potentia repository root, or apply `packed-tables.patch` with `git apply`
from the repository root.

## Packed counting tables (`core/src/dfa.cpp`, `DfaRanker`)
**The waste.** Every number in a counting table used to be its own BigUint: a vector plus a heap
allocation each, even for zero. Measured on the table for every text plugin at 32 characters
(236,034 states × 33 rows = 7.8 million numbers), it held 358 MB to store 68 MB of numbers.

**The packing.** Each row is now one block of limbs, plus where each state's number starts.

| That table | Before | After |
| :--- | :--- | :--- |
| Memory held | 358 MB | 98 MB |
| Build time | about 1.0 s | 0.4 s |

**The estimate** (`table_bytes`) now follows the packed size. It's still on the safe side: 160 MB
estimated against 98 MB measured. More stacks therefore count within the same filter memory. The
pages stack at 128 characters now needs 1.4 GB, where it needed 2.0 GB.

## No more copies of the automaton
- **Counting tables:** a table holds its automaton by shared pointer rather than its own copy (25 MB
  for the table above).
- **Plugins:** each stack used to receive its own copy of every plugin's compiled automaton, tens of
  MB per stack for the dictionary plugins, across the 97 counts X runs. `compile_plugin_shared` hands
  out the kept compile instead.

## A longer table serves shorter lengths
Rows 0 to r of a table answer every length up to r.

**Shorter rankers.** A ranker at a shorter length can use a longer table, with nothing built.
`shared_table` looks for the shortest table of the same plugins at the asked length or longer.

**Merges no longer depend on length.** They're kept by the line's kind and symbols alone. An
automaton filter's automaton never depends on the length (the plugin compile cache already relied
on this).
- The books' pages (128 characters) now reuse the pages line's merge instead of merging the same
  plugins again (about 5 s).
- A table built at one length serves every shorter one.

## utf8-valid (`core/src/filekind.cpp`, `Utf8Counter`)
**Grouped sums.** Building the table, ranking and unranking each added one number for every byte
value: 256 additions of numbers up to 8n bits. A byte only ever leads to one of 9 states, so a small
precomputed table now counts how many bytes lead where, and each step is at most 9 multiply-adds.
Unrank finds the byte by binary search instead of trying them one by one.

**The estimate.** It took every number at full length. They grow with the bytes left, so on
average they're half that, and the old figure was twice the real table.

At 4,000-byte files, measured against the committed code:

| | Before | After |
| :--- | :--- | :--- |
| Table build | 1,694 ms | 74 ms |
| Rank | 225 ms | 3 ms |
| Unrank | 192 ms | 13 ms |
| Memory estimate | 138 MB | 78 MB |
| Memory actually held | 71 MB | 76 MB |

So utf8-valid now counts files about a third longer in the same filter memory.

## Overall
**X with every filter:** 10 s before this round, 5.8 s now (28.6 s before the review started).

**Counts are unchanged.** Every survivor bar and figure is identical. The screenshot after X differs
in one place: the filter memory bar now reads 1.4 GB instead of 2.0 GB, from the corrected estimate.

## Tests (tests/test_core.cpp)
- **Shorter lengths:** a ranker on a longer table gives the same count, ranks and units as a table
  built at its own length, at every length from 0 to 30. Asking for a longer length than the table
  is refused.
- **Stacks:** a shorter stack of the same plugins is served by the longest matching table in use or
  kept.
- **utf8-valid:** the existing 304 vectors, and the joint counts checked against every file, still
  pass with the grouped sums.

## Checked
- tests/test_core.cpp: 56,264 checks over both passes, 0 failures.
- Run locally, all these CI steps pass:
  - same addresses;
  - filters judge, count and rank the same;
  - models and the setup menu;
  - the bytes256 line;
  - books;
  - the whole hallway step.
