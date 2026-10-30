# Every way tried, logged and documented (30 October 2026)

Unzip over the repository root. This goes on top of your last update. Committed locally (not
pushed). Code changed: rebuild everything, then run the unit suite and CI steps 03 and 10 (all run
here: see the end).

Nothing here changes a `.sieve`. A folder of six mixed files and the docs folder pack byte for byte
the same as with the last commit's build.

## The weighing counts mixing (your "oh definitely yes")

The weighing now counts mixing as the packer found it, so its best way agrees with what the
instructions do. On a 3 MB text it now reads `mixing mix4 B22 D60` at 4,721,512 bits, where it used
to say LZMA2 (5,762,104 bits). On enwik8 optimised for size it should now say mixing rather
than 24.9% by LZMA2; not measured here, as enwik8 isn't here.

## Every way tried, in the log

With Keep a log ticked, nothing tried goes unsaid:

- **Every file not searched gets a line saying why**: under 512 bytes, a copy of another file here,
  a program (through the x86 filter), an item of Sieve's lines, or a mask of another file's lines.
- **A searched file's line now also says:**
  - why it wasn't cut into pieces, for example `pieces not cut (all 24415 blocks of 4 KiB went to
    LZMA2: nothing to cut)`, which is what happened on enwik8;
  - what mixing's trials on its start came to: as bytes, as a picture, with the word list;
  - why mixing wasn't tried, where it wasn't: under 4 KiB, over 16 MiB, decoding first, or stopped.
- **The weighing in the log has every file's row** (the window shows 12), then **every way weighed
  for every file**, with its bits and the best marked `*`.

For example, from a test folder:

```
file 2 of 6: small.txt, 20000 bytes: LZMA2 about 7991, rope 12538 (L8 W1 S1 P0 K0), pieces not cut
(all 5 blocks of 4 KiB went to LZMA2: nothing to cut), mix4 B16 D60 6811 [mixing on its first
20000 bytes: as bytes 7150, with the word list 6809]; taken: mix4, 6811 bytes
file 3 of 6: tiny.txt, 5 bytes: not searched (under 512 bytes); in the lzma2 stream
```

## `Sieve/docs/METHODS.md` (new): every way, for presenting

One page with every way Sieve tries a file, in the order a reading runs:

- what it is set beside (address, zip, 7z);
- each file's way (items, programs, masks, copies, compressed formats);
- the search (LZMA2, the rope, pieces, mixing and its trials);
- files together (groups, the mixed stream);
- the streams' LZMA2 tuning;
- the read-back;
- the weighing.

For each, it says when it is tried and when it isn't, and quotes the log line it writes, so a log
can be followed from the page. It is linked from README, MANUAL and SHOWCASE-PLAN; the plan's
answer to "why is it slower?" now points to it and to the enwik9 breakdown.

## Checked here

Linux, GCC, a Release build of everything, with no warnings.

- A folder of six files read with the log on: the lines above, and the weighing with mixing.
- Byte for byte: the folder and the docs folder packed by this build and by the last commit's,
  identical (1,032,551 bytes for the docs).
  - On the first try they differed by 1,993 bytes. The old build I'd made fresh had no
    `dictionaries` folder beside it, so it mixed without the word list. Given the folder, it made
    the same file.
- The unit suite (step 01, 389 s) and CI steps 03 (13 s) and 10 (820 s) passed.
- Every changed file compiles for Windows with MinGW (`-fsyntax-only`).

## Files
- `Sieve/README.md`
- `Sieve/client/file_locator.cpp`
- `Sieve/client/file_locator.hpp`
- `Sieve/docs/HANDOFF.md`
- `Sieve/docs/MANUAL.md`
- `Sieve/docs/METHODS.md` (new)
- `Sieve/docs/SHOWCASE-PLAN.md`
- `Sieve/tools/cli/pack.cpp`
- `Sieve/tools/cli/pack.hpp`
- `Sieve/tools/cli/weigh.cpp`
- `Sieve/tools/cli/weigh.hpp`
