# Sieve: more filters for every line (Z3 to Z9)

One commit on top of b332754 ("sieve z").

## Z3: noise filters for the fractal families

Two new filters behind the magnifying glass on FRACTALS, for the families whose generated rulesets are
mostly noise:

- **`fractal-strange-v1`** (attractors and flows) passes those whose path stays finite and on which two
  starts a billionth apart end far apart, as a chaotic attractor's do. One that settles on a point or a
  loop, or flies off, fails.
- **`fractal-lasts-v1`** (automata) passes those still alive at the generation you set (32 at first),
  and whose live part, wherever it stands, is none of the previous 8 generations'. One that dies out,
  freezes, repeats or glides along fails.

Measured on 300 generated rulesets from each block:

| Filter | Kept |
| :--- | ---: |
| `fractal-lasts-v1`, on automata | 195 of 300 |
| `fractal-strange-v1`, on attractors and flows | none (random formulas fly off or settle) |

Of the charted ones, every attractor and flow is strange except Bogdanov's map, which traces loops at its
charted settings. The Life oscillators, gliders and spaceship do not last, nor does rule 184. The known
fractals are never judged by these filters anyway.

## Z4: the fractals on the pages and audio lines

- **`fractal-written-v1`** (pages) finds or culls a charted fractal's word at each depth, written in the
  alphabet. With `symbols = any`, it also takes every renaming of the word's characters.
- **`fractal-scored-v1`** (audio notes) turns a fractal into a melody, as Prusinkiewicz made scores of
  L-systems in 1986:
  - Each turtle line across is a note. Its height is the step of the major scale; its length is the
    note's.
  - A substitution's word is one note a symbol.
  - With `transpose = every`, each melody comes at every pitch.
- The third FRACTALS row now sets these too, alongside the image, video and models filters.

| Line | Kept |
| :--- | ---: |
| Pages of 32 on ascii96 | 531 |
| Pages of 32 on lower27 | 43 (the alphabet has no + or -) |
| 16 notes on notes104 | 3,548 melodies |

These are counted and ranked exactly, so the line compacts. On notes2's 304 symbols they are counted
exactly but judged.

## Z5: the video line's own filters

- **`video-moves-v1`** sets aside videos whose frames are all the same. Those are pictures of the image
  line shown again.
- **`video-smooth-v1`** keeps videos in which each frame changes at most a set share of its pixels
  (25%).

Both are counted exactly, but each compares a frame with the one before, so they are judged (mark or
hide) rather than ranked. At 10 x 10, 4 frames, black and white:

| Filter | Keeps |
| :--- | :--- |
| `video-smooth-v1` at 25% | 10^-19.65 of the line |
| `video-smooth-v1` at 5% | 10^-66.61 of the line |

The command line now prints the exact count for a stack of one such filter.

## Z6: the sound line (IDEAS 19.232)

- **`sound-crossings-v1`**: at most 200 zero crossings per 1,000 neighbouring pairs. Noise crosses about
  every other sample; a 440 Hz tone at 8 kHz crosses 110 times per 1,000.
- **`sound-bend-v1`**: the second difference is at most 25% of the range, so the wave turns gently.

At 800 samples:

| Filter | Keeps |
| :--- | :--- |
| `sound-crossings-v1`, 8-bit | 10^-67.87 of the line |
| `sound-bend-v1`, 4-bit | 10^-315.44 of the line |

Both are exact automata where they fit. `sound-bend-v1` is judged at 8 bits.

## Z7: small page filters (IDEAS 19.232, 19.235)

- **`few-values-v1`**: more than 2^k distinct symbols (k = 1 at first). A page of at most 2^k values
  reads as bits, so it belongs to the binary line.
- **`two-symbol-run-v1`**: no stretch longer than 12 on at most two symbols, which catches padding like
  "aaaabbbb". The cap comes from 1,304 pages of English prose: the longest such stretch was 3 to 7 on all
  but 11 pages, and 15 at most.
- **`golden-word-v1`**: a page of two symbols that is a window of the golden ratio's Fibonacci word. With
  `slopes = every`, any Sturmian word. At 32 letters that is 23,166 pages, or 1,280,448 with every slope;
  counted exactly and judged.

**Not done:** reading bits the other way round (not-written-v2). The idea notes say to measure first how
many pages it would catch; it is still to be measured.

## Z8: filter ideas from the documents

- **`life-causal-v1`** (video, two colours) keeps films in which each frame is the one before stepped by
  the Game of Life, or another rule set as B../S... There are 2^100 at 10 x 10: the first frame is free
  and the rest follow (IDEAS 19.141).
- **`image-mirror-v1`** keeps pictures that are their own mirror image: across, down, both, or turned
  half round. At 10 x 10 black and white that is 2^50 across, or 2^25 both ways.

## Z9: my own designs, aimed at meaning

Each filter names a mark that meaning leaves and noise almost never does. Each was measured on real
material before its default was set.

| Filter | Rule | Measured |
| :--- | :--- | :--- |
| `pronounceable-v1` (pages) | Every word could be said aloud: a vowel in words of 4 letters or more, no more than 5 consonants or 5 vowels together. | Keeps 10^-29.07 of 1,000-letter lower27 pages. The project's prose passes: manual 150 of 206 pages, specifications 301 of 335, speeches all. |
| `word-length-v1` (pages) | No word longer than 20 letters. | Keeps 10^-24.23. Every manual page passes. |
| `letter-case-v1` (pages with capitals) | Capitals only begin or fill a word, and begin each sentence. | Keeps 10^-42.46 of ascii96 pages. Strict: mixed-case names (macOS, GitHub) fail. |
| `brackets-v1` (pages) | Every bracket is closed by its own kind, in order, at most 4 deep. | Keeps 10^-14.87. |
| `video-pan-v1` | A camera panning: each frame is the last moved a pixel. | |
| `melody-motif-v1` | A 4-note figure, heard as its steps and lengths, comes back. | About 1 in 100,000 random melodies pass. Judged. |
| `sound-periodic-v1` | The sound has a pitch: a period of 50 Hz to 1 kHz repeating. | A tone passes; noise and silence do not. Judged. |

The page filters are automata, so they count, rank and compact exactly. The next round of designs is
written down in IDEAS 19.245: horizons, specks, scene cuts, fades, note envelopes, cadences, contrary
motion, solid models, and words that name the picture beside them.

## Checks

- Unit tests: a new `test_z_filters`, plus the fractal checks. It also covers English and nonsense pages,
  a symmetric face, a Life blinker against a panning bar, a repeated figure, and a 220 Hz tone. It covers the counts above, a 440 Hz tone
  passing both sound filters while noise fails, still and flickering videos, and the golden word either
  way round.
- CI steps 01, 03, 04, 07, 09 and 10 on this tree. Step 03's weighing figure moves by one bit (531 bits, not 530) because the audio line now offers one more filter to choose from.
- Help, help library and settings checks pass.

## Files to delete by hand

None.
