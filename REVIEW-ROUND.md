# Review round: what to test, and the task list sorted by testing

This covers everything since **sieve-wireframe.zip** (the last round you built, ran and tested): the zips
`sieve-menu-names`, `sieve-hold-place`, `sieve-favourites`, `sieve-bookcase`, `sieve-keys-viewer`, and the
next one (files opened by their kind). Every item below already has CI checks in step 10. What CI
cannot judge is listed under **user**: how it looks, feels and reads.

Categories (IDEAS 19.202):
- **user**: only you can judge it (a look, a feel, a sound). Listed with how to reach it.
- **CI**: the CI steps that reach the files changed (here: step 10, the hallway; step 04 for the menus).
- **unit**: the unit tests (step 01).
- **docs**: nothing to run but the help and settings checks.

---

## 1. For you to test (user)

Tick each box (`[x]`) as it is tested. A change that touches something already tested unticks its box
in the next zip (and says so in CHANGES.md), so the boxes left unticked are always what needs testing now.

- [ ] **Recheck everything.** Tick this, and the next zip unticks every box below for a fresh round (and this one).

### Personal dimension (main menu > Personal Dimensions > Personal Dimension)
- [x] 1. **Walking and looking.** WASD/arrows, mouse, Shift to run, PgUp/PgDn to look up and down. Does it feel right? Is the hill, the sky and the clouds drawn well at your window size?
- [x] 2. **The catalogue (Tab).** Up/Down, Enter places the item in front of you, facing you. Backspace takes away the nearest one. The **bookcase** is at the end of the list.
- [x] 3. **Holding (look at a thing, E or click).**
   - R / T / G turn it (Shift: the other way). Does the turning feel natural?
   - Return or a click places it exactly where it is, even floating in the air.
   - Q drops it: it should fall, bounce a little and come to rest. Walk into it and it is knocked along.
   - Esc puts it back where it was.
   - Hold it up to the bookcase: it should rest on a shelf or on the top.
- [x] 4. **Place panel (Tab while holding).** It slides in like the setup menu's panels. Up/Down choose a row, Left/Right step it (Shift ten times), type a number and Enter, and "place it here". Does the slide match the setup menu's?
- [x] 5. **Map and compass.** The compass along the top, the map at the top right. **+ / -** zoom the map. Turn each off in Personal Dimensions > **Features** and back on.
- [x] 6. **Real Graphics off** (Settings): everything in the dimension should be wireframe (the ground an open grid, objects edged, clouds outlined, on black), and the characters too.
- [ ] 7. **Drawing order.** Walk round the bookcase with things on it: nothing should show through what is in front of it; characters behind objects should be hidden.

### Keys (Settings > Controls > Keys)
- [x] 8. Every key of your dimension is listed by section (moving, catalogue, holding, map). Enter on one, press a new key (try Shift+something), check the footers in the dimension show the new key and that it works. Backspace restores the default. Is the screen readable, and does it scroll well?

### Favourites
- [x] 9. In the hallway, take an item and press **H**: "Kept as a favourite". H again removes it. Try a page, a picture, a model (also with the models line compacted), a world, a book.
- [ ] 10. In your dimension, the catalogue lists the favourites after the shelf items. Place one, look at it, press **O**: the hallway should open on its line with the item in hand **and its viewer open** (a model on the 3D tab).
- [x] 11. Main menu > Personal Dimensions > **Favourites**: every favourite listed, with the Media Player's.

### The viewer (Z, or a click on the item in hand)
- [ ] 12. A **model** or **world**: opens on the **3D** tab. Arrows/WASD turn it; Tab goes to the .obj text and the picture. Is the 3D view large and clear enough?
- [ ] 13. A **file** on the binary line: a PNG/JPEG/GIF/BMP opens as its picture (a GIF plays), a text file as its text, with the bytes (HEX) as the next tab. A file of any other kind opens as hex, and the viewer says a viewer needs making. Check `viewers-needed.txt` beside your settings file.

### Menus
- [ ] 14. Menus picked by name (`Item:ID` in `--press`) changed nothing you can see. Just check the main and pause menus still go where they should, and Back returns to the row you came from.

