# The plan from here

## The long plan (11 November 2026), and where it stands (12 November 2026)

Edward: "lets lay out a long plan for what we can implement next for hallway and sieve etc utilising
the ideas to get as much done as we can", then "get as much of all of that list done without me as
you can". Seven stages, each leaving something finished; the older plan of 31 October follows below.
**What it is all for:** a format that can name and give back any file exactly, smaller than anything
else where it can be, and compatible forever (IDEAS §19.137); a hallway that walks every line of it
and is on its way to being a game and a game engine; an AI line whose every model replays bit for
bit; and, underneath, records that cannot be quietly changed (§19.136). Every step is measured first,
and built only where it pays.

| Stage | Items | Where it stands |
| --- | --- | --- |
| 1. Loose ends | entry order, recipe digits, rope tie, CI pins and warnings, the repeat loop, harness sort | **Done** |
| 2. Finish and freeze v34 | x86 filter per program; BMP RLE, AVI/MOV PCM, WAD/PAK; MPEG-2 MP3s; the freeze | x86 restart **built**; the three formats measured too small; MPEG-2 MP3s open; **the freeze is Edward's call** |
| 3. Sieve's voice | read aloud everywhere; words CMUdict lacks; more recordings; prosody; a trained voice | first two **built**; three hours measured (19.8% against 23.3%), not taken; prosody and the trained voice open |
| 4. The AI line | training speed; QK-norm; corpora; wider arithmetic; a real model | speed **built** (about twice on four threads); QK-norm measured, not built; documents as text **built**; wider arithmetic and a real model left for Edward |
| 5. The hallway | chat commands, logs, chains; PCM filters; filter ideas; music filter designer; a game dimension | chains steered **built**; sound plugins and `pcm-quiet-ends` **built**; designer plays what it keeps, Media Player's M **built**; game dimension **designed** (GAME.md §11); chain branches open |
| 6. Compression research | the rope; shortcut seed, cmix ideas; video, game streaming | rope: all measured, ground now mixing's and FLAC's, nothing to build; seed: its winners built in v33; cmix's SSE retired before, x86 model built; streaming measured (no version needed); packets measured, **whole-set plain weighing built**; mixing models for meshes, code points, grammar and MIDI open |
| 7. Infrastructure and outreach | docs kept up from the code; the website; the rope post | open; the rope post waits for the v34 freeze |

**What is waiting on Edward:** the v34 freeze; wider arithmetic (a training version); the three hours
of voice recordings; and the designs to decide on (the games line, GAME.md §11; distributed
computing, IDEAS §19.139). **What can go on without him:** Stage 7's documentation pass, chain
branches, prosody in the voice, and the open mixing models, each measured first.

### Next: the ideas filtered (Edward, 12 November 2026)

"continue to evaluate ideas as you go forward and give me updates as you go, sort workable and
unworkable ideas, as well as technical notes and metaphorical jargon between notes and ideas and
voices etc accordingly as well as the rest of the documentation". In order:
1. **Every idea triaged** (docs/IDEAS-TRIAGE.md): each entry of IDEAS.md given its kind (technical,
   metaphorical, both, a way of working), its status, and a verdict (done, workable, workable but
   large, waiting on Edward, unworkable, not wanted, metaphor only) with the reason from its own text.
2. **Sorted by kind:** technical ideas stay in IDEAS with their notes; Edward's metaphorical lines go
   to VOICES.md's candidates (the file is cleaned only when he asks); notes about how to work go to
   HANDOFF.
3. **The workable ones planned:** the open, workable ideas ranked by reach and cost into the stages
   above; the unworkable ones marked so, with why, so they are not measured again.
