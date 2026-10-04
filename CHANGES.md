# Shared counting tables, and the test fixes (relative to origin/main 303dc41)

- `this-round-only.patch` applies on top of review-batch-2.
- `files/` and `full-vs-origin-main.patch` include batch 2, since it isn't on origin/main yet.

## The test failures
The failing check was in a test I wrote in batch 1:

    CHECK(written_max_states(a.size()) * 2 <= [&] { set_filter_memory(2.0 * 1024 * 1024); return written_max_states(a.size()); }());

The right-hand side changes the filter memory to 2 MB. C++ doesn't fix which side of `<=` is worked
out first.
- **GCC (here):** worked out the left side at 1 MB first, so the check passed.
- **MSVC (yours):** worked out the right side first, so both sides were taken at 2 MB, and
  "x × 2 ≤ x" failed.

It now takes the two values on separate lines. The code under test was always right.

The same test had a second problem, already fixed in review-batch-2. The suite runs twice where
the CPU has SHA instructions (portable, then hardware SHA-256), and the test assumed it ran once.

## Shared counting tables (`core/src/filter.cpp`, `shared_table`)
The pages line and the books' title count the same plugins at the same length. The table for every
text plugin at 32 characters (236,034 states) is about 308 MB and takes about a second to build.
Each was building its own.

**While in use:**
- A table is built once while any stack still uses it.
- A second stack that needs it in the meantime waits for that build and uses the same table.
- This holds whatever the merge cache is set to, and costs no extra memory: the table goes when
  the last stack using it is done.

**Between counts:**
- The table is also kept with the merged automata, in the MERGE CACHE share of the filter memory,
  when it fits there.
- At the defaults (50% of 512 MB, so 256 MB) the 308 MB table doesn't fit. Raise MERGE CACHE to
  about 65% and it's kept, so counting the line again doesn't rebuild it.

**Effect:**
- **Memory at the peak:** X uses one 308 MB table where it used two.
- **CPU:** a second of build work saved.
- **Wall time:** X is unchanged at about 10–11 s, because the two builds used to run side by side
  on separate cores.
- **Counts:** unchanged. The screenshot after X is byte-identical.

**Also:**
- not-written's ranker now holds the plugins' table as a shared table and reads the automaton from
  it. It no longer keeps a second copy of the automaton (about 25 MB).
- Merges are now keyed by the line alone, not by the order the filters were ticked in, so two
  stacks with the same plugins ticked in a different order share too. This was a bug in batch 1's
  merge cache, which this test found.

## Tests (tests/test_core.cpp)
- **The evaluation-order fix** above.
- **Sharing:** the same plugins in either order now give the same table (one object), and a table
  still in use is shared even with the merge cache at 0.

## Checked
- tests/test_core.cpp: 56,258 checks over both passes, 0 failures.
- Run locally, all these CI steps pass:
  - same addresses;
  - filters judge, count and rank the same;
  - models and the setup menu;
  - the bytes256 line;
  - books;
  - the whole hallway step.
