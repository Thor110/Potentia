# Making Sieve instructions: time by stage (2026-11-17)

Intel64 Family 6 Model 167 Stepping 1, GenuineIntel, 12 threads, Windows 10. `sieve locate FILE --installer` (the newest instructions), each file alone; seconds, from `--timings`. Stages nest in the total, and the pieces' rope includes its own search; cutting includes the blocks and the boundaries.

## Search 1 s a file

| File | Kind | Bytes | Made | s | MB/s | rope search | pieces' rope | cutting | blocks | boundaries | rope code | LZMA2 quick | LZMA2 trial | LZMA2 pass 1 | LZMA2 tuned | 7-Zip pass | archives | mixing |
| :--- | :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| prose.md | prose | 867,040 | 206,190 | 4.60 | 0.19 |  |  |  |  |  |  | 0.08 | 0.00 | 0.00 | 0.00 | 0.00 |  | 2.63 |
| code.cpp | code | 231,994 | 40,246 | 1.43 | 0.16 |  |  |  |  |  |  | 0.07 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.94 |
| oracle.py | code | 535,980 | 104,557 | 2.79 | 0.19 |  |  |  |  |  |  | 0.07 | 0.00 | 0.00 | 0.00 | 0.00 |  | 1.74 |
| table.csv | table | 17,831 | 7,249 | 2.83 | 0.01 | 0.50 | 0.10 | 0.69 | 0.05 | 0.64 | 0.00 | 0.21 | 0.25 | 0.00 | 0.00 | 0.00 |  | 0.97 |
| words.txt | word list | 2,921,105 | 287,033 | 14.51 | 0.20 | 0.14 |  | 0.07 | 0.06 |  | 0.08 | 0.76 | 0.26 | 0.00 | 0.00 | 0.00 |  | 4.54 |
| rope.txt | prose | 11,440 | 3,293 | 1.20 | 0.01 | 0.03 |  | 0.00 |  |  | 0.00 | 0.00 | 0.26 | 0.00 | 0.00 | 0.00 |  | 0.69 |
| screenshot.png | picture (PNG) | 92,628 | 80,502 | 2.05 | 0.05 | 0.00 | 0.02 | 0.31 | 0.07 | 0.24 | 0.00 | 0.29 | 0.22 | 0.00 | 0.07 | 0.02 |  | 0.87 |
| vitruvian.jpg | picture (JPEG) | 207,500 | 206,946 | 1.06 | 0.20 | 0.01 | 0.01 | 0.02 | 0.01 | 0.01 | 0.00 | 0.05 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.88 |
| door.jpg | picture (JPEG) | 69,298 | 63,856 | 0.57 | 0.12 | 0.00 |  | 0.01 | 0.01 |  | 0.00 | 0.03 | 0.03 | 0.00 | 0.03 | 0.01 |  | 0.34 |
| spitfire.obj | mesh | 469,717 | 53,137 | 2.58 | 0.18 |  |  |  |  |  |  | 0.08 | 0.00 | 0.00 | 0.00 | 0.00 |  | 1.65 |
| hymn.mid | melody (MIDI) | 1,377 | 221 | 0.03 | 0.05 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |  |  |
| film.mp4 | video (MP4) | 234,633 | 233,863 | 1.42 | 0.17 | 0.01 | 0.01 | 0.01 | 0.01 | 0.01 | 0.00 | 0.11 | 0.07 | 0.00 | 0.08 | 0.04 |  | 0.83 |
| sound.wav | sound (WAV) | 88,244 | 3,233 | 2.00 | 0.04 | 1.28 | 0.13 | 0.08 | 0.00 | 0.08 | 0.00 | 0.43 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.14 |
| program.exe | program (Windows) | 5,337,607 | 4,223,640 | 21.41 | 0.25 |  |  |  |  |  |  | 1.58 | 0.00 | 0.00 | 0.00 | 0.00 |  | 10.56 |
| folder.tar | tar | 5,765,120 | 1,311,355 | 16.23 | 0.36 |  |  |  |  |  |  | 0.05 | 0.00 | 0.00 | 0.00 | 0.00 |  | 7.77 |
