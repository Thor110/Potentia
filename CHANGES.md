# The shortcut seed begins; oddities recorded (commits 9cfa73e, 902d090, f79981a)

Nothing in Sieve itself changed: these are new tools, results and notes. Unzip over the repository.

## New tools

- **`tools/shortcut_seed.py`**: the shortcut seed, stage 1. A small language of shortcuts to a file:
  - the shortcuts: copy an earlier stretch, the same distance again, a counting run, a grid copy
    (above, above-left, above-right), and your skip-and-insert;
  - the transforms in front of them: differences, the rope's four waves, read by columns, or the
    values a file uses;
  - each recipe searched for the fewest bits, decoded again and checked byte for byte;
  - measured beside zlib, xz, the rope and mixing, and in both of your directions (the seed on what
    beat it, and a transform in front of xz and mixing).

  Run:
  `python tools/shortcut_seed.py PATH\TO\sieve.exe FILE... --out FOLDER`

  Each file is cut to its first 32 KiB by default (`--cut`). It's a Python prototype, slow:
  minutes a file.
- **`tools/shortcut_tiny.py`**: stage 1b.
  - Every file of four sets of 65,536 small files, searched whole.
  - Real files' 2-, 3- and 4-byte arrangements counted by kind, then tried on held-out halves.

## Results (`results/`)

- **`shortcut-seed-2026-11-01.md` and `.tsv`** (24 files, the repository's own):
  - Designed files: under every other way on several (the gradient 13 bytes, against xz's 401).
  - Text and code: within 2 to 5% of xz, behind mixing.
  - Seeds on what beat it: no shortcut found into xz's or mixing's output.
- **`shortcut-arrangements-2026-11-01.md`**: the tiny sets. Regular arrangements come cheapest, but
  every file costs more than 16 bits, because of the framing.
- **`shortcut-counting-2026-11-01.md`**: real files' arrangements by rank cost 4.7 to 5.6 bits a
  byte, against 8. The shuffle carries over only between kinds that are alike.
- **Your 20-byte example** (`00 01 02 0F ...`): found as one shortcut, 7 bytes, as you worked it.

## Notes

- **`docs/ODDITIES.md`**, new:
  - Open: mozilla. Mixing the whole tar is 3% over LZMA2, so a longer-reaching copy model is wanted.
    Also some of your JPEGs, to fill in.
  - Resolved: control.py (591 bytes at v32, 28.10% under its 7z), the Try-every-way group bug, the
    release script's smaller `.sieve`, and the tar sizes.
  - Measured: the floor, between 192 and 256 bytes for text.
- **IDEAS §19.111** records the plan and your ideas:
  - geometric filters and gliders;
  - recording what carried each result;
  - reading a `.sieve` back (light and full);
  - the world's energy cost and saving, and "Sieve Power Savings" on the site;
  - the picture viewer and its checks;
  - Optimise Search;
  - and every measurement so far.

## Still running

The stage 1 rerun with skip-and-insert, columns and palette. Its table comes in the next zip.
