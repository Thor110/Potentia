# Changes: the format list worked through (v34 draft)

Five commits on top of 24a9871 (your "sieve metrics"). Each format was measured before it was built.
Every file listed below was installed byte for byte.

## de81c00: floating-point sound, compressed TIFFs

**Float sound.** Float WAV, AIFF-C fl32/fl64, .au and CAF whose every sample is a whole 24-bit step
now go through FLAC at 24 bits, by the existing `flac` recipe. The reader turns each sample back into
its float by integer arithmetic alone, so every machine gives the same bits. The rule was checked
against IEEE floats for all 16.8 million 32-bit values. On twelve files (two recordings in six
layouts): 21% to 32% smaller, made 13 to 29 times faster (results/float-sound-2026-11-08.md).

**Compressed TIFFs.** A new recipe, `tiff`. Deflate strips go through preflate. LZW and PackBits
strips are made again by libtiff's own coders, written for Sieve from libtiff 4.7.0's source; its
licence is permissive, and noted in third_party_licenses/libtiff. The horizontal predictor is undone,
so mixing sees the pixels at the image's width. On 18 ImageMagick TIFFs: 36.9% smaller together.
One PackBits scan came out 0.5% larger (results/tiff-compressed-2026-11-08.md).

## c85866a: zlib streams inside any file, PCX

**zlib streams.** Any file that no other route opens is searched for zlib streams. Each is carried
as what it holds, through preflate, by the existing `sk` recipe, so every reader since v21 can
install it. Your git pack: 27.8% smaller. Two WOFF fonts: 12.7% and 22.2% smaller.

**PCX.** A new recipe, `pcx`: the pixels with their planes interleaved, plus the header. ImageMagick
and Pillow both use the PCX specification's own coder. Five of six PCXs 16% to 66% smaller; the
sixth wasn't worth opening.

**Measured, not built:**
- PSD: Photoshop's and ImageMagick's PackBits coders are their own, not libtiff's.
- Run-length TGA: keeping the packet heads would gain 3% to 6% on photos but lose 17% to 20% on
  screenshots.

(results/zlib-pcx-2026-11-08.md)

## f7fe4e4: Sound as FLAC keeps tags

A WAV's LIST INFO tags and an AIFF's NAME, AUTH, ANNO and (c) chunks are written into the `.flac` as
Vorbis comments (TITLE, ARTIST, and so on, in UTF-8). WAVs of 3 to 8 channels are installed as
`.flac` too, where their channel mask is FLAC's own order.

## 5c6d7ea: MP3 measured, DDS, ICO

**MP3: not built.** The licence fits: packMP3 is LGPL v3. It made five MP3s 10.6% to 14.4% smaller
and gave each back exactly. But its decoder crashed on 264 of 300 fuzzed streams under
AddressSanitizer (buffer overflows and null pointers). In the reader, that would let a crafted
`.sieve` corrupt the installer's memory. Your call whether it's worth making packMP3's decoder safe
first (results/mp3-dds-ico-2026-11-08.md).

**DDS.** An uncompressed DDS's width is now read from its header for mixing: 8.5% smaller on one
texture. BC1 textures are already carried well.

**ICO.** Already carried well.

## 71c291d: a crash in preflate fixed (in the reader since v21)

Fuzzing the reader with the zlib test file found one crash. It isn't in the new code: it has been
there since v21, for zips and gzips. A damaged `.sieve` could make preflate read about 4 GiB past its
buffer while installing: its token predictor trusted a corrected length (0 in the cases found) and a
stored block's length taken straight from the instructions.

A second checked patch to the fetched preflate, beside the one from 7 November, makes each such case
fail as a damaged stream does, so the install refuses it. Your git pack (2,737 streams) and two PDFs
pack to the same sizes as before and install byte for byte (results/fuzz-2026-11-08.md). Your
Windows/MSVC build will apply the patch when CMake next configures.

## Checks

- **Unit suite:** 154,388 checks, 0 failures.
- **CI step 11:** new blocks for float sound, TIFF, zlib streams, PCX and Sound as FLAC tags. The
  whole step passes here with all of them, before and after the preflate patch (1,691 s and 1,673 s).
- **Fuzzing,** under AddressSanitizer and UBSan, in three rounds:
  - TIFF and float seeds: 8,680 runs, no findings.
  - PCX, zlib and TIFF seeds: 7,964 runs, 6 findings with one cause (the preflate crash above).
  - The same seeds after the fix: 5,501 runs, no findings.
- **Not done here:** CI on Windows and macOS, or the benchmark.

Pillow is now installed in CI step 11 (pip_add "pillow==12.0.0") to write the test TIFFs and PCXs.
