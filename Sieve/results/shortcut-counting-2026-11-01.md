# Counting the cards (1 November 2026)

Made by `tools/shortcut_tiny.py --only 2` (IDEAS §19.111), on the files of the stage 1 set but the designed files and the random bytes.

The repository's files (the stage 1 set, each up to its first 4 MiB) by kind. Each file's first half counted, its second half coded (held out). Bits a byte: the bytes as they are; each arrangement named by its rank in the counted order (the commonest first, an Elias gamma code; one not counted by an escape and its bytes); and by the counts' own share (smoothed).

| Kind | Files | Arrangement | As they are | By rank (the shuffle) | By share |
| :--- | ---: | ---: | ---: | ---: | ---: |
| text | 4 | 2 bytes | 8.00 | 5.55 | 4.12 |
| text | 4 | 3 bytes | 8.00 | 5.44 | 5.06 |
| text | 4 | 4 bytes | 8.00 | 5.49 | 6.42 |
| compressed | 4 | 2 bytes | 8.00 | 14.38 | 8.04 |
| compressed | 4 | 3 bytes | 8.00 | 19.40 | 7.99 |
| compressed | 4 | 4 bytes | 8.00 | 16.69 | 7.99 |
| data | 3 | 2 bytes | 8.00 | 8.35 | 5.18 |
| data | 3 | 3 bytes | 8.00 | 9.68 | 6.90 |
| data | 3 | 4 bytes | 8.00 | 11.56 | 7.71 |
| code | 2 | 2 bytes | 8.00 | 5.51 | 4.24 |
| code | 2 | 3 bytes | 8.00 | 5.74 | 5.65 |
| code | 2 | 4 bytes | 8.00 | 6.44 | 6.74 |
| pixels | 2 | 2 bytes | 8.00 | 5.80 | 4.26 |
| pixels | 2 | 3 bytes | 8.00 | 4.92 | 4.24 |
| pixels | 2 | 4 bytes | 8.00 | 4.69 | 5.42 |
| program | 1 | 2 bytes | 8.00 | 7.72 | 5.49 |
| program | 1 | 3 bytes | 8.00 | 8.64 | 5.94 |
| program | 1 | 4 bytes | 8.00 | 9.17 | 6.97 |

## The shuffle counted on one kind, tried on another (2-byte arrangements, bits a byte by rank)

| Counted on \ tried on | text | compressed | data | code | pixels | program |
| :--- | ---: | ---: | ---: | ---: | ---: | ---: |
| text | 5.55 | 19.10 | 10.98 | 8.27 | 18.30 | 19.29 |
| compressed | 12.78 | 14.38 | 13.49 | 13.53 | 11.88 | 12.99 |
| data | 13.71 | 16.44 | 8.35 | 14.39 | 15.14 | 15.80 |
| code | 7.18 | 19.05 | 10.66 | 5.51 | 17.52 | 19.24 |
| pixels | 19.78 | 19.58 | 19.87 | 18.14 | 5.80 | 17.48 |
| program | 13.29 | 17.08 | 15.54 | 13.35 | 13.32 | 7.72 |
