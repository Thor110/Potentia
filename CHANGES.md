# Real Graphics for tracks and movies; binary at both ends of the title rule

Against `f30537f` (new higher dimensions).

## What changed

- **Models of their own:** `data/meshes/book-tracks.obj` / `.mtl` and `book-movies.obj` / `.mtl`,
  begun as copies of audio's record sleeve and video's tape box (their `mtllib` and object names
  changed to match), so each can be modelled from there.
- **`faces.ini`:** `[tracks]` and `[movies]` sections (copies of audio's and video's), and every
  section in door order.
- **Code:** the dimensions table's separate `models` folder field is gone. Every line now loads
  `<model>-<id>.obj` and its own `faces.ini` section, then falls back to the template, as the
  other lines already did.
- **Main menu:** the rule under the title is now a band per line as the corridor runs, binary at
  both ends, so ten bands.
- **Docs:**
  - `data/meshes/templates/README.md`: every line's files in door order (tracks, movies, models and
    binary added), how binary's room model loads, and which item models began as copies; the
    lines whose items vary in size.
  - README's file map lists the nine lines.
  - IDEAS §13 and HANDOFF: an entry, and the earlier note that compositions borrow audio's and
    video's models corrected.
- **CI:** tracks and movies each load their own item model under Real Graphics.

The templates themselves (`templates/*.obj`) are one per kind of model, not per line, so there was
nothing to copy there; the per-line copies live one folder up, and the templates' README lists them.

## Checked

- Builds with no warnings; unit tests 36,410 checks, 0 failures.
- The hallway and models CI steps pass (the steps this touches), with the new check.
- `main.png`: the main menu's rule, ten bands. `real-tracks.png`: tracks under Real Graphics,
  with `book=book-tracks.obj` reported (and `book-movies.obj` on movies).
