# GIFs opened: what their frames come to (6 November 2026)

IDEAS §19.123, measured before anything is built. Six animated GIFs, the demonstrations of Edward's
InscribedSquareProblem repository, all written by gifenc 1.0.3 (its `tools/record-gifs.js`): every
frame the whole canvas, its own local palette, no rectangles and no transparency, so one frame
repeats the last almost wholly and LZW, which sees one frame at a time, takes none of it.

## Made again exactly: yes

A compress-style LZW encoder (a clear code first, greedy longest matches, a new code each time up
to 4,096, then a clear code and a fresh table: Weiner's LZWEncoder, which gifenc follows) gives
back every frame's LZW byte for byte: **346 of 346** frames, all six files. gifenc cuts its LZW
into sub-blocks of 254 bytes (the last shorter), every one, so that too is made again by rule.

## What they come to

| GIF | Frames | Size | As now (Sieve, Optimise Search) | Opened: indices + palettes, by xz -9e |
| :--- | ---: | ---: | ---: | ---: |
| cube_demonstration | 54 | 1,775,899 | | 572,808 (32%) |
| rectangle_demonstration | 61 | 1,603,351 | | 671,720 (41%) |
| square_demonstration | 61 | 1,620,168 | | 856,784 (52%) |
| tetrahedron_demonstration | 67 | 2,208,885 | | 1,332,872 (60%) |
| triangle_demonstration | 42 | 1,152,896 | | 674,912 (58%) |
| viewfinder_demo | 61 | 1,255,478 | | 609,608 (48%) |
| **all six** | 346 | **9,616,677** | **8,377,298 (87%), in 25 s** | **4,718,704 (49%)** |

"Opened" is each file's frames' palette indices (as its own LZW decoder gives them, 119,539,120
bytes in all) by xz -9e, and its palettes by xz -9e apart; the skeleton (headers, delays, the
recipe) is left out, a few hundred bytes a file. xz alone: neither the rope nor mixing tried, nor
the frame before as context. A palette that changes each frame (gifenc quantises every frame
anew) means an index need not mean the same colour from one frame to the next, so the frame
before helps less than it might; mapping each frame to one palette is not a way of making it again
exactly, so it is left aside.

(Pillow is no way to read the indices: it gave later frames re-quantised, which a first run here
took for their indices.)

## Built: v34's `gif` (the same day)

The route, behind `--v34-draft` (SPECIFICATIONS §12.2 v34, `tools/cli/gif_lzw.hpp`): each GIF
carried as its frames' indices then its skeleton, the LZW coded again on installing. All six opened
(0.13 to 0.19 s each) and installed byte for byte, each checked against its SHA-256. Made with
`sieve locate src --installer ... --optimise-search --v34-draft`:

| Settings | Size | Of the GIFs | Making | Installing |
| :--- | ---: | ---: | ---: | ---: |
| v33, Optimise Search | 8,377,298 | 87% | 25 s | |
| v34, indices after the skeleton, shape guessed | 4,331,766 | 45.0% | 105 s | 29 s |
| v34 as built: indices first, shape from the recipe | **4,150,626** | **43.2%** | 101 s | 27 s |
| the same, Prioritise Decoding Speed (LZMA2) | 4,652,864 | 48.4% | 161 s | 3.3 s |

Mixing beats xz -9e on the same indices (4,718,704, palettes apart) by 12%. With Prioritise
Decoding Speed, 145 s of the 160 s making it went to LZMA2's three passes over the 119,811,620
bytes of indices (`--timings`: the tuned pass 88 s, the first 65 s, 7-Zip's 60 s, some at once).

Pillow 12's GIF writer codes as compress does too: an animated GIF it wrote (12 frames) opened and
was given back exactly, so the route is not gifenc's alone.

## Making faster: the stream's LZMA2 passes at once (the same day)

The lzma2 stream's search (`pack_stream`, `tune`) had run the preset's pass alone beside the
half-second settings trial, then the other three passes (the settings chosen, at the preset's
match length and at 64; 7-Zip's encoder) together once it was done. Now the trial runs first and
all four passes at once, as the memory allows (each about twelve times the dictionary: 4 at once
here, with 15 GB and 4 cores). The same passes, compared in the same order.

| The six GIFs, Prioritise Decoding Speed | Making | Size |
| :--- | ---: | ---: |
| before | 161 s | 4,652,864 |
| the passes at once | **96 s** | 4,652,864, byte for byte the same `.sieve` |

Which pass won (`SIEVE_PASS_SIZES=1`) on the 119,811,620 bytes of indices: the preset's 4,719,913;
the settings chosen (lc2 lp0 pb0) 4,652,654, kept; at match length 64 5,110,752; 7-Zip's 4,678,648.
