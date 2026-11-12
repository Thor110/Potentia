# A game's files streamed: one `.sieve`, or many (12 November 2026)

IDEAS §19.104 (Edward: ALTEngine "the first to get the sieve treatment ... stream its assets in and
out of its engine"). A `.sieve` decodes as a whole, so its first file costs the time of all of it.
Measured first: what splitting a game's files into several `.sieve`s costs in size, and what it
buys in time to the first file. No format change is involved: each layout is ordinary `.sieve` files.

## The files

A stand-in for an old game's assets, from the repository's own files: 95 files, 11,477,963 bytes.

| Kind | Files | Bytes |
| --- | ---: | ---: |
| Meshes (.obj, .mtl) | 21 | 88 KB |
| Font | 1 | 16 KB |
| Text (the language file) | 1 | 92 KB |
| Icons | 2 | 180 KB |
| Sounds (LJ Speech clips, FLAC) | 40 | 7.0 MB |
| Sprites (screenshots, PNG) | 30 | 3.9 MB |

## The layouts

`sieve locate DIR --installer` at the defaults (v33, Balanced), then `sieve install`. Install times
are the best of three runs, whole processes, on this machine (4 threads of a 2.1 GHz Xeon).

| Layout | Bytes | Time to the first file | Time to all |
| --- | ---: | ---: | ---: |
| One `.sieve` | 10,342,984 | 0.97 s (the whole) | 0.97 s |
| One per kind (6) | 10,336,318 (−0.06%) | 0.23 to 1.66 s (that kind's whole) | 3.36 s |
| One per file (95) | 10,364,291 (+0.21%) | median 0.018 s, at most 0.315 s | 3.61 s |

- **One per file** costs 0.21% in size and gives any one file in 18 ms (median), against 0.97 s for
  the whole set. Most of each 18 ms is starting a process: an engine reading `.sieve`s through
  Sieve's library would pay less.
- **Time to all** is higher when split (3.6 s against 1.0 s), for the same reason: 95 processes. A
  cache filled ahead of the game, or files decoded as asked for, pays it once.
- **The sounds alone took longer than the whole set** (1.66 s against 0.97 s): packed alone, the
  packer weighed some of them for mixing (`mixt`, `mixn`), which decodes slower; packed with
  everything else, they went to LZMA2. So a layout's times depend on what each part is weighed to.
  Under Prioritise Decoding Speed (`--priority speed`) mixing is left out, and that would not happen.

## What it says for ALTEngine

The first step of §19.104 needs no new version: an engine's assets as one small `.sieve` a file (or
a few, by kind), each named by its SHA-256, read when the engine asks for it. That costs about 0.2%
in size here and gives a file in milliseconds. A cache in front of it serves files asked for again.
The engines of the same game (the original through DOSBox, ALTEngine) could share one such set,
since each file is found by its name and checked by its hash. Measuring it on the game's own files,
on Edward's machine, is the next step; ALTEngine's loader is where it would be tried.
