# Making Sieve instructions: time by stage (2026-10-29)

Intel64 Family 6 Model 167 Stepping 1, GenuineIntel, 12 threads, Windows 10. `sieve locate FILE --installer` (the newest instructions), each file alone; seconds, from `--timings`. Stages nest in the total, and the pieces' rope includes its own search; cutting includes the blocks and the boundaries.

## Search 1 s a file

| File | Kind | Bytes | Made | s | MB/s | rope search | pieces' rope | cutting | blocks | boundaries | rope code | LZMA2 quick | LZMA2 trial | LZMA2 pass 1 | LZMA2 tuned | 7-Zip pass | archives | mixing |
| :--- | :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| prose.md | prose | 415,641 | 105,747 | 3.77 | 0.11 | 0.07 |  | 0.02 | 0.02 |  | 0.07 | 0.49 | 0.20 | 0.00 | 0.00 | 0.00 |  | 1.92 |
| code.cpp | code | 166,436 | 29,704 | 1.75 | 0.10 | 0.13 |  | 0.05 | 0.05 |  | 0.10 | 0.16 | 0.00 | 0.00 | 0.00 | 0.00 |  | 1.08 |
| oracle.py | code | 453,576 | 90,806 | 3.02 | 0.15 | 0.16 |  | 0.02 | 0.02 |  | 0.12 | 0.54 | 0.00 | 0.00 | 0.00 | 0.00 |  | 1.85 |
| table.csv | table | 17,831 | 7,262 | 0.89 | 0.02 | 0.59 | 0.06 | 0.02 | 0.00 | 0.02 | 0.00 | 0.15 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.17 |
| words.txt | word list | 2,921,105 | 310,401 | 9.75 | 0.30 | 0.19 |  | 0.13 | 0.12 |  | 0.12 | 2.62 | 0.00 | 0.00 | 0.00 | 0.00 |  | 4.71 |
| rope.txt | prose | 11,440 | 3,633 | 0.14 | 0.08 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.11 |
| screenshot.png | picture (PNG) | 92,628 | 80,506 | 0.58 | 0.16 | 0.00 | 0.01 | 0.04 | 0.01 | 0.03 | 0.00 | 0.34 | 0.02 | 0.02 | 0.03 | 0.01 |  | 0.29 |
| vitruvian.jpg | picture (JPEG) | 207,500 | 206,956 | 0.69 | 0.30 | 0.01 | 0.01 | 0.01 | 0.01 | 0.01 | 0.00 | 0.14 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.57 |
| door.jpg | picture (JPEG) | 69,298 | 63,860 | 0.36 | 0.19 | 0.00 |  | 0.00 | 0.00 |  | 0.00 | 0.05 | 0.02 | 0.01 | 0.02 | 0.01 |  | 0.20 |
| spitfire.obj | mesh | 469,717 | 53,519 | 2.03 | 0.23 | 0.05 |  | 0.02 | 0.02 |  | 0.04 | 0.49 | 0.00 | 0.00 | 0.00 | 0.00 |  | 1.28 |
| hymn.mid | melody (MIDI) | 1,377 | 230 | 0.02 | 0.07 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |  |  |
| film.mp4 | video (MP4) | 234,633 | 233,867 | 1.26 | 0.19 | 0.01 | 0.01 | 0.02 | 0.01 | 0.01 | 0.00 | 0.24 | 0.07 | 0.07 | 0.08 | 0.03 |  | 0.69 |
| sound.wav | sound (WAV) | 88,244 | 3,257 | 2.25 | 0.04 | 1.68 | 0.14 | 0.09 | 0.00 | 0.09 | 0.00 | 0.61 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.20 |
| program.exe | program (Windows) | 5,337,607 | 4,275,365 | 3.46 | 1.54 |  |  |  |  |  |  |  | 0.38 | 1.51 | 3.08 | 1.31 |  |  |
| folder.tar | tar | 5,160,960 | 1,214,649 | 17.98 | 0.29 | 1.03 |  | 0.18 | 0.18 |  | 0.07 | 3.99 | 0.00 | 0.00 | 0.00 | 0.00 |  | 8.53 |
