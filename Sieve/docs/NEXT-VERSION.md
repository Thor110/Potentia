# The next manifest version: a plan for `sieve-manifest-v32`

*Written 30 October 2026, from a reading of the documentation (IDEAS, HANDOFF, REVIEW,
SHOWCASE-PLAN, SPECIFICATIONS, SIEVE-FORMAT, STATISTICS) and the code. Nothing here is built. Every
figure is quoted from the document named beside it; "not measured" means nothing was found.*

## 1. Where things stand

**The freeze.** The format has rested at `sieve-manifest-v31` since 27 October 2026 (HANDOFF §"The
standing rules"). Edward: "it is time I stopped pushing more manifest versions, that was just
caused by a succession of ideas for filters that cascaded into many manifest versions". The rule:
packer changes that need no new format land freely, and anything needing v32 "waits until it is
worth a version on its own". IDEAS's list "Waiting for the next format version" says most of what
waits is small, and "best gathered into one version when the pause ends". SHOWCASE-PLAN adds why
the freeze matters: "others can only build on a format that holds still."

**What the pause produced.** Since then everything has landed within v31: speed, the logs, the
weighing, measurements of enwik8 and enwik9, installs that ask about each file. Each `.sieve` was
checked to be the same, byte for byte, before and after.

**Where Sieve is weakest now** (Proof/STATISTICS.md):

| Where | Sieve | Against | Gap |
| :--- | ---: | ---: | ---: |
| WoWRevived (a game, mostly compressed already) | 1,075,924,966 | 7z 1,073,748,445 | +0.20% |
| A 1,597-byte script (`control.py`), first measured | 832 | 7z 822 | +1.22% (now 731, −11.07%: §4a) |
| enwik8, optimised for size | 21,675,870 | PPMd var. J 21,388,296 | +1.34% |
| enwik9, optimised for size | 186,081,966 | PPMd var. J 183,964,915 | +1.15% |
| enwik8 against the context-mixing programs | 21,675,870 | cmix v21 14,623,723 | "about a third smaller" |
| The `sieve` program and the docs (8.4 MB, IDEAS §19.95), 22 October | 2,949,958 | 7z 2,836,214 | 7z ahead then, its x86 filter BCJ2 (30 October, 12.7 MB: Sieve 4,159,742 against a BCJ 7z's 4,277,050, 2.74% smaller) |

And the cost of the size: enwik9 optimised for size installs in 1,153.583 s, against 15.9 s
prioritised for decoding (21.12% larger). Where Sieve leads, it leads well: the source tree is
24.07% under 7z, the release folder 18.82%.

## 2. Rules for v32 (from the documents, gathered here)

1. **One version for several ideas.** Gather what is ready into v32 rather than one version each
   (IDEAS; HANDOFF: "best gathered into one version").
2. **Measure first.** Each idea goes in only with a measured gain on named files, and the cost in
   making and decoding time beside it. Nothing goes in on a guess (IDEAS §19.95: "measure first").
3. **Every older version still read** (HANDOFF: "A new manifest version for each format change,
   every older one still read"; Edward, 25 October: "maintaining backwards compatibility until that
   point is ideal").
4. **A file names the oldest version that holds what it uses** (IDEAS §17.7). The packer is always
   the newest ("a .sieve should always be the latest", Edward, 27 October), but a file that uses
   nothing new stays v31, v20 or older, so older readers still read it. The code already works this
   way: the version flags are set from what was used.
5. **The oracle is written from the specification**, never from the C++, and never "fixed" to
   agree with it (HANDOFF). v32's paragraph in SPECIFICATIONS §12.2 comes first, then the oracle
   from it.
6. **A file carries only what it needs** (Edward, IDEAS §16.3): optional parts behind flags, at no
   cost to ordinary files.
7. **Decoders pinned, licences shipped** (IDEAS §19.88): third-party code fetched at a pinned tag,
   with its licence in `third_party_licenses/`.
8. **Edward's order for compression work** (IDEAS §19.88): layering other schemes (JPEG, MP3 and the
   like) is "among the last things looked at, so that the basic utility of the tool is proven
   first".

One fact to plan around: **the head's flags byte is full.** All eight bits are used (1 root, 2 x86,
4 lzma2, 8 items, 16 rope, 32 body packed, 64 a graph, 128 the body as text: `locate.cpp`,
`binary_head`). IDEAS §19.93 still says two bits are free, which is out of date. A v32 that needs
a new optional part must say so by its version byte, or add a second flags byte when the version
is 32 or more.

## 3. What could go into v32

Every candidate from IDEAS's list and the rest of the documents, with what is measured. Grouped by
how ready it is.

### 3a. Measured gain, ready to design

**A file carried as its differences from another (the `delta` way; IDEAS §19.106).**
- The case: WoWRevived's `MAPS.WoW` and `MAPS-Larger.WoW`, 160 MB each, mostly the same content.
- Measured, within v31, on pairs made here where B is A with regions replaced by random bytes:
  - side by side in the lzma2 stream: a 20 MB pair, 9,637,859 bytes, under solid xz's 9,649,776;
  - past the dictionary: a 30 MB pair, 30,914,001 bytes with a 16 MiB dictionary, 16,605,654 with
    64 MiB (one file alone 15,094,454);
  - mixed: each alone 2,817,882 bytes in all; the two as one group 1,571,543, 44% smaller.
- The `delta` way itself: **not measured**.
- Two routes need no new version at all: a larger LZMA2 dictionary (MAPS would want 192 MiB or more;
  the format allows 4 GiB; the packer stops at 64 MiB), or grouping two alike large files for
  mixing. Both change what the packer makes, so they are Edward's choice (IDEAS §19.106).
- **First step:** Keep a log on WoWRevived shows how MAPS.WoW is carried now. If a larger
  dictionary or grouping gets most of the way, `delta` may not be worth a version.

**A small file's overhead (IDEAS §19.90; Edward, 28 October: `control.py` "10 bytes over its 7z").**
- Measured (first, before v16 and v20): `control.py` 832 against 7z's 822; now 731 (§4a). A small entry costs about 12 bytes in bytes
  (`mark.js`: 7 bytes of its 474, 1.5%). The starting page: 32 bytes as 159.
- What cannot go: the head's 32-byte SHA-256, which every install checks.
- What could: the root's name, varint sizes, the version and flags bytes, the listing's own
  framing. Exactly how much each costs on `control.py` is **not measured**.
- Edward: "not vital now", but "even the small savings will add up".
- **First step:** a byte-by-byte breakdown of `control.py`'s 832 bytes. This decides whether 10
  bytes can be taken off.

### 3b. A lead, to measure before deciding

**BCJ2 for programs (IDEAS §19.95).** 7z beats Sieve on a folder that is mostly a program (2,836,214
against 2,949,958 on the `sieve` program and the docs), "its x86 filter being BCJ2 ... against
Sieve's BCJ: a lead to measure next". BCJ2 in Sieve: **not measured**. **First step:** run 7-Zip's
BCJ2 on Sieve's own programs and set its streams against BCJ's lzma2 stream, without building
anything into Sieve.

**Mixing with word frequencies (IDEAS §19.102).** mix-v3's word list gave Don Quixote's first 800
KB 191,190 → 187,650 (1.9%) and a 3 KB text 1,303 → 1,212 (7%). Frequencies would weight the words
by use; SCOWL has none, so it would be a new pinned file. **Not measured.** This is where the gap to
PPMd on enwik8 and enwik9 is (1.34% and 1.15%). Measured ideas not taken, for reference: LOCO-I bias
0.4% on one picture; parts of speech 0.15% to 0.55% on Don Quixote and worse on small texts; tables
of 2^23 under 1% at twice the memory.

**Mixing as a kind of piece, and each rope piece its own wave (IDEAS §19.103).** The step that needed
no version gained nothing (the same size under four weighings, "because mixing the whole file beat
every cut"). What is left needs a version: a tar of a picture and text, mixed whole as one picture,
"its text included". **Not measured.**

**Decoding one file without the rest (IDEAS §19.104).** Today the first file costs the whole
decode. **First step, no version:** a cache filled at start-up, on Edward's *Alien Trilogy* engine as
the test bed. Only if that is not enough: reading a group at a time, or the listing first and each
stream when asked for, which needs a version.

**A shared web dictionary (IDEAS §19.92).** On 102 unseen pages, a 120 KB dictionary came 0.5% under
brotli (smaller on 66 of 102). On Edward's own 7 pages it was 6.6% over brotli, smaller on none.
"Not yet a new version": it goes in only once it beats brotli, measured on today's pages, not the
2014 set.

### 3c. Small, or measured as not worth a version alone

- **The archive's format as a number, with its recipe** (IDEAS §19.88). Saves a few bytes of text a
  temporary archive (a varint is 1 byte for the first 128 formats). Not measured. v20 already writes
  remade archives' formats as numbers; what is left is v31's `skm` text and the recipe settings.
  Worth taking only as part of v32, not on its own.
- **Headers modelled** (IDEAS §19.88): "Measured, the headers are small and LZMA2 already packs
  their repeats, so it waits." Leave out.
- **The rope's next contexts** (IDEAS §19.87): "at most 0.3% on one file" (`pi6.txt`). Leave out.
- **Other lines' places coded by the rope** (IDEAS §19.87). Not measured; leave out until measured.

### 3d. Larger, and last by Edward's order

- **JPEG made again from its coefficients** (IDEAS §19.88): about 20% smaller, as Lepton, packJPG and
  JPEG XL do. JPEGs are where Sieve gains least today (−0.37% to −0.47% against 7z), and WoWRevived's
  +0.20% is in already-compressed data. The largest single gain on the table, but a large build: an
  outside `cjxl`/`djxl`, or Sieve's own baseline-JPEG reader and writer.
- **A sound model, and PNGs remade by zlib itself** (HANDOFF's 27 October plan, steps 3 and 5).

## 4. What v32 holds (Edward, 30 October 2026)

**One version, gathered.** Edward: "Lets work on things we add as just v32 for now so that we don't
add multiple manifest versions again". So everything below goes into v32 and nothing ships on its
own.

**How that is kept:** v32 is built behind a **draft switch** (`--v32-draft`, for testing only).
Without it, every build keeps writing v31, as now, until v32 is complete, specified, read by the
oracle, and frozen. Files made with the switch are test files: the finished v32 need not read them,
and nothing made with it is published. Only then does v32 become the newest, in one step.

**In v32, in the order to build them:**
1. **The `delta` way** (§3a): a file as copies from another file of the folder plus new bytes, the
   new bytes carried as any file's are. Found automatically, with no box: finding alike files costs
   under 1 MB and under a second for a 160 MB pair, and the index about 92 MB, built only for a pair
   found alike (IDEAS §19.106, measured 30 October). It goes in where it comes out smaller than the
   file's best other way.
2. **A small file's overhead** (§3a), once `control.py`'s bytes are broken down.
3. **The archive's format as a number** (§3c), small, while the listing is being changed anyway.
4. **JPEG made again from its coefficients** (§3d; Edward: "JPEG recoding is a good idea"), the
   largest build and the largest gain on JPEGs, last so the rest is not held up by it.
5. **BCJ2** and **word frequencies for mixing**, only if their measurements (§3b) earn a place.

## 4a. Progress

**1. The `delta` way: built, behind the draft switch (30 October 2026).**
- **Built:** SPECIFICATIONS §12.2's v32 paragraph; the packer, the reader and the oracle (which reads
  delta-v1 from the specification, not the C++); `--v32-draft` for `sieve` and the hallway, and
  `[locator] v32_draft`; CI step 11's v32 block; METHODS §2a.
- **Measured** (IDEAS §19.106): a 160 MB pair, 22,537,063 → 17,947,687 bytes (20.36% smaller),
  made in 627 s instead of 1,113.
- **For Edward to try:** WoWRevived with `--v32-draft` and Keep a log.
- **Edward's first run (30 October 2026):** WoWRevived-English-v1.1.2, Try every way and Prioritise
  decoding speed ticked, Keep a log not ticked. 1,075,924,966 bytes, exactly the size STATISTICS
  already gave, so no file was carried as a delta. Either the draft was not on, or MAPS.WoW and
  MAPS-Larger.WoW share less than the half the packer asks. `sieve alike MAPS.WoW MAPS-Larger.WoW`
  (added the same day) says which, in seconds: how much of one the other holds, and whether the
  packer would pair them.
- **`sieve alike` on Edward's maps (30 October 2026):** MAPS.WoW 160,890,897 bytes, MAPS-Larger.WoW
  162,426,225. 42,222 of the larger's 42,589 blocks are in the smaller: 160,318,357 bytes (98.7%).
  As a delta: 104 copies covering 160,338,860 bytes (98.7%), 2,087,365 new bytes, a script of
  2,087,830 bytes before LZMA2. The packer would pair them ("taken yes"), in 1.99 s. So the draft
  was not on in the first run; the next, with `--v32-draft` and Keep a log, measures it.

**2. A small file's overhead: measured, to build next (30 October 2026).** `control.py` is not in the
repository; the stand-in is `tools/fetch_corpus.py` (2,191 bytes), whose `.sieve` is 1,153 bytes:

| Bytes | What | v32 |
| ---: | :--- | :--- |
| 5 | `sieve` | stays |
| 1 + 1 | the version and the flags | stay |
| 2 | the lzma2 stream's packed size | can go: the stream is the file's last part, so it is what is left |
| 2 | its unpacked size | can go: the listing gives it |
| 1 | its dictionary's byte | can go where it is the one its size gives |
| 32 | the tree's SHA-256 | stays (Edward) |
| 22 | the listing (its name, size and way) | stays |
| 7 | LZMA2's framing (one chunk's header and the end mark) | can go: a stream under 2 MiB as plain LZMA, one byte of settings |
| 1,080 | the file, packed | |

About 11 bytes can be taken off: 1,153 to about 1,142.

**`control.py` itself (Edward sent it, 30 October 2026):** its `.sieve` is now **731 bytes**, not the
832 STATISTICS gave, so it is already 91 bytes (11.07%) under 7z's 822. The 832 was from before v16
and v20 wrote the head and the listing in bytes. Of the 731: 670 the file packed, 32 the SHA-256,
17 the listing, 7 the magic, version and flags, 5 the stream's sizes and dictionary. The same 11 or
so bytes could go (to about 720). Worth taking while v32 is open, but no longer urgent.

**3. JPEG made again from its coefficients: begun (30 October 2026).** Edward: "make a start on the
jpeg plans". IDEAS §19.88 gives two routes: an outside tool (`cjxl`/`djxl`, or brunsli), or Sieve's
own reader and writer of JPEG's coding, which "fits the aim of proving it within Sieve". This plan
takes Sieve's own, with the outside tools as the yardstick.

*First measurements* (a prototype written from ITU-T T.81 in the scratchpad, not Sieve's code; the
repository's seven JPEGs, all baseline: the public-domain Vitruvian and the test site's six):

- **Exact.** Each JPEG's coefficients were read out of its Huffman coding (no pixels made) and coded
  again with its own tables. All seven came back **byte for byte**. So a JPEG can be carried as
  its headers (308 to 892 bytes by xz) and its coefficients, and made again exactly.
- **A general model is not enough.** Laid out plainly (each component's blocks, coefficient by
  coefficient, two bytes each):

  | | JPEG | `.sieve` now | coefficients by LZMA2 | by Sieve's mixer |
  | :--- | ---: | ---: | ---: | ---: |
  | Vitruvian | 207,500 | 206,956 | 229,291 | 204,008 |
  | audio | 70,028 | 66,894 | 84,772 | 74,018 |
  | book | 82,905 | 80,569 | 97,499 | 82,973 |
  | door | 69,298 | 63,860 | 80,997 | 70,858 |
  | filters | 70,029 | 65,997 | 84,313 | 73,477 |
  | picture-in-hand | 59,365 | 55,013 | 70,027 | 60,508 |
  | vitruvian (site) | 116,168 | 116,235 | 130,724 | 112,509 |

  The JPEG's own coding is about as good as a general model can do on the coefficients. The gain
  that Lepton and packJPG report, about 20%, comes from a model made for them.

*The way, to build next:*
1. **A coefficient model of Sieve's own** (`jpeg-v1`). Each block's count of non-zero values is
   coded first, from its neighbours' counts. Then each value is coded from the same frequency in
   the blocks above and to the left, its place in the block, and how many non-zero values are left.
   The DC values are coded from their neighbours. One binary arithmetic coder, as mixing's.
   Prototyped and measured on the seven JPEGs and Edward's photos before anything goes into Sieve.
2. **In the format:** a remake kind (`jpg`, the next archive-format number): the JPEG carried as its
   headers and its coefficients coded by `jpeg-v1`, made again exactly on installing. The packer
   makes it again before keeping it, and keeps it only where it comes back exact and smaller; any
   other JPEG is carried as it is. The oracle can make it too (the model is Sieve's own and
   specified), which an outside tool would not allow.
3. **Baseline JPEGs first**, then restart markers, then progressive JPEGs (a different scan order;
   the coefficients are the same).

*The yardsticks, measured here (30 October 2026; Edward allowed GitHub).* Google's brunsli (MIT,
commit c18b9625) and JPEG XL's `cjxl --lossless_jpeg=1 -e 9` (libjxl, BSD-3, commit 90d03e49), each
built here from source, on the seven JPEGs and on two Pillow re-saves of Vitruvian at quality 90
(one progressive, one with optimised Huffman tables). Every file came back from both **byte for
byte**:

