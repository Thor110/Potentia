# Making Sieve instructions: time by stage (2026-11-08)

Intel64 Family 6 Model 167 Stepping 1, GenuineIntel, 12 threads, Windows 10. `sieve locate FILE --installer` (the newest instructions), each file alone; seconds, from `--timings`. Stages nest in the total, and the pieces' rope includes its own search; cutting includes the blocks and the boundaries.

## Search 1 s a file

| File | Kind | Bytes | Made | s | MB/s | rope search | pieces' rope | cutting | blocks | boundaries | rope code | LZMA2 quick | LZMA2 trial | LZMA2 pass 1 | LZMA2 tuned | 7-Zip pass | archives | mixing |
| :--- | :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| prose.md | prose | 555,248 | 137,658 | 2.71 | 0.20 |  |  |  |  |  |  | 0.06 | 0.00 | 0.00 | 0.00 | 0.00 |  | 1.79 |
| code.cpp | code | 176,669 | 31,133 | 3.31 | 0.05 |  |  |  |  |  |  | 0.04 | 0.48 | 0.00 | 0.00 | 0.00 |  | 2.10 |
| oracle.py | code | 534,555 | 104,242 | 3.21 | 0.17 |  |  |  |  |  |  | 0.11 | 0.00 | 0.00 | 0.00 | 0.00 |  | 2.21 |
| table.csv | table | 17,831 | 7,249 | 0.90 | 0.02 | 0.50 | 0.04 | 0.02 | 0.00 | 0.01 | 0.00 | 0.12 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.30 |
| words.txt | word list | 2,921,105 | 287,033 | 18.09 | 0.16 | 0.15 |  | 0.09 | 0.09 |  | 0.10 | 1.08 | 0.30 | 0.00 | 0.00 | 0.00 |  | 9.77 |
| rope.txt | prose | 11,440 | 3,293 | 0.44 | 0.03 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.40 |
| screenshot.png | picture (PNG) | 92,628 | 80,502 | 0.86 | 0.11 | 0.00 | 0.02 | 0.06 | 0.01 | 0.05 | 0.00 | 0.34 | 0.03 | 0.00 | 0.04 | 0.02 |  | 0.46 |
| vitruvian.jpg | picture (JPEG) | 207,500 | 206,946 | 1.02 | 0.20 | 0.01 | 0.01 | 0.02 | 0.01 | 0.01 | 0.00 | 0.13 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.86 |
| door.jpg | picture (JPEG) | 69,298 | 63,856 | 0.56 | 0.12 | 0.00 |  | 0.00 | 0.00 |  | 0.00 | 0.05 | 0.02 | 0.00 | 0.03 | 0.01 |  | 0.36 |
| spitfire.obj | mesh | 469,717 | 53,137 | 2.70 | 0.17 |  |  |  |  |  |  | 0.07 | 0.00 | 0.00 | 0.00 | 0.00 |  | 1.86 |
| hymn.mid | melody (MIDI) | 1,377 | 221 | 0.02 | 0.06 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 0.01 | 0.00 | 0.00 | 0.00 |  |  |
| film.mp4 | video (MP4) | 234,633 | 233,863 | 1.76 | 0.13 | 0.01 | 0.01 | 0.02 | 0.01 | 0.01 | 0.00 | 0.19 | 0.09 | 0.00 | 0.15 | 0.04 |  | 1.02 |
| sound.wav | sound (WAV) | 88,244 | 3,233 | 2.32 | 0.04 | 1.83 | 0.11 | 0.10 | 0.00 | 0.09 | 0.00 | 0.62 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.15 |
| program.exe | program (Windows) | 5,337,607 | 4,255,565 | 20.49 | 0.26 |  |  |  |  |  |  | 1.79 | 0.00 | 0.00 | 0.00 | 0.00 |  | 13.21 |
| folder.tar | tar | 5,396,480 | 1,232,410 | 14.57 | 0.37 |  |  |  |  |  |  | 0.06 | 0.00 | 0.00 | 0.00 | 0.00 |  | 7.89 |
