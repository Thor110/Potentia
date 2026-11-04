# Making Sieve instructions: time by stage (2026-11-04)

Intel64 Family 6 Model 167 Stepping 1, GenuineIntel, 12 threads, Windows 10. `sieve locate FILE --installer` (the newest instructions), each file alone; seconds, from `--timings`. Stages nest in the total, and the pieces' rope includes its own search; cutting includes the blocks and the boundaries.

## Search 1 s a file

| File | Kind | Bytes | Made | s | MB/s | rope search | pieces' rope | cutting | blocks | boundaries | rope code | LZMA2 quick | LZMA2 trial | LZMA2 pass 1 | LZMA2 tuned | 7-Zip pass | archives | mixing |
| :--- | :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| prose.md | prose | 488,140 | 121,826 | 4.18 | 0.12 |  |  |  |  |  |  | 0.06 | 0.00 | 0.00 | 0.00 | 0.00 |  | 3.25 |
| code.cpp | code | 171,395 | 30,324 | 2.43 | 0.07 |  |  |  |  |  |  | 0.03 | 0.00 | 0.00 | 0.00 | 0.00 |  | 2.06 |
| oracle.py | code | 487,013 | 96,232 | 4.11 | 0.12 |  |  |  |  |  |  | 0.07 | 0.00 | 0.00 | 0.00 | 0.00 |  | 3.22 |
| table.csv | table | 17,831 | 7,249 | 1.05 | 0.02 | 0.48 | 0.05 | 0.02 | 0.00 | 0.02 | 0.00 | 0.13 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.47 |
| words.txt | word list | 2,921,105 | 287,033 | 10.13 | 0.29 | 0.16 |  | 0.10 | 0.09 |  | 0.11 | 1.13 | 0.00 | 0.00 | 0.00 | 0.00 |  | 5.62 |
| rope.txt | prose | 11,440 | 3,293 | 0.46 | 0.03 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.42 |
| screenshot.png | picture (PNG) | 92,628 | 80,502 | 1.29 | 0.07 | 0.00 | 0.01 | 0.04 | 0.01 | 0.04 | 0.00 | 0.34 | 0.03 | 0.00 | 0.03 | 0.01 |  | 0.99 |
| vitruvian.jpg | picture (JPEG) | 207,500 | 206,946 | 2.12 | 0.10 | 0.01 | 0.01 | 0.01 | 0.01 | 0.01 | 0.00 | 0.12 | 0.00 | 0.00 | 0.00 | 0.00 |  | 2.00 |
| door.jpg | picture (JPEG) | 69,298 | 63,856 | 0.90 | 0.08 | 0.00 |  | 0.00 | 0.00 |  | 0.00 | 0.04 | 0.02 | 0.00 | 0.02 | 0.01 |  | 0.74 |
| spitfire.obj | mesh | 469,717 | 53,144 | 3.73 | 0.13 |  |  |  |  |  |  | 0.07 | 0.00 | 0.00 | 0.00 | 0.00 |  | 2.98 |
| hymn.mid | melody (MIDI) | 1,377 | 221 | 0.02 | 0.07 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |  |  |
| film.mp4 | video (MP4) | 234,633 | 233,863 | 2.75 | 0.09 | 0.01 | 0.01 | 0.02 | 0.01 | 0.01 | 0.00 | 0.17 | 0.06 | 0.00 | 0.08 | 0.03 |  | 2.23 |
| sound.wav | sound (WAV) | 88,244 | 3,263 | 2.51 | 0.04 | 1.68 | 0.12 | 0.09 | 0.00 | 0.09 | 0.00 | 0.63 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.50 |
| program.exe | program (Windows) | 5,337,607 | 4,255,565 | 36.80 | 0.15 |  |  |  |  |  |  | 1.46 | 0.00 | 0.00 | 0.00 | 0.00 |  | 26.55 |
| folder.tar | tar | 5,273,600 | 1,205,763 | 21.66 | 0.24 |  |  |  |  |  |  | 0.07 | 0.00 | 0.00 | 0.00 | 0.00 |  | 12.49 |
