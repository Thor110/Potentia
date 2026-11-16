# X4 fix, third step: the turtle and the dimension in Sieve's own arithmetic too

The digest (7b01695) named the two fractals whose pictures differed between your machine and this one:
gosper-curve and lars-cross-tree, both L-systems, drawn by a turtle whose turns used the system's cos and
sin; a line's end fell on one side of a pixel's edge under Microsoft's library and the other under GNU's
(1,056 pictures against 1,057). Every other fractal's 153 hashes matched.

- core/src/fractal.cpp: the turtle (v1/v2 headings, v3's turns in three dimensions and its fixed view)
  uses Sieve's own sin and cos, as the formulas do; the box-counting dimension uses its own log and a
  line's length by a square root (no log2, no hypot), as fractal-dimension-v1's threshold reads it.
- docs/SPECIFICATIONS.md: says so, with what the hashes found.

Here: 1,057 pictures still; of the 155 fractals' hashes only sierpinski-triangle's moved (b076937d60bd,
was 662afcd8dedb), which your machine drew as this one did before, so with the same functions both now
draw it alike. Dimensions unchanged (dragon 1.73, orb-web 1.341, Hilbert 3D 1.932, fern 1.279, Koch
1.289, Sierpinski 1.575, Hilbert 2.000); the sample's 894 / 691 / 700 / 0 / 180 / 688 unchanged. Core
tests 12,813,492 checks, 0 failures; CI steps 04 and 10 pass.
