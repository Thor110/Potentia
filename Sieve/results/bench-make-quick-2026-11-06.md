# Making Sieve instructions: time by stage (2026-11-06)

Intel64 Family 6 Model 167 Stepping 1, GenuineIntel, 12 threads, Windows 10. `sieve locate FILE --installer` (the newest instructions), each file alone; seconds, from `--timings`. Stages nest in the total, and the pieces' rope includes its own search; cutting includes the blocks and the boundaries.

## Search 1 s a file

| File | Kind | Bytes | Made | s | MB/s | rope search | pieces' rope | cutting | blocks | boundaries | rope code | LZMA2 quick | LZMA2 trial | LZMA2 pass 1 | LZMA2 tuned | 7-Zip pass | archives | mixing |
| :--- | :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| prose.md | prose | 514,891 | 128,307 | 3.81 | 0.14 |  |  |  |  |  |  | 0.06 | 0.00 | 0.00 | 0.00 | 0.00 |  | 2.77 |
| code.cpp | code | 174,622 | 30,967 | 1.92 | 0.09 |  |  |  |  |  |  | 0.03 | 0.00 | 0.00 | 0.00 | 0.00 |  | 1.51 |
| oracle.py | code | 492,562 | 97,299 | 4.00 | 0.12 |  |  |  |  |  |  | 0.08 | 0.00 | 0.00 | 0.00 | 0.00 |  | 2.93 |
| table.csv | table | 17,831 | 7,249 | 1.07 | 0.02 | 0.51 | 0.05 | 0.02 | 0.00 | 0.02 | 0.00 | 0.12 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.45 |
| words.txt | word list | 2,921,105 | 287,033 | 16.28 | 0.18 | 0.18 |  | 0.11 | 0.10 |  | 0.12 | 1.26 | 0.54 | 0.00 | 0.00 | 0.00 |  | 6.14 |
| rope.txt | prose | 11,440 | 3,293 | 0.60 | 0.02 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.54 |
| screenshot.png | picture (PNG) | 92,628 | 80,502 | 1.25 | 0.07 | 0.00 | 0.01 | 0.05 | 0.01 | 0.05 | 0.00 | 0.31 | 0.04 | 0.00 | 0.03 | 0.02 |  | 0.88 |
| vitruvian.jpg | picture (JPEG) | 207,500 | 206,946 | 2.04 | 0.10 | 0.01 | 0.01 | 0.02 | 0.01 | 0.01 | 0.00 | 0.14 | 0.00 | 0.00 | 0.00 | 0.00 |  | 1.88 |
| door.jpg | picture (JPEG) | 69,298 | 63,856 | 0.89 | 0.08 | 0.00 |  | 0.00 | 0.00 |  | 0.00 | 0.04 | 0.03 | 0.00 | 0.03 | 0.01 |  | 0.68 |
| spitfire.obj | mesh | 469,717 | 53,144 | 3.71 | 0.13 |  |  |  |  |  |  | 0.07 | 0.00 | 0.00 | 0.00 | 0.00 |  | 2.77 |
| hymn.mid | melody (MIDI) | 1,377 | 221 | 0.03 | 0.05 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 0.01 | 0.00 | 0.00 | 0.00 |  |  |
| film.mp4 | video (MP4) | 234,633 | 233,863 | 2.65 | 0.09 | 0.01 | 0.01 | 0.02 | 0.01 | 0.01 | 0.00 | 0.18 | 0.09 | 0.00 | 0.10 | 0.03 |  | 2.05 |
| sound.wav | sound (WAV) | 88,244 | 3,263 | 2.38 | 0.04 | 1.75 | 0.12 | 0.10 | 0.01 | 0.10 | 0.00 | 0.64 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.30 |
| program.exe | program (Windows) | 5,337,607 | 4,255,565 | 26.59 | 0.20 |  |  |  |  |  |  | 1.43 | 0.00 | 0.00 | 0.00 | 0.00 |  | 17.91 |
| folder.tar | tar | 5,314,560 | 1,213,980 | 17.45 | 0.30 |  |  |  |  |  |  | 0.06 | 0.00 | 0.00 | 0.00 | 0.00 |  | 9.42 |
