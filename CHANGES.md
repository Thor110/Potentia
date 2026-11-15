# List S, step 1 reworked: the fractal dimension as a hallway line

You said the fractal screen should instead be books on a hallway line that can be picked up. You also
said every dimension we add should be its own line, made dynamic so new ones slip in easily, and that
the screen's list would still be useful as one first book. This update does all three. The separate
screen (`--fractals`) is gone.

## What changed

- **The FRACTALS line.** It is the twelfth door, after AI and before binary (`... ai  fractals |
  binary`), in light purple. Its shelves hold the fractals charted by hand in `data/fractals/chart.txt`:
  - **Book 1 is the index.** Its cover is a small drawing of every fractal charted. Its page is the
    numbered list of them.
  - **Books 2 on are the 11 fractals,** each drawn on its cover.
  - **Picked up, a fractal looks like any other item in hand.** There is a summary line, its picture,
    "depth X of Y" and its ruleset as text. The zoom keys or N and B step the depth, which starts at
    5 or the deepest it goes, whichever is less.
  - **The viewer opens on the picture,** with the ruleset under the text tab.
  - **What the line stores is each fractal's ruleset, not a picture.** F saves a fractal as
    `Title.fractal`, byte for byte as the chart holds it, and the index as a `.txt` list. The
    every-degree sample writes them the same way. The picture is only how it is shown, as you said.
    The viewer's own F still exports the view on screen, as it does on every line.
  - **Favourites and Go to work as elsewhere.** Go to takes a number or a fractal's id
    (`--warp dragon-curve`). A number past the end is refused, with a message saying so.
- **Listed lines, a new kind of line (`client/listed_shelf.hpp`, new).** These are lines whose items
  are a finite list rather than an address space. The hallway, the hand, the viewer, saving, the setup
  menu and the minimap all go through one interface, `ListedShelf`. A new dimension of this kind is a
  row in `kDimensions` (flag `listed`) and one shelf class, with no branch per line.
- **The setup page has a fractals column,** in the same bar layout as the others. The bar is full
  height, with a Koch-curve break where the other lines show their gap.
- **Removed:** `client/fractal_library.cpp/.hpp`, the `--fractals` switch, its keys and its
  language keys.
- **Docs:**
  - SPEC 12.0t rewritten for the line.
  - The door diagrams in SPEC, HANDOFF and MANUAL show twelve lines.
  - DIMENSIONS, the fractal help page, `sieve help`'s fractal text and the catalogue entry updated.
  - IDEAS 19.235 notes your three points.
  - MANUAL's sample figures were measured again: 3,972 items, 13,322,835 bytes, in a `.sieve` of
    7,919 bytes. The same sample with the fractals written as PNGs made a `.sieve` of 77,980 bytes.
- **CI:**
  - Step 4 walks the doors past FRACTALS.
  - Step 10 checks the index book, depth stepping both ways, a name not charted (refused), a number
    past the end (refused) and the new sample count. It also checks that the sample's
    `Dragon curve.fractal` matches the chart's file byte for byte, and that no PNG is written for
    the line. The setup menu's completion estimate now has twelve lines, and the corridor walk goes
    ten doors each way, through FRACTALS.
  - Every step passes (01 to 12).

## Not done

- The logo still shows the old number of lines (you said that is fine).
- A larger minimap view is noted for later.

## Files to delete by hand

- `Sieve/client/fractal_library.cpp`
- `Sieve/client/fractal_library.hpp`
