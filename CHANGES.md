# One renderer: your dimension drawn by the hallway's MeshBatch

## IDEAS 19.207, its first step
- The hallway's renderer, `MeshBatch` (client/mesh.hpp: a depth test at every pixel, drawn in bands on several threads), can now:
  - lay what it draws over what is already on the screen (`begin_overlay`: the ground and sky stay as drawn);
  - take faces already placed and lit by their owner (`add_face`);
  - put a picture on a face, read with perspective (`add_picture`);
  - draw a wireframe's edges where nothing nearer covers them (`add_edge`).
- Your dimension's objects, favourites' pictures and wireframe are drawn by it, seen by a camera that is the world's own eye. The depth layer added in the last zip (a second copy of the same idea) is gone, with its three fill functions.
- The pictures are the same, pixel for pixel, as the last zip's: the wireframe, the solid view and a favourite's picture were each rendered both ways and compared.
- It may be faster than the last zip's layer (MeshBatch draws on several threads); not timed here.

## Next, a choice (in IDEAS 19.207)
- The characters' spheres. The Character Creator and the setup menu draw them as discs seen square-on (no perspective), in four drawings (pixels, wireframe, voxels, shaded), far to near. Drawn as meshes in MeshBatch they would look faceted and in perspective. Kept as they look, MeshBatch would need to take a sphere as a sphere (its depth worked out at each pixel, so it is exact against faces) and a square-on camera.

## REVIEW-ROUND.md
- Item 7 (drawing order) stays unticked: drawn by MeshBatch now.

## Files to delete by hand
- None.
