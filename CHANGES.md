# Changes: Online off by default, Game Systems, and your personal dimension's world

## Online, off by default (Settings > Online)
- A new setting, `[online] enabled`, off by default. Nothing in Sieve needs the network.
- While Online is off, these do nothing and say "Online is off", even where ticked:
  - Infinite RAID's sharing;
  - keeping others' saves;
  - Check my shares.
- The RAID on your own drives needs no network and is unaffected.

## Game Systems (Settings > Game Systems)
- A screen of its own, holding **AI Explorers** (`[game] ai_explorers`, off by default, "(to come)").
- IDEAS §19.189 notes what it is for. Nothing reads the toggle yet.

## Your personal dimension's world (IDEAS §19.187)
- Main menu > Personal Dimensions > Personal Dimension now opens a 3D world rather than the setup menu.
  - The world: a grassy hill under a blue sky, with clouds drifting.
  - Your character stands before you, and your store's characters round the top of the hill.
  - Keys: WASD or the arrows walk, the mouse looks, Shift runs, Esc goes back to the main menu.
- **Catalogue:**
  - Tab opens it. It lists Sieve's shelf items, one per line: a sheet of paper, a canvas, a record sleeve, a tape case, a book, crates and a box.
  - Up/Down choose an item and Enter places it before you, facing you.
  - Backspace takes away the nearest item within 8 m.
- **The items are preserved apart from the overridable meshes:**
  - They live in `data/objects/shelf-items.sieve`: the 22 .obj/.mtl files (21,060 bytes), packed as 2,014 bytes.
  - The file is read in memory, with every member checked against its SHA-256.
  - Changing Real Graphics' meshes folder never changes them.
  - The build copies the folder beside the program as `objects/`, and releases include it.
- **What you place** is kept in `worlds/personal.world` beside the settings, as `sieve-personal-world-v1` (SPECIFICATIONS §12.0r), and is read back next time.
- **Picture option:** `--personal-world --screenshot FILE`, with `--world-file`, `--pworld-walk S`, `--pworld-back S` and `--press`.
- IDEAS §19.187 records your aim: a lens for people's own interpretation of the Sieve space, for games, or just spaces to hang out in.

## Docs and tests
- IDEAS §19.187 (built), §19.189 (new); ROADMAP 5b done.
- IDEAS, planned and not built yet:
  - §19.190: favourites in every dimension, and one combined catalogue of them.
  - §19.191: layout rulesets and the fractal dimension (data + ruleset, ruleset + ruleset).
  - §19.192: picking up, placing, rotating and dropping items, with physics, for authors and visitors.
- SPECIFICATIONS §12.0r; MANUAL; INFINITE-RAID.md (the Online gate).
- CI step 10:
  - the Online gate (serve ticked without Online answers nothing);
  - placing, keeping, re-reading and removing objects;
  - a bad world-file line left out.

## Files to delete by hand
- None.
