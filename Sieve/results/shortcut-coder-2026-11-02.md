# The shortcut seed's coder, change by change (2 November 2026)

Each file's first 32 KiB (README its first 4 KiB), searched by `tools/shortcut_seed.py` with every transform, each column one more change to how the recipe's steps are coded (CODER in the tool): the choice of literal or shortcut counted by the step before; a second recent distance to copy from again; the literals counted by the two bytes before (falling back to one and none), learned from the literals alone or from every byte. Bytes; xz -9e (raw LZMA2) beside.

| File | xz | first coder | + choices by the step before | + a second recent distance | + literals by 2 bytes, from literals | + literals by 2 bytes, from every byte |
| :--- | ---: | ---: | ---: | ---: | ---: | ---: |
| README.md | 2078 | 2082 | 2081 | 2081 | 2081 | 2081 |
| pack.cpp | 10189 | 10368 | 10337 | 10339 | 10339 | 10339 |
| pg996.txt | 11727 | 12000 | 11966 | 11966 | 11855 | 11966 |
| sieve-program.elf | 6887 | 8214 | 8200 | 8118 | 8118 | 8118 |
| utah_teapot.obj | 5860 | 6458 | 6388 | 6366 | 6366 | 6366 |
| designed-checker.bin | 83 | 20 | 20 | 21 | 21 | 21 |
| designed-gradient.bin | 401 | 13 | 13 | 13 | 13 | 13 |

## The matched literal (a literal just after a copy counted by the byte the copy would give next)

| File | xz | without the matched literal | with it |
| :--- | ---: | ---: | ---: |
| pack.cpp | 10189 | 10339 | 10339 |
| pg996.txt | 11727 | 11855 | 11967 |
| sieve-program.elf | 6887 | 8115 | 8115 |
| utah_teapot.obj | 5860 | 6359 | 6359 |

Counted as whole bytes, a table for each byte a copy would give, it does not help (LZMA codes it bit by bit, so what it learns of one byte carries to like ones); left off.
