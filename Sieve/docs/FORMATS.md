# Formats: what Sieve does with each, and where it is going

Every format Sieve recognises, what each of its three methods does to it, and what has been
measured. For how a file is chosen a way, see [METHODS.md](METHODS.md); for the bytes, see
[SIEVE-FORMAT.md](SIEVE-FORMAT.md) and SPECIFICATIONS §12.2. The last section is the long-run aim:
Sieve as an engine that measures every method on every format, and picks the best for each.

## The three methods

The File Locator's **Compressed formats** boxes (`--compressed-temporary`,
`--compressed-permanent`), off unless ticked:

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
| **WAV** (plain integer PCM: 8, 16, 24 or 32 bits, 1 to 8 channels) | Sieve, libFLAC | v34 draft: its samples as libFLAC 1.4.3 codes them, the rest of the file as it was (`flac`); how hard libFLAC works follows the priority (Size, Balanced, Decoding speed) | as temporary (a repack to `.flac` files is not yet built) | floating-point WAVs, and one whose data chunk is cut short, as their bytes |
| **AIFF**, **AIFF-C** (`.aif`, `.aiff`, `.aifc`: plain samples, 8 to 32 bits; AIFF-C uncompressed, `NONE`, `twos` or `sowt`) | Sieve, libFLAC | v34 draft: as WAV, by the same recipe (`flac`), its skeleton an AIFF's | as temporary | floating-point or compressed AIFF-C, and one whose COMM comes after its SSND, as their bytes |
| **RF64**, **BW64** (WAVs past 4 GiB), **Wave64** (`.w64`), Sun's **`.au`** (`.snd`), Apple's **CAF** (`.caf`): plain samples, 8 to 32 bits | Sieve, libFLAC | v34 draft: as WAV, by the same recipe (`flac`); an `.au`'s or a CAF's samples may run to the end of the file, their count the FLAC stream's | as temporary | floating-point or compressed ones (an `.au` in mu-law, a CAF in AAC), as their bytes |
| **FLAC** (`.flac`) | libFLAC | as its bytes (a FLAC is coded already) | with **Sound as FLAC** (below): re-coded at the priority's effort where 0.5% smaller, its tags, pictures and cue sheet kept, its padding and seek table dropped | Ogg FLAC, or a FLAC after an ID3 tag, as its bytes |

