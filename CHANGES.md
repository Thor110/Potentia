# enwik9 optimised for size, and "last new best" (30 October 2026)

Unzip over the repository root. This goes on top of the last two zips ("Your enwik logs", then
"enwik8 optimised for size"). Committed locally (not pushed). One small code change: rebuild the
hallway.

## Your enwik9 size run, in `Proof/STATISTICS.md`

- **Made in 6,631.6 s** (1 h 50 min), 186,081,966 bytes as before.
- **Unpacked in 1,153.583 s**, then checked and written in 5.332 s: about 0.87 MB a second.
- **Where the time went**, a new column in the table:
  - 902.9 s for the samples, reading it, and zip and 7z beside it (7z alone 834.0 s);
  - 4,496.8 s packing: the rope's search was 3,264.2 s of it, and mixing 1,115.5 s;
  - 1.1 s for the lzma2 streams (33 bytes);
  - 1,139.8 s reading it back, against your install's 1,153.583 s;
  - 90.9 s for the rest;
  - 6,631.6 s in all.
- The estimate did well this time: 1 h 20 min at first, 1 h 50 min as it went, and it took 1 h 50 min.
- Every enwik figure is now measured, the making times whole.
- I corrected a sentence of mine: LZMA2's passes did change the decoding-prioritised enwik9. They
  took the stream from 7-Zip's encoder's 211,776,158 bytes to 211,195,636. Only the rope's search
  left the result as it was.

## "last new best" (the hallway)

Your log put "last new best" at 5,400.9 s, an hour after the search's last improvement (4,179.5 s).
It came from a 33-byte stream that went from 39 bytes to 37. That is a real improvement, but it took
a moment and says nothing about the search.

A step on less than 64 KiB is still marked "new best" in the log, but no longer moves "last new
best", in the log or under the graph. The log's header says so.

Tried on a 6 MB text: "last new best" is the rope's last improvement (13.7 s), with the small
streams' firsts still listed.

## Checked here

- The hallway builds with no warnings.
- CI step 10 (626 s) passed.
- `client/file_locator.cpp` compiles for Windows with MinGW (`-fsyntax-only`).

## Files
- `Proof/STATISTICS.md`
- `Sieve/client/file_locator.cpp`
- `Sieve/docs/HANDOFF.md`
- `Sieve/docs/MANUAL.md`
