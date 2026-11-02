# Making Sieve instructions: time by stage (2026-11-02)

Intel64 Family 6 Model 167 Stepping 1, GenuineIntel, 12 threads, Windows 10. `sieve locate FILE --installer` (the newest instructions), each file alone; seconds, from `--timings`. Stages nest in the total, and the pieces' rope includes its own search; cutting includes the blocks and the boundaries.

## Search 1 s a file

| File | Kind | Bytes | Made | s | MB/s | rope search | pieces' rope | cutting | blocks | boundaries | rope code | LZMA2 quick | LZMA2 trial | LZMA2 pass 1 | LZMA2 tuned | 7-Zip pass | archives | mixing |
| :--- | :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| prose.md | prose | 462,268 | 115,361 | 3.87 | 0.12 | 0.07 |  | 0.02 | 0.02 |  | 0.07 | 0.56 | 0.03 | 0.00 | 0.00 | 0.00 |  | 2.67 |
| code.cpp | code | 169,689 | 29,952 | 1.70 | 0.10 | 0.09 |  | 0.01 | 0.01 |  | 0.09 | 0.18 | 0.00 | 0.00 | 0.00 | 0.00 |  | 1.15 |
| oracle.py | code | 471,849 | 93,529 | 2.71 | 0.17 | 0.14 |  | 0.02 | 0.02 |  | 0.12 | 0.53 | 0.00 | 0.00 | 0.00 | 0.00 |  | 1.70 |
| table.csv | table | 17,831 | 7,249 | 1.01 | 0.02 | 0.56 | 0.05 | 0.02 | 0.00 | 0.02 | 0.00 | 0.13 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.34 |
| words.txt | word list | 2,921,105 | 280,994 | 9.23 | 0.32 | 0.18 |  | 0.11 | 0.11 |  | 0.12 | 2.50 | 0.00 | 0.00 | 0.00 | 0.00 |  | 4.50 |
| rope.txt | prose | 11,440 | 3,293 | 0.34 | 0.03 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.31 |
| screenshot.png | picture (PNG) | 92,628 | 80,502 | 0.61 | 0.15 | 0.00 | 0.01 | 0.04 | 0.00 | 0.04 | 0.00 | 0.35 | 0.03 | 0.02 | 0.03 | 0.01 |  | 0.32 |
| vitruvian.jpg | picture (JPEG) | 207,500 | 206,946 | 0.68 | 0.30 | 0.01 | 0.01 | 0.02 | 0.01 | 0.01 | 0.00 | 0.16 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.56 |
| door.jpg | picture (JPEG) | 69,298 | 63,856 | 0.37 | 0.19 | 0.00 |  | 0.00 | 0.00 |  | 0.00 | 0.05 | 0.02 | 0.02 | 0.02 | 0.01 |  | 0.21 |
| spitfire.obj | mesh | 469,717 | 53,524 | 2.08 | 0.23 | 0.06 |  | 0.02 | 0.02 |  | 0.04 | 0.51 | 0.00 | 0.00 | 0.00 | 0.00 |  | 1.33 |
| hymn.mid | melody (MIDI) | 1,377 | 221 | 0.02 | 0.07 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |  |  |
| film.mp4 | video (MP4) | 234,633 | 233,863 | 1.09 | 0.22 | 0.01 | 0.01 | 0.02 | 0.01 | 0.01 | 0.00 | 0.20 | 0.06 | 0.06 | 0.06 | 0.02 |  | 0.60 |
| sound.wav | sound (WAV) | 88,244 | 3,255 | 2.05 | 0.04 | 1.58 | 0.12 | 0.09 | 0.00 | 0.09 | 0.00 | 0.59 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.16 |
| program.exe | program (Windows) | 5,337,607 | 4,275,360 | 3.53 | 1.51 |  |  |  |  |  |  |  | 0.39 | 1.63 | 3.10 | 1.30 |  |  |
| folder.tar | tar | 5,232,640 | 1,196,462 | 16.19 | 0.32 | 1.02 |  | 0.19 | 0.18 |  | 0.07 | 4.01 | 0.00 | 0.00 | 0.00 | 0.00 |  | 7.46 |
