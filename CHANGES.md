# The rope's search, ordered by the file (24 October 2026)

## What was measured
`sieve rope FILE --seconds 0` now says which wave was best and when it was found. With no time
limit, on every file of the quick set:
- **Text** (prose, code, the oracle, the word list, the mesh): the best was the very last wave
  tried, always the newest line wave (`C2 G1 A1`). A short search never got that far. On the word
  list it codes 564,366 bytes, found after 15.5 s, where a 1 s search left it at 633,244. So
  "10 s found nothing" meant the search hadn't reached it.
- **Already-packed bytes** (JPEGs, PNG, MP4, a small table and text): the best was the first wave,
  the plain byte counts. The 160 to 230 waves after it found nothing.
- **The rest** (WAV, MIDI, the program, the tar) found their best part way through.

## What changed (in the search itself, so for every file and every caller)
- **Text gets its line waves early,** straight after the first quick waves. Text means at least
  95% of the bytes are printable characters, whitespace or UTF-8, in lines of at most 1,024 bytes.
  The tar counts as text but has 4,096-byte lines, where one such wave took 17 s and can't be cut
  short, so it and other files keep them last, as before.
- **Bytes the first waves can't shrink by 2% aren't searched further.** No later wave has helped
  such a file.
- **You can still search longer:**
  - `--search-every-wave` turns the early stop off, on every command and in the hallway;
  - in the File Locator the same switch is `[locator] search_every_wave = 1`;
  - the search time is yours to set, as before.
- With no limit and every wave, the best is the same as before on every file; only the order
  changed.

## Results at 1 s a file (against the build before)
Every file came out the same size or smaller. The quick set took 30.5 s instead of 67.0 s:

| File | Before | After |
| :--- | --- | --- |
| word list | 12.45 s, 633,244 bytes | 3.07 s, 564,462 bytes (10.86% smaller) |
| PNG | 3.60 s | 0.56 s |
| film | 2.73 s | 0.42 s |
| JPEGs | 1.2 to 2.3 s | 0.15 to 0.31 s |

Full tables: `results/bench-make-quick-2026-10-24-rope.md`; the earlier one is renamed
`-threads.md`.

## The File Locator's estimate (measured, not changed yet)
- **The 52 MB recording** took 11 min 30 s against an estimate of 8 min 01 s, 30% low.
- **A 20 MB text** (the word list repeated seven times) took 5 min 36 s against 1 min 35 s. Its
  repetition slows LZMA2 on the whole file, and no small sample can contain it.
- **Tried and dropped:** packing the samples with the whole file's dictionary changed nothing
  measurable, so I took it back out.
- **Next:** the File Locator does more than `sieve locate` on the whole file (the zip and 7z sizes,
  and the weighing): 11.5 min against about 6. Which of those parts the samples undercount gets
  measured next, phase by phase, before the estimate is changed.

## Tested here
The unit tests and the full local CI pass, including the 1-thread against 4-thread comparison.

## To apply
1. Delete `Sieve/results/bench-make-quick-2026-10-24.md` and `.tsv`. They're now named `-threads`,
   and come in this zip.
2. Copy `Sieve/` over, rebuild, and run the tests.
3. Rerun `bench_make.bat` to compare on your machine.
