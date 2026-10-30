# PNG and JPEG under all three methods: measured (30 October 2026)

Unzip over the repository root, on top of the last zip ("v32's second way: JPEGs"). Committed
locally (not pushed). Documents only.

## A flaw in permanent, for PNGs

With **permanent alone**, a PNG is carried as "what it holds" and installed as that. For a PNG,
that's its pixel rows. `tests/example_book_cover.png` (859 bytes) installs as a file named `cover`:
76,800 bytes of headerless pixels that no viewer opens. With temporary, or both boxes, it comes back
as `cover.png`, identical.

## Twelve of the repository's PNGs

`.sieve` sizes under each method. The last column is the PNG re-saved at its smallest, as permanent
would install it: every one came out pixel for pixel the original.

| PNG | original | normal | temporary | permanent | re-saved |
| :--- | ---: | ---: | ---: | ---: | ---: |
| example_book_cover | 859 | 342 | 150 | 155 | 212 |
| icon16 | 1,258 | 1,321 | 1,321 | 1,321 | 1,200 |
| icon128 | 27,749 | 27,595 | 19,775 | 19,787 | 27,649 |
| beautiful-screens | 16,289 | 11,418 | 7,217 | 7,232 | 15,705 |
| viewer-picture | 29,899 | 3,391 | 1,732 | 1,734 | 5,512 |
| font-test-french | 31,413 | 6,593 | 3,788 | 3,791 | 6,554 |
| gallery-tiles | 38,986 | 37,438 | 26,384 | 26,406 | 34,666 |
| hallway-door | 46,647 | 33,863 | 7,882 | 7,892 | 20,053 |
| hallway-compact | 53,569 | 42,171 | 16,058 | 16,060 | 24,869 |
| m1_sieve_scowl60 | 146,344 | 128,597 | 84,640 | 84,654 | 142,677 |
| long-pages-64MB-cache | 128,597 | 121,905 | 45,666 | 45,670 | 75,225 |
| long-pages-1024MB-cache | 129,272 | 122,481 | 45,971 | 45,975 | 75,327 |

- **Permanent saves nothing in the `.sieve`.** Temporary is 2 to 22 bytes smaller.
- **Re-saving at its smallest pays in the installed file:** smaller every time, the same picture.
- **The box matters a great deal for PNGs:** hallway-door's `.sieve` is 33,863 without it and 7,882
  with it.

## JPEG, the same idea

Permanent would install the same coefficients with optimised Huffman tables, which is what
`jpegtran -optimize` does. The pixels were identical every time.

- **The repository's seven:** −1.93% on Vitruvian, within ±0.2% on the rest (already optimised).
- **Your three originals:** −0.01%, −2.20% and −2.10% (measured in a scratch folder and deleted
  again).

The safety check refused building `jpegtran` itself, as code from outside, so I measured with Sieve's
own probe, using the JPEG standard's table method (Annex K).

## The design, for you to confirm

| | normal | temporary | permanent |
| :--- | :--- | :--- | :--- |
| **PNG** | as it is | its pixels, made again exactly (built) | its pixels (the same `.sieve`), installed as the same picture re-saved by Sieve at its smallest |
| **JPEG** | as it is | brunsli, made again exactly (moved under the box) | brunsli, installed with optimised tables (about 2% smaller on phone photos) |

For these two formats, permanent would then mean "the same picture, its smallest usable file", not
"what the file holds". The installed file differs from the original, as permanent's always do. Both
permanent ways are new kinds, so they'd be v32's.

Recorded in NEXT-VERSION §4a (4).

## Files
- `Sieve/docs/NEXT-VERSION.md`
