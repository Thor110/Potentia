# The best ideas from here

*Gathered 31 October 2026, for Edward and Claude to look at together. It is drawn from
[NEXT-VERSION.md](NEXT-VERSION.md), [IDEAS.md](IDEAS.md) (§19.87 to §19.108), [FORMATS.md](FORMATS.md),
HANDOFF's plans, SHOWCASE-PLAN and the measurements in `results/` and Proof/STATISTICS.md. Every
figure was measured, and the section it came from is named; "not measured" means no figure exists.
Nothing here is decided: the order is a recommendation.*

## The short list

If only five things were done next, in this order:

1. **Finish and freeze v32** (§1). Most of it is built. What's left is small, plus two decisions and
   Edward's WoWRevived run.
2. **Word frequencies for mixing** (§2.1). *Built 31 October behind the draft (`F80`): 1.8% to 8.8%
   smaller on five English texts.* What's left is to measure it on enwik8 and enwik9.
3. **The Silesia benchmark** (§4.1). It's the one public corpus not yet measured, and nothing is
   posted without it.
4. **Making large files faster, the same bytes out** (§3). No version is needed. Two of the four
   items were already built (see §3). What's left is mixing's own speed: enwik9 installs in 1,153 s.
5. **A PNG's rows carried unfiltered under temporary too** (§2.2). This idea came out of this week's
   work and looks large on some pictures; it's to be measured before v32 freezes.

## 1. Finishing v32

v32 is built behind `--v32-draft`. These are its ways so far:
- **`delta`:** a 160 MB pair went from 22,537,063 bytes to 17,947,687, 20.36% smaller.
- **`jbr`:** JPEGs 15.25% to 21.91% smaller, given back byte for byte.
- **`pngr`, and JPEG tables under permanent:** every PNG measured was installed smaller, with the
  same pixels.

What is left, in order:

| | What | Measured | Cost |
| :--- | :--- | :--- | :--- |
| 1.1 | **Edward's WoWRevived run** with the draft and Keep a log | The maps are 98.7% alike (`sieve alike`). The +0.20% against 7z is in data already compressed, file by file not yet known | One run on his machine |
| 1.2 | **The group question**: a v30 group with another file in its middle, which the reader takes and the specification and oracle refuse | Making the packer cut such groups cost 14% | Recommended: change the specification and oracle to allow it (a); no bytes change |
| 1.3 | **A small file's overhead** | About 11 bytes a small `.sieve`: two stream sizes, a dictionary byte, LZMA2's framing. `control.py` 731 to about 720 | Small; the listing changes anyway |
| 1.4 | **The archive's format as a number** (v31's `skm` text and the recipes' settings) | A few bytes a temporary archive; not measured | Small; take it while 1.3 is open |
| 1.5 | **A second flags byte** | The first is full (all eight bits used) | A decision, before any new optional part |
| 1.6 | **Freeze**: SIEVE-FORMAT's v32 edition, the Proof samples made again, the same codes on Windows, Linux and macOS | | NEXT-VERSION §5, steps 4 and 5 |

The rope's wave direction was measured for v32 and is **not** to go in (§5).

## 2. The largest gains still open

