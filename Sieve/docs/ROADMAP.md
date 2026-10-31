# The plan from here

*Gathered 31 October 2026, for Edward and Claude to look at together; made again the same day as a
plan, from the work in progress to the best ideas. It is drawn from
[NEXT-VERSION.md](NEXT-VERSION.md), [IDEAS.md](IDEAS.md) (§19.87 to §19.108), [FORMATS.md](FORMATS.md),
HANDOFF's plans, SHOWCASE-PLAN, REVIEW, `results/` and Proof/STATISTICS.md. Every figure was
measured, and where it came from is named; "not measured" means no figure exists. Part D holds new
ideas from the same look round, three of them measured that day. Nothing here is decided: the order
is a recommendation.*

## The short list

If only seven things were done next, in this order:

1. **Step 11 green on Windows** (A.1): the case clash fixed, Edward's run under way.
2. **A PNG's rows carried unfiltered under temporary** (D.1). *Measured 31 October:* exact by
   construction. On the two PNGs it applies to, about 22% to 41% smaller at worst (the rows alone
   31% and 47%). It's the largest open gain on
   pictures, and it belongs in v32.
3. **Finish and freeze v32** (part B): two decisions, Edward's WoWRevived run, and three small
   format items.
4. **Mixing tried on a hallway screenshot's pixels** (D.2). No version is needed: one screenshot's
   `.sieve` would be 11.8% smaller.
5. **Names equal but for case, refused before installing** (D.3). On Windows and macOS, a `.sieve`
   made on Linux could otherwise lose a file without a word.
6. **The Silesia benchmark** (C.4): the one public corpus not yet measured. Nothing is posted
   without it.
7. **Word frequencies on enwik8 and enwik9** (C.1): built; only the large measurement is left.

## A. In progress now

| | What | Where it stands | Next |
| :--- | :--- | :--- | :--- |
| A.1 | **CI step 11 on Windows** | It failed at line 820: `Vitruvian.jpg` and the test site's `vitruvian.jpg` are one name where case is ignored. Fixed (the anchor copied as `vitruvian-man.jpg`); the JPEG block passed here alone | Edward's run. The checks after the JPEG block have run only on Linux so far |
| A.2 | **v32's draft ways** | `delta`, `jbr`, `pngr`, JPEG tables under permanent, and `F80`, all behind `--v32-draft`, all read by the oracle | Part B |
| A.3 | **WoWRevived with the draft** | The maps are 98.7% alike (`sieve alike`); the first run was without the draft | Edward: one run with `--v32-draft` and Keep a log |
| A.4 | **The File Locator beside 7z's BCJ2** | 7z makes 3,289,322 bytes for tools/sieve.exe and the docs; Sieve is 2.74% under a 7z with BCJ | Edward: the File Locator's figure for the same folder |

## B. Finishing v32

v32 is built behind `--v32-draft`. These are its ways so far:
- **`delta`:** a 160 MB pair went from 22,537,063 bytes to 17,947,687, 20.36% smaller.
- **`jbr`:** JPEGs 15.25% to 21.91% smaller, given back byte for byte.
- **`pngr`, and JPEG tables under permanent:** every PNG measured was installed smaller, with the
  same pictures; Vitruvian.jpg 207,500 → 203,455 bytes.
- **`F80`:** word frequencies for mixing, 1.8% to 8.8% smaller on five English texts.

What is left, in order:

| | What | Measured | Cost |
| :--- | :--- | :--- | :--- |
| B.1 | **PNG rows unfiltered under temporary** (D.1) | About 22% to 41% at worst, on PNGs other programs wrote | A new way; the packer, the reader, the oracle, CI |
| B.2 | **The group question**: a v30 group with another file in its middle, which the reader takes and the specification and oracle refuse | Making the packer cut such groups cost 14% | Recommended: change the specification and oracle to allow it (a); no bytes change |
| B.3 | **A small file's overhead** | About 11 bytes a small `.sieve`: two stream sizes, a dictionary byte, LZMA2's framing. `control.py` 731 to about 720 | Small; the listing changes anyway |
| B.4 | **The archive's format as a number** (v31's `skm` text and the recipes' settings) | A few bytes a temporary archive; not measured | Small; take it while B.3 is open |
| B.5 | **A second flags byte** | The first is full (all eight bits used) | A decision, before any new optional part |
| B.6 | **Freeze**: SIEVE-FORMAT's v32 edition, the Proof samples made again, the same codes on Windows, Linux and macOS | | NEXT-VERSION §5, steps 4 and 5 |

The rope's wave direction was measured for v32 and is **not** to go in (part E).

## C. The largest gains still open (from the documents)

