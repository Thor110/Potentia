# Making Sieve instructions: time by stage (2026-11-12)

Intel64 Family 6 Model 167 Stepping 1, GenuineIntel, 12 threads, Windows 10. `sieve locate FILE --installer` (the newest instructions), each file alone; seconds, from `--timings`. Stages nest in the total, and the pieces' rope includes its own search; cutting includes the blocks and the boundaries.

## Search 1 s a file

| File | Kind | Bytes | Made | s | MB/s | rope search | pieces' rope | cutting | blocks | boundaries | rope code | LZMA2 quick | LZMA2 trial | LZMA2 pass 1 | LZMA2 tuned | 7-Zip pass | archives | mixing |
| :--- | :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| prose.md | prose | 627,652 | 152,067 | 2.79 | 0.22 |  |  |  |  |  |  | 0.07 | 0.00 | 0.00 | 0.00 | 0.00 |  | 1.73 |
| code.cpp | code | 208,445 | 36,425 | 1.20 | 0.17 |  |  |  |  |  |  | 0.04 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.81 |
| oracle.py | code | 535,980 | 104,557 | 3.16 | 0.17 |  |  |  |  |  |  | 0.07 | 0.00 | 0.00 | 0.00 | 0.00 |  | 1.79 |
| table.csv | table | 17,831 | 7,249 | 2.05 | 0.01 | 0.99 | 0.06 | 0.24 | 0.08 | 0.16 | 0.00 | 0.12 | 0.07 | 0.00 | 0.00 | 0.00 |  | 0.58 |
| words.txt | word list | 2,921,105 | 287,033 | 14.69 | 0.20 | 0.15 |  | 0.10 | 0.09 |  | 0.10 | 1.16 | 0.30 | 0.00 | 0.00 | 0.00 |  | 4.28 |
| rope.txt | prose | 11,440 | 3,293 | 1.28 | 0.01 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 0.26 | 0.00 | 0.00 | 0.00 |  | 0.79 |
| screenshot.png | picture (PNG) | 92,628 | 80,502 | 1.71 | 0.05 | 0.00 | 0.02 | 0.58 | 0.14 | 0.43 | 0.00 | 0.38 | 0.06 | 0.00 | 0.05 | 0.02 |  | 0.65 |
| vitruvian.jpg | picture (JPEG) | 207,500 | 206,946 | 1.05 | 0.20 | 0.01 | 0.01 | 0.02 | 0.01 | 0.01 | 0.00 | 0.12 | 0.01 | 0.00 | 0.00 | 0.00 |  | 0.89 |
| door.jpg | picture (JPEG) | 69,298 | 63,856 | 0.61 | 0.11 | 0.00 |  | 0.01 | 0.01 |  | 0.00 | 0.04 | 0.03 | 0.00 | 0.03 | 0.01 |  | 0.37 |
| spitfire.obj | mesh | 469,717 | 53,137 | 2.77 | 0.17 |  |  |  |  |  |  | 0.10 | 0.00 | 0.00 | 0.00 | 0.00 |  | 1.78 |
| hymn.mid | melody (MIDI) | 1,377 | 221 | 0.03 | 0.05 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |  |  |
| film.mp4 | video (MP4) | 234,633 | 233,863 | 1.61 | 0.15 | 0.01 | 0.01 | 0.02 | 0.01 | 0.01 | 0.00 | 0.20 | 0.08 | 0.00 | 0.13 | 0.03 |  | 0.94 |
| sound.wav | sound (WAV) | 88,244 | 3,233 | 2.36 | 0.04 | 1.64 | 0.10 | 0.10 | 0.00 | 0.09 | 0.00 | 0.66 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.16 |
| program.exe | program (Windows) | 5,337,607 | 4,223,640 | 21.82 | 0.24 |  |  |  |  |  |  | 1.85 | 0.00 | 0.00 | 0.00 | 0.00 |  | 11.41 |
| folder.tar | tar | 5,498,880 | 1,252,843 | 16.79 | 0.33 |  |  |  |  |  |  | 0.07 | 0.00 | 0.00 | 0.00 | 0.00 |  | 8.02 |
