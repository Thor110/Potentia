# Making Sieve instructions: time by stage (2026-10-24)

Intel(R) Xeon(R) Processor @ 2.80GHz, 4 threads, Linux 6.18.44-fc-v153. `sieve locate FILE --installer` (the newest instructions), each file alone; seconds, from `--timings`. Stages nest in the total, and the pieces' rope includes its own search.

## Search 1 s a file

| File | Kind | Bytes | Made | s | MB/s | rope search | pieces' rope | cutting | rope code | LZMA2 quick | LZMA2 trial | LZMA2 pass 1 | LZMA2 tuned | 7-Zip pass | archives |
| :--- | :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| prose.md | prose | 346,528 | 112,534 | 1.76 | 0.20 | 1.05 |  | 0.02 | 0.06 | 0.21 | 0.33 | 0.14 | 0.26 | 0.14 |  |
| code.cpp | code | 161,633 | 34,486 | 1.37 | 0.12 | 1.05 |  | 0.01 | 0.05 | 0.11 | 0.12 | 0.05 | 0.11 | 0.05 |  |
| oracle.py | code | 399,245 | 95,144 | 1.77 | 0.23 | 1.00 |  | 0.02 | 0.07 | 0.23 | 0.36 | 0.15 | 0.26 | 0.11 |  |
| table.csv | table | 17,831 | 7,739 | 0.91 | 0.02 | 0.76 | 0.05 | 0.08 | 0.00 | 0.30 | 0.01 | 0.00 | 0.01 | 0.00 |  |
| words.txt | word list | 2,921,105 | 564,462 | 3.01 | 0.97 | 1.01 |  | 0.13 | 0.13 | 1.89 | 0.00 | 0.00 | 0.00 | 0.00 |  |
| rope.txt | prose | 11,440 | 4,609 | 0.43 | 0.03 | 0.40 |  |  | 0.00 | 0.00 | 0.02 | 0.00 | 0.00 | 0.00 |  |
| screenshot.png | picture (PNG) | 92,628 | 80,512 | 0.52 | 0.18 | 0.00 | 0.01 | 0.32 | 0.00 | 1.28 | 0.05 | 0.02 | 0.03 | 0.01 |  |
| vitruvian.jpg | picture (JPEG) | 207,500 | 206,966 | 0.25 | 0.82 | 0.01 | 0.01 | 0.03 | 0.01 | 0.17 | 0.00 | 0.00 | 0.00 | 0.00 |  |
| door.jpg | picture (JPEG) | 69,298 | 63,866 | 0.11 | 0.61 | 0.00 |  | 0.01 | 0.00 | 0.04 | 0.04 | 0.01 | 0.02 | 0.01 |  |
| spitfire.obj | mesh | 469,717 | 69,054 | 1.87 | 0.25 | 1.00 |  | 0.03 | 0.04 | 0.27 | 0.40 | 0.17 | 0.34 | 0.18 |  |
| hymn.mid | melody (MIDI) | 1,377 | 236 | 0.02 | 0.07 | 0.01 |  |  | 0.00 | 0.00 | 0.01 | 0.00 | 0.00 | 0.00 |  |
| film.mp4 | video (MP4) | 234,633 | 233,874 | 0.37 | 0.64 | 0.02 | 0.03 | 0.04 | 0.01 | 0.22 | 0.15 | 0.07 | 0.09 | 0.03 |  |
| sound.wav | sound (WAV) | 88,244 | 3,742 | 2.09 | 0.04 | 1.59 | 0.18 | 0.26 | 0.00 | 1.00 | 0.01 | 0.00 | 0.01 | 0.02 |  |
| program.exe | program (Windows) | 5,337,607 | 4,275,368 | 5.26 | 1.01 |  |  |  |  |  | 1.02 | 2.40 | 4.41 | 1.86 |  |
| folder.tar | tar | 5,038,080 | 1,540,784 | 9.44 | 0.53 | 1.03 |  | 0.22 | 0.22 | 3.18 | 0.98 | 2.62 | 4.83 | 2.13 |  |
