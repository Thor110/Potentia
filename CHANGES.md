# Actions to animations, and the Character Creator (IDEAS 19.186)

## The action map: `sieve-actions-v1`

- Covers seven actions: idle, walk, run, jump, take, read and wave.
- Each action names the animation that plays it, by SHA-256, or `none`.
- An animation **fits** a character when it has no more bones than the character has spheres.
- Maps are stored as `actions/<SHA-256>.actions` beside the settings file, and `[character] actions` names the one in use.
- With `[raid] drives` set, Ctrl+S also keeps the map in the RAID.

## The save: `sieve-save-v2`

- Adds an `actions SHA-256` line, so a custom character brings its actions with it.
- A save without a map is still written as v1, so every existing save keeps its id.

## The Character Creator

- Opens from the main menu (after the System Viewer) or with **C** on the CHARACTER page. It uses the whole window.
- The character's settings are on the left and the character is drawn large on the right; drag, the wheel, or [ and ] turn it.
- The seven actions are listed below the settings, each with its animation:
  - **Tab** switches between the settings and the actions.
  - **Left/Right** go through `none` and every animation in your store that fits. One that doesn't fit, or isn't in the store, is shown in red.
  - **Enter** plays the chosen action's animation, or says there is none.
- Ctrl+S/O/I/K work as on the CHARACTER page. **Esc** goes back to where you opened it.

## In the hallway

- With a map, your character plays idle when you stand, walk when you move, run with Shift, and read while you hold an item.
- An action with no animation plays nothing, and the hallway says so once for each action.
- Without a map, its one animation plays whatever it does, as before.
- For pictures: `--action NAME` holds one action, and `--creator` (with `--menu`) opens the creator.

## Still to come

- Jump, take and wave are in the map but not yet triggered in the hallway.
- Characters having their own extra actions (emotes) would be a later version.

## Checked

- Unit tests for the format and save v2.
- CI 01, 04 and 10 passed, with new checks for the main menu row, C on the CHARACTER page, setting idle and walk in the creator, the map kept, and the hallway playing walk and saying jump has none.

## Files

- New: `core/include/sieve/actions.hpp` and `core/src/actions.cpp`. Re-run CMake.
- No files to delete.
