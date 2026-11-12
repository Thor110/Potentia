# Changes since 7f2ea7d "sieve extraction" (12 November 2026)

Two commits (027a049, ee658d1). The animation now plays in the hallway and is kept with your character; one new idea noted.

## Code
- **The animation in the hallway** (IDEAS §19.170): your character plays its animation as it walks beside you. Its feet and its place in the corridor come from its rest pose, so they hold still while it moves. Files: Sieve/client/hallway.cpp, Sieve/client/hallway.hpp.
- **Kept with your character:** every animation goes into a store beside the settings file, `animations/<SHA-256>.animation`, and `[character] animation` names it by that SHA-256. On the next start it is read back and checked against its name; one that is missing, refused, or changed is left out. Files: Sieve/client/character_store.hpp, Sieve/client/app_settings.cpp/.hpp, Sieve/client/menu.cpp.
- **Shift+A** on the CHARACTER page takes the animation off; A still pauses and plays it.
- **One-off runs:** `--animation FILE` plays a `.animation` file in the hallway for one run (with `--character`, only so), and `--animation-frame N` now holds it at frame N in the hallway as well as on the CHARACTER page; the readout says `animation beside you:` with its SHA-256. File: Sieve/client/app_main.cpp.
- **Tests:** CI step 10 checks the animation's id in the settings file, its file in the store, that the next start plays it, and the arm in the hallway with its readout. CI step 10 passes in full (923 s); the unit suite passes (178,130 checks). The help check passes (0 missing).

## Documents
- **Sieve/docs/ideas/WORK-DONE.md:** your real-world test noted: the 72-file zip unpacked by Sieve's right-click extraction on your machine, and every file came out identical to the commits it was made from (only the zip's CHANGES.md differed, as it was never committed here). One zip, one machine: it shows the extraction gave the files back exactly, not how fast.
- **Sieve/docs/MANUAL.md, SPECIFICATIONS.md (§12.0k), IDEAS.md (§19.170):** the animation in the hallway and kept.
- **Sieve/docs/IDEAS.md §19.174:** an interaction dimension: hands and limbs as tools, the species that choosing the tools first might reveal (crab-like, or the Keepers of Mass Effect), and interaction from physical to digital for every species. In the catalogue of dimensions as `interaction` (Sieve/core/src/dimensions.cpp), linked from species.
