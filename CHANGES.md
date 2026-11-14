# Characters as true 3D spheres, drawn to 2D, in every space

Edward, 14 November 2026: "we should seek to render them and have them interact with one another as true 3D but render to 2D".

## The renderer every space shares (MeshBatch, IDEAS 19.207)
- **Spheres as spheres** (`add_sphere`): at each pixel of its disc, the depth of the sphere's surface there. So spheres cut into one another, into bones and into faces exactly where they meet, pixel by pixel.
- In the characters' four drawings: a flat disc (pixels), a ring (wireframe), voxels (screen-fixed cells, each the sphere's depth at its centre, a darker edge), and lit from the upper left (shaded).
- **A square-on camera** (`begin_ortho`), for the characters drawn without perspective.

## Where characters are drawn by it
- **The Character Creator and the setup menu's CHARACTER page**: the same four drawings as before. One difference: the bones from the body to the head, and across the base, now show where the menu is scaled down to fit a small window (they were one-pixel lines lost in the scaling).
- **Your dimension**: characters in the same image as the objects. A bookcase in front of one hides exactly what it covers, pixel by pixel (the line-of-sight test per sphere is gone). Drawn lit from the upper left, as the Character Creator draws them (before: a slightly different radial shading), or as rings in the wireframe.
- **The hallway, Real Graphics on**: your characters are in the models' image, shaded spheres among the shelves and books, depth-tested against them. With Real Graphics off, rings and bones as before.

## Pixels and voxels (Edward: "do the voxels not look like actual voxels and instead are just squares?"; "square sizes need normalising")
- **Voxels** are true voxels now: cubes on a grid fixed to the character (they turn with it). Every cell whose middle lies in a sphere is filled, so the spheres run together into one blocky body; only the faces between a filled cell and an empty one are drawn, lit by the way each faces, their edges darker.
- **Pixels** are the flat squares the voxels were (a pixel-art drawing: each square the colour of the sphere nearest you at its middle). Before, pixels were a dot a sphere.
- Both take **one cell size for every character**: its height in 16 cells (before, a quarter of its body's radius, so it differed from one character to another).

## The render scales sheet, as a recipe
- `tools/render_scales.py --hallway build/hallway --out render-scales.png` makes the sheet sent on 14 November: every panel a frame the hallway draws (the character's settings line and the small worlds written by the script itself; nothing personal read), laid out with captions. A scale is added as one more step in its list.

## Checked here
- The Character Creator's four drawings compared with pictures taken before the change (side by side): the same, but for the bones above.
- In your dimension, a bookcase placed to stand in front of half your character cuts it cleanly at its edge, solid and wireframe.

## REVIEW-ROUND.md
- New item 22: characters in the Character Creator (all four drawings), your dimension (beside a bookcase) and the hallway with Real Graphics on.

## A bug fixed
- `--character FILE` and `--animation FILE` read the name from two different copies of it (each `get()` makes a new string), which happened to work for names of up to 15 characters and failed for longer ones (`basic_string::_M_create`). Each now reads it once. CI step 10 checks a path of 19 characters.

- In your dimension, an object whose base sits below the grass (a bookcase set down on a slope) could be left out in plain sight: the line of sight that leaves out what a hill hides went to half a metre above its base, under the grass. It now goes to the object's top.

## Ideas recorded
- IDEAS 19.212: catalogues of world systems for the personal dimensions (skies, starry nights, the Milky Way, times of day, fog).
- IDEAS 19.213: building in the personal dimensions in the third person (walls, doors, houses; roads later).
- IDEAS 19.215: greedy meshing for the voxels (flat runs of faces joined into few rectangles), for later.
- IDEAS 19.216: voxels for everything (meshes voxelised) and how big a voxel is: one lattice, powers of two of a base, each shape at the level its thinnest part needs, coarser with distance (an octree).
- IDEAS 19.214: the drawing itself a choice under Real Graphics (points, voxels, triangles; quads and smooth surfaces in time), also by distance.

## Files to delete by hand
- None.
