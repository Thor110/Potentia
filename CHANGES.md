# Changes

One commit on top of d48fba8 (review 4, batch C): batch D, the personal dimension, characters and the hallway.

## Fixes you might notice
- **A hang in the personal dimension:** an object placed very far out (by a world file, or by typing a large number on
  the Place panel's X row) froze the program on the next frame. The line of sight now stops at the edge of the
  ground, and the Place panel keeps X, Y and Z on the ground's square.
- **Character skins with holes:** where many spheres overlap (joints added on the same place, or a high Skin Blend)
  the skin could come out cut open at the edge of its grid. The grid now reaches far enough. The CI's readout for the
  test character changes from grid 79x55x82 to 81x57x84, with the same triangles. At a high blend and detail the cell
  is made coarser sooner, so the grid stays within the limit it promises.
- **Keys that did not stick:** a key bound to ; was lost on restart (the settings file read it as a comment), and
  Keypad , could not be bound at all. They are written Semicolon and Keypad Comma now. Holding Enter while capturing
  a key no longer binds the action to Return.
- **Documentation window:** a search with no results, then Home and Enter, read past the start of the list.
- **Opening a MIDI file in the viewer** no longer makes its whole sound first; it is made the first time you press
  Space.

## Smaller
- A world file's fav: and char: kinds must be SHA-256s, as model: kinds already were.
- Characters coming into sight no longer make every subdivided skin again; the smooth drawing reads normals the skin
  already holds.
- Duplicated helpers (the skin's neighbour lists; the documentation window's lower, trim and split) are now single
  copies.

## Files to delete by hand
None.