4. **The rest of the documentation:** MANUAL, SPECIFICATIONS, FILTER-PLUGINS and the others checked
   against the code (Stage 7's pass, IDEAS §19.134).
- **Step 1 done (12 November 2026):** [IDEAS-TRIAGE.md](IDEAS-TRIAGE.md), 569 ideas: 183 done, 204
  workable, 72 workable but large, 14 waiting on Edward, 49 unworkable, 31 not wanted, 16 metaphor
  only. Twenty-three of Edward's lines added to VOICES' candidates.
- **Where ideas go from here (Edward, 12 November 2026):** "the ideas will slowly get filtered onto
  the task list … and get deprecated accordingly or utilised … keeping record of the old ideas
  somewhere". Workable ideas move onto this file a few at a time. Worked-through ones move whole to
  `docs/ideas/`: `IMPLEMENTED.md` (done, each checked against the code), `DEPRECATED.md`
  (unworkable and not wanted, with the reason) and `WORK-DONE.md` (a dated log, a chart across
  time). IDEAS.md keeps a one-line pointer at each old number, so no link breaks, and holds only
  what is open.
- **Step 2 begun (12 November 2026):** 39 done entries moved whole to
  [ideas/IMPLEMENTED.md](ideas/IMPLEMENTED.md), pointers left in IDEAS.md. Next: §17 and the
  unworkable ones to `ideas/DEPRECATED.md`, then the dated `ideas/WORK-DONE.md`.
- **A rule over all of it, the tactile Sieve (IDEAS §19.140):** Sieve is meant to be felt as well as
  seen: colours and notes carried by braille, touch and sensation screens, for the disabled
  throughout time; that is why the interface is kept simple. Every new screen keeps few elements,
  each with a name, a value and a place, and no meaning held by colour or position alone. First
  steps: the colour-blindness pass (2.4), read-aloud for the hallway's panels, and a "describe this
  unit" key that speaks or brailles the unit in hand exactly.

---


*Gathered 31 October 2026, for Edward and Claude to look at together; made again the same day as a
plan, from the work in progress to the best ideas. It is drawn from
[NEXT-VERSION.md](NEXT-VERSION.md), [IDEAS.md](IDEAS.md) (§19.87 to §19.108), [FORMATS.md](FORMATS.md),
HANDOFF's plans, SHOWCASE-PLAN, REVIEW, `results/` and Proof/STATISTICS.md. Every figure was
measured, and where it came from is named; "not measured" means no figure exists. Part D holds new
ideas from the same look round, three of them measured that day. Nothing here is decided: the order
is a recommendation.*

*Worked through the same day (Edward: "Work through all of that"):*
- **Built:** D.1 (`skr`), D.2 (pixels mixed as their own shape) and D.3 (names one where case is
  ignored). D.5's tool is built.
- **Measured:** C.2.
- **Left for Edward's decision:** B.3 and B.4, since they take bytes out of v32's head, which is
  B.5's question. *Decided the same day (Edward): (a) for B.2, the version byte for B.5; B.2 to B.5
  then built.*
- Each item below says where it stands.

## The short list

If only seven things were done next, in this order:

1. **Step 11 green on Windows** (A.1): the case clash fixed, Edward's run under way.
2. **A PNG's rows carried unfiltered under temporary** (D.1). *Built 31 October, behind the draft:*
   the five PNGs it applies to are 23% to 40% smaller, each back byte for byte.
3. **Finish and freeze v32** (part B): two decisions, Edward's WoWRevived run, and three small
   format items.
4. **Mixing tried on a hallway screenshot's pixels** (D.2). *Built 31 October, behind the draft*
   (packer rule: no change without it): the 18 hallway screenshots 245,100 → 211,937 bytes.
5. **Names equal but for case, refused before installing** (D.3). *Built 31 October.* Edward's
   Windows run of step 11 checks the refusal.
6. **The Silesia benchmark** (C.4). *Measured 1 November (Edward, `tools/bench_files.bat`):*
   42,091,120 bytes for the twelve files, 13.13% under 7z's LZMA2 9e; at v33 (3 November),
   41,710,177, 13.92% under. Still to set beside it: 7-Zip
   itself, xz, zstd, brotli and paq8px (`tools/bench/compare.py`).
7. **Word frequencies on enwik8 and enwik9** (C.1): built; only the large measurement is left.

## A. In progress now

| | What | Where it stands | Next |
| :--- | :--- | :--- | :--- |
| A.1 | **CI step 11 on Windows** | It failed at line 820: `Vitruvian.jpg` and the test site's `vitruvian.jpg` are one name where case is ignored. Fixed (the anchor copied as `vitruvian-man.jpg`); the JPEG block passed here alone | Edward's run. The checks after the JPEG block have run only on Linux so far |
| A.2 | **v32's draft ways** | `delta`, `jbr`, `pngr`, JPEG tables under permanent, and `F80`, all behind `--v32-draft`, all read by the oracle | Part B |
| A.3 | **WoWRevived with the draft** | *Run 31 October (size, Try every way):* 1,001,994,144 bytes, 6.68% under Edward's 7z (was +0.20%). MAPS-Larger as its differences; the WAVs 71.8% (433 MB), the MP4s 99.9% (406 MB) Decoding speed: 1,016,369,457 bytes (5.34% under 7z), installed in 30.1 s against 210.3 s | Done. The WAVs are the largest gain left (a sound model, H) |
| A.4 | **The File Locator beside 7z's BCJ2** | 7z makes 3,289,322 bytes for tools/sieve.exe and the docs; Sieve is 2.74% under a 7z with BCJ | Edward: the File Locator's figure for the same folder |

## B. Finishing v32

v32 was built behind `--v32-draft`, and frozen 31 October (B.6). These are its ways:
- **`delta`:** a 160 MB pair went from 22,537,063 bytes to 17,947,687, 20.36% smaller.
- **`jbr`:** JPEGs 15.25% to 21.91% smaller, given back byte for byte.
- **`pngr`, and JPEG tables under permanent:** every PNG measured was installed smaller, with the
  same pictures; Vitruvian.jpg 207,500 → 203,455 bytes.
- **`F80`:** word frequencies for mixing, 1.8% to 8.8% smaller on five English texts.

What is left, in order:

| | What | Measured | Cost |
| :--- | :--- | :--- | :--- |
| B.1 | **PNG rows unfiltered under temporary** (D.1) | *Built:* 23% to 40% on the five PNGs other programs wrote | Done, behind the draft |
| B.2 | **The group question**: a v30 group with another file in its middle, which the reader takes and the specification and oracle refused | Making the packer cut such groups cost 14% | *Decided (a), done 31 October:* the specification and the oracle allow it; no bytes change |
| B.3 | **A small file's overhead** | *Built 31 October, behind the draft:* 9 bytes a small `.sieve` (1,153 → 1,144) | Done: v32's head (NEXT-VERSION §4a, 7) |
| B.4 | **The archive's format as a number** (the recipes' settings) | *Built:* 18 to 26 bytes on four remade files | Done, with B.3 |
| B.5 | **A second flags byte** | The first is full (all eight bits used) | *Decided (Edward):* none; the version byte says how a head is read |
| B.6 | **Freeze**: SIEVE-FORMAT's v32 edition, the Proof samples made again, the same codes on Windows, Linux and macOS | *Frozen 31 October (Edward):* v32 the newest, the draft switch gone, SIEVE-FORMAT's v32 edition written | Edward: the CI on all three systems; the Proof samples and STATISTICS measured again |

