# Making Sieve instructions: time by stage (2026-10-24)

Intel(R) Xeon(R) Processor @ 2.80GHz, 4 threads, Linux 6.18.44-fc-v153. `sieve locate FILE --installer` (the newest instructions), each file alone; seconds, from `--timings`. Stages nest in the total, and the pieces' rope includes its own search.

## Search 1 s a file

| File | Kind | Bytes | Made | s | MB/s | rope search | pieces' rope | cutting | rope code | LZMA2 quick | LZMA2 trial | LZMA2 pass 1 | LZMA2 tuned | 7-Zip pass | archives |
| :--- | :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| prose.md | prose | 350,119 | 113,651 | 0.89 | 0.39 | 0.07 |  | 0.03 | 0.06 | 0.22 | 0.40 | 0.16 | 0.30 | 0.16 |  |
| code.cpp | code | 161,633 | 34,486 | 0.41 | 0.39 | 0.06 |  | 0.02 | 0.06 | 0.11 | 0.14 | 0.06 | 0.12 | 0.07 |  |
| oracle.py | code | 399,245 | 95,144 | 0.96 | 0.42 | 0.10 |  | 0.03 | 0.08 | 0.24 | 0.43 | 0.20 | 0.28 | 0.13 |  |
| table.csv | table | 17,831 | 7,739 | 1.03 | 0.02 | 0.86 | 0.06 | 0.08 | 0.00 | 0.31 | 0.01 | 0.01 | 0.01 | 0.00 |  |
| words.txt | word list | 2,921,105 | 564,462 | 2.60 | 1.12 | 0.39 |  | 0.14 | 0.13 | 2.05 | 0.00 | 0.00 | 0.00 | 0.00 |  |
| rope.txt | prose | 11,440 | 4,609 | 0.03 | 0.37 | 0.01 |  |  | 0.00 | 0.00 | 0.01 | 0.00 | 0.01 | 0.00 |  |
| screenshot.png | picture (PNG) | 92,628 | 80,512 | 0.61 | 0.15 | 0.00 | 0.02 | 0.37 | 0.00 | 1.47 | 0.06 | 0.03 | 0.05 | 0.01 |  |
| vitruvian.jpg | picture (JPEG) | 207,500 | 206,966 | 0.24 | 0.86 | 0.01 | 0.01 | 0.04 | 0.01 | 0.18 | 0.00 | 0.00 | 0.00 | 0.00 |  |
| door.jpg | picture (JPEG) | 69,298 | 63,866 | 0.11 | 0.62 | 0.00 |  | 0.01 | 0.00 | 0.04 | 0.04 | 0.01 | 0.03 | 0.01 |  |
| spitfire.obj | mesh | 469,717 | 69,054 | 1.06 | 0.44 | 0.06 |  | 0.03 | 0.04 | 0.30 | 0.50 | 0.20 | 0.35 | 0.20 |  |
| hymn.mid | melody (MIDI) | 1,377 | 236 | 0.02 | 0.08 | 0.01 |  |  | 0.00 | 0.00 | 0.01 | 0.00 | 0.00 | 0.00 |  |
| film.mp4 | video (MP4) | 234,633 | 233,874 | 0.42 | 0.56 | 0.02 | 0.03 | 0.04 | 0.01 | 0.21 | 0.16 | 0.07 | 0.11 | 0.03 |  |
| sound.wav | sound (WAV) | 88,244 | 3,742 | 2.11 | 0.04 | 1.61 | 0.19 | 0.25 | 0.00 | 0.97 | 0.01 | 0.00 | 0.01 | 0.03 |  |
| program.exe | program (Windows) | 5,337,607 | 4,275,368 | 5.88 | 0.91 |  |  |  |  |  | 1.16 | 2.79 | 4.89 | 2.19 |  |
| folder.tar | tar | 5,038,080 | 1,541,292 | 10.21 | 0.49 | 1.03 |  | 0.24 | 0.63 | 3.15 | 1.09 | 2.73 | 5.50 | 2.35 |  |
