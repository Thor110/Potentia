# Changes: `sieve-manifest-v13`, files cut into pieces (20 October 2026)

Copy `Sieve/` over the repository's `Sieve/`. No new files.

## What your photos showed

Measured on the originals you sent in the zip:

| part | bytes | LZMA2 | the rope's byte counts |
|---|---|---|---|
| 20230830_163945: phone metadata (start) | 17,733 | **4,678** | 7,206 |
| 20230830_163945: picture data | 4,121,949 | 4,122,154 (no gain) | **4,105,653** |
| 20240407_153939: phone metadata (start) | 25,652 | **12,536** | 16,126 |
| 20240407_153939: picture data | 4,561,723 | 4,561,949 (no gain) | **4,544,237** |

So 7z's gain was all in the metadata, and the rope's was all in the picture. Chaining them (the
rope on LZMA2's output) lost on a test photo, because LZMA2 scrambled the uneven spread of bytes
that the rope uses.

## What v13 does

A file can now be cut into pieces, each coded the way that suits it: LZMA2 where bytes repeat, the
rope where they're unevenly spread.

- Sieve finds the cut from the bytes alone, with nothing JPEG-specific. It looks at 4 KB blocks,
  then moves each boundary in 64-byte steps to where the two ways cost least.
- It's used only where it's smaller than either way alone.
- The way reads, for example, `pieces z17728 r4121954 rope L8 W1 S1 P0 K0`. The `z` pieces go into
  the LZMA2 stream; the `r` pieces are joined and coded once by the rope.
- Nothing from outside Sieve is used: LZMA2 was already Sieve's.
- `--v12` (with `--items`) makes the older form.

## Measured here, on your photos

| photo | file | 7z | `.jpg.sieve` before | your `.7z.sieve` | **now** |
|---|---|---|---|---|---|
| 20230830_163945 | 4,139,682 | 4,127,434 | 4,122,470 | 4,112,669 | **4,111,592** |
| 20240407_153939 | 4,587,473 | 4,575,201 | 4,569,466 | 4,559,241 | **4,558,132** |

- On the first photo, the cut fell at 17,728 bytes; the metadata ends at 17,733.
- Both photos install byte for byte with `sieve install`, `sieve-install` and the oracle.
- Each took about 45 seconds with no search time limit.
- pi6.txt is unchanged at 419,096, since it has nothing for LZMA2 to take.

Your photos were only measured. Nothing of them is in the repository.

## Next, as agreed

The **"Optimise Archive Formats"** box, off by default, with a command-line flag to match. It's
written up in IDEAS §19.88 and not started yet.

## Checked here

- The full build, with no warnings.
- The unit suite: 153,210 checks, 0 failures.
- The hallway CI step, start to end. New in it: a file of 20,000 bytes of text followed by 300,000
  unevenly spread bytes.
  - It's cut within a block of where the text ends.
  - It's smaller than v12 and than `xz -9e`.
  - All three readers install it.
  - The same instructions relabelled v12 are refused.

**Docs:** SPECIFICATIONS §12.2, IDEAS §19.88, README, HANDOFF, help.
