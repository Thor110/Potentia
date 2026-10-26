# The SSE stages retired (`sse-v1`): mix-v4, `sieve-manifest-v28` (27 October 2026)

Unzip over the repository root. Committed locally (not pushed), on top of my 324a698 (the decoding-speed and word-list update, if you haven't applied it yet: this zip holds both, as three commits). One new instructions version, v28. Every older one reads as before: mix-v2 and mix-v3 keep the stages they were made with, and their pinned codes are unchanged.

## What you asked for
- **Both correction stages retired as one filter, `sse-v1`,** with their figures and the reasons kept in three places:
  - the "Retired filters" list in FILTERS-CONFLICTS.md (a stage of mixing rather than a line filter, but one place for everything retired);
  - beside the code in `core/src/mix.cpp`;
  - SPECIFICATIONS §12.2, with IDEAS §19.102 and HANDOFF too.
- **mix-v4 is mixing without them.** It covers text (`B22`), pictures (`B22 I949x1`) and text with the word list (`B22 D60`). The packer uses it in mix-v2's and mix-v3's place, by the same tests.

| File | With both stages | First only | Neither (mix-v4) | Decode time, both against neither |
| :--- | ---: | ---: | ---: | ---: |
| Don Quixote, first 800 KB | 191,190 | 191,526 | 192,884 | +9% |
| Vitruvian picture | 728,919 | 729,163 | 729,462 | +29% |
| Screenshot pixels, 1 MB | 15,821 | 15,811 | 16,185 | +21% |
| Utah teapot | 25,258 | 25,295 | 25,391 | +15% |

## As instructions (two runs each)
- **Don Quixote:** 533,148 bytes (v27: 527,837, so +1.0%), installed in 5.7 to 6.0 s (v27: 6.8 to 7.2 s, so about 15% faster).
- **The ten PNGs:** 217,170 bytes (v27: 210,046, so +3.4%).
  - On one thread they install in 24.6 s against 27.4 s (10% faster).
  - On four threads both take about 9.5 s. Four mixings at once are held back by memory speed, so the saving disappears there. On your 12 threads it may differ.
- **In short:** text gets 10 to 15% quicker at about 1% in size. Pictures gain mostly when installed one at a time.

## Using it
- **`sieve mix FILE`** now makes mix-v4: `--picture 1280x3` for a picture, `--words 60` for the word list. `--v2` gives the versions with the stages (mix-v2, or mix-v3 with `--words`), and `--v1` gives mix-v1.
- **`sieve locate --items`** and the File Locator make v28. `--v27` keeps the stages (v27 or older).
- **The oracle** has its own mix-v4 (`mix-unpack --v4`), which is mix-v2's decoder without the stages. It matched on its first run and reads v28.

## Checks run here
- The build passes, as do the unit tests (77,108 checks, 0 failures; `test_mix4` pins three codes: a text, the same with the word list, a picture) and `check_help` (604 options, none missing).
- CI steps 10 and 11 pass (10 in 905 s, 11 in 2,124 s here).
- CI step 11's v26 and v27 checks now name their versions (`--v2`, `--v26`, `--v27`). A new v28 check covers a text with the word list and a guessed picture in one folder, read back by Sieve, the oracle and sieve-install.

## Files (this zip: all three commits since your f155116)
- `Sieve/core/`: include/sieve/mix.hpp, src/mix.cpp
- `Sieve/tools/`: cli/pack.cpp, pack.hpp, locate.cpp, locate.hpp, items.cpp, items.hpp, rope.cpp, help.cpp, args.hpp; sieve_cli.cpp
- `Sieve/client/`: file_locator.cpp, file_locator.hpp, app_main.cpp, app_settings.cpp, app_settings.hpp
- `Sieve/data/lang/en.txt`
- `Sieve/reference/sieve_ref.py`
- `Sieve/tests/test_core.cpp`
- `Sieve/ci/steps/11-oracle.sh`
- `Sieve/docs/`: SPECIFICATIONS.md, IDEAS.md, HANDOFF.md, FILTERS-CONFLICTS.md; `Sieve/README.md`
