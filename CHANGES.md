# Sieve: OPTIMISE FILTERS sets lines that are off to compact

One commit on top of b16aad7 (the help-library rename).

## The change (your answer)

OPTIMISE FILTERS now also sets each line whose display mode is **off** to **compact**. Off shows every
item whatever is ticked, so before this, optimising a line that was off changed nothing in the hallway.

- **Other modes are kept.** A line you set to mark, hide, full or excluded by hand keeps that mode.
- **The fractals line stays off.** Its filters are all judge-only, so OPTIMISE ticks none of them and
  leaves the line alone. Making the fractal filters countable is the next piece of work there.
- **A stack that can't count still works.** Compact falls back to hide, as it always has, and the top bar
  says why.

## Your readout

Your Books figure, 10^-118.30, is stronger than the 10^-115.95 measured here because of your settings:

- With 3 GB of filter memory, OPTIMISE keeps more of the books' filters ticked before their combined
  table is full. Here the default is 512 MB.
- Every other line matches what was measured here.

Both figures, and your answer above, are recorded in IDEAS 19.248.

## Docs

- **MANUAL, the "Limits" paragraph:** says OPTIMISE switches lines that are off to compact.
- **MANUAL, the compact paragraph:** no longer says every other ticked filter must be one the counting
  filter implies. Since list W, automata merge and compact together, and only filters with a ranker of
  their own still need that.

## Checks

- **CI step 04:** after OPTIMISE, a line that was off is compact. With the image line set to mark
  beforehand, it stays mark while audio becomes compact.
- **CI step 04 passed in full,** including the optimised metrics figures, which are unchanged.

## Files to delete by hand

None.
