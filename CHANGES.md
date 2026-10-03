# Filter memory: counted only on X (relative to origin/main 86ae73f)

`this-round-only.patch` applies on top of the filter-memory round. `files/` and
`full-vs-origin-main.patch` include the earlier rounds not yet on origin/main (the books bar and
message, and the filter memory setting).

## What changed
- **Changing the filter memory row only saves it.** Nothing is counted again, so it can be stepped
  freely without stutter.
- **A red line under the four memory bars:** "Memory Limit Change Detected : Press X to re-optimise
  all dimensions." It shows while the setting differs from the limit the tallies were counted with.
- **X re-optimises.** It applies the new limit, unticks everything in reach and ticks it all again,
  with the clashes weighed under the new limit. Then the red line goes. X now also works on the setup
  screen itself, except on the key's row, where X is a letter of the key.
- **"Calculating Dimension... (PAGES, BOOKS)"** appears in the same place while any line is still
  being counted, or while X is weighing clashes. It names the lines still in progress and clears
  when they're done.
- **The row and the filter memory bar show your setting**, which is what X will count with, not the
  limit currently in use.
- **Going into the hallway** uses the setting as it stands, X or not.
- **Spacing:** the line under the bars is always reserved, so nothing jumps when it appears. The
  spacing around the bars is tighter, so ENTER THE HALLWAY stays clear of the footer.

## Screenshots
- `pend1.png`: the setting stepped from 512 to 768 MB. The red line shows, the bar reads 2.0 GB of
  768 MB, and nothing was re-counted.
- `pend2.png`: after X, everything is counted with 768 MB and the line is gone. The filters were
  re-ticked, not unticked.
- `calc.png`: mid-count, "Calculating Dimension... (PAGES, AUDIO, BOOKS, BINARY)".

## Checked
The setup menu checks all pass: the filter memory row saves its value, the line rows still open the
right filters, and the display cache and FIND MY LIMITS work as before.