| | JPEG | `.sieve` now | brunsli | `cjxl` |
| :--- | ---: | ---: | ---: | ---: |
| Vitruvian | 207,500 | 206,956 | 170,260 (−17.95%) | 173,542 (−16.37%) |
| audio | 70,028 | 66,894 | 57,310 (−18.16%) | 57,812 (−17.44%) |
| book | 82,905 | 80,569 | 67,684 (−18.36%) | 67,978 (−18.00%) |
| door | 69,298 | 63,860 | 54,117 (−21.91%) | 55,341 (−20.14%) |
| filters | 70,029 | 65,997 | 55,468 (−20.79%) | 56,318 (−19.58%) |
| picture-in-hand | 59,365 | 55,013 | 46,757 (−21.24%) | 47,910 (−19.30%) |
| vitruvian (site) | 116,168 | 116,235 | 98,454 (−15.25%) | 99,986 (−13.93%) |
| Vitruvian, progressive q90 | 282,087 | | 212,680 (−24.60%) | 196,754 (−30.25%) |
| Vitruvian, optimised q90 | 284,505 | | 212,027 (−25.48%) | 195,676 (−31.22%) |

- On the seven originals brunsli is the smaller by 0.4 to 1.7 points; on the two quality-90 re-saves
  `cjxl` is, by about 6 points.
