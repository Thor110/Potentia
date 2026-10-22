# A time estimate in the File Locator, and pieces cut faster (22 October 2026)

## Built
- **A time estimate** (`Sieve/client/file_locator.*`, `Sieve/data/lang/en.txt`). For a file or
  folder of 16 MiB or more, the File Locator first times the same work on 2 MiB of it, taken in
  eight stretches from across the file (a folder's largest). It draws that out to the whole size,
  adds the most the search can take, and shows the result under the status line with the time so
  far. When it's done, it says "took X (estimated Y)". Saving gives its own estimate, from how long
  the packing took when the file was read. With the search set to no limit, it says the search
  can't be estimated.
  - Measured, with the search at 5 s a file: 40 MB of a 7z, estimated 3 min 03 s, took 3 min 46 s;
    a 40 MB tar of the source, estimated 38 min 34 s, took 45 min 11 s. So it runs 15–20% short
    for now.
- **Cutting a file into pieces is much faster on mixed files** (`Sieve/tools/cli/pack.cpp`). This
  is what made large files take so long. Each boundary between pieces was moved in 64-byte steps,
  and every step compressed the whole stretch beside it again, up to 1 MiB. Now each step looks at
  a fixed window of 64 KiB.
  - The output is byte for byte the same on all 16 files of the varied set and on the 40 MB tar.
  - The tar packs in 21 minutes instead of 34, and the File Locator reads it in 45 minutes
    instead of 68.

## Noted (IDEAS)
- §19.97 the estimate and the pieces fix; §19.98 recurring values, scale repeats and the
  locked random byte, measured, with your note to keep watching for them as more computing
  becomes available; §19.99 threads for the search and the packer.

**Checked:** unit tests, and every workflow step (6 to 16) locally.

**Docs:** IDEAS §19.97 to §19.99, SPECIFICATIONS §12.2 (the pieces' window), README (File
Locator), HANDOFF, VOICES.

To apply: copy `Sieve/` over the repository's `Sieve/` folder (the workflow is unchanged).
