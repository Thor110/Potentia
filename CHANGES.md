# Y1, Y2, X5 and Y4: CI step 01 in parts; automata, attractors, flows and aggregates; their filters

Two commits: Y1 + Y2, then X5 + Y4.

## Y1: CI step 01 in eleven parts (IDEAS 19.236)
- The core suite is eleven ctest entries (core-spaces, core-characters, core-hallway, core-fractals,
  core-models, core-vectors, core-filters, core-cross, core-books, core-text-sound, core-mixing), so a failure
  names its part. `sieve_tests DIR --part NAME`, `--times` (each test's seconds), `--list` (with its part).
- `SIEVE_CI_PARTS=core-fractals ci/run.sh 01` runs one part (ci/README). Measured: core-cross 143 s,
  core-filters 47 s, the rest 15 s or less. Steps 04, 10 and 11 by line: left.

## Y2: cellular automata as a family of the fractal dimension (IDEAS 19.239, documented as asked)
- sieve-fractal-v5: `elementary RULE` (0-255) with an optional `start ROW`, or `life B.../S...` with
  `start ROW/ROW/...`. Depth is the generation; depth is time for every family.
- fractal-space-v5, format-fractal-v5 (2,244 states; 624 documents checked, all agree but the 3 a walk cannot).
- 32 charted (187 then): 13 elementary rules (rule 90 measures 1.585) and 19 life-like patterns and rules,
  each transcribed pattern run against what its source says it does (FRACTAL-SOURCES).
- `sieve fractal draw --all-depths`: every depth a frame.
- The six range-loop warnings in the tests fixed.

## X5: strange attractors, flows and aggregates (sieve-fractal-v6)
- `attractor` (x and y formulas), `flow STEP` (x, y, z, Runge-Kutta), `aggregate SEED` (diffusion-limited).
- fractal-space-v6, format-fractal-v6 (3,439 states; 641 documents, all agree but 3).
- 10 charted (197 in all): Henon (1.259, known 1.26), Ikeda, Tinkerbell, Gingerbreadman, Duffing, Bogdanov;
  Lorenz, Rossler, Thomas (started at 0.1 0 0: from 1 1 1 it stays on a line); DLA.
- The box-count grid shift: measured, no better (1.594-1.596), so left as it is (IDEAS 19.235).
- Left: parametric L-systems, circle inversion, three-dimensional sets.

## Y4: filters for the new families
- Noise filters (draws, grows, no idle, canonical, dimension) for automata, attractors, flows, aggregates.
- fractal-moving-v1 on the video line: a charted fractal growing, its frames its depths in turn; set with
  fractal-drawn-v1 by the third FRACTALS row. 1,774 at 10 x 10 x 4 mono; fractal-drawn-v1 now 5,279.

## Ideas recorded
- 19.239: your decision (automata a family, not a dimension), and every frame a texture/image.
- 19.244: every colouring of each fractal, and your follow-up on frames and GIFs.

## Figures that moved
- Shelves 197; FRACTALS only holds 198; catalogue document doors 18; count at 44 20,010,504,322,671,072.
- Sample: 897 / 682 / 690 / 0 / 195 / 681.
