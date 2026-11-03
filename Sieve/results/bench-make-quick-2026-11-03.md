# Making Sieve instructions: time by stage (2026-11-03)

Intel64 Family 6 Model 167 Stepping 1, GenuineIntel, 12 threads, Windows 10. `sieve locate FILE --installer` (the newest instructions), each file alone; seconds, from `--timings`. Stages nest in the total, and the pieces' rope includes its own search; cutting includes the blocks and the boundaries.

## Search 1 s a file

| File | Kind | Bytes | Made | s | MB/s | rope search | pieces' rope | cutting | blocks | boundaries | rope code | LZMA2 quick | LZMA2 trial | LZMA2 pass 1 | LZMA2 tuned | 7-Zip pass | archives | mixing |
| :--- | :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| prose.md | prose | 477,546 | 119,201 | 4.13 | 0.12 |  |  |  |  |  |  | 0.06 | 0.00 | 0.00 | 0.00 | 0.00 |  | 3.27 |
| code.cpp | code | 171,395 | 30,324 | 2.78 | 0.06 |  |  |  |  |  |  | 0.04 | 0.00 | 0.00 | 0.00 | 0.00 |  | 2.30 |
| oracle.py | code | 487,013 | 96,232 | 4.15 | 0.12 |  |  |  |  |  |  | 0.07 | 0.00 | 0.00 | 0.00 | 0.00 |  | 3.21 |
| table.csv | table | 17,831 | 7,249 | 1.13 | 0.02 | 0.53 | 0.05 | 0.02 | 0.00 | 0.02 | 0.00 | 0.13 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.50 |
| words.txt | word list | 2,921,105 | 287,033 | 10.53 | 0.28 | 0.18 |  | 0.17 | 0.17 |  | 0.11 | 2.04 | 0.00 | 0.00 | 0.00 | 0.00 |  | 5.71 |
| rope.txt | prose | 11,440 | 3,293 | 0.44 | 0.03 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.41 |
| screenshot.png | picture (PNG) | 92,628 | 80,502 | 1.28 | 0.07 | 0.00 | 0.01 | 0.04 | 0.01 | 0.03 | 0.00 | 0.33 | 0.03 | 0.00 | 0.03 | 0.01 |  | 0.98 |
| vitruvian.jpg | picture (JPEG) | 207,500 | 206,946 | 2.15 | 0.10 | 0.01 | 0.01 | 0.01 | 0.01 | 0.01 | 0.00 | 0.11 | 0.00 | 0.00 | 0.00 | 0.00 |  | 2.03 |
| door.jpg | picture (JPEG) | 69,298 | 63,856 | 0.89 | 0.08 | 0.00 |  | 0.00 | 0.00 |  | 0.00 | 0.04 | 0.02 | 0.00 | 0.02 | 0.01 |  | 0.73 |
| spitfire.obj | mesh | 469,717 | 53,144 | 3.82 | 0.12 |  |  |  |  |  |  | 0.06 | 0.00 | 0.00 | 0.00 | 0.00 |  | 3.00 |
| hymn.mid | melody (MIDI) | 1,377 | 221 | 0.02 | 0.06 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |  |  |
| film.mp4 | video (MP4) | 234,633 | 233,863 | 2.69 | 0.09 | 0.01 | 0.01 | 0.02 | 0.01 | 0.01 | 0.00 | 0.18 | 0.06 | 0.00 | 0.08 | 0.02 |  | 2.20 |
| sound.wav | sound (WAV) | 88,244 | 3,263 | 2.26 | 0.04 | 1.56 | 0.11 | 0.09 | 0.00 | 0.08 | 0.00 | 0.61 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.40 |
| program.exe | program (Windows) | 5,337,607 | 4,255,565 | 35.86 | 0.15 |  |  |  |  |  |  | 1.36 | 0.00 | 0.00 | 0.00 | 0.00 |  | 25.80 |
| folder.tar | tar | 5,263,360 | 1,203,099 | 20.66 | 0.25 |  |  |  |  |  |  | 0.08 | 0.00 | 0.00 | 0.00 | 0.00 |  | 11.77 |
