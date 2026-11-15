# List F3: v34 measured whole

v34 was already frozen on 12 November (docs/NEXT-VERSION.md). This update measures it whole against
v33, as v33 was measured on 3 November.

## What changed

- **`results/v34-measured-2026-11-15.md` (new):** v34 against v33, made by the same build. Both ways
  of the light bench were used (for size with Try every way, and for decoding without it), and all
  188 `.sieve` files installed back byte for byte.

| Folder | Way | v34 / v33 |
| :--- | :--- | ---: |
| Standard benchmark (Silesia's twelve, enwik8, `pg996.txt`, `control.py`) | for size | 0.998 |
| Standard benchmark | for decoding | 1.000 |
| v34's kinds, with Compressed formats temporary | for size | 0.666 |
| v34's kinds, with Compressed formats temporary | for decoding | 0.732 |
| v34's kinds, no box | for size | 0.980 |
| v34's kinds, no box | for decoding | 0.997 |

- **Standard benchmark:** every file gives the same bytes in both versions except Silesia's one
  program, `ooffice`. It is 6.6% smaller made for size (E1, the x86 instruction model) and decodes in
  9.9 s against 8.0 s.
- **v34's kinds**, per file:
  - GIFs: 0.33 to 0.57 of v33 made for size. They also take far longer to make.
  - TIFF: LZW 0.30, PackBits 0.74.
  - PCX: 0.66.
  - Sound: WAVs 0.85 to 0.94, made and decoded faster. The MP3 is 0.89.
  - Programs: 0.92 and 0.93, decoding about a third slower.
- **One question for v35:** `python3` is opened as a container and mixed without E1, so it came out
  the same in both versions. Whether E1 would pay there was not measured.
- **Times on this machine wander by up to about a fifth for the same bytes.** The record says so and
  reads only the larger differences.
- **`tools/bench/bench_files.py`:** gains `--also FLAGS`, extra flags for every making. With
  `--also=--v33` the same files are made as the older version, and the results folder's name ends in
  `-v33`.
- **NEXT-VERSION, ROADMAP and REVIEW-ROUND (F3):** each points at the record.
- **IDEAS §19.235 (new):** the golden ratio's binary word (the Fibonacci word, the 1D model of
  Shechtman's quasicrystals) as a filter and a first entry of the fractal dimension. It records your
  words, with the property checked here: exactly N + 1 windows of each length N, the Sturmian
  property. It also records your reasoning that a file a fractal makes need only keep its pattern, and
  your timing: after the present work and testing.

## Checked

- CI: `ci/run.sh --changed` reaches no step (docs, a results record, a bench script). The help check:
  0 missing.
- The bench script compiles, and its `--also` was used for every run in the record.
