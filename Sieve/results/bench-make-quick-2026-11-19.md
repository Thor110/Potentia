# Making Sieve instructions: time by stage (2026-11-19)

Intel64 Family 6 Model 167 Stepping 1, GenuineIntel, 12 threads, Windows 10. `sieve locate FILE --installer` (the newest instructions), each file alone; seconds, from `--timings`. Stages nest in the total, and the pieces' rope includes its own search; cutting includes the blocks and the boundaries.

## Search 1 s a file

| File | Kind | Bytes | Made | s | MB/s | rope search | pieces' rope | cutting | blocks | boundaries | rope code | LZMA2 quick | LZMA2 trial | LZMA2 pass 1 | LZMA2 tuned | 7-Zip pass | archives | mixing |
| :--- | :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| prose.md | prose | 905,343 | 215,339 | 4.06 | 0.22 |  |  |  |  |  |  | 0.12 | 0.00 | 0.00 | 0.00 | 0.00 |  | 2.23 |
| code.cpp | code | 239,807 | 41,519 | 1.39 | 0.17 |  |  |  |  |  |  | 0.05 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.88 |
| oracle.py | code | 535,980 | 104,557 | 5.66 | 0.09 |  |  |  |  |  |  | 0.10 | 0.42 | 0.00 | 0.00 | 0.00 |  | 2.33 |
| table.csv | table | 17,831 | 7,249 | 0.97 | 0.02 | 0.52 | 0.06 | 0.02 | 0.00 | 0.01 | 0.00 | 0.11 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.34 |
| words.txt | word list | 2,921,105 | 287,033 | 18.04 | 0.16 | 0.15 |  | 0.06 | 0.06 |  | 0.10 | 0.68 | 0.40 | 0.00 | 0.00 | 0.00 |  | 6.33 |
| rope.txt | prose | 11,440 | 3,293 | 1.50 | 0.01 | 0.04 |  | 0.00 |  |  | 0.00 | 0.00 | 0.28 | 0.00 | 0.00 | 0.00 |  | 0.95 |
| screenshot.png | picture (PNG) | 92,628 | 80,502 | 1.13 | 0.08 | 0.00 | 0.02 | 0.13 | 0.02 | 0.11 | 0.00 | 0.30 | 0.05 | 0.00 | 0.05 | 0.02 |  | 0.58 |
| vitruvian.jpg | picture (JPEG) | 207,500 | 206,946 | 1.06 | 0.20 | 0.01 | 0.01 | 0.02 | 0.01 | 0.01 | 0.00 | 0.05 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.90 |
| door.jpg | picture (JPEG) | 69,298 | 63,856 | 0.55 | 0.13 | 0.00 |  | 0.00 | 0.00 |  | 0.00 | 0.03 | 0.03 | 0.00 | 0.03 | 0.01 |  | 0.34 |
| spitfire.obj | mesh | 469,717 | 53,137 | 2.41 | 0.20 |  |  |  |  |  |  | 0.07 | 0.00 | 0.00 | 0.00 | 0.00 |  | 1.57 |
| hymn.mid | melody (MIDI) | 1,377 | 221 | 0.03 | 0.05 | 0.02 |  | 0.00 |  |  | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |  |  |
| film.mp4 | video (MP4) | 234,633 | 233,863 | 1.53 | 0.15 | 0.01 | 0.01 | 0.02 | 0.01 | 0.01 | 0.00 | 0.13 | 0.08 | 0.00 | 0.13 | 0.05 |  | 0.85 |
| sound.wav | sound (WAV) | 88,244 | 3,233 | 2.23 | 0.04 | 1.46 | 0.12 | 0.11 | 0.01 | 0.11 | 0.00 | 0.43 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.15 |
| program.exe | program (Windows) | 5,337,607 | 4,223,640 | 22.11 | 0.24 |  |  |  |  |  |  | 1.62 | 0.00 | 0.00 | 0.00 | 0.00 |  | 10.52 |
| folder.tar | tar | 5,806,080 | 1,321,726 | 17.85 | 0.33 |  |  |  |  |  |  | 0.07 | 0.00 | 0.00 | 0.00 | 0.00 |  | 8.42 |
