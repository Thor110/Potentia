# Sieve of Potentia: Statistics

What Sieve's `.sieve` files have come to, file by file, against 7-Zip on the same files. Most of
these were measured on Edward's machine (Windows) at earlier versions of the `.sieve` format, as
each was built; the version is given where it is known. Sizes are in bytes. "On disk" is the space
Windows gives a file, in whole clusters of 4,096 bytes. The ratios and percentages are worked out
from the sizes given. The `sieve-manifest-v33` figures (3 November 2026) were made on a Linux
machine of 4 logical processors: the sizes are the same on any machine, the times are not.

## Contents

- [At a glance](#at-a-glance)
- [The application itself](#the-application-itself)
- [Sieve's source tree, against the usual compressors](#sieves-source-tree-against-the-usual-compressors)
- [enwik8: the first 100 MB of English Wikipedia](#enwik8-the-first-100-mb-of-english-wikipedia)
- [enwik9: the first 1 GB of English Wikipedia](#enwik9-the-first-1-gb-of-english-wikipedia)
- [Four ways each: Try every way and not](#four-ways-each-try-every-way-and-not)
- [Silesia: the standard corpus of mixed files](#silesia-the-standard-corpus-of-mixed-files)
- [A game archive: MAPS.WoW](#a-game-archive-mapswow)
- [A game's release: WoWRevived](#a-games-release-wowrevived)
- [A game engine: ALTEngine](#a-game-engine-altengine)
- [A million digits of pi](#a-million-digits-of-pi)
- [A book: Don Quixote](#a-book-don-quixote)
- [Photographs (JPEG)](#photographs-jpeg)
- [A small script](#a-small-script)
- [Files that are items of Sieve's own lines](#files-that-are-items-of-sieves-own-lines)
- [What carried each file](#what-carried-each-file)

## At a glance

| What | Original | 7z | `.sieve` | Ratio | Against 7z |
| :--- | ---: | ---: | ---: | ---: | ---: |
| [Sieve's own release](#the-application-itself) | 45,967,736 | 8,235,077 | 6,470,236 | 7.10:1 | −21.43% |
| [Sieve's source tree](#sieves-source-tree-against-the-usual-compressors) | 35,396,955 | 6,844,677 | 5,196,966 | 6.81:1 | −24.07% |
| [enwik8](#enwik8-the-first-100-mb-of-english-wikipedia), public test data | 100,000,000 | 26,080,445 | 21,431,896 | 4.67:1 | −17.82% |
| [enwik9](#enwik9-the-first-1-gb-of-english-wikipedia), public test data | 1,000,000,000 | 230,120,517 | 185,105,231 | 5.40:1 | −19.56% |
| [Silesia](#silesia-the-standard-corpus-of-mixed-files), public test data, 12 files | 211,938,580 | 48,455,266 | 41,710,177 | 5.08:1 | −13.92% |
| [MAPS.WoW](#a-game-archive-mapswow), a game's archive | 160,890,897 | 60,791,826 | 52,897,726 | 3.04:1 | −12.99% |
| [WoWRevived](#a-games-release-wowrevived), a game's release, mostly films and music | 1,426,355,971 | 1,073,748,445 | 1,001,994,144 | 1.42:1 | −6.68% |
| [ALTEngine](#a-game-engine-altengine), a game engine's release (v0.9.4) | 25,694,035 | 7,712,804 | 7,345,022 | 3.50:1 | −4.77% |
| [A million digits of pi](#a-million-digits-of-pi) | 1,111,103 | 512,745 | 417,049 | 2.66:1 | −18.66% |
| [Don Quixote](#a-book-don-quixote), plain text | 2,391,678 | 686,020 | 523,367 | 4.57:1 | −23.71% |
| [A photograph](#photographs-jpeg), JPEG | 4,587,473 | 4,575,201 | 4,424,154 | 1.04:1 | −3.30% |
| [A Python script](#a-small-script) of 1,597 bytes | 1,597 | 822 | 591 | 2.70:1 | −28.10% |
| [Every dimension at every degree](#files-that-are-items-of-sieves-own-lines) | 13,321,756 | 22,961 | 7,686 | 1,733:1 | −66.53% |

"Against 7z" is how much smaller (−) or larger (+) the `.sieve` is than the `.7z` of the same files.

## The application itself

Sieve's release folder (the programs and their data) as a `.sieve`, version by version:

| Format | `.sieve` | Unpacks to | Ratio | Of the folder |
| :--- | ---: | ---: | ---: | ---: |
| v28 | 6,383,087 | | | |
| v29 | 6,312,557 | 44,542,021 | 7.06:1 | 14.17% |
| v30 | 6,311,463 | 44,582,469 | 7.06:1 | 14.16% |
| v31 | 6,318,477 | 44,621,183 | 7.06:1 | 14.16% |
| v31, the release | 6,319,561 | 44,647,549 | 7.06:1 | 14.15% |
| **v32** (1 November 2026; the figure in the READMEs) | **6,470,236** | **45,967,736** | **7.10:1** | **14.08%** |

The same folder at v30 as a 7z: **7,774,880** bytes (5.73:1), which the `.sieve` beat by 18.82%.

From v29 to v30 the payload grew by 40,448 bytes and the `.sieve` shrank by 1,094. From v30 to v31
the payload grew by 38,714 bytes and the `.sieve` grew by 7,014.

**v32** was made on Edward's machine on 1 November 2026, in the File Locator with Try every way and
compressed formats as they are: 166 files, 45,967,736 bytes. The folder is 1,320,187 bytes larger
than v31's release, and its `.sieve` 150,675. The File Locator's own 7z (LZMA2 9e, one stream) made
8,235,077, so the `.sieve` is 21.43% under it. It took 265.6 s to make, and installed in 8.8 s
(decoded in 8.457 s, checked and written in 0.294 s).

**The release's source folder** (`release/source`, as the release package carries it, with
`data/anchors/` included: 802 files, 97,770,905 bytes, 85,336,833 of them `data/`), the same day
and the same way:

| | Bytes | Ratio | Of the folder | Against 7z |
| :--- | ---: | ---: | ---: | ---: |
| zip | 59,331,890 | 1.65:1 | 60.68% | +20.43% |
| 7z (the File Locator's: LZMA2 9e, one stream) | 49,266,046 | 1.98:1 | 50.39% | |
| **`.sieve`** | **47,352,238** | **2.06:1** | **48.43%** | **−3.88%** |

It took 572.5 s to make, and installed in 13.4 s (decoded in 12.003 s, checked and written in 1.378
s). Most of its size is files already compressed among the anchors, so the lead is small. The table
below is the tree without `data/anchors/`, measured with the usual compressors.

## Sieve's source tree, against the usual compressors

This repository's own `Sieve` folder at commit `4864f4d`, without `data/anchors/`: 743 files,
35,396,955 bytes. Measured with `Sieve/tools/bench/compare.py` on Linux (4 logical processors), 27
October 2026, every result checked to give the folder back. The 7z was made by py7zr (LZMA2 at its
strongest), and zstd and brotli by their Python packages.

| Method | Bytes | Ratio | Of the folder | Make | Unpack |
| :--- | ---: | ---: | ---: | ---: | ---: |
| **Sieve** (`.sieve`) | **5,196,966** | **6.81:1** | **14.68%** | 224.7 s | 19.39 s |
| 7z (LZMA2 9 extreme, solid) | 6,844,677 | 5.17:1 | 19.34% | 52.8 s | 1.04 s |
| xz -9e | 6,852,652 | 5.17:1 | 19.36% | 50.3 s | 0.58 s |
| brotli 11, window 24 | 6,943,118 | 5.10:1 | 19.62% | 70.1 s | 0.14 s |
| zstd 22, long window | 7,082,928 | 5.00:1 | 20.01% | 28.0 s | 0.07 s |
| zstd 19 | 7,144,868 | 4.95:1 | 20.18% | 16.1 s | 0.05 s |
| gzip -9 | 10,857,358 | 3.26:1 | 30.67% | 7.2 s | 0.17 s |
| bzip2 -9 | 11,029,078 | 3.21:1 | 31.16% | 3.9 s | 1.66 s |
| zip (deflate 9) | 11,194,250 | 3.16:1 | 31.62% | 7.1 s | 0.26 s |

Part of Sieve's lead here is the folder's own: nine English word lists, most of them the lines of
the largest, which Sieve finds and carries as masks over it.

## enwik8: the first 100 MB of English Wikipedia

`enwik8`, the file of the [Large Text Compression Benchmark](https://www.mattmahoney.net/dc/text.html)
and the Hutter Prize: 100,000,000 bytes of Wikipedia's XML. Anyone can download it and repeat this.
Measured on Edward's machine (Windows; an 11th Gen Intel Core i5-11400F, 6 cores and 12 logical
processors, 32 GB of memory), 28 October 2026, at `sieve-manifest-v31`, made in the File Locator
with **Try every way** ticked (so the file was mixed whole: without it, mixing is tried only on
files of 4 KiB to 16 MiB), and made again with **Prioritise Decoding Speed** ticked as well (no
mixing). Made again at `sieve-manifest-v32` on 31 October 2026, the same way: v32's mixing takes
the word list counted by use (`F80`).

| Method | Bytes | Ratio | Of the file | Against 7z | Make | Unpack |
| :--- | ---: | ---: | ---: | ---: | ---: | ---: |
| zip | 35,194,836 | 2.84:1 | 35.19% | +34.95% | about 9 s | under 1 s |
| 7z | 26,080,445 | 3.83:1 | 26.08% | | about 28 s | under 1 s |
| Sieve v31, prioritised for decoding | 24,768,410 | 4.04:1 | 24.77% | −5.03% | 630.0 s | 1.241 s, then checked and written in 0.495 s |
| Sieve v32, prioritised for decoding | 24,768,406 | 4.04:1 | 24.77% | −5.03% | 612.3 s | 1.270 s, then checked and written in 0.494 s |
| Sieve v31, optimised for size | 21,675,870 | 4.61:1 | 21.68% | −16.89% | 691.4 s | 119.643 s, then checked and written in 0.518 s |
| Sieve v32, optimised for size | 21,547,893 | 4.64:1 | 21.55% | −17.38% | 664.3 s | 112.563 s, then checked and written in 0.527 s |
| **Sieve v33** (`.sieve`), optimised for size | **21,431,896** | **4.67:1** | **21.43%** | **−17.82%** | | 145.085 s, then checked and written in 0.427 s (on the Linux machine) |
| PPMd var. J (`enwik8.pmd`), as published | 21,388,296 | 4.68:1 | 21.39% | −17.99% | | |

**At `sieve-manifest-v33` (3 November 2026, the Linux machine, Try every way):** 21,431,896 bytes,
115,997 (0.54%) under v32 and 43,600 (0.20%) over PPMd: mixed with the word list and the match
model's second candidate (`mix4 B22 F80 C2`). The first v33 run chose all three of the match
model's ways on the file's first 256 KiB and made 21,870,393, larger than v32; the packer now
weighs them again on the first 4 MiB of a file over 4 MiB (results/v33-measured-2026-11-03.md).
Prioritised for decoding it is 24,768,406, as at v32. The making time was not measured alone.

Optimised for size, Sieve v32 unpacks it at about 0.89 MB a second: mixing decodes as slowly as it
codes. v32's word list counted by use took 127,977 bytes off v31's (0.59%). Prioritised for
decoding (no mixing, so no word list), it is 14.95% larger than that but still 5.03% under 7z, and
unpacks in under two seconds in all (1.774 s).

The times were measured again on 29 October 2026 with **Keep a log** ticked, which records each
step (the making times are the whole reading: the sizes beside zip and 7z included; the unpacking
times are installing from the hallway). Where the time went is under enwik9. The size-optimised
reading was first made straight after the decoding one, in the same window, and took that one's
rope search as found (the File Locator kept searches between readings then; it no longer does):
414.9 s. Made again on its own, with its own search, it took 691.4 s, the figure above.

PPMd's figure is not measured here: it is `enwik8.pmd`, which the benchmark offers as a download
on its page about the test data (<https://mattmahoney.net/dc/textdata.html>, last updated 1
September 2011), made by PPMd var. J at its strongest (`-m256 -o10 -r1`, 256 MB of memory). Sieve
optimised for size is 159,597 bytes (0.75%) larger than it at v32 (287,574, 1.34%, at v31).

**Where that sits in the benchmark.** The [Large Text Compression
Benchmark](https://mattmahoney.net/dc/text.html) (its table as updated 14 October 2026) ranks
programs by enwik9's size plus the zipped size of the program that unpacks it. Some of its rows, as
published (sizes in bytes; its times are on other machines, so they are left out):

| Program, options (as published) | enwik8 | enwik9 |
| :--- | ---: | ---: |
| cmix v21 `-t` | 14,623,723 | 107,963,380 |
| paq8px_v206fix1 `-12L` | 15,849,084 | 124,696,410 |
| ppmonstr J `-m1700 -o16` | 19,055,092 | 157,007,383 |
| 7zip 4.46a `-m0=ppmd:mem=1630m:o=10 ...` | 21,197,559 | 178,965,454 |
| ppmd J1 `-m256 -o10 -r1` | 21,388,296 | 183,964,915 |
| xz 5.2.1 `--lzma2=preset=9e,dict=1GiB,lc=4,pb=0` | 24,703,772 | 197,331,816 |
| zstd 0.6.0 `-22 --ultra` | 25,405,601 | 215,674,670 |
| brotli 18-Feb-2016 `-q 11 -w 24` | 25,764,698 | 223,597,884 |
| bzip2 1.0.2 `-9` | 29,008,736 | 253,977,839 |
| gzip 1.3.5 `-9` | 36,445,248 | 322,591,995 |

On enwik8, Sieve optimised for size (21,547,893 at v32) falls between PPMd and xz at its strongest; the
context-mixing programs at the top, made for this kind of test and far slower, are about a third
smaller. The table's leaders now (transformer models, about 92 MB on enwik9) give no enwik8 figure. For Sieve, the
program that unpacks is `sieve-install`.

## enwik9: the first 1 GB of English Wikipedia

`enwik9`, the benchmark's larger file: 1,000,000,000 bytes, of which enwik8 is the first tenth.
Measured on Edward's machine as enwik8 was, both ways, with **Try every way** ticked (29 October
2026, `sieve-manifest-v31`, the build with the third review's changes), and zip and 7z beside them,
made by 7-Zip's right-click "Add to" at its default settings (Normal: LZMA2 for the `.7z`, Deflate
for the `.zip`), not at their strongest.

| Method | Bytes | Ratio | Of the file | Against 7z | Make | Unpack |
| :--- | ---: | ---: | ---: | ---: | ---: | ---: |
| zip | 312,712,170 | 3.20:1 | 31.27% | +35.89% | 1 min 30 s | about 4 s |
| 7z | 230,120,517 | 4.35:1 | 23.01% | | 1 min 37 s | about 1 s |
| Sieve v31, prioritised for decoding | 211,195,701 | 4.73:1 | 21.12% | −8.22% | 5,999.7 s (1 h 40 min) | 10.892 s, then checked and written in 4.962 s |
| Sieve v32, prioritised for decoding | 211,195,696 | 4.73:1 | 21.12% | −8.22% | 5,035.7 s (1 h 24 min) | 10.902 s, then checked and written in 4.982 s |
| Sieve v31, optimised for size | 186,081,966 | 5.37:1 | 18.61% | −19.14% | 6,631.6 s (1 h 50 min) | 1,153.583 s, then checked and written in 5.332 s |
| **Sieve v32** (`.sieve`), optimised for size | **185,105,231** | **5.40:1** | **18.51%** | **−19.56%** | 5,627.9 s (1 h 34 min) | 1,097.360 s, then checked and written in 5.050 s |
| PPMd var. J (`enwik9.pmd`), as published | 183,964,915 | 5.44:1 | 18.40% | −20.06% | | |

Made again at `sieve-manifest-v32` on the night of 31 October 2026 by `tools/bench_files.bat` (no
search limit; for size with Try every way, for decoding without it, which makes the same file):
v32's word list counted by use took 976,735 bytes (0.52%) off v31's, and the gap to PPMd from 1.15%
to 0.62%. At v31: optimised for size, Sieve was 19.14% under 7z (on enwik8, 16.89%) and 2,117,051 bytes (1.15%) larger
than PPMd at its strongest; it unpacks at about 0.87 MB a second (mixing decodes as slowly as it codes). Prioritised for decoding, it is
13.50% larger than that, still 8.22% under 7z, and decodes at about 92 MB a second: 15.9 s in all,
checked and written (29 October 2026, measured again).

**Where the making time goes** (29 October 2026, from **Keep a log**: the seconds between the
reports, so the steps add up to the whole, give or take the rounding). Each with **Try every way**
ticked and no search limit; enwik9 for size was measured on 30 October.

| Step | enwik8, for decoding | enwik8, for size | enwik9, for decoding | enwik9, for size |
| :--- | ---: | ---: | ---: | ---: |
| timing samples for the estimate, reading it, and zip and 7z beside it | 99.9 s | 100.2 s | 837.8 s | 902.9 s |
| … of which 7z (LZMA2, preset 9 extreme) | 87.3 s | 81.7 s | 777.1 s | 834.0 s |
| packing: the rope's search, the pieces, and (for size) mixing | 345.2 s | 462.4 s | 3,420.2 s | 4,496.8 s |
| … of which the rope's search | 308.7 s | 304.0 s | 3,302.7 s | 3,264.2 s |
| packing: the lzma2 stream, LZMA2's settings tried | 175.4 s | 0.1 s (32 bytes) | 1,628.7 s | 1.1 s (33 bytes) |
| reading it back, as installing will (every file decoded and compared) | 1.3 s | 121.9 s | 10.4 s | 1,139.8 s |
| the instructions set beside zip and 7z, their address, and the weighing | 8.2 s | 6.8 s | 102.6 s | 90.9 s |
| **in all** | **630.0 s** | **691.4 s** | **5,999.7 s** | **6,631.6 s** |

On enwik9 for decoding the rope's search is 55% of the whole and LZMA2's passes 27%. The search
did not change what was made: the rope's best, 598,993,276 bytes, was larger than LZMA2's
211,195,636 and mixing's 186,081,894, so it was not taken either way. LZMA2's passes did: they
took the stream from 7-Zip's encoder's 211,776,158 bytes to 211,195,636. For size, the packing
row holds mixing too (1,115.5 s of it). Reading it back costs what installing costs, since it is
the same decoding: on enwik8 optimised for size, 121.9 s against an install's 119.643 s; on enwik9,
1,139.8 s against 1,153.583 s. The log did not yet say so when these
were made (it was missing from the reports: now "reading them back, as installing will"). The
weighing's own search, the 734 s and 790.5 s of the first runs, took 1.0 s on enwik9 here: it now
takes the file's ways as packing found them.

At v32 it is 1,140,316 bytes (0.62%) larger than PPMd, and unpacks at about 0.91 MB a second.

PPMd's figure is from the same page as enwik8's, not measured here; the benchmark's other published
enwik9 sizes are in the enwik8 section's second table.

## Four ways each: Try every way and not

`tools/bench_files.bat` (31 October to 1 November 2026, Edward's machine, Sieve at v32, no search
limit) made each file four ways, each installed again and checked: for size and for decoding speed,
each with Try every way and without. The `.sieve` in bytes; the 7z here is LZMA2 at preset 9e as one
stream, as the File Locator gives it (not 7-Zip's own, as in the sections above):

| File | 7z (9e) | size, Try every way | size, without | decoding, Try every way | decoding, without |
| :--- | ---: | ---: | ---: | ---: | ---: |
| enwik9 | 211,776,158 | **185,105,231** | 211,195,696 | 211,195,696 | 211,195,696 |
| enwik8 | 24,831,587 | **21,547,893** | 24,768,406 | 24,768,406 | 24,768,406 |
| 20240407_153939.jpg | 4,575,072 | **4,425,909** | 4,557,967 | 4,557,967 | 4,557,967 |
| 20230830_163945.jpg | 4,127,288 | **3,980,423** | 4,111,467 | 4,111,467 | 4,111,467 |
| A picture at 3653×5480 | 3,844,130 | **3,716,743** | **3,716,743** | 3,826,022 | 3,826,022 |
| pg996.txt | 685,794 | **524,389** | **524,389** | 683,194 | 683,194 |
| pi6.txt | 505,978 | **417,049** | **417,049** | **417,049** | **417,049** |
| control.py | 690 | **591** | 722 | 722 | 722 |

What it shows:
- **Try every way matters for size where mixing's limits apply:** files over 16 MiB (enwik8 and
  enwik9, mixed only with it) and under 4 KiB (`control.py`). It matters too where mixing's test on a
  file's start says no: two of the photographs, whose starts mix no better than LZMA2.
- **Elsewhere it changes nothing:** Don Quixote, the third photograph and pi come to the same bytes
  without it. Making takes longer with it, though: pi 10.0 s against 2.4.
- **For decoding speed it changes nothing here**, since no file is mixed. It only lifts the rope's
  early stops, and on these files that found nothing more.
- **The time:**
  - enwik9 took 5,627.9 s for size with it, against 5,213.5 without.
  - Installing it took 1,097.4 s mixed, against 11.4 s.

The full tables, with each way's making, decoding and checking times, are in the run's results.md.

**At `sieve-manifest-v33` (3 November 2026, Edward's machine, the same four ways):** the three
photographs come to 3,715,757, 3,978,510 and 4,424,154 for size with Try every way (each 986 to
1,913 bytes under v32) and to v32's bytes otherwise; Don Quixote 523,367 for size (with Try every
way or without), 683,194 for decoding; pi6.txt 417,049 and control.py 591 and 722, as at v32. Every
`.sieve` installed again the same.

## Silesia: the standard corpus of mixed files

The [Silesia corpus](https://sun.aei.polsl.pl/~sdeor/index.php?page=silesia): twelve files of
211,938,580 bytes in all (texts, a dictionary, XML, a database, programs, medical and astronomical
data), each measured on its own. Made by `tools/bench_files.bat` on Edward's machine, 1 November
2026, Sieve at v32, no search limit, four ways each; every `.sieve` was installed again and checked.
The 7z is LZMA2 at preset 9e as one stream a file, without its container (as the File Locator gives
it), not 7-Zip's own.

| File | Bytes | 7z (9e) | size, Try every way | size, without | decoding (either) | Against 7z, for size |
| :--- | ---: | ---: | ---: | ---: | ---: | ---: |
| mozilla | 51,220,480 | 13,376,180 | **13,376,239** | 13,376,239 | 13,376,239 | +59 bytes |
| webster | 41,458,703 | 8,368,609 | **6,246,725** | 8,355,800 | 8,355,800 | −25.36% |
| nci | 33,553,445 | 1,449,209 | **1,293,653** | 1,440,114 | 1,440,114 | −10.73% |
| samba | 21,606,400 | 3,739,464 | **3,287,995** | 3,728,015 | 3,728,015 | −12.07% |
| dickens | 10,192,446 | 2,831,149 | **2,210,442** | 2,210,442 | 2,825,117 | −21.92% |
| osdb | 10,085,684 | 2,844,493 | **2,389,894** | 2,389,894 | 2,835,826 | −15.98% |
| mr | 9,970,564 | 2,751,831 | **1,967,650** | 1,967,650 | 2,314,195 | −28.50% |
| x-ray | 8,474,240 | 4,491,203 | **3,622,683** | 3,622,683 | 4,146,903 | −19.34% |
| sao | 7,251,944 | 4,425,604 | **4,170,594** | 4,413,371 | 4,413,371 | −5.76% |
| reymont | 6,627,202 | 1,315,531 | **1,093,050** | 1,093,050 | 1,313,857 | −16.91% |
| ooffice | 6,152,192 | 2,427,163 | **2,079,028** | 2,079,028 | 2,079,028 | −14.34% |
| xml | 5,345,280 | 434,830 | **353,167** | 353,167 | 433,871 | −18.78% |
| **all twelve** | **211,938,580** | **48,455,266** | **42,091,120** | **45,029,453** | **47,262,336** | **−13.13%** |

| All twelve | Bytes | Ratio | Of the corpus | Against 7z | Made in | Installed in |
| :--- | ---: | ---: | ---: | ---: | ---: | ---: |
| zip (deflate 9) | 67,643,201 | 3.13:1 | 31.92% | +39.60% | | |
| 7z (LZMA2 9e) | 48,455,266 | 4.37:1 | 22.86% | | | |
| Sieve, prioritised for decoding | 47,262,336 | 4.48:1 | 22.30% | −2.46% | 803.3 s | 3.6 s |
| Sieve, for size, without Try every way | 45,029,453 | 4.71:1 | 21.25% | −7.07% | 875.4 s | 49.6 s |
| **Sieve, for size, Try every way** | **42,091,120** | **5.04:1** | **19.86%** | **−13.13%** | 1,279.5 s | 141.6 s |

- **Try every way's share:** without it, mixing is tried only on files of 4 KiB to 16 MiB. With it,
  `webster` (41 MB) went from 8,355,800 to 6,246,725, and `nci` and `samba` were mixed too.
- **`mozilla`, a tar of programs:** it was carried by LZMA2 as 7z carries it, 59 bytes larger (the
  head and listing). Sieve reads it as one file of unknown kind, so its programs inside get no x86
  filter, and mixing came out no smaller. A tar read as its members would let each program through
  the x86 filter, but nothing here measures how much that would save.
- **For decoding speed:** Try every way made no difference to any file.

**At `sieve-manifest-v33` (3 November 2026, the Linux machine, by `tools/bench/bench_files.py`,
for size with Try every way and for decoding without it; every `.sieve` installed again and
checked).** All twelve came to **41,710,177 bytes for size** (5.08:1, 13.92% under 7z, 380,943
under v32) and 47,262,336 for decoding (v32's bytes, made in 565.4 s and installed in 5.6 s in
all here):

| File | 7z (9e) | v33, for size, Try every way | Against v32 | v33, for decoding | Against 7z, for size | Ways (for size) |
| :--- | ---: | ---: | ---: | ---: | ---: | :--- |
| mozilla | 13,376,180 | **13,318,582** | −57,657 | 13,376,239 | −0.43% | `mix4 B22 R1 M4 C2` |
| webster | 8,368,609 | **6,226,845** | −19,880 | 8,355,800 | −25.59% | `mix4 B22 F80 C2` |
| nci | 1,449,209 | **1,281,583** | −12,070 | 1,440,114 | −11.57% | `mix4 B22 C2` |
| samba | 3,739,464 | **3,277,403** | −10,592 | 3,728,015 | −12.36% | `mix4 B22 C2` |
| dickens | 2,831,149 | **2,205,961** | −4,481 | 2,825,117 | −22.08% | `mix4 B22 F80 C2` |
| osdb | 2,844,493 | **2,364,331** | −25,563 | 2,835,826 | −16.88% | `mix4 B22 R1 M4 C2` |
| mr | 2,751,831 | **1,967,650** | 0 | 2,314,195 | −28.50% | `mix4 B22 I512x2` |
| x-ray | 4,491,203 | **3,614,947** | −7,736 | 4,146,903 | −19.51% | `mix4 B22 I1901x2 R1 M4 C2` |
| sao | 4,425,604 | **4,149,429** | −21,165 | 4,413,371 | −6.24% | `mix4 B22 I12264x1 R1 M4 C2` |
| reymont | 1,315,531 | **1,089,497** | −3,553 | 1,313,857 | −17.18% | `mix4 B22 C2` |
| ooffice | 2,427,163 | **1,864,843** | −214,185 | 2,079,028 | −23.17% | `mix4 B22 R1 M4 C2 X` |
| xml | 434,830 | **349,106** | −4,061 | 433,871 | −19.71% | `mix4 B22 F80 C2` |
| **all twelve** | **48,455,266** | **41,710,177** | **−380,943** | **47,262,336** | **−13.92%** | |

- **v33's ways:** the match model's (`R1 M4 C2`, or `C2` alone) on every file mixed but `mr`, and
  the x86 filter on `ooffice`, a program, 10.3% smaller than v32 made it (in the x86 stream).
- **Two faults found by this run and fixed (no change to the format):** `samba` and enwik8 had all
  three match ways chosen on their first 256 KiB and came out larger than at v32 (samba 3,342,302);
  and the rope's test on a file's start (2 November) left `mr` to LZMA2 for decoding (2,735,609).
  Both are in results/v33-measured-2026-11-03.md.

## A game archive: MAPS.WoW

A file from Jeff Wayne's *The War of the Worlds* (1998, PC), in the game's own `.WoW` archive format.

| | Bytes | Ratio | Against 7z |
| :--- | ---: | ---: | ---: |
| MAPS.WoW | 160,890,897 | | |
| MAPS.7z | 60,791,826 | 2.65:1 | |
| MAPS.WoW.sieve, early results | 60,130,147 | 2.68:1 | −1.09% |

Since v28, two ways of making it, against 7-Zip:

| | Size | Making it | Unpacking it |
| :--- | ---: | ---: | ---: |
| 7-Zip | 60,791,826 | about 18 s | about 1 s |
| Sieve, optimised for size | 52,996,145 | 350.1 s | 202.769 s, then checked and written in 0.918 s |
| **Sieve v33, optimised for size** | **52,897,726** | 2,684.8 s | 166.710 s, then checked and written in 1.137 s |
| Sieve, optimised for decoding | 60,130,138 | 349.5 s | 4.563 s, then checked and written in 0.928 s |

Optimised for size, it is 12.82% smaller than 7-Zip's and takes about three and a half minutes to
unpack; optimised for decoding, 1.09% smaller and about five and a half seconds in all.

**At `sieve-manifest-v33` (3 November 2026, Edward's machine, the File Locator with Keep a log;
Optimise Search on, which turns Try every way on for a file over 16 MiB):** 52,897,726 bytes (3.04:1),
98,419 under v32's and 12.99% under 7-Zip's, mixed whole with the match model's second candidate
(`mix4 B22 C2`); installed in 167.8 s, checked against its SHA-256. Making it took 44 min 45 s, most
of it the rope: its search on the whole file took about 1,020 s, and the weighing's repeated it (987
s), to 127,663,746 bytes, though LZMA2 makes 60,130,073 and mixing 52,897,658. The rope's test on
the file's start did not leave it out this time (IDEAS §19.118).

The potential utility line: at 100 MB or more, the search's time begins to tell.

## A game's release: WoWRevived

Edward's WoWRevived, the final release of his restoration of *The War of the Worlds* (English,
v1.1.2): 237 files, most of the size upscaled films and music, already compressed. Made on Edward's
machine, 28 October 2026, in the File Locator with Try every way and Prioritise Decoding Speed both
ticked (so no file mixed).

| | Bytes | Ratio | Of the folder | Against 7z |
| :--- | ---: | ---: | ---: | ---: |
| The folder | 1,426,355,971 | | | |
| zip | 1,195,057,828 | 1.19:1 | 83.78% | +11.30% |
| 7z | 1,073,748,445 | 1.33:1 | 75.28% | |
| **`.sieve`** | **1,075,924,966** | **1.33:1** | **75.43%** | **+0.20%** |
| The installer program (`sieve-install` and the `.sieve`) | 1,080,422,398 | 1.32:1 | 75.75% | +0.62% |

7z won here, by 2,176,521 bytes (0.20%): on files already compressed there is little for either to
take, and which files the difference is in is not yet measured (Sieve's mixing was off). Installed from the
hallway: 237 files, decoded in 62.658 s, checked and written in 7.215 s; `sieve-install` (the
installer program) took 6 to 12 GB of memory while it read the file, and stopped answering until it
had.

What it found: reading a `.sieve` went through one number the size of the file, and its hex text,
several copies at once; and the rope's coder held every byte as 64 bits in several arrays. Both were
remade the same day with the same results (IDEAS §19.105). Measured here on a folder of 377 MB made
like it (three 125 MB files of noise, as compressed films are, and this repository's docs): installing
63 s and 11.5 GB at most → 13.6 s and 2.4 GB; making 205 s and 11.8 GB → 182 s and 4.1 GB.

The installer program less the `.sieve` and its 24-byte trailer gives `sieve-install` itself on
Windows: 4,497,408 bytes, every installer program's fixed cost.

**Made again with v32 (31 October 2026, Edward's machine, the File Locator, Try every way and Keep
a log; compressed formats as they are).** The two maps are 98.7% alike, and v32 carries the larger
as its differences from the smaller (a script of 2,087,830 bytes before LZMA2, for 162,426,225):

| | Bytes | Ratio | Of the folder | Against 7z | Made in | Installed in |
| :--- | ---: | ---: | ---: | ---: | ---: | ---: |
| **`.sieve`, for size** | **1,001,994,144** | **1.42:1** | **70.25%** | **−6.68%** | 4,997.6 s | 210.3 s |
| `.sieve`, Prioritise Decoding Speed | 1,016,369,457 | 1.40:1 | 71.26% | −5.34% | 4,966.7 s | 30.1 s |

- **For size:**
  - MAPS.WoW mixed, 52,996,076 bytes;
  - the 8 WAVs (433,241,104 bytes) to 311,258,496, 71.8%;
  - the two MP4s (405,723,932) carried as pieces at 99.9%.
- **For decoding speed:** nothing mixed. MAPS.WoW by LZMA2 (60,136,858) and the WAVs by the rope
  (318,253,720, 73.5%).
- **Installing:** the size run decoded in 203.3 s and the decoding-speed run in 22.9 s, each then
  checked and written in about 7 s.

## A game engine: ALTEngine

Edward's ALTEngine v0.9.4, his reimplementation of the *Alien Trilogy* engine: 46 files, most of it
programs (the engine and FFmpeg's and SDL3's DLLs). Made at `sieve-manifest-v33` on Edward's machine
on 3 November 2026, in the File Locator with Keep a log; Try every way off and Optimise Search on,
Prioritise Decoding Speed off, compressed formats as they are. It took 109.1 s to make and installed
in 14.8 s (decoded in 14.630 s, checked and written in 0.144 s), every file checked against its
SHA-256.

| | Bytes | Ratio | Of the folder | Against 7z |
| :--- | ---: | ---: | ---: | ---: |
| The folder | 25,694,035 | | | |
| `ALTEngine-v0.9.4.zip` | 10,075,403 | 2.55:1 | 39.21% | +30.63% |
| `ALTEngine-v0.9.4.7z` | 7,712,804 | 3.33:1 | 30.02% | |
| **`.sieve`** | **7,345,022** | **3.50:1** | **28.59%** | **−4.77%** |

- **The programs, mixed through the x86 filter (v33's `X`):** six of the seven, 22,151,168 bytes,
  came to 6,953,189 mixed (`mix4 ... R1 M4 C2 X`), against LZMA2 with the x86 filter's estimate of
  7,329,091 for the same six. The smallest, `swresample-6.dll` (124,416 bytes), stayed in the x86
  stream, which was smaller for it. The largest, `avcodec-62.dll` (13,606,400 bytes), kept all
  three match ways after v33's check on its first 4 MiB: 4,387,190 against 4,560,553.
- **Installing:** mixed programs decode as slowly as they were coded, so most of the 14.8 s is
  theirs.
- **The File Locator's own 7z** (LZMA2 9e, one stream) made 7,943,117; the `.sieve` is 7.53% under it.

## A million digits of pi

`pi6.txt`, a million digits of pi, from
[the University of Exeter](https://www.exeter.ac.uk/v8media/facultysites/ese/physics/pi6.txt): a
stress test, since digits of pi are as close to random as text gets.

| | Bytes | On disk |
| :--- | ---: | ---: |
| pi6.txt, the original | 1,111,103 | 1,114,112 |
| pi6.7z | 512,745 | 516,096 |
| pi6.txt.sieve, v32 (v33 the same) | **417,049** | |

**The floor.** A million digits treated as random hold 1,000,000 × log₂10 ÷ 8 = **415,241** bytes of
information, and no file of them can be smaller on average. The latest `.sieve` is **1,808 bytes**
above it, within 0.44%. Its progress, from an early v4 to the newest version of the format (its
overhead and filter settings):

| Step | `pi6.txt.sieve` | On disk | Above the floor | The floor below it | Against 7z |
| :--- | ---: | ---: | ---: | ---: | ---: |
| An early v4 | 490,473 | 491,520 | +18.1177% | −15.3387% | −4.34% |
| | 421,491 | 421,888 | +1.5052% | −1.4828% | −17.80% |
| | 419,115 | 421,888 | +0.9330% | −0.9243% | −18.26% |
| | 419,096 | 421,888 | +0.9284% | −0.9198% | −18.26% |
| | 417,139 | 417,792 | +0.4571% | −0.4550% | −18.65% |
| Another step's reductions | 417,061 | 417,792 | +0.4383% | −0.4364% | −18.66% |
| The rope wave updated | 417,052 | 417,792 | +0.4361% | −0.4342% | −18.66% |
| v32's smaller head (31 October 2026) | **417,049** | | +0.4354% | −0.4335% | −18.66% |

The v32 figure was made on Edward's machine with Try every way (in 11.4 s; installed in 0.04 s): the
rope, 416,970 bytes, came under mixing (418,822) and LZMA2 (507,033), as before, and v32's head took
3 bytes off.

Made again on 2 November 2026 (Sieve 0.13.77, on Edward's machine, Try every way off and Optimise
Search on, the File Locator's defaults): the same 417,049 bytes in **2.8 s** (the rope's search 0.05
s; mixing, weighed beside it, 1.7 s), installed and checked in 0.045 s. The rope came to 416,970
bytes again, under mixing (418,822) and the File Locator's own 7z of the file (505,978). The twelve
bytes from 417,061 are the last two steps above: 9 from the rope wave's update, 3 from v32's head.

"Above the floor" is how much larger than 415,241 bytes the file is, as a share of the floor; "the
floor below it" is how much smaller the floor is, as a share of the file; "against 7z" is how much
smaller the `.sieve` is than `pi6.7z` (512,745 bytes).

## A book: Don Quixote

`pg996.txt`, Project Gutenberg's *Don Quixote*, as plain text.

| | Bytes | On disk | Ratio | Against 7z |
| :--- | ---: | ---: | ---: | ---: |
| pg996.txt | 2,391,678 | 2,392,064 | | |
| pg996.7z | 686,020 | 688,128 | 3.49:1 | |
| pg996.txt.sieve, v31 | 533,148 | | 4.49:1 | −22.28% |
| pg996.txt.sieve, v32 | 524,389 | | 4.56:1 | −23.56% |
| **pg996.txt.sieve, v33** | **523,367** | | **4.57:1** | **−23.71%** |

The `.sieve` was measured again on 30 October 2026, at `sieve-manifest-v31` (mixed, with the word
list), with Sieve 0.13.77's `sieve locate pg996.txt --installer` at its defaults: 533,148 bytes,
installed back identical. That is the size HANDOFF gives for it at v28. The 683,205 shown here
before was from before mixing (v23 gave 683,195). On the Linux machine here it took 13.3 s to make
and 5.3 s to install; its size on disk was not measured.

Made again at `sieve-manifest-v32` on 31 October 2026, on Edward's machine with Try every way: **524,389
bytes**, 8,759 (1.64%) under v31's, mixed with v32's word list counted by use (`mix4 B22 F80`, 524,319
of it the file). It took 21.8 s to make and 2.9 s to install.

At `sieve-manifest-v33` (3 November 2026, the Linux machine, Try every way): **523,367 bytes**, 1,022
(0.19%) under v32's, mixed with the word list and the match model's second candidate (`mix4 B22 F80
C2`), made in 25.2 s and installed in 4.2 s here.

## Photographs (JPEG)

A JPEG is compressed already, so neither 7-Zip nor Sieve can take much off. Both `.sieve` files are
given: of the photograph itself, and of its `.7z`.

| | Original | 7z | `.7z.sieve` | `.jpg.sieve` | Against 7z |
| :--- | ---: | ---: | ---: | ---: | ---: |
| A picture at 3653×5480 someone sent a link to | 3,844,213 | 3,843,942 | 3,829,453 | 3,716,747 | −3.31% |
| 20230830_163945.jpg | 4,139,682 | 4,127,434 | 4,112,541 | 4,111,478 | −0.39% |
| 20240407_153939.jpg | 4,587,473 | 4,575,201 | 4,559,089 | 4,557,978 | −0.38% |

The `.jpg.sieve` column was measured again on 30 October 2026 (Sieve 0.13.77, `sieve locate` at
its defaults, from Edward's originals); the `.7z.sieve` column and the sizes on disk are from the
first measurement.

**At `sieve-manifest-v32`, 31 October 2026,** made on Edward's machine with Try every way and the
Compressed formats boxes off (so each JPEG is carried as its bytes, not as its brunsli form). Each
was mixed as a picture of the shape its bytes suggest, 3% to 4% under the original:

| | Original | `.jpg.sieve`, v32 | Against 7z | Made in | Installed in |
| :--- | ---: | ---: | ---: | ---: | ---: |
| A picture at 3653×5480 | 3,844,213 | 3,716,743 | −3.31% | 51 s | 4.4 s |
| 20230830_163945.jpg | 4,139,682 | 3,980,423 | −3.56% | 52 s | 4.7 s |
| 20240407_153939.jpg | 4,587,473 | 4,425,909 | −3.26% | 60 s | 5.0 s |

With the temporary box ticked, v32 carries each as its brunsli form (the table below: about 20% off).

**At `sieve-manifest-v33`, 3 November 2026,** on Edward's machine by `tools/bench_files.bat`, Try
every way, the Compressed formats boxes off. Each is mixed, one now with the match model's second
candidate (`C2`); every one installed again the same:

| | Original | `.jpg.sieve`, v33 | Against v32 | Against 7z | Made in | Decoded in |
| :--- | ---: | ---: | ---: | ---: | ---: | ---: |
| A picture at 3653×5480 | 3,844,213 | 3,715,757 (`mix4 B22`) | −986 | −3.33% | 56.6 s | 4.9 s |
| 20230830_163945.jpg | 4,139,682 | 3,978,510 (`mix4 B22 C2`) | −1,913 | −3.61% | 55.1 s | 5.2 s |
| 20240407_153939.jpg | 4,587,473 | 4,424,154 (`mix4 B22`) | −1,755 | −3.30% | 62.0 s | 5.6 s |

With a Compressed formats box ticked, each is its brunsli form: 3,085,301, 3,284,965 and 3,660,094
bytes with temporary (19.74%, 20.65% and 20.22% under the original), made in about 5 s and decoded
in about 0.4 s; with permanent the same but for the two photographs' 78 and 85 bytes more
(3,285,043 and 3,660,179); those two installed as what permanent allows, not byte for byte (Sieve
checked what it wrote), the picture at 3653×5480 byte for byte.

**What a JPEG recompressor takes off (now v32's `jbr`; NEXT-VERSION §4a).** The same three files,
the same day, each given back byte for byte:

| | Original | brunsli | `cjxl --lossless_jpeg=1 -e 9` |
| :--- | ---: | ---: | ---: |
| A picture at 3653×5480 | 3,844,213 | 3,085,201 (−19.74%) | 3,109,231 (−19.12%) |
| 20230830_163945.jpg | 4,139,682 | 3,284,887 (−20.65%) | 3,312,460 (−19.98%) |
| 20240407_153939.jpg | 4,587,473 | 3,660,016 (−20.22%) | 3,703,771 (−19.26%) |

brunsli made each in 0.6 to 0.9 s and gave it back in about 0.5 s; `cjxl` took about 6.3 s and gave
it back in about 0.2 s. brunsli is in Sieve now, as v32's `jbr`, under the Compressed formats boxes.

On disk:

| | Original | 7z | `.7z.sieve` | `.jpg.sieve` |
| :--- | ---: | ---: | ---: | ---: |
| 1-amorgos.jpg.pagespeed.ce.Mgygi9l3S-.jpg | 3,846,144 | 3,846,144 | 3,829,760 | 3,829,760 |
| 20230830_163945.jpg | 4,141,056 | 4,128,768 | 4,116,480 | 4,112,384 |
| 20240407_153939.jpg | 4,587,520 | 4,575,232 | 4,562,944 | 4,558,848 |

## A small script

`control.py`, a Python script of 1,597 bytes. It was first measured at 832 bytes, 10 more than the
7z: then, what every `.sieve` carries besides the file (its listing, and the file's hash for
checking it) cost more than Sieve saved on a file this small. Measured again on 30 October 2026
(Sieve 0.13.77, `sieve-manifest-v20`, by `sieve locate` and by the File Locator alike), it is
**731 bytes, 11.07% under the 7z**: since v16 and v20 the head and the listing are written in bytes.
Of the 731, 670 are the file packed; the rest is 5 bytes of `sieve`, the version and flags, 5 for
the stream's sizes and dictionary, the 32-byte SHA-256, and 17 for the listing (its name, size and
way). The `.7z.sieve` row is from the first measurement and was not measured again.

**At `sieve-manifest-v32`, 31 October 2026** (Edward's machine, Try every way): **591 bytes**, 28.10%
under the 7z. Try every way lifts mixing's 4 KiB floor, so the script was mixed with the word list
counted by use (`mix4 B16 F80`): 522 bytes against LZMA2's 690. The other 69 bytes are the head, the
listing (with mixing's parameters, longer than `raw`) and the code's length.

| | Bytes | Against 7z |
| :--- | ---: | ---: |
| control.py | 1,597 | |
| control.7z | 822 | |
| control.7z.sieve | 959 | +16.67% |
| control.py.sieve, v20 | 731 | −11.07% |
| **control.py.sieve, v32** | **591** | **−28.10%** |

All four take one cluster on disk, 4,096 bytes.

## Files that are items of Sieve's own lines

`degrees.sieve` (in this folder) holds 3,960 files: an item of every dimension at every whole degree
from 0 to 359. Each is an item of one of Sieve's lines, so the `.sieve` carries only its place on its
line, and Sieve makes it again on installing. 13,321,756 bytes come to **7,686** (1,733:1; v33,
made again 3 November 2026 on the Linux machine with Try every way, in 5.8 s, and installed there
the same, 3,960 files checked; 7,865 as v32, 7,887 as v20), where a 7z of the same folder is 22,961 (the File
Locator's own LZMA2 at preset 9e, one stream, makes 9,226). At v32 it was made in 149.3 s with Try every way, on Edward's machine.
Installed there: 3,960 files, every one checked against its SHA-256, decoded in 0.356
s, checked and written in 1.428 s.

This works only for files that are Sieve's own items, made from their places; a file from anywhere
else is carried as itself. [PROOF.md](PROOF.md) has the sample, and `welcome to the sieve.sieve`
(the starting page: 32 bytes as 158; 159 as v20), where the overhead is more than the file.

## What carried each file

Sieve tries a file every way it has and keeps the smallest, so a `.sieve` is not "LZMA2 with extras":
which way carried each file is written in its listing, and `tools/ways_tally.py` reads it back from
any number of `.sieve` files, with each file that has a code of its own set against LZMA2 (xz -9e,
raw, as 7-Zip makes it) on the same bytes.

**Whose each way is.** Sieve's own: the rope, mixing (Sieve's models and code; context mixing as a
method is an established one, the PAQ family's), runs and ramps, copies, the lines of another file,
deltas, binary written as text decoded, archives and PNGs made again, and items of Sieve's lines.
Borrowed: LZMA2 and the x86 filter (7-Zip's), and brunsli for JPEGs (Google's). Each `.sieve` is
read back and checked against every file's SHA-256 before it is used, whatever carried it.

**Silesia, Try every way (4 November 2026, Sieve 0.13.77 at v33, on the Linux machine; each file
alone):** mixing carried all twelve. Against LZMA2 on the same files:

| File | Bytes | `.sieve` | LZMA2 alone | Against LZMA2 |
| :--- | ---: | ---: | ---: | ---: |
| dickens | 10,192,446 | 2,205,961 | 2,831,149 | −22.08% |
| mozilla | 51,220,480 | 13,318,582 | 13,376,180 | −0.43% |
| mr | 9,970,564 | 1,967,650 | 2,751,831 | −28.50% |
| nci | 33,553,445 | 1,281,583 | 1,449,209 | −11.57% |
| ooffice | 6,152,192 | 1,864,843 | 2,427,163 | −23.17% |
| osdb | 10,085,684 | 2,364,331 | 2,844,493 | −16.88% |
| reymont | 6,627,202 | 1,089,497 | 1,315,531 | −17.18% |
| samba | 21,606,400 | 3,277,403 | 3,739,464 | −12.36% |
| sao | 7,251,944 | 4,149,429 | 4,425,604 | −6.24% |
| webster | 41,458,703 | 6,226,845 | 8,368,609 | −25.59% |
| x-ray | 8,474,240 | 3,614,947 | 4,491,203 | −19.51% |
| xml | 5,345,280 | 349,106 | 434,830 | −19.71% |
| **All twelve** | **211,938,580** | **41,710,177** | **48,455,266** | **−13.92%** |

**Elsewhere, the same day and the same way:** Sieve's `docs` folder (42 files, 2,689,351 bytes)
mixed as one group: 1,088,596 bytes against LZMA2's 1,222,601 on the same files joined (−10.96%).
The web test site (11 files, 478,405 bytes, most of it JPEG and PNG pictures, which LZMA2 carried in
the shared stream): its pages mixed, 2,823 bytes against 4,915.

**From Edward's own benchmark (3 November 2026, his machine, v33):** optimised for size, the rope
carried `pi6.txt` (416,970 bytes, under mixing's 418,822 and LZMA2's 507,033); mixing carried Don
Quixote (523,291) and, with Try every way, the small script (520; without it, LZMA2's 670). The
three photographs: with compressed formats as they are, mixing with Try every way and the rope cut
into pieces without it; with JPEGs made again (permanent or temporary), brunsli's form inside the
LZMA2 stream, the smallest of all.
Optimised for decoding, mixing is left out (it decodes far more slowly): LZMA2 then carried the book
(683,133) and the script (670), and the rope still carried `pi6.txt`, and the photographs where
compressed formats were kept as they are, as it decodes quickly.

So where the smallest file is wanted, Sieve's own ways carried nearly every byte measured here; where
fast decoding is wanted, LZMA2 takes over what mixing carried, on purpose, and the rope keeps its
files. `python tools/ways_tally.py FOLDER --sieve SIEVE` (or `tools\ways_tally.bat`, which asks
for the folder) gives the same tables for any `.sieve` files.
