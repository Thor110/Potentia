# Changes: holding and placing in your dimension, the Place panel, map and compass, fixes

## Fixes
- **CI step 11 (Windows):** `sieve catalogue --check-records` now reads the records before anything else, so nothing earlier in the command can stop it. Every line gets a verdict, whatever goes wrong with it. `tools/check_stat_plugin.py` decodes output as UTF-8, and if the counts ever differ it prints each program's exit code and error message.
  - In the folder your failed run kept, `sieve catalogue --check-records stat-records.txt` would show what stopped it there.
- **CI step 10:** "document" now has 7 doors. The previous zip's new `sieve-replay-v1` catalogue entry added one, and that zip's step 10 still expected 6. My mistake.

## Your personal dimension (IDEAS §19.192)
- Picking up, turning, placing and dropping:

  | Key | What it does |
  | :--- | :--- |
  | E or a click | Pick up what the crosshair is on; it is held 2 m along the crosshair. |
  | R / T / G | Turn it 15° about the vertical, its side or its front (Shift turns the other way). |
  | Return or a click | Place it exactly where it is, floating if it is in the air. |
  | Q | Drop it, so it falls. |
  | Esc | Put it back where it was. |
  | O | Open it. Shelf items say they are not items of a line; favourites will open in their own viewer. |

- **Physics** runs in fixed steps of 1/120 s: gravity, a small bounce, friction, knocks when you walk into something, and objects pushed apart. Measured here:
  - a page dropped from the hand comes to rest on the grass within 3 s;
  - a book walked into is knocked 2.6 m and comes to rest.
- **Place panel:** Tab while holding opens it, sliding in as the setup menu's panels do.
  - It shows the exact place in metres and the turns in degrees.
  - Up/Down choose a row, and Left/Right move by a step (0.1 m or 5°; Shift for ten steps).
  - Type a number and press Enter to set a value exactly; Enter on "place it here" places the item.
- **One sliding panel for every screen:** `client/slide_panel.hpp`, with the setup menu's 180 ms slide and its edge tab. The setup menu takes its timing from it.
- **Saving:** the world is written as `sieve-personal-world-v2` once anything is off the ground, tipped or dropped. Lines it doesn't know are kept and written back.

## Map and compass (IDEAS §19.197)
- **Compass**, along the top: the heading in degrees, plus N, E, S and W (north is +z).
- **Map**, at the top right: the whole world from above with north up. The ground is shaded by height, or drawn as contours with Real Graphics off. Placed items are dots, characters are rings, and you are an arrow.
- **Main menu > Personal Dimensions > Features:** each can be turned off there (`[personal] map`, `compass`; both on by default).

## Docs written from the code
- `docs/SETTINGS.md` lists every setting, its default and what it is. `tools/settings_doc.py` generates it, and CI step 10 fails if it falls out of date.

## IDEAS
| § | Topic |
| :--- | :--- |
| 19.155 | Settled: AI settings live under Game Systems. |
| 19.192 | Holding, placing and dropping: built. |
| 19.196 | Menus and layouts as data, and "everything dynamic and modular" as a rule (also in the ROADMAP). |
| 19.197 | Map and compass, plus minimap line of sight: roofs above you left out. |
| 19.198 | The maze door, with rules from the fractal dimension's sub-dimensions. |

## Files to delete by hand
- None.
