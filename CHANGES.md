# Changes since 0677219 "sieve stages" (12 November 2026)

Stage 4 of the plan. Nothing pushed. Extract this zip from the repository root.

## Training faster, every result the same

Training's arithmetic is all integers, so the order of the sums can change without changing any
result. Taken (`core/src/training.cpp`):

- **Sums held in registers.** The backward pass, the gradient and the forward pass now work four rows
  by eight columns at a time (sixteen with AVX-512). The sums stay in registers over the whole
  inner loop and go to memory once, instead of being added into memory one row at a time.
- **AVX-512 kernels** where the processor has them, chosen at start-up like AVX2.
- **The forward pass** reads each matrix turned on its side, turned once per step.
- **The weights' update** is shared among the trainer's threads.

Measured here (4 threads of a 2.1 GHz Xeon with AVX-512), whole `ai-train` runs:

| | Width 64, 1 thread | Width 64, 4 threads | Width 256, 1 thread | Width 256, 4 threads |
| --- | ---: | ---: | ---: | ---: |
| Before (AVX2) | 3.01 s | 1.42 s | 9.47 s | 5.18 s |
| Now | 1.81 s | 0.72 s | 5.5–5.8 s | 3.37 s |

- On a machine with AVX2 but not AVX-512, expect roughly the "AVX2, tiles" row in
  `results/training-kernels-2026-11-11.md` (for example, width 256 on one thread: 9.5 s down to 7.5 s).
- **Checked:**
  - the unit tests run the training vectors on the plain, AVX2 and AVX-512 paths;
  - the resume files of every timed run are identical to before;
  - CI step 04 passed, so every shipped and test run still reproduces exactly.
- **Tried and not taken:** putting a step's windows through each matrix together. It gave the same
  result but was no faster.

## QK-norm: measured, not built

`results/qk-norm-2026-11-12.md`, IDEAS §19.119. I tested it with a temporary build, which isn't in
the commits. It does stop attention scores reaching ±128 in the run without a loss scale (17.5
million held values down to none). But the default loss scale already holds none, and learnt more:

| Run | Right byte (last 3 checkpoints) |
| --- | ---: |
| Default loss scale, no QK-norm | 15.99% |
| Default loss scale, with QK-norm | 15.82% |

So a training-v4 would change every model reader for no gain at this size. It's worth measuring again
on a much larger model.

## Documents as training text

`sieve ai-train --record FOLDER --out RUN --as-text` (`tools/cli/doc_text.cpp`):

- **What it reads:** HTML, Word (.docx), OpenDocument (.odt), EPUB (chapters in reading order) and
  RTF, each put in the run as its text under its own name plus `.txt`.
- **How the text comes out:** markup, styles, scripts, fonts and pictures are left out, paragraphs
  become lines, and entities and Unicode are read.
- **A file that doesn't read** is kept as it is, and the program says so.
- **Not read:** PDF.
- **Reproducibility:** the run carries the text itself, so it trains again the same anywhere.
- **Also changed:**
  - help, the manual and a CI step 04 check (texts, a broken file, a replay and the oracle's replay);
  - `libarchive_read` is now shared from compress.cpp rather than duplicated.

## Left for you

- **Wider arithmetic (Q32):** this would be a new training version, with matching changes to the
  spec, the reference trainer and the test vectors. Nothing measured so far shows Q16 limiting
  learning once the default loss scale is on.
- **"Documents as text" in the hallway's New run settings:** it's command-line only for now. Adding it
  means a new settings row and updated CI key walks.

## Checks run here

- Unit suite: 154,568 checks, 0 failures.
- CI step 04 passed with the training changes. Its new documents block was run on its own and passed
  (the full step ran before that block existed).
- `check_help`: 0 missing.

## Commits (local only)

- 152d523 Training faster with the same results; QK-norm measured
- 3488360 Documents read as their text when recording a training run
