# Decoding speed, and grammar as a model: `sieve-manifest-v27` (27 October 2026)

Unzip over the repository root. Two commits, local only (not pushed), on top of your f155116. One new instructions version (v27); every older one reads as before, and mix-v1's and mix-v2's codes are unchanged (their pinned SHA-256s still match).

## 1. Prioritise Decoding Speed (your box)
- **Where:** a third box in the File Locator (key P), `--prioritise-decoding` for `sieve` and the hallway, and `[locator] prioritise_decoding` in the ini.
- **What it does:** the packer mixes no file, and keeps the smallest of the other ways. I measured decoding speeds to set this rule: the rope 15 to 20 MB/s, LZMA2 faster still, mixing 0.3 to 1 MB/s.
- **The ten PNGs:** 247,692 bytes installed in 0.6 s with it, against 210,046 bytes in 28 s without.
- **The report says so:** `decoding  prioritised: mixing not tried on N file(s)`.
- **Cached instructions:** the File Locator now makes them again when Try every way or this box changes. Before, it only checked the time and the compressed-formats box.
- **The boxes wrap to a second row** when the window is too narrow, as the filter settings do. At 1920 × 1080 all three fit on one row.

## 2. Decoding faster, the same bytes
- **Installing decodes files at once:** the rope's and mixing's files are decoded in parallel, up to SIEVE_PACK_THREADS (else every core), with no new one started while those under way hold half the free memory. The ten PNGs went from 28.3 s to 9.1 s on 4 cores; your 12 threads should do better still.
- **Mixing itself is faster, with the same codes:**
  - A counter's division is now a multiply by a reciprocal, checked exact for every pair it can meet.
  - The counter tables are aligned to cache lines, and each slot is fetched early.
  - The SSE stages' rows are fetched before mixing, since their contexts are known by then.
  - Decoding, best of five runs: Don Quixote's first 800 KB 2.35 s → 2.16 s; the Vitruvian picture 3.97 s → 3.27 s; the screenshot about the same.
- **Where the rest of the time is:** the two SSE stages are about 40% of mixing's time, for 0.07% (pictures) to 1.7% (text) of its size. Dropping them would be a format change; I've left that for you to weigh.

## 3. Grammar as a model: mix-v3 and v27
I measured your grammar idea three ways:

| Way | Don Quixote (800 KB) | 3 KB text |
| :--- | ---: | ---: |
| mix-v2 | 191,190 | 1,303 |
| Parts of speech of the words before (Moby) as contexts | −0.15% to −0.55% | larger |
| **The word list as a prediction (taken)** | **187,650 (−1.9%)** | **1,212 (−7%)** |

- **How the word list works:** from the word so far, SCOWL gives the letters that can come next (and the word's end), each weighted by how many words go on that way. That becomes a probability for each bit, mixed in with the rest.
- **Rules as predictions:** a word missing from the list costs a few bits and never fails, as a hard filter would.
- **Other gains:** `IDEAS.md` −1.5%, a 20 KB text −3.5%, a word list −35%, the parts-of-speech table −5%.
- **Where it costs:** a PDF +0.45%, a table of numbers +0.16%. So the packer codes a text's start both with and without it and keeps the smaller.
- **Which list:** sizes 35, 60 and 80 came within 0.2% of each other, so I used 60 (`scowl-en-60`, the registered file, SHA-256 checked).
- **As Sieve instructions:**
  - Don Quixote 527,837 bytes (v26: 532,501), installed in 7.7 s (v26: 7.1 s).
  - A small mixed folder: 72,692 bytes (v26: 88,341).
- **The format:** a way `mix3 B22 D60`. `sieve mix --words 60` makes one. `--v26` makes v26 or older.
- **Who can read it:** `sieve`, the hallway and `sieve-install` all get the list from Sieve's data folder. A copy of `sieve-install` without that folder says to use Sieve.
- **The oracle:** it has its own trie and mix-v3, written from the specification, and matched on its first run.

## 4. A finding and a suggestion
Mixing is only tried on files of 4 KiB or more. I checked whether that limit should come down now that small texts mix so much better:

| 294 small texts from the repository (512 B to 4 KiB, 628,320 bytes) | Bytes |
| :--- | ---: |
| Each mixed alone (mix-v3) | 228,502 |
| All together, LZMA2 (what Sieve does now) | 110,835 |
| All together, mixed | 95,425 |

- **The limit is right:** small files share so much that packing them together beats mixing each alone, two to one.
- **But mixing the shared stream would save another 14%.** That's the stream every file carried as it is goes into, coded by mixing instead of LZMA2. It's a format change (a v28), and I'd suggest it as the next step.

## Checks run here
- The build passes, as do the unit tests (77,093 checks, 0 failures; mix-v1's and mix-v2's pins unchanged, mix-v3's new) and `check_help` (602 options, none missing).
- CI steps 10 and 11 pass (step 11 in 2,237 s here).
- Screenshots at 1920 × 1080 and in a narrow window show the boxes and the wrap.

## Files
- `Sieve/core/`: include/sieve/mix.hpp, src/mix.cpp
- `Sieve/tools/`: cli/pack.cpp, pack.hpp, locate.cpp, locate.hpp, items.cpp, items.hpp, rope.cpp, help.cpp, args.hpp; sieve_cli.cpp
- `Sieve/client/`: file_locator.cpp, file_locator.hpp, app_main.cpp, app_settings.cpp, app_settings.hpp
- `Sieve/data/lang/en.txt`
- `Sieve/reference/sieve_ref.py`
- `Sieve/tests/test_core.cpp`
- `Sieve/ci/steps/11-oracle.sh`
- `Sieve/docs/`: SPECIFICATIONS.md, IDEAS.md, HANDOFF.md; `Sieve/README.md`
