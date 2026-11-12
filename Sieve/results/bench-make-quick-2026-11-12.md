# Making Sieve instructions: time by stage (2026-11-12)

Intel64 Family 6 Model 167 Stepping 1, GenuineIntel, 12 threads, Windows 10. `sieve locate FILE --installer` (the newest instructions), each file alone; seconds, from `--timings`. Stages nest in the total, and the pieces' rope includes its own search; cutting includes the blocks and the boundaries.

## Search 1 s a file

| File | Kind | Bytes | Made | s | MB/s | rope search | pieces' rope | cutting | blocks | boundaries | rope code | LZMA2 quick | LZMA2 trial | LZMA2 pass 1 | LZMA2 tuned | 7-Zip pass | archives | mixing |
| :--- | :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| prose.md | prose | 627,559 | 152,046 | 3.02 | 0.21 |  |  |  |  |  |  | 0.07 | 0.00 | 0.00 | 0.00 | 0.00 |  | 1.84 |
| code.cpp | code | 208,297 | 36,378 | 1.20 | 0.17 |  |  |  |  |  |  | 0.04 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.74 |
| oracle.py | code | 535,980 | 104,557 | 2.88 | 0.19 |  |  |  |  |  |  | 0.05 | 0.00 | 0.00 | 0.00 | 0.00 |  | 1.90 |
| table.csv | table | 17,831 | 7,249 | 3.07 | 0.01 | 0.92 | 0.08 | 1.17 | 0.20 | 0.97 | 0.00 | 0.14 | 0.09 | 0.00 | 0.00 | 0.00 |  | 0.70 |
| words.txt | word list | 2,921,105 | 287,033 | 12.60 | 0.23 | 0.15 |  | 0.10 | 0.10 |  | 0.09 | 1.25 | 0.23 | 0.00 | 0.00 | 0.00 |  | 4.27 |
| rope.txt | prose | 11,440 | 3,293 | 1.34 | 0.01 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 0.48 | 0.00 | 0.00 | 0.00 |  | 0.52 |
| screenshot.png | picture (PNG) | 92,628 | 80,502 | 2.99 | 0.03 | 0.00 | 0.02 | 1.04 | 0.22 | 0.82 | 0.00 | 0.37 | 0.45 | 0.00 | 0.06 | 0.02 |  | 0.85 |
| vitruvian.jpg | picture (JPEG) | 207,500 | 206,946 | 1.53 | 0.14 | 0.01 | 0.01 | 0.26 | 0.17 | 0.09 | 0.00 | 0.10 | 0.01 | 0.00 | 0.00 | 0.00 |  | 1.09 |
| door.jpg | picture (JPEG) | 69,298 | 63,856 | 0.67 | 0.10 | 0.00 |  | 0.01 | 0.01 |  | 0.00 | 0.04 | 0.04 | 0.00 | 0.03 | 0.01 |  | 0.40 |
| spitfire.obj | mesh | 469,717 | 53,137 | 2.61 | 0.18 |  |  |  |  |  |  | 0.08 | 0.00 | 0.00 | 0.00 | 0.00 |  | 1.73 |
| hymn.mid | melody (MIDI) | 1,377 | 221 | 0.03 | 0.05 | 0.02 |  | 0.00 |  |  | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |  |  |
| film.mp4 | video (MP4) | 234,633 | 233,863 | 1.65 | 0.14 | 0.01 | 0.01 | 0.02 | 0.01 | 0.01 | 0.00 | 0.20 | 0.11 | 0.00 | 0.13 | 0.03 |  | 0.93 |
| sound.wav | sound (WAV) | 88,244 | 3,233 | 1.95 | 0.05 | 1.42 | 0.12 | 0.11 | 0.01 | 0.10 | 0.00 | 0.63 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.15 |
| program.exe | program (Windows) | 5,337,607 | 4,255,565 | 21.26 | 0.25 |  |  |  |  |  |  | 1.87 | 0.00 | 0.00 | 0.00 | 0.00 |  | 13.68 |
| folder.tar | tar | 5,498,880 | 1,252,776 | 16.32 | 0.34 |  |  |  |  |  |  | 0.07 | 0.00 | 0.00 | 0.00 | 0.00 |  | 7.83 |
