# Changes since 01cf848 "sieve systems" (12 November 2026)

Two commits (993e8c9, 1916fd5): the LANGUAGE page beside CHARACTER, and the character's save.

**Build:** a new core source, `core/src/save.cpp`, is in CMakeLists.txt, so CMake must run again.

**One file to delete by hand (a zip cannot delete):** `Sieve/data/catalogue/language-english.format`. It was still in your update; English lives in `Sieve/data/languages/` now, and with both, the same id twice, the hand-written entries are refused (`sieve catalogue --check` stopped with an error; with it gone, all 187 entries pass).

## Code
- **LANGUAGE beside CHARACTER** (IDEAS §19.183; Sieve/client/menu.cpp): the right-hand panel's tabs are now CHARACTER, LANGUAGE, YOURS, SYSTEMS (PgUp/PgDn between them; the system viewer's tab is called SYSTEMS, the system shown named inside it). LANGUAGE lists the language catalogue by family; Up/Down choose and Space ticks the languages your character speaks. The choice is kept at once in the settings file (`[character] speaks`) and read back on the next start.
- **The character's save, `sieve-save-v1`** (Sieve/core/include/sieve/save.hpp, Sieve/core/src/save.cpp; SPECIFICATIONS §12.0n): everything chosen in character creation named together by one SHA-256: the character, its animation and the languages it speaks; each system that joins character creation adds a line of its own later. The page shows the save's id. With `[raid] drives` set, Ctrl+S on the CHARACTER page keeps it in the infinite RAID as a version of "your save", beside the character and its animation.
- **Readout:** `speaks:` and `save:` for scripted runs.
- **Tests:** `test_save` (order, once each, read back, refusals); CI step 10 ticks French and Korean, checks the settings file, and reads the same save back on a second start; the two YOURS checks now press PgDn twice. The unit suite passes (180,444 checks); CI step 10 passes (911 s).

## Documents
- SPECIFICATIONS §12.0n; MANUAL (LANGUAGE, the tab order); IDEAS §19.183 (built).
