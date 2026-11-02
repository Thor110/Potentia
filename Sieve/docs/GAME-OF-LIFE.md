# The Game of Life as a filter

Edward's idea (1 November 2026, IDEAS §19.111): "what chaos a few gliders might cause through a file
from the game of life, or how the rules from Conway's game of life might be turned into a random
filter: pick some random bytes, fire them off, have them alter values as they go through the bytes
with a +1/-1 type schema ... making sure to wrap the value around". "Probably the most chaotic one I
can think of for sure, perhaps even useless." This page keeps what has been tried and measured, and
what is still to try. Nothing here is in Sieve: it is the shortcut seed's exploratory work
(`tools/shortcut_gliders.py`), measured as the rest is.

## How it works

- **The board:** the file laid out as a grid of rows of W bytes, its edges wrapped (a torus), so a
  glider leaving one side comes back on the other.
- **The recipe:** a few gliders, each a place (row and column) and one of four directions; and a
  step. On bytes the step wraps round, so it is 1 to 255 (a step of 256 is the byte itself; ±256
  counts only on 16-bit values).
- **The run:** Conway's rules (a cell is born with three live neighbours and lives on with two or
  three) for as many generations as the grid has rows. Each generation, every live cell's byte has the
  step added.
- **Undoing it:** what the gliders change depends on the gliders and the rules alone, never on the
  file, so installing makes the same board and takes the same changes away. (Life itself cannot be
  run backwards, as many boards come to the same next board, so the file must never steer it.)
- **What it costs:** how many gliders (2 bits), the step (8), and each glider's row, column and
  direction (about 6 + 6 + 2 bits on a grid of 64 by 64).

## The search

One glider at a time, at every place and in every direction, each scored by what the changed file
would cost (its bytes counted by how often each value comes), the best kept, then the next, up to
four, for a step of +1 and of −1. Then the changed file is given to the shortcut seed, and the
gliders' own bits are paid.

## Measured (2 November 2026, `results/shortcut-gliders-2026-11-02.md`)

| File | Bytes | Gliders found | The seed without | With the gliders | xz |
| :--- | ---: | :--- | ---: | ---: | ---: |
| two gliders flying apart through zeros (64 by 64) | 4,096 | both, exactly | 47 | **15** | 71 |
| two gliders that meet, through zeros | 4,096 | none | 65 | 65 | 78 |
| the grey picture's first 4 KiB | 4,096 | none | 2,749 | 2,749 | 2,930 |
| Sieve's program's first 4 KiB | 4,096 | none | 1,080 | 1,080 | 966 |

- **What the 15 bytes are:** not a page of zeros (that costs a few bytes already, as one run), but
  a page of zeros through which two gliders were fired, their trails left in it. The search found the
  two gliders that drew it, so the file is named by its cause: where the gliders started and which
  way they flew.
- **When gliders meet:** two gliders from random places met and left a still-life behind (cells alive
  for 44 to 46 generations). What they made is then not the sum of each one's trail: one right glider
  alone makes the file dearer (748 bits against 526 as it is), and only both together cancel it (38
  bits). A search one glider at a time cannot find them; every pair of places and directions here is
  about 270 million boards. That is the chaos Edward foresaw, and the combinatorial explosion with it.
- **Real files:** no glider paid on the picture or the program.

## Still to try

- **More of Life:** other patterns than gliders (spaceships, oscillators, glider guns, which fire a
  stream of gliders from one place, so a few numbers name many trails); other rules than Conway's
  (Life-like rules, B36/S23 "HighLife" and others, each a rule number of a few bits); more
  generations than the grid has rows; the board's state, not only its trails, as the change.
- **Searching pairs and more:** a search that tries gliders together where they meet, guided rather
  than whole (the places where the file's bytes change most first; a glider's trail matched against
  the file before the board is run).
- **Which data might hold it:** files made by cellular automata themselves (game levels, generated
  textures, simulations' saved states), where a rule and a seed are the file's real cause.
- **As a step among others:** gliders before the seed's other shortcuts, or after them, on what they
  leave; each measured on its own, then together, the gliders' bits always paid.
