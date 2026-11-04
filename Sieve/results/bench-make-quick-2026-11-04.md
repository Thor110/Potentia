# Making Sieve instructions: time by stage (2026-11-04)

Intel64 Family 6 Model 167 Stepping 1, GenuineIntel, 12 threads, Windows 10. `sieve locate FILE --installer` (the newest instructions), each file alone; seconds, from `--timings`. Stages nest in the total, and the pieces' rope includes its own search; cutting includes the blocks and the boundaries.

## Search 1 s a file

| File | Kind | Bytes | Made | s | MB/s | rope search | pieces' rope | cutting | blocks | boundaries | rope code | LZMA2 quick | LZMA2 trial | LZMA2 pass 1 | LZMA2 tuned | 7-Zip pass | archives | mixing |
| :--- | :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| prose.md | prose | 488,337 | 121,879 | 4.29 | 0.11 |  |  |  |  |  |  | 0.06 | 0.00 | 0.00 | 0.00 | 0.00 |  | 3.35 |
| code.cpp | code | 171,605 | 30,381 | 2.79 | 0.06 |  |  |  |  |  |  | 0.04 | 0.00 | 0.00 | 0.00 | 0.00 |  | 2.37 |
| oracle.py | code | 487,013 | 96,232 | 5.03 | 0.10 |  |  |  |  |  |  | 0.07 | 0.00 | 0.00 | 0.00 | 0.00 |  | 4.04 |
| table.csv | table | 17,831 | 7,249 | 1.14 | 0.02 | 0.53 | 0.05 | 0.02 | 0.00 | 0.02 | 0.00 | 0.13 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.51 |
| words.txt | word list | 2,921,105 | 287,033 | 10.23 | 0.29 | 0.16 |  | 0.10 | 0.10 |  | 0.10 | 1.17 | 0.00 | 0.00 | 0.00 | 0.00 |  | 5.62 |
| rope.txt | prose | 11,440 | 3,293 | 0.53 | 0.02 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.49 |
| screenshot.png | picture (PNG) | 92,628 | 80,502 | 1.25 | 0.07 | 0.00 | 0.01 | 0.04 | 0.01 | 0.03 | 0.00 | 0.33 | 0.03 | 0.00 | 0.03 | 0.01 |  | 0.95 |
| vitruvian.jpg | picture (JPEG) | 207,500 | 206,946 | 2.18 | 0.10 | 0.01 | 0.01 | 0.01 | 0.01 | 0.01 | 0.00 | 0.13 | 0.00 | 0.00 | 0.00 | 0.00 |  | 2.05 |
| door.jpg | picture (JPEG) | 69,298 | 63,856 | 0.93 | 0.07 | 0.00 |  | 0.01 | 0.00 |  | 0.00 | 0.04 | 0.02 | 0.00 | 0.02 | 0.01 |  | 0.76 |
| spitfire.obj | mesh | 469,717 | 53,144 | 3.81 | 0.12 |  |  |  |  |  |  | 0.07 | 0.00 | 0.00 | 0.00 | 0.00 |  | 3.03 |
| hymn.mid | melody (MIDI) | 1,377 | 221 | 0.02 | 0.07 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |  |  |
| film.mp4 | video (MP4) | 234,633 | 233,863 | 2.82 | 0.08 | 0.01 | 0.01 | 0.02 | 0.01 | 0.01 | 0.00 | 0.17 | 0.06 | 0.00 | 0.10 | 0.02 |  | 2.29 |
| sound.wav | sound (WAV) | 88,244 | 3,263 | 2.61 | 0.03 | 1.70 | 0.14 | 0.10 | 0.00 | 0.09 | 0.00 | 0.66 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.55 |
| program.exe | program (Windows) | 5,337,607 | 4,255,565 | 39.58 | 0.13 |  |  |  |  |  |  | 1.51 | 0.00 | 0.00 | 0.00 | 0.00 |  | 29.05 |
| folder.tar | tar | 5,273,600 | 1,205,873 | 20.90 | 0.25 |  |  |  |  |  |  | 0.07 | 0.00 | 0.00 | 0.00 | 0.00 |  | 11.53 |
