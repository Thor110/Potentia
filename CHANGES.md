# The File Locator from the main menu, the calculating banner, the degrees sample

Against `87bd260` (idea update). Items 2, 5 and 9 of your list of 10 October (IDEAS §16.2, §16.5,
§16.9).

## 16.2 The File Locator from the main menu
- **Main menu:** File Locator now sits under Settings and above the Filter Designer. It is the same
  locator as the pause menu's: choose, weigh, tailor, save and install.
- **Go to it:** offered only in the world. From the main menu there is no hallway to walk in, so
  the Go to it button, its Enter key and the "binary line" row are not shown.
- **Weighing:** files are weighed under the lines the setup menu's current settings would build.
  These are made when the first weighing starts.
- **Use the tailored filters:** from the main menu, saves them for the next hallway built. In the
  world it builds the hallway again with them, as before.
- **Code:** the locator is now its own class (`client/file_locator.hpp`). It no longer belongs to
  the hallway, which supplies only its lines, filters, BINARY length and the walk to a file.

## 16.5 "Calculating Dimension..." where it is seen
- **The banner:** while any line is being counted, or X is weighing clashes, the setup menu says
  "Calculating Dimensions..." in red at the top, centred between the title and the budget bars.
  The words never change, so it keeps its size and place. The menu's narrowest width now leaves
  room for it.
- **Which lines:** still under the bars, in grey, as before.
- **Fixed since the first version you tested:** the banner was sized and centred on what it said,
  and what it said shrank as each line finished. So it grew and moved, and in a narrow window it
  jumped to a line under the subtitle.
- **ENTER THE HALLWAY:** greyed and refused until the counting is done. A scripted Return waits
  for the counts, as a person would.

## 16.9 A demonstration: every dimension at every degree
- **The command:** `hallway --no-menu --sample-degrees DIR --screenshot x.png` saves the item at
  every whole degree (0 to 359) of every dimension into `DIR/<dimension>/<ddd>.<ext>`. Each
  degree names the first unit at or past that bearing, the navigator's rule. The folder is then
  made into Sieve instructions: `DIR.sieve`, or `--sample-out FILE`.
- **The files:** pictures are saved one pixel a pixel, the file J reads; F saves them at 16 pixels
  a pixel, with the same picture. Binary files are saved as their own bytes.
- **The size:** at the default shapes, 3,240 items and 566,236 bytes, packed into a `.sieve` of
  303,829 bytes (v4). It takes about 3.5 seconds in a Release build here.
- **Checked:** it is the same on every run. `sieve install` gives every file back. A bearing typed
  in the navigator (X, Tab, 90, Enter on pages; 13 on models) lands on the same item, byte for
  byte.
- **Only 225 distinct files:** the 3,240 items are all different, but only 225 of the files are.
  The address includes each item's title (and a cover, where there is one); the file saved holds
  only the unit (the page, picture or melody). At whole degrees the unit part repeats: every 40
  degrees on pages, image, models, binary and books, every 72 degrees on audio, tracks, video and
  movies. The navigator gives the same units at those bearings. That is why the `.sieve` is so
  small.
- **A curiosity:** the pages unit at 90 degrees is `ftftft...fu`. A quarter of the line, written in
  base 27, repeats.

## Checked here (Linux)
- **Unit tests:** 94,850 checks, 0 failures.
- **CI:** the whole Linux hallway step, with the new checks, passes when run here.
- **Screenshots:** the main menu, the locator from both places, and the setup menu while counting
  at 1920 and 1240 wide.
- **Not checked:** I have not built on Windows.

## Files
- **New:** `client/file_locator.hpp`.
- **Changed:**
  - `client/file_locator.cpp`, `hallway.{hpp,cpp}`, `hud.cpp`, `font.{hpp,cpp}`, `app_main.cpp`,
    `main_menu.{hpp,cpp}`, `menu.{hpp,cpp}`, `item_save.cpp`;
  - `data/lang/en.txt`;
  - `.github/workflows/build.yml`;
  - `README.md`, `docs/HANDOFF.md`, `docs/IDEAS.md`.
