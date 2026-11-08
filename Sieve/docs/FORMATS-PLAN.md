# Formats still to add: the plan after v34's sound and pictures

Edward (8 November 2026): "work through all of those planned steps and formats then when you are done
make a plan for any remaining formats we can add afterwards". Done in the v34 draft by then: WAV,
AIFF and AIFF-C, RF64, Wave64, Sun `.au` and CAF through FLAC (results/flac-way-2026-11-08.md); Sound as
FLAC, FLAC files re-coded; FLAC on every core; plain pictures' shapes from their headers
(results/plain-pictures-2026-11-08.md). What is below is not built. Each is to be measured before it
is built, as these were: no gain is claimed here that was not measured.

Three ways a format can be taken, as now:
- **temporary**: carried as what it holds and made again byte for byte on installing (only where
  Sieve's maker gives the file back exactly; checked before it is chosen);
- **permanent** (or Sound as FLAC): installed as the same content in other bytes, asked for by name;
- **the packer's choice alone**: no format change, a better way chosen for the file's bytes.

## In order of what each could be worth

1. **Built 8 November 2026** (results/pdf-2026-11-08.md). **PDF's deflate streams (temporary).** A PDF's pages, fonts and pictures are mostly `FlateDecode`
   streams. preflate (already in Sieve, for zip and gzip) can carry each as what it holds and make the
   deflate again exactly, as the zip route does; the PDF itself as a skeleton. Sieve carries PDFs as
   their bytes today. To measure: the share of a PDF in deflate streams, and what mixing makes of
   them opened, on the repository's own PDF (wotw-man-english-v1.pdf in WoWRevived) and a few more.
2. **Built 8 November 2026: floats** (results/float-sound-2026-11-08.md). **Measured: mu-law and A-law
   not worth it** (Sieve's mixing on the 8-bit codes made 1,026,561 and 162,491 bytes of two files
   where FLAC on the widened samples made 1,614,712 and 236,962).
   **Sound in companded or floating-point samples (temporary).** A WAV or `.au` in mu-law or A-law (8
   bits a sample, formats 7 and 6; `.au` encodings 1 and 27): each code maps to one linear value and
   back, so the samples can be widened to 16 bits, coded by FLAC, and narrowed again exactly. A float
   WAV or AIFF-C (`fl32`) whose every value is a whole number over 2^23 (common where the float came
   from 24-bit integers): carried as those integers through FLAC, else as it is. To measure: FLAC on
   the widened samples against the rope and mixing on the bytes; how many float files are so.
3. **Built 8 November 2026** (results/tiff-compressed-2026-11-08.md). **TIFF compressed (temporary).** Deflate (8, 32946) through preflate as above; LZW (5) made again as
   the GIF route makes GIFs, where libtiff's coder gives it back; PackBits (32773) where its runs are
   the greedy ones. Then each picture's pixels mixed with its header's shape. To measure: TIFFs from
   libtiff, ImageMagick and Photoshop, which of them are given back.
3a. **Built 8 November 2026: zlib streams inside any file, PCX; measured and not built: PSD,
   run-length TGA** (results/zlib-pcx-2026-11-08.md).
4. **Built 8 November 2026, after packMP3's decoder was made safe** (results/mp3-pmp-2026-11-08.md). **MP3 (temporary).** Its Huffman-coded frames can be recoded by a better model and made again
   exactly (packMP3 and similar tools do this). Before anything: a coder whose licence fits Sieve's,
   and whether it gives back every MP3 exactly. To measure on MP3s from LAME and others.
5. **Built 8 November 2026: tags and 3 to 8 channels** (WavPack, Monkey's Audio, TTA and ALAC still
   to do). **Sound as FLAC, further.** A WAV's LIST INFO tags and an AIFF's NAME and ANNO chunks written as
   FLAC's Vorbis comments, so a repack keeps them; more than 2 channels where the WAV's channel mask
   is FLAC's own order; WavPack, Monkey's Audio, TTA and ALAC files decoded and installed as FLAC (their
   decoders' licences to check: WavPack's is BSD).
6. **Run-length pictures (temporary).** TGA RLE, BMP RLE8 and RLE4, PCX: their runs undone, made again
   where the encoder's choice of runs is the plain greedy one. To measure: which encoders' files are
   given back (ffmpeg's TGA, GIMP's, Paint's BMP).
7. **Built 8 November 2026: uncompressed DDS's shape; BC1 measured as carried well already**
   (results/mp3-dds-ico-2026-11-08.md). **Game and texture formats.** DDS (uncompressed: its header's shape for mixing, as above; BC1 to
   BC7 blocks: their endpoints and indices apart), WAD and PAK lumps listed as files (carried each its
   own way, as a tar's members are), and WoWRevived's own DAT where its layout is known.
8. **Measured 8 November 2026: ICO already carried well; AVI and MOV not measured.** **Containers holding these.** An AVI or MOV holding PCM sound: its sound through FLAC and the rest
   as it is (only where the sound is plain samples; WoWRevived's film packets were not); ICO and CUR,
   which hold PNGs or BMPs, opened as those.

## Not planned, and why

- **Lossy sound and video (AAC, Vorbis, Opus, H.264, HEVC, AV1)**: already compressed to near their
  information; no exact recoder is known to Sieve that would pay for its code.
- **JPEG XL, AVIF, HEIC, WebP lossy**: the same; JPEG already goes through brunsli (`jbr`).
- **Encrypted or signed files** (PDF with encryption, signed installers): their bytes must stay as
  they are.

## How each is to be taken on

As the FLAC way was: a prototype measured on real files first (and a control); then the packer's side,
the reader's, the oracle's (written from the specification alone), a CI block with awkward files and
ones to be left alone, a fuzz of the reader's new part under AddressSanitizer and UBSan, the
specification, FORMATS.md, NEXT-VERSION.md, and a results file.
