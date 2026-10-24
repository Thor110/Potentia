# The File Locator, measured step by step (24 October 2026)

## Measuring
The File Locator now times each step of its samples and of the whole file. With `--timings`, the
`estimate.*` steps appear beside the matching `locator.*` ones. On the 52 MB recording:
`hallway --no-menu --locate FILE --search-total 1 --timings`.

## A speedup found: the cut into pieces is kept
The 52 MB recording was being cut into pieces twice: once to make its instructions, and again by
the weighing, 170 s each time. The rope's search already reused its answer between the two; the cut
didn't. It's now kept by the file's SHA-256: at most four cuts, 1 GiB in all, and never a cut that a
stop ended early.
- The weighing: 257 s → 79 s.
- The whole File Locator run: **11 min 43 s → 8 min 28 s** (28% faster).
- Every file of the quick set comes out the same, byte for byte.

## The estimate
- **Put back:** packing the samples with the whole file's dictionary. On the recording it raised
  the estimate from 7 min 15 s to 8 min 01 s, nearer the true time; last round I'd dropped it on
  the evidence of the repeated-text file alone.
- **Still a third low:** 5 min 37 s against 8 min 28 s. On a large file the time grows faster than
  in proportion:
  - making the instructions takes 1.4 times what the samples predict;
  - the weighing 1.75 times;
  - the 7z comparison 1.7 times.

  Two small samples can't show by how much, so a better first guess isn't the fix.
- **Next: refine it while it works.** Once the instructions are made, about two thirds of the run,
  the File Locator knows how far the samples were off on that file, and can rescale the time left.

## Tested here
Unit tests and the full local CI pass.

## To apply
Copy `Sieve/` over, rebuild, and run the tests.
