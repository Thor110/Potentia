# Files open in the viewer made for their kind

## The viewer (IDEAS 19.200, its second step)
- A file on the binary line now opens by its kind (file-kinds-v1, read from its first bytes) in the viewer made for that kind, with its bytes (HEX) as the next tab:
  - PNG, JPEG, GIF and BMP as their picture, read by the same reader as sieve-view (`client/picture_source.cpp`, now also built into the hallway), every limit checked before anything is decoded; a GIF's frames play.
  - TXT, BOOK and MANIFEST as their text.
- Which kind goes to which viewer is a table in `client/viewer.cpp` (one line a kind).
- Only kinds with no viewer at all are flagged and noted in `viewers-needed.txt`.
- A picture its reader refuses is shown as its bytes, and the viewer says why.

## Ideas recorded
- IDEAS 19.203: the Character Creator: limbs added, taken away and dragged by their joints (idea only).

## CI (step 10)
- A PNG taken to the binary line opens on its PIXELS tab; a text file on its TEXT tab; neither is flagged.

## REVIEW-ROUND.md (at the root of this zip)
- What to test as a user since sieve-wireframe.zip, as boxes to tick; the remaining tasks sorted by the testing they need; and the order of this review (your testing, then the three multi-agent reviews).

## Files to delete by hand
- None.
