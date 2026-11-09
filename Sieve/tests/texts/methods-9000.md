# Every way Sieve tries a file

This is what happens when the File Locator reads a file or a folder, in the order it happens:
every way tried, when it is not tried, and how the smallest is chosen. **Keep a log** writes a line
for each step as it runs. The log lines are quoted here as they read, so a log can be followed from
this page. For the format itself, see [SIEVE-FORMAT.md](SIEVE-FORMAT.md); for the commands,
[MANUAL.md](MANUAL.md).

Nothing here is chosen by a rule of thumb alone. Where two ways compete, both are measured on the
file's own bytes and the smaller is kept, and the instructions always install back to the
original, byte for byte. Packing reads its own result back to prove that before it is used (§6).

## 1. Before packing: what the file is set beside

| Step | What it measures | Log |
| :--- | :--- | :--- |
| An estimate (files of 16 MiB or more) | the same work timed on 1 MiB and 2 MiB taken from across the largest file, drawn out to the whole | (the status line) |
| Its address | the file as a number, its place on the binary line: as raw bytes and in hex | `reading it, its address and its SHA-256` |
| zip | deflate at level 9, each file on its own, without the zip container | `what zip makes of it (deflate, level 9)` |
| 7z | LZMA2 at preset 9 extreme, everything as one stream, without the 7z container | `what 7z makes of it (LZMA2, preset 9 extreme)` |

## 2. Packing: each file's way (`packing its Sieve instructions`)

Each file is given the first of these that applies; the rest go on to §3.