- Both took hundredths of a second to decode; `cjxl -e 9` 0.15 to 0.69 s to encode, brunsli 0.02 to
  0.05 s.
- Their decoders: brunsli's library is 435,904 bytes; libjxl's (encoder and decoder) 4,772,216, with
  highway, skcms and brotli beside it.

*For Edward to decide: which way.*
- **(a) brunsli in Sieve**, fetched at a pinned commit as preflate is. It is small, fast and the
  better on these originals. The oracle reads its recipe but cannot make the file (as with preflate).
- **(b) libjxl**: better on high-quality JPEGs, but about eleven times the size in `sieve-install`.
- **(c) Sieve's own model:** the oracle could make the file itself, but it is the most work, and the
  general-model test above shows nothing comes cheap.

*On Edward's three originals* (he sent them in a zip, 30 October 2026; measured in a scratch folder
and deleted, nothing of them kept): brunsli 19.74%, 20.65% and 20.22% smaller; `cjxl` 19.12%, 19.98%
and 19.26%; every file given back byte for byte by both (Proof/STATISTICS.md, Photographs). brunsli
is the smaller on all three, and about ten times quicker to make them.

The recommendation is (a), brunsli: the smaller on the originals measured (ten of them), the
quicker, and about a tenth of libjxl's size in `sieve-install`.

