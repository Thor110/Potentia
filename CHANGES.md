# LZMA2's settings chosen per stream (21 October 2026)

Your 137 pictures: 21 came out larger than 7z's (not only the 8 you sent), by 15 to 393 bytes.
All 21 are small (the largest 364 KB), and none came out larger than the picture itself.

**Why:** Sieve packed every stream with one fixed set of LZMA2 settings, made for text (lc 3,
pb 2). A JPEG's bytes have no 2- or 4-byte alignment, and other settings (pb 0 with lc 0 or 4)
packed each of your eight 113 to 614 bytes smaller.

**What changed** (`Sieve/tools/cli/pack.cpp`): when making the newest version, Sieve tries a few
standard settings on each stream's first MiB, then packs the whole stream with the best of them
and also as before, and keeps the smallest. Nothing is fitted to a file's content, and no file can
come out larger than before.
- No new format version: LZMA2 stores these settings inside its own stream, and every reader (the
  tool, `sieve-install`, the oracle) already takes them from there.
- `--v16` and older make exactly the bytes they made before (checked: identical).
- The cost: LZMA2's part of the time about triples (the `sieve` program as one file: 4.7 s → 16.5 s).

**Measured** (one file's instructions, before → after; all unpacked byte for byte by both readers):

| File | Before | After | 7z |
|---|---:|---:|---:|
| WIN_20240103_23_06_39_Pro.jpg | 53,139 | 52,757 | 52,987 |
| program.jpg | 59,891 | 59,694 | 60,014 |
| TheBigBus.jpg | 108,688 | 108,654 | 108,450 |
| victorian-architecture-….jpg | 131,406 | 131,213 | 131,360 |
| K2_00000.jpg | 169,848 | 169,685 | 169,682 |
| K2_00001.jpg | 175,356 | 175,122 | 175,069 |
| 353007588_….jpg | 192,666 | 192,305 | 192,509 |
| app.jpg | 209,816 | 209,202 | 210,144 |

Five of the eight now beat 7z. The varied set improved too: Sieve's C++ 162,271 → 161,808,
IDEAS.md 98,742 → 98,512, a log 19,504 → 19,312, a CSV 7,651 → 7,571, a WAV 5,242 → 4,803, a PNG
34,013 → 33,858. `pi6.txt` is unchanged at 417,061, because the rope carries it, not LZMA2.

**Still larger than 7z:** K2_00000 (3 bytes), K2_00001 (53), TheBigBus (204). 7-Zip's own LZMA
encoder packs a little tighter than liblzma's, and the repository has only 7-Zip's decoders. Adding
its encoder as one more candidate is the next step there; I'd measure it first.

**CI** (`.github/workflows/build.yml`):
- New check: the oracle's source packed as one file is smaller than `--v16` makes it, and both
  readers give it back.
- The five-pages check is now 339 bytes, measured (`--v16` still 345, also checked).
- The setup menu's reset check compared screenshots, and the menu shows a time measured on each
  run, so a busy machine could fail it. It now compares the saved settings.

**Docs:** SPECIFICATIONS §12.2, HANDOFF, VOICES (your quote).

Your pictures were only measured: none is in the repository, and I deleted my copies.

To apply: copy `.github/` and `Sieve/` over the repository's top folder.
