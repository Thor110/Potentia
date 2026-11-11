# Making Sieve instructions: time by stage (2026-11-11)

Intel64 Family 6 Model 167 Stepping 1, GenuineIntel, 12 threads, Windows 10. `sieve locate FILE --installer` (the newest instructions), each file alone; seconds, from `--timings`. Stages nest in the total, and the pieces' rope includes its own search; cutting includes the blocks and the boundaries.

## Search 1 s a file

| File | Kind | Bytes | Made | s | MB/s | rope search | pieces' rope | cutting | blocks | boundaries | rope code | LZMA2 quick | LZMA2 trial | LZMA2 pass 1 | LZMA2 tuned | 7-Zip pass | archives | mixing |
| :--- | :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| prose.md | prose | 558,540 | 138,492 | 5.17 | 0.11 |  |  |  |  |  |  | 0.08 | 0.00 | 0.00 | 0.00 | 0.00 |  | 3.81 |
| code.cpp | code | 178,012 | 31,382 | 1.27 | 0.14 |  |  |  |  |  |  | 0.06 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.83 |
| oracle.py | code | 534,973 | 104,336 | 2.65 | 0.20 |  |  |  |  |  |  | 0.06 | 0.00 | 0.00 | 0.00 | 0.00 |  | 1.72 |
| table.csv | table | 17,831 | 7,249 | 0.94 | 0.02 | 0.43 | 0.04 | 0.02 | 0.00 | 0.02 | 0.00 | 0.12 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.41 |
| words.txt | word list | 2,921,105 | 287,033 | 17.46 | 0.17 | 0.36 |  | 0.32 | 0.31 |  | 0.24 | 1.61 | 0.01 | 0.00 | 0.00 | 0.00 |  | 10.11 |
| rope.txt | prose | 11,440 | 3,293 | 0.46 | 0.03 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.41 |
| screenshot.png | picture (PNG) | 92,628 | 80,502 | 0.85 | 0.11 | 0.00 | 0.02 | 0.05 | 0.01 | 0.05 | 0.00 | 0.34 | 0.03 | 0.00 | 0.04 | 0.02 |  | 0.47 |
| vitruvian.jpg | picture (JPEG) | 207,500 | 206,946 | 1.00 | 0.21 | 0.01 | 0.01 | 0.02 | 0.01 | 0.01 | 0.00 | 0.13 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.84 |
| door.jpg | picture (JPEG) | 69,298 | 63,856 | 0.57 | 0.12 | 0.00 |  | 0.00 | 0.00 |  | 0.00 | 0.04 | 0.02 | 0.00 | 0.03 | 0.01 |  | 0.34 |
| spitfire.obj | mesh | 469,717 | 53,137 | 2.42 | 0.19 |  |  |  |  |  |  | 0.06 | 0.00 | 0.00 | 0.00 | 0.00 |  | 1.61 |
| hymn.mid | melody (MIDI) | 1,377 | 221 | 0.03 | 0.05 | 0.02 |  | 0.00 |  |  | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |  |  |
| film.mp4 | video (MP4) | 234,633 | 233,863 | 1.52 | 0.15 | 0.01 | 0.01 | 0.02 | 0.01 | 0.01 | 0.00 | 0.21 | 0.08 | 0.00 | 0.10 | 0.04 |  | 0.87 |
| sound.wav | sound (WAV) | 88,244 | 3,233 | 2.05 | 0.04 | 1.53 | 0.16 | 0.10 | 0.00 | 0.10 | 0.00 | 0.62 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.15 |
| program.exe | program (Windows) | 5,337,607 | 4,255,565 | 22.54 | 0.24 |  |  |  |  |  |  | 1.80 | 0.00 | 0.00 | 0.00 | 0.00 |  | 14.75 |
| folder.tar | tar | 5,396,480 | 1,233,575 | 16.22 | 0.33 |  |  |  |  |  |  | 0.06 | 0.00 | 0.00 | 0.00 | 0.00 |  | 7.90 |
