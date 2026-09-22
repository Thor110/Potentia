# Real Graphics templates

These are the four starting models for the **Real Graphics** option (Settings > Graphics). Each one is built to the hallway's exact measurements, so a model made from it lines up with the wireframe, the book picking and the doors. `tools/build_mesh_templates.py` regenerates them.

| Template | What it is |
| :--- | :--- |
| `hallway.obj` | One 8 m tile of corridor: floor, ceiling, both walls, and in each wall a doorway with its frame and a recessed door |
| `bookshelf.obj` | The bookcase on the **left** wall. The right wall uses the same model mirrored in X |
| `book.obj` | One book at the uniform size, its spine facing the corridor |
| `marker.obj` | The checkered start/finish strip on the floor |

Each has a `.mtl` beside it with one placeholder material per part (`floor`, `wall`, `door`, `shelf_wood`, `book_spine`, `marker_light`, ...).

## Per-medium copies

Every line gets its own set. Copy each template to `<model>-<medium>.obj`, together with its `.mtl`, and model from there:

| Medium | Files |
| :--- | :--- |
| pages | `hallway-pages.obj`, `bookshelf-pages.obj`, `book-pages.obj`, `marker-pages.obj` |
| image | `hallway-image.obj`, `bookshelf-image.obj`, `book-image.obj`, `marker-image.obj` |
| audio | `hallway-audio.obj`, `bookshelf-audio.obj`, `book-audio.obj`, `marker-audio.obj` |
| video | `hallway-video.obj`, `bookshelf-video.obj`, `book-video.obj`, `marker-video.obj` |
| books | `hallway-books.obj`, `bookshelf-books.obj`, `book-books.obj`, `marker-books.obj` |

Put the copies in `data/meshes/` (one folder up). When you rename a copy, also change its `mtllib` line to point at the renamed `.mtl`. The "book" of a line is whatever stands in its slots: a page, a canvas (image), a record (audio), a tape (video) or a book.

## Coordinates

- **Units:** metres. **Y** is up, the corridor runs along **+Z**, and **X** goes across it, with the left wall at x = -2 and the right wall at x = +2.
- **Hallway:** z 0 to 8, x -2 to 2, y 0 to 3. It is drawn once per tile and repeated every 8 m. The doorways are at z 6.4 to 7.6 and 2.2 m high. Leave them there: walking through a doorway is how you change line.
- **Bookshelf:** z 0 to 6 of the tile, from the wall (x = -2) to its front (x = -1.65), 2.85 m tall. There are 4 rows of 16 books, 0.375 m apart along Z. A book in row r (0 at the top) stands on the board at y = 2.65 - (r + 1) × 0.55 + 0.02.
- **Book:**
  - Its origin is the bottom of the spine, centred across it. The spine faces +X, towards the corridor from the left wall.
  - It is 0.28 m wide (Z), 0.40 m tall (Y) and 0.30 m deep (towards -X).
  - The engine places it at x = -1.65, centred in its slot, and mirrors it for the right wall.
- **Marker:** a strip across the corridor from z = -0.25 to +0.25 about the line where a loop starts (z = 0 of a tile), 1 mm above the floor. Where every line starts together it is drawn twice, 1 m apart.

## Book sizes

- **Pages, image and books** vary in height from slot to slot, from 0.34 m to 0.46 m, just as the wireframe does. The engine scales the book model in Y by the slot's height ÷ 0.40.
- **Audio (records) and video (tapes)** never vary. Every slot is exactly the model's own size.

`media_sizes_vary()` in `client/world.hpp` is that switch, and the wireframe already follows it. Model the audio and video books at the size they should be. A record sleeve or a tape box can be any shape, as long as it fits its slot: 0.375 m along the shelf, 0.50 m up to the next board, 0.35 m deep.
