# List M: menus and keys (14 November 2026)

Built and checked here: the hallway built, the help and settings checks, the unit tests and CI step by
step on what changed. To test: section 1 of REVIEW-ROUND.md, items 40 to 44 (new).

## M1. The hallway's keys on the registry (IDEAS 19.199)
- 48 hallway actions in five new sections on Potentia Settings > Controls > Keys, after the Creator's
  (no existing row moves): walking, anywhere, empty-handed, holding, typing. The defaults are the keys
  the hallway used before. Changes are kept under `[keys]`, e.g. `hallway.anywhere/thousand_on = J`.
- The footer, the item's key lines, the HELP and COST tabs, SORT, the map hint and Settings > Controls
  name the keys as set. With the defaults every picture is as before.
- A key press is matched exactly first, otherwise by the action whose key asks most of the modifiers
  held, so Shift (running) with E still takes the book.
- Esc, the mouse buttons and the wheel stay fixed.
- The registry's window-free part and the favourites' reader and writer moved to core/, with unit
  tests of their own.

## M2. Every other screen's keys (IDEAS 19.199)
- Ten more sections: the menus (one section every list reads: up, down, left, right, choose, toggle,
  back, next, page up/down), the setup menu, its filters window, its right-hand panel, SYSTEMS, the
  Media Player and its rings, the main menu, the Encryption Engine and the CHAIN tab.
- Still fixed, and listed in IDEAS: the pause menu, the File Locator, the Filter Designer, the Harness,
  Manual Navigation, the viewer, CHAT, the Navigation System, the Node Graph Viewer, F12.

## M3. The main menu's new order (IDEAS 19.188)
- Start Sieve, File Locator, Media Player, Filter Designer, AI Training Harness, Personal Dimensions,
  Character Creator, Replays, Encryption Engine, Manual Navigation, System Viewer, Potentia Settings,
  Exit Sieve. Settings is now Potentia Settings, in every title and help line. `Item:` ids unchanged.
- The rows are drawn closer where the window is too short for them (as Graphics already was), so at
  1024x600 every row fits above the footer, and at 1280x720 Exit Sieve no longer touches it.

## M4. Wireframe (IDEAS 19.223)
- Real Graphics is now **Wireframe**, on by default (the old Real Graphics off); the default picture is
  unchanged. Older settings files with `real_graphics` are read and rewritten as `wireframe`.
  `--real-graphics` still works; `--wireframe on|off` is new.

## M5. Footers wrap (IDEAS 19.224)
- Every footer of keys goes through one shared footer: if it fits it reads as before, otherwise it
  breaks between keys onto more lines, upwards. New `--footers` prints each footer's lines and width.
- Left: the File Locator's and Media Player's buttons do not yet move up for a wrapped footer.

## Fixed after the review
- The setup menu's "row above" and "row below" given a letter now still move from the key's row
  (other letters there are typed into the key).
- The COST tab's text and the HELP tab's descriptions name the keys as set (F, K, L, V, [ ], R, A/D,
  Enter), not the defaults.
- Two CI lines corrected (Potentia Settings read aloud; a footer check that named nothing).

## Docs
- IDEAS 19.230: the DNA dimension as a place to learn (your words of today), for later.
- REVIEW-ROUND: M1 to M5 ticked, test items 40 to 44 added.

## Files to delete by hand
None.
