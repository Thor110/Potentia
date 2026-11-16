# X4: fractals beyond rewriting (sieve-fractal-v4)

You asked to include every fractal ever found, known or theorised, and to chart longer ones. Most known fractals are not L-systems, so this batch adds the next families. The rest follow in X5 (below).

## New ruleset format: sieve-fractal-v4
- **IFS** (iterated function system): `ifs`, then one or more `map A B C D E F` lines. Each map takes (x, y) to (A x + B y + E, C x + D y + F), as Fractint's IFS files write them. It is drawn as the points reached by every sequence of maps, starting from the first map's fixed point; N goes a level deeper.
- **Escape-time**: `escape FORMULA`, for example `escape + ^ z 2 c` (the Mandelbrot set). The formula is z's next value written operator first: `+ - * / ^`, `abs conj cos exp log sin`, `z c i` and numbers. An optional `julia RE IM` line fixes c and starts z at the point; an optional `view X Y SPAN` line sets the window. N follows each point for twice as many steps.
- **Convergence**: `converge FORMULA` (Newton's method). Each point is coloured by where it settles.
- **The whole space** is now fractal-space-v4: every v4 ruleset comes after v3's. The shortest is 26 characters (`escape c` is the first).
- **The grammar** format-fractal-v4 (2,059 states) passes exactly the v4 rulesets the reader takes. It checks against the reader and against the space's counts (12 at 26 characters, 28,770 at 30).

## Charted: 48 more, 155 in all
- **From Fractint's `fractint.ifs`**: Binary, Coral, Crystal, Dragon, Floor, Koch3, Spiral, Swirl5, Tree, Triangle, Zigzag2, and "Fractint" (the program's name in 22 maps, 690 characters).
- **From Fractint's built-in types**, with its default parameters and windows: the Mandelbrot set, its default Julia set, z^4 + c and its Julia set, the lambda set, two magnet fractals of type 1 and type 2's Mandelbrot set, and Newton's method for z^3 - 1 and z^4 - 1.
- **From Wikipedia**:
  - Barnsley's fern and its Cyclosorus variety.
  - The Heighway dragon, twindragon, terdragon and Lévy C curve as IFS.
  - The Cantor set, and the Sierpinski triangle and carpet, the Vicsek fractal and the Koch curve as IFS. The last four are written from each article's construction, and their source lines say so.
  - The Burning Ship, the tricorn and the Multibrot of degree 3.
  - Nine Julia sets from its galleries, the Douady rabbit and corabbit, and the Collatz fractal.
- Every entry's source, numbers and roundings are in docs/FRACTAL-SOURCES.md, under X4.

## Measured
- Image line: 1,057 charted pictures at 10 × 10 (was 782).
- A sample of 1,000 across the space: 894 draw, 691 grow, 700 have no idle rule, 180 pass the dimension filter, 688 pass the stack.
- Sierpinski's triangle as an IFS measures 1.60, close to the exact 1.585. Barnsley's fern measures 1.28: its points crowd where its maps shrink least.

## To try
- Warp to `mandelbrot-set`, `barnsley-fern`, `newton-z3-minus-1` and `douady-rabbit`, take them, and press N to go deeper.
- `sieve fractal draw burning-ship --out ship.png`
- Warp to `fractint-ifs-fractint`: it says it is past the longest (690 against 256). Four charted ones are past 256 now (with Fractint's Koch3, 272, and the Sierpinski carpet, 331); raise the longest to 690 to see them all on the line.

## Not yet (X5)
- Parametric L-systems, for The Algorithmic Beauty of Plants' trees.
- Strange attractors (Lorenz, Hénon, Martin).
- Circle inversion (the Apollonian gasket).
- Three-dimensional sets (the Mandelbulb, Fractint's 3D IFS).
- Random fractals (diffusion-limited aggregation).
- Formulas that need z's earlier value (phoenix) or a test inside the step (Fractint's barnsley types).
- Magnet type 2's Julia set: drawn as an escape picture it was noise, so it is not charted.

Files to delete by hand: none.
