# Changes since 377c166 (12 November 2026)

Extract from the repository root. This replaces the earlier zips of today (the notes and the Stage 6
measurements).

## The whole-set plain weighing (built, at your go-ahead)

After the newest instructions are made, the set is packed once more as the plain installer, every
file in one LZMA2 stream, and the smaller is kept. This only happens for two files or more, where
nothing was changed on packing, and where no map graph or items are carried. The report gains a
`plain` line saying which was kept. No new version: every Sieve since v4 reads the plain installer.

| Set | Before | After |
| --- | ---: | ---: |
| A film and a trailer cut from it | 3,811,383 | 3,449,600 (7-Zip 3,449,028) |
| 95 game-like files | 10,342,984 | unchanged (the newest is smaller) |
| Small text, mesh and doc folders | | unchanged |

- **Cost:** making takes about 12% longer on the game-like set.
- **Found and fixed during testing:** maps carry their graph in the listing, which the plain
  installer has no place for, so maps (and items) are left out of the plain weighing.
- **Checks:**
  - CI steps 02, 04, 10 and 11 passed;
  - the unit suite: 154,568 checks, 0 failures;
  - `check_help`: 0 missing.

## Stage 6 measurements

- **Streaming a game's files** (results/stream-layouts-2026-11-12.md): one `.sieve` per file costs
  0.21% in size and gives any file in 18 ms, against 0.97 s for one `.sieve` of everything.
- **Films that share packets** (results/video-packets-2026-11-12.md, `tools/bench/video_packets.py`):
  90% of a trailer was packets already in its film. This is what led to the fix above.

## Stage 6 closed out, and the plan in ROADMAP.md

- **The rope's order-1 coding and trimming** were looked at again before building. `mr`, `x-ray` and
  `sao` now go to mixing (as pictures), and WAVs to FLAC in v34, so they weren't built (IDEAS
  §19.127).
- **ROADMAP.md** now opens with the seven-stage plan, where each stage stands, and what waits on you.

## Your notes

- **GAME.md §11:**
  - the lenses;
  - Alien Trilogy first, its cabinet streaming as you approach;
  - ALTEngine's own stripping and streaming;
  - one hallway of game engines folding through time;
  - releases mapped as deltas;
  - a source tree across computing;
  - graphics from pixels to NURBS;
  - the story pool and verified end-of-game messages;
  - games already released free at one end.
- **IDEAS:**
  - 19.136: history fixed and mapped, the known, unknown and lost;
  - 19.137: compatible forever (also SIEVE-FORMAT §7);
  - 19.138: your aside on time, slavery and war;
  - 19.139: technical to metaphorical; distributed computing, a DNA dimension, an infinite RAID.
- **VOICES.md:** a strong note that it needs cleaning up again, and your new lines in a candidates
  block, waiting for that clean-up.

## Commits (local only)

5c8d249, b3f6b2b, 540aac7, 7e21398, 2eb4f77, 0a59872, dd4f388, f7282a2, ea1a8ad, cd09163, and the commit closing Stage 6.
