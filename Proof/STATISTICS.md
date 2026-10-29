# Sieve of Potentia: Statistics

What Sieve's `.sieve` files have come to, file by file, against 7-Zip on the same files. Most of
these were measured on Edward's machine (Windows) at earlier versions of the `.sieve` format, as
each was built; the version is given where it is known. Sizes are in bytes. "On disk" is the space
Windows gives a file, in whole clusters of 4,096 bytes. The ratios and percentages are worked out
from the sizes given.

## Contents

- [At a glance](#at-a-glance)
- [The application itself](#the-application-itself)
- [Sieve's source tree, against the usual compressors](#sieves-source-tree-against-the-usual-compressors)
- [enwik8: the first 100 MB of English Wikipedia](#enwik8-the-first-100-mb-of-english-wikipedia)
- [enwik9: the first 1 GB of English Wikipedia](#enwik9-the-first-1-gb-of-english-wikipedia)
- [A game archive: MAPS.WoW](#a-game-archive-mapswow)
- [A game's release: WoWRevived](#a-games-release-wowrevived)
- [A million digits of pi](#a-million-digits-of-pi)
- [A book: Don Quixote](#a-book-don-quixote)
- [Photographs (JPEG)](#photographs-jpeg)
- [A small script](#a-small-script)
- [Files that are items of Sieve's own lines](#files-that-are-items-of-sieves-own-lines)

## At a glance

| What | Original | 7z | `.sieve` | Ratio | Against 7z |
| :--- | ---: | ---: | ---: | ---: | ---: |
| [Sieve's own release](#the-application-itself) | 44,647,549 | | 6,319,561 | 7.06:1 | |
| [Sieve's source tree](#sieves-source-tree-against-the-usual-compressors) | 35,396,955 | 6,844,677 | 5,196,966 | 6.81:1 | −24.07% |
| [enwik8](#enwik8-the-first-100-mb-of-english-wikipedia), public test data | 100,000,000 | 26,080,445 | 21,675,870 | 4.61:1 | −16.89% |
| [enwik9](#enwik9-the-first-1-gb-of-english-wikipedia), public test data | 1,000,000,000 | 230,120,517 | 186,081,966 | 5.37:1 | −19.14% |
| [MAPS.WoW](#a-game-archive-mapswow), a game's archive | 160,890,897 | 60,791,826 | 52,996,145 | 3.04:1 | −12.82% |
| [WoWRevived](#a-games-release-wowrevived), a game's release, mostly films and music | 1,426,355,971 | 1,073,748,445 | 1,075,924,966 | 1.33:1 | +0.20% |
| [A million digits of pi](#a-million-digits-of-pi) | 1,111,103 | 512,745 | 417,052 | 2.66:1 | −18.66% |
| [Don Quixote](#a-book-don-quixote), plain text | 2,391,678 | 686,020 | 683,205 | 3.50:1 | −0.41% |
| [A photograph](#photographs-jpeg), JPEG | 4,587,473 | 4,575,201 | 4,558,132 | 1.01:1 | −0.37% |
| [A Python script](#a-small-script) of 1,597 bytes | 1,597 | 822 | 832 | 1.92:1 | +1.22% |
| [Every dimension at every degree](#files-that-are-items-of-sieves-own-lines) | 13,321,756 | 22,961 | 7,887 | 1,689:1 | −65.65% |

"Against 7z" is how much smaller (−) or larger (+) the `.sieve` is than the `.7z` of the same files.

## The application itself

Sieve's release folder (the programs and their data) as a `.sieve`, version by version:

| Format | `.sieve` | Unpacks to | Ratio | Of the folder |
| :--- | ---: | ---: | ---: | ---: |
| v28 | 6,383,087 | | | |
| v29 | 6,312,557 | 44,542,021 | 7.06:1 | 14.17% |
| v30 | 6,311,463 | 44,582,469 | 7.06:1 | 14.16% |
| v31 | 6,318,477 | 44,621,183 | 7.06:1 | 14.16% |
| v31, the release in the READMEs | 6,319,561 | 44,647,549 | 7.06:1 | 14.15% |

The same folder at v30 as a 7z: **7,774,880** bytes (5.73:1), which the `.sieve` beat by 18.82%.

From v29 to v30 the payload grew by 40,448 bytes and the `.sieve` shrank by 1,094. From v30 to v31
the payload grew by 38,714 bytes and the `.sieve` grew by 7,014.

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
mixing).

| Method | Bytes | Ratio | Of the file | Against 7z | Make | Unpack |
| :--- | ---: | ---: | ---: | ---: | ---: | ---: |
| zip | 35,194,836 | 2.84:1 | 35.19% | +34.95% | about 9 s | under 1 s |
| 7z | 26,080,445 | 3.83:1 | 26.08% | | about 28 s | under 1 s |
| Sieve, prioritised for decoding | 24,768,410 | 4.04:1 | 24.77% | −5.03% | 102.4 s | 1.792 s, then checked and written in 0.538 s |
| **Sieve** (`.sieve`), optimised for size | **21,675,870** | **4.61:1** | **21.68%** | **−16.89%** | 109.3 s | 148.7 s, then checked and written in 0.537 s |
| PPMd var. J (`enwik8.pmd`), as published | 21,388,296 | 4.68:1 | 21.39% | −17.99% | | |

Optimised for size, Sieve makes it at about 0.91 MB a second and unpacks it at about 0.67: mixing
decodes as slowly as it codes. Prioritised for decoding, it is 14.27% larger than that but still
5.03% under 7z, and unpacks in under two and a half seconds in all.

PPMd's figure is not measured here: it is `enwik8.pmd`, which the benchmark offers as a download
on its page about the test data (<https://mattmahoney.net/dc/textdata.html>, last updated 1
September 2011), made by PPMd var. J at its strongest (`-m256 -o10 -r1`, 256 MB of memory). Sieve
optimised for size is 287,574 bytes (1.34%) larger than it.

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

On enwik8, Sieve optimised for size (21,675,870) falls between PPMd and xz at its strongest; the
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
| Sieve, prioritised for decoding | 211,195,701 | 4.73:1 | 21.12% | −8.22% | longer than 790.5 s (see below) | 11.261 s, then checked and written in 5.065 s |
| **Sieve** (`.sieve`), optimised for size | **186,081,966** | **5.37:1** | **18.61%** | **−19.14%** | longer than 734 s (see below) | 1,137.862 s, then checked and written in 5.194 s |
| PPMd var. J (`enwik9.pmd`), as published | 183,964,915 | 5.44:1 | 18.40% | −20.06% | | |

Optimised for size, Sieve is 19.14% under 7z (on enwik8, 16.89%) and 2,117,051 bytes (1.15%) larger
than PPMd at its strongest; it unpacks at about 0.88 MB a second (mixing decodes as slowly as it codes). Prioritised for decoding, it is
13.50% larger than that, still 8.22% under 7z, and decodes at about 89 MB a second: 16.3 s in all,
checked and written.

**The making times are not yet the whole time.** The 734 s and 790.5 s are the weighing's search
(the line under the result, `search: ...; took`, now `the weighing's search took`): the time the
weighing spent finding the file's best way to be named, one pass of several. Reading the file also
packs its instructions (the rope's search and mixing, which for a gigabyte take far longer), and
sets zip and 7z beside them, so the whole was longer; how much was not kept. The graph's width at
the end (3 h 12 min and 1 h 09 min) was set from the first estimate, not measured. The File Locator
now says the whole time to the tenth of a second when it is done, and when its last new best came;
with **Keep a log** ticked, each run also writes every report it made as it went (each best so far
with its time, which the window shows only until the next), the result, and its phases' times to a
text file in the logs folder beside its settings. To be measured again with that.

PPMd's figure is from the same page as enwik8's, not measured here; the benchmark's other published
enwik9 sizes are in the enwik8 section's second table.

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
| Sieve, optimised for size | **52,996,145** | 350.1 s | 202.769 s, then checked and written in 0.918 s |
| Sieve, optimised for decoding | 60,130,138 | 349.5 s | 4.563 s, then checked and written in 0.928 s |

Optimised for size, it is 12.82% smaller than 7-Zip's and takes about three and a half minutes to
unpack; optimised for decoding, 1.09% smaller and about five and a half seconds in all.

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

## A million digits of pi

`pi6.txt`, a million digits of pi, from
[the University of Exeter](https://www.exeter.ac.uk/v8media/facultysites/ese/physics/pi6.txt): a
stress test, since digits of pi are as close to random as text gets.

| | Bytes | On disk |
| :--- | ---: | ---: |
| pi6.txt, the original | 1,111,103 | 1,114,112 |
| pi6.7z | 512,745 | 516,096 |
| pi6.txt.sieve, the latest | **417,052** | 417,792 |

**The floor.** A million digits treated as random hold 1,000,000 × log₂10 ÷ 8 = **415,241** bytes of
information, and no file of them can be smaller on average. The latest `.sieve` is **1,811 bytes**
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
| The rope wave updated | **417,052** | 417,792 | +0.4361% | −0.4342% | −18.66% |

"Above the floor" is how much larger than 415,241 bytes the file is, as a share of the floor; "the
floor below it" is how much smaller the floor is, as a share of the file; "against 7z" is how much
smaller the `.sieve` is than `pi6.7z` (512,745 bytes).

## A book: Don Quixote

`pg996.txt`, Project Gutenberg's *Don Quixote*, as plain text.

| | Bytes | On disk | Ratio | Against 7z |
| :--- | ---: | ---: | ---: | ---: |
| pg996.txt | 2,391,678 | 2,392,064 | | |
| pg996.7z | 686,020 | 688,128 | 3.49:1 | |
| pg996.txt.sieve | 683,205 | 684,032 | 3.50:1 | −0.41% |

## Photographs (JPEG)

A JPEG is compressed already, so neither 7-Zip nor Sieve can take much off. Both `.sieve` files are
given: of the photograph itself, and of its `.7z`.

| | Original | 7z | `.7z.sieve` | `.jpg.sieve` | Against 7z |
| :--- | ---: | ---: | ---: | ---: | ---: |
| A picture at 3653×5480 someone sent a link to | 3,844,213 | 3,843,942 | 3,829,453 | 3,826,046 | −0.47% |
| 20230830_163945.jpg | 4,139,682 | 4,127,434 | 4,112,541 | 4,111,592 | −0.38% |
| 20240407_153939.jpg | 4,587,473 | 4,575,201 | 4,559,089 | 4,558,132 | −0.37% |

On disk:

| | Original | 7z | `.7z.sieve` | `.jpg.sieve` |
| :--- | ---: | ---: | ---: | ---: |
| 1-amorgos.jpg.pagespeed.ce.Mgygi9l3S-.jpg | 3,846,144 | 3,846,144 | 3,829,760 | 3,829,760 |
| 20230830_163945.jpg | 4,141,056 | 4,128,768 | 4,116,480 | 4,112,384 |
| 20240407_153939.jpg | 4,587,520 | 4,575,232 | 4,562,944 | 4,558,848 |

## A small script

`control.py`, a Python script of 1,597 bytes. Here the `.sieve` is 10 bytes larger than the 7z: what
every `.sieve` carries besides the file (its listing, and the file's hash for checking it) costs more
than Sieve saves on a file this small. Without that overhead and the hash it would be smaller.

| | Bytes | Against 7z |
| :--- | ---: | ---: |
| control.py | 1,597 | |
| control.7z | 822 | |
| control.7z.sieve | 959 | +16.67% |
| control.py.sieve | 832 | +1.22% |

All four take one cluster on disk, 4,096 bytes.

## Files that are items of Sieve's own lines

`degrees.sieve` (in this folder) holds 3,960 files: an item of every dimension at every whole degree
from 0 to 359. Each is an item of one of Sieve's lines, so the `.sieve` carries only its place on its
line, and Sieve makes it again on installing. 13,321,756 bytes come to **7,887** (1,689:1), where a
7z of the same folder is 22,961. Installed on Edward's machine: 3,960 files, every one checked
against its SHA-256, decoded in 0.356 s, checked and written in 1.461 s.

This works only for files that are Sieve's own items, made from their places; a file from anywhere
else is carried as itself. [PROOF.md](PROOF.md) has the sample, and `welcome to the sieve.sieve`
(the starting page: 32 bytes as 159), where the overhead is more than the file.