| Way | When | Log |
| :--- | :--- | :--- |
| An **item of Sieve's lines** (pages, image, audio, video): carried as its place on the line (v5) | the file is exactly an item of a line, and its place is shorter than the file (v6) | `not searched (an item of Sieve's lines, carried as its place on the line)` |
| A **program** through the **x86 filter**, in an x86 stream of its own | an executable (EXE or ELF) | `not searched (carried as it is; a program, through the x86 filter)` |
| A **mask of another file's lines** | a text made from another text's lines (a list drawn from it), where the mask packs smaller than the text | `not searched (made from another file's lines (NAME), carried as a mask of them)` |
| **Carried as it is**, in the lzma2 stream | under 512 bytes, or a copy of another file in the folder (the stream carries a copy for almost nothing) | `not searched (under 512 bytes)`, `not searched (a copy of another file here ...)` |
| **Compressed formats** (only with the boxes ticked): an archive or compressed file made again exactly from what it holds, or carried as what it holds | see below | the archive's notes on installing |

**Compressed formats.** With **temporary** ticked, an archive is carried as its contents and made
again exactly on installing, where Sieve can and that is smaller. With **permanent** ticked, it is
carried as its contents and installed as them. The kinds:

- xz, bzip2, zstd, lz4, brotli and lzma: made again by their own encoders at the settings found;
- gzip and zip: made again by preflate from their skeleton;
- 7z and zip: made again from their skeleton and the data they hold;
- RAR 5: made again by the machine's own `rar`, where that gives it back byte for byte;
- PNG: made again from its skeleton (with permanent, re-saved at its smallest: §2a);
- lzip: carried as its contents.

Every format, and what each method does to it, is in [FORMATS.md](FORMATS.md).

Before the costly search for the levels that make an archive's blocks again, its contents are
weighed cheaply, and the search is not started where they would come out no smaller.

## 2a. Files alike, and pictures recoded (v32)

From v32 (`--v31` and older make none of this), a file over 8 MiB carried as it is may be **written as its differences from another file of the folder** (the `delta` way, delta-v1):

- **Finding the pair:** each such file is cut into blocks where its content says (about every 4
  KiB), one fingerprint a block. A file whose blocks are mostly another's is paired with it.
- **Writing it:** the file is written as copies of at least 64 bytes from the other, plus new bytes.
  The new bytes go in the lzma2 stream, so they are packed as any file's are.
- **Kept** where the copies cover at least half of it. It is decided before the search, so such a
  file is not searched.
- **Not used below 8 MiB:** there, the groups mixed together and LZMA2's window find what files
  share, and better.

Log: `file N of M: NAME, S bytes: as its differences from OTHER: C copies covering B bytes, F new
bytes, a script of L bytes in the lzma2 stream`, then `not searched (carried as its differences from
OTHER (above))`.

PNGs and JPEGs are packed under the **Archive and compressed formats** boxes (once Compressed
formats), as the other compressed formats and archives are, a tar among them ([FORMATS.md](FORMATS.md)):

| | no box | temporary | permanent |
| :--- | :--- | :--- | :--- |
| **JPEG** | as it is | its brunsli form, made again byte for byte (`jbr`) | the same coefficients with tables made for them, smaller, as its brunsli form; else as temporary |
| **PNG** | as it is | made again exactly (v25, v21), its rows carried unfiltered where it is v21's (`skr`) | re-saved at its smallest (`pngr`), the same picture; else made again exactly; else as it is |
| **tar** | as it is | made again exactly from its headers, its files carried each its own way (`skm`) | its files, where that is smaller |

With both boxes, made again exactly first.

- **A PNG's rows carried unfiltered** (`skr`): PNG's row filters are sums and differences modulo
  256, so with each row's filter byte kept they are undone for carrying and done again on
  installing, exactly; preflate's corrections are the same. The rows unfiltered come first, so they
  are mixed as the picture they are, then the filter bytes and the skeleton. Measured on the five
  PNGs here that other programs wrote, all 23% to 40% smaller as a `.sieve` (FORMATS.md).
- **A PNG's pixels mixed as their own shape:** mixing weighs the width and bytes a pixel a PNG's
  recipe gives (`stb` or `skr`) beside the shapes it weighed before. A screenshot whose pixels the
  rope read as three bytes wide: 16,058 bytes to 12,772; the 18 hallway screenshots together
  245,100 to 211,937.
- **A JPEG's brunsli form:** Google's brunsli recodes the JPEG's own coded values by a model made
  for them. It's kept where brunsli gives the file back byte for byte (checked before it is kept)
  and the form is smaller. Measured: 15.25% to 21.91% off ten original JPEGs.
- **A JPEG's tables (permanent):** Huffman tables made for its own values (Annex K, as `jpegtran
  -optimize`), for a sequential JPEG. Kept where it reads back to the same coefficients and is
  smaller. Measured: Vitruvian.jpg 207,500 bytes to 203,455; JPEGs already optimised are left as
  they are.
- **A PNG re-saved (permanent):** every chunk as it was but its pixels', which are written again as
  one IDAT. The rows are filtered six ways (each filter alone, or each row its own) and deflated by
  zlib at level 9 with two strategies, and the smallest is kept. The `.sieve` carries the rows
  unfiltered. Kept where it is smaller than the PNG. Measured: smaller every time on twelve of the
  repository's PNGs, the same pixels (hallway-door 46,647 bytes to 20,053).
- **Not searched:** a JPEG's form is coded already. A PNG's rows are searched as any file is.

Log: `file N of M: NAME, a JPEG of S bytes, as its brunsli form of F (made again byte for byte)`, or
with permanent `..., installed with tables made for its values, N bytes, the same picture, as its
brunsli form of F`; then `not searched (a JPEG carried as its brunsli form (above), coded already)`.
For a PNG: `NAME, a PNG of S bytes, installed re-saved at its smallest, R bytes, the same picture
(RECIPE)`.

## 3. The search, for every other file (`file N of M: NAME, the rope and pieces`)

Each file left is tried every one of these ways on its own. Several files are searched at once, as
many as the threads and memory allow, and the result is the same at any thread count.

1. **LZMA2 alone** (estimated at preset 6, quickly), the baseline.
2. **The rope** (rope-wave): the file's bytes coded under a wave pulled tight to them. The search
   tries wave after wave (lengths, widths, steps, phases, columns, lines, groups, the line above)
   for the time given, or with no limit until done. With **Try every way**, its shortcuts are off:
   the early stops on text and on bytes already packed.
   - Log: `the rope's search: N ways tried, best so far B bytes of S (PARAMETERS)`, marked `first`,
     then `new best` each time it improves.
   - Not searched for a text whose start the rope cannot win (2 November 2026): a file of 64 KiB or
     more whose first 256 KiB are 95% printable is coded there by the rope's line wave alone
     (`C2 G1 A1`, its best on text), and where that comes to 1.25 times LZMA2's estimate of the same
     or 