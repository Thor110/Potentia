# X3: the fractals v2 could not write

## What changed

**sieve-fractal-v3: a third, wider grammar** for the fractals X2 had to set aside. It adds:
- **Swapped turns.** `!` swaps what + and - turn.
- **Exact angles.** An angle can be written as a fraction of a turn (`360/34`), or with up to six decimals.
- **Digits defined as turns or step ratios.** A line such as `define 1 turn 42` or `define 2 scale 4.783` turns by stated degrees or makes the step that many times longer. This covers Fractint's `\` and `/` turns and its scales.
- **Several rules for one symbol.** At each place one of them is picked from a `seed`. The same ruleset grows the same every time, and a different seed grows a different bush.
- **Context rules.** `rule A<B C` changes B only where A comes before it, with an `ignore` line for the symbols to skip when looking. These are what Hogeweg and Hesper's trees need.
- **Three dimensions.** `&` `^` pitch and `\` `/` roll the turtle, as The Algorithmic Beauty of Plants draws it. A 3D drawing is shown from one fixed side.
- **Longer words:** up to 255 symbols, up from 64.

**The whole space now includes v3 (fractal-space-v3).**
- v3 rulesets are numbered after the v2 ones.
- v1 addresses are unchanged. v2 and v3 addresses depend on the longest ruleset setting.
- I checked the numbering by converting 2,003 v3 addresses, spread across the space, to their rulesets and back.
- A new grammar filter, `format-fractal-v3`, gives the same counts as the space where they can be compared.

**107 fractals charted, up from 75.** The 32 new ones:
- **From Fractint's files:** the Cesaro and double Cesaro curves, Mandelbrot's four snowflake sweeps, the pentagram, Voderberg's spiral tiling, the two Vertigo spirals, two more Penrose tilings, Mandelbrot's dragon and flowsnake, a second island, plants, trees, Man and Lace.
- **From The Algorithmic Beauty of Plants:** the 5 x 5 FASS curve, the Hilbert curve in 3D, a bush and a flowering plant in 3D, stochastic plants, and Hogeweg and Hesper's five trees grown by contexts.

The transcriber checked the 21 Fractint entries line by line against a copy of Fractint's own turtle, and the five Hogeweg and Hesper trees against the book's own system. The 3D ones were only checked by the reader. I looked over all the drawings.

**Eight X2 rulesets are now exact.** X2 had rounded them; v3 writes them exactly or to six decimals:
- Ken Philip's plant at `360/14`;
- the Pythagoras tree, the H tree and Mandelbrot's Peano2 at 0.707107 (1/√2);
- Penrose's kites and darts at 0.618034;
- the branching pattern, compound leaf 2 and the spiral tiling.

Their addresses change because of this.

**docs/FRACTAL-SOURCES.md** is new. It has every charted fractal's rules as transcribed, every change from the source's notation, and what is still not charted and why.

**The default longest ruleset is now 256 characters (was 208).** At 256 every charted fractal is on the line except Voderberg's tiling (590 characters). That one needs *longest ruleset* set to 590 or more; the maximum is now 1024 (was 512). A longer setting takes longer to count when you enter the line: 0.65 s at 256, 4 s at 512 (measured here). **Your saved setting stays as it was.**

**Image line:** fractal-drawn now finds 782 pictures at 10 x 10 in black and white (was 567).

## Fixed while testing
- The hallway started the fractal line at a longest ruleset of 96 characters, while everything else uses 256. It now uses 256, and its --fractal-longest help says 44 to 1024 (default 256).
- CI: the degree sample's fractal rulesets now run to 256 characters and are stored at about their own size (74,527 of the v7 manifest's 84,225 bytes), so v7 is checked to be under a fifth of v6 rather than a tenth. The count at 44 characters is 4,083,300 (v1, v2 and v3 together).

## To try
- Warp to `hilbert-curve-3d` and `abop-bush-3d` and step them deeper: they are drawn from one side.
- Warp to `cesaro-fractint`, `vertigo-spiral-46` and `hogeweg-hesper-a`.
- Warp to `abop-stochastic-plant`. Copy its ruleset with a different `seed` and run `sieve fractal draw` on the copy to see another plant of the same kind.
- Warp to `voderberg-spiral-tiling`: it says it is past the longest, 256.

## Still not charted
- The Algorithmic Beauty of Plants' row of trees: it needs step ratios whose inverses no finite decimal holds, so its lengths would drift.
- Its chapter 2 trees: they turn by several angles about different axes.
- The Kolakoski sequence: it is not a substitution.
- The Fibonacci word fractal: no published L-system text could be read.
- Paul Bourke's pages and the OEIS were still blocked from here.
