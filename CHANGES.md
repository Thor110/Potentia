# Mixing existing methods in a new way: mix-v2, `sieve-manifest-v26` (27 October 2026)

Unzip over the repository root. Committed locally (not pushed), on top of your 66ddf7a. This is a new instructions version, and every older one reads as before. Details are in IDEAS §19.102, SPECIFICATIONS §12.2 (mix-v2 and v26) and HANDOFF.

## What it is
IDEAS §19.102: your filters, each made a model inside mixing, weighed bit by bit. I measured every part first on the repository's own files and took it only where it paid. mix-v2 is mix-v1 with:
- **A match model:** the longest earlier stretch the last six bytes repeat. This is what LZ does, and mix-v1 lacked it, which is why LZMA2 beat it on the teapot and the screenshots.
- **The column (your rope's "byte above") and the field,** for tables and word lists.
- **Sparse contexts, two mixers, and two final correction stages (SSE).**
- **Your triangle inside mixing:** for a picture, the triangle's prediction and its neighbours become models. The picture's shape comes from the rope's search, or is guessed from the bytes when the search misses it (it does on small pictures).

It runs at about the same speed as mix-v1, despite twice the models, because the counters sit together in memory (two places a byte instead of eight). It takes about 240 MB while coding or decoding (mix-v1: 128 MB).

## Measured (bytes)

| File (all from the repository) | LZMA2 | mix-v1 | mix-v2 |
| :--- | ---: | ---: | ---: |
| Don Quixote | 685,794 | 565,596 | 532,168 |
| IDEAS.md | 122,303 | 102,129 | 95,289 |
| test_core.cpp | 64,735 | 64,777 | 53,002 |
| Utah teapot (.obj) | 34,380 | 53,595 | 24,720 |
| moby-pos table (1 MB) | 261,365 | 236,321 | 158,706 |
| SCOWL word list | 84,882 | 84,763 | 41,010 |
| Vitruvian, raw grey (rope-wave-v6: 762,766) | 865,400 | 802,822 | 727,842 |
| A hallway screenshot's pixels | 93,936 | 105,134 | 62,925 |

As Sieve instructions:
- **Don Quixote:** 532,501 bytes (v25: 565,664).
- **The ten PNGs from v25's measurement:** 210,046 bytes (v25: 247,692).

## Measured and not taken
- **LOCO-I's bias correction for the rope:** 0.4% on the Vitruvian, and the screenshot came out twice as large. Mixing with the triangle does 4.6% better anyway.
- **A dictionary (SCOWL) read first,** as a model every reader already has: 2% better on Don Quixote, but worse on a 3 KB text (1,316 bytes against 1,279).
- **Two-rate counters, or inputs weighted by how often their context was seen:** each helped one file and hurt most.
- **Tables twice as large (2^23):** under 1% smaller, for twice the memory.

## Using it
- **`sieve mix FILE`** now makes mix-v2. `--picture 1280x3` treats the file as a picture's pixels; `--v1` gives mix-v1.
- **`sieve locate --items`** and the File Locator make v26 where it's smaller. `--v25` makes no mix-v2.
- **The oracle has its own mix-v2,** written from the specification. It matched Sieve on its first run (text, grey and RGB pictures). Its commands are `mix-unpack --v2` and `unpack` for v26.

## Worth knowing: install time
| | v25 | v26 |
| :--- | ---: | ---: |
| Don Quixote installs in | 11.7 s | 8.4 s |
| The ten PNGs install in | 0.75 s | 28.3 s |

- **Text gets both smaller and quicker,** because mix-v2 decodes faster than mix-v1.
- **Pictures are the trade-off:** 15% smaller, but much slower to install. In v25 they were carried by the rope and LZMA2, which decode almost instantly. Now mixing decodes their 24.8 MB of pixels at about 1 MB/s.
- **Packing takes about the same time:** 271 s against 262 s for the PNGs; Don Quixote in 19 s.
- **I've left it choosing the smallest,** as Sieve always has. If you'd rather it weighed install time too (say, not mixing a picture unless it saves more than some share), that's a small change in the packer. Your call.

## Worth knowing: other things
- **Two codes' SHA-256 are pinned** in the unit tests (`test_mix2`: a text and a 60 × 40 RGB picture). Windows and macOS must make the same bytes, so please check step 01 there.
- **Decoding speed:** mixing still decodes at about 0.3 MB/s. Large raw pictures (a PNG's pixels, say) take a while to install. Speed is first on the "Next" list.
- **A small finding:** on random words, where nothing ever repeats, mix-v2 is a little larger than mix-v1. Its counters settle faster, which costs on perfectly steady data. Every real file measured came out smaller.

## Checks run here
- The build passes, and so do the unit tests (77,079 checks, 0 failures) and `check_help` (599 options, none missing).
- CI steps 10 and 11 pass. Step 11 took 2,284 s here against 1,372 s before. Part of that is my timing runs alongside it; part is the oracle decoding mix-v2 in Python, which is slow.
- I didn't run the other steps; they don't touch this change.

## Files
- `Sieve/core/`: include/sieve/mix.hpp, src/mix.cpp
- `Sieve/tools/`: cli/pack.cpp, pack.hpp, locate.cpp, locate.hpp, rope.cpp, help.cpp, args.hpp, items.hpp; sieve_cli.cpp; check_help.py
- `Sieve/client/file_locator.cpp`
- `Sieve/reference/sieve_ref.py`
- `Sieve/tests/test_core.cpp`
- `Sieve/ci/steps/`: 10-hallway.sh, 11-oracle.sh
- `Sieve/docs/`: SPECIFICATIONS.md, IDEAS.md, HANDOFF.md, SIEVE-INSTALL-USAGE.md
