# Changes: a bookcase in your dimension, things resting on things, better drawing

## The bookcase
- The catalogue's last object is **a bookcase**: the corridor's own case, 6 m long with four shelves.
- It's kept in `data/objects/shelf-items.sieve` with the shelf items, apart from the meshes folder that can be overridden. The pack now holds 24 files (28,370 bytes), packed as 2,885 bytes.

## Things rest on things
- Held or dropped things rest on the highest upward-facing surface beneath them: the grass, a shelf board, the case's top, or another object's lid.
- While you hold something, the crosshair rests it on the first such surface it meets within 3 m, so you can stand items on the shelves.
- A board thinner than one physics step's fall still stops what falls.

| Measured here | Came to rest |
| :--- | :--- |
| A page dropped from 17 m over the case | on the case's top |
| A record sleeve dropped onto a shelf | on that shelf's board |

- Only dropped things knock each other along. A placed thing is a surface to land on, not something to push.

## Better aiming and drawing
- **Aiming:** E and O pick the face the crosshair actually hits, so the 6 m case no longer hides a small item in front of it.
- **Faces turned away are left out**, and the rest are drawn far first, so items on a shelf show in front of the case's back panel.
- **Characters:** a character standing behind something placed is hidden, sphere by sphere and bone by bone.

## Keys
- **PgUp/PgDn** look up and down by 5°, as the mouse does. This helps keyboard-only use.

## IDEAS
- §19.187: the bookcase and shelves, marked built.
- §19.199 (new): one registry for every key.
  - Each system declares its actions under a section path, with default keys.
  - They appear on the controls listing by section, and any of them can be changed there.
  - The minimap zoom (+ and -) comes first. I'm building this next.

## Files to delete by hand
- None.
