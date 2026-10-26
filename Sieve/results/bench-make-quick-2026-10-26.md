# Making Sieve instructions: time by stage (2026-10-26)

Intel64 Family 6 Model 167 Stepping 1, GenuineIntel, 12 threads, Windows 10. `sieve locate FILE --installer` (the newest instructions), each file alone; seconds, from `--timings`. Stages nest in the total, and the pieces' rope includes its own search; cutting includes the blocks and the boundaries.

## Search 1 s a file

| File | Kind | Bytes | Made | s | MB/s | rope search | pieces' rope | cutting | blocks | boundaries | rope code | LZMA2 quick | LZMA2 trial | LZMA2 pass 1 | LZMA2 tuned | 7-Zip pass | archives |
| :--- | :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| prose.md | prose | 381,022 | 97,187 | 2.45 | 0.16 | 0.07 |  | 0.02 | 0.02 |  | 0.06 | 0.34 | 0.00 | 0.00 | 0.00 | 0.00 |  |
| code.cpp | code | 164,395 | 29,321 | 1.22 | 0.14 | 0.07 |  | 0.01 | 0.01 |  | 0.06 | 0.09 | 0.00 | 0.00 | 0.00 | 0.00 |  |
| oracle.py | code | 442,021 | 88,803 | 2.77 | 0.16 | 0.11 |  | 0.02 | 0.02 |  | 0.09 | 0.33 | 0.00 | 0.00 | 0.00 | 0.00 |  |
| table.csv | table | 17,831 | 7,262 | 0.87 | 0.02 | 0.52 | 0.05 | 0.02 | 0.00 | 0.02 | 0.00 | 0.12 | 0.00 | 0.00 | 0.00 | 0.00 |  |
| words.txt | word list | 2,921,105 | 310,401 | 11.48 | 0.25 | 0.22 |  | 0.10 | 0.09 |  | 0.17 | 2.26 | 0.00 | 0.00 | 0.00 | 0.00 |  |
| rope.txt | prose | 11,440 | 3,633 | 0.16 | 0.07 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |  |
| screenshot.png | picture (PNG) | 92,628 | 80,506 | 0.54 | 0.17 | 0.01 | 0.01 | 0.04 | 0.00 | 0.04 | 0.00 | 0.31 | 0.02 | 0.02 | 0.03 | 0.01 |  |
| vitruvian.jpg | picture (JPEG) | 207,500 | 206,956 | 0.79 | 0.26 | 0.01 | 0.02 | 0.01 | 0.01 | 0.01 | 0.01 | 0.15 | 0.00 | 0.00 | 0.00 | 0.00 |  |
| door.jpg | picture (JPEG) | 69,298 | 63,860 | 0.31 | 0.22 | 0.00 |  | 0.00 | 0.00 |  | 0.00 | 0.03 | 0.02 | 0.02 | 0.02 | 0.01 |  |
| spitfire.obj | mesh | 469,717 | 53,519 | 2.27 | 0.21 | 0.06 |  | 0.02 | 0.02 |  | 0.06 | 0.41 | 0.00 | 0.00 | 0.00 | 0.00 |  |
| hymn.mid | melody (MIDI) | 1,377 | 230 | 0.02 | 0.07 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |  |
| film.mp4 | video (MP4) | 234,633 | 233,867 | 0.93 | 0.25 | 0.01 | 0.02 | 0.02 | 0.01 | 0.01 | 0.01 | 0.17 | 0.07 | 0.06 | 0.08 | 0.02 |  |
| sound.wav | sound (WAV) | 88,244 | 3,257 | 2.29 | 0.04 | 1.76 | 0.11 | 0.10 | 0.01 | 0.09 | 0.01 | 0.63 | 0.00 | 0.00 | 0.00 | 0.00 |  |
| program.exe | program (Windows) | 5,337,607 | 4,275,365 | 3.47 | 1.54 |  |  |  |  |  |  |  | 0.39 | 1.50 | 3.08 | 1.27 |  |
| folder.tar | tar | 5,120,000 | 1,203,638 | 21.07 | 0.24 | 1.07 |  | 0.17 | 0.17 |  | 0.16 | 3.68 | 0.00 | 0.00 | 0.00 | 0.00 |  |
