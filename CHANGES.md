# Sieve Secure link and the root tidied (4 November 2026)

Commit bb07a3a (local, not pushed), on top of your 963d70a.

## Changed

- **README.md and Sieve/README.md:** Sieve Secure now links the live page, https://thor110.github.io/Potentia/Pages/secure.html, instead of Pages/secure.html, which GitHub shows as raw HTML. This session could not reach github.io, so please click it once to confirm.
- **The two sample files** moved from the repository root to Sieve/results/, with their date:
  - diffusion-samples.txt is now diffusion-samples-2026-10-22.txt;
  - metrics-example.txt is now metrics-example-2026-10-22.txt.

  Nothing refers to either file. Metrics are now written for every build in Sieve/metrics/builds/.

## Delete these by hand when you sync

The two files at the repository root (they now live in Sieve/results/):
- diffusion-samples.txt
- metrics-example.txt

CHANGES.md stays at the root.
