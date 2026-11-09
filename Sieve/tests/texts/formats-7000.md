# Formats: what Sieve does with each, and where it is going

Every format Sieve recognises, what each of its three methods does to it, and what has been
measured. For how a file is chosen a way, see [METHODS.md](METHODS.md); for the bytes, see
[SIEVE-FORMAT.md](SIEVE-FORMAT.md) and SPECIFICATIONS §12.2. The last section is the long-run aim:
Sieve as an engine that measures every method on every format, and picks the best for each.

## The three methods

The File Locator's **Compressed formats** boxes (`--compressed-temporary`,
`--compressed-permanent`), off unless ticked, for archives, compressed files and pictures; and, for
sound alone, the **Sound as FLAC** button (v34 draft: temporary unless changed; below, *Sound: every
format, and where it goes*). The same three methods in each:

| Method | What happens to a compressed file | What is installed |
| :--- | :--- | :--- |
| **normal** (no box) | carried as it is, packed by Sieve's ways like any file | the file, byte for byte |
| **temporary** | carried as what it holds, with a recipe; made again on installing, where Sieve can make it byte for byte | the file, byte for byte |
| **permanent** | carried as what it holds | what it holds, in its place: a `.tar.gz` gives its `.tar`, a `.7z` its folder |

Both boxes together: made again where Sieve can, else carried as what it holds. Whatever the method,
every installed file is checked against its SHA-256 before anything is written.

**Optimise Search routes temporary (0.13.78).** With Optimise Search ticked (the File Locator's
default; `--optimise-search`), temporary is taken whether its box is ticked or not, and the box is
shown greyed: every file above that Sieve can make again exactly is opened where that is clearly
smaller (by 64 bytes and 0.5% on the estimate; a JPEG where its brunsli form is smaller), else left
as it is, so nothing comes out larger, and everything installs byte for byte. Prioritise Decoding
Speed forks it: with it, what an opened file holds is carried by LZMA2 or the rope, never mixed
(quicker to make and to install; without it, a PNG's pixels are mixed as a picture, the smallest and
the slowest). Never permanent, which changes what is installed; and never a RAR's remaking, which needs the installing machine's own `rar` at a
version that gives it back: a RAR is opened only where the box is ticked with Optimise Search
unticked, and then the File Locator says so in a box, and `sieve locate` in a warning, when it
reads or saves (0.13.78: it was only here, so no one making one learned it), naming each RAR and
the rar that made it: it comes back byte for byte only where the installing machine has that rar,
and elsewhere its files are installed in its place, the install saying so.

*Measured* (6 November 2026, this container, 4 threads; every `.sieve` installed back byte for
byte; the search with no time limit, as the File Locator's default):

| Folder | Optimise Search before 0.13.78 | Optimise Search now | with Prioritise Decoding Speed too |
| :--- | ---: | ---: | ---: |
| the repository's `Images` (67 PNGs, 25 WebP; 10 MB) | 8,884,242 bytes; made in 60 s, installed in 2 s | 5,574,710 (−37.3%); 349 s, 30 s | 6,367,562 (−28.3%); 309 s, 4 s |
| the web test site (pages, pictures) | 451,120; 13 s | 382,940 (−15.1%); 1 s | 384,360; 1 s |

A PNG opened is its pixels (a 1920 by 1080 screenshot about 6 MB), which packing takes far longer
than the PNG itself: the making's cost, paid once. Two things made it slower than it need be and
were mended on the way (the same bytes either way): the rope's whole search of the pixels to decide
whether opening paid, where LZMA2's estimate already said so (21 s of one screenshot's 30), and
each PNG decoded and encoded ten times to learn it was not stb_image_write's, where its first bytes
say so.

## The formats

| Format | Read by | temporary: made again by | permanent installs | Notes |
| :--- | :--- | :--- | :--- | :--- |
| xz (`.xz`, `.txz`) | liblzma | liblzma, at the preset and check found (v14) | what it holds | one stream; several streams carried as what they hold |
| gzip (`.gz`, `.tgz`) | zlib | zlib at the level found (v17), else preflate from its skeleton (v21) | what it holds | several members carried as what they hold |
| bzip2 (`.bz2`, `.tbz2`, `.tbz`) | libbzip2 | libbzip2 (v19) | what it holds | |
| zstd (`.zst`, `.tzst`) | libzstd | libzstd 1.5.7 only (v19): another version is refused | what it holds | the version is in the recipe |
| lz4 (`.lz4`, `.tlz4`) | liblz4 | liblz4 (v20) | what it holds | |
| brotli (`.br`) | brotli | brotli (v20) | what it holds | brotli has no mark: by its name |
| lzma (`.lzma`) | liblzma | liblzma (v20) | what it holds | |
| lzip (`.lz`, `.tlz`) | Sieve | not made again | what it holds | |
| 7z (`.7z`) | the LZMA SDK | from its skeleton and its files (v21; its files carried one by one since v31) | its folder | made again only where 7-Zip packed it on one thread (`-mmt=1`) |
| zip (`.zip`, `.zipx`) | Sieve, preflate | from its skeleton and its files, deflate by preflate (v21, v31) | its folder | |
| RAR (`.rar`) | libarchive | by the machine's own `rar`, where it gives the archive back byte for byte (v22) | its folder | RAR 5; RAR 4 only as its contents; the real `rar` not yet measured |
| cab, ISO, LHA, cpio, ar (`.deb`), WARC | libarchive | not made again | its folder | |
| tar (`.tar`, or a POSIX or GNU tar by its `ustar` mark) | Sieve | from its skeleton (its headers and padding) and its files, each its own way (v31's `skm`; since 1 November 2026) | its folder (libarchive) | it compresses nothing, which is why the boxes are "Archive and compressed formats"; a program inside goes through the x86 filter |
| **PNG** | Sieve, preflate | from its pixels where stb_image_write made it (v25), else from its skeleton (v21) | v32: **the same picture re-saved at its smallest** (`pngr`), else made again exactly; v31: its pixel rows, a flaw (below) | |
| **WAV** (plain integer PCM: 8, 16, 24 or 32 bits, 1 to 8 channels; floating point of 32 or 64 bits whose values are whole 24-bit steps) | Sieve, libFLAC | v34 draft: its samples as libFLAC 1.4.3 codes them, the rest of the file as it was (`flac`); how hard libFLAC works follows the priority (Size, Balanced, Decoding speed) | as temporary, or Sound as FLAC's permanent (below) | floating-point WAVs of other values (a fraction of a step, -0), and one whose data chunk is cut short, as their bytes |
| **AIFF**, **AIFF-C** (`.aif`, `.aiff`, `.aifc`: plain samples, 8 to 32 bits; AIFF-C uncompressed, `NONE`, `twos` or `sowt`; `fl32` and `fl64` as a float WAV) | Sieve, libFLAC | v34 draft: as WAV, by the same recipe (`flac`), its skeleton an AIFF's | as temporary | compressed AIFF-C, floats of other values, and one whose COMM comes after its SSND, as their bytes |
| **RF64**, **BW64** (WAVs past 4 GiB), **Wave64** (`.w64`), Sun's **`.au`** (`.snd`), Apple's **CAF** (`.caf`): plain samples, 8 to 32 bits, or floats as a float WAV | Sieve, libF