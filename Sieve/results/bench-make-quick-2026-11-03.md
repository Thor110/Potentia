# Making Sieve instructions: time by stage (2026-11-03)

Intel64 Family 6 Model 167 Stepping 1, GenuineIntel, 12 threads, Windows 10. `sieve locate FILE --installer` (the newest instructions), each file alone; seconds, from `--timings`. Stages nest in the total, and the pieces' rope includes its own search; cutting includes the blocks and the boundaries.

## Search 1 s a file

| File | Kind | Bytes | Made | s | MB/s | rope search | pieces' rope | cutting | blocks | boundaries | rope code | LZMA2 quick | LZMA2 trial | LZMA2 pass 1 | LZMA2 tuned | 7-Zip pass | archives | mixing |
| :--- | :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| prose.md | prose | 477,546 | 119,201 | 4.27 | 0.11 |  |  |  |  |  |  | 0.06 | 0.00 | 0.00 | 0.00 | 0.00 |  | 3.26 |
| code.cpp | code | 171,086 | 30,242 | 2.60 | 0.07 |  |  |  |  |  |  | 0.04 | 0.00 | 0.00 | 0.00 | 0.00 |  | 2.22 |
| oracle.py | code | 487,013 | 96,232 | 3.89 | 0.13 |  |  |  |  |  |  | 0.06 | 0.00 | 0.00 | 0.00 | 0.00 |  | 3.05 |
| table.csv | table | 17,831 | 7,249 | 1.02 | 0.02 | 0.47 | 0.04 | 0.02 | 0.00 | 0.01 | 0.00 | 0.12 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.46 |
| words.txt | word list | 2,921,105 | 287,033 | 11.16 | 0.26 | 0.16 |  | 0.09 | 0.09 |  | 0.10 | 1.09 | 0.00 | 0.00 | 0.00 | 0.00 |  | 6.29 |
| rope.txt | prose | 11,440 | 3,293 | 0.49 | 0.02 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.46 |
| screenshot.png | picture (PNG) | 92,628 | 80,502 | 1.20 | 0.08 | 0.00 | 0.01 | 0.04 | 0.00 | 0.03 | 0.00 | 0.32 | 0.02 | 0.00 | 0.03 | 0.01 |  | 0.92 |
| vitruvian.jpg | picture (JPEG) | 207,500 | 206,946 | 2.00 | 0.10 | 0.01 | 0.01 | 0.02 | 0.01 | 0.01 | 0.00 | 0.13 | 0.00 | 0.00 | 0.00 | 0.00 |  | 1.88 |
| door.jpg | picture (JPEG) | 69,298 | 63,856 | 0.96 | 0.07 | 0.00 |  | 0.00 | 0.00 |  | 0.00 | 0.05 | 0.02 | 0.00 | 0.04 | 0.01 |  | 0.74 |
| spitfire.obj | mesh | 469,717 | 53,144 | 3.74 | 0.13 |  |  |  |  |  |  | 0.08 | 0.00 | 0.00 | 0.00 | 0.00 |  | 2.93 |
| hymn.mid | melody (MIDI) | 1,377 | 221 | 0.02 | 0.07 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |  |  |
| film.mp4 | video (MP4) | 234,633 | 233,863 | 2.73 | 0.09 | 0.01 | 0.02 | 0.02 | 0.01 | 0.01 | 0.00 | 0.18 | 0.06 | 0.00 | 0.07 | 0.02 |  | 2.24 |
| sound.wav | sound (WAV) | 88,244 | 3,263 | 2.33 | 0.04 | 1.52 | 0.13 | 0.09 | 0.00 | 0.09 | 0.00 | 0.65 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.46 |
| program.exe | program (Windows) | 5,337,607 | 4,255,565 | 36.10 | 0.15 |  |  |  |  |  |  | 1.32 | 0.00 | 0.00 | 0.00 | 0.00 |  | 26.26 |
| folder.tar | tar | 5,263,360 | 1,203,037 | 21.31 | 0.25 |  |  |  |  |  |  | 0.08 | 0.00 | 0.00 | 0.00 | 0.00 |  | 11.76 |
