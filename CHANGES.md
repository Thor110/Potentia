# Keep a log, and the 734 s explained (29 October 2026)

Unzip over the repository root. This goes on top of the last zip ("enwik9 and times"); apply that
one first if you haven't. Committed locally (not pushed). Code changed: rebuild the hallway and
`sieve`, then run CI steps 03 and 10 (run here: see the end).

## Keep a log (the File Locator)

There is a new tick box beside Prioritise decoding speed: **Keep a log**.

- Its key is L. It is kept in `sieve-hallway.ini` as `[locator] keep_log`, and `--locate-log`
  turns it on for one run.
- When it's ticked, each reading and each saving writes a text file in a `logs` folder beside
  `sieve-hallway.ini`, named `locate-<date>-<time>-<name>.txt`. The status says where it was written.

Each log holds:

- the settings the run used: the search limits, Try every way, Prioritise decoding speed and the
  compressed formats;
- every report the run made, each with its seconds in and a step's new best marked. This is
  everything the window shows only until the next report: each rope-search best with its
  parameters, each LZMA2 pass, mixing, and the weighing;
- the status, how long it took to the thousandth of a second, and when the last new best came;
- the result's tables: the comparison with zip and 7z, and the weighing;
- the phases' times (the `--timings` table, for this run alone).

For example, a 1.4 MB text here shows each of the rope's three bests with its time and parameters
(the third at 8.98 s), then "took 16.940 s in all", then the tables.

## The 734 s

You remembered right that it was the time for one part, not the whole run. The 734 s and 790.5 s
were in the line under the result, `search: ...; took 734 s`. That is the weighing's own search:
the time it spent finding the file's best way to be named.

Reading the file also packs its instructions (the rope's search and mixing, which for a gigabyte
take far longer) and runs zip and 7z, so the whole was longer. It wasn't kept anywhere.

- That line now reads `the weighing's search took ...`.
- STATISTICS now says the making times are longer than those figures, and why.
- MANUAL describes the box and each of the times.

## enwik9's zip and 7z (`Proof/STATISTICS.md`)

Your figures are in the table:

- zip: 312,712,170 bytes, 3.20:1.
- 7z: 230,120,517 bytes, 4.35:1.
- Sieve against 7z: optimised for size, 19.14% smaller (on enwik8, 16.89%); prioritised for
  decoding, 8.22% smaller.
- PPMd's published figure is 20.06% under 7z.

"At a glance" has enwik9's 7z column filled. The table notes that zip and 7z came from 7-Zip's
right-click "Add to" at its default settings (Normal), not at their strongest.

## Checked here

Linux, GCC, Release: `sieve` and the hallway built with no warnings. The box and the logs were tried
headless: reading a file, then saving it, with the log on, wrote two logs. CI steps 03 (the filters
and the weighing's line, 12 s) and 10 (the hallway and the File Locator, 661 s) passed.

## Files
- `Proof/STATISTICS.md`
- `Sieve/ci/steps/03-filters.sh`
- `Sieve/client/app_main.cpp`
- `Sieve/client/app_settings.cpp`
- `Sieve/client/app_settings.hpp`
- `Sieve/client/file_locator.cpp`
- `Sieve/client/file_locator.hpp`
- `Sieve/data/lang/en.txt`
- `Sieve/docs/MANUAL.md`
- `Sieve/tools/cli/args.hpp`
- `Sieve/tools/cli/timings.cpp`
- `Sieve/tools/cli/timings.hpp`
- `Sieve/tools/cli/weigh.cpp`
