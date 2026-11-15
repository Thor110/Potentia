# List S, step 1: the fractal dimension

Agreed before building (IDEAS 19.191, 19.235, in your words):
- the fractal dimension comes first, then layouts (for personal dimensions, not Sieve's own lines), then
  the maze door;
- it is a line like the others, with its shelves charted by hand;
- its first families are substitutions and L-systems.

## What it is

- **The line.** A ruleset is a few lines of text in a new format, `sieve-fractal-v1`:
  - a substitution, like the Fibonacci word (0 becomes 01, 1 becomes 0); or
  - an L-system, like Koch's snowflake or the dragon curve.

  Its grammar is a filter plugin, `format-fractal-v1` (330 states, compiled by `tools/grammars.py`
  like list F's formats). Tick it on the text line over ascii96, and every survivor is a valid
  ruleset; a ruleset's survivor number is its address.

  Counts agree between the engine and the oracle. At 44 characters, the shortest, there are exactly
  1,520,532 rulesets, the same as worked out by hand: 9 angles × 57 axioms × 52 rewritten letters ×
  57 words. Held to its reader: 518 of 520 test documents get the same verdict, and the other 2 are
  rules out of order, which only the reader weighs.
- **The shelves, charted by hand.** They live in `data/fractals/`: `chart.txt` names each fractal,
  and `ID.fractal` holds its ruleset. There are eleven to begin, all mathematical objects: the
  Fibonacci word, Thue–Morse, the Cantor set, the Koch curve and snowflake, Sierpinski's triangle, the
  dragon curve, the Hilbert, Lévy C and Gosper curves, and a fractal plant. To chart another, add a
  line to `chart.txt` and its `.fractal` file.
- **The screen.** The Fractal Dimension shows the shelves on the left. On the right is the chosen
  fractal, drawn at the depth you choose, with its ruleset beneath.
  - Up/Down choose a fractal; Right or = goes a step deeper, Left or - a step back; Home shows its
    axiom and End the deepest drawn; Esc goes back.
  - The keys are on the registry under "The Fractal Dimension".
  - Substitutions are drawn as their words one row per depth, so the pattern shows repeating at every
    scale. L-systems are drawn as a turtle draws them, coloured red to violet along the path.
- **The setup page.** A FRACTALS column stands after AI, in light purple, as you asked.
  - Its bar is the line of rulesets at 128 characters, always broken, as AI's is.
  - The break is drawn as a Koch curve rather than straight lines.
  - Inside the bar, the charted fractals are a sliver.
  - Its magnifying glass and its dots open the shelves. It is a column of its own, not a door of the
    hallway.
- **`sieve fractal`:** `list`, `show ID --depth N`, `draw ID --out FILE.png` and `check FILE`.

## Also

- Catalogue entries for both formats, so the "document" ring now has 12 doors, and an entry in the
  dimensions catalogue (linked from pages).
- A help-library page, "The Fractal Dimension".
- SPECIFICATIONS 12.0t, and updates to DIMENSIONS, FILTER-PLUGINS §23, IDEAS 19.191 and 19.235, and
  REVIEW-ROUND S1.
- IDEAS also records your words from this sitting:
  - layouts are for personal dimensions, with models made for each shape;
  - the shelves are charted by hand;
  - fractals can act as filters;
  - plants and decorations grown from fractal rulesets for personal dimensions;
  - fractal models and textures as a filter, and in time one fractal filter of every entry with its
    rulesets shifted and adjusted procedurally;
  - that filter's include/exclude setting (include charts what it finds; exclude knocks it out).

## For you to look at (the sitting)

Screenshots come with this zip, sent apart from it: the setup page's column, the Fibonacci word's rows, and the fractal
plant. Please judge:
- the column's look;
- the Koch-curve break;
- whether a plant should grow upwards rather than sideways. Every turtle starts heading east, which
  suits the curves; plants could start heading up.

## Still to come in the fractal dimension

- Walking the line beyond the shelves, a ruleset's neighbours by address.
- Fractals as filters on other lines.
- More families: iterated function systems and tilings.
- Then layouts (step 2) and the maze door (S2).

## Checked

- CI: all 12 steps passed.
- Unit tests: `test_fractals`, 188 checks.
- The help check: 0 missing. The settings check: 141 match. The library check: all keys and links
  found.
