# Making Sieve instructions: time by stage (2026-11-13)

Intel64 Family 6 Model 167 Stepping 1, GenuineIntel, 12 threads, Windows 10. `sieve locate FILE --installer` (the newest instructions), each file alone; seconds, from `--timings`. Stages nest in the total, and the pieces' rope includes its own search; cutting includes the blocks and the boundaries.

## Search 1 s a file

| File | Kind | Bytes | Made | s | MB/s | rope search | pieces' rope | cutting | blocks | boundaries | rope code | LZMA2 quick | LZMA2 trial | LZMA2 pass 1 | LZMA2 tuned | 7-Zip pass | archives | mixing |
| :--- | :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| prose.md | prose | 679,110 | 163,735 | 3.15 | 0.22 |  |  |  |  |  |  | 0.07 | 0.00 | 0.00 | 0.00 | 0.00 |  | 1.90 |
| code.cpp | code | 210,617 | 36,749 | 1.29 | 0.16 |  |  |  |  |  |  | 0.06 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.83 |
| oracle.py | code | 535,980 | 104,557 | 2.99 | 0.18 |  |  |  |  |  |  | 0.07 | 0.00 | 0.00 | 0.00 | 0.00 |  | 1.92 |
| table.csv | table | 17,831 | 7,249 | 2.46 | 0.01 | 0.83 | 0.08 | 0.38 | 0.11 | 0.26 | 0.00 | 0.12 | 0.24 | 0.00 | 0.00 | 0.00 |  | 0.73 |
| words.txt | word list | 2,921,105 | 287,033 | 14.46 | 0.20 | 0.20 |  | 0.13 | 0.13 |  | 0.12 | 1.45 | 0.00 | 0.00 | 0.00 | 0.00 |  | 4.39 |
| rope.txt | prose | 11,440 | 3,293 | 1.34 | 0.01 | 0.02 |  | 0.00 |  |  | 0.00 | 0.00 | 0.31 | 0.00 | 0.00 | 0.00 |  | 0.77 |
| screenshot.png | picture (PNG) | 92,628 | 80,502 | 2.37 | 0.04 | 0.00 | 0.02 | 0.58 | 0.15 | 0.43 | 0.00 | 0.37 | 0.22 | 0.00 | 0.07 | 0.02 |  | 0.91 |
| vitruvian.jpg | picture (JPEG) | 207,500 | 206,946 | 1.76 | 0.12 | 0.01 | 0.01 | 0.33 | 0.11 | 0.22 | 0.00 | 0.10 | 0.01 | 0.00 | 0.00 | 0.00 |  | 1.23 |
| door.jpg | picture (JPEG) | 69,298 | 63,856 | 0.61 | 0.11 | 0.00 |  | 0.01 | 0.01 |  | 0.00 | 0.04 | 0.03 | 0.00 | 0.04 | 0.01 |  | 0.36 |
| spitfire.obj | mesh | 469,717 | 53,137 | 2.68 | 0.18 |  |  |  |  |  |  | 0.08 | 0.00 | 0.00 | 0.00 | 0.00 |  | 1.76 |
| hymn.mid | melody (MIDI) | 1,377 | 221 | 0.03 | 0.05 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 0.01 | 0.00 | 0.00 | 0.00 |  |  |
| film.mp4 | video (MP4) | 234,633 | 233,863 | 1.62 | 0.14 | 0.01 | 0.01 | 0.02 | 0.01 | 0.01 | 0.00 | 0.19 | 0.08 | 0.00 | 0.13 | 0.03 |  | 0.93 |
| sound.wav | sound (WAV) | 88,244 | 3,233 | 2.11 | 0.04 | 1.39 | 0.11 | 0.10 | 0.01 | 0.09 | 0.00 | 0.64 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.16 |
| program.exe | program (Windows) | 5,337,607 | 4,223,640 | 23.26 | 0.23 |  |  |  |  |  |  | 1.81 | 0.00 | 0.00 | 0.00 | 0.00 |  | 11.97 |
| folder.tar | tar | 5,560,320 | 1,265,007 | 17.71 | 0.31 |  |  |  |  |  |  | 0.07 | 0.00 | 0.00 | 0.00 | 0.00 |  | 8.55 |
