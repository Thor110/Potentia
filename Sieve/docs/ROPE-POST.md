# The rope, and the state space it works in

A draft for encode.su: what Sieve is, why its state space alone does not compress, and the rope,
Sieve's own transform. Every figure here was measured, and the section it comes from is named.
(docs/POST-DRAFT.md is Edward's own draft and is separate.)

## 1. Where it comes from

Sieve is Edward James Gordon's design, built with Claude. The idea is older than the code: his
Gallery of Babel application (github.com/Thor110/GOB, commit 2201d24, 7 August 2024) and this
repository, first called AIrchive (github.com/Thor110/Potentia, initial commit dd88b5e, 11 April
2025). Edward dates the idea itself a year or two before the first of those; earlier papers and
files have not been checked.

The steps of the design, in the order Edward took them:

1. **The Library of Babel as a line.** Borges's hexagons are impractical to walk, so every possible
   file of a kind is put on one line, in order: a file's place on the line is the file read as one
   number.
2. **The line closed into a circle.** A place is then a bearing on a compass and a walk from where
   the bearing lands. This became variable length addressing: a file is named by its place without
   leading zeros, or by a short bearing and a walk, whichever is shorter (IDEAS §3.6).
3. **The state space split by media type.** Pages, pictures, sound, video, books, models, worlds,
   AI models and plain files each have their own line (a "dimension"), whose units are that kind's
   every possible item.
4. **Filters that take out the noise.** On each line, filters set aside units no one would keep
   (on the pages line, a letter repeated past a limit, or binary written out as text; on the models
   line, a mesh with a vertex or a face's index repeated, or not in its one canonical order), and
   what is left, the survivors, is counted and ranked exactly, so a survivor is named by its number
   among them.
5. **Filtering across the dimensions.** A file that is exactly an item of another line (a page as
   its text, a picture as its PNG) is named on that line, and a line's filters can set aside what
   belongs to another line (`not-other-line-v1` on the pages line: a melody or a mesh written out as
   text; `not-an-item-v1` on the binary line: a file that is exactly another line's item).
6. **And then the finding that this saves almost nothing.** Section 2.

None of this is new as mathematics: naming a file by its place among all files of its kind is
enumerative coding (Cover, 1973), and no naming can beat the file's own information on average
(Kolmogorov complexity is the bound). What others have built on the Library of Babel has stayed
a curiosity: libraryofbabel.info finds any page's place, but the place is as long as the page;
πfs keeps files as their places in π, which is longer still and can take any time to find. Sieve
takes the state space as a working compressor: every file is addressed, filtered, weighed against
real compressors, carried by whichever way is smallest, and installed back byte for byte.

## 2. Why the state space alone does not compress

A filter that keeps a share f of a line saves log2(1/f) bits off every address on it. Each 9 added
to the share it removes saves log2(10), about 3.32 bits more:

| Removed | Kept | Bits saved |
| :--- | ---: | ---: |
| 90% | 1 in 10 | 3.3 |
| 99.99% | 1 in 10,000 | 13.3 |
| 99.9999999999% (12 nines) | 1 in 10^12 | 39.9 |
| 1,440,000 nines | 1 in 10^1,440,000 | about 4.78 million |

Removing 99.99% of a space sounds like nearly everything, and it saves 13 bits. To save a real share
of an address, a filter has to remove a share with as many nines as the bits it saves, divided by
3.32, and only a filter that knows the file's structure can do that: then it is a model of the data.

Sieve's own lines, under its reference filters (`data/metrics/reference-filters.ini`, the metrics
list of 6 November 2026, Edward's machine; each line at the setup menu's first shapes):

| Line | Kept | Bits saved | Of an address of | Share saved |
| :--- | ---: | ---: | ---: | ---: |
| Binary (every file of 32 bytes or fewer) | 10^-4.34 | 14.4 | 408.3 bits | 3.5% |
| Pages | 10^-0.00062 | 0.002 | 304.3 bits | 0.0007% |
| Books | 10^-0.0032 | 0.011 | 860.7 bits | 0.0012% |
| Images | 10^-0.000020 | 0.0001 | 252.1 bits | under 0.0001% |
| Models | 10^-21.47 | 71.3 | 356.1 bits | 20.0% |
| Worlds | 10^-85.90 | 285.4 | 1,122.5 bits | 25.4% |
| AI models | 10^-145.65 | 483.8 | 32,768.2 bits | 1.5% |

Models and worlds lose a fifth to a quarter of their addresses because their filters know what a
mesh is (`distinct-vertices-v1`, `distinct-indices-v1`, `canonical-mesh-v1`: the same mesh written
in one order only); pages and pictures lose almost nothing, because their filters set aside only
what is plainly noise (`max-run-v1`, `not-written-v1`, `not-packed-v1`). That is why Sieve weighs every file's
address against real compressors, and carries each file by the smallest of its ways: LZMA2, the
rope, pieces of the two, or a context-mixing model.

## 3. The rope

The rope (`rope-wave-v1` to `v6`, SPECIFICATIONS; `core/src/rope_wave.cpp`) is Edward's
"tightened rope" (IDEAS §19.87): a wave run through a file's values predicts each one, and only
what the wave misses is coded, by how often each leftover byte occurs. Its parameters are written
as a short text, for example `L16 W2 S1 P3 K0 T1`:

- `L`: the file read as values of 8, 16 or 32 bits.
- `W`: values in a line, so that "the line above" means something (2 for 16-bit stereo sound: the
  same channel one frame back; the width of a picture's row).
- `S`: how far back "the value before" is.
- `P`: the prediction: none (the rope alone); the value before; the mean of before and above;
  before + above − diagonal; or (P4) the median edge detector of LOCO-I, as JPEG-LS uses it.
- `K`: keep. Values at the start, a third, the centre, two thirds and the end are kept as they are,
  and a straight rope is drawn between them. With `P0` the rope is the prediction; with a
  neighbour predictor it stands in only where a value has no neighbour.
- Columns (`C1`, `C2`), groups of columns sharing a table (`G1`), the byte above as context (`A1`),
  and (`T1`) the leftovers split by how busy each value's neighbourhood is (|a − c| + |b − c| against
  the bounds 2 to 128: eight bands, and a ninth for a value with no such neighbours), each band with
  its own tables.

What is left, `(value − prediction) mod 2^L`, is cut into byte planes and coded by rANS, one table
for each plane (or column, group or band). Taken mod 2^L, a signed sample read as unsigned costs
nothing where it wraps. **The search** (`rope_search`) tries the parameters in a fixed order (no wave;
the value before, at each value length; then for each value length, lines of one value and up to four
widths found from the file's own repeats, with every phase and step, and columns; last, the best with
keep) and keeps the smallest estimated code. Decoding is one
pass: predict, add the leftover, next.

**An example.** The Vitruvian picture as 1,233,700 bytes of grey pixels: the search finds
`L8 W949 S1 P3 K0` (bytes, rows of 949, each pixel from the one before, the one above and the
diagonal), and codes it in 832,146 bytes, where LZMA2 takes 865,400. As colour pixels at 475 × 650,
`L8 W3 S1 P3 K0` (each colour from the one beside it and the same colour a pixel before) codes it in
494,774 against LZMA2's 623,717 (SPECIFICATIONS, rope-wave-v1).

**The WAVs' recipe, in full.** Six of the seven WAVs the rope won in section 4 took `L16 W2 S1 P3`
with `T1` (two of them with keep, `K1`), the seventh `L32 W1 S1 P2 K1`; and the rope alone carries
each: its code is a stream of its own, kept
as it is, with no LZMA2 over it. Read as 16-bit values in lines of two, a line is one stereo frame,
left then right:
- the left sample has no value before it in its line, so it is predicted from the line above, the
  left sample of the frame before;
- the right sample is predicted from before + above − diagonal: the left sample of this frame, plus
  the right of the frame before, less the left of the frame before. That is the right channel's last
  value moved by as much as the left channel just moved: the two channels predict each other;
- `T1` splits what is left by how busy each sample's neighbourhood is (|a − c| + |b − c|, nine bands,
  quiet passages apart from loud ones), and each band's two byte planes get a table of their own,
  coded by rANS.

The search found this itself: it tried the value widths, line widths from the file's repeats (2 for
stereo), every predictor and step, and kept the smallest.

