# Making Sieve instructions: time by stage (2026-11-06)

Intel64 Family 6 Model 167 Stepping 1, GenuineIntel, 12 threads, Windows 10. `sieve locate FILE --installer` (the newest instructions), each file alone; seconds, from `--timings`. Stages nest in the total, and the pieces' rope includes its own search; cutting includes the blocks and the boundaries.

## Search 1 s a file

| File | Kind | Bytes | Made | s | MB/s | rope search | pieces' rope | cutting | blocks | boundaries | rope code | LZMA2 quick | LZMA2 trial | LZMA2 pass 1 | LZMA2 tuned | 7-Zip pass | archives | mixing |
| :--- | :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| prose.md | prose | 511,897 | 127,545 | 3.67 | 0.14 |  |  |  |  |  |  | 0.07 | 0.00 | 0.00 | 0.00 | 0.00 |  | 2.74 |
| code.cpp | code | 174,227 | 30,914 | 1.98 | 0.09 |  |  |  |  |  |  | 0.04 | 0.00 | 0.00 | 0.00 | 0.00 |  | 1.55 |
| oracle.py | code | 487,013 | 96,232 | 4.03 | 0.12 |  |  |  |  |  |  | 0.08 | 0.00 | 0.00 | 0.00 | 0.00 |  | 3.05 |
| table.csv | table | 17,831 | 7,249 | 1.07 | 0.02 | 0.53 | 0.05 | 0.02 | 0.00 | 0.02 | 0.00 | 0.13 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.44 |
| words.txt | word list | 2,921,105 | 287,033 | 13.71 | 0.21 | 0.19 |  | 0.11 | 0.11 |  | 0.12 | 1.36 | 0.00 | 0.00 | 0.00 | 0.00 |  | 5.93 |
| rope.txt | prose | 11,440 | 3,293 | 1.52 | 0.01 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 0.42 | 0.00 | 0.00 | 0.00 |  | 0.81 |
| screenshot.png | picture (PNG) | 92,628 | 80,502 | 4.11 | 0.02 | 0.00 | 0.02 | 1.06 | 0.18 | 0.88 | 0.00 | 0.43 | 0.72 | 0.00 | 0.05 | 0.03 |  | 1.51 |
| vitruvian.jpg | picture (JPEG) | 207,500 | 206,946 | 2.32 | 0.09 | 0.01 | 0.02 | 0.09 | 0.07 | 0.03 | 0.00 | 0.08 | 0.01 | 0.00 | 0.00 | 0.00 |  | 2.06 |
| door.jpg | picture (JPEG) | 69,298 | 63,856 | 0.92 | 0.08 | 0.00 |  | 0.01 | 0.01 |  | 0.00 | 0.04 | 0.02 | 0.00 | 0.02 | 0.01 |  | 0.71 |
| spitfire.obj | mesh | 469,717 | 53,144 | 3.81 | 0.12 |  |  |  |  |  |  | 0.08 | 0.00 | 0.00 | 0.00 | 0.00 |  | 2.85 |
| hymn.mid | melody (MIDI) | 1,377 | 221 | 0.03 | 0.05 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |  |  |
| film.mp4 | video (MP4) | 234,633 | 233,863 | 2.88 | 0.08 | 0.01 | 0.01 | 0.02 | 0.01 | 0.01 | 0.00 | 0.19 | 0.07 | 0.00 | 0.11 | 0.05 |  | 2.21 |
| sound.wav | sound (WAV) | 88,244 | 3,263 | 2.49 | 0.04 | 1.81 | 0.12 | 0.11 | 0.00 | 0.10 | 0.00 | 0.66 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.31 |
| program.exe | program (Windows) | 5,337,607 | 4,255,565 | 29.48 | 0.18 |  |  |  |  |  |  | 1.56 | 0.00 | 0.00 | 0.00 | 0.00 |  | 19.85 |
| folder.tar | tar | 5,304,320 | 1,212,090 | 18.53 | 0.29 |  |  |  |  |  |  | 0.07 | 0.00 | 0.00 | 0.00 | 0.00 |  | 9.96 |
