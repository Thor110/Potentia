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
- PNG: made again from its skeleton;
- lzip: carried as its contents.

Before the costly search for the levels that make an archive's blocks again, its contents are
weighed cheaply, and the search is not started where they would come out no smaller.

## 2a. Files alike (v32 in the making, only with `--v32-draft`)

With the draft switch (`--v32-draft`, `[locator] v32_draft`), a file over 8 MiB carried as it is
may be **written as its differences from another file of the folder** (the `delta` way, delta-v1):

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

With the draft too, a **JPEG** of 1 KiB or more is carried as its **brunsli form** (the `jbr` way):
- **What it is:** Google's brunsli recodes the JPEG's own coded values by a model made for them.
- **When it's kept:** where brunsli gives the file back byte for byte (checked before it is kept)
  and the form is smaller; any other JPEG is carried as it is.
- **Not searched:** the form is coded already.
- **Measured:** 15.25% to 21.91% off ten original JPEGs.

Log: `file N of M: NAME, a JPEG of S bytes, as its brunsli form of F (made again byte for byte)`,
then `not searched (a JPEG carried as its brunsli form (above), coded already)`.

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
       from its bytes;
     - for a text, with an English word list.
   - It is then mixed whole only where that start came out at least 3% under LZMA2's.
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
| **The stream mixed** (v29): the files mixed one by one, mixed together instead in the stream with those carried as they are | where the stream with them, mixed whole, comes out smaller than their codes and the stream without them | `the files mixed one by one, mixed together`, `the files carried together, mixed` |

Neither runs with **Prioritise decoding speed** ticked.

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