**Sound as FLAC** (v34 draft; `--sound-to-flac`, the File Locator's box W, `[locator] sound_to_flac`; off unless ticked, as it changes what is installed): a WAV, AIFF, RF64, Wave64, `.au` or CAF of 1 or 2 channels, up to 24 bits, at a rate FLAC's streamable subset holds, installed as `name.flac`, the same samples, which any player opens; its other chunks (a WAV's LIST tags) are not kept. Where `name.flac` is already taken (a namesake in the folder), the file is carried as temporary carries it. A FLAC file is re-coded as above. Packing warns, naming each file.
| **JPEG** | brunsli | v32: from its brunsli form (`jbr`) | v32: **the same coefficients with tables made for them**, from its brunsli form, else made again exactly | normal: as it is |

`sieve-install` extracts every format above (the right-click menu's Extract), one layer at a time.

## Pictures: every format, and what each part of Sieve does with it

Edward, 6 November 2026: the picture formats listed as the compressed ones are above. Sieve meets
pictures in four places: packing them into a `.sieve` (above: PNG and JPEG have ways of their own
under the boxes), the image line (`sieve warp --line image --file`, J on the binary line, the
setup menu's pictures), the picture viewer and Explorer's thumbnails (`sieve-view`,
`sieve-thumbs.dll`), and the vault (pictures matched by PDQ, near copies too). Decoding is
stb_image's, built with every format it has but HDR; for the image line, what stb_image does not
read goes to ffmpeg, where one is found.

| Format | Packed into a `.sieve` | Image line | Viewer and thumbnails | Vault |
| :--- | :--- | :--- | :--- | :--- |
| **PNG** | temporary: made again from its pixels or its skeleton, its rows carried unfiltered, its pixels mixed as a picture of its own shape, as its palette where it has 256 colours or fewer; permanent: the same picture re-saved at its smallest (`pngr`); no box: as it is | yes | yes; Export saves pictures as PNG | yes |
| **JPEG** (`.jpg`, `.jpeg`, `.jpe`, `.jfif`) | temporary: its brunsli form, made again exactly (`jbr`, 1 KiB and over); permanent: the same coefficients with tables made for them; no box: as it is | yes, progressive too | yes; one too large for the memory free shown at an eighth, from its blocks' averages | yes |
| **GIF** | as its bytes; with the v34 draft (`--v34-draft`), temporary: its frames' colour indices, mixed as a picture as wide as its frames, made again exactly where its LZW was coded as compress codes it (gifenc's, Pillow's; `gif`), else as its bytes | every frame (more than one: the video line) | played, each frame for its own delay, where its frames fit in half the memory free (else its first, and why); P pauses, comma and full stop step; thumbnails its first frame | every frame |
| **BMP** | as its bytes; uncompressed pixels may be mixed as a picture where a shape guessed from them is at least 2% smaller on the file's start; with the v34 draft, its header's shape too (and its rows as one byte a pixel), where its rows need no padding (results/plain-pictures-2026-11-08.md) | yes | yes | yes |
| **TGA** | as BMP (an uncompressed one's header read; RLE as its bytes) | yes | yes | yes |
| **PNM** (`.ppm`, `.pgm`, `.pnm`) | as BMP (a binary one's header read) | yes | not offered (by name; the viewer opens the six above) | yes |
| **PSD** (its flattened picture), **PIC** (Softimage) | as their bytes | yes | not offered | yes |
| **WebP, TIFF, AVIF and others** | as their bytes | through ffmpeg, where one is found | not offered | not by PDQ |
| **`.simage`** (Sieve instructions holding only pictures) | is itself a `.sieve` | its pictures, as any `.sieve`'s files | yes: every picture in it, and its first as the thumbnail | its pictures, once installed |

A picture made of noise (a camera's grain, a compressed film's frames) has nothing for any of these
to take: under every box it is carried as its bytes, which LZMA2 and mixing weigh like any file.

## What has been measured

- **7z, temporary (v31):** the source's 7z, without the anchors folder, 8,005,603 bytes to
  6,189,674, given back byte for byte.
- **PNG** (twelve of the repository's PNGs, NEXT-VERSION §4a, 4):
  - normal against temporary, for example hallway-door 33,863 bytes against 7,882, and the long
    screenshots 121,905 against 45,666;
  - permanent saves nothing in the `.sieve` over temporary (2 to 22 bytes more);
  - a PNG re-saved at its smallest was smaller than its original every time, pixel for pixel the
    same: hallway-door 46,647 to 20,053.
- **JPEG** (NEXT-VERSION §4a, 3):
  - brunsli: 15.25% to 21.91% off ten original JPEGs, every one given back byte for byte; JPEG XL's
    `cjxl` 13.93% to 20.14% on the same, and ahead only on two quality-90 re-saves;
  - the same coefficients with optimised Huffman tables: −2.10% and −2.20% on Edward's phone photos,
    about nothing on JPEGs already optimised.

## Decided and built (31 October 2026, v32)

Edward confirmed the design ("go ahead with the png/jpeg design yes"); built behind `--v32-draft`,
and since v32 was frozen (31 October 2026) the default:

- **PNG under permanent** no longer installs as its headerless pixel rows, a file no viewer opens
  (v31: `tests/example_book_cover.png`, 859 bytes, as `cover`, 76,800 bytes). It installs as the
  same picture re-saved by Sieve at its smallest (`pngr`), under its own name. With `--v31`,
  v31's packer is unchanged, the flaw with it.
- **JPEG under the boxes:** normal as it is; temporary its brunsli form, made again exactly;
  permanent the same coefficients with optimised tables, from its brunsli form.
- So for pictures, permanent means "the same picture, its smallest usable file" rather than "what
  the file holds".
- *Measured* (each PNG alone; every installed picture the same pixels by Pillow, and the same
  bytes from Sieve, sieve-install and the oracle):

  | PNG | original | installed | `.sieve` temporary | `.sieve` permanent |
  | :--- | ---: | ---: | ---: | ---: |
  | example_book_cover | 859 | 202 | 151 | 183 |
  | icon16 | 1,258 | 936 | 1,322 | 947 |
  | icon128 | 27,749 | 17,537 | 19,776 | 11,376 |
  | beautiful-screens | 16,289 | 14,824 | 7,218 | 5,517 |
  | viewer-picture | 29,899 | 5,509 | 1,733 | 1,768 |
  | font-test-french | 31,413 | 6,150 | 3,789 | 3,054 |
  | gallery-tiles | 38,986 | 28,562 | 26,385 | 17,950 |
  | hallway-door | 46,647 | 20,053 | 7,883 | 7,914 |
  | hallway-compact | 53,569 | 24,828 | 16,059 | 16,102 |
  | m1_sieve_scowl60 | 146,344 | 105,324 | 84,641 | 48,136 |
  | long-pages-64MB-cache | 128,597 | 58,841 | 45,667 | 45,704 |
  | long-pages-1024MB-cache | 129,272 | 59,298 | 45,972 | 46,009 |

  - **Installed:** every PNG came out smaller, and smaller than Pillow's re-save at zlib 9 with
    optimisation on all but hallway-door (the same 20,053). For example icon128 is 17,537 against
    Pillow's 27,649.
  - **The `.sieve`:** under permanent it is often the smaller of the two, since rows carried
    unfiltered pack better than rows as PNG filtered them (temporary's). The scowl is 48,136 against
    84,641, and icon128 11,376 against 19,776. Where it is larger, it is by 31 to 43 bytes, the
    recipe's.
  - **JPEGs** (the repository's seven): permanent installed Vitruvian.jpg in 203,455 bytes, from
    207,500 (−1.95%). The other six are optimised already, so they come back exact. The `.sieve`
    is 555,738 bytes against temporary's 555,677, and 659,333 with no box (v31's).
  - **JPEGs that Pillow wrote** with its standard tables, from the hallway door at quality 85 to 92,
    made for this test:
    - baseline 172,212 bytes to 156,746;
    - greyscale 148,510 to 132,517;
    - with restart markers 172,470 to 156,855;
    - 4:4:4 at quality 92 346,331 to 310,963.

    Every one has the same pixels. A progressive JPEG is left exact: its scans code their values
    otherwise, and the optimiser does not yet handle them.
    - *A progressive JPEG's tables, measured 31 October:* libjpeg makes a progressive JPEG's tables
      for its values every time, so optimising them again gains nothing. Pillow's progressive JPEGs
      of four pictures at quality 75, 85 and 92 were the same size with `optimize` on and off.
    - What would pay is installing a baseline JPEG as progressive. At quality 85, the hallway door
      goes from 156,746 bytes (baseline, optimised) to 147,587, the hallway compact from 163,440 to
      153,821, and the scowl from 102,366 to 95,883, about 6% off each. icon128 went from 7,898 to
      7,913, so it would have to be weighed. It needs a progressive JPEG writer of Sieve's own;
      not built.

**A PNG's rows carried unfiltered, for temporary** (`skr`, 31 October 2026, v32; the default since v32 was frozen). Under
temporary, a PNG that preflate makes again (`sk`) carried its rows as PNG filtered them. Its rows
are now carried unfiltered, with their filter bytes, and filtered again on installing. Each PNG
filter is a sum or difference modulo 256, so the original comes back byte for byte, and preflate's
corrections are the same. The five PNGs here that other programs wrote (the `.sieve`, bytes; each
back byte for byte):

| PNG | `sk` (v25) | `skr` | |
| :--- | ---: | ---: | ---: |
| icon16 | 1,321 (as it is) | 1,014 | −23.2% |
| icon32 | 2,757 | 1,911 | −30.7% |
| icon48 | 4,446 | 3,215 | −27.7% |
| icon128 | 19,775 | 11,863 | −40.0% |
| m1_sieve_scowl60 | 84,640 | 59,895 | −29.2% |

PNGs that Sieve wrote (stb) are carried as their pixels already. Since v32, mixing weighs
those pixels as the picture the recipe says they are, beside the shapes it guessed before.
hallway-compact, whose pixels the rope had read as three bytes wide, went from 16,058 bytes to
12,772 (−20.5%). The repository's 18 hallway screenshots (docs/images) as one `.sieve`: 245,100
bytes to 211,937 (−13.5%), 18 of them mixed as pictures where 11 were, and given back the same.

## The long run: an optimisation and benchmark engine

Edward (30 October 2026): "in the long run the compression engine can also be tuned to be an
optimisation engine and benchmark engine for checking stats across the board with different
compression methods / mechanisms / mixes, then we can look at defaults", aiming "the best methods
at the formats they work best with", perhaps with a **"best defaults"** box.

**Why it is worth building.** Sieve already holds most of it:

- **It already measures every method on every file:** Keep a log writes what LZMA2, the rope, the
  pieces and mixing each came to for each file, and the weighing sets every way side by side
  (METHODS.md). An engine that gathers these across many files is a step from there, not a new
  thing.
- **Its defaults would be chosen by evidence, not by guess.** Each format's best method (temporary
  or permanent, mixing or not, which encoder for which kind of picture) would be read off measured
  tables, then a "best defaults" box applies them. The PNG table above already shows how far the
  right choice moves a result: one picture's `.sieve` from 33,863 bytes to 7,882.
- **It benchmarks against the rest on the same files:** zip, 7z, xz, brunsli, JPEG XL, beside
  Sieve's own ways, in one run, every figure reproducible. That is what a presentation and the
  public benchmarks (enwik, Silesia) ask for.
- **It catches regressions:** a change that makes any format worse shows at once, against the
  tables kept.
- **The trade-offs stay visible:** size, making time and decoding time per method and format, so a
  default for size and a default for decoding speed can both be offered honestly.

**A benchmark script and test files, for the future** (Edward, 30 October 2026: "setting up a more in
depth benchmark script and test files would be worthwhile in the future"). Its first piece is begun:
`tools/rope_designs.py`, designed test files for the rope, each made from a seed and measured
against `xz` and Sieve's ways in one run (IDEAS §19.108).

**What it would need** (nothing built yet):
1. A corpus per format (public files, each listed with its source), and a run that packs each under
   every method and writes the results as a table, as `make_release` writes its metrics.
2. Per-format results kept in the repository (as `results/` is), so defaults follow the data.
3. The "best defaults" box (and option), choosing each format's method from those results.
