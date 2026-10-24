# Making Sieve instructions: time by stage (2026-10-24)

Intel64 Family 6 Model 167 Stepping 1, GenuineIntel, 12 threads, Windows 10. `sieve locate FILE --installer` (the newest instructions), each file alone; seconds, from `--timings`. Stages nest in the total, and the pieces' rope includes its own search.

## Search 1 s a file

| File | Kind | Bytes | Made | s | MB/s | rope search | pieces' rope | cutting | rope code | LZMA2 quick | LZMA2 trial | LZMA2 pass 1 | LZMA2 tuned | 7-Zip pass | archives |
| :--- | :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| prose.md | prose | 351,395 | 114,023 | 0.44 | 0.80 | 0.06 |  | 0.01 | 0.05 | 0.22 | 0.12 | 0.11 | 0.17 | 0.07 |  |
| code.cpp | code | 161,633 | 34,486 | 0.25 | 0.66 | 0.06 |  | 0.01 | 0.06 | 0.09 | 0.05 | 0.04 | 0.07 | 0.03 |  |
| oracle.py | code | 399,245 | 95,144 | 0.52 | 0.76 | 0.08 |  | 0.01 | 0.08 | 0.21 | 0.13 | 0.12 | 0.20 | 0.09 |  |
| table.csv | table | 17,831 | 7,739 | 0.47 | 0.04 | 0.38 | 0.04 | 0.03 | 0.00 | 0.37 | 0.01 | 0.00 | 0.01 | 0.00 |  |
| words.txt | word list | 2,921,105 | 564,462 | 1.21 | 2.41 | 0.17 |  | 0.08 | 0.13 | 1.53 | 0.00 | 0.00 | 0.00 | 0.00 |  |
| rope.txt | prose | 11,440 | 4,609 | 0.02 | 0.52 | 0.01 |  |  | 0.00 | 0.00 | 0.01 | 0.00 | 0.00 | 0.00 |  |
| screenshot.png | picture (PNG) | 92,628 | 80,512 | 0.26 | 0.36 | 0.00 | 0.01 | 0.14 | 0.00 | 1.55 | 0.02 | 0.01 | 0.02 | 0.01 |  |
| vitruvian.jpg | picture (JPEG) | 207,500 | 206,966 | 0.15 | 1.40 | 0.01 | 0.01 | 0.02 | 0.01 | 0.26 | 0.00 | 0.00 | 0.00 | 0.00 |  |
| door.jpg | picture (JPEG) | 69,298 | 63,866 | 0.06 | 1.08 | 0.00 |  | 0.00 | 0.00 | 0.03 | 0.01 | 0.01 | 0.02 | 0.01 |  |
| spitfire.obj | mesh | 469,717 | 69,054 | 0.52 | 0.90 | 0.05 |  | 0.02 | 0.05 | 0.28 | 0.15 | 0.13 | 0.25 | 0.10 |  |
| hymn.mid | melody (MIDI) | 1,377 | 236 | 0.01 | 0.10 | 0.01 |  |  | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |  |
| film.mp4 | video (MP4) | 234,633 | 233,874 | 0.19 | 1.25 | 0.01 | 0.02 | 0.03 | 0.01 | 0.28 | 0.05 | 0.05 | 0.05 | 0.02 |  |
| sound.wav | sound (WAV) | 88,244 | 3,742 | 1.61 | 0.05 | 1.30 | 0.09 | 0.19 | 0.00 | 2.05 | 0.01 | 0.00 | 0.01 | 0.01 |  |
| program.exe | program (Windows) | 5,337,607 | 4,275,368 | 2.89 | 1.84 |  |  |  |  |  | 0.34 | 1.22 | 2.56 | 1.04 |  |
| folder.tar | tar | 5,038,080 | 1,542,117 | 5.47 | 0.92 | 1.04 |  | 0.15 | 0.13 | 2.80 | 0.37 | 1.31 | 2.81 | 1.28 |  |
