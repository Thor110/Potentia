# The two mixing orders, and your two code ideas measured (2 November 2026)

Unzip this over the repository root, on top of the last zip ("sieve-ramps-review"). Two more local
commits, 1295bb1 and 490dbf4, not pushed. The unit suite passes (77,133 checks, 0 failures). The
packer's default results are unchanged.

## The two mixing orders

`SIEVE_MIX_ORDER=stream-first` is a new testing setting. It weighs the files mixed alone against the
shared stream before the groups are made, and skips the groups if the stream takes the files back.
Without it, everything is as before.

| Folder | Default (bytes, s) | Stream first (bytes, s) |
| :--- | :--- | :--- |
| Sieve's docs/, 2.6 MB | 1,074,255, 34.0 | 1,074,255, 34.6 |
| Sieve's sources, 2.5 MB | 378,949, 18.3 | 378,908, 23.7 |
| public-domain anchors, 3.0 MB | 999,994, 27.7 | 999,994, 29.6 |
| 16 programs, 60 MB | 16,485,171, 260.3 | 16,485,171, 239.2 |
| mozilla taken apart, 51 MB | 13,371,335, 407.7 | 13,371,335, 429.0 |

The sizes are the same or within 41 bytes. Stream first is quicker on the programs and slower on
the rest, so neither order wins: no checkbox and no change to the default for now. The setting is
there to try on your folders.

The figures, with mozilla's breakdown, are in `results/weighing-order-2026-11-02.md`.

## Your two code ideas, measured and noted (IDEAS §19.113)

- **Operators as mortar.** Each operator's place still has to be carried, and the operator itself
  is the cheapest marker of it. pack.cpp by xz: 61,316 bytes as it is, 61,305 with a one-byte
  separator, 61,703 with block lengths.
- **Code cut at its operators, each block packed.** Packed alone, the blocks come to 3.6 to 4.1
  times the whole file. Gathered by operator, 28% to 41% larger. Code's context runs across its
  operators.
