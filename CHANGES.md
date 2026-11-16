X1, part 1: the fractal dimension's whole space

- core/fractal_space: every ruleset the reader takes (fractal-space-v1), 44 to a longest
  (default 96, at most 512) characters, each at one address; exact counts; at() and index_of().
- sieve fractal count | at ADDRESS | index ID|FILE [--longest N].
- The FRACTALS line stands on the whole space: the index first, then every ruleset, the charted
  ones named where they stand. Addresses are BigUint.
- Setup > FRACTALS: two rows, longest ruleset and known fractals (include / exclude / only);
  --fractal-longest and --fractal-known.
- Tests: counts checked against the format-fractal-v1 DFA for 44-53 characters; charted and
  997 spread addresses round-trip.
- CI 10: sample 4,320 items, whole-space checks; CI 04: binary's setup row is now 49.
- Docs: SPECIFICATIONS 12.0t, fractal-dimension help page, MANUAL, DIMENSIONS, SETTINGS,
  IDEAS 19.235 (your notes on the next steps), REVIEW-ROUND X1.

Files to delete by hand: none.
