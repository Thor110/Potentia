# What Sieve does: its features, each with what was measured

Sieve packs a file or a folder into one `.sieve` (its instructions), and gives it back byte for
byte. Every file it reads is weighed every way it has, and the smallest way is kept: no one method
is applied to everything. This page lists those ways and the tools around them, each with the
figure measured for it and where the measurement is. How a file is weighed, step by step, is in
[METHODS.md](METHODS.md); the format, in [SIEVE-FORMAT.md](SIEVE-FORMAT.md); the full tables, in
[Proof/STATISTICS.md](../../Proof/STATISTICS.md). Figures are as measured on the day given.

## How a folder is packed

1. Every file is read once, its SHA-256 worked out, and the folder's listing made.
2. **Copies** (the same bytes as another file anywhere in the folder) are found first and carried
   as copies, so nothing after looks at them again.
3. **Compressed formats** (archives, PNGs, JPEGs) are opened, where the Compressed formats boxes
   ask for it, and what they hold is weighed in their place (below).
4. **Repeating bytes and patterns** are found next, and carried as counts.
5. Each other file is weighed: LZMA2 alone; the rope (Sieve's own wave coder); the file cut into
   pieces, LZMA2 and the rope by turns; and mixing (a context-mixing model). The smallest is kept,
   by a margin.
6. Files that share content are tried together: mixed in groups, or carried as differences.
7. The streams are packed, LZMA2's settings tried on each, and the whole read back and checked
   before it is written.

## The ways, by kind of data

**Text.** Mixing, with an English word list whose words are counted by how common they are (2.6%
smaller on Don Quixote's start than without the list). Texts that share words are mixed together
in groups: 34 MB of the repository's files that compress, 5,740,563 bytes mixed one by one against
4,884,492 in groups of 8 MiB (27 October 2026; SPECIFICATIONS §12.2, v30). The rope's search is skipped on a text whose start it cannot win: 12.9 MB of
the repository's texts packed in 43.2 s instead of 97.7 s, the same bytes
(`results/weighing-order-2026-11-02.md`).

**Web pages** (HTML, CSS, JavaScript; v33). Mixed with the model first primed on a 64 KiB dictionary
learnt from MDN's public-domain examples, which ships with Sieve; pages under 4 KiB are mixed too.
On pages it never saw, each page's whole `.sieve` against brotli: 258 pages saved in 2014, 8.8% under
brotli (smaller on 253); Edward's site's 11 pages, 10.3% under (smaller on all 11)
(`results/web-dictionary-2026-11-03.md`).

**Tables of digits and numbers.** The rope, which reads a file as lines and columns: a million
digits of pi, 1,111,103 bytes, in 417,049, 1,808 bytes above the 415,241 that a million random
digits need (7z: 512,745; STATISTICS).

**Programs** (Windows and Linux, x86 and x86-64). Their own stream through the x86 filter, as 7-Zip
does; and from v33, mixed whole through that filter where that is smaller: 16 programs of 98 KB to
11.8 MB came out 3.0% to 15.1% under LZMA2 with the filter
(`results/programs-mixed-2026-11-02.md`).

**Pictures as raw pixels.** Mixing reads them as pictures, rows and pixels, at the shape the rope
finds or one guessed from the bytes. From v33 a picture of 256 colours or fewer is mixed as its
palette (one byte a pixel, then the colours): the hallway's 17 screenshots, 185,088 bytes as
pictures mixed, 146,999 as palettes (`results/palette-2026-11-02.md`).

**JPEGs** (with a Compressed formats box). Recoded losslessly by brunsli and made again byte for
byte on installing: 15.25% to 21.91% smaller on ten original photos; a 3,844,213-byte photograph
in 3,085,201 (FORMATS.md, STATISTICS). With *permanent*, the same picture's coefficients get
tables made for them first (up to 2.2% more on Edward's phone photos: 0.01%, 2.10% and 2.20% on three).

**PNGs** (with a box). Made again from their pixels where stb_image_write wrote them, else from
their skeleton (preflate gives back the deflate exactly); with *permanent*, the same picture
re-saved at its smallest: twelve of the repository's PNGs, every one smaller (FORMATS.md).

**Archives** (zip, 7z, tar, gzip, xz, bzip2, zstd, lz4, brotli, lzma, and RAR by the machine's own
`rar`; with a box). Opened, each file inside carried its own way, and the archive made again exactly
from its skeleton; or with *permanent*, carried as its files (FORMATS.md lists each format).

**Files mostly alike** (over 8 MiB). One carried as its differences from the other: Edward's two
game maps of 160 MB each, 98.7% alike, the second as a 2,087,830-byte script (STATISTICS,
WoWRevived).

**Repeating bytes, patterns and ramps** (v33). Carried as counts: 8 MiB of a 16-bit counter in 83
bytes (v32: 44,470); 300 MB of zeros packed in about 30 s (SPECIFICATIONS §12.2, v33).

**Copies** (v33). A file with the same bytes as another, wherever it is in the folder, carried as
a copy: 100 copies of a 1.2 MB JPEG in 963,838 bytes and 13.6 s, where v32 took 90.2 s
(SPECIFICATIONS §12.2, v33).

**Binary written as text** (v33): base64 (data: URLs, mail attachments, PEM keys), hex dumps, and
lists of byte values (C arrays, escaped strings, decimal lists). Each span decoded, carried its own
way (a JPEG reaching brunsli), and written again exactly on installing: pack.cpp as base64 76,813 →
50,323 bytes; a JPEG in an HTML page 203,542 → 170,423; a 101,888-byte program as a C array 52,065
→ 41,639, its own size packed being 41,667 (SPECIFICATIONS §12.2, v33).

**Mixing's match model** (v33). Two more ways for its match model, weighed on each mixed file's
start: Silesia's mozilla, mixed whole, came under LZMA2 for the first time, 13,318,505 bytes
against 13,376,180 (`results/mixing-match-ways-2026-11-02.md`).

## Choosing how hard it works

- **Size, Balanced or Decoding speed** (the File Locator's Priority; `--priority`): mixing decodes
  slowly, about as slowly as it packs. Size takes it wherever it is smallest; Decoding speed leaves it
  out (Edward's MAPS.WoW: 52,897,726 bytes unpacked in 166.7 s for size, v33; 60,130,138 unpacked in
  4.6 s for decoding speed: STATISTICS); Balanced takes it only where it saves 4% of the
  file (23 files and folders: 3.8% larger than Size, installed in about a third of its time).
  **Defaults: Balanced in the File Locator and Create .sieve; Size on the command line**, unless `--priority` says otherwise.
- **Try every way:** the search's shortcuts off, every file mixed whatever its size.
- **Optimise Search** (on unless unticked): Try every way chosen file by file, from what was
  measured on each kind of file.
- **A search time a file, and for the run**, or none.
- **Older versions:** `--v32`, `--v31` and older make a `.sieve` an older Sieve reads.
- Searches that cannot win are cut short: a 16 MiB map packed in 42 s instead of 166 s, the same
  bytes (`results/rope-probe-2026-11-02.md`).

## What is checked

- Every file installed is checked against SHA-256 before anything is written: by default one
  SHA-256 over the whole tree, every path and every file's own hash (`--file-hashes` keeps each
  file's own hash in the listing too).
- Everything is lossless and given back byte for byte, but with the *permanent* box, where a
  compressed format's content is kept exactly (a JPEG's picture, a PNG's pixels, an archive's
  files) and its container repacked smaller.
- A second decoder, the oracle (`reference/sieve_ref.py`), is written in Python from the
  specification alone, shares no code with Sieve, and unpacks what Sieve makes; the CI checks
  both agree on every version.
- The vault (VAULT.md) refuses chosen files, as files, as pieces of files and written out as
  text.

## The tools

- **`sieve`**, the command-line tool (`sieve help`).
- **The hallway**, a 3D walk through Sieve's lines of possible files, with the File Locator for
  packing and installing, its log recording every way each file was weighed and the one taken.
- **`sieve-install`**, the installer: run on a `.sieve`, or copied with one into a single program
  that installs itself (SIEVE-INSTALL-USAGE.md).
- **`sieve-view`**, the picture viewer, for pictures, `.simage` files (Sieve instructions holding
  pictures) and `.sieve` files holding only pictures (MANUAL.md); on Windows, `sieve-thumbs.dll`
  shows a `.simage`'s picture as its thumbnail in Explorer.
- **The right-click menu** (Windows): Extract here and Extract to a folder for `.sieve` files and
  archives by 28 file extensions (`.zip`, `.7z`, `.rar`, `.tar.gz` and the rest), and `.simage` files opening in the viewer (`context/README.md`).
- **The web extension** (Chrome and Edge): shows the `.sieve` files a page links to, each checked
  against the hash its link states (`web/README.md`).
- **The training harness:** models of Sieve's AI line trained in integer arithmetic, so the same
  run gives the same model, bit for bit, on any machine (SPECIFICATIONS §12.0e).
