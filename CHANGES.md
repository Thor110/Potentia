# v33 frozen (3 November 2026)

v33 (`sieve-manifest-v33`) is now what Sieve makes by default. A `.sieve` is v33 only where one
of v33's ways is used; otherwise it is v32, as before.

## The freeze

- `kNewestRope` is 16 and `kDraftVersion` 34 (`tools/cli/pack.hpp`, `tools/cli/draft.hpp`).
- `--v32` makes v32 byte for byte as the old default did. `--v33-draft` is still accepted but does
  nothing. `--v34-draft` is the new, empty draft (the hallway's `[locator] v34_draft`).
- Checked on five folders, with and without the Compressed formats boxes: the frozen default gives
  the same bytes as the old `--v33-draft`, and `--v32` the same as the old default.
- The refusal messages name the palette (`P`) along with the match ways and the x86 filter.
- The installer line names v33 ("v33: as v32, and copies, runs and ramps, ...").

## Two packer bugs found by CI steps 10 and 11, both fixed (`tools/cli/pack.cpp`)

- **A copy of one of Sieve's items.** The hallway's degree sample holds MIDI items with the same
  bytes. One was carried as a copy, which a reader refuses, because an item's size is listed as
  made. Such a file is now weighed as itself.
- **`--v5` and other text listings.** Copies and spans were still tried there, though a text listing
  can't hold them. They're now tried only where the newest instructions are made in bytes, the same
  rule the rope's newer versions already follow.

## CI (`ci/steps/11-oracle.sh`)

- `--v32` added to the v32 lines of the v33 blocks, and `--v33-draft` dropped from the v33 lines.
- The mt.sieve check expects `sieve-manifest-v33`.
- Three manifest-version patterns accept v33.
- The hallway screenshot's check expects v33's way: its palette with the match ways, 9,674 bytes
  against v32's 12,761. It expects v32's way under `--v32`.
- With `--v31`, that screenshot is now mixed at a shape guessed from its bytes: 13,990 bytes, a v28
  listing, where it used to be carried as it was. The cause is the rope probe (2 November). It
  skips the rope's search on these pixels, whose start comes to 1.74 times LZMA2, so mixing wins.
  Every reader since v26 reads that shape. The check expects it and installs it.
- Two texts mixed as a group now take the match ways (`mixt B16 F80 C2`). The word list's v32
  check now packs with `--v32`, and a v33 group is checked beside it. Sieve, sieve-install and the
  oracle each give the v33 group back exactly.

Results on this build:

- Unit suite: 154,292 checks, 0 failures.
- CI step 10: passed.
- CI step 11: passed. Lines 1 to 1,268 passed in a full run. The rest, after the last two fixes,
  passed in the folder that run kept.

## Documents

- **New:** `docs/FEATURES.md` lists what Sieve does, each feature with its measured figure and where
  that figure was measured. It's linked from README.md's docs table, for the post.
- **Updated:** SPECIFICATIONS (v33 frozen), SIEVE-FORMAT.md (the v33 header, format 24 `spn`, and the
  new ways and decoder needs), METHODS, MANUAL, POWER-USAGE, HANDOFF (frozen at v33, v34 behind
  `--v34-draft`) and NEXT-VERSION (the freeze, the review, and the CI findings above).

## Still to do

- Your full CI on Windows, Linux and macOS (I ran only steps 10 and 11 here).
- (2) The web dictionary, once you have chosen the sample pages.
- (3) STATISTICS.md and the Proof samples measured again at v33.
