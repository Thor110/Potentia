# Making Sieve instructions: time by stage (2026-10-25)

Intel64 Family 6 Model 167 Stepping 1, GenuineIntel, 12 threads, Windows 10. `sieve locate FILE --installer` (the newest instructions), each file alone; seconds, from `--timings`. Stages nest in the total, and the pieces' rope includes its own search; cutting includes the blocks and the boundaries.

## Search 1 s a file

| File | Kind | Bytes | Made | s | MB/s | rope search | pieces' rope | cutting | blocks | boundaries | rope code | LZMA2 quick | LZMA2 trial | LZMA2 pass 1 | LZMA2 tuned | 7-Zip pass | archives |
| :--- | :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| prose.md | prose | 365,945 | 118,586 | 0.49 | 0.74 | 0.06 |  | 0.01 | 0.01 |  | 0.05 | 0.23 | 0.15 | 0.14 | 0.19 | 0.09 |  |
| code.cpp | code | 162,218 | 34,662 | 0.27 | 0.59 | 0.06 |  | 0.01 | 0.01 |  | 0.06 | 0.09 | 0.05 | 0.05 | 0.08 | 0.04 |  |
| oracle.py | code | 403,875 | 95,955 | 0.59 | 0.69 | 0.09 |  | 0.02 | 0.02 |  | 0.09 | 0.27 | 0.14 | 0.14 | 0.21 | 0.10 |  |
| table.csv | table | 17,831 | 7,739 | 0.50 | 0.04 | 0.43 | 0.04 | 0.02 | 0.00 | 0.02 | 0.00 | 0.13 | 0.01 | 0.00 | 0.01 | 0.00 |  |
| words.txt | word list | 2,921,105 | 564,462 | 1.55 | 1.88 | 0.20 |  | 0.09 | 0.09 |  | 0.15 | 1.85 | 0.00 | 0.00 | 0.00 | 0.00 |  |
| rope.txt | prose | 11,440 | 4,609 | 0.02 | 0.53 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |  |
| screenshot.png | picture (PNG) | 92,628 | 80,512 | 0.18 | 0.52 | 0.00 | 0.01 | 0.04 | 0.01 | 0.03 | 0.00 | 0.30 | 0.03 | 0.02 | 0.03 | 0.01 |  |
| vitruvian.jpg | picture (JPEG) | 207,500 | 206,966 | 0.17 | 1.20 | 0.01 | 0.02 | 0.01 | 0.01 | 0.01 | 0.01 | 0.16 | 0.00 | 0.00 | 0.00 | 0.00 |  |
| door.jpg | picture (JPEG) | 69,298 | 63,866 | 0.07 | 1.05 | 0.00 |  | 0.00 | 0.00 |  | 0.00 | 0.03 | 0.01 | 0.01 | 0.02 | 0.01 |  |
| spitfire.obj | mesh | 469,717 | 69,054 | 0.62 | 0.76 | 0.05 |  | 0.02 | 0.02 |  | 0.07 | 0.35 | 0.17 | 0.16 | 0.26 | 0.14 |  |
| hymn.mid | melody (MIDI) | 1,377 | 236 | 0.02 | 0.09 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |  |
| film.mp4 | video (MP4) | 234,633 | 233,874 | 0.25 | 0.96 | 0.01 | 0.02 | 0.02 | 0.01 | 0.01 | 0.01 | 0.18 | 0.06 | 0.06 | 0.08 | 0.02 |  |
| sound.wav | sound (WAV) | 88,244 | 3,742 | 1.75 | 0.05 | 1.53 | 0.09 | 0.10 | 0.00 | 0.10 | 0.00 | 0.66 | 0.01 | 0.01 | 0.01 | 0.01 |  |
| program.exe | program (Windows) | 5,337,607 | 4,275,368 | 3.33 | 1.60 |  |  |  |  |  |  |  | 0.38 | 1.51 | 2.75 | 1.15 |  |
| folder.tar | tar | 5,058,560 | 1,547,309 | 6.46 | 0.78 | 1.06 |  | 0.16 | 0.16 |  | 0.13 | 3.21 | 0.41 | 1.68 | 3.39 | 1.52 |  |
