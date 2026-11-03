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
     or more, neither the rope nor the pieces are tried. Where the rope wins on text (tables of
     digits, CSVs, word lists) it came to 0.84 to 0.98 there; prose and code 1.74 to 2.54
     (results/weighing-order-2026-11-02.md). Not with Try every way ticked itself (only where
     Optimise Search turns it on). Log: `rope not searched: a text whose start its line wave makes
     R times LZMA2's`.
   - Not searched for any other file the rope cannot win (2 November 2026): a file of more than 512
     KiB whose first 256 KiB are not text has the rope's search made on those 256 KiB first, and
     where that comes to 1.1 times LZMA2's estimate of the same or more, neither the rope nor the
     pieces are tried. Where the rope won whole (pictures, a sound) the test came to 0.69 to 0.94;
     where it lost (programs, a map, mozilla's tar, screenshots) 1.23 to 3.71; the map packed in 42
     s where it took 166, to the same bytes (results/rope-probe-2026-11-02.md). Where the start comes to
     1.1 but under 1.2, 256 KiB from the file's middle is asked too, and the rope searched where it
     wins there (under 1.0) (3 November 2026): Silesia's `mr`, a scan, came to 1.09 at its start and
     0.92 in its middle, and the rope carries it whole in 2,314,195 bytes against LZMA2's 2,735,555.
     Not with Try every way ticked itself. Log: `rope not searched: its first 256 KiB the rope's
     search makes R times LZMA2's`, with the middle's after it where it was asked.
   - Not searched either for a repeating byte or pattern (found before anything else): LZMA2, the
     rope's wave of differences and (from v33) the segments are weighed, with no search.
3. **Pieces**: the file cut into 4 KiB blocks, each weighed by LZMA2 against the rope. Runs of
   blocks go to the way that codes them smaller, and runs saving under 64 bytes join their
   neighbours. Each boundary is then moved, up to a block either way, to where the two cost least.
   - Not cut where the file is under 16 KiB, or where every block went the same way.
   - Log: `cutting it into pieces: weighing its N blocks`; in the file's line, `pieces B` or
     `pieces not cut (all N blocks of 4 KiB went to LZMA2: nothing to cut)`.
4. **Mixing** (mix-v4): a context-mixing model, coding and decoding at about the same slow speed.
   - First tried on the file's first 256 KiB, three ways, the smallest kept:
     - as bytes;
     - as a picture: the rope's shape where its best wave reads the file as one, or a shape guessed
       from its bytes (that too where the rope's picture is under 16 wide, a pixel or two a line);
     - (from v33) a picture of 2 to 4 bytes a pixel holding 256 colours or fewer, as its palette:
       one-byte indices, then the colours (mix-v4's `P`; the hallway's screenshots 20.6% smaller,
       results/palette-2026-11-02.md);
     - for a text, with an English word list (from v32, SCOWL's size 80 with each word
       counted by how common it is, `F80`: 2.6% smaller on Don Quixote's start; v31, size 60);
     - (from v33, reopened 3 November 2026) for a web text (HTML, CSS, JavaScript: tags or braces
       at a page's density), primed on MDN's examples first (`W1`), with the word list where that was
       taken: the log says "primed on the web dictionary". A web text of 128 bytes to 4 KiB, which
       mixing otherwise leaves alone, is weighed so too. Groups and the lzma2 stream's mixing weigh it
       on their start the same way (results/web-dictionary-2026-11-03.md).
   - From v33 (frozen 3 November 2026; `--v32` makes none), each of those codes also weighs the match model's ways on
     the same start: none, `C2`, or `R1 M4 C2`, the smallest kept (programs: Silesia's mozilla 3.3%
     smaller mixed whole, under LZMA2; SPECIFICATIONS, v33). Where all three win there and the file
     is over 4 MiB, they are weighed again against `C2` alone on its first 4 MiB (3 November 2026):
     `R1` and `M4` cost more as a file runs on, so enwik8 came out 1.5% larger with them whole
     (21,870,393 bytes against `C2`'s 21,431,822) and samba 2.0% (3,342,227 against 3,277,334),
     though their first 256 KiB said otherwise; on the five Silesia files where all three win whole,
     the first 4 MiB said so too. It costs two codes of 4 MiB (samba: 256.9 s to make, from 231.7)
     (results/v33-measured-2026-11-03.md).
   - A shape guessed from the bytes alone (not the rope's, not a PNG's own) is taken only where it
     wins the start by 2% (2 November 2026: a near tie is no guide to the whole; Silesia's mozilla).
   - It is then mixed whole only where that start came out at least 3% under LZMA2's; and mixed whole,
     it replaces LZMA2 only where it is under LZMA2's estimate by 64 bytes and 0.25% (the estimate
     can come out above the stream as made: five small JPEGs, 2 November 2026).
   - Not tried for a file under 4 KiB or over 16 MiB, nor with **Prioritise decoding speed** ticked.
   - With **Try every way**, every file is mixed whole, of any size, with no test first.
   - Log: in the file's line, `mix4 B22 D60 SIZE [mixing on its first 262144 bytes: as bytes X,
     with the word list Y]`, or `mixing not tried (WHY)`, or `mixing's test on its start X (not 3%
     under LZMA2's, so not tried whole)`.

The smallest of these is taken. The file's line says what each came to, and which was taken:

```
file 1 of 1: enwik8, 100000000 bytes: LZMA2 about 24865184, rope 59719961 (L8 W4096 S1 P0 K0
C2 G1 A1), pieces not cut (...), mix4 B22 D60 21675799 [...]; taken: mix4, 21675799 bytes
```

## 4. Files together

| Way | When | Log |
| :--- | :--- | :--- |
| **Groups mixed together** (v30): files in the listing's order, in groups of up to 8 MiB, each group mixed as one | for files mixed alone or carried as they are, that compress; a group is kept where it comes out smaller than its files apart | `group N: K files from A to B, S bytes: mixed together X, apart Y; taken` (or `apart taken`) |
| **Mixed files weighed against the stream** (1 November 2026, no new way; from v30, after the groups): the files mixed, alone or in groups, carried as they are instead, in the lzma2 stream they would share | where the stream with them comes out smaller (by LZMA2's estimate, by 64 bytes and 0.5%) than the stream without them and their codes; weighed twice, the files that are not pictures, then the pictures; not for one file alone in the stream (Silesia's mozilla taken apart: 13,794,627 bytes to 13,530,583) | `files mixed: X bytes (N files' codes C, the stream S); all in the stream: T; the stream taken` (or `mixing kept`); the report's `weighed   N file(s) mixed carried as they are instead, smaller in the stream they share (T bytes by LZMA2's estimate, against X mixed)` |
| **The stream mixed** (v29): the files mixed one by one, mixed together instead in the stream with those carried as they are | where the stream with them, mixed whole, comes out smaller than their codes and the stream without them | `the files mixed one by one, mixed together`, `the files carried together, mixed` |

None of them runs with **Prioritise decoding speed** ticked.

## 5. The streams

What is carried as it is goes into one **lzma2 stream**; programs go into an **x86 stream**; a
listing and an items stream may have streams of their own. Each is packed this way:

1. LZMA2 at preset 9 extreme, its own settings, as the baseline.
2. At the same time, LZMA2's literal and position settings (lc, lp, pb) tried on the stream's first
   MiB at preset 6: seven settings that suit text, unaligned bytes, and 2- and 4-byte samples.
3. The whole stream at the settings chosen, at the preset's match length and at 64, and by 7-Zip's
   own encoder. The smallest of all is kept, so a stream is never larger than at the preset.

Log: `the lzma2 stream (S bytes): packing it, and trying LZMA2's settings on its first MiB`, then
`best so far B bytes; packing it at lc4 lp0 pb0, at two match lengths and with 7-Zip's encoder`, then
`best so far B bytes; done`.

## 6. Read back (`reading them back, as installing will: every file decoded and compared`)

The instructions are read back as installing reads them: every stream decoded, every file made,
and each compared with the original. Nothing is saved that does not give every file back. This
costs what installing costs: for enwik8 optimised for size, 121.9 s against an install's 119.643 s.

## 7. After packing: set beside zip and 7z, and weighed

- **The instructions compared**: zip and 7z of the `.sieve` itself (`what zip and 7z make of them`),
  and its address.
- **The weighing**: each file against its own address, every way it can be named, in bits:
  - its own address on the binary line;
  - the binary line compact, where its filters rank the file;
  - an item of a line (pages, image, audio, video), on the line and compact under its filters;
  - the rope, LZMA2 alone and pieces, as packing found them;
  - mixing, as packing found it (since 30 October 2026).

  What a reader must be told for a way (its shape, its filters) is paid once, however many files
  use it, and a way whose shared cost outweighs what it saves is turned off. **Tailor the filters to
  these files** tailors each line's filters to the files here, paying for their description.
  - Log: the table, then `every way weighed`, every file's every way with its bits, the best marked
    `*`.

## 8. What the log adds up to

At the end come the status, `took S s in all`, `last new best at S s in`, the result's tables, and
the phases' times. A step on less than 64 KiB (a stream of a few bytes) is marked but does not
count for `last new best`. Each reading starts afresh: no search an earlier reading made is taken
as found, so its time is its own. Where the time went for enwik8 and enwik9, step by step, is in
[Proof/STATISTICS.md](../../Proof/STATISTICS.md).
