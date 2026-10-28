# A folder's files worked on several at once (28 October 2026)

Unzip over the repository root. Committed locally (not pushed). Code changed: rebuild. No new
manifest version: the same `.sieve` files, made sooner.

- `Sieve/tools/cli/pack.cpp`: each file's way (the rope, pieces, mixing's trial and code) is found
  on its own, several files at once: as many as the threads, and no more than half the memory free
  holds a mixer each (the groups' rule). Each file's own work takes its share of the threads, so a
  timed rope search keeps a core as it had alone. What each finds is taken in the listing's order,
  as one thread took it, so the instructions are the same at any thread count (but where a search's
  time limit cuts it short, as before). `for_each_index` can pass the File Locator's progress on
  (through a lock), and `pack_threads()` gives a worker its share.
- `Sieve/core/include/sieve/stop.hpp`, `Sieve/core/src/stop.cpp`: `current_progress()`, this
  thread's progress sink, for the threads it starts.
- `Sieve/tools/bench/bench_make.py`: times mixing (`pack.mix`), and `--without-own-anchors` leaves
  out Edward's own anchors folder (and the tar's copies of it).
- `Sieve/ci/steps/11-oracle.sh`: the test site's folder made on one thread and on four, compared,
  and unpacked by the oracle.
- `Sieve/docs/IDEAS.md` §19.99, `Sieve/docs/HANDOFF.md`: measured and noted.

Measured here (Linux, 4 threads), this repository's `docs` folder (33 files, 2.4 MB): 40.8 s → 20.1 s
at 1 s of search a file; with the search unlimited, 51.3 s on one thread → 23.0 s on four. The
`.sieve` the same byte for byte on one thread and four (search unlimited, and at 1 s), and installed
back identical. A single file is as fast as before: mixing it is one stream.

Checked: the unit suite, 154,218 checks, 0 failures (and 77,108 with the CPU's SHA-256); CI steps 10
(666 s) and 11 (1,771 s) passed. Step 11's new folder check was run on its own (it was added after
that run started): the same bytes on one thread and four, unpacked by the oracle.

## Files
- `Sieve/ci/steps/11-oracle.sh`
- `Sieve/core/include/sieve/stop.hpp`
- `Sieve/core/src/stop.cpp`
- `Sieve/docs/HANDOFF.md`
- `Sieve/docs/IDEAS.md`
- `Sieve/tools/bench/bench_make.py`
- `Sieve/tools/cli/pack.cpp`
