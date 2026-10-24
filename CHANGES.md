# The pieces' boundaries, a twelfth of the time (24 October 2026)

## What was slow
When a file is cut into pieces (some for LZMA2, some for the rope), each boundary between two
pieces is moved, up to a block either way, to where the two together cost least. Timed apart
(the new `pack.pieces.blocks` and `pack.pieces.boundaries` in `--timings`), the 52 MB recording's
cut was almost all boundaries:
- 584 boundaries, 68,825 steps, each one LZMA2 over up to 64 KiB: **164 s**;
- weighing its 12,815 blocks: 2 s.

## What changed
Each boundary is now tried every 512 bytes first, then every 64 bytes within 448 of the best of
those (14,633 steps on the recording in place of 68,825). Each step is weighed over 16 KiB beyond it, in place of
64 KiB. The format and the reader are unchanged. Only how the cuts are found changed.

## Measured here
Every one of these made the same instructions, byte for byte, as before:
| File | Boundaries | Boundaries' time | `sieve locate` |
| :--- | ---: | --- | --- |
| 52 MB recording | 584 | 164 s → 13 s | 318 s → 170 s |
| 10.6 MB of the repository's pictures, films and sounds, joined | 38 | 6.8 s → 0.64 s | 27.5 s → 21.5 s |
| The quick set (15 files; the WAV has 14 boundaries) | up to 14 | | same bytes on all |

Each change was also tried alone: steps of 512 then 64 (boundaries 39 s), and a window of 16 KiB
(49 s). Both made the same bytes. Not moving the boundaries at all was tried too: Vitruvian Man came
out 41 bytes larger, so they still move.

Most of the recording's time is now LZMA2's full passes and the rope's search.

## Also
- `tools/bench/bench_make.py`'s table has two new columns: blocks and boundaries.
- SPECIFICATIONS (v13's pieces), IDEAS §19.99 and HANDOFF updated.

## Tested here
Unit tests and the full local CI pass.

## To apply
Copy `Sieve/` over and rebuild.
