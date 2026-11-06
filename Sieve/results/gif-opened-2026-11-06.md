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
