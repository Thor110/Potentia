# Making Sieve instructions: time by stage (2026-11-16)

Intel64 Family 6 Model 167 Stepping 1, GenuineIntel, 12 threads, Windows 10. `sieve locate FILE --installer` (the newest instructions), each file alone; seconds, from `--timings`. Stages nest in the total, and the pieces' rope includes its own search; cutting includes the blocks and the boundaries.

## Search 1 s a file

| File | Kind | Bytes | Made | s | MB/s | rope search | pieces' rope | cutting | blocks | boundaries | rope code | LZMA2 quick | LZMA2 trial | LZMA2 pass 1 | LZMA2 tuned | 7-Zip pass | archives | mixing |
| :--- | :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| prose.md | prose | 850,067 | 202,187 | 3.93 | 0.22 |  |  |  |  |  |  | 0.06 | 0.00 | 0.00 | 0.00 | 0.00 |  | 2.31 |
| code.cpp | code | 227,969 | 39,647 | 1.53 | 0.15 |  |  |  |  |  |  | 0.05 | 0.00 | 0.00 | 0.00 | 0.00 |  | 1.01 |
| oracle.py | code | 535,980 | 104,557 | 2.97 | 0.18 |  |  |  |  |  |  | 0.06 | 0.00 | 0.00 | 0.00 | 0.00 |  | 1.80 |
| table.csv | table | 17,831 | 7,249 | 2.03 | 0.01 | 0.47 | 0.05 | 0.02 | 0.00 | 0.01 | 0.00 | 0.12 | 0.61 | 0.00 | 0.00 | 0.00 |  | 0.42 |
| words.txt | word list | 2,921,105 | 287,033 | 12.14 | 0.24 | 0.21 |  | 0.10 | 0.10 |  | 0.12 | 0.75 | 0.00 | 0.00 | 0.00 | 0.00 |  | 4.29 |
| rope.txt | prose | 11,440 | 3,293 | 1.71 | 0.01 | 0.05 |  | 0.00 |  |  | 0.00 | 0.00 | 0.28 | 0.00 | 0.00 | 0.00 |  | 1.06 |
| screenshot.png | picture (PNG) | 92,628 | 80,502 | 2.57 | 0.04 | 0.03 | 0.02 | 0.65 | 0.19 | 0.46 | 0.00 | 0.35 | 0.22 | 0.00 | 0.06 | 0.04 |  | 0.96 |
| vitruvian.jpg | picture (JPEG) | 207,500 | 206,946 | 1.84 | 0.11 | 0.01 | 0.01 | 0.26 | 0.08 | 0.19 | 0.00 | 0.06 | 0.05 | 0.00 | 0.00 | 0.00 |  | 1.25 |
| door.jpg | picture (JPEG) | 69,298 | 63,856 | 0.69 | 0.10 | 0.00 |  | 0.01 | 0.01 |  | 0.00 | 0.03 | 0.05 | 0.00 | 0.03 | 0.02 |  | 0.42 |
| spitfire.obj | mesh | 469,717 | 53,137 | 2.67 | 0.18 |  |  |  |  |  |  | 0.09 | 0.00 | 0.00 | 0.00 | 0.00 |  | 1.76 |
| hymn.mid | melody (MIDI) | 1,377 | 221 | 0.03 | 0.05 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |  |  |
| film.mp4 | video (MP4) | 234,633 | 233,863 | 1.65 | 0.14 | 0.01 | 0.01 | 0.02 | 0.01 | 0.01 | 0.00 | 0.11 | 0.09 | 0.00 | 0.16 | 0.05 |  | 0.93 |
| sound.wav | sound (WAV) | 88,244 | 3,233 | 2.06 | 0.04 | 1.34 | 0.13 | 0.09 | 0.00 | 0.09 | 0.00 | 0.47 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.15 |
| program.exe | program (Windows) | 5,337,607 | 4,223,640 | 20.99 | 0.25 |  |  |  |  |  |  | 1.77 | 0.00 | 0.00 | 0.00 | 0.00 |  | 10.48 |
| folder.tar | tar | 5,744,640 | 1,306,738 | 16.49 | 0.35 |  |  |  |  |  |  | 0.06 | 0.00 | 0.00 | 0.00 | 0.00 |  | 7.93 |
