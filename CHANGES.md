# Sieve: word-length-v2, and the setup page's glasses and bar breaks

One commit on top of 6e3c7c4 ("sieve metrics").

## word-length-v2 (your note)

The cap stays an adjustable setting on the filter, now from 5 to 100 letters, with 28 as the default,
so "antidisestablishmentarianism" passes.

On 1,000-letter pages, a cap of 28 keeps 10^-12.92 of the line, against 10^-24.23 at v1's cap of 20.
Changing the default changes what a stack keeps, so v1 (20, 1 to 64) is retired and kept, and saved stacks
still give the same results.

## The magnifying glasses on the setup page

Every line has its glass back. On 16 November (the fractal work, X1 batch 2), the condition that let
FRACTALS have a glass was written so that only FRACTALS had one. Now every ordinary line has its glass,
and so does a listed line that has filters.

## The AI bar's break

- **After "find my limits":** this is the rule as designed. A bar is broken when its line is more than
  8 times the median size, too long to set the scale. After "find my limits" every line is larger and
  WORLDS is the longest, so AI fits the scale and is drawn whole, to size. The FRACTALS bar is broken
  whatever its size.
- **A real fix:** with filters ticked on a broken line, the filled bar showing what survives was drawn
  over the break and hid it. The break is now drawn last.

## Also recorded

IDEAS 19.246 records your notes on storage for the infinite RAID and on a typo filter. Next steps:

- **Storage:** `[raid] space_mb` and `per_peer_mb` exist only in the settings file. Next is a row for them
  on the Infinite RAID page.
- **Typos:** words-v1 already sets aside pages with typos. The typo filter's use is the step between a
  real word and noise: a word one edit away from a real one.

## Checks

- Unit tests: the word-length checks are new. Step 01 passed on the filter change.
- CI steps 03, 04 and 10 ran on the menu fixes.
- Screenshots before and after "find my limits".

## Files to delete by hand

None.
