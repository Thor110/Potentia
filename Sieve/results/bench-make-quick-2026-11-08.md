# Making Sieve instructions: time by stage (2026-11-08)

Intel64 Family 6 Model 167 Stepping 1, GenuineIntel, 12 threads, Windows 10. `sieve locate FILE --installer` (the newest instructions), each file alone; seconds, from `--timings`. Stages nest in the total, and the pieces' rope includes its own search; cutting includes the blocks and the boundaries.

## Search 1 s a file

| File | Kind | Bytes | Made | s | MB/s | rope search | pieces' rope | cutting | blocks | boundaries | rope code | LZMA2 quick | LZMA2 trial | LZMA2 pass 1 | LZMA2 tuned | 7-Zip pass | archives | mixing |
| :--- | :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| prose.md | prose | 552,543 | 136,996 | 3.60 | 0.15 |  |  |  |  |  |  | 0.06 | 0.00 | 0.00 | 0.00 | 0.00 |  | 1.68 |
| code.cpp | code | 176,203 | 31,058 | 2.47 | 0.07 |  |  |  |  |  |  | 0.14 | 0.02 | 0.00 | 0.00 | 0.00 |  | 1.68 |
| oracle.py | code | 518,814 | 102,293 | 2.75 | 0.19 |  |  |  |  |  |  | 0.09 | 0.00 | 0.00 | 0.00 | 0.00 |  | 1.88 |
| table.csv | table | 17,831 | 7,249 | 0.90 | 0.02 | 0.49 | 0.04 | 0.02 | 0.00 | 0.02 | 0.00 | 0.12 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.32 |
| words.txt | word list | 2,921,105 | 287,033 | 18.79 | 0.16 | 0.15 |  | 0.09 | 0.09 |  | 0.11 | 1.11 | 0.22 | 0.00 | 0.00 | 0.00 |  | 10.92 |
| rope.txt | prose | 11,440 | 3,293 | 0.44 | 0.03 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.40 |
| screenshot.png | picture (PNG) | 92,628 | 80,502 | 0.85 | 0.11 | 0.00 | 0.02 | 0.06 | 0.01 | 0.05 | 0.00 | 0.35 | 0.04 | 0.00 | 0.04 | 0.02 |  | 0.44 |
| vitruvian.jpg | picture (JPEG) | 207,500 | 206,946 | 0.98 | 0.21 | 0.01 | 0.01 | 0.02 | 0.01 | 0.01 | 0.00 | 0.13 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.81 |
| door.jpg | picture (JPEG) | 69,298 | 63,856 | 0.67 | 0.10 | 0.00 |  | 0.00 | 0.00 |  | 0.00 | 0.04 | 0.03 | 0.00 | 0.05 | 0.01 |  | 0.35 |
| spitfire.obj | mesh | 469,717 | 53,137 | 2.49 | 0.19 |  |  |  |  |  |  | 0.08 | 0.00 | 0.00 | 0.00 | 0.00 |  | 1.72 |
| hymn.mid | melody (MIDI) | 1,377 | 221 | 0.02 | 0.06 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 0.01 | 0.00 | 0.00 | 0.00 |  |  |
| film.mp4 | video (MP4) | 234,633 | 233,863 | 1.52 | 0.15 | 0.01 | 0.01 | 0.02 | 0.01 | 0.01 | 0.00 | 0.20 | 0.09 | 0.00 | 0.10 | 0.03 |  | 0.87 |
| sound.wav | sound (WAV) | 88,244 | 3,233 | 2.29 | 0.04 | 1.75 | 0.13 | 0.11 | 0.01 | 0.10 | 0.00 | 0.64 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.15 |
| program.exe | program (Windows) | 5,337,607 | 4,255,565 | 20.53 | 0.26 |  |  |  |  |  |  | 1.72 | 0.00 | 0.00 | 0.00 | 0.00 |  | 13.52 |
| folder.tar | tar | 5,376,000 | 1,228,860 | 14.26 | 0.38 |  |  |  |  |  |  | 0.06 | 0.00 | 0.00 | 0.00 | 0.00 |  | 7.73 |
