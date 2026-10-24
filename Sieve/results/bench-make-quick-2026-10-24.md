# Making Sieve instructions: time by stage (2026-10-24)

Intel64 Family 6 Model 167 Stepping 1, GenuineIntel, 12 threads, Windows 10. `sieve locate FILE --installer` (the newest instructions), each file alone; seconds, from `--timings`. Stages nest in the total, and the pieces' rope includes its own search.

## Search 1 s a file

| File | Kind | Bytes | Made | s | MB/s | rope search | pieces' rope | cutting | rope code | LZMA2 quick | LZMA2 trial | LZMA2 pass 1 | LZMA2 tuned | 7-Zip pass | archives |
| :--- | :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| prose.md | prose | 345,569 | 112,246 | 1.34 | 0.26 | 1.00 |  | 0.01 | 0.01 | 0.18 | 0.13 | 0.09 | 0.18 | 0.07 |  |
| code.cpp | code | 161,519 | 34,453 | 1.14 | 0.14 | 1.01 |  | 0.01 | 0.00 | 0.09 | 0.05 | 0.03 | 0.06 | 0.03 |  |
| oracle.py | code | 399,245 | 95,144 | 1.46 | 0.27 | 1.03 |  | 0.01 | 0.08 | 0.24 | 0.12 | 0.12 | 0.18 | 0.08 |  |
| table.csv | table | 17,831 | 7,739 | 0.48 | 0.04 | 0.39 | 0.04 | 0.04 | 0.00 | 0.38 | 0.01 | 0.00 | 0.01 | 0.00 |  |
| words.txt | word list | 2,921,105 | 633,244 | 3.66 | 0.80 | 1.01 |  | 0.09 | 0.08 | 1.60 | 0.30 | 0.80 | 1.75 | 0.76 |  |
| rope.txt | prose | 11,440 | 4,609 | 0.12 | 0.10 | 0.10 |  |  | 0.00 | 0.00 | 0.01 | 0.00 | 0.00 | 0.00 |  |
| screenshot.png | picture (PNG) | 92,628 | 80,512 | 1.76 | 0.05 | 1.00 | 0.50 | 0.14 | 0.00 | 1.63 | 0.02 | 0.01 | 0.03 | 0.01 |  |
| vitruvian.jpg | picture (JPEG) | 207,500 | 206,966 | 2.16 | 0.10 | 1.01 | 1.01 | 0.02 | 0.01 | 0.26 | 0.00 | 0.00 | 0.00 | 0.00 |  |
| door.jpg | picture (JPEG) | 69,298 | 63,866 | 1.07 | 0.06 | 1.01 |  | 0.00 | 0.00 | 0.03 | 0.01 | 0.01 | 0.02 | 0.01 |  |
| spitfire.obj | mesh | 469,717 | 69,054 | 1.41 | 0.33 | 1.00 |  | 0.02 | 0.01 | 0.28 | 0.15 | 0.13 | 0.21 | 0.11 |  |
| hymn.mid | melody (MIDI) | 1,377 | 236 | 0.01 | 0.10 | 0.01 |  |  | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |  |
| film.mp4 | video (MP4) | 234,633 | 233,874 | 2.26 | 0.10 | 1.06 | 1.02 | 0.03 | 0.01 | 0.31 | 0.05 | 0.05 | 0.06 | 0.02 |  |
| sound.wav | sound (WAV) | 88,244 | 3,742 | 1.60 | 0.06 | 1.29 | 0.08 | 0.20 | 0.00 | 2.15 | 0.01 | 0.00 | 0.01 | 0.01 |  |
| program.exe | program (Windows) | 5,337,607 | 4,275,368 | 2.92 | 1.83 |  |  |  |  |  | 0.33 | 1.28 | 2.52 | 1.05 |  |
| folder.tar | tar | 5,038,080 | 1,540,416 | 5.46 | 0.92 | 1.05 |  | 0.14 | 0.13 | 2.73 | 0.36 | 1.28 | 2.91 | 1.30 |  |
