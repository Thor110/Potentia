# Making Sieve instructions: time by stage (2026-10-27)

Intel64 Family 6 Model 167 Stepping 1, GenuineIntel, 12 threads, Windows 10. `sieve locate FILE --installer` (the newest instructions), each file alone; seconds, from `--timings`. Stages nest in the total, and the pieces' rope includes its own search; cutting includes the blocks and the boundaries.

## Search 1 s a file

| File | Kind | Bytes | Made | s | MB/s | rope search | pieces' rope | cutting | blocks | boundaries | rope code | LZMA2 quick | LZMA2 trial | LZMA2 pass 1 | LZMA2 tuned | 7-Zip pass | archives |
| :--- | :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| prose.md | prose | 389,951 | 99,471 | 2.86 | 0.14 | 0.08 | 0.12 | 0.03 | 0.01 | 0.02 | 0.06 | 0.58 | 0.00 | 0.00 | 0.00 | 0.00 |  |
| code.cpp | code | 165,852 | 29,620 | 1.56 | 0.11 | 0.07 |  | 0.01 | 0.01 |  | 0.08 | 0.15 | 0.00 | 0.00 | 0.00 | 0.00 |  |
| oracle.py | code | 447,532 | 89,567 | 3.18 | 0.14 | 0.11 |  | 0.02 | 0.02 |  | 0.10 | 0.44 | 0.00 | 0.00 | 0.00 | 0.00 |  |
| table.csv | table | 17,831 | 7,262 | 0.73 | 0.02 | 0.46 | 0.04 | 0.02 | 0.00 | 0.01 | 0.00 | 0.13 | 0.00 | 0.00 | 0.00 | 0.00 |  |
| words.txt | word list | 2,921,105 | 310,401 | 13.07 | 0.22 | 0.22 |  | 0.17 | 0.15 |  | 0.18 | 2.65 | 0.00 | 0.00 | 0.00 | 0.00 |  |
| rope.txt | prose | 11,440 | 3,633 | 0.23 | 0.05 | 0.01 |  | 0.00 |  |  | 0.00 | 0.01 | 0.02 | 0.00 | 0.00 | 0.00 |  |
| screenshot.png | picture (PNG) | 92,628 | 80,506 | 0.72 | 0.13 | 0.00 | 0.01 | 0.04 | 0.01 | 0.03 | 0.00 | 0.34 | 0.03 | 0.02 | 0.04 | 0.01 |  |
| vitruvian.jpg | picture (JPEG) | 207,500 | 206,956 | 0.86 | 0.24 | 0.01 | 0.02 | 0.02 | 0.01 | 0.01 | 0.01 | 0.15 | 0.00 | 0.00 | 0.00 | 0.00 |  |
| door.jpg | picture (JPEG) | 69,298 | 63,860 | 0.42 | 0.17 | 0.00 |  | 0.00 | 0.00 |  | 0.00 | 0.04 | 0.02 | 0.01 | 0.02 | 0.01 |  |
| spitfire.obj | mesh | 469,717 | 53,519 | 2.53 | 0.19 | 0.06 |  | 0.02 | 0.02 |  | 0.05 | 0.46 | 0.00 | 0.00 | 0.00 | 0.00 |  |
| hymn.mid | melody (MIDI) | 1,377 | 230 | 0.02 | 0.07 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 0.01 | 0.00 | 0.00 | 0.00 |  |
| film.mp4 | video (MP4) | 234,633 | 233,867 | 1.43 | 0.16 | 0.01 | 0.02 | 0.01 | 0.01 | 0.01 | 0.01 | 0.19 | 0.07 | 0.07 | 0.10 | 0.03 |  |
| sound.wav | sound (WAV) | 88,244 | 3,257 | 2.41 | 0.04 | 1.75 | 0.11 | 0.10 | 0.01 | 0.09 | 0.01 | 0.67 | 0.00 | 0.00 | 0.00 | 0.00 |  |
| program.exe | program (Windows) | 5,337,607 | 4,275,365 | 3.70 | 1.44 |  |  |  |  |  |  |  | 0.40 | 1.63 | 3.22 | 1.37 |  |
| folder.tar | tar | 5,130,240 | 1,206,973 | 20.95 | 0.24 | 1.01 |  | 0.19 | 0.18 |  | 0.14 | 3.78 | 0.00 | 0.00 | 0.00 | 0.00 |  |
