# Changes (commit fb47b03, 8 November 2026)

Replace these files in the repository (paths from its root):

- Sieve/tools/bench/trim_passes.py (new): measures trimming what the wave predicts exactly.
- Sieve/results/rope-trim-2026-11-08.md (new): the results.
- Sieve/docs/IDEAS.md: §19.131 added.
- Sieve/docs/ROPE-POST.md: the trimming result in section 4; "Its relatives" in section 3 names the
  closer relatives a prior-art search found (CALIC, WebP lossless, MPEG-4 ALS, ZPAQ) and narrows what
  the post claims as new.

Trimming, in short: one trim takes 0.15 to 0.69% off the rope's T1 code on three WAVs and x-ray, and
4.36% on mr; a second pass or a noise pass loses. Not built.
