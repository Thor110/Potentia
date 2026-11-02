# The rope not searched on a file it cannot win (2 November 2026)

On Edward's two 160 MB maps the rope's search took 768 s of a 1,518 s run, and LZMA2 carried the
file (results/weighing-order-2026-11-02.md). The rope's search has no time limit by default, so on
a large file it cannot win, all of that time is spent for nothing. Texts already had a test on
their start (the rope's line wave on the first 256 KiB); other files had none.

**The test.** The rope's own search on the file's first 256 KiB, against LZMA2's estimate of the
same. Measured here (4 processors) with `sieve rope` on 22 files: pictures as raw RGB pixels (the
repository's PNGs and JPEGs decoded), a synthetic 16-bit stereo sound (two tones and noise, made
for this: no real sound was at hand), four programs, the first 16 MiB of the map and of mozilla's
tar, 4 MiB of random bytes, and the rope's test picture.

| File | Bytes | Rope, whole | LZMA2, whole | Rope / LZMA2 | Search (s) | On its first 256 KiB | Test (s) |
| :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| maps-16MiB.dat | 16,777,216 | 9,216,245 | 3,508,807 | 2.63 | 168.3 | 1.75 | 3.4 |
| mozilla-16MiB.tar | 16,777,216 | 12,685,135 | 5,937,869 | 2.14 | 100.9 | 1.58 | 3.7 |
| pic-Vitruvian-949x1300.rgb | 3,701,100 | 1,627,622 | 2,165,914 | **0.75** | 17.1 | **0.69** | 2.9 |
| pic-audio-800x450.rgb | 1,080,000 | 444,017 | 312,351 | 1.42 | 10.8 | 1.32 | 7.1 |
| pic-book-800x450.rgb | 1,080,000 | 198,740 | 141,776 | 1.40 | 6.1 | 1.35 | 2.3 |
| pic-door-800x450.rgb | 1,080,000 | 299,058 | 260,292 | 1.15 | 9.8 | 1.43 | 5.3 |
| pic-filters-800x450.rgb | 1,080,000 | 152,319 | 100,532 | 1.52 | 6.1 | 1.31 | 2.1 |
| pic-hallway-audio-1280x720.rgb | 2,764,800 | 65,845 | 11,532 | 5.71 | 12.2 | 3.71 | 3.4 |
| pic-hallway-book-random-1280x720.rgb | 2,764,800 | 23,211 | 18,073 | 1.28 | 13.3 | 2.18 | 1.7 |
| pic-hallway-menu-1280x720.rgb | 2,764,800 | 24,822 | 10,842 | 2.29 | 13.1 | 2.29 | 2.0 |
| pic-hallway-menu-sieved-1280x720.rgb | 2,764,800 | 24,753 | 8,600 | 2.88 | 11.1 | 2.32 | 2.0 |
| pic-hallway-text-1280x720.rgb | 2,764,800 | 38,443 | 16,711 | 2.30 | 10.7 | 2.02 | 1.6 |
| pic-m1_sieve_scowl60-1500x900.rgb | 4,050,000 | 142,549 | 57,570 | 2.48 | 15.3 | 1.23 | 2.4 |
| pic-picture-in-hand-800x450.rgb | 1,080,000 | 256,952 | 209,824 | 1.22 | 7.3 | 1.37 | 6.2 |
| pic-vitruvian-560x767.rgb | 1,288,560 | 596,047 | 817,696 | **0.73** | 7.6 | **0.69** | 2.9 |
| prog-bash | 1,446,024 | 1,111,238 | 583,737 | 1.90 | 11.0 | 1.97 | 2.4 |
| prog-git | 4,066,232 | 3,243,512 | 1,682,872 | 1.93 | 28.5 | 1.96 | 2.5 |
| prog-libc.so | 2,129,424 | 1,678,556 | 771,784 | 2.17 | 15.1 | 1.79 | 2.9 |
| prog-python3 | 6,536,120 | 4,822,349 | 2,076,295 | 2.32 | 38.0 | 2.03 | 3.4 |
| random-4MiB.bin | 4,194,304 | 4,195,317 | 4,194,515 | 1.00 | 2.3 | 1.00 | 0.2 |
| synthetic-sound-s16le-stereo.raw | 705,600 | 484,087 | 660,998 | **0.73** | 9.1 | **0.74** | 5.8 |
| vitruvian-centre-256.gray | 65,536 | 44,729 | 47,525 | **0.94** | 2.3 | **0.94** | 2.3 |

Where the rope won whole (0.73 to 0.94 of LZMA2), the test came to 0.69 to 0.94; where it lost
(1.15 to 5.71), 1.23 to 3.71. Random bytes are already left by the search's own stop for bytes
nothing packs (1.00 either way). A 1 MiB test separated them no better (0.72 to 0.94 against 1.14
to 4.67) and took 1.5 to 5 times as long.

**Built.** A file of more than 512 KiB whose first 256 KiB are not text has the rope's search made
on its first 256 KiB first; where that comes to 1.1 times LZMA2's estimate of the same or more,
neither the rope's search nor the pieces are tried (LZMA2 alone is weighed, and mixing after it, as
for a text the rope cannot win). Not with Try every way ticked itself. Log: `rope not searched: its
first 256 KiB the rope's search makes R times LZMA2's`.

**Packed whole, before and after** (the packer's defaults, v32; each pair at once, so under the same
load):

| File | Bytes before | Bytes after | Time before (s) | Time after (s) |
| :--- | ---: | ---: | ---: | ---: |
| maps-16MiB.dat | 3,503,484 | 3,503,484 | 165.9 | 42.2 |
| mozilla-16MiB.tar | 5,937,958 | 5,937,958 | 97.5 | 26.2 |
| pic-door-800x450.rgb | 202,118 | 202,118 | 11.6 | 8.4 |
| pic-hallway-menu-1280x720.rgb | 10,475 | **8,939** | 16.5 | 9.6 |
| pic-m1_sieve_scowl60-1500x900.rgb | 55,611 | **49,221** | 20.6 | 10.5 |
| prog-git | 1,546,897 | 1,546,897 | 8.8 | 8.7 |
| prog-python3 | 1,951,780 | 1,951,780 | 10.5 | 10.4 |
| random-4MiB.bin | 4,194,390 | 4,194,390 | 6.8 | 6.8 |
| pic-Vitruvian-949x1300.rgb | 1,478,558 | 1,478,558 | 24.4 | 26.8 |
| synthetic-sound-s16le-stereo.raw | 484,223 | 484,223 | 15.2 | 20.1 |

The same bytes or fewer for every file. Where the rope loses, 1.4 to 4 times quicker; where it
wins, the test is added to the search (2.4 s and 4.9 s here). Programs and random bytes take no
longer either way (a program is carried in the x86 stream; random bytes stop the search at once).

**Why two pictures came out smaller.** Both are mixed as pictures, at a shape taken from the rope's
best wave where there is one, else guessed from the bytes. The rope's wave read the screenshot as
3,840 one-byte pixels a row (`I3840x1`); with no search, the shape guessed from the bytes was 1,280
pixels of 3 bytes (`I1280x3`), its true one, and mixing made 8,811 bytes of it against 10,347. So
the rope's shape can be worse than the guess where the rope itself loses; where it wins, it is
searched, and its shape used, as before.

**Not yet measured:** real sound (none at hand here); Edward's folders and the 160 MB maps whole.
