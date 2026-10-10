# The degrees sample by title, and the settings as a slide-in panel

Against `8c7fcf4` (locator banner degrees).

## What changed
- **Names:** `--sample-degrees` now names each file by its item's title, as F names it: for example
  `pages/ftftftftftftftftftftftftftftftft.txt`. The degree is no longer in the name.
- **Contents:** each file is the item's unit. Covers are not saved, since you said the pictures are
  for looks.
- **Which degree gave which file:** the report says so, one line an item (`pages 090
  pages/ftft...ft.txt`). Nothing extra is written into the folder.
- **Two items with one title:** told apart by " (2)". The default shapes have none.

## What it shows
- **Contents still repeat:** at whole degrees the units repeat (every 40 degrees on pages, image,
  models, binary and books, every 72 on audio, tracks, video and movies). So only 225 of the 3,240
  contents differ.
- **Names now tell the items apart:** every name is different, as every item is.
- **Size:** the `.sieve` is now 391,724 bytes, up from 303,829, because the names are longer. The
  3,240 items are still 566,236 bytes.

## For the next schema (manifest and map together)
- **Recorded in IDEAS §16.3:** the design we agreed, as notes, not yet built.
- **Items, not bytes:** an item is named by its line (written once) and its number, or its bearing.
  Its name, folder and bytes are derived, and ranges make a regular sample one entry a line.
- **Hashes:** one over the whole tree, always, and one per line. Per-file hashes are off for items.
  Where the tree hash fails, a command lists every regenerated file's SHA-256 on both machines, to
  find which file differs.
- **The installer** carries the code that makes the items.

## 16.6 The settings as a slide-in panel
- **The panel:** the settings, GLOBAL down to ENTER THE HALLWAY, slide in from the left to where
  they stood. **Tab**, or the SETTINGS tab at the left edge, slides them out, and the map takes the
  whole width. Tab again, or moving through the settings (arrows, PgUp/PgDn, Enter), brings them
  back.
- **While calculating:** once the dimensions have been calculating for a fifth of a second, the
  panel slides out by itself, so the bars can be watched changing. It comes back when the counting
  is done, unless you closed it by hand. Tab brings it back in the meantime.
- **Keys:** they act on the settings whether the panel is in or out.
- **Not part of it:** the footers, the over-budget line and the "Calculating Dimensions..." banner.
- **On opening:** the first counts when the menu opens slide it out too, if they take longer than
  a fifth of a second (a Debug build, a large stack).

## Checked here (Linux)
- **The panel:** screenshots out (Tab) and back (Tab, Down), both now in CI. I could not catch
  the slide-out while calculating in a picture; please check it on Windows.
- **CI lines:** the sample made twice and identical; installed by `sieve install`, identical.
- **Bearings in the navigator:** 90 degrees on pages and 13 on models, each taken and saved, then
  compared with the file the report names for that degree.
- **Not checked:** Windows.

## Files
- `client/item_save.cpp`, `client/hallway.hpp`, `client/app_main.cpp`, `client/menu.{hpp,cpp}`
- `data/lang/en.txt`
- `.github/workflows/build.yml`
- `README.md`, `docs/HANDOFF.md`, `docs/IDEAS.md` (§16.3: the next schema's design, §16.9)
