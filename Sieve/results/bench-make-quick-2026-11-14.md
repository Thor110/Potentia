# Making Sieve instructions: time by stage (2026-11-14)

Intel64 Family 6 Model 167 Stepping 1, GenuineIntel, 12 threads, Windows 10. `sieve locate FILE --installer` (the newest instructions), each file alone; seconds, from `--timings`. Stages nest in the total, and the pieces' rope includes its own search; cutting includes the blocks and the boundaries.

## Search 1 s a file

| File | Kind | Bytes | Made | s | MB/s | rope search | pieces' rope | cutting | blocks | boundaries | rope code | LZMA2 quick | LZMA2 trial | LZMA2 pass 1 | LZMA2 tuned | 7-Zip pass | archives | mixing |
| :--- | :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| prose.md | prose | 704,228 | 169,333 | 5.50 | 0.13 |  |  |  |  |  |  | 0.07 | 0.56 | 0.00 | 0.00 | 0.00 |  | 2.45 |
| code.cpp | code | 210,617 | 36,749 | 1.33 | 0.16 |  |  |  |  |  |  | 0.05 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.88 |
| oracle.py | code | 535,980 | 104,557 | 2.68 | 0.20 |  |  |  |  |  |  | 0.06 | 0.00 | 0.00 | 0.00 | 0.00 |  | 1.66 |
| table.csv | table | 17,831 | 7,249 | 0.83 | 0.02 | 0.41 | 0.04 | 0.02 | 0.00 | 0.01 | 0.00 | 0.12 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.32 |
| words.txt | word list | 2,921,105 | 287,033 | 17.82 | 0.16 | 0.22 |  | 0.45 | 0.44 |  | 0.13 | 1.42 | 0.27 | 0.00 | 0.00 | 0.00 |  | 9.93 |
| rope.txt | prose | 11,440 | 3,293 | 0.41 | 0.03 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.37 |
| screenshot.png | picture (PNG) | 92,628 | 80,502 | 0.86 | 0.11 | 0.00 | 0.02 | 0.05 | 0.01 | 0.04 | 0.00 | 0.34 | 0.04 | 0.00 | 0.04 | 0.02 |  | 0.49 |
| vitruvian.jpg | picture (JPEG) | 207,500 | 206,946 | 1.02 | 0.20 | 0.01 | 0.01 | 0.02 | 0.01 | 0.01 | 0.00 | 0.14 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.86 |
| door.jpg | picture (JPEG) | 69,298 | 63,856 | 0.62 | 0.11 | 0.00 |  | 0.00 | 0.00 |  | 0.00 | 0.05 | 0.03 | 0.00 | 0.03 | 0.01 |  | 0.37 |
| spitfire.obj | mesh | 469,717 | 53,137 | 2.59 | 0.18 |  |  |  |  |  |  | 0.09 | 0.00 | 0.00 | 0.00 | 0.00 |  | 1.74 |
| hymn.mid | melody (MIDI) | 1,377 | 221 | 0.03 | 0.05 | 0.02 |  | 0.00 |  |  | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |  |  |
| film.mp4 | video (MP4) | 234,633 | 233,863 | 1.46 | 0.16 | 0.01 | 0.01 | 0.02 | 0.01 | 0.01 | 0.00 | 0.19 | 0.08 | 0.00 | 0.11 | 0.04 |  | 0.82 |
| sound.wav | sound (WAV) | 88,244 | 3,233 | 2.11 | 0.04 | 1.41 | 0.12 | 0.09 | 0.00 | 0.09 | 0.00 | 0.62 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.14 |
| program.exe | program (Windows) | 5,337,607 | 4,223,640 | 23.08 | 0.23 |  |  |  |  |  |  | 1.64 | 0.00 | 0.00 | 0.00 | 0.00 |  | 11.12 |
| folder.tar | tar | 5,580,800 | 1,270,699 | 18.00 | 0.31 |  |  |  |  |  |  | 0.07 | 0.00 | 0.00 | 0.00 | 0.00 |  | 7.94 |
