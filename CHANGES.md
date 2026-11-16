X1c: the fractals in other lines, the image line first

- New image-line filter fractal-drawn-v1 (keep = include | exclude): a picture that is a charted
  fractal drawn at any depth it grows to (within 2^18 symbols), drawn four times the unit's size a
  side (at most 512) and fitted to the line as a picture warped in is (canon-image-v2).
  include keeps only those pictures, exclude sets them aside.
- Exact: it counts and ranks (a trie of its pictures), so compact works both ways round on palettes
  of 256 colours or fewer. Measured: 112 pictures at 10 x 10 black and white, 125 at 32 x 32.
- Setup > FRACTALS gains a third row, "in other lines": off / include / exclude. It is that filter
  ticked on the image line (and turns the image line's mode from off to mark), so the image line's
  magnifying glass shows the same setting.
- The programs tell the core where the fractal chart is; the depth limit the shelves draw to is
  now one shared function (same depths as before).
- Tests (counts both ways, the Koch curve drawn passes, ranks round-trip); CI 04 (the row's three
  settings; binary's row is now 50) and CI 10 (the counts, and the image line compacted to 112).
- Docs: SPECIFICATIONS 12.0t, the fractal help page, DIMENSIONS, filters help page, REVIEW-ROUND.

Files to delete by hand: none.
