# Making Sieve instructions: time by stage (2026-10-25)

Intel64 Family 6 Model 167 Stepping 1, GenuineIntel, 12 threads, Windows 10. `sieve locate FILE --installer` (the newest instructions), each file alone; seconds, from `--timings`. Stages nest in the total, and the pieces' rope includes its own search; cutting includes the blocks and the boundaries.

## Search 1 s a file

| File | Kind | Bytes | Made | s | MB/s | rope search | pieces' rope | cutting | blocks | boundaries | rope code | LZMA2 quick | LZMA2 trial | LZMA2 pass 1 | LZMA2 tuned | 7-Zip pass | archives |
| :--- | :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| prose.md | prose | 356,751 | 115,757 | 0.60 | 0.59 | 0.07 | 0.08 | 0.03 | 0.01 | 0.02 | 0.05 | 0.42 | 0.15 | 0.15 | 0.21 | 0.10 |  |
| code.cpp | code | 161,633 | 34,486 | 0.27 | 0.60 | 0.06 |  | 0.01 | 0.01 |  | 0.06 | 0.09 | 0.05 | 0.04 | 0.07 | 0.04 |  |
| oracle.py | code | 401,578 | 95,532 | 0.58 | 0.69 | 0.10 |  | 0.02 | 0.02 |  | 0.09 | 0.25 | 0.14 | 0.14 | 0.20 | 0.09 |  |
| table.csv | table | 17,831 | 7,739 | 0.51 | 0.04 | 0.44 | 0.04 | 0.02 | 0.00 | 0.01 | 0.00 | 0.12 | 0.01 | 0.00 | 0.01 | 0.00 |  |
| words.txt | word list | 2,921,105 | 564,462 | 1.39 | 2.11 | 0.17 |  | 0.09 | 0.09 |  | 0.16 | 1.72 | 0.00 | 0.00 | 0.00 | 0.00 |  |
| rope.txt | prose | 11,440 | 4,609 | 0.02 | 0.52 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 0.01 | 0.00 | 0.00 | 0.00 |  |
| screenshot.png | picture (PNG) | 92,628 | 80,512 | 0.19 | 0.49 | 0.00 | 0.01 | 0.04 | 0.01 | 0.03 | 0.00 | 0.31 | 0.02 | 0.02 | 0.03 | 0.01 |  |
| vitruvian.jpg | picture (JPEG) | 207,500 | 206,966 | 0.17 | 1.25 | 0.01 | 0.01 | 0.01 | 0.01 | 0.01 | 0.01 | 0.15 | 0.00 | 0.00 | 0.00 | 0.00 |  |
| door.jpg | picture (JPEG) | 69,298 | 63,866 | 0.06 | 1.07 | 0.00 |  | 0.00 | 0.00 |  | 0.00 | 0.03 | 0.01 | 0.01 | 0.02 | 0.01 |  |
| spitfire.obj | mesh | 469,717 | 69,054 | 0.61 | 0.77 | 0.08 |  | 0.02 | 0.02 |  | 0.06 | 0.32 | 0.17 | 0.16 | 0.23 | 0.13 |  |
| hymn.mid | melody (MIDI) | 1,377 | 236 | 0.01 | 0.10 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |  |
| film.mp4 | video (MP4) | 234,633 | 233,874 | 0.20 | 1.16 | 0.01 | 0.02 | 0.01 | 0.01 | 0.01 | 0.01 | 0.15 | 0.06 | 0.06 | 0.07 | 0.03 |  |
| sound.wav | sound (WAV) | 88,244 | 3,742 | 1.65 | 0.05 | 1.44 | 0.09 | 0.09 | 0.00 | 0.09 | 0.00 | 0.64 | 0.01 | 0.00 | 0.01 | 0.01 |  |
| program.exe | program (Windows) | 5,337,607 | 4,275,368 | 3.26 | 1.64 |  |  |  |  |  |  |  | 0.36 | 1.47 | 2.72 | 1.15 |  |
| folder.tar | tar | 5,048,320 | 1,544,086 | 6.30 | 0.80 | 1.06 |  | 0.17 | 0.16 |  | 0.15 | 3.19 | 0.40 | 1.60 | 3.35 | 1.50 |  |
