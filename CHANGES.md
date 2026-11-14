# List R: characters and the renderer (14 November 2026)

Files to delete by hand: none.

## R2: one drawing everywhere (IDEAS 19.214)
- How characters are drawn is now one setting, `[character] drawing`. You can change it on Settings > Graphics > **Character Drawing** (the last row before Back) or on the Character Creator's drawing row.
- It applies in the Creator, on the CHARACTER page, in your personal dimension, and to your stored characters standing in the hallway.
- The default is still shaded, so nothing changes until you choose another drawing.
- With Real Graphics off, every space draws the wireframe and the readout says so.

## R3: voxel size and joined faces (IDEAS 19.215, 19.216)
Settings > Graphics > **Character Voxels** has these rows:
- **Voxel Size:** a character's height in 16 cells, as today and still the default, or one world lattice.
- **Lattice Base:** 10 mm by default.
- **Thinnest Part:** 2 cells by default.
- **Join Voxel Faces** (greedy meshing, off by default): faces of one colour lying in one plane are drawn as one rectangle. The picture stays the same and the readout gives the face count.

## R6: the skin (IDEAS 19.225)
- A fifth drawing, **skin**: the spheres wrapped in one surface that is smooth where they meet (smooth minimum of their distances, then surface nets). It follows the joints as an action plays.
- **Skin Blend** (0 to 200%) and **Skin Detail** are on Character Voxels.

## R5: quads and smooth (IDEAS 19.214)
- Two more drawings built on the skin:
  - **quads:** four-sided faces with darker sides;
  - **smooth:** normals averaged at each corner, with the light and colour shaded across each face (Gouraud).
- MeshBatch gained faces with a colour at each corner.

## R4: editing by the joints (IDEAS 19.203)
- In the Creator, click a sphere to choose it, then drag it; the limb beyond it follows.
- Keys:
  - **.** / **Comma**: choose the next or previous sphere
  - **N**: grow a limb
  - **X**: take the limb away
  - **J** / **Shift+J**: add or take out a joint
  - **T**: stretching on or off
  - **Ctrl+arrows**: nudge the chosen sphere
  - **Ctrl+Z**: undo
- Bilateral and radial characters change on every side at once.
- An edited character is saved in the new format `sieve-character-v4`, specified in SPECIFICATIONS 12.0j.

## Fixed after the review
- **The hallway stopped on a very wide character drawn as voxels.** It asked for a grid too large to hold, and the Creator had always done the same. Under the height rule the cell now grows until the widest side fits in 256 cells, and the readout says "made coarser to fit". There is a unit test and a CI line for it.
- **Skins of still characters could be remade every frame.** Only eight skins were kept, so with more characters in sight they kept pushing each other out. Each space now says how many characters it draws, and twice that many skins are kept (at least eight).
- **The Creator's own keys are now on the key registry**, under Settings > Controls > Keys > "The Character Creator and the CHARACTER page":
  - **A** and **Shift+A**: play and take off the animation
  - **[** and **]**: turn
  - **Shift+[** and **Shift+]**: tilt
  - **Home**: the view as it opens
  - **R**: a seed at random
  - **G** and **Shift+G**: the seed walk
  - **Delete** and **Backspace**: take the held model away
  - **Ctrl+S**, **Ctrl+O**, **Ctrl+I**, **Ctrl+K**: the files

  The Creator's and CHARACTER page's footers read these keys as they are set. A key given to both a Creator key and an editing key is now named in red on the Keys screen; at the default keys nothing clashes. MANUAL notes that Ctrl+arrows now nudge in the Creator.

## Notes taken down
- IDEAS 19.227: greedy meshing that keeps every voxel's colour (a texture, compiled and kept by the grid's SHA-256; breaking apart by chunks).
- IDEAS 19.228: a face that shows feeling (waits on the DNA dimension).
- IDEAS 19.216: the voxel size default decided. The height rule stays: it always fits, never breaks a character apart, and is cheapest. The lattice is to be looked at again for the story's human-like characters.
- CI step 10: three checks that something is *not* printed (`grep -c` counting 0) stopped the step under `set -eo pipefail`, because grep fails when it counts nothing. They now read `test "$(grep -c PATTERN FILE || true)" = 0`, which still fails if the line appears.