**Found on the way, for Edward to decide: a group with another file in its middle.** SPECIFICATIONS
v30 says a reader refuses a group of files mixed together that holds a file of another way in its
middle, and the oracle does. But v30's and v31's packer never cuts a group there, and Sieve's reader
accepts it (the `mixt` comment in `locate.hpp` even says "other ways may come between"). So a v30 or
v31 file can install with Sieve and be refused by the oracle.

Shown here on three files (a text, random bytes, a text), packed by the ordinary build: the oracle
refuses it, and Sieve gives it back identical. Two ways out:

- **(a) The specification and the oracle allow it,** as the reader and the files already made do. No
  file changes.
- **(b) The packer cuts groups there.** It costs: on those three files, 156,411 bytes against
  136,939, 14% larger.

(a) is the recommendation. The draft packs groups as v31 does until Edward decides.

## 5. The order of work

**Step 0: measure (no version, no change to any `.sieve`).**
1. Keep a log on WoWRevived: how MAPS.WoW is carried, and where the +0.20% is.
2. A byte-by-byte breakdown of `control.py`'s `.sieve`.
3. BCJ2 against BCJ on Sieve's own programs, outside Sieve.
4. Silesia, still not measured (SHOWCASE-PLAN: nothing is posted before it is).
5. (Done 30 October 2026: STATISTICS' Don Quixote row gave 683,205, about v23's figure. Measured
   again, it is 533,148 bytes, 22.28% under 7z.)

