# Every arrangement of a small file (1 November 2026)

Made by `tools/shortcut_tiny.py --only 1` (IDEAS §19.111), before skip-and-insert, columns and palette were added.

Each set holds 65,536 files; naming one of them takes 16 bits. The shortcut seed does not know the set: it codes bytes, as it would any file, so its bits include saying which bytes. Bits are the recipe's, before rounding to bytes.


## every 2-byte file

- Bits: least 23.0, median 23.0, mean 23.0, most 23.0; under 16: 0 of 65,536.
- Sum of 2^-bits over every file: 0.007812 (at most 1 for any code that installs every file exactly).
- The cheapest ten: `00 00` 23.0; `00 01` 23.0; `00 02` 23.0; `00 03` 23.0; `00 04` 23.0; `00 05` 23.0; `00 06` 23.0; `00 07` 23.0; `00 08` 23.0; `00 09` 23.0.
- The dearest three: `ff fd` 23.0; `ff fe` 23.0; `ff ff` 23.0.
- Transforms taken: stored 65,536.
- Shortcuts used: literals only 65,536.

## 16 bytes, each 00 or FF

- Bits: least 45.6, median 99.3, mean 98.4, most 118.9; under 16: 0 of 65,536.
- Sum of 2^-bits over every file: 0.000000 (at most 1 for any code that installs every file exactly).
- The cheapest ten: `00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00` 45.6; `ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff` 45.6; `ff 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00` 54.6; `ff 00 00 00 ff 00 00 00 ff 00 00 00 ff 00 00 00` 54.6; `ff 00 00 ff 00 00 ff 00 00 ff 00 00 ff 00 00 ff` 54.6; `ff 00 ff 00 ff 00 ff 00 ff 00 ff 00 ff 00 ff 00` 54.6; `ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff 00` 54.6; `00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 ff` 54.7; `00 ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff` 54.7; `00 ff 00 ff 00 ff 00 ff 00 ff 00 ff 00 ff 00 ff` 55.7.
- The dearest three: `00 ff ff 00 ff 00 00 ff ff ff 00 00 00 ff 00 ff` 117.9; `ff 00 00 ff 00 ff ff 00 00 00 ff ff ff 00 ff 00` 117.9; `00 ff 00 00 00 ff ff ff 00 ff 00 00 ff 00 ff ff` 118.9.
- Transforms taken: none 54,032, left 3,295, diff4 2,131, diff3 2,091, triangle 1,621, up 1,067, diff2 924, diff1 374, average 1.
- Shortcuts used: copy 29,767, copy+grid 26,870, copy+again 4,653, copy+again+grid 3,306, grid 939, literals only 1.

## 8 bytes, each 00 to 03

- Bits: least 42.6, median 74.3, mean 73.3, most 75.0; under 16: 0 of 65,536.
- Sum of 2^-bits over every file: 0.000000 (at most 1 for any code that installs every file exactly).
- The cheapest ten: `00 00 00 00 00 00 00 00` 42.6; `01 01 01 01 01 01 01 01` 42.6; `01 01 01 01 02 02 02 02` 42.6; `01 01 01 02 02 02 03 03` 42.6; `02 02 02 02 02 02 02 02` 42.6; `03 03 03 03 03 03 03 03` 42.6; `01 00 00 00 00 00 00 00` 51.6; `01 00 00 00 01 00 00 00` 51.6; `01 00 00 01 00 00 01 00` 51.6; `01 00 01 00 01 00 01 00` 51.6.
- The dearest three: `03 03 03 02 03 01 00 02` 75.0; `03 03 03 02 03 01 01 00` 75.0; `03 03 03 02 03 02 00 01` 75.0.
- Transforms taken: stored 26,161, none 20,567, diff4 4,011, diff3 3,729, diff2 2,558, average 2,457, up 2,091, left 1,665, diff1 1,439, triangle 858.
- Shortcuts used: literals only 41,018, copy 13,480, grid 7,185, run 2,075, copy+grid 826, run+grid 771, copy+run 165, copy+again 16.

## 4 bytes, each 00 to 0F

- Bits: least 34.6, median 41.0, mean 41.0, most 41.0; under 16: 0 of 65,536.
- Sum of 2^-bits over every file: 0.000000 (at most 1 for any code that installs every file exactly).
- The cheapest ten: `00 00 00 00` 34.6; `01 01 01 01` 34.6; `01 01 01 02` 34.6; `01 01 02 02` 34.6; `01 02 03 04` 34.6; `02 02 02 02` 34.6; `02 02 02 04` 34.6; `02 02 04 04` 34.6; `02 04 06 08` 34.6; `03 03 03 03` 34.6.
- The dearest three: `0f 0f 0f 0c` 41.0; `0f 0f 0f 0d` 41.0; `0f 0f 0f 0e` 41.0.
- Transforms taken: stored 65,317, none 83, diff3 61, diff2 46, diff1 29.
- Shortcuts used: literals only 65,317, run 186, copy 33.
