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
- [ ] 1. **Walking and looking.** WASD/arrows, mouse, Shift to run, PgUp/PgDn to look up and down. Does it feel right? Is the hill, the sky and the clouds drawn well at your window size?
- [ ] 2. **The catalogue (Tab).** Up/Down, Enter places the item in front of you, facing you. Backspace takes away the nearest one. The **bookcase** is the last of Sieve's own objects, before your favourites and your characters.
- [ ] 3. **Holding (look at a thing, E or click).**
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
- [ ] 8. Every key of your dimension is listed by section (moving, catalogue, holding, map). Enter on one, press a new key (try Shift+something), check the footers in the dimension show the new key and that it works. Backspace restores the default. Is the screen readable, and does it scroll well?

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
- [ ] 22. **Characters as true 3D spheres**: in the Character Creator (all four drawings), spheres cut into one another where they meet, **pixels** are flat squares of one size and **voxels** are real cubes that turn with the character (try [ and ]), the same cell size for every character; in your dimension, walk so a bookcase stands between you and your character: it hides exactly what it covers; in the hallway with Real Graphics on, your store's characters are shaded spheres among the shelves (yours is you, not drawn, since item 23). Watch the frame rate in each.
- [ ] 23. **Your character is you** in the hallway: not drawn (no figure ahead and to your right); the hallway's scale still follows its size, and its actions still play (the output says `your character:`).
- [ ] 24. **The System Viewer from your dimension's pause menu** comes back to your dimension on Esc (it went to the main menu). Not checked by CI: the return is beyond what a screenshot run reaches.
- [ ] 25. **Your characters are objects in your dimension**: yours stands ahead of where you begin, as before. Look at it, E picks it up, R/T/G turn it, Return places it, Q drops it, Backspace takes it away (and it stays away next time); the catalogue (Tab) ends with "your character" and your store's, to place them again. Change your character on the CHARACTER page and the one in your world changes with it.
- [ ] 26. **The Character Creator tilts** (and the CHARACTER page): drag up and down, Shift+[ and Shift+], or Shift with the wheel, 15 degrees a press up to 85 either way; **Home** puts the view back as it opens. Try each drawing tilted; the voxels and the pedestal should look right from above and below.
- [ ] 27. **The ground shaper** (P1) in your dimension: **1** takes it out, **#** (or the backslash) puts it away. An amber ring on the grass marks the brush; the left button or Z raises, the right or X lowers, the middle or C flattens; the wheel or [ and ] change its size. Walk away and back, leave and come in again: the shape stays. Size and strength are on Personal Dimensions > Features. Then take it out, aim at a placed favourite, press O, and close the viewer with Z: the ground should not rise.
- [ ] 28. **O opens a favourite in place** (P2): a red "calculating" with dots by the crosshair, then the viewer over your dimension; Esc or Z closes it and you are where you stood. A second favourite on the same line opens at once. Item 10 changes with it: Personal Dimensions > Features > *O opens a favourite in place* off goes into the hallway as before.
- [ ] 29. **Your own models** (P3): with the catalogue open, **I** opens the file dialog (the dialog is not tested by CI); pick an .obj (with its .mtl beside it) or a glTF. It is listed after the characters, chosen at once, and Return places it. Is the size right? (An .obj's units are taken as metres.)
- [ ] 30. **Your world on the worlds line** (P4): **N** shows its address; Return goes into the hallway on the worlds line with your world in hand, from the main menu and from the hallway's pause menu (neither is tested by CI, only the scripted way).
- [ ] 31. **Music for your dimension** (P5): Media Player, Mode Right past WORLD to **PERSONAL**; "Music here" gives the dimension music of its own, written into its file. Go into the dimension: does it play that music, and WORLD's again once "Music here" is pressed off? (Not heard by CI: its player is silent.)
- [ ] 32. **Favourites' covers** (P6): Tab in your dimension, each favourite with a picture shows a small copy left of its row and the chosen one larger below the list. Off on Features.
- [ ] 33. **Turning rings** (P7): hold something; three rings round it (x red, y green, z blue). After R, T or G that ring is bright and the wheel turns it 15 degrees a notch. Settings > Graphics > Axis Colours steps to Okabe and Ito's; Features has the rings on/off and the wheel's step.
- [ ] 34. **Day and night** (P8): Features > Day and night on; the sun, sky, light and stars follow the time of day (UTC). **L** sets the day's length in seconds (try 60 to watch a whole day). Is the night too dark or too light? Off, the dimension looks as before.
- [ ] 35. **One drawing everywhere** (R2): Settings > Graphics > **Character Drawing** (the last row before Back) and the Creator's drawing row change the same setting. Choose voxels: the Creator, the CHARACTER page, your dimension and your stored characters in the hallway all draw cubes. Back to shaded, everything looks as before.
- [ ] 36. **Voxel size and joined faces** (R3): Settings > Graphics > **Character Voxels**. Voxel Size on the world lattice (Lattice Base, Thinnest Part) and Join Voxel Faces on: the picture should look the same, with the readout saying fewer faces. Does the lattice look right on your characters? (Seed 12345 breaks apart on it. The height rule stays the default, decided 14 November: see IDEAS 19.216.)
- [ ] 37. **The skin** (R6): drawing **skin** (one Right from shaded), then Skin Blend (0 to 200%) and Skin Detail on Character Voxels. Is the surface smooth where spheres meet, and does it follow the joints while an action plays?
- [ ] 38. **Quads and smooth** (R5): drawings **quads** and **smooth** (two and three Rights from shaded): quads drawn with darker sides, smooth lit softly across each face. Its outline still shows the skin's facets (subdivision is left for later).
- [ ] 39. **Editing by the joints** (R4), in the Creator: click a sphere to choose it, drag it (the limb beyond follows); **.** and **Comma** step through spheres, **N** grows a limb, **X** takes it away, **J** / **Shift+J** add or take out a joint, **T** stretching, **Ctrl+arrows** nudge, **Ctrl+Z** undo. A bilateral or radial character changes on every side at once; Ctrl+S saves it as a v4 character. Settings > Controls > Keys lists these and the Creator's own keys (A, [ ], Home, R, G, the files); give one an editing key's key and that screen names the clash in red.
- [ ] 40. **The hallway's keys** (M1): Settings > Controls > Keys, after the Creator's rows, five sections for the hallway (walking, anywhere, empty-handed, holding, typing). Change one (try take, E, to Y, or the thousand-tile jump to J): the footer, the item's HELP and COST tabs, and Settings > Controls name the new key, and it works. Backspace on the row gives the default back. Esc, the mouse and the wheel stay fixed.
- [ ] 41. **Every other screen's keys** (M2): the menus (Up, Down, Enter, Esc...), the setup menu, the Media Player, the rings, the Encryption Engine and the CHAIN tab have sections of their own. Give the setup menu's "the row below" a letter (J): it still moves down, even off the key's row, where other letters are typed into the key.
- [ ] 42. **The main menu's new order** (M3): Start Sieve, File Locator, Media Player, Filter Designer, AI Training Harness, Personal Dimensions, Character Creator, Replays, Encryption Engine, Manual Navigation, System Viewer, Potentia Settings, Exit Sieve. In a small window (1024x600) every row should fit above the footer (the rows are drawn closer); at 1280x720 they are a little closer than before too.
- [ ] 43. **Wireframe** (M4): Settings > Graphics > **Wireframe**, on by default (it is the old Real Graphics, turned round). An older settings file with `real_graphics = on` should open with Wireframe off, and `--real-graphics` still works beside the new `--wireframe`.
- [ ] 44. **Footers wrap** (M5): make the window narrow (800 wide) on each screen: the line of keys at the foot breaks between keys onto more lines, upwards, never off the edge. The File Locator's and the Media Player's own buttons do not move up yet (IDEAS 19.224).
- [ ] 45. **Limbs** (R7): Potentia Settings > Graphics > Character Voxels > **Limbs** on, then the skin, quads or smooth drawing in the Creator: each limb becomes one smooth tube from sphere to sphere, the gaps bridged (try seed 12345, whose long limb was a row of balls). The head joins the body as a limb does, so they become one dome: say if you would rather the head stay apart. Off, the skin is exactly as before.
- [ ] 46. **Skin Smoothing** (R8): the row after Limbs, 0 (off) to 20 rounds. The quads grow more even and the light softer; the knuckles where sphere sizes change stay. The Creator's readout gives the volume before and after (it should stay within about 1%).
- [ ] 47. **Skin Subdivision** (R9): the row after Skin Smoothing, 0 to 3 steps, for the quads and smooth drawings: the outline goes from faceted to round. A fine skin may be given fewer steps than asked (the readout says so). In your dimension and the hallway the bound is shared by the characters in sight, so with many placed each is given fewer steps: watch the frame rate there with 1 and 2 steps. At 2 or 3 steps the quads drawing's many small edges darken it (left for later).
- [ ] 48. **Sound files in the viewer** (V1): on the binary line, warp to a WAV, a FLAC and, if you have one, an AIFF (T, then the file's path; the line's length must be at least the file's), pick it up (E) and open it (Z): its waveform, a band a channel. **Space** plays it (through the music, which fades out under it) and again stops it; closing the viewer stops it too. Try a long one (over about 12 minutes of CD sound): it is refused and shown as its bytes, saying why. An MP3 is still shown as its bytes and noted in `viewers-needed.txt`.
- [ ] 49. **MIDI files in the viewer** (V1): the same with a .mid: its notes, each voice on its own lines, at the tempo the file sets first; Space plays them (square tones, as the audio line plays notes). Try one with chords or many tracks: a voice plays one note at a time and there are at most 15 voices, so a chord's other notes and the tracks past the fifteenth are left out, and the foot of the screen says how many.
- [ ] 50. **Model files in the viewer** (V1): the same with an .obj, a .glb and a glTF whose buffer is inside it: each opens on its 3D view, turned with the arrows or WASD; Tab gives the .obj's or glTF's text. A glTF whose buffer is a file beside it says it cannot be read from its bytes and opens as its text.
- [ ] 51. **The Documentation** (V2): Main menu > **Documentation**, before Potentia Settings. Read a few pages from CONTENTS (Up/Down, Enter); follow a link (Right to the page, Tab to a link, Enter) and come back (Backspace); look a word up in INDEX (Tab on the list); type a word to search. The 25 pages were written from the manual for the person using Sieve: say where a page is wrong, unclear or missing, and which pages you would want next. Change a key the pages name (Settings > Controls > Keys) and see the page show it as set.

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
- [x] P1 (R2). The ground shaper: key 1 takes it out, # puts it away (IDEAS 19.209). *Made 14 November 2026.*
- [x] P2 (R3). O opens the hallway's viewer in place, "calculating..." by the crosshair (19.208). *Made 14 November 2026.*
- [x] P3. Own objects in the catalogue (your models from the models line, a glTF imported). *Made 14 November 2026.*
- [x] P4. The WORLDS address: your world as an item of the worlds line, openable from the hallway. *Made 14 November 2026.*
- [x] P5. Music for your dimension: a third space in the Media Player, kept in the dimension's file (19.220). Also `[07]`. *Made 14 November 2026.*
- [x] P6. Favourites' covers in the catalogue (their pictures are on their fronts once placed, since F7). *Made 14 November 2026.*
- [x] P7. Coloured rings round what you hold, one a turn, and the mouse wheel turning along the one chosen (19.221). *Made 14 November 2026.*
- [x] P8. Day and night, on or off, the day's length set and kept in the dimension's file (19.222). *Made 14 November 2026.*
- P9. Import and export: any object as its parts (a page as .png and .txt, a model as .obj), the whole world as one .obj with a sub-object each and their files beside it (textures, texts, audio, shelf weights as .safetensors), a manifest of SHA-256s; brought back the same way (19.226). Also `[01]`.

### List R: characters and the renderer `[10]` + you, one sitting in the Character Creator
- [x] R1 (R4). The Character Creator's view tilted as well as turned (diagonal rotation), and Home to reset it. *Made 14 November 2026.*
- [x] R2. The four drawings as one global setting under Settings > Graphics, as well as in the Creator. *Made 14 November 2026.*
- [x] R3. The voxels' cell size made the same for everything, with that switchable (19.215), then greedy meshing. *Made 14 November 2026.*
- [x] R4. Add/remove limbs and drag joints (19.203). *Made 14 November 2026.*
- [x] R5. A shell grown over the spheres, for quads and smooth surfaces (19.214). *Made 14 November 2026.*
- [x] R6. The skin: a stage of the mesh scaling system wrapping the spheres in one smooth surface (a field, marching cubes), its blend a setting, measured against the other drawings (19.225). R5's first step. *Made 14 November 2026.*
- [x] R7. Limbs that join their spheres: a rounded cone for each bone in the skin's field, a switch (19.229). *Made 15 November 2026.*
- [x] R8. Smooth plus: the skin's surface smoothed (Taubin), its rounds a setting (19.229). *Made 15 November 2026.*
- [x] R9. The quads subdivided (Catmull–Clark) so the outline is round too, its steps a setting (19.229). *Made 15 November 2026.*

### List M: menus and keys `[04]` + `[10]` + you, one sitting in Settings > Keys
- [x] M1. The hallway's keys onto the registry; unit tests for the registry and the favourites' files, so
  later changes to them need `[01]` rather than `[10]`. *Made 14 November 2026.*
- [x] M2. The setup menu's, the Media Player's and each other system's keys, one commit each. *Made 14 November 2026.*
- [x] M3. The main menu reordered and Settings renamed (19.188). *Made 14 November 2026.*
- [x] M4. "Real Graphics" renamed "Wireframe", its sense turned round, old files and `--real-graphics` still read (19.223). Also `[03]`, `[11]` where their lines name it. *Made 14 November 2026.*
- [x] M5. Every footer of keys wraps upwards as the viewer's does, on every screen, with a CI check that no footer line is wider than its window (19.224). *Made 14 November 2026.*

### List V: viewers `[10]` + you, one sitting with files of each kind
- [x] V1. Sound files to the audio player, .obj/glTF files to the 3D view (the rest of 19.200). *Made 15 November 2026.*
- [x] V2. The documentation viewer (19.201). *Made 15 November 2026.*

### List F: formats and grammars `[01]` + `[05]`/`[06]` (+ `[11]` where the reader changes); no sitting
- F1. Grammars and catalogue entries for actions, replay, save-v2, personal-world v1/v2 (old A3). *Made 15 November 2026: actions, save (v1 and v2), replay, and personal-world v1 and v2 (FILTER-PLUGINS §23).*
- F2. The save formats fixed for the RAID's sieved saves (19.210), before anything is shared. *Made 15 November 2026: `sieve-raid-piece-v1` and `sieve-raid-tally-v1` (SPECIFICATIONS 12.0s), the code, its placement and leaving rules and their tests; not switched on.*
- F3. v34 measured and frozen (task 118). *Frozen 12 November 2026; measured whole 15 November 2026 (results/v34-measured-2026-11-15.md).*

### List S: new spaces `[10]` + you, a sitting each
- S1. Layouts and the fractal dimension (19.191); S2. the maze door (19.198). *Agreed 15 November 2026: the fractal dimension first, then layouts (for personal dimensions), then the maze door. Step 1 made the same day: the fractal dimension's grammar, shelves and `sieve fractal`, then reworked as the hallway's FRACTALS line (its door after AI, its index book first, a new listed kind of line any later dimension can be) (SPECIFICATIONS 12.0t). Step 2, layouts, talked through and delayed the same day (IDEAS 19.191: own space behind a placed doorway, a sub-dimension, its line's shape fixed and stored; the rooms from `Previous/models/`): the personal dimension needs dedicated work first.*

### List T: testing itself (its own CI change, checked by running it)
- [x] T1. Screenshots made by a tool, and the CI runner running only the steps a change reaches (19.202). *First step made 14 November 2026: `ci/run.sh --changed`, `tools/screenshots.py`.*
  This is what makes batches cheap without picking the steps by hand, so it could go into the first batch.

### List O: optimisations left by the reviews (Sieve/docs/OPTIMISATIONS.md), each with its own CI
- O1. RAID reads buffered `[04, 10]`; O2. sharing's questions sent together `[01, 04, 10]` (*O1, O2 and O6
  made 15 November 2026*); O3. the Creator's
  picture the size of its box `[04, 10]`; O4. recordings kept as text `[10]`; O5. edges walked only in
  their band `[04, 10]` (*O3 and O5 made 15 November 2026*; *O4 the same day*); O6. held pieces written outside the lock `[04, 10]`; O7. the trial mixers measured
  on a large file (a measurement, no CI; *measured the same day: no change at the peak on enwik8, so
  nothing to change*). *List O done.*
  Each sits well with others of the same steps: O1, O2 and O6 together are one RAID batch; O3 and O5 go
  with list R.

### Last on the list: the fractal dimension reviewed
- X1. The fractal dimension reviewed, and its line made every ruleset of the grammar, generated (Edward, 15
  November 2026, IDEAS 19.235): the charted ones found in it by address, the index book still naming them,
  and the known fractals as its first filter, with exclude, include and only (*made 16 November 2026:
  fractal-space-v1, SPECIFICATIONS 12.0t*; *then the index under the setting too, Warp naming the setting
  that keeps a fractal off the line, and the magnifying glass back with noise filters, 16 November 2026*);
  a second filter, set from the
  fractal line, turning its cross-dimensional filtering off, on to find what the fractals make in the other
  lines (include), or on to knock it out (exclude) (Edward, 16 November 2026; *made for the image line, 16
  November 2026: fractal-drawn-v1 and the in other lines row; the other lines to follow*). `[01]` + `[10]`.
- X2. Then as many known fractals charted as can be tracked down (Edward, the same day: "implement as many
  fractals as we can track down", and "X1/X2 can also include charting more known fractals when we are
  done"): each a `sieve-fractal-v1` ruleset where the grammar can write it, the grammar widened (a new
  version) where a well-known fractal needs what it cannot say; each one charted becomes another named place
  in X1's generated line. Spider webs among them if they can be (Edward, the same day; IDEAS 19.235: an orb
  web's spiral needs a step that shrinks, a widened grammar) (*made 16 November 2026: sieve-fractal-v2,
  an angle with a tenth or a scaled step, its rulesets after v1's in fractal-space-v2; 75 charted, each
  with where it was transcribed from (The Algorithmic Beauty of Plants, Wikipedia, Fractint's files), an
  orb web, the Pythagoras tree and the H tree written for Sieve; the box-counting dimension measured, and
  a filter by it*). `[01]` + `[10]`.
- X3. The fractals v2 cannot write (Edward, 16 November 2026: "we should work towards the fractals that cant
  be written also"): a grammar widened again (sieve-fractal-v3) for those X2 set aside (a swap of left and
  right turns, turns by stated degrees and several step ratios, angles as fractions of a turn, longer
  rules, stochastic and context-sensitive rules, the turtle in three dimensions), its rulesets in the
  whole space after v2's, and those fractals charted (*made 16 November 2026: sieve-fractal-v3 and
  fractal-space-v3; 32 more charted, 107 in all, from Fractint's files and The Algorithmic Beauty of
  Plants, eight of X2's written again exactly; their notes in Sieve/docs/FRACTAL-SOURCES.md; still not:
  ABOP's row of trees and chapter 2 trees, the Kolakoski sequence, the Fibonacci word fractal*). `[01]` + `[10]`.
- X4. Every fractal ever found, known or theorised (Edward, 16 November 2026: "we should seek to include every
  possible fractal ever found, known or even theorised, including charting ones with larger character
  lengths"; IDEAS 19.235): the families beyond rewriting, a family at a time, each charted from its sources.
  First (X4): iterated function systems, escape-time formulas (Mandelbrot and Julia sets, the Burning Ship,
  the tricorn, Multibrots, magnets, Collatz) and convergence formulas (Newton's), as sieve-fractal-v4, its
  rulesets in the whole space after v3's (fractal-space-v4) with a grammar that passes them exactly (*made 16
  November 2026: sieve-fractal-v4, fractal-space-v4 and format-fractal-v4; 48 more charted, 155 in all, from
  Fractint's fractint.ifs and built-in types and from Wikipedia, their notes in Sieve/docs/FRACTAL-SOURCES.md*). Then
  (X5) parametric L-systems for ABOP's trees, strange attractors, circle inversion, three-dimensional sets
  and random ones (*strange attractors, flows and aggregates made 16 November 2026: sieve-fractal-v6,
  fractal-space-v6 and format-fractal-v6; 10 charted, 197 in all; parametric L-systems, circle inversion
  and the three-dimensional sets left*). `[01]` + `[10]` + `[11]`.
- Y1. CI steps split further (Edward, 16 November 2026, IDEAS 19.236: "splitting the test scripts down even
  further than the current 11 odd steps into step 1a,b etc"): step 01's core suite as eleven parts, each its
  own ctest entry, so a failure names its part (*made 16 November 2026*; steps 04, 10 and 11 by line, left).
  `[01]`.
- Y2. Cellular automata as a family of the fractal dimension, not a dimension (Edward, 16 November 2026:
  "perhaps cellular automata should be considered fractals instead"; IDEAS 19.239), and depth as time for
  every family (*made 16 November 2026: sieve-fractal-v5, fractal-space-v5 and format-fractal-v5; 32 charted,
  187 in all; `sieve fractal draw --all-depths`*). `[01]` + `[04]` + `[10]` + `[11]`.
- Y4. Filters for the new families (Edward, 16 November 2026: "insert after 3 working on filters for the new
  dimensions as we still need to do that"): the noise filters for automata, attractors, flows and aggregates,
  and fractal-moving-v1 on the video line, a charted fractal growing, set with fractal-drawn-v1 (*made 16
  November 2026*). `[01]` + `[10]`.
- Z1. Every colouring of each fractal (Edward, 16 November 2026, IDEAS 19.244): fractal-drawn-v2 and
  fractal-moving-v2, each picture and video also turned, mirrored and inverted, or in any colouring of its
  shape (*made 17 November 2026*). `[01]` + `[04]` + `[10]`.
- Z2. The models line (Edward, 17 November 2026: "in theory those fractals are already there, so culling
  would make the most sense"): the charted fractals' meshes found on the models line, or culled from it
  (*made 17 November 2026*). `[01]` + `[04]`.
- Z3 to Z7 (Edward, 17 November 2026: "go ahead with 1 through 5 next"; "the filters dictate what we get and
  how we find it so the more the better"), *made 17 November 2026*:
  - Z3. Noise filters for the new families: fractal-strange-v1 (attractors and flows) and fractal-lasts-v1
    (automata). `[01]`.
  - Z4. The fractals in the pages and audio lines: fractal-written-v1 and fractal-scored-v1 (as Prusinkiewicz
    scored L-systems), set by the third FRACTALS row. `[01]` + `[04]` + `[10]`.
  - Z5. The video line's own: video-moves-v1 and video-smooth-v1, counted exactly and judged. `[01]` + `[10]`.
  - Z6. The sound line's: sound-crossings-v1 and sound-bend-v1 (IDEAS 19.232). `[01]` + `[07]`.
  - Z7. The pages': few-values-v1, two-symbol-run-v1 and golden-word-v1 (IDEAS 19.232, 19.235); the bits'
    other order left to be measured. `[01]` + `[10]`.
- Z8 and Z9 (Edward, 17 November 2026: "look for any more ideas for filters that are lying around and work on
  those", then "design as many as you can ... make meaningful content pop out of the walls"), *made 17
  November 2026* (IDEAS 19.245):
  - Z8. From the documents: life-causal-v1 (video) and image-mirror-v1 (pictures). `[01]` + `[10]`.
  - Z9. Designed: pronounceable-v1, letter-case-v1, brackets-v1 and word-length-v1 (pages), video-pan-v1, melody-motif-v1
    and sound-periodic-v1; the next round's designs listed in IDEAS 19.245. `[01]` + `[10]`.

### List W: filters that count together (Edward, 17 November 2026; IDEAS 19.247)

- [x] W1. >> OPTIMISE FILTERS << under FIND MY LIMITS: X's function, ending with every filter ticked, and
  each stack kept within the filter memory. `[04]` + `[10]`.
- [x] W2. A filters page no longer counts at every tick: "counted when this page is closed". `[10]`.
- [x] W3. The fractals' lists in other lines as automata, so that they merge. `[01]` + `[10]`.
- [x] W4. Every filter with a ranker of its own looked at; those that cannot count moved to the
  judge-only tab (*all done 17 November 2026: the pages' and melodies' fractal lists merge; the pictures', videos', neighbour-agreement's, utf8-valid's and rgb24's stay their own, with reasons in IDEAS 19.247*). `[01]`.
- [x] W5. The filter that overfills the filter memory named. `[10]`.
- Kept in mind throughout (Edward, the same day, IDEAS 19.235): how a fractal translates into each other
  dimension's data, in many ways, so it can be filtered out of them; every system and dimension added asks
  what its fractal ways are.

### List F: the fractals' filters counted (Edward, 17 November 2026; IDEAS 19.250)

- [x] F-A. fractal-draws, -grows, -no-idle and -canonical counted exactly on v1 and v2, by the set the axiom
  reaches; checked against every ruleset of the smallest lengths, judged by the filters. `[01]` + `[10]`.
- [x] F-A3. The same on v3 (nearly all of the line): the four off the judge-only tab. `[01]` + `[04]` + `[10]`.
- [ ] F-B. Compact on the fractals line: the k-th survivor and back. *Left for later (Edward, 18 November 2026:
  "I think we should leave it for now, that all sounds very heavy indeed"): numbering the survivors needs the
  walk's tables (about 0.5 GB a filter set at 229 characters) or counts made on demand, seconds a region.*

### List P: page filters, the next round (Edward, 18 November 2026; IDEAS 19.245)

- [x] P1. word-shape-v1, punctuation-v1, letters-and-digits-v1: automata, measured on prose, checked against
  their rules written plainly. `[01]` + `[10]`. A sentence's length and verse not built (IDEAS 19.245 says why).

### List Q: picture filters (Edward, 18 November 2026; IDEAS 19.245)

- [x] Q1. border-v1, specks-v1, gradient-v1: automata, checked against their rules written plainly on every
  picture of small shapes and on random 10 x 10 ones. `[01]` + `[10]`. Border and gradient pick a kind of picture,
  so X and OPTIMISE FILTERS leave them (picks_a_kind) `[04]`. The horizon not built (IDEAS 19.245 says why).

### List M2: melody and sound filters (Edward, 18 November 2026; IDEAS 19.245)

- [x] M2-1. sound-envelope-v1, sound-syllables-v1 (counted exactly block by block; automata where their tables fit),
  sound-interval-v1 (judged), measured on random sound and on clips of speech, notes, chords, tones and noise
  `[01]` + `[07]`.
- [x] M2-2. melody-cadence-v1 (a plugin, checked against the oracle `[11]`) and melody-contrary-v1 (judged on all
  voices at once, FilterSpec::whole_unit) `[01]` + `[07]`.

### List N: the night's filters (Edward, 18 November 2026; IDEAS 19.251 and 19.252)

- [x] N1. degree-window-v1: one stretch of any line of digits by bearing, counted over whatever else is ticked
  (a WindowRanker over the stack's ranker), checked against every unit of small lines, alone, over plugins, two
  windows at once, and two voices `[01]` + `[03]`.
- [x] N2. `sieve window`: a unit's bearing and the windows about it, weighed (route in the window plus naming it,
  against the route without one), measured on 1,881 pages of Don Quixote: no window pays on its own, and over a
  stack real pages sit where survivors are thicker, so windows lose on average `[03]`.
- [x] N3. Video filters that rank: life-causal-v1, video-pan-v1 and image-mirror-v1 (places free or forced),
  video-smooth-v2 (v1 with still videos set aside, counted by changes so far), each checked against every unit of
  small lines; the video line compacts `[01]` + `[03]`.
- [x] N4. rows-alike-v1 (each row within `changes` pixels of the row above; row-runs' other half), an automaton,
  measured on 20 real pictures from the public-domain anchors `[01]`. A fault found on the way and fixed: merges of
  code-made automata kept under their settings alone, so two stacks could share a count (IDEAS 19.252) `[01]`.
- [x] N5. The rest, for the days after (Edward, 18 November 2026: "prepare a massive task list"):
  - Degree windows
    - [x] N5-1. The degree window set in the filter itself, not from a hallway row (Edward, 19 November 2026: "I was
      thinking that should just be something you enter into the filter rather than a window. A filter
      setting/parameter."): its `from`, `width` and `decimals` typed in the filters window as numbers, where today
      they are stepped with the arrows. No new rows or windows on the main or pause menus for it (Edward, 19 November 2026: "we need
      to be careful not to add new windows, the main menu and pause menu is getting crowded as it is ... one can
      just go straight back to the setup menu if they wish to adjust the numbers and recalculate everything").
    - [x] N5-2. A window across 0 degrees (350 to 10): two stretches, ranked as one (the lower first).
    - [x] N5-3. The degree-window filter (the filter, with its from/width settings, not a screen) made to work on
      the binary line and the fractals line, where today it is offered on the digit lines only. On the binary line a
      file's bearing is its place among every file up to the line's length (shortest first, then by bytes); on the
      fractals line, a ruleset's place in the line's own order. Nothing added to any menu. Built: counted and ranked on
      the binary line (BinarySieve), judged on the fractals line `[01]`.
    - [x] N5-4. `sieve window` over many simple stacks at once, its search paid for (log2 of the choices), to
      show in one table where a window ever pays; and the same for the books line.
  - Video and pictures
    - [x] N5-5. video-moves-v1 ranked: by the same ranker as smooth-v2, every change allowed and one needed.
    - [x] N5-6. rows-alike-v1 counted and ranked on every palette (done with N4: the same ranker, the rows as its
      parts).
    - [x] N5-7. Counting row-runs and rows-alike together on large palettes: set aside. Each counts by the changes so far
      alone, but together a pixel's choices depend on whether its left and upper neighbours share a colour, which the
      colours decide, so no count by changes holds; on small palettes both are automata and merge already.
    - [x] N5-8. Video filters measured on the Roundhay Garden Scene cut many ways (clips, sizes, palettes), as
      the pictures were.
    - [x] N5-9. Smooth as an automaton: set aside. Its state must hold the whole frame before (2^25 at the line's
      5 x 5 default), so it would merge only on frames of a few pixels, where video-smooth-v2 already ranks.
  - Sound
    - [x] N5-10. sound-envelope-v1 and sound-syllables-v1 ranked at any length (their counts already go block by
      block: a walk sample by sample with the block's state can rank by those counts), so a second of sound
      compacts with them.
    - [x] N5-11. melody-contrary-v1 counted (rules between voices: a walk over both voices' moves at once): on two
      voices, exact against every unit of small lines; the default 16 notes a voice in 8 s `[01]`.
  - Pages
    - [x] N5-12. Quotation marks spaced as prose spaces them (an opening mark after a space, a closing one before
      a space or a mark), and a dialogue's turns (IDEAS 19.21), as automata on ascii96.
    - [x] N5-13. The `.obj` filter on the pages line (IDEAS 7.3): set aside, not built. A grammar keeping only `.obj`
      pages was written and then removed: not-other-line-v1 already sets aside a page that is a model's `.obj` text
      (or a melody), which belongs on the models line (Edward, 19 November 2026: "shouldn't .obj already be filtered
      out through the cross dimensional filtering which is always supposed to be active").
    - [x] N5-21. J opens a model (.obj, .gltf, .glb) on the models line, fitted to its shape `[10]`.
    - [x] N5-22. The File Locator: Choose a file... opens without sieving, Sieve a file... sieves, Go to it walks to
      the .sieve where sieved and the file where not.
    - [x] N5-20. Not needed (Edward, 19 November 2026: "that is fine as it is then, if optimise filters already ticks them"). His idea, the same day: "we could have that as a global setting as well": the cross-dimensional
      exclusions (not-other-line, not-a-file, not-packed, not-an-item) as one global setting, on by default, rather
      than filters each line ticks; and a plugin's header able to say it picks a kind, so X and OPTIMISE leave the
      format grammars alone on ascii96 pages.
    - [x] N5-14. Join filters (IDEAS §13): filters that look only at the joins between a book's pages: join-leap-v1
      (tracks) and join-cut-v1 (movies), counted and ranked `[01]` + `[03]`; with each unit's own filters, still open.
  - Models and the AI line
    - [x] N5-15. More filters for the models line (no degenerate triangles, edges shared by two faces, a mesh that
      closes), counted with the model rules: mesh-edges, mesh-closed and mesh-area built, judged only (an edge joins
      faces, so the closed forms cannot count them) `[01]` + `[04]`.
    - [x] N5-16. Filters for the AI line's weights (their spread, their signs), measured on trained runs: weight-signs-v1
      (the trained model passes at 10%); a spread filter set the trained model aside and was dropped `[01]` + `[04]`.
  - Throughout
    - [x] N5-17. OPTIMISE FILTERS measured again on every line with list N's filters, the new shares in
      STATISTICS.
    - [x] N5-18. An audit of every other cache kept by provenance (the plugin cache on disk, the tables), for the
      fault list N4 found in the merges.
    - [x] N5-19. The manual: the degree window and `sieve window` in the filters chapter.

- [ ] O. Pages of a range of lengths (Edward, 19 November 2026: "realistically the line should still find a 20
  character 'welcome to the sieve' ... perhaps we should change 'length' to have 'minimum length and maximum length'
  instead under the pages options"; the default 1 to 32; books left as they are; MIN LENGTH and MAX LENGTH rows).
  Every page of `min` to `max` characters, the shorter first, then in order within a length, as the binary line holds
  its files; no padding, so a page is what was typed. With `min` = `max` the line is exactly today's (its addresses
  unchanged).
  - [x] O1. The address space: a pages space of a range (count of the shorter pages, then the page's own number;
    scrambled by shuffle-sha256-v1 over the whole range, as the binary line is; at `min` = `max` the digit Feistel as
    today), hex, steps along the loop, the fraction; the oracle and its vectors.
    - Space gains a minimum length (`Space(alphabet, min, max, key)`; the old constructor is min = max). Size
      B^min + ... + B^max; the pages shorter than L, (B^L - B^min) / (B - 1).
    - A page's place: the pages shorter than it, plus its letters read as a base-B number; the place back to a page
      by finding its length from the place, as the binary line does. Positional address = the place; scrambled =
      shuffle-sha256-v1 of it over the whole range, keyed with the line's key, its domain the space's id.
    - An address is a number below the size, so its digits are written to as many places as the size needs: hex
      width from the size, a step along the loop is adding modulo the size, the fraction is place / size, and
      "go to 36.5%" is floor(size x 0.365). With min = max every one of these is today's, digit for digit.
    - The id: `lower27/L1-32/key=sieve/...` for a range (one length keeps `L32`, so its addresses keep their
      domain); digits_of and text_of take any length in the range.
    - The oracle (reference/sieve_ref.py) gains the same space; vectors of places, addresses and pages at several
      ranges (and at min = max, today's vectors unchanged); unit tests walk every page of small ranges both ways.
    - *Built (19 November 2026):* Space(alphabet, min, max, key); every one of the 20,439 pages of 1 to 3 letters
      walked both ways in both orders (the scrambled order a permutation of the places), steps round the loop, the
      fraction; min = max the fixed space address for address; the oracle's RangeSpace and 84 vectors (ranges 1-3,
      1-32, 5-8, 19-21 with the greeting, ascii95 1-8), agreed by the C++ `[01]` + `[11]`. Nothing uses it yet
      (O2 to O6).
  - [ ] O2. Canonicalisation without padding on a range line (a text longer than `max` still cut into pages).
  - [ ] O3. Filters over a range: each length counted by its own stack and the counts summed (an automaton's table
    answers every shorter length already), a survivor's rank the survivors of the shorter lengths and its rank
    within its own; compact over the range. Filters that need a whole page of one length judge only.
  - [ ] O4. The hallway: the pages line, its title shelves (titles stay fixed), shelving and walking, compact, J/F,
    the File Locator's page items, not-an-item's pages on the binary line, the greeting at its own length.
  - [ ] O5. Guided ordering on a range: a guided order within each length, the lengths in turn.
  - [ ] O6. The setup menu (MIN LENGTH, MAX LENGTH; FIND MY LIMITS), the settings file (`length` read as both, for
    old settings), the CLI (`--min-length`, `--length` as the maximum), docs, CI pins moved to the new default.
  - [ ] O8. Seeds, studied (Edward, 19 November 2026: "a guided sweep of a small number of seeds at fractional
    intervals ... try those seeds against a variety of different file types ... sorting the seeds by efficiency ...
    analysing the way that the seeds move the file"): a `sieve` command first. n seeds evenly spaced through the
    seeds (n adjustable), a spread of file types (text, PNG, JPEG, MIDI, programs, .sieve files); for each seed and
    file, where it lands and the bits saved against its place once the seed is named; the seeds sorted by
    efficiency, overall and by type; and the movement: whether a seed good for one file of a type is good for
    another, whether neighbouring seeds move a file to neighbouring places, whether landings are even. Two families:
    the binary line's shuffle (SHA-256 rounds: expected even, no seed better across files, the best of n saving about
    log2 n, what naming it costs) and an affine one, x -> (a x + b) mod N, whose path from A to B can be solved for
    (b = t - a x) and whose seed then holds the file's bits. Results in IDEAS; in the File Locator (a few seeds, a
    setting, 0 for none) only if a seed ever pays.
  - [ ] O9. Orders by sort key (Edward: "use a number of different sorting algorithms to sort seeds ... to guide the
    scrambling"): a few named orders of a line, each its units sorted by a key that means something (a model's
    probability, as guided order is; letters' entropy; a filter's count of passes), tried for a file and the best
    named, as O8 names a seed. A random key is just another shuffle (no gain); a key that puts real content early
    can save more than naming it costs. Measured first (IDEAS), built only where it pays, and only keys that can be
    counted under a prefix, so an order of an endless line can be walked without sorting it.
  - [ ] O10. Seed Search in the File Locator, after O8 has measured (Edward, 19 November 2026: a "Seed Search"
    option that toggles between "Every Step" and "At The End"). At The End: a few seeds tried on the finished .sieve,
    one kept only where its scrambled place plus naming it beats the plain place. Every Step: tried on each candidate
    the sieving weighs as it goes (each a different file, so fresh tries), the winner kept only where it beats the
    finished file with At The End's best, all counted in bits at the end. The number of seeds a setting; off by
    default until O8's figures say whether either pays.
    - Its time: a seed costs one shuffle of the file's place, about 0.19 ms a KB here (measured 19 November 2026:
      1 KB 0.20 ms, 16 KB 3.1 ms, 128 KB 24 ms, 1 MB 196 ms a seed). The row says what n seeds will add for the
      file chosen ("16 seeds: about 3.1 s"), and a cap, the smaller of 2 seconds and 5% of the sieving's own time,
      lowers n to what fits (none, saying so, for a file too large); Every Step counts its tries against the same cap.
    - Every Step multiplies the cost: n seeds for each candidate weighed, and the candidates are many (each way of
      each file). So its time is worked out as the sieving goes: the time spent on seeds so far, and the candidates
      still to come times a seed's cost at their sizes, shown in the progress line; when the cap would be passed
      it stops trying seeds and the rest of the sieving goes on as without them, saying so at the end.
- [x] O7. OPTIMISE FILTERS leaves near-model-v1 and language-v1 alone (they pick a run's neighbourhood and language);
  its last step, keeping each stack within the filter memory, on a worker, so the window answers.

### Later (in ROADMAP's final tasks)
- Face finder (19.193), modelling tools (19.194), the hypercube background (19.195), minimap line of sight (19.197), interface designer (19.206), plugin dimensions (19.217), AI explorers, the website (task 32), voice (task 119); low priority: the hash viewer (19.231, once V3).

---

## 4. The order of this review (Edward's notes, 13 November 2026)

- [x] 1. A list of the new features for Edward to test as a user (section 1 above).
- [ ] 2. Edward tests them; the fixes that follow are made (each fix zipped, its box ticked when retested).
- [x] 3. Multi-agent code review 1: optimisations and performance, to speed things up. *Run 14 November 2026
  over everything changed since 939666d (127 files): six finders, each checked by a skeptic; 26 findings,
  19 kept (confirmed, or plausible and worth their code), 7 set aside (not worth the code they would add,
  or refuted). Fixes in three batches by CI step, below.*
- [x] 3a. Batch A, the hallway program (step 10): the Replays screen hashes every recording every frame;
  the Character Creator and the hallway hash every animation every frame to find the one playing; your
  dimension's character lookup hashes each stored character each time (my own, from the last zip); the
  CHARACTER page and YOURS list re-hash every frame; physics works out every pair's size 120 times a second;
  holding something tests every object 75 times a frame; aiming turns every triangle of every object; the
  hill's heights every frame; faces.ini read per favourite per frame; a picture's pixels kept twice in the
  viewer; the Keys screen's listing rebuilt per row. Every one keeps every output the same.
- [x] 3b. Batch B, the RAID and the rest (steps 01, 04, 10): the RAID's list re-walks every chain in each
  comparison (a save took 3 s at 684 versions, and grows); a piece sent as two writes waits on the network
  (4 times slower, measured); sharing asks each peer in turn; a .glb's data copied twice; a steered chain
  stage on one thread.
- [x] 3c. Batch C, making (steps 01, 02, 11): two trial mixers each zero-fill a buffer the whole file's size;
  under Balanced a program is mixed with and without E1 side by side, doubling the mixer's memory;
  html_text's script/style skip is quadratic (16 MB page: 4.5 s, measured). The bytes made stay the same.
- [x] 3d. All three batches made (14 November 2026), each output compared with the build before it (pictures,
  saved worlds, RAID lists and reports, .sieve bytes, HTML text): the same. Measured: 200 dropped objects for
  20 s 20.9 s to 0.8 s; the RAID list at 301 versions 0.195 s to 0.024 s, a put 0.372 s to 0.047 s; an 8 MiB
  share 2.12 s to 0.14 s (which also answers "sharing asks each peer in turn"); a 16 MB HTML page 4.59 s to
  0.14 s. The set-aside findings checked again by fresh agents: two fixed by these changes, six left (each
  measured or traced: a menu page's idle cost, an opt-in recording's, a hundredth of a millisecond), and
  the three fixes' reviewers found three points, all fixed. Items 1, 2, 3 and 8 above unticked (the physics,
  the catalogue, holding and the Keys screen were touched); 7, 10, 12, 13, 17, 22 and 25 were already open.
- [x] 4. Multi-agent code review 2: duplicate code, brought to one copy. *Run 14 November 2026 over the same
  code as review 1: seven finders (six areas and one across them), each checked by a skeptic; 90 findings,
  about 60 distinct once the areas' overlaps are joined (the same is_hex64, comma list, thousands grouping
  and hex digit found from several sides); 79 kept, 11 set aside (5 refuted, among them the harness's key
  wrapping, which differs on purpose; 6 not worth the code). Fixes in batches below.*
- [x] 4a. Batch D1, small helpers to one copy each (steps 01, 02, 04, 10, 11): SHA-256 hex checks and text ids,
  hex digits and hex writing, line splitting (strict and lenient), comma lists, thousands grouping,
  lower-casing and trimming, UTF-8 writing, percent-decoding, whole-file reads, write-then-rename.
- [x] 4b. Batch D2, characters, animations and importers (steps 01, 10): held-model lines, settings words,
  bone hex groups, pose turns, glTF node transforms, files beside a model, the stores named by SHA-256.
- [x] 4c. Batch D3, the RAID (steps 01, 04, 10): the peer ranking, connecting to peers, the key file, the
  folder tag, a manifest by id, the share and check notes, address resolution, the command's options.
- [x] 4d. Batch D4, the hallway program (step 10): one near-plane clip, your dimension through the Camera it
  already builds, the data-folder search, mesh bounds, colours, a character's foot, the pause menu's keys,
  note names, outlines, the setup menu's panels through SlidePanel, the side lists, settings numbers,
  screenshot lines, the drawn picture's chat entry.
- [x] 4e. Batch D5, the command line, catalogue, documents and training (steps 01, 04, 11): manifests and
  install trees, sieve_cache's helpers, the catalogue's searches and lookups, format statistics, doc_text's
  zip members, digit values in training, mix-v1's coder, mix-v4's setup, CPU feature probing.
- [x] 4f. Made (14 November 2026): 54 merges, 69 files, about 520 lines fewer, each batch's outputs compared
  with the build before review 2 byte for byte; the rest left with their reasons (each would change an
  output, or need so many switches that one copy is no simpler). One bug found by a batch's reviewer and
  fixed (--pworld-look past 90 degrees). Asked: a new RAID folder's id from the system's generator
  (MinGW's std::random_device can repeat).
- [x] 5. Multi-agent code review 3: the code compared with the documentation, each made to agree. *Run 14
  November 2026: seven finders (the manual twice, the specifications, the RAID's documents, the help, the
  ideas and roadmap, the comments), each checked by a skeptic; 57 findings, none refuted: 44 where the
  documentation (or a comment) is wrong or missing, 13 where the code is.*
- [x] 5a. Batch E1, the documentation (no CI but the help and settings checks): the manual's personal
  dimension, menus, Settings, drawings, panels and keys; the specifications' world, favourite, replay and
  RAID lines; the RAID documents; the help's catalogue, import, locate and hallway options; IDEAS 19.187,
  FEATURES, this file's items 22 and 25; stale comments.
- [x] 5b. Batch E2, the code (steps 01, 04, 10): the dimension's footer (Esc) and O on a character; a world's
  `place model:` lines kept rather than lost on saving; a replay of long typed text read back; Keep others'
  saves saying when Online is off; "Settings > Infinite RAID" in the notes; `sieve raid` naming check; the
  catalogue entries' section numbers; the draft switches' help.

The three reviews run one after another, each looking at a different side of the project, and only once
step 2 is done. Each review's findings are checked before anything is changed, its fixes go through
CI, and each ends in its own zip. The batches from section 3 start after review 3.

---

**Suggested pause:** after this round's zip (files opened by their kind), stop for your review of
section 1, then pick the first batch from section 3 (T1 in it if cutting testing time matters most).
