# A wave direction for the rope, measured (IDEAS §19.108)

Made by `tools/rope_direction_probe.py`, 30 October 2026. A file is read as rows of W bytes.
Each remainder is measured two ways: by its order-0 entropy (what the rope's own coder, a table of
how often each value comes, would need) and by LZMA2 at preset 9 extreme. Stretches are 16 rows,
and each stretch's choice is counted in the figures.

The ways compared:

- **triangle:** the rope's P4, the median edge detector;
- **one direction:** the best single neighbour for the whole file (left, above, above-left or
  above-right), copied exactly;
- **a direction a stretch:** one of those four neighbours chosen for each stretch;
- **direction or triangle:** a stretch may choose the triangle as a fifth option.

## The designed files (rows of 256; `tools/rope_designs.py`)

| File | xz -9e of the file | triangle | one direction | a direction a stretch | direction or triangle |
| :--- | ---: | ---: | ---: | ---: | ---: |
| checker | 292 | 131,075 | 2,815 | 2,879 | 2,889 |
| diagonal | 848 | 926,317 | **8,964** | 9,028 | 9,039 |
| vertical | 552 | 660 | 658 | 722 | 732 |
| gradient | 764 | **3** | 683 | 104 | 77 |
| diagonal+noise | 133,552 | 976,609 | **127,507** | 127,571 | 127,581 |
| diagonal-250 (rows of 250) | 876 | 928,586 | **9,092** | 9,158 | 9,168 |
| two-directions | 1,148 | 972,274 | 597,283 | **9,633** | 9,643 |
| noise | 1,048,688 | 1,048,555 | 1,048,549 | 1,048,613 | 1,048,627 |

Bytes, by order-0 entropy.

- Where a file is a copy of one neighbour, a direction takes it from about the file's size to under
  1% of it.
- Where the direction changes partway, choosing it a stretch at a time is what does it:
  two-directions goes from 597,283 bytes with one direction to 9,633.
- With one byte in ten moved by one (diagonal+noise), the direction's 127,507 comes under xz's
  133,552.
- In each other case, xz on the file as it is stays smaller.

## Real files

The pictures are raw pixels. Their rows are given as W, and the left neighbour is a pixel back
(3 bytes for RGB, 4 for RGBA), so it is the same colour of the pixel before. Each table was measured
at the width the probe's search found.

| File | Bytes | Rows of | xz -9e of the file | triangle | one direction | a direction a stretch | direction or triangle |
| :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| hallway-door (1280×720 RGB) | 2,764,800 | 3,840 | 14,220 | **38,167** | 100,212 | 76,325 | 38,180 |
| font-test-french (1280×720 RGB) | 2,764,800 | 3,840 | 3,144 | **17,285** | 27,511 | 27,443 | 17,298 |
| gallery-tiles (686×494 RGB) | 1,016,652 | 2,058 | 22,060 | 316,259 | 234,395 | **233,825** | 233,826 |
| icon128 (128×128 RGBA) | 65,536 | 512 | 11,760 | 32,406 | 32,385 | **29,660** | 29,660 |
| survey-average-2026-10-05.tsv | 137,012 | 2 | 6,340 | 66,482 | **64,341** | 67,988 | 68,294 |
| vectors_image_v2.tsv | 176,632 | 8 | 69,272 | 106,321 | **99,550** | 99,735 | 99,713 |
| sieve8x8.hex | 11,265 | 22 | 3,016 | 4,910 | 4,563 | 4,582 | **4,551** |
| the first MiB of the sieve program (ELF) | 1,048,576 | 48 | 372,048 | 963,166 | 930,632 | **907,274** | 907,303 |

Bytes, by order-0 entropy. The bold figure is the best of the four ways.

The same files with their remainders packed by LZMA2 (bytes):

| File | triangle | one direction | a direction a stretch | direction or triangle |
| :--- | ---: | ---: | ---: | ---: |
| hallway-door | 13,816 | 15,248 | 14,988 | 13,830 |
| font-test-french | 3,872 | 3,344 | 3,460 | 3,886 |
| gallery-tiles | 26,216 | 30,564 | 31,572 | 31,573 |
| icon128 | 20,292 | 19,536 | 17,582 | 17,583 |
| survey-average tsv | 10,648 | 8,648 | 16,283 | 17,639 |
| vectors_image_v2 tsv | 99,972 | 96,260 | 97,125 | 97,333 |
| sieve8x8.hex | 4,128 | 3,876 | 3,880 | 3,906 |
| the sieve program | 833,156 | 506,296 | 505,858 | 505,973 |

## What it shows

- **On the real pictures, the triangle stays best or about even:**
  - the photograph-like door and the font picture are best under the triangle;
  - a direction a stretch helps the icon (29,660 against 32,406 bytes, 8.5% less) and the
    tiles (233,825 against 316,259; the tiles' rows repeat, which one direction copies).
- **On tables and the program, a direction takes off 3% to 7% against the triangle by entropy.**
  The survey table: 64,341 against 66,482; the program: 907,274 against 963,166.
- **No remainder came under xz on the file as it is**, by either measure, on any real file.
  The nearest is the font, by LZMA2: 3,344 against 3,144.
- **So a wave direction cancels designed files, not real ones.** On the real files here it would
  rarely change which way Sieve takes, since the rope is beaten by LZMA2 on all of them already.
- **Not measured:** Edward's game data (sprite sheets, textures), where rows that copy one
  neighbour may be commoner. The probe runs on any file:
  `python tools/rope_direction_probe.py FILE --width W --pixel P`.
