# Making Sieve instructions: time by stage (2026-11-16)

Intel64 Family 6 Model 167 Stepping 1, GenuineIntel, 12 threads, Windows 10. `sieve locate FILE --installer` (the newest instructions), each file alone; seconds, from `--timings`. Stages nest in the total, and the pieces' rope includes its own search; cutting includes the blocks and the boundaries.

## Search 1 s a file

| File | Kind | Bytes | Made | s | MB/s | rope search | pieces' rope | cutting | blocks | boundaries | rope code | LZMA2 quick | LZMA2 trial | LZMA2 pass 1 | LZMA2 tuned | 7-Zip pass | archives | mixing |
| :--- | :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| prose.md | prose | 850,067 | 202,187 | 4.08 | 0.21 |  |  |  |  |  |  | 0.07 | 0.00 | 0.00 | 0.00 | 0.00 |  | 2.29 |
| code.cpp | code | 227,969 | 39,647 | 1.46 | 0.16 |  |  |  |  |  |  | 0.05 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.94 |
| oracle.py | code | 535,980 | 104,557 | 3.46 | 0.15 |  |  |  |  |  |  | 0.07 | 0.00 | 0.00 | 0.00 | 0.00 |  | 2.07 |
| table.csv | table | 17,831 | 7,249 | 3.95 | 0.00 | 0.57 | 0.20 | 1.46 | 0.17 | 1.29 | 0.00 | 0.19 | 0.33 | 0.00 | 0.00 | 0.00 |  | 1.18 |
| words.txt | word list | 2,921,105 | 287,033 | 11.18 | 0.26 | 0.18 |  | 0.07 | 0.07 |  | 0.11 | 0.79 | 0.00 | 0.00 | 0.00 | 0.00 |  | 4.39 |
| rope.txt | prose | 11,440 | 3,293 | 2.13 | 0.01 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 0.96 | 0.00 | 0.00 | 0.00 |  | 0.63 |
| screenshot.png | picture (PNG) | 92,628 | 80,502 | 2.75 | 0.03 | 0.00 | 0.02 | 0.85 | 0.21 | 0.64 | 0.00 | 0.36 | 0.38 | 0.00 | 0.06 | 0.02 |  | 0.93 |
| vitruvian.jpg | picture (JPEG) | 207,500 | 206,946 | 1.97 | 0.11 | 0.02 | 0.01 | 0.23 | 0.14 | 0.09 | 0.00 | 0.05 | 0.14 | 0.00 | 0.00 | 0.00 |  | 1.34 |
| door.jpg | picture (JPEG) | 69,298 | 63,856 | 0.73 | 0.10 | 0.00 |  | 0.01 | 0.01 |  | 0.00 | 0.02 | 0.04 | 0.00 | 0.03 | 0.01 |  | 0.44 |
| spitfire.obj | mesh | 469,717 | 53,137 | 3.15 | 0.15 |  |  |  |  |  |  | 0.09 | 0.00 | 0.00 | 0.00 | 0.00 |  | 2.19 |
| hymn.mid | melody (MIDI) | 1,377 | 221 | 0.03 | 0.05 | 0.02 |  | 0.00 |  |  | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |  |  |
| film.mp4 | video (MP4) | 234,633 | 233,863 | 1.53 | 0.15 | 0.01 | 0.01 | 0.02 | 0.01 | 0.01 | 0.00 | 0.14 | 0.09 | 0.00 | 0.11 | 0.04 |  | 0.88 |
| sound.wav | sound (WAV) | 88,244 | 3,233 | 2.15 | 0.04 | 1.43 | 0.13 | 0.10 | 0.00 | 0.09 | 0.00 | 0.49 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.15 |
| program.exe | program (Windows) | 5,337,607 | 4,223,640 | 21.09 | 0.25 |  |  |  |  |  |  | 1.69 | 0.00 | 0.00 | 0.00 | 0.00 |  | 10.42 |
| folder.tar | tar | 5,744,640 | 1,306,738 | 17.05 | 0.34 |  |  |  |  |  |  | 0.06 | 0.00 | 0.00 | 0.00 | 0.00 |  | 7.85 |
