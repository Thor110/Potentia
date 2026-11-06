# Symphony No. 6 (FLAC): Sieve, HX and hs_sieve (6 November 2026)

Edward asked for two things about the repository github.com/CombaryMaker/Harringtons_Insights
(Michael Hugh Harrington's), cloned at ff6585f: whether anything in it is lifted from Sieve, and
what its tools make of `Symphony No.6 (1st movement).flac` (filesamples.com, 51,997,326 bytes,
SHA-256 421dd83e48cc53b6e23e7e282040baf924e2add6f9b296d2cf6f22d8806707dd).

## Anything lifted from Sieve

Nothing found.
- **Code:** every line of 45 characters or more in their C, C++, Python and JavaScript sources
  (13,272 distinct lines) was looked for in Sieve's (35,173, third-party code left out). Four were
  shared, all ordinary Python: an `argparse` line, two `getrusage` calls and a script finding its
  own folder.
- **Words and names:** none of Sieve's own terms appear (sieve-manifest, mix4, the rope, Try every
  way, the binary line, the File Locator, the vault, Gallery of Babel, Optimise Search and others).
  Their "The Sieve" graphic is a step of their own: data that cannot shrink set aside.
- **Sieve itself:** only `hs_sieve/`, which says it is a wrapper that runs Sieve's `sieve` program
  and uses none of its source, and it does just that. Its `hs_sieve.cpp` is the file sent to Edward.

## The FLAC

Each made once on this machine (Linux, 4 logical processors, 16 GB), installed or decoded again
and compared byte for byte: every one came back the same. HX was built from its own source as its
BUILD.txt describes (two gaps filled: bzip3's VERSION define, and four files of 7-Zip's C that its
list leaves out); its licence allows building, benchmarking and publishing results for research.
Edward's zip and 7z sizes are from his machine.

| | Bytes | Of the FLAC | Made in | Decoded in | Peak memory making |
| :--- | ---: | ---: | ---: | ---: | ---: |
| The FLAC | 51,997,326 | | | | |
| zip (Edward's) | 51,942,347 | −0.106% | | | |
| 7z (Edward's) | 51,944,928 | −0.101% | | | |
| HX Fast (default) | 51,934,580 | −0.121% | 34.1 s | 0.15 s | 418 MiB |
| HX `--complete` (its best) | 51,893,617 | −0.199% | 219.9 s | 9.35 s | 927 MiB |
| hs_sieve (Sieve's default, in 6 blocks on 4 cores) | 51,784,112 | −0.410% | 14.0 s | 0.52 s | 416 MiB each (four at once) |
| **Sieve**, default | **51,783,003** | **−0.412%** | 33.4 s | 1.3 s | 1,124 MiB |
| **Sieve**, Optimise Search, mixed whole (as Edward made it) | **51,707,991** | **−0.556%** | 476.7 s | 65.9 s | 1,273 MiB |

- **HX's best is 0.199% off the FLAC; Sieve's is 0.556%**, 2.8 times as much taken off, and Sieve's
  default (and now its Optimise Search too, below) 0.412%, twice HX's best, installing in 1.3 s.
- **hs_sieve** is Sieve run in blocks: 1,109 bytes larger than Sieve alone, faster to make and to
  unpack on four cores. Its gain on this file is speed, bought with size, as block splitting is.
- `--max` gave HX nothing more here: alone it is the Fast result, and with `--complete` the same
  bytes as `--complete`.

## What it changed in Sieve

Edward's `.sieve` was mixed whole for 0.556%, and a 52 MB mixed stream installs in 65.9 s. The rising
bar for mixing over 16 MiB (built the same day) had been asked only against LZMA2; LZMA2 cannot
shrink a FLAC, so the rope was the best before mixing and the bar was skipped. It is now asked
against whatever is best: this file is cut into pieces and pulled tight by the rope, 75,012 bytes
more (0.412% off the FLAC), installing in 1.3 s.
