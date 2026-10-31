# A PNG's rows carried unfiltered under temporary, measured (ROADMAP D.1)

Made by `tools/png_unfiltered_probe.py` and Sieve's Release build, 31 October 2026. For each PNG:

- **temporary today:** the PNG alone, `sieve locate --items --compressed-temporary`, the `.sieve`'s
  size;
- **rows unfiltered:** the same rows with every filter undone, written as a PNG of their own (filter
  0 on every row, zlib 9), carried the same way;
- **filters:** the original's filter bytes (one a row), by `xz -9e`;
- **skeleton:** preflate's skeleton for each, as the listing names it (`sk <skeleton> <inner>`).

The probe's PNG has the same pixels as the original (checked by Pillow on icon128).

| PNG | size | written by | temporary today | rows unfiltered | filters | skeleton, original / probe's |
| :--- | :--- | :--- | ---: | ---: | ---: | ---: |
| icon128 | 128×128 RGBA | another program | 19,775 | 10,468 | 100 | 1,657 / 534 |
| m1_sieve_scowl60 | 1500×900 RGBA | another program | 84,640 | 58,438 | 252 | 7,537 / 213 |
| example_book_cover | 160×160 RGB | another program | 150 | 215 | 80 | |
| hallway-door | 1280×720 RGB | Sieve (stb) | 7,882 | 18,296 | 120 | |
| hallway-compact | 1280×720 RGB | Sieve (stb) | 16,058 | 14,161 | 156 | |

**Reading it:**
- The way carries the original's skeleton, not the probe's, and the filter bytes too.
- At worst (the skeletons' difference and the filter bytes not packed at all):
  - icon128: 10,468 + 1,123 + 100 = 11,691, 40.9% under 19,775;
  - the scowl: 58,438 + 7,324 + 252 = 66,014, 22.0% under 84,640.
- **Larger on small and flat pictures:** the book cover and hallway-door. So it is weighed, file by
  file.
- **The two hallway screenshots are carried by `stb` as their pixels already.**
  - hallway-compact's difference is the way those pixels were packed: LZMA2, 15,957 bytes, against
    mixing as a picture (`mix4 B22 I3841x1`), 14,059 (ROADMAP D.2).
  - hallway-door's goes the other way: LZMA2 is the better there.
