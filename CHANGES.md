# Files mixed together: `sieve-manifest-v29` (27 October 2026)

Unzip over the repository root. Committed locally (not pushed), on top of your be8b2d6. One new instructions version (v29); every older one reads as before.

## What step 1 turned into
- **The plan was to mix the shared stream** (every file carried as it is) instead of packing it with LZMA2.
- **That alone gains little:** in real instructions the stream is small, since every file over 4 KiB is already mixed on its own. Sieve's tools folder: 79 KB of 1.36 MB. Mixing it saved only 0.3 to 0.6%.
- **The real gain was in the files mixed one by one.** Each one starts from nothing; mixed together in that stream, each starts from what the files before it taught. So v29 tries exactly that, for every file that would be mixed alone (pictures keep their own models), and keeps it only where it's smaller.

| Folder | v28 | v29 | Change | Install, v28 → v29 |
| :--- | ---: | ---: | ---: | ---: |
| Sieve/tools (63 files) | 349,689 | 260,529 | −25.5% | about 3 s |
| Sieve/client (47 files) | 336,403 | 247,045 | −26.6% | about 3 s |
| Sieve/docs | 1,045,190 | 973,631 | −6.8% | 1.1 s → 4.2 s |
| Sieve/tests | 401,156 | 396,337 | −1.2% | 1.0 s → 3.3 s |
| one file alone | — | the same | — | — |

## The costs
- **Packing takes about 25% longer:** the files are mixed a second time, together.
- **Installing can be slower:** one stream is decoded in one piece, where files mixed alone were decoded at once. In docs, its screenshots (already compressed) sit in the mixed stream too, costing time and gaining nothing.
- **It's held to 16 MiB of stream.** So a folder the size of the release (Sieve's data alone is 27 MB) isn't mixed together, and `sieve.sieve` won't change yet.
- **Prioritise decoding speed turns it off,** as it does all mixing.

## The format
- **The stream line:** `stream lzma2 P B 0 mix4 B22 D60`.
- **In bytes:** the dictionary byte 0xFE, then the parameters.
- **`--v28`** makes v28 or older.
- **The report line:** `stream    the files carried together mixed by mix-v4 (B22 D60): N bytes, LZMA2's M; K of them mixed together rather than one by one (A bytes apart)`.
- **The oracle** reads it, mixing the stream back with its own mix-v4.

## Next, to remove those costs (suggestion)
- **Several mixed streams:** the folder's files in groups, each group mixed, the groups decoded at once. That lifts the 16 MiB limit and brings installing back to its old speed.
- **Already-compressed files left out** of the mixed streams.
- Both are a further format step (v30). I haven't started either.

## Checks run here
- The build passes, as do the unit tests (154,214 checks, 0 failures) and `check_help` (605 options, none missing).
- CI steps 10 and 11 pass (10 in 837 s, 11 in 1,950 s here).
- **CI changes:** step 11's v28 block now names `--v28`. A new v29 block mixes five small repository files together (9,982 bytes against v28's 11,280) and reads them back with Sieve, the oracle and sieve-install; `--v28` and Prioritise Decoding Speed make no v29.

## Files
- `Sieve/tools/`: cli/pack.cpp, pack.hpp, locate.cpp, locate.hpp, items.hpp, help.cpp, args.hpp; sieve_cli.cpp
- `Sieve/client/`: file_locator.cpp, app_settings.cpp
- `Sieve/reference/sieve_ref.py`
- `Sieve/ci/steps/11-oracle.sh`
- `Sieve/docs/`: SPECIFICATIONS.md, IDEAS.md, HANDOFF.md
