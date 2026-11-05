# Making Sieve instructions: time by stage (2026-11-05)

Intel64 Family 6 Model 167 Stepping 1, GenuineIntel, 12 threads, Windows 10. `sieve locate FILE --installer` (the newest instructions), each file alone; seconds, from `--timings`. Stages nest in the total, and the pieces' rope includes its own search; cutting includes the blocks and the boundaries.

## Search 1 s a file

| File | Kind | Bytes | Made | s | MB/s | rope search | pieces' rope | cutting | blocks | boundaries | rope code | LZMA2 quick | LZMA2 trial | LZMA2 pass 1 | LZMA2 tuned | 7-Zip pass | archives | mixing |
| :--- | :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| prose.md | prose | 500,553 | 124,980 | 4.03 | 0.12 |  |  |  |  |  |  | 0.07 | 0.00 | 0.00 | 0.00 | 0.00 |  | 2.98 |
| code.cpp | code | 171,965 | 30,411 | 2.21 | 0.08 |  |  |  |  |  |  | 0.04 | 0.00 | 0.00 | 0.00 | 0.00 |  | 1.75 |
| oracle.py | code | 487,013 | 96,232 | 4.22 | 0.12 |  |  |  |  |  |  | 0.08 | 0.00 | 0.00 | 0.00 | 0.00 |  | 3.07 |
| table.csv | table | 17,831 | 7,249 | 1.12 | 0.02 | 0.55 | 0.05 | 0.02 | 0.00 | 0.02 | 0.00 | 0.13 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.46 |
| words.txt | word list | 2,921,105 | 287,033 | 12.37 | 0.24 | 0.26 |  | 0.11 | 0.11 |  | 0.12 | 1.29 | 0.00 | 0.00 | 0.00 | 0.00 |  | 5.71 |
| rope.txt | prose | 11,440 | 3,293 | 1.51 | 0.01 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 0.35 | 0.00 | 0.00 | 0.00 |  | 0.93 |
| screenshot.png | picture (PNG) | 92,628 | 80,502 | 3.83 | 0.02 | 0.00 | 0.02 | 0.62 | 0.14 | 0.48 | 0.00 | 0.39 | 0.51 | 0.00 | 0.08 | 0.02 |  | 1.89 |
| vitruvian.jpg | picture (JPEG) | 207,500 | 206,946 | 3.46 | 0.06 | 0.01 | 0.02 | 0.37 | 0.16 | 0.21 | 0.00 | 0.13 | 0.01 | 0.00 | 0.00 | 0.00 |  | 2.84 |
| door.jpg | picture (JPEG) | 69,298 | 63,856 | 0.97 | 0.07 | 0.00 |  | 0.01 | 0.01 |  | 0.00 | 0.04 | 0.03 | 0.00 | 0.03 | 0.01 |  | 0.74 |
| spitfire.obj | mesh | 469,717 | 53,144 | 4.20 | 0.11 |  |  |  |  |  |  | 0.08 | 0.00 | 0.00 | 0.00 | 0.00 |  | 3.21 |
| hymn.mid | melody (MIDI) | 1,377 | 221 | 0.03 | 0.04 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 0.01 | 0.00 | 0.00 | 0.00 |  |  |
| film.mp4 | video (MP4) | 234,633 | 233,863 | 2.88 | 0.08 | 0.01 | 0.01 | 0.02 | 0.01 | 0.01 | 0.00 | 0.23 | 0.09 | 0.00 | 0.12 | 0.04 |  | 2.22 |
| sound.wav | sound (WAV) | 88,244 | 3,263 | 2.59 | 0.03 | 1.84 | 0.16 | 0.11 | 0.00 | 0.10 | 0.00 | 0.66 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.34 |
| program.exe | program (Windows) | 5,337,607 | 4,255,565 | 28.37 | 0.19 |  |  |  |  |  |  | 1.60 | 0.00 | 0.00 | 0.00 | 0.00 |  | 19.17 |
| folder.tar | tar | 5,294,080 | 1,209,004 | 18.97 | 0.28 |  |  |  |  |  |  | 0.07 | 0.00 | 0.00 | 0.00 | 0.00 |  | 10.21 |
