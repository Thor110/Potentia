# Making faster: the optimisation pass of 6 November 2026

Edward: "Run a full code review and optimisation pass. Look for any speed up you can get across
any and every file type." Each file packed alone with `sieve locate FILE --installer`, by the
build before this pass and the build after it, on this machine (4 cores); and three folders with
`--optimise-search`. Every `.sieve` compared byte for byte: **the same bytes in every case**.

## What changed

1. **Mixing's three trials at once.** Every way mixing tries on a file (as bytes, as a picture
   of a shape, with the word list, primed) is weighed on the file's start three times (without
   the match model's ways, with C2, with all three), and a file over 4 MiB twice more on its first
   4 MiB. They ran one after another; now at once, as the file's share of the cores allows,
   compared in the same order as before.
2. **A program's LZMA2 figure beside its mixing.** A program's LZMA2 estimate (preset 6, through
   the x86 filter) was made before it was mixed, though nothing reads it until mixing is weighed
   against it; now it is made on a thread of its own while the program is mixed (IDEAS §19.120 a,
   for programs).
3. **A group's weighings at once.** A group of files mixed together is weighed with its optional
   files and without them, and on its start with and without the word list: at once now.
4. **A group of pictures not mixed as bytes.** A group whose files are every one mixed alone as a
   picture is not mixed together: groups have no picture's shape, so their pixels were mixed as
   bytes. On `docs/` five such groups (three hallway screenshots each, 8.3 MB) came to 1.9 to 3
   times their own codes and took most of the 15 s the groups took. This one is a choice, not
   only an order: a group of pictures that would have won as bytes is no longer tried. None did
   here.
5. **The File Locator's zip and 7z beside the packing.** What zip and 7z make of what was read is
   worked out on a thread of its own while Sieve packs, not before.
6. **Earlier today:** the lzma2 stream's four LZMA2 passes at once (161 s to 96 s on six GIFs), and
   a sieved GIF decoded once in the viewer, its frames hashed at once (0.63 s to 0.29 s).

## Single files

| File | Bytes | Before | After |
| :--- | ---: | ---: | ---: |
| code.cpp (sieve_cli.cpp) | 174,622 | 1.43 s | 0.85 s |
| oracle.py (sieve_ref.py) | 492,562 | 2.69 s | 1.80 s |
| prose.md (IDEAS.md) | 514,891 | 2.77 s | 1.98 s |
| words.txt (scowl en-80) | 2,921,105 | 7.63 s | 6.93 s |
| screenshot.png | 92,628 | 0.91 s | 0.58 s |
| vitruvian.jpg (the test site's) | 116,168 | 0.77 s | 0.42 s |
| door.jpg | 69,298 | 0.55 s | 0.33 s |
| film.mp4 (Roundhay Garden Scene) | 234,633 | 1.58 s | 0.93 s |
| a program (this build's own `sieve`, ELF) | 11,264,800 | 40.3 s | 32.3 s |

(The program's 32 s: mixing it whole, 11 MB, and reading it back to check it, about 11 s each,
are what is left; neither can run beside anything it needs.)

## Folders (`--optimise-search`)

| Folder | Bytes | Before | After |
| :--- | ---: | ---: | ---: |
| `docs/` | 2.8 MB, 42 files | 46.5 s | 41.9 s |
| the six GIFs | 9.6 MB | 26.9 s | 22.7 s |
| the nine files above together | 16.0 MB | 62.2 s | 58.2 s |

(The folders' "after" for the GIFs and the nine files is before change 4, which they do not
touch.) In the File Locator on the six GIFs, zip and 7z (3.7 s) now end during the packing; what
zip and 7z make of the instructions themselves (2.7 s) still comes after it, as it needs them
(IDEAS §19.124).

## What was looked at and left

- **The rope's probe on pictures:** 25 s of processor time on `docs/` (18 screenshots, 1.4 s each),
  on four cores; it is what lets the rope's search be skipped, so it stays.
- **Mixing a large text whole** (the docs group, 1.7 MB, 9 s): one core by its nature.
- **Reading back** every file as installing will: a check, kept.
