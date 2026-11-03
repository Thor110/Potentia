# Making Sieve instructions: time by stage (2026-11-03)

Intel64 Family 6 Model 167 Stepping 1, GenuineIntel, 12 threads, Windows 10. `sieve locate FILE --installer` (the newest instructions), each file alone; seconds, from `--timings`. Stages nest in the total, and the pieces' rope includes its own search; cutting includes the blocks and the boundaries.

## Search 1 s a file

| File | Kind | Bytes | Made | s | MB/s | rope search | pieces' rope | cutting | blocks | boundaries | rope code | LZMA2 quick | LZMA2 trial | LZMA2 pass 1 | LZMA2 tuned | 7-Zip pass | archives | mixing |
| :--- | :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| prose.md | prose | 477,546 | 119,201 | 4.20 | 0.11 |  |  |  |  |  |  | 0.06 | 0.00 | 0.00 | 0.00 | 0.00 |  | 3.25 |
| code.cpp | code | 171,045 | 30,226 | 2.69 | 0.06 |  |  |  |  |  |  | 0.04 | 0.00 | 0.00 | 0.00 | 0.00 |  | 2.27 |
| oracle.py | code | 485,429 | 95,852 | 4.58 | 0.11 |  |  |  |  |  |  | 0.12 | 0.00 | 0.00 | 0.00 | 0.00 |  | 3.57 |
| table.csv | table | 17,831 | 7,249 | 1.06 | 0.02 | 0.49 | 0.04 | 0.02 | 0.00 | 0.02 | 0.00 | 0.13 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.47 |
| words.txt | word list | 2,921,105 | 287,033 | 9.78 | 0.30 | 0.18 |  | 0.10 | 0.09 |  | 0.11 | 1.14 | 0.00 | 0.00 | 0.00 | 0.00 |  | 5.40 |
| rope.txt | prose | 11,440 | 3,293 | 0.46 | 0.02 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.43 |
| screenshot.png | picture (PNG) | 92,628 | 80,502 | 1.30 | 0.07 | 0.00 | 0.01 | 0.04 | 0.00 | 0.04 | 0.00 | 0.33 | 0.03 | 0.00 | 0.03 | 0.01 |  | 1.00 |
| vitruvian.jpg | picture (JPEG) | 207,500 | 206,946 | 2.07 | 0.10 | 0.01 | 0.01 | 0.01 | 0.01 | 0.01 | 0.00 | 0.11 | 0.00 | 0.00 | 0.00 | 0.00 |  | 1.94 |
| door.jpg | picture (JPEG) | 69,298 | 63,856 | 0.85 | 0.08 | 0.00 |  | 0.00 | 0.00 |  | 0.00 | 0.04 | 0.02 | 0.00 | 0.02 | 0.01 |  | 0.69 |
| spitfire.obj | mesh | 469,717 | 53,144 | 3.56 | 0.13 |  |  |  |  |  |  | 0.06 | 0.00 | 0.00 | 0.00 | 0.00 |  | 2.83 |
| hymn.mid | melody (MIDI) | 1,377 | 221 | 0.02 | 0.07 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |  |  |
| film.mp4 | video (MP4) | 234,633 | 233,863 | 2.63 | 0.09 | 0.01 | 0.01 | 0.02 | 0.01 | 0.01 | 0.00 | 0.17 | 0.06 | 0.00 | 0.07 | 0.02 |  | 2.15 |
| sound.wav | sound (WAV) | 88,244 | 3,263 | 2.31 | 0.04 | 1.54 | 0.11 | 0.09 | 0.00 | 0.09 | 0.00 | 0.61 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.46 |
| program.exe | program (Windows) | 5,337,607 | 4,255,565 | 23.56 | 0.23 |  |  |  |  |  |  | 1.38 | 0.30 | 0.00 | 0.00 | 0.00 |  | 9.52 |
| folder.tar | tar | 5,263,360 | 1,202,646 | 19.03 | 0.28 |  |  |  |  |  |  | 0.07 | 0.00 | 0.00 | 0.00 | 0.00 |  | 10.45 |
