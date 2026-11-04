# Making Sieve instructions: time by stage (2026-11-04)

Intel64 Family 6 Model 167 Stepping 1, GenuineIntel, 12 threads, Windows 10. `sieve locate FILE --installer` (the newest instructions), each file alone; seconds, from `--timings`. Stages nest in the total, and the pieces' rope includes its own search; cutting includes the blocks and the boundaries.

## Search 1 s a file

| File | Kind | Bytes | Made | s | MB/s | rope search | pieces' rope | cutting | blocks | boundaries | rope code | LZMA2 quick | LZMA2 trial | LZMA2 pass 1 | LZMA2 tuned | 7-Zip pass | archives | mixing |
| :--- | :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| prose.md | prose | 488,140 | 121,826 | 4.18 | 0.12 |  |  |  |  |  |  | 0.07 | 0.00 | 0.00 | 0.00 | 0.00 |  | 3.29 |
| code.cpp | code | 171,605 | 30,381 | 2.27 | 0.08 |  |  |  |  |  |  | 0.04 | 0.00 | 0.00 | 0.00 | 0.00 |  | 1.92 |
| oracle.py | code | 487,013 | 96,232 | 4.01 | 0.12 |  |  |  |  |  |  | 0.07 | 0.00 | 0.00 | 0.00 | 0.00 |  | 3.12 |
| table.csv | table | 17,831 | 7,249 | 1.11 | 0.02 | 0.51 | 0.04 | 0.02 | 0.00 | 0.01 | 0.00 | 0.13 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.50 |
| words.txt | word list | 2,921,105 | 287,033 | 10.69 | 0.27 | 0.16 |  | 0.10 | 0.09 |  | 0.11 | 1.11 | 0.00 | 0.00 | 0.00 | 0.00 |  | 6.13 |
| rope.txt | prose | 11,440 | 3,293 | 0.47 | 0.02 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.43 |
| screenshot.png | picture (PNG) | 92,628 | 80,502 | 1.20 | 0.08 | 0.00 | 0.01 | 0.04 | 0.00 | 0.03 | 0.00 | 0.31 | 0.02 | 0.00 | 0.03 | 0.01 |  | 0.91 |
| vitruvian.jpg | picture (JPEG) | 207,500 | 206,946 | 2.01 | 0.10 | 0.01 | 0.01 | 0.01 | 0.01 | 0.01 | 0.00 | 0.12 | 0.00 | 0.00 | 0.00 | 0.00 |  | 1.89 |
| door.jpg | picture (JPEG) | 69,298 | 63,856 | 0.90 | 0.08 | 0.00 |  | 0.00 | 0.00 |  | 0.00 | 0.04 | 0.02 | 0.00 | 0.02 | 0.01 |  | 0.74 |
| spitfire.obj | mesh | 469,717 | 53,144 | 3.66 | 0.13 |  |  |  |  |  |  | 0.06 | 0.00 | 0.00 | 0.00 | 0.00 |  | 2.90 |
| hymn.mid | melody (MIDI) | 1,377 | 221 | 0.02 | 0.07 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |  |  |
| film.mp4 | video (MP4) | 234,633 | 233,863 | 2.68 | 0.09 | 0.01 | 0.01 | 0.02 | 0.01 | 0.01 | 0.00 | 0.17 | 0.06 | 0.00 | 0.07 | 0.03 |  | 2.21 |
| sound.wav | sound (WAV) | 88,244 | 3,263 | 2.29 | 0.04 | 1.53 | 0.12 | 0.09 | 0.00 | 0.09 | 0.00 | 0.66 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.43 |
| program.exe | program (Windows) | 5,337,607 | 4,255,565 | 36.33 | 0.15 |  |  |  |  |  |  | 1.38 | 0.00 | 0.00 | 0.00 | 0.00 |  | 27.02 |
| folder.tar | tar | 5,273,600 | 1,205,815 | 19.20 | 0.27 |  |  |  |  |  |  | 0.08 | 0.00 | 0.00 | 0.00 | 0.00 |  | 10.95 |
