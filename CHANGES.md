# PNG and JPEG under all three methods (31 October 2026)

Unzip this over the repository root, on top of the last zip ("rope direction"). It is committed
locally, not pushed. Everything here is behind `--v32-draft`. Without it, every output is byte for
byte what the build before made: the twelve PNGs and the seven JPEGs were measured with no box,
temporary and permanent.

New files: `tools/cli/png_resave.{hpp,cpp}`, added to `sieve_compare` in CMakeLists.txt.

## What each method does now (with the draft)

| | no box | temporary | permanent |
| :--- | :--- | :--- | :--- |
| **JPEG** | as it is | brunsli form, given back byte for byte (`jbr`) | the same coefficients with optimised Huffman tables, carried as its brunsli form; else as temporary |
| **PNG** | as it is | made again exactly (as before) | **re-saved at its smallest** (`pngr`), the same picture under its own name; else made again exactly; else as it is |

With both boxes, a file is made again exactly first.

- **JPEGs and the boxes:** a JPEG now follows the boxes, as other compressed formats do. Before,
  the draft used brunsli with no box ticked. So the CI's JPEG check now passes
  `--compressed-temporary`.
- **The PNG flaw:** a PNG is never installed as its pixel rows any more (v31 installed
  `example_book_cover.png` as a 76,800-byte file named `cover`).

## Measured

**PNG under permanent**, each of the twelve on its own:

| PNG | original | installed | `.sieve` temporary | `.sieve` permanent |
| :--- | ---: | ---: | ---: | ---: |
| example_book_cover | 859 | 202 | 151 | 183 |
| icon128 | 27,749 | 17,537 | 19,776 | 11,376 |
| hallway-door | 46,647 | 20,053 | 7,883 | 7,914 |
| m1_sieve_scowl60 | 146,344 | 105,324 | 84,641 | 48,136 |
| long-pages-64MB-cache | 128,597 | 58,841 | 45,667 | 45,704 |

The other seven are in FORMATS.md.

- Every PNG was installed smaller, with the same pixels (checked by Pillow).
- Sieve, sieve-install and the oracle gave back the same bytes.
- The installed PNGs were smaller than Pillow's re-save on eleven of the twelve, and the same on
  hallway-door.
- The `.sieve` under permanent is often smaller than under temporary, since rows carried
  unfiltered pack better. Where it's larger, it is by 31 to 43 bytes.

**JPEG under permanent:**
- **The repository's seven:** Vitruvian.jpg went from 207,500 to 203,455 bytes (−1.95%). The other
  six are optimised already, so they come back exact.
- **JPEGs Pillow wrote with its standard tables:** 9% to 11% smaller, for example baseline 172,212
  to 156,746. Restart markers, greyscale and 4:4:4 all work.
- **Progressive JPEGs** are left exact; the optimiser doesn't handle them yet.

Every picture had the same pixels.

## Checked here

- Unit suite: 77,110 checks, 0 failures.
- CI step 11: the new lines (JPEG with no box, temporary and permanent; PNG permanent; without the
  draft), run on their own, pass.
- The oracle made all twelve re-saved PNGs and agreed with Sieve.

## Changed

- `pack.cpp`: the PNG branch, the JPEG step under the boxes, and two new report lines, `optimised`
  and `pngs`.
- `jpeg_glue`: `jpeg_optimised`.
- `locate.cpp`: the reader takes `pngr`.
- `sieve_ref.py`: `pngr_make`.
- `help.cpp`: the help text.
- CI step 11: the new checks.
- Docs: SPECIFICATIONS v32, METHODS §2a, FORMATS (now "decided and built"), NEXT-VERSION §4a item
  4, HANDOFF.
- IDEAS §19.88: an idea found on the way. Temporary could carry a PNG's rows unfiltered too and
  still give back the original exactly. Not measured.
