# Making Sieve instructions: time by stage (2026-11-06)

Intel64 Family 6 Model 167 Stepping 1, GenuineIntel, 12 threads, Windows 10. `sieve locate FILE --installer` (the newest instructions), each file alone; seconds, from `--timings`. Stages nest in the total, and the pieces' rope includes its own search; cutting includes the blocks and the boundaries.

## Search 1 s a file

| File | Kind | Bytes | Made | s | MB/s | rope search | pieces' rope | cutting | blocks | boundaries | rope code | LZMA2 quick | LZMA2 trial | LZMA2 pass 1 | LZMA2 tuned | 7-Zip pass | archives | mixing |
| :--- | :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| prose.md | prose | 520,284 | 129,708 | 2.83 | 0.18 |  |  |  |  |  |  | 0.08 | 0.00 | 0.00 | 0.00 | 0.00 |  | 1.94 |
| code.cpp | code | 174,622 | 30,754 | 1.00 | 0.17 |  |  |  |  |  |  | 0.04 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.70 |
| oracle.py | code | 492,594 | 97,311 | 2.40 | 0.21 |  |  |  |  |  |  | 0.06 | 0.00 | 0.00 | 0.00 | 0.00 |  | 1.60 |
| table.csv | table | 17,831 | 7,249 | 0.99 | 0.02 | 0.53 | 0.06 | 0.02 | 0.00 | 0.02 | 0.00 | 0.13 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.35 |
| words.txt | word list | 2,921,105 | 287,033 | 10.03 | 0.29 | 0.21 |  | 0.11 | 0.10 |  | 0.14 | 1.27 | 0.00 | 0.00 | 0.00 | 0.00 |  | 4.92 |
| rope.txt | prose | 11,440 | 3,293 | 0.32 | 0.04 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.29 |
| screenshot.png | picture (PNG) | 92,628 | 80,502 | 0.66 | 0.14 | 0.00 | 0.01 | 0.04 | 0.01 | 0.03 | 0.00 | 0.32 | 0.02 | 0.00 | 0.03 | 0.01 |  | 0.37 |
| vitruvian.jpg | picture (JPEG) | 207,500 | 206,946 | 0.76 | 0.27 | 0.01 | 0.01 | 0.01 | 0.01 | 0.01 | 0.00 | 0.12 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.64 |
| door.jpg | picture (JPEG) | 69,298 | 63,856 | 0.44 | 0.16 | 0.00 |  | 0.00 | 0.00 |  | 0.00 | 0.04 | 0.02 | 0.00 | 0.02 | 0.01 |  | 0.28 |
| spitfire.obj | mesh | 469,717 | 53,137 | 4.99 | 0.09 |  |  |  |  |  |  | 0.06 | 0.54 | 0.00 | 0.00 | 0.00 |  | 2.70 |
| hymn.mid | melody (MIDI) | 1,377 | 221 | 1.59 | 0.00 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 1.04 | 0.00 | 0.00 | 0.00 |  |  |
| film.mp4 | video (MP4) | 234,633 | 233,863 | 4.62 | 0.05 | 0.04 | 0.02 | 0.77 | 0.35 | 0.42 | 0.00 | 0.25 | 0.61 | 0.00 | 0.27 | 0.08 |  | 1.89 |
| sound.wav | sound (WAV) | 88,244 | 3,233 | 3.63 | 0.02 | 3.00 | 0.15 | 0.14 | 0.01 | 0.14 | 0.00 | 0.59 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.17 |
| program.exe | program (Windows) | 5,337,607 | 4,255,565 | 22.01 | 0.24 |  |  |  |  |  |  | 1.75 | 0.00 | 0.00 | 0.00 | 0.00 |  | 14.66 |
| folder.tar | tar | 5,314,560 | 1,215,398 | 14.52 | 0.37 |  |  |  |  |  |  | 0.06 | 0.00 | 0.00 | 0.00 | 0.00 |  | 8.02 |
