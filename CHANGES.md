# Manual Navigation: opening a file fixed; the byte sheet and an example to drive (4 November 2026)

Copy `Sieve/` over the repository's. The two pictures beside this file are only to look at: the
2 by 2 example after a few turns, and control.py as a sheet 16 bytes wide.

## Why opening a file did nothing

The screen named itself as the owner of its file windows but never registered as one, so the
dialog code dropped every answer: Load File, Load Log and Save Steps all did nothing. My earlier
checks loaded files from the command line, which never goes through a window, so they missed it.

It registers now. `--manual-pick FILE` hands a file over the way the window does, through the same
check, and CI step 10 uses it. Built without the fix, that check fails, so it would have caught this.
(The other screens that open windows are all parts of the hallway, which registers.)

## What the screen does now

- **It opens on an example:** a square of random bytes, 2 by 2 to begin with, so there's always
  something to drive. Load File puts a file in its place.
- **WIDTH (W; Shift+W back): 2 to 16.**
  - For the example, it is the side of the square: 3 by 3, and on up to 16 by 16 (256 bytes).
  - For a file, it is the number of bytes on each row of the sheet.
- **FRESH RANDOM BYTES (F, or its box; on by default):** when the width changes, the example is made
  of new random bytes. Unticked, it keeps the bytes it has, adds random ones or cuts the end. It does
  nothing while a real file is loaded. The name is mine, as your message was cut off at "call it";
  it is one string to change.
- **A file is shown whole** as a sheet, with each row's first offset at its left.
  - It scrolls with the wheel, PgUp/PgDn and Home/End; a bar beside it shows where you are.
  - Choose a byte with Left/Right, Ctrl+Up/Down for the row above or below, or a click.
  - Only the chosen byte has arrows, at its top and bottom. Turn it with them, with Up/Down (Shift:
    16), with Shift+wheel, or by typing hex.
  - Bytes that differ from the file as loaded are drawn in the accent colour.
- **Logs of the example** record its bytes in the log's head (`# example,`), so Load Log makes the
  same example again with no file.
- **A log now begins at the first step,** so just opening the screen leaves no empty logs.
- **Remembered:** the width and FRESH RANDOM BYTES are kept in the settings file under `[manual]`,
  beside Keep a log and the undo depth.
- **Also fixed:** W only worked once a file was loaded.

## Checks

- CI step 10 now covers:
  - the window path;
  - the example from a fixed seed (the same bytes on every system), its log and its replay;
  - widening with FRESH RANDOM BYTES on (nine new bytes) and off (the first four kept, five added).
- Step 10 passes here, and the unit suite passes (154,314 checks, 0 failures).
- I can't click here, so the mouse wants a try in the hallway itself: the arrows, clicking a byte,
  the wheel and Shift+wheel, and the buttons.
