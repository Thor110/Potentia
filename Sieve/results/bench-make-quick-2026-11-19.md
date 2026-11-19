# Making Sieve instructions: time by stage (2026-11-19)

Intel64 Family 6 Model 167 Stepping 1, GenuineIntel, 12 threads, Windows 10. `sieve locate FILE --installer` (the newest instructions), each file alone; seconds, from `--timings`. Stages nest in the total, and the pieces' rope includes its own search; cutting includes the blocks and the boundaries.

## Search 1 s a file

| File | Kind | Bytes | Made | s | MB/s | rope search | pieces' rope | cutting | blocks | boundaries | rope code | LZMA2 quick | LZMA2 trial | LZMA2 pass 1 | LZMA2 tuned | 7-Zip pass | archives | mixing |
| :--- | :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| prose.md | prose | 905,343 | 215,339 | 4.17 | 0.22 |  |  |  |  |  |  | 0.09 | 0.00 | 0.00 | 0.00 | 0.00 |  | 2.25 |
| code.cpp | code | 239,807 | 41,519 | 1.40 | 0.17 |  |  |  |  |  |  | 0.05 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.92 |
| oracle.py | code | 535,980 | 104,557 | 3.29 | 0.16 |  |  |  |  |  |  | 0.08 | 0.00 | 0.00 | 0.00 | 0.00 |  | 1.91 |
| table.csv | table | 17,831 | 7,249 | 1.38 | 0.01 | 0.86 | 0.06 | 0.04 | 0.01 | 0.03 | 0.00 | 0.11 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.38 |
| words.txt | word list | 2,921,105 | 287,033 | 15.35 | 0.19 | 0.15 |  | 0.06 | 0.05 |  | 0.09 | 0.67 | 0.17 | 0.00 | 0.00 | 0.00 |  | 5.95 |
| rope.txt | prose | 11,440 | 3,293 | 1.66 | 0.01 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 0.29 | 0.00 | 0.00 | 0.00 |  | 0.97 |
| screenshot.png | picture (PNG) | 92,628 | 80,502 | 2.76 | 0.03 | 0.01 | 0.02 | 0.65 | 0.13 | 0.52 | 0.00 | 0.46 | 0.29 | 0.00 | 0.07 | 0.02 |  | 1.12 |
| vitruvian.jpg | picture (JPEG) | 207,500 | 206,946 | 1.19 | 0.17 | 0.01 | 0.01 | 0.10 | 0.07 | 0.03 | 0.00 | 0.05 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.94 |
| door.jpg | picture (JPEG) | 69,298 | 63,856 | 0.63 | 0.11 | 0.00 |  | 0.00 | 0.00 |  | 0.00 | 0.03 | 0.02 | 0.00 | 0.03 | 0.01 |  | 0.39 |
| spitfire.obj | mesh | 469,717 | 53,137 | 2.71 | 0.17 |  |  |  |  |  |  | 0.09 | 0.00 | 0.00 | 0.00 | 0.00 |  | 1.73 |
| hymn.mid | melody (MIDI) | 1,377 | 221 | 0.03 | 0.05 | 0.02 |  | 0.00 |  |  | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |  |  |
| film.mp4 | video (MP4) | 234,633 | 233,863 | 1.54 | 0.15 | 0.01 | 0.01 | 0.01 | 0.01 | 0.01 | 0.00 | 0.13 | 0.08 | 0.00 | 0.12 | 0.03 |  | 0.86 |
| sound.wav | sound (WAV) | 88,244 | 3,233 | 2.17 | 0.04 | 1.49 | 0.11 | 0.08 | 0.00 | 0.08 | 0.00 | 0.47 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.15 |
| program.exe | program (Windows) | 5,337,607 | 4,223,640 | 23.64 | 0.23 |  |  |  |  |  |  | 1.68 | 0.00 | 0.00 | 0.00 | 0.00 |  | 10.22 |
| folder.tar | tar | 5,806,080 | 1,321,726 | 18.29 | 0.32 |  |  |  |  |  |  | 0.06 | 0.00 | 0.00 | 0.00 | 0.00 |  | 9.03 |
