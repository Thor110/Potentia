# Making Sieve instructions: time by stage (2026-11-08)

Intel64 Family 6 Model 167 Stepping 1, GenuineIntel, 12 threads, Windows 10. `sieve locate FILE --installer` (the newest instructions), each file alone; seconds, from `--timings`. Stages nest in the total, and the pieces' rope includes its own search; cutting includes the blocks and the boundaries.

## Search 1 s a file

| File | Kind | Bytes | Made | s | MB/s | rope search | pieces' rope | cutting | blocks | boundaries | rope code | LZMA2 quick | LZMA2 trial | LZMA2 pass 1 | LZMA2 tuned | 7-Zip pass | archives | mixing |
| :--- | :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| prose.md | prose | 552,884 | 137,088 | 2.61 | 0.21 |  |  |  |  |  |  | 0.06 | 0.00 | 0.00 | 0.00 | 0.00 |  | 1.71 |
| code.cpp | code | 176,669 | 31,133 | 2.88 | 0.06 |  |  |  |  |  |  | 0.03 | 0.33 | 0.00 | 0.00 | 0.00 |  | 1.68 |
| oracle.py | code | 534,555 | 104,242 | 3.60 | 0.15 |  |  |  |  |  |  | 0.10 | 0.00 | 0.00 | 0.00 | 0.00 |  | 2.56 |
| table.csv | table | 17,831 | 7,249 | 0.93 | 0.02 | 0.52 | 0.04 | 0.02 | 0.00 | 0.02 | 0.00 | 0.12 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.31 |
| words.txt | word list | 2,921,105 | 287,033 | 17.53 | 0.17 | 0.14 |  | 0.09 | 0.09 |  | 0.10 | 1.11 | 0.36 | 0.00 | 0.00 | 0.00 |  | 8.66 |
| rope.txt | prose | 11,440 | 3,293 | 0.48 | 0.02 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.43 |
| screenshot.png | picture (PNG) | 92,628 | 80,502 | 0.89 | 0.10 | 0.00 | 0.02 | 0.06 | 0.01 | 0.05 | 0.00 | 0.35 | 0.03 | 0.00 | 0.04 | 0.02 |  | 0.49 |
| vitruvian.jpg | picture (JPEG) | 207,500 | 206,946 | 1.02 | 0.20 | 0.01 | 0.01 | 0.02 | 0.01 | 0.01 | 0.00 | 0.12 | 0.01 | 0.00 | 0.00 | 0.00 |  | 0.86 |
| door.jpg | picture (JPEG) | 69,298 | 63,856 | 0.57 | 0.12 | 0.00 |  | 0.01 | 0.01 |  | 0.00 | 0.05 | 0.02 | 0.00 | 0.02 | 0.01 |  | 0.35 |
| spitfire.obj | mesh | 469,717 | 53,137 | 2.46 | 0.19 |  |  |  |  |  |  | 0.08 | 0.00 | 0.00 | 0.00 | 0.00 |  | 1.74 |
| hymn.mid | melody (MIDI) | 1,377 | 221 | 0.02 | 0.06 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 0.01 | 0.00 | 0.00 | 0.00 |  |  |
| film.mp4 | video (MP4) | 234,633 | 233,863 | 1.61 | 0.15 | 0.01 | 0.01 | 0.02 | 0.01 | 0.01 | 0.00 | 0.21 | 0.08 | 0.00 | 0.14 | 0.04 |  | 0.89 |
| sound.wav | sound (WAV) | 88,244 | 3,233 | 2.29 | 0.04 | 1.77 | 0.13 | 0.11 | 0.01 | 0.10 | 0.00 | 0.63 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.14 |
| program.exe | program (Windows) | 5,337,607 | 4,255,565 | 20.63 | 0.26 |  |  |  |  |  |  | 1.76 | 0.00 | 0.00 | 0.00 | 0.00 |  | 13.15 |
| folder.tar | tar | 5,396,480 | 1,231,849 | 14.89 | 0.36 |  |  |  |  |  |  | 0.06 | 0.00 | 0.00 | 0.00 | 0.00 |  | 8.02 |
