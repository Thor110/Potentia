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
- [x] 19. **Personal Dimensions on the hallway's pause menu** opens the Personal Dimensions screen over the hallway (not the setup menu); Esc there, or leaving your dimension entered from there, comes back to the paused hallway.
- [ ] 20. **Esc in your dimension** opens its pause menu, without the hallway's tools; Personal Dimensions > Features > *The hallway's tools in its pause menu* adds them. Try a few rows: Settings and the Filter Designer come back to your dimension; the File Locator, the Harness and the Encryption Engine come back to it after; the Media Player goes into the hallway.
- [ ] 21. **A favourite placed in your dimension shows its picture** on its front (keep a page with H, place it, look at it). Item 7 is unticked as its drawing changed (a depth test at every pixel: watch the frame rate), item 10 as favourites now carry their pictures.
- [ ] 22. **Characters as true 3D spheres**: in the Character Creator (all four drawings), spheres cut into one another where they meet, **pixels** are flat squares of one size and **voxels** are real cubes that turn with the character (try [ and ]), the same cell size for every character; in your dimension, walk so a bookcase stands between you and your character: it hides exactly what it covers; in the hallway with Real Graphics on, your character is shaded spheres among the shelves. Watch the frame rate in each.
- [ ] 23. **Your character is you** in the hallway: not drawn (no figure ahead and to your right); the hallway's scale still follows its size, and its actions still play (the output says `your character:`).
- [ ] 24. **The System Viewer from your dimension's pause menu** comes back to your dimension on Esc (it went to the main menu). Not checked by CI: the return is beyond what a screenshot run reaches.
- [ ] 25. **Your characters are objects in your dimension**: yours stands ahead of where you begin, as before. Look at it, E picks it up, R/T/G turn it, Return places it, Q drops it, Backspace takes it away (and it stays away next time); the catalogue (Tab) ends with "your character" and your store's, to place them again. Change your character on the CHARACTER page and the one in your world changes with it.

---


## 2. Already covered by CI (run once for the round)
- Step 10 (the hallway), about 13 minutes: everything in section 1 that has an output to check.
- Step 04 (the menus): the `Item:ID` presses.
- Step 01 (unit tests), about 5 minutes: nothing of this round's is unit-tested yet (the key registry
  and the favourites' files are checked only through the hallway in step 10). Unit tests for both would
  let later changes to them skip step 10: task M1 in section 3.
- Step 11, about 27 minutes: only for the Windows fix in item 15.

---

## 3. The task list, in lists by area, each task with the CI it needs

(Edward, 14 November 2026: "categorise work to be done ... align the tasks we need to perform with one
another to reduce the need for running the full CI every time ... we might end up picking a task or
multiple tasks from each list, or sometimes we might pick only tasks from one list".)

**How a batch is picked.** Each task says which CI steps check it (`[10]` is step 10, the hallway, about
13 minutes; `[01]` the unit tests, about 5; `[04]` the menus, quick; `[11]` the oracle, about 27) and where
you test it. A batch is any tasks, from one list or several, and is checked by the steps its tasks name
together, not the full CI: tasks that all say `[10]` cost one run of step 10 between them. The full CI is
run before a release package, and after each of the three reviews. Each batch is still one zip.

### List P: your personal dimension `[10]` + you, one sitting in the dimension
- P1 (R2). The ground shaper: key 1 takes it out, # puts it away (IDEAS 19.209).
- P2 (R3). O opens the hallway's viewer in place, "calculating..." by the crosshair (19.208).
- P3. Own objects in the catalogue (your models from the models line, a glTF imported).
- P4. The WORLDS address: your world as an item of the worlds line, openable from the hallway.
- P5. Music for your dimension: a third space in the Media Player, kept in the dimension's file (19.220). Also `[07]`.
- P6. Favourites' covers in the catalogue (their pictures are on their fronts once placed, since F7).
- P7. Coloured rings round what you hold, one a turn, and the mouse wheel turning along the one chosen (19.221).
- P8. Day and night, on or off, the day's length set and kept in the dimension's file (19.222).

### List R: characters and the renderer `[10]` + you, one sitting in the Character Creator
- R1 (R4). The Character Creator's view tilted as well as turned (diagonal rotation).
- R2. The four drawings as one global setting under Settings > Graphics, as well as in the Creator.
- R3. The voxels' cell size made the same for everything, with that switchable (19.215), then greedy meshing.
- R4. Add/remove limbs and drag joints (19.203).
- R5. A shell grown over the spheres, for quads and smooth surfaces (19.214).

### List M: menus and keys `[04]` + `[10]` + you, one sitting in Settings > Keys
- M1. The hallway's keys onto the registry; unit tests for the registry and the favourites' files, so
  later changes to them need `[01]` rather than `[10]`.
- M2. The setup menu's, the Media Player's and each other system's keys, one commit each.
- M3. The main menu reordered and Settings renamed (19.188).

### List V: viewers `[10]` + you, one sitting with files of each kind
- V1. Sound files to the audio player, .obj/glTF files to the 3D view (the rest of 19.200).
- V2. The documentation viewer (19.201).

### List F: formats and grammars `[01]` + `[05]`/`[06]` (+ `[11]` where the reader changes); no sitting
- F1. Grammars and catalogue entries for actions, replay, save-v2, personal-world v1/v2 (old A3).
- F2. The save formats fixed for the RAID's sieved saves (19.210), before anything is shared.
- F3. v34 measured and frozen (task 118).

### List S: new spaces `[10]` + you, a sitting each
- S1. Layouts and the fractal dimension (19.191); S2. the maze door (19.198).

### List T: testing itself (its own CI change, checked by running it)
- T1. Screenshots made by a tool, and the CI runner running only the steps a change reaches (19.202).
  This is what makes batches cheap without picking the steps by hand, so it could go into the first batch.

### Later (in ROADMAP's final tasks)
- Face finder (19.193), modelling tools (19.194), the hypercube background (19.195), minimap line of sight (19.197), interface designer (19.206), plugin dimensions (19.217), AI explorers, the website (task 32), voice (task 119).

---

## 4. The order of this review (Edward's notes, 13 November 2026)

- [x] 1. A list of the new features for Edward to test as a user (section 1 above).
- [ ] 2. Edward tests them; the fixes that follow are made (each fix zipped, its box ticked when retested).
- [ ] 3. Multi-agent code review 1: optimisations and performance, to speed things up.
- [ ] 4. Multi-agent code review 2: duplicate code, brought to one copy.
- [ ] 5. Multi-agent code review 3: the code compared with the documentation, each made to agree.

The three reviews run one after another, each looking at a different side of the project, and only once
step 2 is done. Each review's findings are checked before anything is changed, its fixes go through
CI, and each ends in its own zip. The batches from section 3 start after review 3.

---

**Suggested pause:** after this round's zip (files opened by their kind), stop for your review of
section 1, then pick the first batch from section 3 (T1 in it if cutting testing time matters most).