**C.1 Word frequencies for mixing** (IDEAS §19.102; *built 31 October 2026, behind the draft*).
SCOWL has no frequencies, but its sizes rank its words by how common they are, and the three lists
Sieve ships nest. So no new file was needed: each word of scowl-en-80 counts 4,096 if scowl-en-35
holds it, 64 if only scowl-en-60 does, else 1 (mix-v4's `F80`).
- Against today's scowl-en-60: Don Quixote's first 800 KB 193,808 bytes to 188,719 (−2.6%); 200 KB
  of HANDOFF −1.8%; three small texts −5.6% to −8.8% (NEXT-VERSION §4a, 5).
- Still to measure: enwik8 and enwik9, where the gap to PPMd var. J is 1.34% (21,675,870 against
  21,388,296) and 1.15%.
- A finer ranking (SCOWL's sizes 10 to 95, or real counts) would need new pinned files.

**C.2 A progressive JPEG under permanent.** The table optimiser leaves a progressive JPEG exact,
since its scans code values otherwise. Two ways are open: optimise its tables scan by scan, or under
permanent install a baseline JPEG as progressive, which is often smaller. Neither is measured here.

**C.3 BCJ2 for programs** (IDEAS §19.95). Waits on A.4: if 7z still leads there, BCJ2's streams are
the reason to look at; if not, BCJ2 can be dropped.

**C.4 Silesia** (SHOWCASE-PLAN §2): 211 MB of mixed files, run by `tools/bench/compare.py` against
7-Zip, xz, zstd and brotli, and paq8px for honesty.

**C.5 Mixing as a kind of piece** (IDEAS §19.103): a picture and text in one file (a tar), each
mixed as what it is. Not measured.

## D. New ideas from this look round (31 October 2026)

**D.1 A PNG's rows carried unfiltered under temporary** (IDEAS §19.88, made concrete). Today a PNG
under temporary is carried as its inflated rows, *as PNG filtered them*, with preflate's skeleton.
PNG's five filters can each be undone and done again exactly, row by row, given the row's filter
byte (they are sums and differences modulo 256). So the rows can be carried unfiltered, the filter
bytes beside them, and installing filters them again and deflates them by the same skeleton. **The
original comes back byte for byte by construction:** preflate's corrections do not change, since it
sees the same filtered stream.

*Measured* by `tools/png_unfiltered_probe.py` (tables in `results/png-unfiltered-2026-10-31.md`).
Each PNG was written again with every row unfiltered (filter 0, zlib 9), both were carried under
temporary, and the filter bytes were carried alone:

| PNG | written by | temporary today | rows unfiltered | the filter bytes alone (xz) | original's skeleton / zlib copy's |
| :--- | :--- | ---: | ---: | ---: | ---: |
| icon128 | another program | 19,775 | 10,468 (−47.1%) | 100 | 1,657 / 534 bytes |
| m1_sieve_scowl60 | another program | 84,640 | 58,438 (−31.0%) | 252 | 7,537 / 213 bytes |
| example_book_cover | another program | 150 | 215 | 80 | |
| hallway-door | Sieve (stb) | 7,882 | 18,296 | 120 | |
| hallway-compact | Sieve (stb) | 16,058 | 14,161 | 156 | |

- The "rows unfiltered" figure carries the zlib copy's skeleton, not the original's. So the way
  would cost the filter bytes and the skeletons' difference more. For icon128 that is 1,123 bytes
  of skeleton and 100 of filter bytes, about 11,700 in all even if the skeleton packed not at all:
  still about 40% under today. For the scowl: 7,324 and 252, about 66,000 at most, 22% under
  84,640. Building it will give the exact figure.
- **It's a choice per PNG, weighed, never always:** the book cover (160 by 160) comes out larger.
- Sieve's own screenshots (stb) are carried as their pixels already, so they gain nothing from this.
  Their two rows show how those pixels are packed instead (D.2): hallway-compact's smaller by
  mixing, hallway-door's larger (large flat areas, which LZMA2 takes better).
- Under permanent, `pngr` is smaller still (48,135 for the scowl), since it needs no skeleton.
  D.1 is for temporary, where the file must come back exactly.

**D.2 Mixing tried on a hallway screenshot's pixels** (no version: v28's mixing of pictures).
- hallway-compact under temporary: its pixels (2,764,800 bytes, from `stb`) carried raw by LZMA2,
  15,957 bytes. The same pixels with a zero byte a row, as rows of 3,841: mixed as a picture
  (`mix4 B22 I3841x1`), 14,059 bytes, 11.9% smaller.
- So the packer didn't try mixing on the `stb` way's pixels, where it did on the skeleton way's
  rows. Why it didn't is still to find (the log says why for each file).
- The fix is to weigh mixing as a picture of width w·c on an `stb` PNG's pixels, as on any picture.
  Every `.sieve` this changes stays v28 to v31, read by every reader since.
- hallway-door shows the weighing must stay: there LZMA2 (7,882) beat mixing (18,296).

**D.3 Names equal but for case** (found by step 11's failure).
- A `.sieve` made on Linux can hold `A.txt` and `a.txt`.
- Installing checks before writing that no file is already there (locate.cpp's first phase). Both
  pass, since neither exists yet, so on Windows or macOS the second is written over the first. One
  file is lost without a word.
- This was read from the code, not tried on Windows.
- **The fix**, no version:
  - the installer refuses or asks before writing anything, naming the pair;
  - the packer warns when it reads such a folder;
  - a CI check that no tracked path and no step's copy has such a pair.
- Today's scan found none in the repository. The step 11 copy was the only clash.

**D.4 Undo any filter that can be undone, before modelling.** D.1 is one case of a rule: where a
format filters reversibly (PNG's row filters, TIFF's horizontal predictor, a WAV's delta coding), the
models do better on the values before filtering, as pngr already showed under permanent. Only PNG is
measured; the others are for the per-format corpus (C.7).

**D.5 CI step 11's own time.** It took 2,479 s here and 1,781 s on Edward's machine (to its failure)
of the full CI. The p32 block filters every PNG row in Python. Each block's time from the `.trace`
would show where the time goes. A slow block could then use a smaller fixture, leaving the oracle
as plain as it is. Not measured.

**D.6 Word lists beyond English.** F80 is English-only, since SCOWL is. A text in another language
gets nothing from it. Another language's ranked list, pinned as SCOWL is, could be a fourth list
(the listing names the list, so no version beyond v32). Not measured; it needs a source with a
licence as open as SCOWL's.

**C.7, the per-format corpus** (FORMATS.md, Edward, 30 October): every method run on every format,
with results kept as tables in `results/` and defaults read from them. A **"best defaults"** box
would apply them, and the same tables would catch regressions. The first pieces exist:
`tools/rope_designs.py` and `tools/rope_direction_probe.py`. D.1, D.2 and D.4 are the kind of
finding it would make routine.

## E. Speed, the same bytes out (no version)

- **The weighing does the packer's work again:** *already built* (`file_ways`'s cache). On 1 MB of
  Don Quixote, locating with the weighing took 3.19 s and without it 3.33 s.
- **Mixing's speed:**
  - enwik9 optimised for size installs in 1,153.583 s.
  - Done so far: the SSE2 sums took 13.9% of the instructions off, and each counter moved with its
    sign known (31 October) took 9.3% more. Don Quixote's coding went from 6.33 s to 5.60 s, and its
    decoding from 6.40 s to 5.56 s.
  - What remains looks structural: fewer contexts, or AVX2 where the machine has it, with the same
    numbers.
- **The rope's decoding table:** *already built*.
- **Threads for the filters' search** (IDEAS §19.99), and packing faster before a right-click
  "Create .sieve" (§19.100).

## F. Evidence: what shows Sieve to others

- **Silesia** (C.4) first.
- **The per-format corpus** (C.7).
- **The ladder of cuts** (`tools/bench/ladder.py`): where Sieve starts to beat 7z for each kind of
  file, from 1 KB up, on Edward's machine with the real 7-Zip.

## G. Measured and set aside (so they are not measured again)

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
| Packing the mixer's inputs once a bit | Cost more instructions than it saved | E |
| Rows unfiltered on every PNG | Larger on small PNGs and on flat pictures: hence weighed (D.1) | D.1 |

The repropagation scheme (Edward's waves, seeds and passes, §19.108) stays open as an idea. What
would revive it is a dictionary of waves that real files are made of; nothing measured so far is
one.

## H. Further off, by Edward's order

- **Other compressed formats** (IDEAS §19.88): MP3 and MP4 recoded as JPEG now is. Edward's rule is
  that such layering comes "among the last things", once the basic tool is proven.
- **A sound model** for raw audio (HANDOFF's 27 October plan, step 3).
- **Decoding one file without the rest** (IDEAS §19.104): first a cache filled at start-up, on the
  *Alien Trilogy* engine as the test bed, which needs no version.
- **Sieve for websites** (IDEAS §19.92): the extension's remaining pieces.
- **The game and the hallway** (GAME.md), and the rest of IDEAS §19 beyond compression.

## Waiting on Edward

- The group question (B.2): (a) or (b).
- WoWRevived with the draft and Keep a log (A.3).
- The File Locator's figure beside 7z's 3,289,322 (A.4).
- enwik8 and enwik9 with F80 (C.1).
- Whether to write FILTERS.md.
- Which of D.1 to D.3 to build first; the plan's order is D.1 (inside v32), then D.2 and D.3
  (neither needs a version).
