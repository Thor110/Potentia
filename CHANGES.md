# bench_files for each Compressed formats box (commit 712b55c)

**The scripts:**
- `Sieve\tools\bench_files_optimise_temporary.bat` and `Sieve\tools\bench_files_optimise_permanent.bat`
  run as `bench_files.bat` does (ask for the folder, every file largest first, four ways, no
  search limit), with that box ticked for every run.
- **Where results go:** `<folder>-bench-temporary-<date>` and `<folder>-bench-permanent-<date>`,
  their tables titled with the box, so they stay apart from the plain run's.
- **How each is checked:**
  - temporary: every file byte for byte;
  - permanent: installs an archive as what it holds, or a picture re-saved, so "The same" says
    `installed` where installing succeeded. Sieve itself checks everything it writes.
- I named the second `..._optimise_permanent.bat` (you wrote "optomise"): say if you want it
  otherwise.
- **Tested here** on a JPEG, a PNG and a gzip, both boxes.

**About mozilla:** neither box will help it.
- A tar isn't a compressed format, and the packer reads one only to extract it. A test tar came out
  the same under both boxes.
- What would help is carrying a tar as v31's `skm` carries a zip: its headers as the skeleton, each
  member as a file of its own, so each program goes through the x86 filter.
- That's a packer change, not a new version. It's written up as ROADMAP C.4a, not built.

**Files:** the two `.bat` files (with Windows line endings in this zip), `bench_files.bat` again (the
same), `tools/bench/bench_files.py` (its `--compressed` option), and MANUAL and ROADMAP.
