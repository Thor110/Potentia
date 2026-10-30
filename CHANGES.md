# 7z's lead on programs, measured again (30 October 2026)

Unzip over the repository root, on top of the last zip ("control.py measured again"). Committed
locally (not pushed). Documents only.

You were right that "7z beats Sieve on programs" is out of date. I measured the same kind of folder
again: Sieve's own program (Linux, 10,236,056 bytes) and the docs, 12,692,639 bytes in all.

| | Bytes |
| :--- | ---: |
| **Sieve** (0.13.77, made v30) | **4,159,742** |
| 7z, LZMA2 9e with BCJ (py7zr, solid) | 4,277,050 (Sieve 2.74% smaller) |
| 7z, LZMA2 9e, no filter | 4,532,855 |

In October 7z won this (2,836,214 against 2,949,958). Your source tree agrees: `source.sieve`
47,348,954 against `sieve-source.7z` 49,320,989.

**What's left is BCJ2 itself.** It's 7-Zip's own x86 filter, which py7zr doesn't have, and it can't
be fetched here. If you'd like it settled, put `tools\sieve.exe` and the `docs` folder in a folder
and make:

- a 7z of it with 7-Zip (right-click, Add to, Normal; Ultra too if you like);
- its `.sieve` in the File Locator.

If Sieve is smaller there too, BCJ2 drops off the v32 list and JPEG is next.

Recorded in IDEAS §19.95 and NEXT-VERSION §1.

## Files
- `Sieve/docs/IDEAS.md`
- `Sieve/docs/NEXT-VERSION.md`
