# Making Sieve instructions: time by stage (2026-10-25)

Intel64 Family 6 Model 167 Stepping 1, GenuineIntel, 12 threads, Windows 10. `sieve locate FILE --installer` (the newest instructions), each file alone; seconds, from `--timings`. Stages nest in the total, and the pieces' rope includes its own search; cutting includes the blocks and the boundaries.

## Search 1 s a file

| File | Kind | Bytes | Made | s | MB/s | rope search | pieces' rope | cutting | blocks | boundaries | rope code | LZMA2 quick | LZMA2 trial | LZMA2 pass 1 | LZMA2 tuned | 7-Zip pass | archives |
| :--- | :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| prose.md | prose | 369,360 | 119,624 | 0.55 | 0.67 | 0.07 |  | 0.01 | 0.01 |  | 0.07 | 0.25 | 0.15 | 0.14 | 0.22 | 0.10 |  |
| code.cpp | code | 162,697 | 34,744 | 0.28 | 0.57 | 0.06 |  | 0.01 | 0.01 |  | 0.06 | 0.10 | 0.05 | 0.06 | 0.08 | 0.04 |  |
| oracle.py | code | 410,455 | 97,231 | 0.58 | 0.71 | 0.10 |  | 0.02 | 0.02 |  | 0.08 | 0.26 | 0.15 | 0.15 | 0.21 | 0.10 |  |
| table.csv | table | 17,831 | 7,733 | 0.50 | 0.04 | 0.42 | 0.04 | 0.02 | 0.00 | 0.02 | 0.00 | 0.12 | 0.01 | 0.00 | 0.01 | 0.00 |  |
| words.txt | word list | 2,921,105 | 564,454 | 1.59 | 1.84 | 0.21 |  | 0.11 | 0.11 |  | 0.18 | 2.10 | 0.00 | 0.00 | 0.00 | 0.00 |  |
| rope.txt | prose | 11,440 | 4,602 | 0.03 | 0.44 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 0.01 | 0.00 | 0.01 | 0.00 |  |
| screenshot.png | picture (PNG) | 92,628 | 80,506 | 0.18 | 0.51 | 0.00 | 0.01 | 0.04 | 0.01 | 0.03 | 0.00 | 0.32 | 0.03 | 0.02 | 0.03 | 0.01 |  |
| vitruvian.jpg | picture (JPEG) | 207,500 | 206,956 | 0.17 | 1.21 | 0.01 | 0.02 | 0.02 | 0.01 | 0.01 | 0.01 | 0.18 | 0.00 | 0.00 | 0.00 | 0.00 |  |
| door.jpg | picture (JPEG) | 69,298 | 63,860 | 0.08 | 0.88 | 0.01 |  | 0.01 | 0.00 |  | 0.00 | 0.04 | 0.02 | 0.01 | 0.02 | 0.01 |  |
| spitfire.obj | mesh | 469,717 | 69,047 | 0.60 | 0.78 | 0.06 |  | 0.02 | 0.02 |  | 0.06 | 0.31 | 0.19 | 0.19 | 0.26 | 0.12 |  |
| hymn.mid | melody (MIDI) | 1,377 | 230 | 0.02 | 0.08 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |  |
| film.mp4 | video (MP4) | 234,633 | 233,867 | 0.22 | 1.06 | 0.01 | 0.02 | 0.02 | 0.01 | 0.01 | 0.01 | 0.16 | 0.07 | 0.06 | 0.10 | 0.03 |  |
| sound.wav | sound (WAV) | 88,244 | 3,736 | 1.68 | 0.05 | 1.48 | 0.08 | 0.09 | 0.00 | 0.08 | 0.00 | 0.59 | 0.01 | 0.01 | 0.01 | 0.01 |  |
| program.exe | program (Windows) | 5,337,607 | 4,275,365 | 3.22 | 1.66 |  |  |  |  |  |  |  | 0.38 | 1.42 | 2.78 | 1.17 |  |
| folder.tar | tar | 5,068,800 | 1,549,613 | 6.33 | 0.80 | 1.02 |  | 0.17 | 0.16 |  | 0.16 | 3.20 | 0.43 | 1.63 | 3.39 | 1.51 |  |
