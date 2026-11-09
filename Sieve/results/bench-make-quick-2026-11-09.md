# Making Sieve instructions: time by stage (2026-11-09)

Intel64 Family 6 Model 167 Stepping 1, GenuineIntel, 12 threads, Windows 10. `sieve locate FILE --installer` (the newest instructions), each file alone; seconds, from `--timings`. Stages nest in the total, and the pieces' rope includes its own search; cutting includes the blocks and the boundaries.

## Search 1 s a file

| File | Kind | Bytes | Made | s | MB/s | rope search | pieces' rope | cutting | blocks | boundaries | rope code | LZMA2 quick | LZMA2 trial | LZMA2 pass 1 | LZMA2 tuned | 7-Zip pass | archives | mixing |
| :--- | :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| prose.md | prose | 556,631 | 137,980 | 5.47 | 0.10 |  |  |  |  |  |  | 0.06 | 1.29 | 0.00 | 0.00 | 0.00 |  | 2.12 |
| code.cpp | code | 176,669 | 31,133 | 1.35 | 0.13 |  |  |  |  |  |  | 0.05 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.90 |
| oracle.py | code | 534,692 | 104,270 | 2.77 | 0.19 |  |  |  |  |  |  | 0.07 | 0.00 | 0.00 | 0.00 | 0.00 |  | 1.71 |
| table.csv | table | 17,831 | 7,249 | 0.89 | 0.02 | 0.49 | 0.04 | 0.02 | 0.00 | 0.01 | 0.00 | 0.11 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.31 |
| words.txt | word list | 2,921,105 | 287,033 | 21.29 | 0.14 | 0.21 |  | 0.17 | 0.16 |  | 0.13 | 1.92 | 0.39 | 0.00 | 0.00 | 0.00 |  | 10.64 |
| rope.txt | prose | 11,440 | 3,293 | 0.58 | 0.02 | 0.03 |  | 0.00 |  |  | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.49 |
| screenshot.png | picture (PNG) | 92,628 | 80,502 | 1.47 | 0.06 | 0.00 | 0.02 | 0.09 | 0.01 | 0.09 | 0.00 | 0.41 | 0.16 | 0.00 | 0.07 | 0.02 |  | 0.64 |
| vitruvian.jpg | picture (JPEG) | 207,500 | 206,946 | 2.15 | 0.10 | 0.01 | 0.02 | 0.20 | 0.06 | 0.13 | 0.00 | 0.13 | 0.04 | 0.00 | 0.00 | 0.00 |  | 1.74 |
| door.jpg | picture (JPEG) | 69,298 | 63,856 | 1.60 | 0.04 | 0.00 |  | 0.08 | 0.08 |  | 0.00 | 0.05 | 0.46 | 0.00 | 0.04 | 0.01 |  | 0.51 |
| spitfire.obj | mesh | 469,717 | 53,137 | 3.43 | 0.14 |  |  |  |  |  |  | 0.12 | 0.00 | 0.00 | 0.00 | 0.00 |  | 2.30 |
| hymn.mid | melody (MIDI) | 1,377 | 221 | 0.02 | 0.06 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 0.01 | 0.00 | 0.00 | 0.00 |  |  |
| film.mp4 | video (MP4) | 234,633 | 233,863 | 1.71 | 0.14 | 0.01 | 0.01 | 0.02 | 0.01 | 0.01 | 0.00 | 0.23 | 0.11 | 0.00 | 0.12 | 0.04 |  | 0.97 |
| sound.wav | sound (WAV) | 88,244 | 3,233 | 2.39 | 0.04 | 1.85 | 0.14 | 0.10 | 0.00 | 0.10 | 0.00 | 0.64 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.17 |
| program.exe | program (Windows) | 5,337,607 | 4,255,565 | 21.88 | 0.24 |  |  |  |  |  |  | 2.07 | 0.00 | 0.00 | 0.00 | 0.00 |  | 14.62 |
| folder.tar | tar | 5,396,480 | 1,232,755 | 16.85 | 0.32 |  |  |  |  |  |  | 0.07 | 0.00 | 0.00 | 0.00 | 0.00 |  | 7.98 |
