# Changes: MP3, with packMP3's decoder made safe

One commit on top of 71c291d (the last zip, sieve-formats.zip).

## What it does

MP3s are carried as packMP3 codes them, by a new recipe, `pmp <inner size>` (format 29, v34 draft).
Five LAME MP3s came out 10.6% to 14.4% smaller, each installed byte for byte in under half a second
(results/mp3-pmp-2026-11-08.md).

## Making the decoder safe

packMP3's decoder trusted everything it read; its own comment says "no error checks needed!". Before:
264 of 300 damaged streams crashed it. Its source is now kept in `third_party/packmp3` (its last
commit, e61c119; LGPL v3, which fits Sieve's AGPL v3). Each change is marked "Sieve" in the code and
listed in `third_party/packmp3/SIEVE-CHANGES.md`:

- **The arithmetic coder:** a damaged stream now throws instead of reading outside its tables.
  Reading far past the end throws too, which ends the decode-until-end-symbol loops. A failed
  allocation throws instead of calling `exit(0)`.
- **Everything decoded is checked against what an MP3 frame may hold**, before it indexes anything:
  - every frame's side information: region tables and bounds, block types, lengths, gains and flags;
  - the header's frame count (1 to 2^20);
  - a VBR frame's bitrate and main-data size;
  - the padding and ancillary bits.
- **The packer's side:** a seek loop that counted to INT_MAX on a 3-byte file now stops.

packMP3's coding itself is unchanged: 17 MP3s (CBR and VBR, mono and stereo, 32 to 48 kHz, with and
without ID3 tags) give the same forms and come back byte for byte.

## Fuzzed (libFuzzer, clang 18, AddressSanitizer and UBSan)

| What | Runs | Crashes |
| :--- | ---: | ---: |
| packMP3's decoder, final build | 83,731 | 0 |
| packMP3's packer side (damaged MP3s in), final build | 124,928 | 0 |
| Sieve's whole-reader fuzzer, with an MP3 `.sieve` | 7,233 | 0 |
| `sieve_mp3_check`, 2,000 damaged forms | 2,000 | 0 (1,997 refused, 3 decoded) |

Three issues were found and fixed along the way, listed in the results file.

## Other files

- **Glue:** `tools/cli/mp3_glue.cpp`. packMP3 keeps its state in globals, so the glue holds one lock
  and catches packMP3's errors at the boundary.
- **Oracle:** reads the recipe and reports the MP3 "not made again by the oracle", as for `jbr`. Its
  message now names packMP3, and the CI checks on that message were updated.
- **CI step 11:** an MP3 block using `tests/mp3/tone.mp3` (3 seconds of two sine tones, made for
  Sieve) that runs `sieve_mp3_check` where the build has it.
- **New test program:** `sieve_mp3_check` (`tests/test_mp3.cpp`), built with the tests and run by
  ctest.
- **Licence:** notice added (third_party_licenses/packmp3), and SPECIFICATIONS, FORMATS,
  NEXT-VERSION and FORMATS-PLAN updated.

## Checks and caveats

- Unit suite: 154,388 checks, 0 failures. CI step 11 passes in full here (1,820 s).
- **Not built here: MSVC.** packMP3 builds as its own library with warnings off and
  `_CRT_SECURE_NO_WARNINGS`. It was originally a Windows program, but I couldn't build it with MSVC
  here; your Debug and Release builds will be its first test there.
- **Time on hostile files:** a damaged stream can still claim up to 2^20 frames, which are decoded
  before the error is found. Such an install takes seconds to minutes, but it reads and writes nothing
  outside its buffers.
