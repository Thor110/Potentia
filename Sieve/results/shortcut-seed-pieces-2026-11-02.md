# The shortcut seed, stage 1 with skip-and-insert, columns and palette (2 November 2026)

The same 24 files as `shortcut-seed-2026-11-01.md`, made by `tools/shortcut_seed.py` with three pieces more: skip-and-insert (Edward's), the file read by columns, and the values a file uses said once (palette). Run in two parts (the first four files, then the rest), joined here. The skip-and-insert counts were not yet in the table when it ran: it took part in three recipes, none the best (the colour picture's up and average, the MP4's diff2).

Each file's first 32,768 bytes (the whole file where smaller). The shortcut seed's best recipe (its transform, then how many steps were literal bytes and how many shortcuts of each kind), every recipe decoded again and checked byte for byte; beside it zlib -9, xz -9e (raw LZMA2), the rope's search (5 s) and mixing (mix-v4), all on the same bytes. W: the row width guessed for the grid and the waves (0: none).

| File | Bytes | W | Shortcut seed | Its transform | Literals / shortcuts (copy, again, run, grid) | xz | Mixing | Rope | zlib |
| :--- | ---: | ---: | ---: | :--- | :--- | ---: | ---: | ---: | ---: |
| sieve-program.elf | 32,768 | 48 | 8,217 | none | 3,793 / 2,381 (1545, 472, 0, 364) | 6,887 | 7252 | 17884 | 8,412 |
| picture-1280x720.rgb | 32,768 | 3840 | 133 | none | 15 / 52 (33, 6, 0, 13) | 186 | 104 | 358 | 168 |
| pg996.txt | 32,768 | 5 | 12,019 | palette | 4,053 / 4,206 (4185, 3, 0, 18) | 11,727 | 9988 | 21617 | 12,611 |
| docs.sieve | 32,768 | 0 | 32,773 | stored | 32,768 / 0 (0, 0, 0, 0) | 32,772 | 34085 | 33776 | 32,784 |
| designed-checker.bin | 32,768 | 512 | 20 | none | 5 / 3 (1, 1, 0, 1) | 83 | 30 | 26 | 171 |
| designed-diagonal+noise.bin | 32,768 | 256 | 7,162 | none | 2,680 / 2,545 (1176, 372, 0, 997) | 5,850 | 4749 | 6192 | 8,076 |
| designed-diagonal-250.bin | 32,768 | 0 | 562 | palette | 381 / 131 (1, 130, 0, 0) | 486 | 622 | 1500 | 685 |
| designed-diagonal.bin | 32,768 | 256 | 591 | none | 383 / 127 (0, 0, 0, 127) | 464 | 586 | 1510 | 695 |
| designed-gradient.bin | 32,768 | 256 | 13 | left | 2 / 1 (1, 0, 0, 0) | 401 | 464 | 167 | 659 |
| designed-noise.bin | 32,768 | 0 | 32,773 | stored | 32,768 / 0 (0, 0, 0, 0) | 32,772 | 34055 | 33780 | 32,784 |
| designed-two-directions.bin | 32,768 | 256 | 593 | none | 383 / 127 (0, 0, 0, 127) | 465 | 596 | 1508 | 698 |
| designed-vertical.bin | 32,768 | 256 | 269 | none | 257 / 1 (0, 0, 0, 1) | 345 | 296 | 1006 | 467 |
| sieve_ref.py | 32,768 | 0 | 10,457 | palette | 3,808 / 3,448 (3303, 144, 1, 0) | 10,116 | 9425 | 19609 | 10,747 |
| IDEAS.md | 32,768 | 45 | 13,556 | palette | 4,161 / 4,760 (4718, 14, 0, 28) | 13,311 | 11529 | 19714 | 14,042 |
| pack.cpp | 32,768 | 0 | 10,379 | palette | 3,465 / 3,457 (3407, 48, 2, 0) | 10,189 | 9102 | 20085 | 10,706 |
| utah_teapot.obj | 32,768 | 31 | 6,511 | palette | 3,244 / 2,339 (1928, 48, 0, 363) | 5,860 | 4767 | 12393 | 6,863 |
| roundhay.mp4 | 32,768 | 0 | 31,773 | none | 30,994 / 267 (254, 13, 0, 0) | 31,737 | 32529 | 33432 | 31,680 |
| Vitruvian.jpg | 32,768 | 0 | 32,416 | none | 32,135 / 93 (72, 4, 17, 0) | 32,634 | 33197 | 33636 | 32,514 |
| picture.png | 32,768 | 4 | 28,697 | none | 26,279 / 1,082 (979, 103, 0, 0) | 28,678 | 29401 | 32780 | 29,000 |
| vitruvian-centre-256.gray | 32,768 | 256 | 22,177 | triangle | 24,120 / 2,398 (1929, 0, 0, 469) | 23,492 | 22951 | 22228 | 25,414 |
| random.bin | 32,768 | 0 | 32,773 | stored | 32,768 / 0 (0, 0, 0, 0) | 32,772 | 34082 | 33780 | 32,784 |
| universalbibliothek.txt | 21,084 | 4 | 8,567 | palette | 2,400 / 3,189 (3168, 20, 0, 1) | 8,490 | 7187 | 12947 | 8,859 |
| m1_sieve_scowl60.csv | 17,456 | 7 | 8,478 | palette | 3,378 / 3,930 (3801, 22, 0, 107) | 7,583 | 7030 | 8558 | 8,881 |
| README.md | 9,469 | 8 | 4,245 | palette | 1,958 / 1,311 (1268, 24, 1, 18) | 4,195 | 3708 | 6352 | 4,328 |
| hurrian-hymn-6.mid | 1,377 | 10 | 177 | none | 64 / 53 (44, 8, 0, 1) | 172 | 136 | 430 | 190 |

## The two prongs

Forward: the shortcut seed searched on what beat it (xz's output, mixing's code): smaller than that output only if a shortcut into it exists. Back: each transform's output put through xz and through mixing, a byte added to say which transform (the best transform shown): smaller than xz or mixing alone only if the shortcut in front helps it.

| File | xz | Seed of xz's output | Mixing | Seed of mixing's code | Transform, then xz | Transform, then mixing |
| :--- | ---: | ---: | ---: | ---: | :--- | :--- |
| sieve-program.elf | 6,887 | 6,891 | 7252 | 7256 | 9,063 (diff1) | 10,105 (diff1) |
| picture-1280x720.rgb | 186 | 189 | 104 | 107 | 187 (triangle) | 111 (up) |
| pg996.txt | 11,727 | 11,731 | 9988 | 9992 | 14,397 (diff1) | 13,639 (diff1) |
| docs.sieve | 32,772 | 32,777 | 34085 | 34090 | 32,773 (diff1) | 34,065 (diff1) |
| designed-checker.bin | 83 | 86 | 30 | 32 | 84 (up) | 24 (up) |
| designed-diagonal+noise.bin | 5,850 | 5,854 | 4749 | 4753 | 6,035 (columns) | 5,291 (columns) |
| designed-diagonal-250.bin | 486 | 489 | 622 | 625 | 581 (diff1) | 689 (diff1) |
| designed-diagonal.bin | 464 | 467 | 586 | 589 | 470 (columns) | 733 (diff1) |
| designed-gradient.bin | 401 | 404 | 464 | 467 | 76 (left) | 17 (left) |
| designed-noise.bin | 32,772 | 32,777 | 34055 | 34060 | 32,773 (diff1) | 34,069 (diff2) |
| designed-two-directions.bin | 465 | 468 | 596 | 599 | 469 (columns) | 725 (diff1) |
| designed-vertical.bin | 345 | 348 | 296 | 299 | 330 (columns) | 290 (up) |
| sieve_ref.py | 10,116 | 10,120 | 9425 | 9429 | 12,717 (diff1) | 13,126 (diff1) |
| IDEAS.md | 13,311 | 13,315 | 11529 | 11533 | 16,472 (diff1) | 15,544 (diff1) |
| pack.cpp | 10,189 | 10,193 | 9102 | 9106 | 12,656 (diff1) | 12,689 (diff1) |
| utah_teapot.obj | 5,860 | 5,864 | 4767 | 4771 | 7,374 (diff1) | 6,353 (diff1) |
| roundhay.mp4 | 31,737 | 31,742 | 32529 | 32534 | 32,031 (diff1) | 32,828 (diff1) |
| Vitruvian.jpg | 32,634 | 32,639 | 33197 | 33202 | 32,751 (diff1) | 33,581 (diff1) |
| picture.png | 28,678 | 28,683 | 29401 | 29406 | 30,293 (diff1) | 31,316 (diff1) |
| vitruvian-centre-256.gray | 23,492 | 23,497 | 22951 | 22956 | 21,617 (triangle) | 21,105 (triangle) |
| random.bin | 32,772 | 32,777 | 34082 | 34087 | 32,773 (diff1) | 34,072 (diff3) |
| universalbibliothek.txt | 8,490 | 8,494 | 7187 | 7191 | 10,729 (diff1) | 9,950 (diff1) |
| m1_sieve_scowl60.csv | 7,583 | 7,587 | 7030 | 7034 | 8,455 (columns) | 7,886 (columns) |
| README.md | 4,195 | 4,199 | 3708 | 3712 | 5,233 (diff1) | 5,099 (diff1) |
| hurrian-hymn-6.mid | 172 | 175 | 136 | 139 | 200 (diff1) | 167 (diff1) |

## Every transform

| File | none | diff1 | diff2 | diff3 | diff4 | left | up | average | triangle | columns | palette | stored |
| :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| sieve-program.elf | 8,217 | 10,341 | 11,982 | 13,047 | 13,216 | 10,972 | 16,702 | 18,118 | 17,365 | 14,075 | 8,248 | 32,773 |
| picture-1280x720.rgb | 133 | 146 | 170 | 175 | 188 | 145 | 152 | 219 | 148 | 207 | 133 | 32,773 |
| pg996.txt | 12,263 | 15,018 | 17,844 | 19,834 | 21,316 | 21,310 | 22,528 | 25,517 | 24,810 | 20,514 | 12,019 | 32,773 |
| docs.sieve | 32,886 | 32,883 | 32,888 | 32,889 | 32,887 |  |  |  |  |  |  | 32,773 |
| designed-checker.bin | 20 | 22 | 22 | 26 | 27 | 22 | 23 | 31 | 23 | 36 | 20 | 32,773 |
| designed-diagonal+noise.bin | 7,162 | 8,788 | 9,721 | 9,991 | 10,246 | 8,795 | 11,033 | 10,057 | 11,116 | 7,616 | 7,176 | 32,773 |
| designed-diagonal-250.bin | 564 | 631 | 843 | 953 | 793 |  |  |  |  |  | 562 | 32,773 |
| designed-diagonal.bin | 591 | 662 | 862 | 829 | 845 | 726 | 858 | 990 | 910 | 677 | 593 | 32,773 |
| designed-gradient.bin | 327 | 16 | 18 | 19 | 19 | 13 | 13 | 13 | 13 | 726 |  | 32,773 |
| designed-noise.bin | 32,885 | 32,886 | 32,890 | 32,889 | 32,885 |  |  |  |  |  |  | 32,773 |
| designed-two-directions.bin | 593 | 660 | 861 | 828 | 845 | 723 | 856 | 990 | 932 | 677 | 596 | 32,773 |
| designed-vertical.bin | 269 | 269 | 270 | 271 | 272 | 269 | 269 | 525 | 269 | 537 | 280 | 32,773 |
| sieve_ref.py | 10,471 | 13,148 | 15,473 | 17,334 | 18,515 |  |  |  |  |  | 10,457 | 32,773 |
| IDEAS.md | 13,686 | 17,069 | 20,149 | 22,689 | 24,450 | 18,083 | 28,026 | 29,015 | 28,224 | 20,115 | 13,556 | 32,773 |
| pack.cpp | 10,492 | 13,020 | 15,474 | 17,449 | 18,801 |  |  |  |  |  | 10,379 | 32,773 |
| utah_teapot.obj | 6,554 | 8,044 | 9,209 | 10,582 | 11,823 | 10,087 | 16,792 | 17,760 | 17,600 | 16,806 | 6,511 | 32,773 |
| roundhay.mp4 | 31,773 | 31,964 | 32,056 | 32,129 | 32,102 |  |  |  |  |  |  | 32,773 |
| Vitruvian.jpg | 32,416 | 32,521 | 32,567 | 32,590 | 32,601 |  |  |  |  |  |  | 32,773 |
| picture.png | 28,697 | 30,141 | 30,785 | 31,123 | 31,355 | 31,078 | 31,355 | 31,598 | 31,559 | 31,044 |  | 32,773 |
| vitruvian-centre-256.gray | 25,155 | 23,592 | 25,627 | 26,281 | 26,599 | 23,575 | 24,955 | 23,632 | 22,177 | 25,238 | 25,158 | 32,773 |
| random.bin | 32,890 | 32,887 | 32,886 | 32,886 | 32,886 |  |  |  |  |  |  | 32,773 |
| universalbibliothek.txt | 8,632 | 11,071 | 13,243 | 14,977 | 15,951 | 15,510 | 15,950 | 17,968 | 17,465 | 13,074 | 8,567 | 21,089 |
| m1_sieve_scowl60.csv | 8,524 | 9,731 | 10,040 | 10,128 | 10,306 | 10,098 | 10,643 | 10,515 | 10,542 | 9,129 | 8,478 | 17,461 |
| README.md | 4,291 | 5,333 | 6,205 | 6,811 | 7,220 | 6,660 | 7,766 | 8,219 | 7,960 | 6,127 | 4,245 | 9,473 |
| hurrian-hymn-6.mid | 177 | 204 | 221 | 227 | 235 | 228 | 298 | 334 | 269 | 307 | 193 | 1,381 |
