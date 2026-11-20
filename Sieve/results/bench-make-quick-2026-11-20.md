# Making Sieve instructions: time by stage (2026-11-20)

Intel64 Family 6 Model 167 Stepping 1, GenuineIntel, 12 threads, Windows 10. `sieve locate FILE --installer` (the newest instructions), each file alone; seconds, from `--timings`. Stages nest in the total, and the pieces' rope includes its own search; cutting includes the blocks and the boundaries.

## Search 1 s a file

| File | Kind | Bytes | Made | s | MB/s | rope search | pieces' rope | cutting | blocks | boundaries | rope code | LZMA2 quick | LZMA2 trial | LZMA2 pass 1 | LZMA2 tuned | 7-Zip pass | archives | mixing |
| :--- | :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| prose.md | prose | 914,177 | 217,547 | 3.99 | 0.23 |  |  |  |  |  |  | 0.06 | 0.00 | 0.00 | 0.00 | 0.00 |  | 2.21 |
| code.cpp | code | 241,318 | 41,889 | 1.45 | 0.17 |  |  |  |  |  |  | 0.05 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.91 |
| oracle.py | code | 544,283 | 105,655 | 3.07 | 0.18 |  |  |  |  |  |  | 0.05 | 0.00 | 0.00 | 0.00 | 0.00 |  | 2.05 |
| table.csv | table | 17,831 | 7,249 | 3.12 | 0.01 | 1.07 | 0.07 | 1.17 | 0.16 | 1.00 | 0.00 | 0.12 | 0.11 | 0.00 | 0.00 | 0.00 |  | 0.65 |
| words.txt | word list | 2,921,105 | 287,033 | 11.60 | 0.25 | 0.14 |  | 0.07 | 0.06 |  | 0.10 | 0.80 | 0.00 | 0.00 | 0.00 | 0.00 |  | 3.96 |
| rope.txt | prose | 11,440 | 3,293 | 1.64 | 0.01 | 0.04 |  | 0.00 |  |  | 0.00 | 0.00 | 0.39 | 0.00 | 0.00 | 0.00 |  | 0.91 |
| screenshot.png | picture (PNG) | 92,628 | 80,502 | 3.03 | 0.03 | 0.04 | 0.02 | 0.65 | 0.12 | 0.53 | 0.00 | 0.41 | 0.59 | 0.00 | 0.06 | 0.02 |  | 0.97 |
| vitruvian.jpg | picture (JPEG) | 207,500 | 206,946 | 2.52 | 0.08 | 0.01 | 0.01 | 0.42 | 0.19 | 0.23 | 0.03 | 0.06 | 0.22 | 0.00 | 0.00 | 0.00 |  | 1.58 |
| door.jpg | picture (JPEG) | 69,298 | 63,856 | 0.66 | 0.10 | 0.00 |  | 0.01 | 0.01 |  | 0.00 | 0.03 | 0.03 | 0.00 | 0.04 | 0.01 |  | 0.39 |
| spitfire.obj | mesh | 469,717 | 53,137 | 2.74 | 0.17 |  |  |  |  |  |  | 0.09 | 0.00 | 0.00 | 0.00 | 0.00 |  | 1.73 |
| hymn.mid | melody (MIDI) | 1,377 | 221 | 0.03 | 0.05 | 0.02 |  | 0.00 |  |  | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |  |  |
| film.mp4 | video (MP4) | 234,633 | 233,863 | 1.56 | 0.15 | 0.01 | 0.01 | 0.02 | 0.01 | 0.01 | 0.00 | 0.12 | 0.08 | 0.00 | 0.13 | 0.05 |  | 0.92 |
| sound.wav | sound (WAV) | 88,244 | 3,233 | 2.19 | 0.04 | 1.46 | 0.11 | 0.09 | 0.00 | 0.08 | 0.00 | 0.46 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.15 |
| program.exe | program (Windows) | 5,337,607 | 4,223,640 | 23.72 | 0.23 |  |  |  |  |  |  | 2.07 | 0.00 | 0.00 | 0.00 | 0.00 |  | 12.20 |
| folder.tar | tar | 5,826,560 | 1,325,368 | 18.76 | 0.31 |  |  |  |  |  |  | 0.07 | 0.00 | 0.00 | 0.00 | 0.00 |  | 8.79 |
