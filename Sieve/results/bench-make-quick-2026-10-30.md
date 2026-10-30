# Making Sieve instructions: time by stage (2026-10-30)

Intel64 Family 6 Model 167 Stepping 1, GenuineIntel, 12 threads, Windows 10. `sieve locate FILE --installer` (the newest instructions), each file alone; seconds, from `--timings`. Stages nest in the total, and the pieces' rope includes its own search; cutting includes the blocks and the boundaries.

## Search 1 s a file

| File | Kind | Bytes | Made | s | MB/s | rope search | pieces' rope | cutting | blocks | boundaries | rope code | LZMA2 quick | LZMA2 trial | LZMA2 pass 1 | LZMA2 tuned | 7-Zip pass | archives | mixing |
| :--- | :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| prose.md | prose | 418,174 | 106,430 | 3.43 | 0.12 | 0.14 |  | 0.10 | 0.10 |  | 0.12 | 0.68 | 0.00 | 0.00 | 0.00 | 0.00 |  | 2.19 |
| code.cpp | code | 166,520 | 29,740 | 1.51 | 0.11 | 0.08 |  | 0.01 | 0.01 |  | 0.07 | 0.19 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.92 |
| oracle.py | code | 456,592 | 91,335 | 3.08 | 0.15 | 0.14 |  | 0.02 | 0.01 |  | 0.11 | 0.48 | 0.00 | 0.00 | 0.00 | 0.00 |  | 1.87 |
| table.csv | table | 17,831 | 7,262 | 0.91 | 0.02 | 0.60 | 0.05 | 0.02 | 0.00 | 0.02 | 0.00 | 0.14 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.19 |
| words.txt | word list | 2,921,105 | 310,401 | 9.78 | 0.30 | 0.18 |  | 0.12 | 0.12 |  | 0.12 | 2.78 | 0.00 | 0.00 | 0.00 | 0.00 |  | 4.70 |
| rope.txt | prose | 11,440 | 3,633 | 0.15 | 0.08 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.11 |
| screenshot.png | picture (PNG) | 92,628 | 80,506 | 0.59 | 0.16 | 0.00 | 0.01 | 0.04 | 0.00 | 0.03 | 0.00 | 0.33 | 0.03 | 0.02 | 0.03 | 0.01 |  | 0.29 |
| vitruvian.jpg | picture (JPEG) | 207,500 | 206,956 | 0.69 | 0.30 | 0.01 | 0.01 | 0.01 | 0.01 | 0.01 | 0.00 | 0.16 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.58 |
| door.jpg | picture (JPEG) | 69,298 | 63,860 | 0.35 | 0.20 | 0.00 |  | 0.00 | 0.00 |  | 0.00 | 0.05 | 0.02 | 0.01 | 0.02 | 0.01 |  | 0.19 |
| spitfire.obj | mesh | 469,717 | 53,519 | 1.79 | 0.26 | 0.06 |  | 0.02 | 0.02 |  | 0.04 | 0.44 | 0.00 | 0.00 | 0.00 | 0.00 |  | 1.12 |
| hymn.mid | melody (MIDI) | 1,377 | 230 | 0.02 | 0.07 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |  |  |
| film.mp4 | video (MP4) | 234,633 | 233,867 | 1.03 | 0.23 | 0.01 | 0.01 | 0.02 | 0.01 | 0.01 | 0.00 | 0.19 | 0.06 | 0.06 | 0.06 | 0.03 |  | 0.58 |
| sound.wav | sound (WAV) | 88,244 | 3,257 | 2.11 | 0.04 | 1.64 | 0.13 | 0.09 | 0.00 | 0.08 | 0.00 | 0.58 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.16 |
| program.exe | program (Windows) | 5,337,607 | 4,275,365 | 3.27 | 1.63 |  |  |  |  |  |  |  | 0.37 | 1.50 | 2.86 | 1.25 |  |  |
| folder.tar | tar | 5,171,200 | 1,215,910 | 17.67 | 0.29 | 1.02 |  | 0.18 | 0.18 |  | 0.06 | 3.87 | 0.00 | 0.00 | 0.00 | 0.00 |  | 8.29 |
