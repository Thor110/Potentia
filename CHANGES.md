# The benchmark as a double-click (24 October 2026)

`Sieve\tools\bench\bench_make.bat` finds Python the way `make_release.bat` does: the py launcher first, then python. It runs the benchmark with the Release build's `sieve.exe`, and keeps the window open at the end.

- Double-click it: the quick set, 1 s of search a file.
- `bench_make.bat --set full`: adds the 52 MB recording (about 6 minutes more here).
- `bench_make.bat --search 1,10`: several search times.
- `bench_make.bat C:\Sieve\tools\sieve.exe --set full`: another `sieve.exe`.

The tables go to `Sieve\results` (`bench-make-*.md` and `.tsv`).

To apply: copy `Sieve/` over. Not run here: there is no Windows machine in this session.
