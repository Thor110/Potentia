# List P: your personal dimension (14 November 2026)

All eight tasks of list P in REVIEW-ROUND section 3, built one after another, then reviewed; the five
problems the review found are fixed (below). Section 1 has a test item for each (27 to 34).

## What is new
- **P1, the ground shaper** (IDEAS 19.209). In your dimension, **1** takes it out and **#** puts it away
  (the backslash too: the key in #'s place on a US keyboard). The left button or **Z** raises the ground,
  the right or **X** lowers it, the middle or **C** flattens it; the wheel or **[ ]** change the brush's
  size. Shaped corners are kept in the world file as `ground X Z CHANGE` lines (SPECIFICATIONS 12.0r).
  New settings: `[personal] shaper_radius`, `shaper_strength` (Features).
- **P2, O opens a favourite in place** (19.208). Its line is built in the background ("calculating" by
  the crosshair) and the hallway's viewer opens over your dimension; Esc or Z closes it. Falls back to
  the old way, into the hallway, when it cannot. Setting `[personal] open_in_place` (on).
- **P3, your own models** (19.187). **I** with the catalogue open imports an .obj (with its .mtl) or a
  glTF/.glb; kept as `objects/SHA.sieve`, listed after the characters, placed as `model:SHA`.
  `--pworld-import FILE` for scripted runs.
- **P4, your world on the worlds line** (19.187). **N** shows its address; Return (now a key of its own,
  `personal.worlds/go`) goes into the hallway on the worlds line with your world in hand.
- **P5, music for your dimension** (19.220). The Media Player's Mode row has a third space, PERSONAL;
  "Music here" gives the dimension music of its own, kept in its world file as a `music` line. A
  dimension without one plays WORLD's music, as before.
- **P6, favourites' covers** in the catalogue: a small picture left of each row, the chosen one larger
  below. Setting `[personal] catalogue_covers` (on).
- **P7, turning rings** (19.221). Three rings round what you hold, x red, y green, z blue; after R, T or G
  the wheel turns it 15 degrees a notch. Settings `[graphics] axis_colours` (global: Settings > Graphics >
  Axis Colours, with an Okabe and Ito preset), `[personal] turning_rings`, `wheel_step_degrees`.
- **P8, day and night** (19.222), off by default (`[personal] day_night`). Sun, sky, light and stars follow
  the time of day (UTC, so everyone sees the same). **L** sets the day's length, 1 to 2^64 - 1 seconds,
  kept as a `day` line.

New key sections on Settings > Controls > Keys, each after the ones before so no row moved: the author's
tools, your own objects, on the worlds line, day and night. `--press` takes WheelUp and WheelDown.

## Fixed after the review
- The Media Player's "Music here" read the world file and wrote it back, dropping without a word any
  line its reading left out. It now refuses to write such a world and says why.
- With the shaper out, Z closing the in-place viewer went on raising the ground while still held. The
  shaper's keys now shape nothing until let go after the viewer opens.
- `day 060`, `ground +10 0 1` and a music line's `volume=030` were read though the spec says they are not
  as written; now left out and said.
- The worlds panel's Return was fixed in the code; it is now the key `personal.worlds/go`.
- The day readout said "0.0 degrees down" at dusk on the sign of a rounding error; it now says
  "on the horizon" there.

## Ideas taken down (IDEAS, and tasks in REVIEW-ROUND section 3)
- 19.223 "Real Graphics" renamed "Wireframe" (task M4).
- 19.224 Every footer of keys wraps upwards, as the viewer's does (task M5).
- 19.225 A skin over the spheres, a stage of the mesh scaling system (task R6, R5's first step).
- 19.226 Import and export for a personal dimension, any object as its parts or the whole world (task P9).

## Checked
- Every old world gives the same picture, output and file as before (compared byte for byte with the
  build before list P, 19 command sets between the builders and the reviewer).
- CI: all 12 steps passed (`ci/run.sh --changed`: the command-line options file reaches every step).

## Files to delete by hand
None.
