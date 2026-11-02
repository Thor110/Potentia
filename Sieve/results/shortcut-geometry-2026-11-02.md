# The geometric filters (2 November 2026)

Made by `tools/shortcut_designs.py --real ...` (IDEAS §19.111): eight designed grids, each built by one shape, then five real files of the stage 1 set.

Each designed file is 64 rows of 64 bytes built by one shape; each real file its first 32 KiB in rows of the width given. The best is over every transform; the columns are what no transform and each geometric filter came to (bytes, its numbers paid).

| File | Bytes | W | Best | Its transform | xz | none | zigzag | diagonals | spiral | rings | morton | tiles | plane | radial |
| :--- | ---: | ---: | ---: | :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| spiral count | 4,096 | 64 | 11 | spiral | 2,652 | 2,205 | 2,202 | 4,140 | 11 | 3,782 | 3,995 | 2,793 | 2,889 | 3,119 |
| rings | 4,096 | 64 | 180 | rings | 890 | 927 | 927 | 1,307 | 582 | 180 | 2,304 | 1,640 | 933 | 580 |
| plane | 4,096 | 64 | 14 | up | 235 | 185 | 185 | 366 | 259 | 2,618 | 336 | 272 | 16 | 189 |
| slope from the centre | 4,096 | 64 | 521 | diff1 | 1,126 | 1,119 | 1,119 | 1,372 | 667 | 619 | 2,713 | 1,802 | 1,125 | 592 |
| diagonal stripes | 4,096 | 64 | 13 | none | 42 | 13 | 55 | 207 | 133 | 1,357 | 106 | 13 | 122 | 170 |
| snake count | 4,096 | 64 | 11 | zigzag | 265 | 24 | 11 | 538 | 476 | 3,276 | 305 | 103 | 191 | 28 |
| Morton count | 4,096 | 64 | 11 | morton | 297 | 297 | 297 | 498 | 1,170 | 3,894 | 11 | 271 | 2,058 | 301 |
| tiles of one value | 4,096 | 64 | 122 | columns | 138 | 122 | 203 | 720 | 341 | 990 | 123 | 123 | 1,167 | 1,712 |
| vitruvian-centre-256.gray | 32,768 | 256 | 21,926 | triangle | 23,492 | 25,017 | 25,040 | 25,243 | 25,122 | 25,248 | 25,230 | 25,204 | 24,858 | 25,093 |
| picture-1280x720.rgb | 32,768 | 3840 | 132 | palette | 186 | 133 | 199 | 221 | 200 | 244 | 318 | 282 | 203 | 137 |
| sieve-program.elf | 32,768 | 48 | 8,118 | none | 6,887 | 8,118 | 10,014 | 15,593 | 14,280 | 14,813 | 14,820 | 10,966 | 14,404 | 15,902 |
| utah_teapot.obj | 32,768 | 31 | 6,366 | palette | 5,860 | 6,412 | 9,891 | 16,274 | 16,734 | 11,931 | 15,280 | 11,535 | 6,420 | 6,418 |
| designed-diagonal.bin | 32,768 | 256 | 585 | none | 464 | 585 | 804 | 1,129 | 830 | 12,895 | 1,054 | 709 | 1,133 | 6,448 |
