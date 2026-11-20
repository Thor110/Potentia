# Making Sieve instructions: time by stage (2026-11-20)

Intel64 Family 6 Model 167 Stepping 1, GenuineIntel, 12 threads, Windows 10. `sieve locate FILE --installer` (the newest instructions), each file alone; seconds, from `--timings`. Stages nest in the total, and the pieces' rope includes its own search; cutting includes the blocks and the boundaries.

## Search 1 s a file

| File | Kind | Bytes | Made | s | MB/s | rope search | pieces' rope | cutting | blocks | boundaries | rope code | LZMA2 quick | LZMA2 trial | LZMA2 pass 1 | LZMA2 tuned | 7-Zip pass | archives | mixing |
| :--- | :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| prose.md | prose | 914,177 | 217,547 | 3.99 | 0.23 |  |  |  |  |  |  | 0.08 | 0.00 | 0.00 | 0.00 | 0.00 |  | 2.18 |
| code.cpp | code | 239,881 | 41,529 | 1.52 | 0.16 |  |  |  |  |  |  | 0.05 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.99 |
| oracle.py | code | 544,283 | 105,655 | 5.07 | 0.11 |  |  |  |  |  |  | 0.09 | 0.93 | 0.00 | 0.00 | 0.00 |  | 2.18 |
| table.csv | table | 17,831 | 7,249 | 0.83 | 0.02 | 0.42 | 0.04 | 0.02 | 0.00 | 0.01 | 0.00 | 0.11 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.33 |
| words.txt | word list | 2,921,105 | 287,033 | 12.15 | 0.24 | 0.13 |  | 0.06 | 0.06 |  | 0.08 | 0.69 | 0.01 | 0.00 | 0.00 | 0.00 |  | 5.12 |
| rope.txt | prose | 11,440 | 3,293 | 1.66 | 0.01 | 0.04 |  | 0.00 |  |  | 0.00 | 0.00 | 0.69 | 0.00 | 0.00 | 0.00 |  | 0.68 |
| screenshot.png | picture (PNG) | 92,628 | 80,502 | 2.70 | 0.03 | 0.00 | 0.02 | 1.03 | 0.26 | 0.77 | 0.00 | 0.30 | 0.39 | 0.00 | 0.05 | 0.02 |  | 0.76 |
| vitruvian.jpg | picture (JPEG) | 207,500 | 206,946 | 1.57 | 0.13 | 0.01 | 0.03 | 0.16 | 0.08 | 0.07 | 0.01 | 0.05 | 0.01 | 0.00 | 0.00 | 0.00 |  | 1.16 |
| door.jpg | picture (JPEG) | 69,298 | 63,856 | 0.69 | 0.10 | 0.00 |  | 0.00 | 0.00 |  | 0.00 | 0.02 | 0.03 | 0.00 | 0.03 | 0.01 |  | 0.42 |
| spitfire.obj | mesh | 469,717 | 53,137 | 3.68 | 0.13 |  |  |  |  |  |  | 0.10 | 0.00 | 0.00 | 0.00 | 0.00 |  | 2.51 |
| hymn.mid | melody (MIDI) | 1,377 | 221 | 0.03 | 0.05 | 0.02 |  | 0.00 |  |  | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |  |  |
| film.mp4 | video (MP4) | 234,633 | 233,863 | 1.52 | 0.15 | 0.01 | 0.01 | 0.01 | 0.01 | 0.01 | 0.00 | 0.12 | 0.08 | 0.00 | 0.10 | 0.04 |  | 0.88 |
| sound.wav | sound (WAV) | 88,244 | 3,233 | 2.24 | 0.04 | 1.48 | 0.13 | 0.09 | 0.00 | 0.08 | 0.00 | 0.44 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.17 |
| program.exe | program (Windows) | 5,337,607 | 4,223,640 | 22.93 | 0.23 |  |  |  |  |  |  | 1.84 | 0.00 | 0.00 | 0.00 | 0.00 |  | 10.34 |
| folder.tar | tar | 5,826,560 | 1,325,024 | 18.41 | 0.32 |  |  |  |  |  |  | 0.06 | 0.00 | 0.00 | 0.00 | 0.00 |  | 9.26 |
