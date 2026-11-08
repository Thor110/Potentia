# Changes (8 November 2026)

Replace these files in the repository (paths from its root):

- Sieve/tools/bench/wave_paths.py (new): wave paths, seeded paths, waves in turn, wider waves, the 5/3 wavelet.
- Sieve/tools/bench/wave_fitted.py (new): a wave fitted to each block (linear prediction), bit-length coding.
- Sieve/results/wave-paths-2026-11-08.md (new): the results, with FLAC on the same samples.
- Sieve/docs/IDEAS.md: section 19.132 added.
- Sieve/docs/ROPE-POST.md: "Against FLAC" added to section 4.

In short: paths and seeds find nothing the waves over every value do not; waves in turn take 0.45 to
1.82% off the WAVs; a wave fitted to each block takes 12.88 to 36.19% off and comes within 4.4 to
7.0% of FLAC; FLAC is 81.2% of the rope's code on the seven WAVs.
