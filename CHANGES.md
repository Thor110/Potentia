# tools/bench_files.bat (commit b03b009)

Double-click `Sieve\tools\bench_files.bat`. It asks for a folder (type the path, or drag the folder
into the window), then runs overnight.

- **What it runs:** each file of that folder, largest first, four ways, with no search limit:
  size/try, size/no, decode/try, decode/no.
- **Each run:** installed again (`sieve install`) and checked against the original.
- **What it keeps:** the `.sieve`'s size, how long making took, and how long decoding and checking
  and writing took. Once a file, beside them: zip (deflate 9) and 7z (LZMA2 9e, one stream), as the
  File Locator gives them.
- **Where the results go:** beside your folder, as `<folder>-bench-<date and time>`, so nothing goes
  into the repository. In it:
  - `results.md`, a table a file;
  - `results.tsv`, every figure;
  - `runs\`, each run's full output with `--timings`.
- **If stopped part way:** the tables are rewritten after every run, so closing the window keeps
  what finished.
- **Options:**
  - `bench_files.bat D:\bench --keep` keeps each `.sieve` made;
  - an `.exe` as the first option uses another `sieve.exe` (the default is
    `out\build\x64-Release\sieve.exe`).

**Tested here** on three small files: every run installed back the same.

Files: `Sieve/tools/bench_files.bat`, `Sieve/tools/bench/bench_files.py`, and MANUAL's row.
