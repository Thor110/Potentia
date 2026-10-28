# Making Sieve instructions: time by stage (2026-10-28)

Intel64 Family 6 Model 167 Stepping 1, GenuineIntel, 12 threads, Windows 10. `sieve locate FILE --installer` (the newest instructions), each file alone; seconds, from `--timings`. Stages nest in the total, and the pieces' rope includes its own search; cutting includes the blocks and the boundaries.

## Search 1 s a file

| File | Kind | Bytes | Made | s | MB/s | rope search | pieces' rope | cutting | blocks | boundaries | rope code | LZMA2 quick | LZMA2 trial | LZMA2 pass 1 | LZMA2 tuned | 7-Zip pass | archives | mixing |
| :--- | :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| prose.md | prose | 404,120 | 102,875 | 4.18 | 0.10 | 0.07 |  | 0.02 | 0.02 |  | 0.06 | 0.52 | 0.17 | 0.00 | 0.00 | 0.00 |  | 2.23 |
| code.cpp | code | 165,882 | 29,630 | 2.38 | 0.07 | 0.10 |  | 0.26 | 0.26 |  | 0.10 | 0.15 | 0.00 | 0.00 | 0.00 | 0.00 |  | 1.38 |
| oracle.py | code | 447,532 | 89,567 | 3.95 | 0.11 | 0.15 |  | 0.02 | 0.02 |  | 0.13 | 0.50 | 0.00 | 0.00 | 0.00 | 0.00 |  | 2.38 |
| table.csv | table | 17,831 | 7,262 | 0.80 | 0.02 | 0.50 | 0.05 | 0.02 | 0.00 | 0.02 | 0.00 | 0.13 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.18 |
| words.txt | word list | 2,921,105 | 310,401 | 13.16 | 0.22 | 0.22 |  | 0.10 | 0.10 |  | 0.19 | 2.28 | 0.00 | 0.00 | 0.00 | 0.00 |  | 6.27 |
| rope.txt | prose | 11,440 | 3,633 | 0.17 | 0.07 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.13 |
| screenshot.png | picture (PNG) | 92,628 | 80,506 | 0.71 | 0.13 | 0.00 | 0.01 | 0.04 | 0.00 | 0.03 | 0.00 | 0.32 | 0.03 | 0.02 | 0.04 | 0.02 |  | 0.37 |
| vitruvian.jpg | picture (JPEG) | 207,500 | 206,956 | 0.88 | 0.24 | 0.01 | 0.02 | 0.02 | 0.01 | 0.01 | 0.01 | 0.16 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.70 |
| door.jpg | picture (JPEG) | 69,298 | 63,860 | 0.46 | 0.15 | 0.00 |  | 0.00 | 0.00 |  | 0.00 | 0.05 | 0.02 | 0.02 | 0.02 | 0.01 |  | 0.27 |
| spitfire.obj | mesh | 469,717 | 53,519 | 2.79 | 0.17 | 0.06 |  | 0.02 | 0.02 |  | 0.05 | 0.46 | 0.00 | 0.00 | 0.00 | 0.00 |  | 1.68 |
| hymn.mid | melody (MIDI) | 1,377 | 230 | 0.02 | 0.06 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |  |  |
| film.mp4 | video (MP4) | 234,633 | 233,867 | 1.51 | 0.15 | 0.02 | 0.02 | 0.02 | 0.01 | 0.01 | 0.01 | 0.23 | 0.10 | 0.07 | 0.10 | 0.05 |  | 0.81 |
| sound.wav | sound (WAV) | 88,244 | 3,257 | 2.42 | 0.04 | 1.88 | 0.10 | 0.09 | 0.01 | 0.08 | 0.01 | 0.60 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.23 |
| program.exe | program (Windows) | 5,337,607 | 4,275,365 | 3.51 | 1.52 |  |  |  |  |  |  |  | 0.38 | 1.51 | 3.01 | 1.24 |  |  |
| folder.tar | tar | 5,150,720 | 1,210,475 | 23.45 | 0.22 | 1.06 |  | 0.19 | 0.18 |  | 0.17 | 3.62 | 0.17 | 0.00 | 0.00 | 0.00 |  | 10.83 |
