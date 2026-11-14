# Making Sieve instructions: time by stage (2026-11-14)

Intel64 Family 6 Model 167 Stepping 1, GenuineIntel, 12 threads, Windows 10. `sieve locate FILE --installer` (the newest instructions), each file alone; seconds, from `--timings`. Stages nest in the total, and the pieces' rope includes its own search; cutting includes the blocks and the boundaries.

## Search 1 s a file

| File | Kind | Bytes | Made | s | MB/s | rope search | pieces' rope | cutting | blocks | boundaries | rope code | LZMA2 quick | LZMA2 trial | LZMA2 pass 1 | LZMA2 tuned | 7-Zip pass | archives | mixing |
| :--- | :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| prose.md | prose | 766,816 | 183,539 | 3.40 | 0.23 |  |  |  |  |  |  | 0.06 | 0.00 | 0.00 | 0.00 | 0.00 |  | 2.05 |
| code.cpp | code | 209,190 | 36,728 | 1.34 | 0.16 |  |  |  |  |  |  | 0.04 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.84 |
| oracle.py | code | 535,980 | 104,557 | 3.35 | 0.16 |  |  |  |  |  |  | 0.06 | 0.00 | 0.00 | 0.00 | 0.00 |  | 1.80 |
| table.csv | table | 17,831 | 7,249 | 2.62 | 0.01 | 1.04 | 0.10 | 0.43 | 0.11 | 0.31 | 0.00 | 0.20 | 0.16 | 0.00 | 0.00 | 0.00 |  | 0.78 |
| words.txt | word list | 2,921,105 | 287,033 | 13.02 | 0.22 | 0.13 |  | 0.09 | 0.09 |  | 0.09 | 1.12 | 0.01 | 0.00 | 0.00 | 0.00 |  | 4.37 |
| rope.txt | prose | 11,440 | 3,293 | 1.26 | 0.01 | 0.02 |  | 0.00 |  |  | 0.00 | 0.00 | 0.26 | 0.00 | 0.00 | 0.00 |  | 0.79 |
| screenshot.png | picture (PNG) | 92,628 | 80,502 | 2.41 | 0.04 | 0.00 | 0.02 | 0.42 | 0.06 | 0.35 | 0.00 | 0.32 | 0.36 | 0.00 | 0.04 | 0.02 |  | 1.05 |
| vitruvian.jpg | picture (JPEG) | 207,500 | 206,946 | 1.58 | 0.13 | 0.01 | 0.01 | 0.36 | 0.15 | 0.21 | 0.00 | 0.11 | 0.03 | 0.00 | 0.00 | 0.00 |  | 1.00 |
| door.jpg | picture (JPEG) | 69,298 | 63,856 | 0.77 | 0.09 | 0.00 |  | 0.02 | 0.02 |  | 0.00 | 0.04 | 0.04 | 0.00 | 0.03 | 0.01 |  | 0.44 |
| spitfire.obj | mesh | 469,717 | 53,137 | 3.41 | 0.14 |  |  |  |  |  |  | 0.10 | 0.01 | 0.00 | 0.00 | 0.00 |  | 2.29 |
| hymn.mid | melody (MIDI) | 1,377 | 221 | 0.04 | 0.04 | 0.02 |  | 0.00 |  |  | 0.00 | 0.00 | 0.01 | 0.00 | 0.00 | 0.00 |  |  |
| film.mp4 | video (MP4) | 234,633 | 233,863 | 1.72 | 0.14 | 0.01 | 0.02 | 0.06 | 0.03 | 0.03 | 0.00 | 0.15 | 0.09 | 0.00 | 0.12 | 0.05 |  | 0.98 |
| sound.wav | sound (WAV) | 88,244 | 3,233 | 2.19 | 0.04 | 1.40 | 0.13 | 0.11 | 0.00 | 0.11 | 0.00 | 0.65 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.16 |
| program.exe | program (Windows) | 5,337,607 | 4,223,640 | 22.59 | 0.24 |  |  |  |  |  |  | 1.90 | 0.00 | 0.00 | 0.00 | 0.00 |  | 12.02 |
| folder.tar | tar | 5,642,240 | 1,285,085 | 18.04 | 0.31 |  |  |  |  |  |  | 0.07 | 0.00 | 0.00 | 0.00 | 0.00 |  | 8.87 |
