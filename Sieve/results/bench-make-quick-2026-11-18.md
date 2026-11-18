# Making Sieve instructions: time by stage (2026-11-18)

Intel64 Family 6 Model 167 Stepping 1, GenuineIntel, 12 threads, Windows 10. `sieve locate FILE --installer` (the newest instructions), each file alone; seconds, from `--timings`. Stages nest in the total, and the pieces' rope includes its own search; cutting includes the blocks and the boundaries.

## Search 1 s a file

| File | Kind | Bytes | Made | s | MB/s | rope search | pieces' rope | cutting | blocks | boundaries | rope code | LZMA2 quick | LZMA2 trial | LZMA2 pass 1 | LZMA2 tuned | 7-Zip pass | archives | mixing |
| :--- | :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| prose.md | prose | 885,003 | 210,562 | 3.76 | 0.24 |  |  |  |  |  |  | 0.07 | 0.00 | 0.00 | 0.00 | 0.00 |  | 2.11 |
| code.cpp | code | 233,849 | 40,542 | 1.43 | 0.16 |  |  |  |  |  |  | 0.06 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.92 |
| oracle.py | code | 535,980 | 104,557 | 5.61 | 0.10 |  |  |  |  |  |  | 0.07 | 0.75 | 0.00 | 0.00 | 0.00 |  | 2.05 |
| table.csv | table | 17,831 | 7,249 | 0.90 | 0.02 | 0.45 | 0.05 | 0.02 | 0.00 | 0.02 | 0.00 | 0.12 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.35 |
| words.txt | word list | 2,921,105 | 287,033 | 17.26 | 0.17 | 0.14 |  | 0.07 | 0.07 |  | 0.10 | 0.67 | 0.26 | 0.00 | 0.00 | 0.00 |  | 5.78 |
| rope.txt | prose | 11,440 | 3,293 | 1.25 | 0.01 | 0.02 |  | 0.00 |  |  | 0.00 | 0.00 | 0.19 | 0.00 | 0.00 | 0.00 |  | 0.84 |
| screenshot.png | picture (PNG) | 92,628 | 80,502 | 1.23 | 0.08 | 0.00 | 0.02 | 0.12 | 0.02 | 0.10 | 0.00 | 0.36 | 0.08 | 0.00 | 0.06 | 0.02 |  | 0.60 |
| vitruvian.jpg | picture (JPEG) | 207,500 | 206,946 | 1.02 | 0.20 | 0.01 | 0.01 | 0.01 | 0.01 | 0.01 | 0.00 | 0.06 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.85 |
| door.jpg | picture (JPEG) | 69,298 | 63,856 | 0.66 | 0.10 | 0.00 |  | 0.00 | 0.00 |  | 0.00 | 0.03 | 0.03 | 0.00 | 0.03 | 0.01 |  | 0.41 |
| spitfire.obj | mesh | 469,717 | 53,137 | 2.67 | 0.18 |  |  |  |  |  |  | 0.08 | 0.00 | 0.00 | 0.00 | 0.00 |  | 1.78 |
| hymn.mid | melody (MIDI) | 1,377 | 221 | 0.03 | 0.05 | 0.02 |  | 0.00 |  |  | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |  |  |
| film.mp4 | video (MP4) | 234,633 | 233,863 | 1.48 | 0.16 | 0.01 | 0.01 | 0.01 | 0.01 | 0.01 | 0.00 | 0.13 | 0.07 | 0.00 | 0.10 | 0.03 |  | 0.86 |
| sound.wav | sound (WAV) | 88,244 | 3,233 | 2.02 | 0.04 | 1.32 | 0.13 | 0.08 | 0.00 | 0.08 | 0.00 | 0.48 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.15 |
| program.exe | program (Windows) | 5,337,607 | 4,223,640 | 22.79 | 0.23 |  |  |  |  |  |  | 1.61 | 0.02 | 0.00 | 0.00 | 0.00 |  | 10.81 |
| folder.tar | tar | 5,785,600 | 1,316,012 | 17.41 | 0.33 |  |  |  |  |  |  | 0.06 | 0.00 | 0.00 | 0.00 | 0.00 |  | 8.29 |
