# The dimensions as data, and the setup menu on one screen

Against `0c5c268` (survey notes ideas). Client and docs only: no address, file format or saved
setting changes. `dimensions.patch` is everything; `menu-on-one-screen.patch` is the menu part
alone, for a tree that already has the earlier dimensions.zip.

## What changed

- **New: `client/dimensions.hpp`.** `kDimensions` defines each dimension once, in door order:
  - its identity (`Media`, moved here from `world.hpp`);
  - its id ("pages") and `--line` name ("text");
  - the unit line it is, if it is one;
  - its two colours (moved here from `theme.hpp`, which now holds only the `Theme` struct);
  - its default WORLD music mode.

  `kLines`, `kBooksLine`, `kModelsLine`, `kBinaryLine`, `theme_of()`, `line_of()`, `line_named()`
  and `menu_ink()` are all derived from it. Binary must stay last, and a static_assert enforces it.
- **A door and an identity are kept apart.**
  - **By door** (where a line stands): walking, the map's columns, the HUD's rings, and the key a
    fifth further at each door. That key is now generated (`fifths_at()`) and gives the same
    C G D A E B F# as before.
  - **By identity** (what a line is): everything configured or saved per dimension. The music
    settings' `modes` list is saved in `Media` order, as before, so existing settings files read
    identically.
- **The hallway** looks up its unit lines by kind (`unit_line()`, `line_at()`), no longer as
  `lines_[0..3]`. J's routing (`open_as_kind`) does the same.
- **The setup menu** takes its start lines, focus list, map columns, section headings, filter
  windows and line sizes from the table. Its own list of start lines is now `kStartLines`.
- **The media player's** per-line music modes, and the music's "world/<line>" ids, also come from
  the table.
- **One visible change:** the main menu's rule under the title was five hardcoded colours, pages
  to books; models and binary had never been added. It is now one band per door, each in the
  colour the setup menu writes that line's name in.
- **Docs:**
  - IDEAS §13 records the decisions so far: the order, the colours, covers kept at both levels,
    the separate settings window ruled out, and step 1 done.
  - HANDOFF has a new entry and a row in the file map.

## The setup menu on one screen

- **The budget's four bars and status line** move from under the settings to the top right, and
  the map starts below them.
- **"the largest that fit the budget"** beside FIND MY LIMITS is gone, along with its language key.
- **Values start just past the widest label** (worked out from the rows, not fixed at 320 px), so
  every value fits whole at 1920 x 1080. One too long for its column now stops at the map's edge
  with "..", instead of running into the map; the edge is one constant, `kMapX`.
- **The menu's smallest size** is worked out from what it draws (the subtitle and the budget beside
  it, the rows and their headings). A window smaller than that shows the whole menu scaled down, as
  before.
- **Docs:** README (the setup menu, and where the filter memory bar is), IDEAS §13 (the layout
  decided), HANDOFF (an entry).
- **Screenshots:** `setup-1920x1080.png` and `setup-1280x720.png` (the same layout, scaled down).

## Not changed

- Nothing has been reordered or added: the doors are where they were.
- The setup menu's rows keep their order (GLOBAL, pages ... binary); they follow the doors' new
  order when the doors are reordered.

## Checked

- **Builds** with no warnings (Linux, GCC 13).
- **Every CI step passes,** run here:
  - same addresses, filters, models, the bytes256 line, sound, other formats, books;
  - the whole hallway step (renders, doors, scripted presses);
  - the reference oracle.
- **Unit tests:** 33,918 checks, 0 failures (and again after the menu changes, with the hallway
  step).
- **The same screens as before:** `setup-now.png` shows the setup menu, with headings and map
  columns unchanged. `main-now.png` shows the main menu, whose rule now has seven bands.
- **The reorder test:** a scratch copy with only the table reordered (image, pages, books, audio,
  video, models, binary) builds with nothing else changed.
  - `setup-reordered.png`: the map in that order.
  - `hall-reordered.png`: the image line, with its left door now leading to pages.
