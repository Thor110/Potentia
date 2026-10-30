# The maps are 98.7% alike; the draft now shows when it's on (30 October 2026)

Unzip over the repository root, on top of the last zip ("sieve alike"). Committed locally (not
pushed). Code changed: rebuild the hallway.

## Your `sieve alike` result

MAPS-Larger.WoW holds **98.7%** of MAPS.WoW:

- 104 copies cover 160,338,860 of its 162,426,225 bytes;
- 2,087,365 bytes are new;
- the script would be 2,087,830 bytes before LZMA2;
- the packer would pair them ("taken yes"), and found that in 1.99 s on your machine.

So the delta way applies, and the draft can't have been on during your WoWRevived run, which came out
the same size to the byte as before.

**What it should save:** today the larger map costs about a second map's worth. As a delta it would
cost about its 2 MB of new bytes once packed. That's an estimate; the next run measures it.

Recorded in NEXT-VERSION §4a and IDEAS §19.106.

## The draft now shows when it's on

So that can't happen unnoticed again:

- **The File Locator** says `v32 DRAFT ON (testing only): ...` in orange, in place of its subtitle,
  while the switch is on. A window without that line is making v31.
- **Keep a log**'s settings line ends with `v32 draft on (...)` or `v32 draft off`, so every log
  says which it was.

For the next run, set `v32_draft = 1` under `[locator]` in `sieve-hallway.ini`, or start the hallway
with `--v32-draft`. Then check for the orange line, and tick Keep a log.

## Checked here

The hallway builds with no warnings. A draft reading showed the orange line and wrote `v32 draft on`
in its log. No CI step pins either line. `file_locator.cpp` compiles for Windows with MinGW
(`-fsyntax-only`).

## Files
- `Sieve/client/file_locator.cpp`
- `Sieve/data/lang/en.txt`
- `Sieve/docs/IDEAS.md`
- `Sieve/docs/NEXT-VERSION.md`
