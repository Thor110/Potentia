# Making Sieve instructions: time by stage (2026-11-08)

Intel64 Family 6 Model 167 Stepping 1, GenuineIntel, 12 threads, Windows 10. `sieve locate FILE --installer` (the newest instructions), each file alone; seconds, from `--timings`. Stages nest in the total, and the pieces' rope includes its own search; cutting includes the blocks and the boundaries.

## Search 1 s a file

| File | Kind | Bytes | Made | s | MB/s | rope search | pieces' rope | cutting | blocks | boundaries | rope code | LZMA2 quick | LZMA2 trial | LZMA2 pass 1 | LZMA2 tuned | 7-Zip pass | archives | mixing |
| :--- | :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| prose.md | prose | 552,543 | 136,996 | 2.73 | 0.20 |  |  |  |  |  |  | 0.07 | 0.00 | 0.00 | 0.00 | 0.00 |  | 1.80 |
| code.cpp | code | 176,203 | 31,058 | 3.80 | 0.05 |  |  |  |  |  |  | 0.09 | 0.47 | 0.00 | 0.00 | 0.00 |  | 2.58 |
| oracle.py | code | 533,344 | 103,975 | 3.08 | 0.17 |  |  |  |  |  |  | 0.09 | 0.00 | 0.00 | 0.00 | 0.00 |  | 2.14 |
| table.csv | table | 17,831 | 7,249 | 0.95 | 0.02 | 0.52 | 0.04 | 0.01 | 0.00 | 0.01 | 0.00 | 0.12 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.34 |
| words.txt | word list | 2,921,105 | 287,033 | 19.15 | 0.15 | 0.24 |  | 0.10 | 0.09 |  | 0.11 | 1.17 | 0.60 | 0.00 | 0.00 | 0.00 |  | 12.11 |
| rope.txt | prose | 11,440 | 3,293 | 0.41 | 0.03 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.37 |
| screenshot.png | picture (PNG) | 92,628 | 80,502 | 0.81 | 0.11 | 0.00 | 0.02 | 0.04 | 0.01 | 0.04 | 0.00 | 0.34 | 0.04 | 0.00 | 0.04 | 0.01 |  | 0.44 |
| vitruvian.jpg | picture (JPEG) | 207,500 | 206,946 | 1.01 | 0.20 | 0.01 | 0.01 | 0.02 | 0.01 | 0.01 | 0.00 | 0.12 | 0.01 | 0.00 | 0.00 | 0.00 |  | 0.86 |
| door.jpg | picture (JPEG) | 69,298 | 63,856 | 0.53 | 0.13 | 0.00 |  | 0.00 | 0.00 |  | 0.00 | 0.04 | 0.02 | 0.00 | 0.02 | 0.01 |  | 0.33 |
| spitfire.obj | mesh | 469,717 | 53,137 | 2.58 | 0.18 |  |  |  |  |  |  | 0.17 | 0.00 | 0.00 | 0.00 | 0.00 |  | 1.74 |
| hymn.mid | melody (MIDI) | 1,377 | 221 | 0.03 | 0.05 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |  |  |
| film.mp4 | video (MP4) | 234,633 | 233,863 | 1.53 | 0.15 | 0.01 | 0.01 | 0.02 | 0.01 | 0.01 | 0.00 | 0.17 | 0.09 | 0.00 | 0.09 | 0.03 |  | 0.89 |
| sound.wav | sound (WAV) | 88,244 | 3,233 | 2.40 | 0.04 | 1.89 | 0.13 | 0.10 | 0.00 | 0.09 | 0.00 | 0.62 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.15 |
| program.exe | program (Windows) | 5,337,607 | 4,255,565 | 20.69 | 0.26 |  |  |  |  |  |  | 1.81 | 0.00 | 0.00 | 0.00 | 0.00 |  | 13.64 |
| folder.tar | tar | 5,396,480 | 1,231,402 | 14.64 | 0.37 |  |  |  |  |  |  | 0.06 | 0.00 | 0.00 | 0.00 | 0.00 |  | 8.06 |