**Its relatives.** Each part of the rope has a relative, some close:
- predicting from neighbours and coding the residual: PNG's filters, the gradient and median
  predictors of LOCO-I / JPEG-LS, FLAC's fixed predictors, 7-Zip's Delta filter;
- `T1`'s bands: CALIC (Wu and Memon, 1997) sorts its residuals by an estimate of local error energy
  built from gradients, cut into eight contexts; JPEG-LS quantises local gradients into contexts too;
- the encoder choosing a predictor and writing its choice in the file: WebP lossless, whose encoder
  picks one of 14 predictors for each block;
- one channel predicted from the other: FLAC's side channel and MPEG-4 ALS's inter-channel prediction;
- the file carrying what its decoder must do: ZPAQ, whose archives hold their decoder as a program;
- finding a record's width: PAQ's record model;
- the kept values with straight lines between them: piecewise-linear detrending.

No part is new on its own. What we have not found elsewhere is the whole: these held in one short
parameter text, searched per file over value width, line width, step and predictor, with the
anchors-and-rope baseline, and weighed file by file against LZMA2, pieces and mixing, so that it is
used only where it wins. (Searched on 8 November 2026: the codecs above, encode.su, and tools built
on the Library of Babel; a closer relative may exist that we did not find.)

## 4. What it does, measured

