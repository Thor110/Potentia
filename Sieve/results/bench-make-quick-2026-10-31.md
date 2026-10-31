# Making Sieve instructions: time by stage (2026-10-31)

Intel64 Family 6 Model 167 Stepping 1, GenuineIntel, 12 threads, Windows 10. `sieve locate FILE --installer` (the newest instructions), each file alone; seconds, from `--timings`. Stages nest in the total, and the pieces' rope includes its own search; cutting includes the blocks and the boundaries.

## Search 1 s a file

| File | Kind | Bytes | Made | s | MB/s | rope search | pieces' rope | cutting | blocks | boundaries | rope code | LZMA2 quick | LZMA2 trial | LZMA2 pass 1 | LZMA2 tuned | 7-Zip pass | archives | mixing |
| :--- | :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| prose.md | prose | 430,073 | 109,382 | 3.62 | 0.12 | 0.15 |  | 0.20 | 0.20 |  | 0.13 | 0.64 | 0.00 | 0.00 | 0.00 | 0.00 |  | 2.24 |
| code.cpp | code | 168,903 | 30,240 | 1.27 | 0.13 | 0.08 |  | 0.01 | 0.01 |  | 0.08 | 0.19 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.77 |
| oracle.py | code | 462,009 | 92,486 | 2.59 | 0.18 | 0.14 |  | 0.02 | 0.02 |  | 0.09 | 0.52 | 0.01 | 0.00 | 0.00 | 0.00 |  | 1.54 |
| table.csv | table | 17,831 | 7,262 | 0.83 | 0.02 | 0.60 | 0.06 | 0.02 | 0.00 | 0.01 | 0.00 | 0.13 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.12 |
| words.txt | word list | 2,921,105 | 310,401 | 9.36 | 0.31 | 0.28 |  | 0.11 | 0.11 |  | 0.19 | 2.75 | 0.00 | 0.00 | 0.00 | 0.00 |  | 3.97 |
| rope.txt | prose | 11,440 | 3,633 | 0.15 | 0.07 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.11 |
| screenshot.png | picture (PNG) | 92,628 | 80,506 | 0.60 | 0.15 | 0.00 | 0.01 | 0.04 | 0.00 | 0.04 | 0.00 | 0.35 | 0.02 | 0.02 | 0.04 | 0.02 |  | 0.31 |
| vitruvian.jpg | picture (JPEG) | 207,500 | 206,956 | 0.69 | 0.30 | 0.01 | 0.01 | 0.02 | 0.01 | 0.01 | 0.00 | 0.18 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.55 |
| door.jpg | picture (JPEG) | 69,298 | 63,860 | 0.41 | 0.17 | 0.00 |  | 0.00 | 0.00 |  | 0.00 | 0.05 | 0.02 | 0.01 | 0.03 | 0.01 |  | 0.21 |
| spitfire.obj | mesh | 469,717 | 53,519 | 1.90 | 0.25 | 0.06 |  | 0.02 | 0.02 |  | 0.04 | 0.45 | 0.00 | 0.00 | 0.00 | 0.00 |  | 1.19 |
| hymn.mid | melody (MIDI) | 1,377 | 230 | 0.02 | 0.07 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |  |  |
| film.mp4 | video (MP4) | 234,633 | 233,867 | 1.16 | 0.20 | 0.01 | 0.01 | 0.02 | 0.01 | 0.01 | 0.00 | 0.21 | 0.07 | 0.06 | 0.07 | 0.02 |  | 0.67 |
| sound.wav | sound (WAV) | 88,244 | 3,257 | 2.39 | 0.04 | 1.59 | 0.19 | 0.31 | 0.02 | 0.30 | 0.00 | 0.47 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.19 |
| program.exe | program (Windows) | 5,337,607 | 4,275,365 | 3.48 | 1.53 |  |  |  |  |  |  |  | 0.44 | 1.61 | 3.07 | 1.32 |  |  |
| folder.tar | tar | 5,191,680 | 1,220,579 | 17.45 | 0.30 | 1.02 |  | 0.18 | 0.18 |  | 0.06 | 3.96 | 0.00 | 0.00 | 0.00 | 0.00 |  | 8.35 |
