# The light bench (2 November 2026)

Unzip over the repository root, on top of the last zip ("sieve-patterns"). One more local commit, not pushed.

`tools\bench_light.bat` works like `bench_files.bat` (it asks for the folder, or takes one), but makes only **decode/no** and **size/try** for each file. On your 153 JPEGs and on enwik9, the other two came to the same bytes as these and took longer.

`bench_files.py` has a new option, `--ways`, which takes any of size/try, size/no, decode/try and decode/no, separated by commas. Without it, all four run as before.
