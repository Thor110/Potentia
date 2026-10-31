# Making Sieve instructions: time by stage (2026-10-31)

Intel64 Family 6 Model 167 Stepping 1, GenuineIntel, 12 threads, Windows 10. `sieve locate FILE --installer` (the newest instructions), each file alone; seconds, from `--timings`. Stages nest in the total, and the pieces' rope includes its own search; cutting includes the blocks and the boundaries.

## Search 1 s a file

| File | Kind | Bytes | Made | s | MB/s | rope search | pieces' rope | cutting | blocks | boundaries | rope code | LZMA2 quick | LZMA2 trial | LZMA2 pass 1 | LZMA2 tuned | 7-Zip pass | archives | mixing |
| :--- | :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| prose.md | prose | 431,812 | 109,811 | 4.02 | 0.11 | 0.06 |  | 0.03 | 0.03 |  | 0.08 | 0.58 | 0.26 | 0.00 | 0.00 | 0.00 |  | 2.65 |
| code.cpp | code | 169,525 | 30,384 | 1.43 | 0.12 | 0.10 |  | 0.01 | 0.01 |  | 0.09 | 0.20 | 0.01 | 0.00 | 0.00 | 0.00 |  | 0.85 |
| oracle.py | code | 464,953 | 92,900 | 3.11 | 0.15 | 0.17 |  | 0.03 | 0.02 |  | 0.15 | 0.67 | 0.06 | 0.00 | 0.00 | 0.00 |  | 1.77 |
| table.csv | table | 17,831 | 7,262 | 0.87 | 0.02 | 0.60 | 0.05 | 0.02 | 0.00 | 0.02 | 0.00 | 0.14 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.17 |
| words.txt | word list | 2,921,105 | 310,401 | 8.87 | 0.33 | 0.28 |  | 0.10 | 0.10 |  | 0.14 | 2.52 | 0.00 | 0.00 | 0.00 | 0.00 |  | 3.88 |
| rope.txt | prose | 11,440 | 3,633 | 0.15 | 0.08 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.11 |
| screenshot.png | picture (PNG) | 92,628 | 80,506 | 0.59 | 0.16 | 0.00 | 0.01 | 0.04 | 0.00 | 0.03 | 0.00 | 0.35 | 0.03 | 0.02 | 0.04 | 0.01 |  | 0.28 |
| vitruvian.jpg | picture (JPEG) | 207,500 | 206,956 | 0.73 | 0.28 | 0.01 | 0.01 | 0.02 | 0.01 | 0.01 | 0.00 | 0.18 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.59 |
| door.jpg | picture (JPEG) | 69,298 | 63,860 | 0.40 | 0.18 | 0.00 |  | 0.00 | 0.00 |  | 0.00 | 0.05 | 0.02 | 0.02 | 0.03 | 0.01 |  | 0.23 |
| spitfire.obj | mesh | 469,717 | 53,519 | 2.08 | 0.23 | 0.06 |  | 0.02 | 0.02 |  | 0.05 | 0.57 | 0.00 | 0.00 | 0.00 | 0.00 |  | 1.27 |
| hymn.mid | melody (MIDI) | 1,377 | 230 | 0.02 | 0.06 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 0.01 | 0.00 | 0.00 | 0.00 |  |  |
| film.mp4 | video (MP4) | 234,633 | 233,867 | 1.19 | 0.20 | 0.01 | 0.01 | 0.02 | 0.01 | 0.01 | 0.00 | 0.24 | 0.06 | 0.06 | 0.09 | 0.03 |  | 0.65 |
| sound.wav | sound (WAV) | 88,244 | 3,257 | 2.27 | 0.04 | 1.77 | 0.13 | 0.09 | 0.00 | 0.08 | 0.00 | 0.60 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.17 |
| program.exe | program (Windows) | 5,337,607 | 4,275,365 | 3.74 | 1.43 |  |  |  |  |  |  |  | 0.41 | 1.69 | 3.33 | 1.42 |  |  |
| folder.tar | tar | 5,191,680 | 1,221,570 | 18.75 | 0.28 | 1.01 |  | 0.19 | 0.19 |  | 0.07 | 4.25 | 0.00 | 0.00 | 0.00 | 0.00 |  | 8.77 |
