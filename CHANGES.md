# Changes

One commit on top of 5fbfaef ("ci step 3 fix").

## The night's filters (list N, IDEAS 19.251 and 19.252)

### A filter for one degree: degree-window-v1
- Keeps only the units whose bearing (the one the hallway shows: floor(address x 360 / units), positional
  order) lies from `from` for `width` degrees, to 0-9 `decimals`. "1-2" is from 1, width 1; "1-5" from 1,
  width 4. A `from` past 360 goes round to 0; a window past 360 stops there.
- Works on every line of digits (pages, pictures, melodies, sound, video, models, AI; not binary or fractals).
- Counts over whatever else is ticked: the other filters are counted as before and the window keeps the part of
  their survivors between its bounds, so it never conflicts and the chosen stretch fills the whole loop in
  compact. A whole degree keeps 10^-2.56 (1/360) of the pages line.
- Soft and "picks a kind", so X and OPTIMISE FILTERS leave it alone.

### `sieve window`: does a window ever write a file shorter?
- Gives a unit's bearing and, for each width (a degree down to a thousandth), the window about it: what it keeps,
  the route inside, the bits to name it, the total, against no window.
- Measured on 1,881 pages of Don Quixote (data/anchors/public-domain/pg996.txt):
  - with nothing else ticked a window saves exactly what naming it costs: never a bit;
  - over clean-data + letter-pairs, real pages sit where the stack's survivors are 9-24% thicker than an even
    spread, so every width loses on average (0.06 to 0.25 bits); the best of the five choices saves 0.116 bits a
    page, before the 2.3 bits that naming the choice costs.
- So the window is a way to make the space smaller when you want to walk a stretch of it, not (measured) a way
  to compress. The filters doing their job is why: they put survivors where real files are.

### The video line compacts
- life-causal-v1, video-pan-v1 and image-mirror-v1 now rank (each place free or forced by those before it), and
  video-moves-v1 ranks too.
- video-smooth-v2: v1 with still videos set aside too (v1 retired), ranked by how many pixels each frame has
  changed so far. On the default 5x5x8 line it keeps 10^-14.95, and the Roundhay Garden Scene passes it.
- life-causal keeps 10^-52.68 and video-pan 10^-42.14 of that line; image-mirror 10^-15.05 of 10x10 pictures.

### What OPTIMISE FILTERS now keeps
- Video 10^-14.95 (was 10^-1.12), movies 10^-99.08 (was 10^-43.05), image 10^-18.90 (was 10^-18.26), books
  10^-116.67 (was 10^-116.02), tracks 10^-95.44.

### rows-alike-v1
- Each row within `changes` pixels (default 5) of the row above: row-runs' other half. An automaton where it fits,
  ranked by changes on larger palettes (rgb24).
- On 20 real pictures cut to 10x10 black and white (Vitruvian crops and Roundhay frames): at 5, 14 pass and the
  line keeps 10^-1.85; at 3, 3 pass and 10^-6.88. Weak at that size; stronger on wider pictures.

### A fault fixed
- Merges of code-made automata were cached under their settings alone ("longest=6" is both word-length-v2's and
  word-shape-v1's), so in one program (the hallway) two such stacks could share one count. A test showed a page
  counted that the filters reject; merges are now cached under each filter's name too.

### The task list
- REVIEW-ROUND.md, list N: N1 to N4 done, and N5-1 to N5-19 for the days after (hallway WINDOW HERE, windows
  across 0 and on the binary line, video-moves ranked, sound envelope ranked, quotes and dialogue on pages, the
  .obj filter, models and AI line filters, OPTIMISE measured again, a cache audit, the manual).

Checked: CI steps 01, 03, 04, 10 and 11 pass on the final code (07 passed earlier; nothing it covers changed since);
help and library checks pass. Steps 04 and 10 now expect the new OPTIMISE figures and the video filters' compact mode.

## Files to delete by hand

None.