### Windows only (I cannot run these here)
- [ ] 15. A Release build on Windows: the hallway (the new `keys.cpp` and `picture_source.cpp` are in its build), and that step 11 (check_stat_plugin) now passes on Windows.
- [ ] 16. **Version 0.13.79**: Properties > Details on Windows, and the release package's name.
- [ ] 17. **Replays** (main menu): the words at the top say how to record a replay and that every replay is listed there.
- [ ] 18. **The viewer's keys** at the bottom wrap upward onto more lines at a narrow window, broken only between keys. The 3D view has its own key line.
- [ ] 19. **Personal Dimensions on the hallway's pause menu** opens the Personal Dimensions screen over the hallway (not the setup menu); Esc there, or leaving your dimension entered from there, comes back to the paused hallway.
- [ ] 20. **Esc in your dimension** opens its pause menu, without the hallway's tools; Personal Dimensions > Features > *The hallway's tools in its pause menu* adds them. Try a few rows: Settings and the Filter Designer come back to your dimension; the File Locator, the Harness and the Encryption Engine come back to it after; the Media Player goes into the hallway.
- [ ] 21. **A favourite placed in your dimension shows its picture** on its front (keep a page with H, place it, look at it). Item 7 is unticked as its drawing changed (a depth test at every pixel: watch the frame rate), item 10 as favourites now carry their pictures.
- [ ] 22. **Characters as true 3D spheres**: in the Character Creator (all four drawings), spheres cut into one another where they meet, **pixels** are flat squares of one size and **voxels** are real cubes that turn with the character (try [ and ]), the same cell size for every character; in your dimension, walk so a bookcase stands between you and your character: it hides exactly what it covers; in the hallway with Real Graphics on, your character is shaded spheres among the shelves. Watch the frame rate in each.

---


## 2. Already covered by CI (run once for the round)
- Step 10 (the hallway), about 13 minutes: everything in section 1 that has an output to check.
- Step 04 (the menus): the `Item:ID` presses.
- Step 01 (unit tests), about 5 minutes: nothing of this round's is unit-tested yet (the key registry
  and the favourites' files are checked only through the hallway in step 10). Unit tests for both would
  let later changes to them skip step 10: worth adding in Round B.
- Step 11, about 27 minutes: only for the Windows fix in item 15.

---

## 3. The task list, sorted so each round needs the least testing

Grouped so that each round touches one area, and so the user testing for a round is one sitting in
one place. Within a group, the order avoids going back over earlier work.

### Round A: personal dimensions, finished (CI step 10 + user, one sitting in the dimension)
- A1. Own objects in the catalogue (your own models from the models line, a glTF imported).
- A2. The WORLDS address: your world as an item of the worlds line, openable from the hallway.
- A3. Favourites' covers: each favourite drawn with its own picture, not its line's shelf item.
- A4. Sound files to the audio player and .obj/glTF files to the 3D view (the rest of IDEAS 19.200).

### Round B: keys everywhere (CI steps 04 + 10 + user, one sitting in Settings > Keys)
- B1. The hallway's keys onto the registry.
- B2. The setup menu's, the Media Player's and each other system's, one commit each.
  (Each is mostly mechanical; the user test is "does every key still do what it did, and can it be changed".)

### Round C: formats and grammars (CI steps 01 + 05/06 + unit; no user testing)
- C1. A3 from the old list: grammars and catalogue entries for actions, replay, save-v2, personal-world v1/v2.

### Round D: new systems (CI + user; each needs its own sitting)
- D1. The Character Creator: add/remove limbs and drag joints (IDEAS 19.203).
- D2. Layouts and the fractal dimension (19.191), the maze door (19.198).
- D3. The documentation viewer (19.201).
- D4. Screenshots made by a tool, and the CI runner running only the steps a change reaches (19.202).
  This one would cut testing for every later round, so it could come first in D.

### Later (in ROADMAP's final tasks)
- The main menu reordered and Settings renamed (19.188), face finder (19.193), modelling tools (19.194), the hypercube background (19.195), minimap line of sight (19.197), AI explorers, the website (task 32), v34 freeze (task 118), voice (task 119).

---

## 4. The order of this review (Edward's notes, 13 November 2026)

- [x] 1. A list of the new features for Edward to test as a user (section 1 above).
- [ ] 2. Edward tests them; the fixes that follow are made (each fix zipped, its box ticked when retested).
- [ ] 3. Multi-agent code review 1: optimisations and performance, to speed things up.
- [ ] 4. Multi-agent code review 2: duplicate code, brought to one copy.
- [ ] 5. Multi-agent code review 3: the code compared with the documentation, each made to agree.

The three reviews run one after another, each looking at a different side of the project, and only once
step 2 is done. Each review's findings are checked before anything is changed, its fixes go through
CI, and each ends in its own zip. Round A (section 3) starts after review 3.

---

**Suggested pause:** after this round's zip (files opened by their kind), stop for your review of
section 1, then go on with Round A, or with D4 first if cutting testing time matters most.
