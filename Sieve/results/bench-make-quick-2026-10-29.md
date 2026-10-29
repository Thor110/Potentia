# Making Sieve instructions: time by stage (2026-10-29)

Intel64 Family 6 Model 167 Stepping 1, GenuineIntel, 12 threads, Windows 10. `sieve locate FILE --installer` (the newest instructions), each file alone; seconds, from `--timings`. Stages nest in the total, and the pieces' rope includes its own search; cutting includes the blocks and the boundaries.

## Search 1 s a file

| File | Kind | Bytes | Made | s | MB/s | rope search | pieces' rope | cutting | blocks | boundaries | rope code | LZMA2 quick | LZMA2 trial | LZMA2 pass 1 | LZMA2 tuned | 7-Zip pass | archives | mixing |
| :--- | :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| prose.md | prose | 409,575 | 104,274 | 2.60 | 0.16 | 0.06 |  | 0.02 | 0.01 |  | 0.06 | 0.42 | 0.00 | 0.00 | 0.00 | 0.00 |  | 1.46 |
| code.cpp | code | 166,018 | 29,665 | 2.27 | 0.07 | 0.14 |  | 0.16 | 0.16 |  | 0.14 | 0.20 | 0.02 | 0.00 | 0.00 | 0.00 |  | 1.28 |
| oracle.py | code | 447,532 | 89,567 | 3.35 | 0.13 | 0.16 |  | 0.04 | 0.04 |  | 0.16 | 0.49 | 0.00 | 0.00 | 0.00 | 0.00 |  | 1.90 |
| table.csv | table | 17,831 | 7,262 | 0.93 | 0.02 | 0.64 | 0.05 | 0.02 | 0.00 | 0.02 | 0.00 | 0.14 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.18 |
| words.txt | word list | 2,921,105 | 310,401 | 10.74 | 0.27 | 0.16 |  | 0.11 | 0.10 |  | 0.17 | 2.36 | 0.00 | 0.00 | 0.00 | 0.00 |  | 5.16 |
| rope.txt | prose | 11,440 | 3,633 | 0.16 | 0.07 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.12 |
| screenshot.png | picture (PNG) | 92,628 | 80,506 | 0.60 | 0.15 | 0.00 | 0.01 | 0.04 | 0.01 | 0.03 | 0.00 | 0.33 | 0.03 | 0.02 | 0.03 | 0.01 |  | 0.30 |
| vitruvian.jpg | picture (JPEG) | 207,500 | 206,956 | 0.76 | 0.27 | 0.01 | 0.01 | 0.02 | 0.01 | 0.01 | 0.00 | 0.15 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.61 |
| door.jpg | picture (JPEG) | 69,298 | 63,860 | 0.40 | 0.17 | 0.00 |  | 0.00 | 0.00 |  | 0.00 | 0.05 | 0.02 | 0.01 | 0.03 | 0.01 |  | 0.22 |
| spitfire.obj | mesh | 469,717 | 53,519 | 2.08 | 0.23 | 0.05 |  | 0.02 | 0.02 |  | 0.05 | 0.46 | 0.00 | 0.00 | 0.00 | 0.00 |  | 1.23 |
| hymn.mid | melody (MIDI) | 1,377 | 230 | 0.02 | 0.06 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |  |  |
| film.mp4 | video (MP4) | 234,633 | 233,867 | 1.23 | 0.19 | 0.01 | 0.01 | 0.02 | 0.01 | 0.01 | 0.00 | 0.20 | 0.07 | 0.07 | 0.09 | 0.03 |  | 0.68 |
| sound.wav | sound (WAV) | 88,244 | 3,257 | 2.16 | 0.04 | 1.62 | 0.11 | 0.10 | 0.01 | 0.10 | 0.01 | 0.67 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.20 |
| program.exe | program (Windows) | 5,337,607 | 4,275,365 | 3.58 | 1.49 |  |  |  |  |  |  |  | 0.45 | 1.68 | 3.03 | 1.25 |  |  |
| folder.tar | tar | 5,150,720 | 1,211,916 | 18.43 | 0.28 | 1.01 |  | 0.16 | 0.16 |  | 0.06 | 3.51 | 0.00 | 0.00 | 0.00 | 0.00 |  | 8.02 |
