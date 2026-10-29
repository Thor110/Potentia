# Making Sieve instructions: time by stage (2026-10-29)

Intel64 Family 6 Model 167 Stepping 1, GenuineIntel, 12 threads, Windows 10. `sieve locate FILE --installer` (the newest instructions), each file alone; seconds, from `--timings`. Stages nest in the total, and the pieces' rope includes its own search; cutting includes the blocks and the boundaries.

## Search 1 s a file

| File | Kind | Bytes | Made | s | MB/s | rope search | pieces' rope | cutting | blocks | boundaries | rope code | LZMA2 quick | LZMA2 trial | LZMA2 pass 1 | LZMA2 tuned | 7-Zip pass | archives | mixing |
| :--- | :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| prose.md | prose | 411,783 | 104,802 | 4.11 | 0.10 | 0.07 |  | 0.01 | 0.01 |  | 0.06 | 0.49 | 0.16 | 0.00 | 0.00 | 0.00 |  | 2.67 |
| code.cpp | code | 166,436 | 29,704 | 1.52 | 0.11 | 0.09 |  | 0.01 | 0.01 |  | 0.08 | 0.16 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.98 |
| oracle.py | code | 452,036 | 90,459 | 3.11 | 0.15 | 0.15 |  | 0.02 | 0.02 |  | 0.12 | 0.50 | 0.00 | 0.00 | 0.00 | 0.00 |  | 1.79 |
| table.csv | table | 17,831 | 7,262 | 0.80 | 0.02 | 0.55 | 0.05 | 0.02 | 0.00 | 0.02 | 0.00 | 0.14 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.14 |
| words.txt | word list | 2,921,105 | 310,401 | 9.39 | 0.31 | 0.17 |  | 0.14 | 0.14 |  | 0.14 | 2.69 | 0.00 | 0.00 | 0.00 | 0.00 |  | 4.20 |
| rope.txt | prose | 11,440 | 3,633 | 0.24 | 0.05 | 0.01 |  | 0.00 |  |  | 0.00 | 0.01 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.18 |
| screenshot.png | picture (PNG) | 92,628 | 80,506 | 0.70 | 0.13 | 0.00 | 0.01 | 0.05 | 0.01 | 0.05 | 0.00 | 0.37 | 0.04 | 0.03 | 0.05 | 0.02 |  | 0.32 |
| vitruvian.jpg | picture (JPEG) | 207,500 | 206,956 | 0.74 | 0.28 | 0.01 | 0.01 | 0.02 | 0.01 | 0.01 | 0.00 | 0.18 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.59 |
| door.jpg | picture (JPEG) | 69,298 | 63,860 | 0.36 | 0.19 | 0.00 |  | 0.00 | 0.00 |  | 0.00 | 0.05 | 0.02 | 0.02 | 0.02 | 0.01 |  | 0.21 |
| spitfire.obj | mesh | 469,717 | 53,519 | 2.03 | 0.23 | 0.07 |  | 0.02 | 0.02 |  | 0.04 | 0.51 | 0.00 | 0.00 | 0.00 | 0.00 |  | 1.28 |
| hymn.mid | melody (MIDI) | 1,377 | 230 | 0.02 | 0.06 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |  |  |
| film.mp4 | video (MP4) | 234,633 | 233,867 | 1.26 | 0.19 | 0.01 | 0.02 | 0.02 | 0.01 | 0.01 | 0.00 | 0.24 | 0.08 | 0.09 | 0.14 | 0.05 |  | 0.66 |
| sound.wav | sound (WAV) | 88,244 | 3,257 | 2.23 | 0.04 | 1.60 | 0.12 | 0.10 | 0.00 | 0.09 | 0.00 | 0.65 | 0.01 | 0.00 | 0.00 | 0.00 |  | 0.26 |
| program.exe | program (Windows) | 5,337,607 | 4,275,365 | 3.49 | 1.53 |  |  |  |  |  |  |  | 0.43 | 1.60 | 3.03 | 1.31 |  |  |
| folder.tar | tar | 5,160,960 | 1,213,361 | 18.25 | 0.28 | 1.01 |  | 0.19 | 0.19 |  | 0.07 | 4.04 | 0.00 | 0.00 | 0.00 | 0.00 |  | 8.41 |
