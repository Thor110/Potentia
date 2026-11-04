# Making Sieve instructions: time by stage (2026-11-04)

Intel64 Family 6 Model 167 Stepping 1, GenuineIntel, 12 threads, Windows 10. `sieve locate FILE --installer` (the newest instructions), each file alone; seconds, from `--timings`. Stages nest in the total, and the pieces' rope includes its own search; cutting includes the blocks and the boundaries.

## Search 1 s a file

| File | Kind | Bytes | Made | s | MB/s | rope search | pieces' rope | cutting | blocks | boundaries | rope code | LZMA2 quick | LZMA2 trial | LZMA2 pass 1 | LZMA2 tuned | 7-Zip pass | archives | mixing |
| :--- | :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| prose.md | prose | 497,319 | 124,099 | 4.12 | 0.12 |  |  |  |  |  |  | 0.07 | 0.00 | 0.00 | 0.00 | 0.00 |  | 3.21 |
| code.cpp | code | 171,605 | 30,381 | 2.29 | 0.07 |  |  |  |  |  |  | 0.03 | 0.00 | 0.00 | 0.00 | 0.00 |  | 1.92 |
| oracle.py | code | 487,013 | 96,232 | 3.97 | 0.12 |  |  |  |  |  |  | 0.06 | 0.00 | 0.00 | 0.00 | 0.00 |  | 3.08 |
| table.csv | table | 17,831 | 7,249 | 1.04 | 0.02 | 0.46 | 0.05 | 0.02 | 0.00 | 0.02 | 0.00 | 0.12 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.49 |
| words.txt | word list | 2,921,105 | 287,033 | 10.27 | 0.28 | 0.15 |  | 0.11 | 0.10 |  | 0.11 | 1.26 | 0.00 | 0.00 | 0.00 | 0.00 |  | 5.58 |
| rope.txt | prose | 11,440 | 3,293 | 0.46 | 0.02 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.43 |
| screenshot.png | picture (PNG) | 92,628 | 80,502 | 1.24 | 0.07 | 0.00 | 0.01 | 0.04 | 0.01 | 0.04 | 0.00 | 0.33 | 0.03 | 0.00 | 0.03 | 0.01 |  | 0.94 |
| vitruvian.jpg | picture (JPEG) | 207,500 | 206,946 | 2.15 | 0.10 | 0.01 | 0.01 | 0.01 | 0.01 | 0.01 | 0.00 | 0.11 | 0.00 | 0.00 | 0.00 | 0.00 |  | 2.02 |
| door.jpg | picture (JPEG) | 69,298 | 63,856 | 0.96 | 0.07 | 0.00 |  | 0.00 | 0.00 |  | 0.00 | 0.04 | 0.02 | 0.00 | 0.02 | 0.01 |  | 0.80 |
| spitfire.obj | mesh | 469,717 | 53,144 | 3.82 | 0.12 |  |  |  |  |  |  | 0.07 | 0.00 | 0.00 | 0.00 | 0.00 |  | 3.02 |
| hymn.mid | melody (MIDI) | 1,377 | 221 | 0.02 | 0.07 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |  |  |
| film.mp4 | video (MP4) | 234,633 | 233,863 | 2.82 | 0.08 | 0.01 | 0.01 | 0.02 | 0.01 | 0.01 | 0.00 | 0.18 | 0.07 | 0.00 | 0.08 | 0.02 |  | 2.31 |
| sound.wav | sound (WAV) | 88,244 | 3,263 | 2.35 | 0.04 | 1.55 | 0.12 | 0.10 | 0.00 | 0.09 | 0.00 | 0.65 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.47 |
| program.exe | program (Windows) | 5,337,607 | 4,255,565 | 36.47 | 0.15 |  |  |  |  |  |  | 1.47 | 0.00 | 0.00 | 0.00 | 0.00 |  | 26.11 |
| folder.tar | tar | 5,283,840 | 1,208,094 | 20.88 | 0.25 |  |  |  |  |  |  | 0.07 | 0.00 | 0.00 | 0.00 | 0.00 |  | 11.62 |
