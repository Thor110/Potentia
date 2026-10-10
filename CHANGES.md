# The settings panel scrolls

Against `f6c116d` (worlds).

## The list scrolls
- **What scrolls:** the settings, GLOBAL down to BINARY. FIND MY LIMITS, RESET EVERY SHAPE and
  ENTER THE HALLWAY stay at the foot.
- **The mouse wheel** over the panel moves the list three rows a notch.
- **Up and Down** bring the row you choose into view, with its section's heading.
- **A thin bar** at the panel's right edge shows where you are in the list, when there is more of
  it than the room.
- **Rows at the edges** of the room are cut off there, rather than running into the actions.

## The chosen row
- **Bunched up:** the highlight sat 3 px above its text and 5 px below, so the chosen row (filter
  memory, in your picture) crowded the row after it. It is now centred, 4 px either side.

## The menu's size
- **Height:** the menu was being scaled down to show every row at once. At 1920 x 1080 it was
  drawn at 95%, which is why everything fitted in your picture.
- **Now:** it need only be tall enough for GLOBAL and the three actions. At 1920 x 1080 it draws
  at full size, and the list scrolls by about two rows; the next dimension adds more.
- **Width:** at 1280 wide the map's last column (the second BINARY) ran off the right edge. The
  menu's narrowest width now takes the map's columns at their narrowest, 80 px each (now a named
  constant, `kMapPitchMin`), so a narrower window shows it whole, scaled down.

## Docs
- **README:** the setup menu's paragraph.
- **IDEAS §16.6:** a note of the scrolling.
- **HANDOFF:** a new entry.

## Checked here (Linux)
- **Screenshots:**
  - 1920 x 1080 and 1280 x 720, at the top of the list and at BINARY;
  - the filters window and the alphabet picker at 1280 x 720.
- **CI's setup-menu checks:** run here, all pass.
- **Not checked:** I have not built on Windows.

## Seen, not changed
- **The filters window's footer at 1280 wide:** its key line runs past the window's right edge.
  It did before this change too.
