# Making Sieve instructions: time by stage (2026-11-13)

Intel64 Family 6 Model 167 Stepping 1, GenuineIntel, 12 threads, Windows 10. `sieve locate FILE --installer` (the newest instructions), each file alone; seconds, from `--timings`. Stages nest in the total, and the pieces' rope includes its own search; cutting includes the blocks and the boundaries.

## Search 1 s a file

| File | Kind | Bytes | Made | s | MB/s | rope search | pieces' rope | cutting | blocks | boundaries | rope code | LZMA2 quick | LZMA2 trial | LZMA2 pass 1 | LZMA2 tuned | 7-Zip pass | archives | mixing |
| :--- | :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| prose.md | prose | 655,081 | 158,313 | 3.04 | 0.22 |  |  |  |  |  |  | 0.07 | 0.00 | 0.00 | 0.00 | 0.00 |  | 1.85 |
| code.cpp | code | 210,508 | 36,722 | 1.40 | 0.15 |  |  |  |  |  |  | 0.05 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.92 |
| oracle.py | code | 535,980 | 104,557 | 4.70 | 0.11 |  |  |  |  |  |  | 0.08 | 0.48 | 0.00 | 0.00 | 0.00 |  | 1.98 |
| table.csv | table | 17,831 | 7,249 | 1.23 | 0.01 | 0.75 | 0.06 | 0.02 | 0.00 | 0.02 | 0.00 | 0.14 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.36 |
| words.txt | word list | 2,921,105 | 287,033 | 13.00 | 0.22 | 0.15 |  | 0.10 | 0.10 |  | 0.10 | 1.17 | 0.21 | 0.00 | 0.00 | 0.00 |  | 4.67 |
| rope.txt | prose | 11,440 | 3,293 | 1.36 | 0.01 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 0.23 | 0.00 | 0.00 | 0.00 |  | 0.85 |
| screenshot.png | picture (PNG) | 92,628 | 80,502 | 2.72 | 0.03 | 0.02 | 0.02 | 0.50 | 0.09 | 0.41 | 0.00 | 0.41 | 0.37 | 0.01 | 0.11 | 0.02 |  | 1.05 |
| vitruvian.jpg | picture (JPEG) | 207,500 | 206,946 | 1.45 | 0.14 | 0.01 | 0.01 | 0.24 | 0.08 | 0.15 | 0.00 | 0.09 | 0.01 | 0.00 | 0.00 | 0.00 |  | 1.02 |
| door.jpg | picture (JPEG) | 69,298 | 63,856 | 0.86 | 0.08 | 0.00 |  | 0.01 | 0.01 |  | 0.00 | 0.04 | 0.09 | 0.00 | 0.04 | 0.01 |  | 0.41 |
| spitfire.obj | mesh | 469,717 | 53,137 | 3.41 | 0.14 |  |  |  |  |  |  | 0.12 | 0.00 | 0.00 | 0.00 | 0.00 |  | 2.27 |
| hymn.mid | melody (MIDI) | 1,377 | 221 | 0.03 | 0.05 | 0.02 |  | 0.00 |  |  | 0.00 | 0.00 | 0.01 | 0.00 | 0.00 | 0.00 |  |  |
| film.mp4 | video (MP4) | 234,633 | 233,863 | 1.75 | 0.13 | 0.01 | 0.01 | 0.02 | 0.01 | 0.01 | 0.00 | 0.23 | 0.08 | 0.00 | 0.14 | 0.05 |  | 1.03 |
| sound.wav | sound (WAV) | 88,244 | 3,233 | 2.37 | 0.04 | 1.56 | 0.14 | 0.10 | 0.00 | 0.10 | 0.00 | 0.64 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.17 |
| program.exe | program (Windows) | 5,337,607 | 4,223,640 | 24.16 | 0.22 |  |  |  |  |  |  | 1.97 | 0.00 | 0.00 | 0.00 | 0.00 |  | 12.51 |
| folder.tar | tar | 5,529,600 | 1,259,476 | 17.12 | 0.32 |  |  |  |  |  |  | 0.08 | 0.00 | 0.00 | 0.00 | 0.00 |  | 8.29 |
