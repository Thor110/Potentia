# Changes

One commit on top of b424b5c ("sieve O4").

## Pages of a range of lengths: O5, guided order
- **Guided order on a range line** (`guided-range-v1`): the lengths in turn, shortest first, each in its own guided
  order. The code takes one step more, first, which names the length (an equal share to each length that has a
  page), then the page's own guided code as before.
- **What it costs, and saves:** naming the length costs about 5 bits on a line of 1 to 32, but a page no longer pays
  for padding. "welcome to the sieve" on 1 to 32 is `9fc46a578d1`, 44 bits, where on the line of 32 it is a padded
  page of 56.
- **Sieved:** each length is sieved by its own filters, so compact's guided view works on a range too. A degree
  window keeps a stretch of the whole range, not of each length, so with one ticked there is no guided order within
  each length, and it says so.
- **The hallway** now offers guided order on `--min-length` lines (M cycles to it), compact as well.
- **Unchanged:** lines of one length are guided as before, every address the same (`guided-ac-v1`).
- **Checked:** the oracle (`sieve_ref.py guided-range-vectors`) writes guided-range-v1 from its definition; its 46
  vectors (ranges 1-32, 1-3, 5-8, 19-21) agree with the C++ (CI steps 01 and 11). Every page of 1 to 3 letters
  tiles the line in order and reads back from its address, plain and sieved two ways. CI step 02 pins the greeting's
  guided address.

## Also
- **`tools/time_open_image.py` and `.bat`:** how long a picture takes to open as itself and as its `.simage`, and to
  install back, checked byte for byte. Drop a picture on the `.bat`.
- **STATISTICS:** two new sections. Ruler01.tga, a KotOR II texture: the `.simage` is 20.41% of the file,
  0.34% under the 7z, and 15% smaller again under Size; opening it took 14.9 ms (4.9%) longer than the TGA on
  your machine. Four War of the Worlds `.WOF` files (0.13.79): the `.sieve` the smallest of zip, 7z and `.sieve`
  for each, 2.75% under the 7z all together.

## Files to delete by hand
None.
