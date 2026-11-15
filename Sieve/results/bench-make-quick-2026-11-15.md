# Making Sieve instructions: time by stage (2026-11-15)

Intel64 Family 6 Model 167 Stepping 1, GenuineIntel, 12 threads, Windows 10. `sieve locate FILE --installer` (the newest instructions), each file alone; seconds, from `--timings`. Stages nest in the total, and the pieces' rope includes its own search; cutting includes the blocks and the boundaries.

## Search 1 s a file

| File | Kind | Bytes | Made | s | MB/s | rope search | pieces' rope | cutting | blocks | boundaries | rope code | LZMA2 quick | LZMA2 trial | LZMA2 pass 1 | LZMA2 tuned | 7-Zip pass | archives | mixing |
| :--- | :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| prose.md | prose | 838,955 | 199,787 | 3.81 | 0.22 |  |  |  |  |  |  | 0.06 | 0.00 | 0.00 | 0.00 | 0.00 |  | 2.17 |
| code.cpp | code | 219,842 | 38,362 | 1.40 | 0.16 |  |  |  |  |  |  | 0.05 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.94 |
| oracle.py | code | 535,980 | 104,557 | 3.00 | 0.18 |  |  |  |  |  |  | 0.06 | 0.00 | 0.00 | 0.00 | 0.00 |  | 1.88 |
| table.csv | table | 17,831 | 7,249 | 2.22 | 0.01 | 0.42 | 0.04 | 0.02 | 0.00 | 0.01 | 0.00 | 0.12 | 0.83 | 0.00 | 0.00 | 0.00 |  | 0.59 |
| words.txt | word list | 2,921,105 | 287,033 | 11.94 | 0.24 | 0.23 |  | 0.40 | 0.40 |  | 0.14 | 1.12 | 0.00 | 0.00 | 0.00 | 0.00 |  | 4.03 |
| rope.txt | prose | 11,440 | 3,293 | 1.73 | 0.01 | 0.05 |  | 0.00 |  |  | 0.00 | 0.00 | 0.37 | 0.00 | 0.00 | 0.00 |  | 1.09 |
| screenshot.png | picture (PNG) | 92,628 | 80,502 | 2.58 | 0.04 | 0.00 | 0.02 | 0.55 | 0.13 | 0.42 | 0.00 | 0.39 | 0.44 | 0.00 | 0.07 | 0.03 |  | 0.88 |
| vitruvian.jpg | picture (JPEG) | 207,500 | 206,946 | 1.78 | 0.12 | 0.01 | 0.03 | 0.24 | 0.13 | 0.11 | 0.00 | 0.09 | 0.07 | 0.00 | 0.00 | 0.00 |  | 1.23 |
| door.jpg | picture (JPEG) | 69,298 | 63,856 | 0.93 | 0.07 | 0.00 |  | 0.04 | 0.04 |  | 0.00 | 0.04 | 0.14 | 0.01 | 0.03 | 0.01 |  | 0.48 |
| spitfire.obj | mesh | 469,717 | 53,137 | 3.13 | 0.15 |  |  |  |  |  |  | 0.10 | 0.00 | 0.00 | 0.00 | 0.00 |  | 2.09 |
| hymn.mid | melody (MIDI) | 1,377 | 221 | 0.03 | 0.05 | 0.02 |  | 0.00 |  |  | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |  |  |
| film.mp4 | video (MP4) | 234,633 | 233,863 | 1.57 | 0.15 | 0.01 | 0.01 | 0.02 | 0.01 | 0.01 | 0.00 | 0.16 | 0.08 | 0.00 | 0.12 | 0.05 |  | 0.90 |
| sound.wav | sound (WAV) | 88,244 | 3,233 | 2.21 | 0.04 | 1.46 | 0.11 | 0.10 | 0.01 | 0.10 | 0.00 | 0.61 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.17 |
| program.exe | program (Windows) | 5,337,607 | 4,223,640 | 23.03 | 0.23 |  |  |  |  |  |  | 1.87 | 0.00 | 0.00 | 0.00 | 0.00 |  | 12.17 |
| folder.tar | tar | 5,724,160 | 1,303,024 | 17.75 | 0.32 |  |  |  |  |  |  | 0.07 | 0.00 | 0.00 | 0.00 | 0.00 |  | 8.38 |
