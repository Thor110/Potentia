# Changes since 7f2ea7d "sieve extraction" (12 November 2026)

Six commits (027a049 to 42cf77a). This zip replaces the animation zip. The animation now plays in the hallway and is kept with your character; the infinite RAID, stage 1, is built; four new ideas noted.

**Build:** a new core source, `core/src/raid.cpp`, is in CMakeLists.txt, so CMake must run again.

**Before you push:** pushing publishes the RAID's code under the repository's licence, the GNU AGPL v3. If you want the RAID under its own charity-share licence (IDEAS §19.177), hold it back until that is decided; see below.

## Code
- **The animation in the hallway** (IDEAS §19.170): your character plays its animation as it walks beside you. Its feet and its place in the corridor come from its rest pose, so they hold still while it moves. Files: Sieve/client/hallway.cpp, Sieve/client/hallway.hpp.
- **Kept with your character:** every animation goes into a store beside the settings file, `animations/<SHA-256>.animation`, and `[character] animation` names it by that SHA-256. On the next start it is read back and checked against its name; one that is missing, refused, or changed is left out. Files: Sieve/client/character_store.hpp, Sieve/client/app_settings.cpp/.hpp, Sieve/client/menu.cpp.
- **Shift+A** on the CHARACTER page takes the animation off; A still pauses and plays it.
- **One-off runs:** `--animation FILE` plays a `.animation` file in the hallway for one run (with `--character`, only so), and `--animation-frame N` now holds it at frame N in the hallway as well as on the CHARACTER page; the readout says `animation beside you:` with its SHA-256. File: Sieve/client/app_main.cpp.
- **Tests:** CI step 10 checks the animation's id in the settings file, its file in the store, that the next start plays it, and the arm in the hallway with its readout. CI step 10 passes in full (923 s); the unit suite passes (178,130 checks). The help check passes (0 missing).

- **The infinite RAID, stage 1** (IDEAS §19.176; Sieve/core/include/sieve/raid.hpp, Sieve/core/src/raid.cpp; SPECIFICATIONS §12.0l; MANUAL):
  - `sieve raid put FILE --drives A,B,C` keeps any save on several folders you choose: cut into chunks of 1 MiB, each named by its SHA-256 and kept on two of the folders (`--copies N` for more), with a small manifest in every folder whose SHA-256 is that version's id. Putting it again keeps a new version that names the one before; the same bytes again add nothing.
  - `list`, `get NAME-or-ID --out FILE`, `verify` (exit 1 if anything is damaged, short of copies, or lost), `repair` (every damaged or missing copy and manifest written again from a good one).
  - Nothing is trusted: every chunk is checked against its name when read; a chunk with no good copy anywhere is reported lost, and reading a version that needs it is refused rather than given wrong.
  - Local only: nothing leaves the machine. Saves as deltas (stage 2) and sharing between machines (stage 3) are not built.
  - **Tests:** `test_raid` in the unit suite (versions, damage, missing copies, repair, loss); the suite passes (178,158 checks). CI step 4 passes (107 s) with a save on three folders, one copy damaged and one folder's manifests taken away: found, repaired, and read back byte for byte.

## Documents
- **Sieve/docs/ideas/WORK-DONE.md:** your real-world test noted: the 72-file zip unpacked by Sieve's right-click extraction on your machine, and every file came out identical to the commits it was made from (only the zip's CHANGES.md differed, as it was never committed here). One zip, one machine: it shows the extraction gave the files back exactly, not how fast.
- **Sieve/docs/MANUAL.md, SPECIFICATIONS.md (§12.0k), IDEAS.md (§19.170):** the animation in the hallway and kept.
- **Sieve/docs/IDEAS.md §19.174:** an interaction dimension: hands and limbs as tools, the species that choosing the tools first might reveal (crab-like, or the Keepers of Mass Effect), and interaction from physical to digital for every species. In the catalogue of dimensions as `interaction` (Sieve/core/src/dimensions.cpp), linked from species.
- **Sieve/docs/IDEAS.md §19.175:** a music visualiser of near endless rings, a lens into sound, for the media player.
- **Sieve/docs/IDEAS.md §19.176:** the distributed infinite RAID in four stages, and the decisions stage 3 needs from you (encryption of others' saves, space given, how machines find one another).
- **Sieve/docs/IDEAS.md §19.177 (a decision before release):** the RAID under a licence of its own, profit shares to charity set democratically. What stands in the way: the AGPL v3 lets anyone who receives the code remove added terms, so the RAID would have to be released under the new licence instead, in a folder of its own. What it needs: your decision, a lawyer to draft it, and the democratic mechanism defined in it. Until then its code stays unpublished.
- **Sieve/docs/VOICES.md:** three lines (it unpacks itself; the licence that encourages charitable works and democratic debate; the infinite, aesthetically warping state space).
- **Sieve/docs/ROADMAP.md:** stage 1 built; the licence decision marked as needed before the RAID is published.
