# Changes

One commit on top of c8eea38 (review 4, batch E): F-B, the fractals line in compact.

## What you will notice
- **Compact on the FRACTALS line:** tick any of draws, grows, no idle rules and canonical on FRACTALS' filters and
  set the mode to compact. The line then holds only what they keep, numbered without gaps: the index, the v1 to v3
  rulesets that pass, then the v4 to v6 ones (which those four judge by drawing or running each, so they are
  hidden where they fail). The status line says "4 compact".
- With all four, an address on the line is 196.89 bits shorter than on the whole line (of 1,345.7); the metrics
  list says so, where it said compact was not on this line yet.
- The survivors are in an order of their own, not the line's: by the set of symbols the axiom reaches, then the
  order it reaches them in. Koch's curve is survivor 1,274,152,196,871. A known fractal the four set aside has no
  place on the line in compact, and warping to it says why (the dragon curve, whose X and Y are a renaming).
- On the command line: `sieve fractal at N --compact --filters FILE` and `sieve fractal index ID --compact
  --filters FILE`; `sieve fractal count --filters FILE` gives the compact count.

## Figures (measured here)
- A survivor of v3 at 256 characters, with all four: 2.5 s to build the numbering and find it, 151 MB at most.
- The hallway in compact, warped to a fractal: 2.7 s a run.
- The first frames at the very start of the line take about 25 s: the shelf there shows v6's last rulesets, which
  the filters judge by running each. Hide was already so without compact (26.7 s, against 1.1 s with no filters);
  item 59 in REVIEW-ROUND asks you to look at it.

## Files to delete by hand
None.
