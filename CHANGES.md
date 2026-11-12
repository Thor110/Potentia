# Changes since 1eedaf3 "sieve ideas" (12 November 2026)

Thirty-six commits (84d4054 to b97916b). This zip replaces every zip sent today. Code: the hallway's shimmer; the character system's seven steps (pedestal, drawing levels, models in spheres, the seed walk); a store of saves; Personal Dimensions, species and the playground; the first importer (.obj); one comment on the website; the rest is documentation.

**Build:** a new core source, `core/src/character.cpp`, is in CMakeLists.txt, so CMake must run again (a fresh configure, or the build does it itself).

## Code
- **The character system** (IDEAS §19.148 to §19.150; SPECIFICATIONS §12.0j; MANUAL, the setup menu):
  - **The right-hand panel:** in the setup menu, opposite SETTINGS. The key left of 1 (read by its place, so ¬/` on a UK keyboard, the same key on any layout) or the CHARACTER tab at the right edge slides it in and out. While it is in, it takes the keys. Files: client/menu.cpp, client/menu.hpp.
  - **`sieve-character-v1`** (core/include/sieve/character.hpp, core/src/character.cpp): a seed and settings unfold a tree of spheres in integers from SHA-256 draws, the same on every machine. The settings are limbs 0 to 16, joints 1 to 12, branches 0 to 3, symmetry none, bilateral or radial, and a head. Bilateral characters are their own mirror. `test_character` in tests/test_core.cpp checks it and pins two characters by SHA-256; the unit suite passes (177,186 checks, 81 tests).
  - **The CHARACTER page:** the six settings (Up/Down, Left/Right, Shift x10 on the seed, R a new seed), and the character drawn as spheres and bones on a pedestal. A click and drag across it, or [ and ], turns it.
  - **Drawing levels:** the CHARACTER page's seventh row draws one character as pixels, wireframe, voxels or shaded; its SHA-256 is the same at each (IDEAS §19.147).
  - **Step 4, saving and the hallway:** `read_character` reads a `.character` file back and checks every sphere against its settings; Ctrl+S and Ctrl+O on the page; the character kept in `sieve-hallway.ini`'s `[character]` (`line`, `drawing`, `beside_you`, `max_hall_scale`); and it walks beside you in the hallway. `--character FILE` and `--no-character` for one run.
  - **Step 5, the hallway scaled to fit it:** a central metric, 4096 units a metre; the hallway grows (as you made smaller: eye, stride, the character) until your character passes a door, at most `max_hall_scale` times (IDEAS §19.153).
  - **Step 6, models in spheres (`sieve-character-v2`):** a sphere holds a model of the models line (51-digit address); a character holding none is still v1, identity unchanged. The page's sphere and model rows; drawn on the page and in the hallway. Every save also kept as `characters/<SHA-256>.character` beside the settings file (the store).
  - **Step 7, the seed walk (`sieve-character-walk-v1`):** G and Shift+G make the next character from this one, the same on every machine.
  - **Personal Dimensions:** on the main and pause menus (before Exit), a screen of three; Personal Dimension opens YOURS, the right-hand panel's second page (PgUp/PgDn): your store grouped by **species** (`sieve-species-v1`), with a **playground** (P fills it along the walk, K keeps one). Your store's characters stand in the hallway on your left; `--character-store DIR`. Other people's and group dimensions are listed and say they wait on sharing.
  - **The first importer:** Ctrl+I (and `--import-obj`) brings any `.obj` into a sphere under the held roof of 8 vertices and 12 faces, saying what it kept. The reader moved into the core (`ModelSpace::read_any_obj`), shared with `sieve mesh --warp`.
  - **Tests:** the unit suite passes (177,334 checks); CI step 4 passes (143 s, the moved .obj reader); CI step 10 passed in full on the build with Personal Dimensions and the walk (1,011 s); the checks added since (species and the playground, the .obj import) were run by hand on the latest build and pass.
  - **Scripted keys** (`--press`) now carry their scancode as real keys do (client/app_main.cpp, Menu::press).
  - **CI step 10:** the key opens and closes the panel, and opens it from the key's row; two characters made through the menu are pinned by SHA-256. CI step 10 passed in full on the panel's first build (920 s); the later checks were run by hand on the latest build and pass.
- **Sieve/client/hallway.cpp:** the item under the crosshair shimmers (IDEAS 5.6): its tint breathes over 1.6 s and a leaning band sweeps across its front. Comments on the binary line's doors corrected (the last line before binary is AI now, not models). CI step 10 passes (916 s).

## The ideas, filtered
- **Sieve/docs/IDEAS-TRIAGE.md (new):** every idea (613) given a kind, a status and a verdict, with its reason.
- **Sieve/docs/ideas/ (new folder):**
  - IMPLEMENTED.md: 40 done entries, moved whole (5.6, the shimmer, among them), and every one checked against the code: all present, none missing; 13 have moved on since and carry a dated note of how.
  - DEPRECATED.md: 19 set-aside entries, all of §17 among them, with their reasons.
  - WORK-DONE.md: a dated log of work, from results/ and today's.
  - VOICES-STASHED.md: VOICES' technical lines, each with where its idea went.
  IDEAS.md keeps a one-line pointer at every moved number.
- **Sieve/docs/ROADMAP.md:** the long plan through the ideas, stages 8 to 18, at the top. Stage 8 (the record) and Stage 10 (hallway polish) are begun.

## New ideas noted (IDEAS §19.140 to §19.159)
- §19.140: the tactile Sieve.
- §19.141: the Game of Life over noise through time, and infinite short routes into the space.
- §19.142: Sieve as a lock in time.
- §19.143: colour at infinite dimensions; the setup menu as a scroll strip; views from other angles. The colour-blindness pass was done (6 November), and the plans that said otherwise are corrected.
- §19.144: an emotion dimension as a circle of doors; highway dimensions; AI and human work preserved; doors through doors.
- §19.145: the cross-dimensional lattice of highways, and ideas checked against what was built.
- §19.146: voices heard once a save; a highway for stories and genres.
- §19.147: characters that speak; a character system from low to high poly; one interface for every engine and device; the scalable graphics seed and the pre-render barrier.
- §19.148: the character system: the right-hand panel, spheres of influence, a skeleton unfolding from a seed, the hallway fitted to the body, settings finding their own space.
- §19.149: a scalable galactic vector information engine (Elite's eight galaxies, No Man's Sky, Spore, The Sims; clothing, armour, health, combat).
- §19.150: a strong save system, an opt-in infinite RAID behind red warning boxes, one character per person, and games as a geometric graph through time; fewer tests while the game systems are built.
- §19.151: pixels back to a model (matching a character to a picture).
- §19.152: species, languages and DNA; a playground for personal dimensions.
- §19.153: the hallway scaled to your character; the yardstick between the door and true scale; caps so no character makes everyone tiny; a middle cartoon-like drawing; true and perception scale, switched by world or person.
- §19.154: a rudimentary infinite RAID (the store, built); a personal dimension to build your own world.
- §19.155: a setting to turn the AI off (to build with the first spawned characters); a human-written Sieve beside this one.
- §19.156: the perfect educational system, at every range.
- §19.157: Personal Dimensions from the menus; others' ranked, group dimensions; everyone's interpretation as one rigid structure.
- §19.158: every interpretation folded into one web (a way to explain the project).
- §19.159: importers for models, skeletons and animation, each with its roof; the engine of engines.

## Documents matched to the code (IDEAS §19.134)
Fifteen places corrected across three files:
- **MANUAL.md:** the door order with worlds and AI; size limits that follow the machine; ITEM MEMORY; META as the fifth tab; the menu walk to Settings; `compress` and `speak`; the tile size; the 729-unit example.
- **SPECIFICATIONS.md:** colour table and door order with worlds and AI; the entropy filters' lines.
- **FILTER-PLUGINS.md:** retired data filters; filter forms.

## VOICES
- 62 technical lines and Gemini's two messages moved to ideas/VOICES-STASHED.md.
- The note at the top updated.
- 67 of Edward's lines added to the candidates today.

## The website
- **Pages/release.html (Build or Buy):** the comment on two constants no longer points at the Orbital page's script, removed on 6 November.

## Other documents
- GAME.md and HANDOFF.md: the colour-blindness pass marked done.
- GAME-OF-LIFE.md: a pointer to §19.141.
