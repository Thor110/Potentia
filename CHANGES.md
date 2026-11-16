# X2: more fractals charted, and the grammar widened

## What changed

**A widened grammar (sieve-fractal-v2).** v1 could not write many well-known fractals, so v2 adds two things:
- **Angles with a tenth of a degree**, such as 22.5 or 25.7. Many plants are drawn at these.
- **A scaled step.** With `scale 0.707`, the symbol `"` makes every step after it 0.707 times as long, and `'` makes it longer by the same factor. `[` and `]` keep and restore the step's length, along with the turtle's place and heading.

This is what spirals, trees that thin out, and spider webs need. A v2 ruleset must use at least one of the two; anything else is still written as v1, so every ruleset has exactly one way to be written.

**The whole space now includes v2 rulesets (fractal-space-v2).**
- Every v1 ruleset keeps its old address.
- The v2 rulesets are numbered after the last v1 one, from the shortest.
- A new grammar filter, `format-fractal-v2`, passes v1 and v2 rulesets. Up to 52 characters its counts match the space's.

**75 fractals charted, up from 11.** Each one now has a `source` line in the chart (`sieve-fractal-chart-v2`) saying where it was transcribed from. You see it on the fractal's hover and page as "; after ...". The sources:
- **The Algorithmic Beauty of Plants** (Prusinkiewicz and Lindenmayer): the Koch islands and variants, the islands and lakes, the quadratic Gosper curve, the FASS curves, the six plants of figure 1.24 and a branching pattern.
- **Wikipedia:** the Sierpinski, Sierpinski square, arrowhead and Moore curves, a Peano curve, the twindragon and terdragon, a binary tree, and six substitutions: the period-doubling, Rudin-Shapiro, paperfolding, Baum-Sweet, tribonacci and Pell words.
- **Fractint's L-system files:** Mandelbrot's islands, curves and trees, Ken Philip's plants, and Herb Savage's Penrose tilings, sphinx and spiral tiling.
- **Three written for Sieve from a description:** an orb web (36 radii, with a spiral across them whose chords shrink by 0.985 each), the Pythagoras tree and the H tree. Their source lines say they are constructions, not transcriptions.

Where a source's notation differed, the change is noted in the chart. The only one that alters the ruleset itself: Fractint's empty rule "F=" cannot be written, so F is rewritten to an unused letter E instead. The drawing is the same.

Every ruleset is read by the reader and was drawn and looked over. I have not compared each drawing with its source's figure one by one.

**The default longest ruleset is now 208 characters (was 96).** The longest charted ruleset (Penrose's kites and darts) is 201 characters, so at 208 every charted fractal is a named place on the line. **Your saved setting stays at whatever it was** (96 if you never changed it). To see all 75 among the generated rulesets, set *longest ruleset* to 208 in Setup > FRACTALS.

**Box-counting dimension.**
- `sieve fractal show` and `sieve fractal list` now measure each L-system's dimension from its drawing. Measured values: Koch curve 1.29 (true value 1.26), Sierpinski triangle 1.58 (1.585), Hilbert curve 2.00, a straight line 1.00.
- A new filter, **fractal-dimension-v1**, sits on the FRACTALS magnifying glass's JUDGE-ONLY tab. It keeps L-systems that measure at least the value you set (1.10 by default). Substitutions always pass.
- In a sample of 1,000 rulesets spread across the space up to 208 characters, 391 measure 1.10 or more.

**The image line's fractal filter (fractal-drawn-v1)** now draws all 75 fractals, so it finds 567 pictures at 10 x 10 in black and white (was 112) and 684 at 32 x 32 (was 125).

## To try
- Warp (T) to `orb-web`, take it (E), and step it deeper with N: the spiral winds in across its radii.
- Warp to `pythagoras-tree`, `penrose-kites-and-darts` or `abop-plant-1-24c`.
- `sieve fractal` lists all 75 with their dimensions and sources.
- On the FRACTALS magnifying glass, tick fractal-dimension and mark or hide what measures below 1.10.

## Not done
- Fractals that need things the grammar still lacks (listed in SPECIFICATIONS 12.0t): a symbol that swaps left and right turns (the Cesàro curve, Mandelbrot's snowflakes), turns by stated degrees, several step ratios at once, rules longer than 64 symbols, 3D, stochastic and context-sensitive rules.
- Paul Bourke's pages and the OEIS could not be reached from here, so nothing is transcribed from them.
