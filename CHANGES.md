# Sieve: picture filters (list Q)

One commit on top of 2db4bc0.

## Three new picture filters

Each one is an automaton, so it is counted exactly and merges with the line's other filters.

- **border-v1:** every pixel of the top and bottom rows and the left and right columns is the same colour.
- **specks-v1:** no lone specks: every pixel shares its colour with at least one of its four neighbours.
  It is only offered where its automaton fits (it is not offered on the 16-colour ega16 palette, even at
  6 x 6).
- **gradient-v1:** along every row (way = across) or down every column (way = down), the colours only rise
  or only fall, the same way throughout.

## What they keep

| Filter | Random 10 x 10 black and white kept | On ega16 | Ten real pictures pass |
| :--- | :--- | :--- | :--- |
| border-v1 | 10^-10.54 | 10^-42.14 | 4 of 10 |
| specks-v1 | 10^-3.35 | not offered | 7 of 10 |
| gradient-v1 | 10^-19.39 (across or down) | 10^-54.97 (across) | 2 of 10 |

## OPTIMISE FILTERS and X leave two of them for you

Border and gradient pick out a kind of picture (framed ones, shaded ones) rather than setting aside
noise, so ticking them for you would throw away everything else. Filters now carry a mark for this, and
X, Z, C and OPTIMISE FILTERS leave such filters alone; you tick them by hand. Specks is ticked, which moves
the optimised Image figure from 10^-18.19 to 10^-18.26 (and Books from 10^-115.95 to 10^-116.02).

## Not built: the horizon

On black and white it would only keep pictures of two flat bands, which gradient (down) already keeps.
On larger palettes it would have to carry the set of colours above every row boundary through the rows,
an automaton that grows with every subset of the palette. Recorded in IDEAS 19.245.

## Checks

- **Unit tests (step 01):** each filter against its rule written as plain code, on every picture of five
  small shapes (exact counts) and 4,000 random 10 x 10 pictures.
- **CI step 04:** OPTIMISE leaves border and gradient, ticks specks; the new optimised figures.
- **CI step 10:** the three random-picture figures.

## Files to delete by hand

None.
