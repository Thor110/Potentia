# Mixing's match model with its ways (2 November 2026)

mix-v4 and the v33 draft's three ways (SPECIFICATIONS, v33): R1 (a match goes on past one wrong
byte, at the same distance), M4 (a match taken after 4 bytes, not 6), C2 (two places a context was
seen, the longer match kept). `sieve mix FILE --params "B22 ..."`, every code read back and checked
by Sieve; xz -9e (raw LZMA2) beside. Measured in this session's container.

| File | xz | mix-v4 | R1 | M4 | C2 | R1 M4 C2 |
| :--- | ---: | ---: | ---: | ---: | ---: | ---: |
| 8 MiB of Silesia's mozilla's programs (Alpha) | 1,912,125 | 1,959,809 | 1,897,128 | 1,955,004 | 1,955,962 | **1,891,229** |
| 4 MiB of Sieve's own program (x86) | 1,511,013 | 1,343,529 | 1,309,520 | 1,332,817 | 1,342,267 | **1,297,930** |
| Don Quixote (2,391,678 bytes) | 685,794 | 537,483 | 539,870 | 543,070 | **536,458** | 544,027 |
| a screenshot's pixels (2,764,800 bytes, `I1280x3`) | 37,439 | **28,494** | 28,773 | 28,561 | 28,534 | 28,565 |

Silesia's mozilla whole (51,220,480 bytes): LZMA2 13,376,180; mix-v4 13,778,604; with R1
13,404,105; with R1 M4 C2 **13,318,505** (coded in 86.9 s, read back in 88.3 s; mix-v4's were
79.9 s and 82.5 s).

Through the packer (`sieve locate --search-per-file 1 --search-every-wave`): 2 MiB of mozilla's
programs 493,215 bytes at v32, 481,476 with `--v33-draft` (`mix4 ... R1 M4 C2`); 64 KiB of them
5,834 and 5,479; 64 KiB of Sieve's program 22,563 and 21,118; each installed back by Sieve, the
smaller ones by the oracle too. Without Try every way a file is mixed whole only where mixing on
its start is 3% under LZMA2, which programs mixed with the ways are not, so they stay LZMA2.
