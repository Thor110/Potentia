# Changes: menu rows by name, world v2 specified, newer formats catalogued

## Menu rows chosen by name (IDEAS §19.188, first step)
- `--press` now accepts `Item:ID`, which chooses the row with that name on the main menu (any of its screens) or the pause menu, just as Up/Down would reach it. Example: `Item:settings,Return,Item:raid,Return`.
- An unknown name is refused, with an error saying so.
- Every CI step that reached a menu row by counting Down presses now names the row instead: 17 sequences in steps 04 and 10.
- Back returns to the row it came from by name.
- Moving or adding a menu row no longer changes any CI step. That clears the way for the reorder and personal menu layouts.

## Specified, not built yet
- `sieve-personal-world-v2` (SPECIFICATIONS §12.0r). One version covers everything the next steps need:
  - 3D place and three turns;
  - placed or physics;
  - favourites;
  - authors;
  - your own models;
  - layouts;
  - the face-finder cache.

  A v2 reader keeps lines it doesn't know and writes them back, so newer additions are never lost. A world using none of this is still written as v1.

## Catalogue
- New entries: `sieve-save-v1`, `sieve-save-v2`, `sieve-replay-v1` and `sieve-personal-world-v1`. The catalogue now has 200 entries, and all reach "information".

## Docs
- ROADMAP 5c: the order of work from here.
- IDEAS §19.195: a hypercube behind the main menu, replaceable visualisations, and menus people arrange themselves.

## Files to delete by hand
- None.
