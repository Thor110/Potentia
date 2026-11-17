# Making Sieve instructions: time by stage (2026-11-17)

Intel64 Family 6 Model 167 Stepping 1, GenuineIntel, 12 threads, Windows 10. `sieve locate FILE --installer` (the newest instructions), each file alone; seconds, from `--timings`. Stages nest in the total, and the pieces' rope includes its own search; cutting includes the blocks and the boundaries.

## Search 1 s a file

| File | Kind | Bytes | Made | s | MB/s | rope search | pieces' rope | cutting | blocks | boundaries | rope code | LZMA2 quick | LZMA2 trial | LZMA2 pass 1 | LZMA2 tuned | 7-Zip pass | archives | mixing |
| :--- | :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| prose.md | prose | 876,180 | 208,403 | 3.92 | 0.22 |  |  |  |  |  |  | 0.07 | 0.00 | 0.00 | 0.00 | 0.00 |  | 2.23 |
| code.cpp | code | 231,994 | 40,246 | 1.42 | 0.16 |  |  |  |  |  |  | 0.05 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.93 |
| oracle.py | code | 535,980 | 104,557 | 5.09 | 0.11 |  |  |  |  |  |  | 0.06 | 0.72 | 0.00 | 0.00 | 0.00 |  | 2.05 |
| table.csv | table | 17,831 | 7,249 | 0.83 | 0.02 | 0.42 | 0.05 | 0.02 | 0.00 | 0.02 | 0.00 | 0.11 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.32 |
| words.txt | word list | 2,921,105 | 287,033 | 14.99 | 0.19 | 0.14 |  | 0.06 | 0.06 |  | 0.09 | 0.67 | 0.19 | 0.00 | 0.00 | 0.00 |  | 5.68 |
| rope.txt | prose | 11,440 | 3,293 | 1.56 | 0.01 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 0.30 | 0.00 | 0.00 | 0.00 |  | 1.02 |
| screenshot.png | picture (PNG) | 92,628 | 80,502 | 1.38 | 0.07 | 0.00 | 0.02 | 0.22 | 0.03 | 0.19 | 0.01 | 0.32 | 0.09 | 0.00 | 0.05 | 0.02 |  | 0.65 |
| vitruvian.jpg | picture (JPEG) | 207,500 | 206,946 | 1.05 | 0.20 | 0.01 | 0.01 | 0.01 | 0.01 | 0.01 | 0.00 | 0.06 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.90 |
| door.jpg | picture (JPEG) | 69,298 | 63,856 | 0.58 | 0.12 | 0.00 |  | 0.00 | 0.00 |  | 0.00 | 0.03 | 0.02 | 0.00 | 0.03 | 0.01 |  | 0.36 |
| spitfire.obj | mesh | 469,717 | 53,137 | 2.52 | 0.19 |  |  |  |  |  |  | 0.07 | 0.00 | 0.00 | 0.00 | 0.00 |  | 1.66 |
| hymn.mid | melody (MIDI) | 1,377 | 221 | 0.02 | 0.06 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |  |  |
| film.mp4 | video (MP4) | 234,633 | 233,863 | 1.37 | 0.17 | 0.01 | 0.01 | 0.01 | 0.01 | 0.01 | 0.00 | 0.11 | 0.07 | 0.00 | 0.08 | 0.02 |  | 0.82 |
| sound.wav | sound (WAV) | 88,244 | 3,233 | 2.00 | 0.04 | 1.32 | 0.11 | 0.08 | 0.00 | 0.08 | 0.00 | 0.44 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.14 |
| program.exe | program (Windows) | 5,337,607 | 4,223,640 | 23.62 | 0.23 |  |  |  |  |  |  | 1.65 | 0.00 | 0.00 | 0.00 | 0.00 |  | 11.72 |
| folder.tar | tar | 5,775,360 | 1,313,560 | 16.47 | 0.35 |  |  |  |  |  |  | 0.06 | 0.00 | 0.00 | 0.00 | 0.00 |  | 7.78 |
