# Making Sieve instructions: time by stage (2026-10-31)

Intel64 Family 6 Model 167 Stepping 1, GenuineIntel, 12 threads, Windows 10. `sieve locate FILE --installer` (the newest instructions), each file alone; seconds, from `--timings`. Stages nest in the total, and the pieces' rope includes its own search; cutting includes the blocks and the boundaries.

## Search 1 s a file

| File | Kind | Bytes | Made | s | MB/s | rope search | pieces' rope | cutting | blocks | boundaries | rope code | LZMA2 quick | LZMA2 trial | LZMA2 pass 1 | LZMA2 tuned | 7-Zip pass | archives | mixing |
| :--- | :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| prose.md | prose | 431,812 | 109,811 | 3.85 | 0.11 | 0.15 |  | 0.24 | 0.24 |  | 0.11 | 0.66 | 0.00 | 0.00 | 0.00 | 0.00 |  | 2.44 |
| code.cpp | code | 169,525 | 30,384 | 1.27 | 0.13 | 0.08 |  | 0.01 | 0.01 |  | 0.08 | 0.17 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.79 |
| oracle.py | code | 470,065 | 93,870 | 2.64 | 0.18 | 0.14 |  | 0.02 | 0.02 |  | 0.11 | 0.52 | 0.00 | 0.00 | 0.00 | 0.00 |  | 1.55 |
| table.csv | table | 17,831 | 7,262 | 0.77 | 0.02 | 0.53 | 0.05 | 0.02 | 0.00 | 0.02 | 0.00 | 0.14 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.13 |
| words.txt | word list | 2,921,105 | 310,401 | 8.92 | 0.33 | 0.17 |  | 0.11 | 0.10 |  | 0.12 | 2.35 | 0.00 | 0.00 | 0.00 | 0.00 |  | 4.16 |
| rope.txt | prose | 11,440 | 3,633 | 0.15 | 0.07 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.11 |
| screenshot.png | picture (PNG) | 92,628 | 80,506 | 0.58 | 0.16 | 0.00 | 0.01 | 0.04 | 0.00 | 0.04 | 0.00 | 0.34 | 0.02 | 0.02 | 0.04 | 0.01 |  | 0.29 |
| vitruvian.jpg | picture (JPEG) | 207,500 | 206,956 | 0.66 | 0.32 | 0.01 | 0.01 | 0.01 | 0.01 | 0.01 | 0.00 | 0.15 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.54 |
| door.jpg | picture (JPEG) | 69,298 | 63,860 | 0.36 | 0.19 | 0.00 |  | 0.00 | 0.00 |  | 0.00 | 0.05 | 0.02 | 0.02 | 0.02 | 0.01 |  | 0.20 |
| spitfire.obj | mesh | 469,717 | 53,519 | 1.82 | 0.26 | 0.05 |  | 0.02 | 0.02 |  | 0.05 | 0.47 | 0.00 | 0.00 | 0.00 | 0.00 |  | 1.13 |
| hymn.mid | melody (MIDI) | 1,377 | 230 | 0.02 | 0.07 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |  |  |
| film.mp4 | video (MP4) | 234,633 | 233,867 | 1.06 | 0.22 | 0.01 | 0.01 | 0.02 | 0.01 | 0.01 | 0.00 | 0.19 | 0.06 | 0.06 | 0.07 | 0.02 |  | 0.57 |
| sound.wav | sound (WAV) | 88,244 | 3,257 | 2.16 | 0.04 | 1.67 | 0.12 | 0.09 | 0.00 | 0.09 | 0.00 | 0.61 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.18 |
| program.exe | program (Windows) | 5,337,607 | 4,275,365 | 3.30 | 1.62 |  |  |  |  |  |  |  | 0.37 | 1.49 | 2.93 | 1.21 |  |  |
| folder.tar | tar | 5,201,920 | 1,222,535 | 16.07 | 0.32 | 1.03 |  | 0.18 | 0.17 |  | 0.08 | 3.99 | 0.00 | 0.00 | 0.00 | 0.00 |  | 7.34 |
