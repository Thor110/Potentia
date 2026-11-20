# Changes

One commit on top of 9c2d024 (review 4, batches A and B): batch C, the filters.

## Memory and speed
- **specks-v1 and rows-alike-v1** keep their automata's states packed. Measured here on black and white:
  specks at 16 x 16 went from 2.35 GB to 0.64 GB of memory (15 s to 9 s), rows-alike at 16 x 10 from 3.26 GB to
  1.38 GB. The automata built are the same.
- **join-cut-v1 and weight-signs-v1** build their counting tables only where the filter memory holds them, and
  otherwise judge, as rows-alike and video-smooth already did (a wide AI line or a large frame ran out of memory).
- **The video and picture rankers** keep each state's recent pixels once instead of twice.
- **few-values-v1** no longer builds its automaton every frame while its row is chosen on the filters page: whether
  it is an automaton is worked out by counting.

## Conflicts shown correctly
- **sound-envelope and sound-syllables** now say they count as their own ranker where their automaton's table does
  not fit the filter memory, as they then do. A pair with sound-step-v1 that could not be compacted together showed no
  conflict before.

## Tidying
- One copy of the automata's size bound and cache (core/src/filters/dfa_bound.hpp), not four.

Counts and verdicts I compared against 0.13.80 are the same.

## Files to delete by hand
None.
