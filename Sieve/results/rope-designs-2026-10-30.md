# The rope's designed files

Made by `tools/rope_designs.py`, the rope searching 5 s a file.

| File | What it is | Bytes | Order-0 entropy | xz -9e | The rope | Its best wave | `.sieve` |
| :--- | :--- | ---: | ---: | ---: | ---: | :--- | ---: |
| checker | 00 and FF along the diagonals (Edward's example) | 1,048,576 | 131,072 | 292 | 24 | `L8 W512 S2 P3 K0` | 101 |
| diagonal | a random first row carried down the diagonals (each row the one above, moved one along) | 1,048,576 | 943,399 | 848 | 14,173 | `L8 W257 S2 P4 K0 T1` | 1,556 |
| vertical | a random first row carried straight down (each row the one above) | 1,048,576 | 943,399 | 552 | 1,350 | `L8 W256 S1 P1 K0 C1 G1` | 532 |
| gradient | each byte its row plus its column (a smooth slope) | 1,048,576 | 1,048,576 | 764 | 1,347 | `L8 W255 S1 P4 K0 T1` | 729 |
| diagonal+noise | the diagonal file with one byte in ten moved by one | 1,048,576 | 971,838 | 133,552 | 165,847 | `L8 W257 S2 P4 K0` | 85,276 |
| diagonal-250 | the diagonal carry in rows of 250 bytes (a width that does not divide 1 MiB) | 1,048,576 | 933,158 | 876 | 14,522 | `L8 W251 S2 P4 K0 T1` | 831 |
| two-directions | the top half carried down-right, the bottom half down-left | 1,048,576 | 948,435 | 1,148 | 175,074 | `L32 W16384 S3 P4 K0 T1` | 1,131 |
| noise | random bytes (the control: nothing can be taken off) | 1,048,576 | 1,048,554 | 1,048,688 | 1,049,589 | `L8 W1 S1 P2 K1` | 1,048,639 |
