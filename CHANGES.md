# Changes

One commit on top of c1ebc60 ("sieve N5"). List N5 is now finished.

## Degree window on the binary and fractals lines (N5-3)
- `degree-window-v2` can now be ticked on the binary line and the fractals line. It is the same filter with the
  same from, width and decimals settings, and nothing was added to any menu.
- **Binary line:** a file's bearing is its place among every file up to the line's length (shortest first, then by
  its bytes).
  - The window keeps the part of the other filters' survivors that falls in it: binary-kind, not-an-item's pages
    and utf8-valid.
  - It counts and ranks there, so the binary line compacts to the window.
  - On the 65,793 files of up to 2 bytes, one whole degree keeps 183 files.
- **Fractals line:** a ruleset's bearing is its place on the line's list. It judges (hide) there, as that line's
  other filters do.

## Counting and new filters
- **melody-contrary-v1 counted (N5-11):** on two voices its survivors are now counted (not ranked) by a walk over
  both voices at once. The count was checked against every unit of 12 small lines.
  - The setup menu's default 16 notes a voice takes about 8 s here.
  - With a percent below 100 it counts up to 5 notes a voice and judges past that.
  - The setup menu now shows a counted but unranked stack's survivors, where it used to say they could not be
    counted.
- **Join filters (N5-14):** on the joined filters of tracks and movies, two filters judge only where one unit meets
  the next. Both count and rank.
  - `join-leap-v1`: a track's step across a join is at most 7 semitones. The default tracks keep 10^-0.79.
  - `join-cut-v1`: a movie's unit begins within 2 pixels of the frame the unit before ended on. The default movies
    keep 10^-15.04.
  - Counting a joined stack together with each unit's own filters is still open.
- **Models line (N5-15):** three new filters, judged only, because an edge joins faces and the line's closed forms
  cannot count them.
  - `mesh-edges-v1`: no edge in more than two faces.
  - `mesh-closed-v1`: a closed, consistently wound surface.
  - `mesh-area-v1`: no face of zero area.
- **AI line (N5-16):** `weight-signs-v1` keeps rows with at least 10% of their weights on each side of zero. It
  counts and ranks.
  - The shipped run's model passes at 10% and fails at 15%.
  - A filter keeping weights off the ends of the scale was dropped: trained 4-bit weights crowd the ends (19.7%
    against noise's 12.5%), so it set the trained model aside.
- **OPTIMISE FILTERS (N5-17):** measured on every line. The figures are in Proof/STATISTICS.md, and the lines
  measured before are unchanged.

## Files to delete by hand
None.