**2.1 Word frequencies for mixing** (IDEAS §19.102; *built 31 October 2026, behind the draft*).
SCOWL has no frequencies, but its sizes rank its words by how common they are, and the three lists
Sieve ships nest. So no new file was needed: each word of scowl-en-80 counts 4,096 if scowl-en-35
holds it, 64 if only scowl-en-60 does, else 1 (mix-v4's `F80`).
- Against today's scowl-en-60: Don Quixote's first 800 KB 193,808 bytes to 188,719 (−2.6%); 200 KB
  of HANDOFF −1.8%; three small texts −5.6% to −8.8% (NEXT-VERSION §4a, 5).
- The trie was made compact on the way, for every build, at the same codes: scowl-en-60's peak
  memory 91 MB to 59 MB.
- Still to measure: enwik8 and enwik9, where the gap to PPMd var. J is 1.34% (21,675,870 against
  21,388,296) and 1.15%.
- A finer ranking (SCOWL's sizes 10 to 95, or real counts) would need new pinned files. The three
  sizes already level off from 1,024 to 1 for the commonest words.

**2.2 A PNG's rows carried unfiltered, made again exactly** (IDEAS §19.88, found 31 October).
Re-saving PNGs showed that rows carried unfiltered pack far better than rows as PNG filtered them:

| PNG | `.sieve` temporary (filtered rows) | `.sieve` permanent (unfiltered rows) |
| :--- | ---: | ---: |
| m1_sieve_scowl60 | 84,641 | 48,136 |
| icon128 | 19,776 | 11,376 |
| gallery-tiles | 26,385 | 17,950 |
| beautiful-screens | 7,218 | 5,517 |

Temporary could carry the rows unfiltered too, with each row's filter byte. Installing would filter
them again and deflate them by preflate as now, giving back the original exactly. What to measure:
whether preflate's corrections stay small for rows filtered again. If they do, this is a large gain
on screenshots and drawings with no change to what is installed.

**2.3 A progressive JPEG under permanent.** The table optimiser leaves a progressive JPEG exact,
since its scans code values otherwise. Two ways are open: optimise its tables scan by scan, or under
permanent install a baseline JPEG as progressive, which is often smaller. Neither is measured here.
brunsli already carries progressive JPEGs, so this only changes the installed file.

**2.4 BCJ2 for programs** (IDEAS §19.95). Sieve is now 2.74% under a 7z with BCJ on the program and
docs (4,159,742 against 4,277,050 bytes). Edward's 7z at its default, BCJ2, makes 3,289,322 for
tools/sieve.exe and the docs. The File Locator's figure for the same folder is still to be set beside
it. If 7z still leads there, BCJ2's streams are the reason to look at. If not, BCJ2 can be dropped.

**2.5 Mixing as a kind of piece** (IDEAS §19.103). The packer-only step gained nothing, because
mixing the whole file beat every cut. What is left is a picture and text in one file (a tar), each
mixed as what it is. Not measured.

## 3. Speed, the same bytes out (no version)

From HANDOFF's plan of 29 October:
- **The weighing does the packer's work again:** *already built* (`file_ways`'s cache, by the file's
  SHA-256 and settings, at most 2 GiB). Checked 31 October: on 1 MB of Don Quixote, locating with
  the weighing took 3.19 s and without it 3.33 s, so the weighing repeats nothing.
- **Mixing's speed.** enwik9 optimised for size installs in 1,153.583 s; prioritised for decoding,
  15.9 s, at a file 21.12% larger. The SSE2 sums took 13.9% of the instructions off.
  - *31 October:* each counter is now moved with its sign known from the bit, which takes 9.3% of
    the instructions off at the same codes. All of Don Quixote (2.3 MB, with the word list, three
    runs each): coding 6.33 s to 5.60 s, decoding 6.40 s to 5.56 s.
  - What remains is mostly the mixers' SSE2 sums and the tables' reads. Packing the inputs once a
    bit was tried and cost more than it saved.
  - Further gains look structural (fewer contexts, or AVX2 where the machine has it, with the same
    numbers), not more tuning.
- **The rope's decoding table:** *already built*. The table is made only for a stream of 16,384
  values or more, and shorter ones search the symbols' starts by halving (`weights_rans.cpp`).
- **Threads for the filters' search** (IDEAS §19.99), and making packing faster before a right-click
  "Create .sieve" (§19.100).

## 4. Evidence: what shows Sieve to others

**4.1 Silesia** (SHOWCASE-PLAN §2): 211 MB of mixed files, the one public corpus not yet measured.
It is run by `tools/bench/compare.py` against 7-Zip, xz, zstd and brotli, and paq8px for honesty.
SHOWCASE-PLAN says nothing is posted before it.

**4.2 The benchmark and optimisation engine** (FORMATS.md, the long run; Edward, 30 October). This
means every method run on every format, with results kept as tables in `results/` and defaults read
from them. A **"best defaults"** box would apply them, and the same tables would catch regressions.
Its first pieces exist: `tools/rope_designs.py` (designed files made from seeds) and
`tools/rope_direction_probe.py`. The next step is a corpus per format (public files, each with its
source) and one run that packs each under every method.

**4.3 The ladder of cuts** (`tools/bench/ladder.py`): where Sieve starts to beat 7z for each kind of
file, from 1 KB up, on Edward's machine with the real 7-Zip.

## 5. Measured and set aside (so they are not measured again)

| Idea | What was found | Where |
| :--- | :--- | :--- |
| A wave direction for the rope | It cancels designed files (the diagonal file 926,317 → 8,964), but on real files no remainder came under xz on the file itself | IDEAS §19.108, `results/rope-direction-2026-10-30.md` |
| More waves over what the first leaves | Worse: Vitruvian 5.219 bits a pixel after one wave, 5.468 after two, 6.274 after three | IDEAS §19.108 |
| A file's opposite found in noise, or seeds | A short seed names too few noises to save anything on almost every file | IDEAS §19.91 |
| Headers modelled | The headers are small, and LZMA2 already packs their repeats | IDEAS §19.88 |
| The rope's next contexts | At most 0.3%, on one file | IDEAS §19.87 |
| A shared web dictionary | 0.5% under brotli on 102 unseen pages, 6.6% over on Edward's 7 | IDEAS §19.92 |
| Pieces weighed with mixing | Nothing gained: mixing the whole file beat every cut | IDEAS §19.103 |
| Parts of speech, LOCO-I bias, larger tables | 0.15% to 0.55%, 0.4%, under 1% at twice the memory | NEXT-VERSION §3b |

The repropagation scheme (Edward's waves, seeds and passes, §19.108) stays open as an idea. What
would revive it is a dictionary of waves that real files are made of; nothing measured so far is
one.

## 6. Further off, by Edward's order

- **Other compressed formats** (IDEAS §19.88): MP3 and MP4 recoded as JPEG now is. Edward's rule is
  that such layering comes "among the last things", once the basic tool is proven. PNG and JPEG are
  now done under all three methods.
- **A sound model** for raw audio (HANDOFF's 27 October plan, step 3).
- **Decoding one file without the rest** (IDEAS §19.104). First a cache filled at start-up, on the
  *Alien Trilogy* engine as the test bed, which needs no version. Reading a group at a time needs
  one.
- **Sieve for websites** (IDEAS §19.92): the extension's remaining pieces.
- **The game and the hallway** (GAME.md), and the rest of IDEAS §19 beyond compression.
