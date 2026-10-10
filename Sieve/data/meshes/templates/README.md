# Real Graphics templates

These are the five starting models for the **Real Graphics** option (Settings > Graphics). Each one is built to the hallway's exact measurements, so a model made from it lines up with the wireframe, the book picking and the doors. `tools/build_mesh_templates.py` regenerates them.

| Template | What it is |
| :--- | :--- |
| `hallway.obj` | One 8 m tile of corridor: floor, ceiling, both walls, and in each wall a doorway with its frame and a recessed door |
| `bookshelf.obj` | The bookcase on the **left** wall. The right wall uses the same model mirrored in X |
| `book.obj` | One book at the uniform size, its spine facing the corridor |
| `marker.obj` | The checkered start/finish strip on the floor |
| `edge.obj` | The binary line's tile: floor, ceiling, the left wall with its doorway, and on the right, where the other wall would be, a short wall 0.95 m high standing on the edge of the drop. The engine mirrors it in X when the shelves are on the right. Copy it to `edge-binary.obj` to model the binary line's room |

Each has a `.mtl` beside it with one placeholder material per part (`floor`, `wall`, `door`, `shelf_wood`, `book_spine`, `marker_light`, ...).

## Per-medium copies

Every line gets its own set. Copy each template to `<model>-<medium>.obj`, together with its `.mtl`, and model from there:

The medium is the line's id, and the lines are listed in door order:

| Medium | Files |
| :--- | :--- |
| image | `hallway-image.obj`, `bookshelf-image.obj`, `book-image.obj`, `marker-image.obj` |
| pages | `hallway-pages.obj`, `bookshelf-pages.obj`, `book-pages.obj`, `marker-pages.obj` |
| books | `hallway-books.obj`, `bookshelf-books.obj`, `book-books.obj`, `marker-books.obj` |
| audio | `hallway-audio.obj`, `bookshelf-audio.obj`, `book-audio.obj`, `marker-audio.obj` |
| tracks | `hallway-tracks.obj`, `bookshelf-tracks.obj`, `book-tracks.obj`, `marker-tracks.obj` |
| video | `hallway-video.obj`, `bookshelf-video.obj`, `book-video.obj`, `marker-video.obj` |
| movies | `hallway-movies.obj`, `bookshelf-movies.obj`, `book-movies.obj`, `marker-movies.obj` |
| models | `hallway-models.obj`, `bookshelf-models.obj`, `book-models.obj`, `marker-models.obj` |
| worlds | `hallway-worlds.obj`, `bookshelf-worlds.obj`, `book-worlds.obj`, `marker-worlds.obj` |
| ai | `hallway-ai.obj`, `bookshelf-ai.obj`, `book-ai.obj`, `marker-ai.obj` (for the AI dimension, which is not built yet) |
| binary | `edge-binary.obj` (its one-sided room; without it, `hallway-binary.obj` with the open side cut away), `bookshelf-binary.obj`, `book-binary.obj`, `marker-binary.obj` |

The item models for image, pages, audio, tracks, video, movies, models, worlds, AI and binary are already there (`book-<medium>.obj`). Some began as copies: `book-binary.obj`, `book-worlds.obj` and `book-ai.obj` of the models line's crate, `book-tracks.obj` of audio's record sleeve and `book-movies.obj` of video's tape box, so each line can be modelled on its own from there. Books still uses the template.

**The picture on the front.** Every line's items carry a pre-rendered picture of what they are (a page's text, an image, a video's first frame, a book's title and cover, a model's mesh). `data/meshes/faces.ini` says where on the item it goes, per medium, in the model's own metres: `bottom` and `top` up from the shelf board, and `half_width` either side of the slot's centre, on the front face at x = 0. When you change an item model, measure its front again and change its section to match; the picture is stretched with the model on lines whose items vary in height.

Put the copies in `data/meshes/` (one folder up), where the item models above already are. The game loads `<model>-<medium>.obj` for the line you are on. It falls back to the template of the same model, and then to wireframe for just that part, and prints to the console which file it used for each part. Only `v`, `vn`, `f`, `mtllib`, `usemtl` and each material's `Kd` colour are read. Faces can have any number of corners, faces pointing away from the camera are not drawn, and textures are not supported yet. When you rename a copy, also change its `mtllib` line to point at the renamed `.mtl`. The "book" of a line is whatever stands in its slots: a page, a canvas (image), a record (audio, tracks), a tape (video, movies), a book, a crate (models, worlds, and AI when it comes) or a file (binary).

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

- **Image, pages and books** vary in height from slot to slot, from 0.34 m to 0.46 m, just as the wireframe does. The engine scales the book model in Y by the slot's height ÷ 0.40.
- **Audio and tracks (records), video and movies (tapes), models and binary** never vary. Every slot is exactly the model's own size.

Each line's `sizes_vary` in `client/dimensions.hpp` is that switch (`media_sizes_vary()` reads it), and the wireframe already follows it. Model the audio and video books at the size they should be. A record sleeve or a tape box can be any shape, as long as it fits its slot: 0.375 m along the shelf, 0.50 m up to the next board, 0.35 m deep.

## The picture on the front

Every line's items show a picture on their front: a page its text, an image its picture, a video its first frame, a book its title over its cover, a model its mesh. Where on the item it goes is `faces.ini`, beside the meshes: one section per medium, giving how far up the model the picture's `bottom` and `top` are and its `half_width` either side of the centre, in the model's own metres, measured on its front (the x = 0 plane, facing +X). When you change a model's front, measure it again and change its section to match; a missing section or value is the whole 0.28 x 0.40 m slot.

