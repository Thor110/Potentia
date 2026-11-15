# Setup page: the FRACTALS column

Two things you found on the setup page after the fractal line went in.

## What changed

- **The FRACTALS column no longer has a magnifying glass.** The line has no filters, so the glass
  did nothing. Now it isn't drawn, and F on that line does nothing either (`client/menu.hpp`,
  `open_filters`).
- **The line names no longer run into the next column.** With the settings panel closed, the names
  were drawn at double size wherever a column was 110 pixels wide. "FRACTALS" at double size is
  wider than that, so it ran into BINARY. Now every name is measured with its glass and dots, and all
  the names are drawn at the largest size where every one of them fits: double, one and a half, or
  plain. At 1920 wide with the panel closed that is one and a half. With the panel open it is plain,
  as before.
- **CI:** step 10 passes.

## Files to delete by hand

- `Sieve/client/fractal_library.cpp`
- `Sieve/client/fractal_library.hpp`

They are still in the repository from the earlier update, and nothing builds them.
