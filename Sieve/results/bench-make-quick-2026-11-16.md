# Making Sieve instructions: time by stage (2026-11-16)

Intel64 Family 6 Model 167 Stepping 1, GenuineIntel, 12 threads, Windows 10. `sieve locate FILE --installer` (the newest instructions), each file alone; seconds, from `--timings`. Stages nest in the total, and the pieces' rope includes its own search; cutting includes the blocks and the boundaries.

## Search 1 s a file

| File | Kind | Bytes | Made | s | MB/s | rope search | pieces' rope | cutting | blocks | boundaries | rope code | LZMA2 quick | LZMA2 trial | LZMA2 pass 1 | LZMA2 tuned | 7-Zip pass | archives | mixing |
| :--- | :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| prose.md | prose | 849,102 | 201,959 | 3.91 | 0.22 |  |  |  |  |  |  | 0.07 | 0.00 | 0.00 | 0.00 | 0.00 |  | 2.30 |
| code.cpp | code | 225,997 | 39,282 | 1.54 | 0.15 |  |  |  |  |  |  | 0.05 | 0.00 | 0.00 | 0.00 | 0.00 |  | 1.02 |
| oracle.py | code | 535,980 | 104,557 | 2.87 | 0.19 |  |  |  |  |  |  | 0.06 | 0.00 | 0.00 | 0.00 | 0.00 |  | 1.80 |
| table.csv | table | 17,831 | 7,249 | 0.87 | 0.02 | 0.42 | 0.04 | 0.02 | 0.00 | 0.01 | 0.00 | 0.12 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.33 |
| words.txt | word list | 2,921,105 | 287,033 | 11.54 | 0.25 | 0.42 |  | 0.61 | 0.60 |  | 0.26 | 1.30 | 0.00 | 0.00 | 0.00 | 0.00 |  | 4.02 |
| rope.txt | prose | 11,440 | 3,293 | 0.43 | 0.03 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.38 |
| screenshot.png | picture (PNG) | 92,628 | 80,502 | 1.10 | 0.08 | 0.00 | 0.02 | 0.08 | 0.01 | 0.07 | 0.00 | 0.32 | 0.06 | 0.00 | 0.05 | 0.02 |  | 0.58 |
| vitruvian.jpg | picture (JPEG) | 207,500 | 206,946 | 2.11 | 0.10 | 0.01 | 0.01 | 0.05 | 0.02 | 0.02 | 0.00 | 0.07 | 0.44 | 0.00 | 0.00 | 0.00 |  | 1.30 |
| door.jpg | picture (JPEG) | 69,298 | 63,856 | 1.30 | 0.05 | 0.00 |  | 0.02 | 0.02 |  | 0.00 | 0.04 | 0.17 | 0.00 | 0.03 | 0.01 |  | 0.70 |
| spitfire.obj | mesh | 469,717 | 53,137 | 3.61 | 0.13 |  |  |  |  |  |  | 0.11 | 0.03 | 0.00 | 0.00 | 0.00 |  | 2.39 |
| hymn.mid | melody (MIDI) | 1,377 | 221 | 0.03 | 0.05 | 0.02 |  | 0.00 |  |  | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |  |  |
| film.mp4 | video (MP4) | 234,633 | 233,863 | 1.78 | 0.13 | 0.01 | 0.01 | 0.03 | 0.01 | 0.01 | 0.00 | 0.14 | 0.11 | 0.00 | 0.14 | 0.05 |  | 1.02 |
| sound.wav | sound (WAV) | 88,244 | 3,233 | 2.55 | 0.03 | 1.61 | 0.16 | 0.14 | 0.01 | 0.13 | 0.00 | 0.51 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.20 |
| program.exe | program (Windows) | 5,337,607 | 4,223,640 | 24.33 | 0.22 |  |  |  |  |  |  | 2.09 | 0.00 | 0.00 | 0.00 | 0.00 |  | 13.21 |
| folder.tar | tar | 5,744,640 | 1,306,150 | 17.86 | 0.32 |  |  |  |  |  |  | 0.07 | 0.00 | 0.00 | 0.00 | 0.00 |  | 8.46 |
