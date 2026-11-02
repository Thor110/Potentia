# Gliders through a file (2 November 2026)

`tools/shortcut_gliders.py` (IDEAS §19.111, Edward's gliders): the file as a grid wrapped at its
edges; a recipe names gliders (a place and one of four directions each) and a step; Conway's rules
run for as many generations as the grid has rows, each live cell's byte changed by the step. The
search adds one glider at a time, trying every place and direction, each scored by the changed
file's bytes counted by how often each value comes, the gliders' own numbers paid; then the changed
file goes to the shortcut seed. Bytes.

| File | Bytes | W | Gliders found | Shortcut seed without | With the gliders | xz |
| :--- | ---: | ---: | :--- | ---: | ---: | ---: |
| two gliders flying apart, through zeros | 4,096 | 64 | both, exactly ((5, 5) and (5, 37), the same way) | 47 | **15** | 71 |
| two gliders that meet, through zeros | 4,096 | 64 | none | 65 | 65 | 78 |
| the grey picture's first 4 KiB | 4,096 | 256 | none | 2,749 | 2,749 | 2,930 |
| Sieve's program's first 4 KiB | 4,096 | 48 | none | 1,080 | 1,080 | 966 |

The two that meet leave a still-life behind (cells alive for 44 to 46 generations), so the file is
not the sum of each glider's trail: one right glider alone makes it dearer (748 bits, against 526
as it is), and only both together cancel it (38 bits). A search one glider at a time cannot find
them; a search of every pair of places and directions here is about 270 million boards.