**Step 1: choose.** Edward picks what goes in, from §4.

**Step 2: specify.** v32's paragraph in SPECIFICATIONS §12.2 first: every way's syntax, where its
bytes go, the version byte, what an older reader does. Then SIEVE-FORMAT.md's v32 edition.

**Step 3: build**, behind a gate (`rope_newest >= 15`), so `--v31` still makes exactly what v31
makes. The places to change (from the commits that added v30 and v31):

| Where | What |
| :--- | :--- |
| `tools/cli/locate.hpp` | the `sieve-manifest-v32` constant and its rules; `bool v32` in `Manifest` |
| `tools/cli/pack.hpp` | `kNewestRope` 14 → 15; `PackReport::v32` and the feature's counts |
| `tools/cli/pack.cpp` | the gate; the feature; `m.v32` and the cascade (`m.v31 = ... \|\| m.v32`); the report lines |
| `tools/cli/locate.cpp` | `version()`, `binary_head`'s byte, `Manifest::parse` (range to 32, head map, cascade, maps' list), the new way in `unpack_v4` and the inner-way lists |
| `tools/sieve_cli.cpp`, `args.hpp`, `help.cpp` | `--v31` (as v31 at its newest), the installer's summary line, `sieve version` |
| `reference/sieve_ref.py` | ranges to 32, the cascades, the head strings, the new way, the 11 inner-way lists |
| `ci/steps/11-oracle.sh` | ranges widened; a v32 block (made, read back by Sieve, the oracle and sieve-install; `--v31` makes no v32) |
| `ci/steps/10-hallway.sh` | the pins on small files, if the overhead changes |
| `tools/cli/web_host.cpp` | refuse or read the feature (it assumes each file is one stretch of the contents) |
| Documents | SPECIFICATIONS §12.2, SIEVE-FORMAT, README, SIEVE-INSTALL-USAGE's version table, HANDOFF's rule, SHOWCASE-PLAN, IDEAS's list, METHODS |

Maps, sites, training runs, the hallway's saves, music and the File Locator follow on their own
(`kNewestRope`, `add_packed_newest`), as does sieve-install (it links the same reader).

**Step 4: prove.**
- Every older version still read: CI step 11's blocks for v29 to v31.
- Every file made with `--v31` byte for byte as before.
- The oracle reads v32.
- The pinned codes are the same on Windows, Linux and macOS.
- The Proof samples made again, with STATISTICS and PROOF.md measured again.

**Step 5: freeze again**, at v32, the same way.

## 6. Decisions

Made by Edward, 30 October 2026:
- The pause ends with work on v32, as one version (§4).
- MAPS.WoW and the like: the `delta` way, found automatically. Edward asked whether finding alike
  files would need a "contains similar files" box for its memory; measured, it does not (§4).
- JPEG recoding is in.

Still open:
1. Whether BCJ2 and word frequencies are worth measuring now.
2. Whether a new optional part takes a second flags byte (the first is full).
