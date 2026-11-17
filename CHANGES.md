# Sieve: fractal colourings and the models line (Z1 + Z2)

One commit on top of 4a96c39 ("sieve y2x5").

## Z1: every colouring of each fractal (IDEAS 19.244)

- `fractal-drawn-v2` (image line) and `fractal-moving-v2` (video line) replace version 1, which is
  retired and kept so earlier stacks reproduce. The setup page's third FRACTALS row now sets version 2.
- Two new settings:
  - `turns`: `drawn`, or `every` (the default). `every` adds each picture turned and mirrored: eight
    ways for a square picture, four otherwise. In a video, every frame is turned alike.
  - `colours`: `drawn`, `inverted` (the default) or `any`.
    - `inverted` adds each picture in its colours' opposites.
    - `any` keeps every colouring of each picture's shape. The pixels that share a colour must share
      one, whatever the colours, each different.
- Measured at 10 x 10 in black and white:

  | Filter | Kept |
  | :--- | ---: |
  | As drawn (version 1's set) | 5,279 |
  | Turned | 31,190 |
  | Turned and inverted (the default) | 62,356 |
  | Any colouring (the same as inverted in black and white) | 62,356 |
  | Videos (10 x 10, 4 frames) | 23,830, against 1,774 |

- On 16 colours (ega16):
  - Turned and inverted: 91,206 pictures.
  - Any colouring: 45,180 shapes and 12,830,096,076,621,376 pictures. That count is exact, but those
    pictures are judged rather than ranked, so the line marks or hides them.
- Everything else counts and ranks exactly, as before (compact mode).

## Z2: the fractals on the models line

- New `fractal-modelled-v1`: the charted fractals' lines as meshes, found (include) or culled (exclude).
  - The meshes come from L-system turtles (in 3D for v3) and from the flows' paths.
  - A line is a face naming its two ends, (a, b, b).
  - Each model appears twice:
    - with its spare faces filled the way the line fills any mesh, (0, 1, 2);
    - with its own lines again in their place.
  - It also appears in each of the 24 turns of a cube.
  - The setup page's third FRACTALS row sets it together with the image and video filters.
- Measured at the default 8 vertices, 12 faces and grid of 16:

  | Shape and filters | Models |
  | :--- | ---: |
  | As fitted | 138 |
  | Every turn | 3,216 |
  | Every turn, with distinct-vertices-v1 | 864 |
  | 16 vertices and 32 faces, every turn | 7,008 |

  Only the shallow depths fit so few vertices.
- Include always ranks. Exclude ranks wherever the line's other filters do, so the line compacts either
  way.
- A line is a degenerate face, so `distinct-indices-v1` already sets every one of these models aside.
- `fractal_lines3` gives the turtle's lines in three dimensions. The pictures are drawn as before.

## Checks

- Unit tests: every part passes. That covers the counts above, ranking both ways round, and a recoloured
  picture passing under `any`.
- CI steps 01, 04 and 10 were run on this tree. Step 04 covers the menu row and the models counts; step 10
  covers the image and video counts.
- Help, help library and settings checks pass.

## Files to delete by hand

None.
