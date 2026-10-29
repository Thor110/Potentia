# enwik9's results, and the File Locator's times made whole (29 October 2026)

Unzip over the repository root. Committed locally (not pushed). Code changed: rebuild the hallway,
then run CI step 10 (run here: see the end).

## enwik9 (`Proof/STATISTICS.md`)

Both of your runs are in, along with ratios and a comparison with PPMd:

- Optimised for size, 186,081,966 bytes: 5.37:1, 1.15% larger than PPMd. It unpacks in 1,137.9 s.
- Prioritised for decoding, 211,195,701 bytes: 13.50% larger than that. It decodes in 11.3 s, about
  89 MB a second.

enwik9 is also a row in "At a glance". The zip and 7z rows are still to be measured.

Of the two making times, 734 s and 790.5 s, neither is known to be the whole run, and the table
says so. You are right that something was off: the code shows how.

- **"took"** under the graph is the last job's time. A Save is a job of its own. A Save that
  packs nothing again ("writing the instructions made when it was read") took seconds and replaced
  the reading's time. A Save that packed again (a setting changed after reading) showed its own
  time, with its own search.
- **"across"** is only the graph's width. It starts at 1.1 × the first estimate and doubles when a
  point passes it, so it says nothing about the run.

## The File Locator (`client/file_locator.cpp`, `.hpp`, `data/lang/en.txt`)

The time a job took is now written exactly as well, past a minute: "took 3 h 05 min (11130.4 s)".
Under the graph, "last new best: X" says when the last improvement came; the search runs on to its
limit after that. A Save that packs nothing again adds the reading's time: "reading it (the search
with it) took ...". Together these give the three numbers a table wants: the whole time, the time
to the result kept, and the save.

## A layout fix (mine, from the review)

The top row's wrapping I added in the review moved every later button down 40 pixels. At
1280×720, Go and Save covered the search row, and the tick boxes covered the status line. Now:

- A top button that doesn't fit goes up beside the title, at the right, so nothing below moves.
- Only when there is no room there does it start a row below.

Checked by screenshots at 1280×720 and 1024×768. At 1024×768 the lower rows were crowded before
this (the review noted it), and they still are.

## Checked here

The hallway built (Linux, GCC, Release); CI step 10 (the hallway, the File Locator and installers) passed.

## Files
- `Proof/STATISTICS.md`
- `Sieve/client/file_locator.cpp`
- `Sieve/client/file_locator.hpp`
- `Sieve/data/lang/en.txt`
