# Making Sieve instructions: time by stage (2026-11-16)

Intel64 Family 6 Model 167 Stepping 1, GenuineIntel, 12 threads, Windows 10. `sieve locate FILE --installer` (the newest instructions), each file alone; seconds, from `--timings`. Stages nest in the total, and the pieces' rope includes its own search; cutting includes the blocks and the boundaries.

## Search 1 s a file

| File | Kind | Bytes | Made | s | MB/s | rope search | pieces' rope | cutting | blocks | boundaries | rope code | LZMA2 quick | LZMA2 trial | LZMA2 pass 1 | LZMA2 tuned | 7-Zip pass | archives | mixing |
| :--- | :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| prose.md | prose | 850,067 | 202,187 | 4.21 | 0.20 |  |  |  |  |  |  | 0.07 | 0.00 | 0.00 | 0.00 | 0.00 |  | 2.48 |
| code.cpp | code | 227,969 | 39,647 | 1.43 | 0.16 |  |  |  |  |  |  | 0.06 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.92 |
| oracle.py | code | 535,980 | 104,557 | 2.86 | 0.19 |  |  |  |  |  |  | 0.07 | 0.00 | 0.00 | 0.00 | 0.00 |  | 1.71 |
| table.csv | table | 17,831 | 7,249 | 0.86 | 0.02 | 0.44 | 0.04 | 0.02 | 0.00 | 0.01 | 0.00 | 0.11 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.33 |
| words.txt | word list | 2,921,105 | 287,033 | 11.84 | 0.25 | 0.39 |  | 0.63 | 0.63 |  | 0.19 | 1.09 | 0.00 | 0.00 | 0.00 | 0.00 |  | 4.33 |
| rope.txt | prose | 11,440 | 3,293 | 1.34 | 0.01 | 0.02 |  | 0.00 |  |  | 0.00 | 0.00 | 0.30 | 0.00 | 0.00 | 0.00 |  | 0.67 |
| screenshot.png | picture (PNG) | 92,628 | 80,502 | 3.55 | 0.03 | 0.00 | 0.02 | 1.07 | 0.11 | 0.96 | 0.00 | 0.36 | 0.50 | 0.00 | 0.07 | 0.01 |  | 1.22 |
| vitruvian.jpg | picture (JPEG) | 207,500 | 206,946 | 2.42 | 0.09 | 0.03 | 0.01 | 0.63 | 0.33 | 0.30 | 0.00 | 0.06 | 0.13 | 0.00 | 0.00 | 0.00 |  | 1.37 |
| door.jpg | picture (JPEG) | 69,298 | 63,856 | 1.42 | 0.05 | 0.00 |  | 0.09 | 0.09 |  | 0.00 | 0.02 | 0.28 | 0.00 | 0.03 | 0.01 |  | 0.63 |
| spitfire.obj | mesh | 469,717 | 53,137 | 3.04 | 0.15 |  |  |  |  |  |  | 0.10 | 0.00 | 0.00 | 0.00 | 0.00 |  | 2.06 |
| hymn.mid | melody (MIDI) | 1,377 | 221 | 0.03 | 0.05 | 0.02 |  | 0.00 |  |  | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |  |  |
| film.mp4 | video (MP4) | 234,633 | 233,863 | 1.62 | 0.14 | 0.01 | 0.01 | 0.02 | 0.01 | 0.01 | 0.00 | 0.16 | 0.09 | 0.00 | 0.14 | 0.05 |  | 0.92 |
| sound.wav | sound (WAV) | 88,244 | 3,233 | 2.31 | 0.04 | 1.47 | 0.14 | 0.12 | 0.00 | 0.11 | 0.00 | 0.47 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.18 |
| program.exe | program (Windows) | 5,337,607 | 4,223,640 | 23.50 | 0.23 |  |  |  |  |  |  | 1.85 | 0.00 | 0.00 | 0.00 | 0.00 |  | 11.74 |
| folder.tar | tar | 5,744,640 | 1,306,738 | 17.51 | 0.33 |  |  |  |  |  |  | 0.10 | 0.00 | 0.00 | 0.00 | 0.00 |  | 7.96 |
