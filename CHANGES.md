# R7 to R9: the mesh scaling pipeline (15 November 2026)

Three new stages for the skin, quads and smooth drawings, each a row on Potentia Settings > Graphics >
Character Voxels, each off by default. Off, every picture and readout is as before (compared byte for
byte with the build before R7). To test: section 1 of REVIEW-ROUND.md, items 45 to 47 (new).

## R7. Limbs (IDEAS 19.229)
- **Limbs** (`[character] skin_limbs`, off by default): each bone becomes a rounded cone in the skin's
  field, so a limb is one smooth tube from sphere to sphere and spheres that do not touch are joined.
  The limbs follow the joints as an action plays. The voxels and pixels are made from the spheres, so
  Limbs leaves them as they are.
- Measured: triangles stay within 8% of the skin without limbs; making the skin takes about 1.4 to 2.1
  times as long (seed 99 at the default detail: 22,676 triangles in about 15 ms, against 23,180 in 9).
- The head is joined to the body like any limb, so the two become one dome.

## R8. Skin Smoothing (IDEAS 19.229)
- **Skin Smoothing** (`[character] skin_smoothing`, 0 to 20 rounds, 0 off): Taubin's smoothing of the
  skin (a shrink step then an inflate step a round), the same triangles, normals made again after.
- Measured: 1 to 2 ms to start, then 0.1 to 0.3 ms a round; after 20 rounds the volume is 100.06% to
  100.84% of the skin's. The knuckles where sphere sizes change quickly stay.

## R9. Skin Subdivision (IDEAS 19.229)
- **Skin Subdivision** (`[character] skin_subdivision`, 0 to 3 steps, 0 off): Catmull and Clark's
  subdivision of the quads and smooth drawings, after the smoothing, so the outline is round too. The
  skin drawing keeps its own triangles, to compare with.
- Each step makes four times the quads (seed 99: 11,590, then 46,360 in about 5 ms, then 185,440 in
  about 29 ms). A step that would pass 200,000 quads is not taken, and the readout says so.

## Fixed after the review
- Limbs on a long slanting bone checked most of the grid: a 4,096-sphere chain zigzagging corner to
  corner took 49 s to skin. Each bone is now worked in short pieces: 1.0 s, the same triangles.
- The 200,000-quad bound was per character, so a dimension with thirty characters could draw and keep
  thirty times that. In your dimension and the hallway the bound is now shared by the characters drawn
  in a frame. The Creator, with one character, is unchanged.
- The MANUAL's "halves with each step" corrected to the measured 7.5, 4.5 and 2.4 degrees.

## Left (IDEAS 19.229)
- At 2 or 3 steps the quads drawing's edges darken it; the colour line between spheres is ragged; the
  frame time with these stages in your dimension is not yet measured; each stage runs on one thread.

## Files to delete by hand
None.