**Where it wins.** WoWRevived's music (Edward's run, 7 November 2026, the File Locator's Balanced):
seven 16-bit stereo WAVs, searched by the rope, each against LZMA2's estimate of it:

| File | Bytes | LZMA2 | The rope | Its wave |
| :--- | ---: | ---: | ---: | :--- |
| Music-Human/02 Track02.wav | 54,686,396 | 48,710,708 (89.1%) | 38,261,233 (70.0%) | `L32 W1 S1 P2 K1` |
| Music-Human/03 Track03.wav | 53,333,996 | 47,404,971 (88.9%) | 39,485,202 (74.0%) | `L16 W2 S1 P3 K0 T1` |
| Music-Human/04 Track04.wav | 54,928,652 | 48,386,561 (88.1%) | 39,871,942 (72.6%) | `L16 W2 S1 P3 K1 T1` |
| Music-Human/05 Track05.wav | 50,695,052 | 44,094,152 (87.0%) | 37,950,298 (74.9%) | `L16 W2 S1 P3 K0 T1` |
| Music-Martian/03 Track03.wav | 54,987,452 | 46,947,264 (85.4%) | 38,655,650 (70.3%) | `L16 W2 S1 P3 K0 T1` |
| Music-Martian/04 Track04.wav | 52,400,252 | 47,766,483 (91.2%) | 40,942,118 (78.1%) | `L16 W2 S1 P3 K0 T1` |
| Music-Martian/05 Track05.wav | 54,869,852 | 49,639,244 (90.5%) | 41,564,208 (75.8%) | `L16 W2 S1 P3 K1 T1` |
| **All seven** | **375,901,652** | **332,949,383 (88.6%)** | **276,730,651 (73.6%)** | |

The rope's code is 83.1% of LZMA2's on them, and it decodes in one pass: the whole release
(1,426,355,971 bytes) made with nothing mixed (v32, Prioritise Decoding Speed, on Edward's machine;
the WAVs by the rope, 73.5% of them) came to 1,016,369,457 bytes, 5.34% under 7z, and installed in
30.1 s, against 210.3 s made for size with mixing (Proof/STATISTICS.md). Silesia's `mr`, a medical scan: 2,314,195 bytes by the
rope against LZMA2's 2,735,555 (METHODS §3). A sine of 16-bit values with noise, 6,003 bytes: 4,583
against LZMA2's 5,324.

**Three ideas measured against it** (IDEAS §19.127, §19.128, §19.131). Rounding a file's place and keeping
the rounding in a table of common roundings: the leftovers repeat across files only as silence and
runs of small values, and coding each leftover byte given the one before does better with no table
(2.2 to 2.7% of these WAVs, a gain the rope does not yet take). Noise run through a file, its
matching bytes clipped out: a seed matches one byte in 256 whatever the file, and saying which costs
more than clipping them saves, on the WAV as on random bytes. Trimming out the values the wave
predicts exactly, and putting them back in order when decoding: the decoder needs a yes/no for
each value, and coded given the yes/nos before them and the value's band they take 0.15 to 0.69% off
the rope's `T1` code on three WAVs and x-ray, and 4.36% on `mr`, where a third of the values are
exact and come in runs; a second pass or a noise pass loses (results/rope-trim-2026-11-08.md).

**Where it loses.** Text is not smooth: on prose and code the rope's line wave comes to 1.74 to 2.54
times LZMA2's, and Sieve does not search it there (results/weighing-order-2026-11-02.md). On
programs, a game's map, a tar of programs and screenshots, its test on a file's start came to 1.23
to 3.71 times LZMA2's (results/rope-probe-2026-11-02.md). And the test can be wrong: the eighth WAV
(Music-Martian/02, 57,339,452 bytes) came to 1.25 times LZMA2's on its first 256 KiB, so the rope
was not searched on it, and it was mixed instead (43,716,129); the test is a shortcut, and that is
its cost. With Try every way ticked itself, the shortcut is off (where Optimise Search turns Try
every way on for a file, it stays).

**What it costs.** The search: on MAPS.WoW (160,890,897 bytes) about 1,020 s (Proof/STATISTICS.md,
MAPS.WoW). Decoding: one pass, a prediction and a table lookup a value, against mixing's about 1.2 s a
megabyte (results/balanced-2026-11-07.md).

## 5. To try it

`sieve locate FILE --installer FILE.sieve` makes the instructions (the rope weighed with the other
ways, file by file; `--progress` says what each file came to by each way, and the rope's parameters);
`sieve install FILE.sieve --to FOLDER` gives the file back, checked against its SHA-256. Every
format is written down in docs/SPECIFICATIONS.md, and an independent Python reader
(`reference/sieve_ref.py`) decodes the rope from that definition alone.
