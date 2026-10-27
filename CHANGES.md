# The READMEs rewritten, a benchmark, and a plan for showing Sieve (27 October 2026)

Unzip over the repository root. Committed locally (not pushed). Nothing to rebuild.

## The READMEs
- **`README.md` (Potentia)** now opens with what you can do today (a table of six uses), then how to
  try it, why it exists (the alignment thesis in one paragraph, in your words), where it stands and
  the roadmap. "Training recorded and replayed" is now ticked on the roadmap, as your Features list
  already said it was built.
- **`VISION.md` (new)**: your vision, moved whole and unedited: the alignment thesis, the evidence,
  the primer, what Potentia is, the filtration layers and methods, units and books, the summary and
  security, with its index and every section link working.
- **`Sieve/README.md`** now opens with what Sieve does, task by task, each with its command: smaller
  archives (with the table below), right-click extraction, checked installers, web pages that prove
  themselves, repeatable training, and the Gallery of Babel. Then getting it, building it, how it is
  checked, and a map of the documentation.
- **`Sieve/docs/MANUAL.md` (new)**: the old Sieve README's reference sections, moved whole (layout,
  build, key terms, lines, the hallway, the filter survey, commands, canonicalisation, conformance,
  performance, M1 and M3 results, next), with links adjusted to their new folder.
- **`Previous/READMEv2.md`, `Previous/SIEVE-READMEv1.md`**: both old READMEs as they were, their
  links adjusted to work from `Previous/`. (You said you would move them there; done, so the
  changelog line points at a real file. Rename them as you like.)

## The benchmark
- **`Sieve/tools/bench/compare.py` (new)**: Sieve against 7-Zip, xz, zstd, brotli, bzip2, gzip and zip
  on any folder, each checked to give the folder back. It uses 7-Zip's own program where it finds
  one (and zstd's and brotli's), else `pip install zstandard brotli py7zr`; anything missing is
  skipped and said so. On Windows: `python tools\bench\compare.py FOLDER --sieve tools\sieve.exe`.
- **First result** (`Sieve/results/compare-sieve-source-2026-10-27.md`, and in `Sieve/README.md`):
  this repository's `Sieve` folder without `data/anchors/` (743 files, 35,396,955 bytes): Sieve
  5,196,966 bytes (6.81:1, 24% smaller than 7z's 6,844,677), made in 224.7 s and unpacked in 19.4 s,
  against 7z's 52.8 s and 1.0 s. Measured here on Linux, 4 logical processors; the 7z was py7zr's.
  Worth running on the release folder on your machine, with the real 7-Zip.
- Your release figure (44,647,549 bytes to 6,319,561, 7.06:1) is quoted in both READMEs.

## The plan
- **`Sieve/docs/SHOWCASE-PLAN.md` (new)**: the format frozen; a `.sieve` v31 specification of its
  own; the benchmark on the release folder, Silesia and enwik8; a public ISO test set before saying
  7-Zip fails where Sieve does not; six two-minute demonstrations; who to show what, and where;
  what to have ready before any post; the order.
- **`HANDOFF.md`**: the format freeze added to the standing rules, in your words, and an entry for
  today's work.

## Found on the way
- `Sieve/lt/` (5 files) is committed: a language-test leftover from `d20b2bf`, which your packaging
  notes delete. Worth removing and adding to `.gitignore`.

## Files
- `README.md`, `VISION.md`, `Previous/READMEv2.md`, `Previous/SIEVE-READMEv1.md`
- `Sieve/README.md`, `Sieve/docs/MANUAL.md`, `Sieve/docs/SHOWCASE-PLAN.md`, `Sieve/docs/HANDOFF.md`
- `Sieve/tools/bench/compare.py`, `Sieve/results/compare-sieve-source-2026-10-27.md`
