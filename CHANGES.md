X1, next batch: the index under the setting, the warning, the glass and its noise filters

- Known fractals set to exclude: the index book goes too, so the line is generated rulesets only
  (its favourites are kept apart from the old exclude shelf's, as the items moved down one place).
- Warp (and Go to) to a charted fractal under exclude, or to the index, says it is set aside by
  "known fractals" and where to change it (Setup > FRACTALS, or --fractal-known); one longer than
  the longest ruleset says the same of --fractal-longest.
- The magnifying glass is back on FRACTALS. Its filters, on the JUDGE-ONLY FILTERS tab:
    fractal-draws-v1      it draws a line (or, a substitution, two colours)
    fractal-grows-v1      its word grows with depth
    fractal-no-idle-v1    no rule for a symbol never met, none rewriting a symbol to itself
    fractal-canonical-v1  one ruleset of each renaming of its symbols
  Mark, hide and excluded work as on other lines; compact hides (they judge only). The index and the
  known fractals are never judged. Saved as [fractals] in sieve-filters.ini.
- sieve filters --line fractals [--longest N] [--sample N]: lists them, and judges N rulesets spread
  evenly over the space. Of 1,000 (to 96 characters): 635 draw, 348 grow, 360 have no idle rule,
  none is canonical, 302 pass the first three together.
- Tests for each filter on charted and made-up rulesets; CI 04 (row 47 opens the fractal filters),
  CI 10 (the warnings, the index under exclude, mark, the sample's figures; the metrics list now
  gives the fractal line a Kept figure, 12 lines).
- Docs: SPECIFICATIONS 12.0t, the fractal help page, DIMENSIONS, filters help page, REVIEW-ROUND.

Files to delete by hand: none.
