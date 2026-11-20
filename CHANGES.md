# Changes

One commit on top of c42c251 ("sieve O11"): the first two batches of review 4's fixes.

## Ranges of lengths
- **Memory:** a pages line of a range of lengths keeps a counting table for every length. Each length checked its own
  against the filter memory, but nothing added them up, so 1 to a few hundred letters with max-run-v1 ran out of
  memory. Now the tables are added up (a table the plugins share across lengths counted once) and the line says
  "the tables of lengths 1 to N need X of memory together, over the filter memory" instead.
- **Speed:** the units shorter than a length are worked out in closed form everywhere (a degree window on a long
  range judged each unit with a power per length), and the length at a place is found by halving.
- **Degree window on a range:** compact used to fail outright (it tried to build the guided order, which a window
  on a range has none of). Now it keeps its positional and scrambled orders.
- **BOOKS size in the setup menu:** worked out from pages of the longest length, as the hallway builds the books.

## Fractals
- **Large pictures with fractal-drawn on:** a 96 x 96 picture line crashed (stack overflow) or used over 12 GB. Now
  the automaton is not built where it cannot fit, and the list's own ranker is bounded by the filter memory: the
  96 x 96 line counts its 97,950 pictures exactly and judges them, in about two minutes.
- **fractal-lasts** grows a life-like automaton once for the generations it compares, not nine times.
- **Smaller speed-ups:** the v3 rule counting, the no-rule bodies counted once, substitution drawings grown once.

Every address, count and verdict I compared against a 0.13.80 build is the same.

## Files to delete by hand
None.
