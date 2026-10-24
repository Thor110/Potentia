# Making Sieve instructions: time by stage (2026-10-24)

Intel64 Family 6 Model 167 Stepping 1, GenuineIntel, 12 threads, Windows 10. `sieve locate FILE --installer` (the newest instructions), each file alone; seconds, from `--timings`. Stages nest in the total, and the pieces' rope includes its own search.

## Search 1 s a file

| File | Kind | Bytes | Made | s | MB/s | rope search | pieces' rope | cutting | rope code | LZMA2 quick | LZMA2 trial | LZMA2 pass 1 | LZMA2 tuned | 7-Zip pass | archives |
| :--- | :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| prose.md | prose | 352,755 | 114,419 | 0.65 | 0.54 | 0.06 | 0.08 | 0.13 | 0.05 | 1.65 | 0.13 | 0.11 | 0.17 | 0.08 |  |
| code.cpp | code | 161,633 | 34,486 | 0.26 | 0.62 | 0.06 |  | 0.01 | 0.06 | 0.09 | 0.05 | 0.04 | 0.07 | 0.03 |  |
| oracle.py | code | 399,245 | 95,144 | 0.52 | 0.77 | 0.08 |  | 0.02 | 0.08 | 0.23 | 0.13 | 0.11 | 0.19 | 0.09 |  |
| table.csv | table | 17,831 | 7,739 | 0.49 | 0.04 | 0.38 | 0.04 | 0.04 | 0.00 | 0.46 | 0.01 | 0.00 | 0.01 | 0.00 |  |
| words.txt | word list | 2,921,105 | 564,462 | 1.34 | 2.18 | 0.17 |  | 0.11 | 0.15 | 1.87 | 0.00 | 0.00 | 0.00 | 0.00 |  |
| rope.txt | prose | 11,440 | 4,609 | 0.02 | 0.53 | 0.01 |  | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |  |
| screenshot.png | picture (PNG) | 92,628 | 80,512 | 0.28 | 0.33 | 0.00 | 0.01 | 0.16 | 0.00 | 1.79 | 0.02 | 0.02 | 0.03 | 0.01 |  |
| vitruvian.jpg | picture (JPEG) | 207,500 | 206,966 | 0.17 | 1.22 | 0.01 | 0.02 | 0.03 | 0.01 | 0.29 | 0.00 | 0.00 | 0.00 | 0.00 |  |
| door.jpg | picture (JPEG) | 69,298 | 63,866 | 0.07 | 1.06 | 0.00 |  | 0.00 | 0.00 | 0.03 | 0.02 | 0.01 | 0.02 | 0.01 |  |
| spitfire.obj | mesh | 469,717 | 69,054 | 0.54 | 0.87 | 0.05 |  | 0.02 | 0.05 | 0.30 | 0.16 | 0.13 | 0.22 | 0.11 |  |
| hymn.mid | melody (MIDI) | 1,377 | 236 | 0.01 | 0.10 | 0.01 |  | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |  |
| film.mp4 | video (MP4) | 234,633 | 233,874 | 0.19 | 1.24 | 0.01 | 0.02 | 0.03 | 0.01 | 0.28 | 0.05 | 0.05 | 0.06 | 0.02 |  |
| sound.wav | sound (WAV) | 88,244 | 3,742 | 1.62 | 0.05 | 1.29 | 0.10 | 0.20 | 0.00 | 2.09 | 0.01 | 0.01 | 0.01 | 0.01 |  |
| program.exe | program (Windows) | 5,337,607 | 4,275,368 | 2.89 | 1.85 |  |  |  |  |  | 0.33 | 1.23 | 2.54 | 1.06 |  |
| folder.tar | tar | 5,038,080 | 1,542,465 | 5.43 | 0.93 | 1.06 |  | 0.15 | 0.13 | 2.72 | 0.36 | 1.28 | 2.82 | 1.28 |  |
