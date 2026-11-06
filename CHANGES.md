# Review and optimisation pass (6 November 2026)

Two commits on top of 56af7f3 ("orbital simulation update"):
- `600ac66`: a sieved GIF opens about twice as fast in the viewer (sent earlier).
- `de89c93`: the review and optimisation pass, below.

Details are in `Sieve/results/review-2026-11-06.md` and `Sieve/results/speed-review-2026-11-06.md`.

## Review: twelve fixes

Two reviewers read everything changed since the 4 November review. Each finding was traced from
a real input and checked again before it was fixed. The main ones:
- **Damaged installer programs:** one could make sieve-install set aside about 10 GB before
  refusing it. Its listed sizes are now checked first, and Windows device names (CON, aux.txt) are
  refused there too.
- **The vault's frame threads:** they could multiply into hundreds on a folder of animated GIFs,
  and a failure in one ended the program. Now at most 4 threads a picture, and a failure is
  reported as before.
- **Memory:** the viewer's kept frames now stay within its memory budget, and the GIF opener no
  longer holds the indices twice.
- **Smaller fixes:**
  - a dropped installer program's own dictionaries are used;
  - `Desktop\Foo` offered folders are read as they were checked;
  - the `gif` recipe's numbers are read the same by the C++ reader and the oracle;
  - `--thumbnail 0` is refused.

## Speed: every `.sieve` byte for byte the same as before

| | Before | After |
| :--- | ---: | ---: |
| code.cpp | 1.43 s | 0.85 s |
| prose.md | 2.77 s | 1.98 s |
| oracle.py | 2.69 s | 1.80 s |
| screenshot.png | 0.91 s | 0.58 s |
| a JPEG | 0.77 s | 0.42 s |
| film.mp4 | 1.58 s | 0.93 s |
| an 11 MB program | 40.3 s | 32.3 s |
| docs/ (42 files) | 46.5 s | 41.9 s |
| the six GIFs | 26.9 s | 22.7 s |

What changed:
- **Mixing's trials at once:** the three trials on a file's start now run together, and so do a
  group's weighings.
- **A program's LZMA2 estimate:** now made while it mixes.
- **Groups of pictures:** a group made only of pictures mixed alone isn't mixed together as bytes.
  On docs/ five such groups lost by 1.9 to 3 times, and they were most of the groups' 15 s. This
  one is a choice rather than a reordering, and none of them won here.
- **The File Locator's zip and 7z** now run beside the packing.

## The idea

IDEAS §19.124: a "Compare with zip and 7z" box, ticked by default. What it would save, measured on
the six GIFs:
- **On the files:** 3.7 s of processor time, which no longer adds to the wait.
- **On the instructions:** 2.7 s comparing them after packing, which still does.

## For your decision

A GIF with a great many frames is checked frame by frame by the vault on every install. There is
no limit on the number of frames, as before today. Adding one is a safety choice (refuse such a
GIF, or check fewer frames), so I've left it for you.

Unit suite: 154,318 checks, 0 failures.
