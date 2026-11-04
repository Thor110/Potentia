# Making Sieve instructions: time by stage (2026-11-04)

Intel64 Family 6 Model 167 Stepping 1, GenuineIntel, 12 threads, Windows 10. `sieve locate FILE --installer` (the newest instructions), each file alone; seconds, from `--timings`. Stages nest in the total, and the pieces' rope includes its own search; cutting includes the blocks and the boundaries.

## Search 1 s a file

| File | Kind | Bytes | Made | s | MB/s | rope search | pieces' rope | cutting | blocks | boundaries | rope code | LZMA2 quick | LZMA2 trial | LZMA2 pass 1 | LZMA2 tuned | 7-Zip pass | archives | mixing |
| :--- | :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| prose.md | prose | 489,031 | 122,065 | 4.59 | 0.11 |  |  |  |  |  |  | 0.07 | 0.00 | 0.00 | 0.00 | 0.00 |  | 3.58 |
| code.cpp | code | 171,605 | 30,381 | 2.65 | 0.06 |  |  |  |  |  |  | 0.03 | 0.00 | 0.00 | 0.00 | 0.00 |  | 2.22 |
| oracle.py | code | 487,013 | 96,232 | 4.13 | 0.12 |  |  |  |  |  |  | 0.07 | 0.00 | 0.00 | 0.00 | 0.00 |  | 3.18 |
| table.csv | table | 17,831 | 7,249 | 1.06 | 0.02 | 0.52 | 0.04 | 0.02 | 0.00 | 0.02 | 0.00 | 0.13 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.45 |
| words.txt | word list | 2,921,105 | 287,033 | 9.80 | 0.30 | 0.14 |  | 0.09 | 0.09 |  | 0.11 | 1.10 | 0.00 | 0.00 | 0.00 | 0.00 |  | 5.39 |
| rope.txt | prose | 11,440 | 3,293 | 0.49 | 0.02 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.45 |
| screenshot.png | picture (PNG) | 92,628 | 80,502 | 1.29 | 0.07 | 0.00 | 0.01 | 0.05 | 0.01 | 0.04 | 0.00 | 0.35 | 0.03 | 0.00 | 0.03 | 0.01 |  | 0.98 |
| vitruvian.jpg | picture (JPEG) | 207,500 | 206,946 | 2.13 | 0.10 | 0.01 | 0.01 | 0.02 | 0.01 | 0.01 | 0.00 | 0.13 | 0.00 | 0.00 | 0.00 | 0.00 |  | 1.99 |
| door.jpg | picture (JPEG) | 69,298 | 63,856 | 0.83 | 0.08 | 0.00 |  | 0.00 | 0.00 |  | 0.00 | 0.04 | 0.02 | 0.00 | 0.02 | 0.01 |  | 0.67 |
| spitfire.obj | mesh | 469,717 | 53,144 | 3.57 | 0.13 |  |  |  |  |  |  | 0.06 | 0.00 | 0.00 | 0.00 | 0.00 |  | 2.81 |
| hymn.mid | melody (MIDI) | 1,377 | 221 | 0.02 | 0.07 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |  |  |
| film.mp4 | video (MP4) | 234,633 | 233,863 | 2.85 | 0.08 | 0.01 | 0.01 | 0.02 | 0.01 | 0.01 | 0.00 | 0.16 | 0.06 | 0.00 | 0.09 | 0.02 |  | 2.36 |
| sound.wav | sound (WAV) | 88,244 | 3,263 | 2.18 | 0.04 | 1.48 | 0.11 | 0.09 | 0.00 | 0.08 | 0.00 | 0.61 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.40 |
| program.exe | program (Windows) | 5,337,607 | 4,255,565 | 36.11 | 0.15 |  |  |  |  |  |  | 1.36 | 0.00 | 0.00 | 0.00 | 0.00 |  | 26.47 |
| folder.tar | tar | 5,273,600 | 1,206,057 | 22.33 | 0.24 |  |  |  |  |  |  | 0.09 | 0.00 | 0.00 | 0.00 | 0.00 |  | 12.53 |
