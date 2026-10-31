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
| tar (`.tar`) | Sieve | (not compressed: carried as it is) | (as it is) | `sieve-install` extracts it |
| **PNG** | Sieve, preflate | from its pixels where stb_image_write made it (v25), else from its skeleton (v21) | v32's draft: **the same picture re-saved at its smallest** (`pngr`), else made again exactly; v31: its pixel rows, a flaw (below) | |
| **JPEG** | brunsli | v32's draft: from its brunsli form (`jbr`) | v32's draft: **the same coefficients with tables made for them**, from its brunsli form, else made again exactly | normal: as it is |

`sieve-install` extracts every format above (the right-click menu's Extract), one layer at a time.

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

## Decided and built (31 October 2026, v32's draft)

Edward confirmed the design ("go ahead with the png/jpeg design yes"); built behind `--v32-draft`:

- **PNG under permanent** no longer installs as its headerless pixel rows, a file no viewer opens
  (v31: `tests/example_book_cover.png`, 859 bytes, as `cover`, 76,800 bytes). It installs as the
  same picture re-saved by Sieve at its smallest (`pngr`), under its own name. Without the draft,
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