The rope's wave direction was measured for v32 and is **not** to go in (part E).

## C. The largest gains still open (from the documents)

**C.1 Word frequencies for mixing** (IDEAS §19.102; *built 31 October 2026, behind the draft*).
SCOWL has no frequencies, but its sizes rank its words by how common they are, and the three lists
Sieve ships nest. So no new file was needed: each word of scowl-en-80 counts 4,096 if scowl-en-35
holds it, 64 if only scowl-en-60 does, else 1 (mix-v4's `F80`).
- Against today's scowl-en-60: Don Quixote's first 800 KB 193,808 bytes to 188,719 (−2.6%); 200 KB
  of HANDOFF −1.8%; three small texts −5.6% to −8.8% (NEXT-VERSION §4a, 5).
- *enwik8 at v32 (Edward, 31 October):* 21,675,870 → 21,547,893 bytes (−0.59%). The gap to PPMd
  var. J (21,388,296) went from 1.34% to 0.75%. Installed in 113.1 s.
- *enwik9 at v32 (1 November, `tools/bench_files.bat`):* 186,081,966 → 185,105,231 (−0.52%). The gap
  to PPMd went from 1.15% to 0.62%.
- A finer ranking (SCOWL's sizes 10 to 95, or real counts) would need new pinned files.

**C.2 A progressive JPEG under permanent** (*measured 31 October*, FORMATS.md). Two ways were open:
- **Optimise a progressive JPEG's tables:** nothing to gain from libjpeg's. It makes a progressive
  JPEG's tables for its values every time; Pillow's were the same size with `optimize` on and off,
  on four pictures at three qualities.
- **Install a baseline JPEG as progressive:** about 6% smaller on the three large pictures at
  quality 85 (the hallway door 156,746 → 147,587), and 0.2% larger on icon128. So it would be
  weighed per JPEG. It needs a progressive JPEG writer of Sieve's own; the brunsli form already
  carries either. Not built: the next large JPEG step, if wanted.

**C.3 BCJ2 for programs** (IDEAS §19.95). Waits on A.4: if 7z still leads there, BCJ2's streams are
the reason to look at; if not, BCJ2 can be dropped.

**C.4 Silesia** (SHOWCASE-PLAN §2): 211 MB of mixed files, run by `tools/bench/compare.py` against
7-Zip, xz, zstd and brotli, and paq8px for honesty. *Sieve's side measured 1 November 2026*
(Proof/STATISTICS.md): 42,091,120 bytes for size, 47,262,336 for decoding speed. `mozilla`, a tar of
programs, was then the one file where Sieve did no better than LZMA2. A tar read as its members, its
programs through the x86 filter, was the lead there. *At v33 (3 November 2026):* 41,710,177 bytes
for size, 47,262,336 for decoding; `mozilla` 13,318,582, 0.43% under 7z, mixed with the match
model's ways.

**C.4a A tar carried as its members** (found on Silesia's `mozilla`, 1 November 2026). A tar is no
compressed format, so neither Compressed formats box touches it (the packer reads one only to
extract it), and its programs get no x86 filter: `mozilla` came to LZMA2's own bytes. v31's `skm`
already carries an archive as a skeleton plus its members as files of their own. A tar's skeleton
would be its headers and padding, each member's data a file, so each program would go through the
x86 filter. That's the packer only, not a new version. *Built 1 November 2026:* a test tar of
two programs and three texts (9,584,640 bytes) went from 3,287,224 to 3,069,619 under temporary
(−6.6%), given back byte for byte, and 3,069,430 under permanent. The boxes are now "Archive and
compressed formats". Silesia's `mozilla` was run again at v33 (C.4): 13,318,582 bytes, 0.43% under 7z.

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
| example_book_cover | Sieve (stb) | 150 | 215 | 80 | |
| hallway-door | Sieve (stb) | 7,882 | 18,296 | 120 | |
| hallway-compact | Sieve (stb) | 16,058 | 14,161 | 156 | |

- The "rows unfiltered" figure carries the zlib copy's skeleton, not the original's. So the way
  would cost the filter bytes and the skeletons' difference more. For icon128 that is 1,123 bytes
  of skeleton and 100 of filter bytes, about 11,700 in all even if the skeleton packed not at all:
  still about 40% under today. For the scowl: 7,324 and 252, about 66,000 at most, 22% under
  84,640. Building it will give the exact figure.
- *Built 31 October* (`skr`): every PNG it applies to came out smaller, so it is taken wherever it
  applies. icon16 1,321 → 1,014, icon32 2,757 → 1,911, icon48 4,446 → 3,215, icon128 19,775 →
  11,863, the scowl 84,640 → 59,895, each back byte for byte.
- The book cover was written by stb, so it's carried as its pixels and `skr` doesn't apply. Its row
  above was the probe's, not the way's.
- Sieve's own screenshots (stb) are carried as their pixels already, so they gain nothing from this.
  Their two rows show how those pixels are packed instead (D.2): hallway-compact's smaller by
  mixing, hallway-door's larger (large flat areas, which LZMA2 takes better).
- Under permanent, `pngr` is smaller still (48,135 for the scowl), since it needs no skeleton.
  D.1 is for temporary, where the file must come back exactly.

**D.2 Mixing tried on a hallway screenshot's pixels** (*built 31 October, behind the draft*).
- **Found:** hallway-compact's pixels (2,764,800 bytes, from `stb`) were carried raw by LZMA2,
  15,957 bytes. The packer's log said why: mixing was tested on its first 256 KiB as a picture
  "3 wide, 1 a pixel", the rope's reading. That came to 2,214 bytes, not 3% under LZMA2's, so
  mixing was not tried on the whole.
- **Built:** with the draft, mixing also weighs the shape the recipe gives (`stb`'s width and
  components, or `skr`'s), beside the shapes it weighed before, and keeps the smallest on the start.
- **Measured:** hallway-compact 16,058 → 12,772 bytes (`mix4 B22 I1280x3`). hallway-door was read
  right already (7,882 either way). All 18 hallway screenshots (docs/images) as one `.sieve`:
  245,100 → 211,937 bytes (−13.5%), 18 mixed as pictures where 11 were, given back the same. The book cover keeps the guessed 480×1, which beats its own
  160×3 (39 bytes against 44): that's why the shapes are weighed side by side, not replaced.
- It writes only v26's picture parameters, read by every reader since. It's behind the draft only
  because the packer must make the same bytes without it.

**D.3 Names equal but for case** (*built 31 October*; found by step 11's failure).
- A `.sieve` made on Linux can hold `A.txt` and `a.txt`. Installing checked before writing that no
  file was already there, and both passed, since neither existed yet. So on Windows or macOS the
  second would be written over the first without a word.
- **Built** (no version):
  - `install_tree`, `unpack_archive` and `unpack_7z` refuse such a pair before writing anything,
    naming it, where the destination ignores case;
  - `sieve locate` warns when it reads such a folder.
  - Whether the destination ignores case is looked at, not assumed: so WSL's `/mnt/c` counts, and a
    Mac folder made to heed case does not.
- **Checked:** here (Linux), both files are written; with the check forced on, all three installers
  refused and wrote nothing. CI step 11 installs `tests/case_pair.sieve` and a zip of the pair, and
  on Edward's Windows run checks the refusal for real.

**D.4 Undo any filter that can be undone, before modelling.** D.1 is one case of a rule: where a
format filters reversibly (PNG's row filters, TIFF's horizontal predictor, a WAV's delta coding), the
models do better on the values before filtering, as pngr already showed under permanent. Only PNG is
measured; the others are for the per-format corpus (C.7).

**D.5 CI step 11's own time.** It took 2,479 s here and 1,781 s on Edward's machine (to its failure)
of the full CI. *Built 31 October:* each traced command now carries the time it started (bash 5's
`EPOCHREALTIME`), and `ci/trace_times.py` adds a step's time up by block:
`python ci/trace_times.py LOGS/11-oracle.trace ci/steps/11-oracle.sh`. Its first run (step 11 here, 2,646 s, every block passing) found:
- **27.6% in one command:** the oracle reading v21's archives, whose contents (1.4 MB) the packer
  mixed as one group, decoded in Python in 726 s. That block checks skeletons, and groups are
  checked on their own, so it now packs with Prioritise Decoding Speed: 3 s, the same checks.
- **Next:** two vector blocks of about 200 s each (lines filtered by others, figure-v1), and the
  v32 blocks, 199 s.
- Step 11 should now take about 1,920 s here.

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
| A shared web dictionary | 0.5% under brotli on 102 unseen pages, 6.6% over on Edward's 7. *Built since, as v33's `W1` (3 November 2026):* a 64 KiB dictionary learnt from MDN's examples, 8.8% under brotli on 258 unseen pages, 10.3% under on Edward's site's 11 (FEATURES.md) | IDEAS §19.92 |
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

- The File Locator's figure beside 7z's 3,289,322 (A.4).
- Whether to write FILTERS.md.
