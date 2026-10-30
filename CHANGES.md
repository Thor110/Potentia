# A wave direction for the rope, measured (30 October 2026)

Unzip this over the repository root, on top of the last zip ("rope designs"). It is committed
locally, not pushed. Nothing in Sieve's code changes: there is one new script, one results table
and one IDEAS entry.

## `tools/rope_direction_probe.py` (new)

```
python tools/rope_direction_probe.py FILE... [--width W] [--rows 16] [--pixel P]
```

It reads a file as rows of W bytes, in one of four ways:

- **one direction:** each byte as its difference from one chosen neighbour (left, above,
  above-left or above-right), for the whole file;
- **a direction a stretch:** the neighbour chosen afresh every 16 rows;
- **direction or triangle:** the same, with the rope's triangle (P4) as a fifth choice;
- **the triangle**, measured beside them.

Each remainder is measured by its order-0 entropy (the rope's own coder) and by LZMA2.
`--pixel 3` makes the left neighbour a pixel back in RGB, so it is the same colour.

## The results (`results/rope-direction-2026-10-30.md`, IDEAS §19.108)

Bytes, by order-0 entropy:

| File | xz of the file | triangle | best direction way |
| :--- | ---: | ---: | ---: |
| diagonal (designed) | 848 | 926,317 | 8,964 |
| two directions (designed) | 1,148 | 972,274 | 9,633 (one direction: 597,283) |
| diagonal + noise (designed) | 133,552 | 976,609 | 127,507 |
| hallway door, 1280×720 RGB | 14,220 | 38,167 | 76,325 |
| icon128, RGBA | 11,760 | 32,406 | 29,660 |
| survey table (tsv) | 6,340 | 66,482 | 64,341 |
| the sieve program, first MiB | 372,048 | 963,166 | 907,274 |

- **Designed files:** a direction cancels what the rope could not. Choosing it a stretch at a time
  handles two directions in one file.
- **Real files:** the triangle stays best, or about even, on the pictures. A direction takes off
  3% to 8.5% on the icon, the tables and the program.
- **Against xz:** no remainder came under xz on the file as it is, on any real file.

**So it is not built.** On these files it would not change which way Sieve takes. Your game data
(sprite sheets, textures) is what would decide it, and the probe runs on it as it is. A raw
texture needs `--width` (its width × bytes a pixel) and `--pixel`.
