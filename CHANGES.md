# v33 measured, two packer faults fixed, installing quicker (3 November 2026)

Copy `Proof/` and `Sieve/` over the repository's. No change to the `.sieve` format: every reader
reads what this build makes, and what it read before. Full figures and method:
`Sieve/results/v33-measured-2026-11-03.md`.

## Measurements at v33 (Linux, 4 processors; sizes the same on any machine)

| | v32 | v33 |
| :--- | ---: | ---: |
| enwik8, for size | 21,547,893 | **21,431,896** (43,600 over PPMd) |
| Silesia, all twelve, for size | 42,091,120 | **41,710,177** (−13.92% against 7z) |
| Silesia, for decoding | 47,262,336 | 47,262,336 |
| Don Quixote | 524,389 | **523,367** |
| control.py | 591 | 591 |
| degrees.sieve | 7,865 | **7,686** |
| welcome to the sieve.sieve | 158 | 158 |

Every `.sieve` was installed again and checked. `Proof/STATISTICS.md` has the v33 rows (At a
glance, enwik8, Silesia file by file, Don Quixote, degrees). `Proof/degrees.sieve` is the v33 one.
`PROOF.md`'s size line now says 7,686. Its installation line ("on my computer ... 0.356s") is about
the v32 file, so you may want to install the new one and update it.

## Two faults the run found, both fixed (`tools/cli/pack.cpp`)

- **The match ways chosen on too short a start.** On enwik8 and samba, all three ways (`R1 M4 C2`)
  won the first 256 KiB and lost whole. enwik8 came to 21,870,393 bytes, larger than v32. `R1` and
  `M4` cost more as a file runs on. Where all three win the first 256 KiB of a file over 4 MiB,
  they are now weighed again against `C2` alone on its first 4 MiB. That 4 MiB choice matched the
  whole file on all seven files tested. Results:
  - enwik8: 21,431,896;
  - samba: 3,277,403;
  - mozilla, osdb, x-ray, sao and ooffice: the same bytes as before.

  Cost: two 4 MiB codes, only on those files (samba took 256.9 s to make, from 231.7 s).
- **The rope's probe (2 November) left `mr`, a scan, to LZMA2 for decoding** (2,735,609 bytes
  against v32's 2,314,195). Its start comes to 1.09 to 1.1, but its middle is 0.92. Where the start
  is from 1.1 up to 1.2, the file's middle is now asked too, and the rope is searched where the
  middle shows it winning. mr is 2,314,195 again, and Silesia's making time is as before (565.4 s).
  Asking the middle on every file cost 238 s and gained nothing more.

## Installing made quicker (`tools/cli/locate.cpp`, `core/src/mix.cpp`)

1. **Files made again from a recipe now use every core.** These are PNGs re-saved, JPEGs from
   brunsli and archives; until now they were made one at a time. On the repository's 95 pictures
   packed with Compressed formats (temporary), installing took **6.2 to 6.5 s, down from 12.9 to
   13.9 s**, with every file the same on all ten installs.
2. **Mixing's large tables on huge pages (Linux only).** Decoding 16 MB of enwik8 took a median
   **25.27 s against 26.16 s** (seven runs each; about 3%). The codes are unchanged.
3. **The x86 stream is decoded beside a mixed lzma2 stream.** Checked with Sieve, sieve-install and
   the oracle, which all give the files back. Not timed: the x86 stream takes under a second.

Looked at and left (reasons in the results file):
- overlapping the mixed lzma2 stream with the other files;
- keeping a primed model between files;
- the rope's decoder;
- mixing in independent blocks (a new format).

## Checks run here

- Unit suite: 154,308 checks, 0 failures.
- CI step 11's rope-probe block: passes, unchanged.
- Your full CI is still to run. Its archive and picture blocks go through the parallel remakes.
  No CI block makes a mixed lzma2 stream beside programs, so the x86 thread was checked by hand
  only (v29, five texts and a program).

## Still yours to run at v33

The photos, pi6, MAPS.WoW, WoWRevived, ALTEngine and the release. STATISTICS still gives those at
v32.
