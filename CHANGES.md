# v33 draft: mixing's long reach; gliders; ways in full (commit 63d53be)

**Sieve itself changed:** a rebuild is needed. Without `--v33-draft`, everything Sieve makes is v32,
exactly as before.

## 1. Mixing's match model with its ways: the v33 draft's first piece

mixing's model of earlier copies already reached across the whole file. What it lacked was three
things, now mix-v4 settings (each off unless written, so every older code reads the same):

- **`R1`:** a match goes on past one wrong byte, at the same distance, as LZMA's last distance does.
  Programs are full of the same instruction with one field changed.
- **`M4`:** a match is taken after 4 matching bytes, not 6.
- **`C2`:** two places each context was seen, the longer match kept.

| | LZMA2 | mix-v4 | **with R1 M4 C2** |
| :--- | ---: | ---: | ---: |
| Silesia's mozilla, mixed whole | 13,376,180 | 13,778,604 | **13,318,505** (under LZMA2 for the first time) |
| 8 MiB of its Alpha programs | 1,912,125 | 1,959,809 | **1,891,229** |
| 4 MiB of Sieve's own program | 1,511,013 | 1,343,529 | **1,297,930** |
| Don Quixote | 685,794 | 537,483 | 544,027 (C2 alone: 536,458) |

They help programs and cost text and pictures a little, so the packer weighs them.

- **Switching it on:** `sieve locate ... --v33-draft`. Each mixed file's start is tried with none,
  with `C2`, and with all three, and the smallest is kept. The `.sieve` is `sieve-manifest-v33`
  only where some file uses them.
- **Mozilla still needs Try every way:** a file is mixed whole only where mixing on its start is 3%
  under LZMA2, unless Try every way is on, and mozilla is over mixing's 16 MiB. So with
  `--v33-draft` alone it stays LZMA2; add `--search-every-wave` to have it mixed. Whether that 3%
  test should allow for the new settings is in ODDITIES.
- **Through the packer, with Try every way:** 2 MiB of mozilla's programs come to 493,215 at v32
  and 481,476 at v33; 64 KiB of Sieve's program to 22,563 and 21,118.
- **Checked:** Sieve, sieve-install and the oracle install v33, and anything older that names the
  new settings is refused.
- **CI step 11** has a new block. I ran it, the unit suite (77,110 checks, 0 failures) and the
  older mixing blocks here; please run the full CI, as usual.
- **Not done yet:** a File Locator box for the draft (only the command line has it).

## 2. Gliders (`tools/shortcut_gliders.py`, `results/shortcut-gliders-2026-11-02.md`)

- **Gliders flying apart:** found exactly, the file 15 bytes against xz's 71. That's only on a file
  the gliders made.
- **Gliders that meet:** they leave a still-life, which a search one glider at a time can't undo,
  and a search of pairs is about 270 million boards.
- **Real files:** nothing.

## 3. Benchmarks: "Ways in full"

`bench_files` now writes each run's exact way and settings, read back from its `.sieve` (for
example `stb 8 -1 1280 720 3 2764800 mix4 B22 I1280x3`), beside "Carried by". So the formula behind
every result is kept without keeping the logs.

## 4. Your JPEG run, recorded (ODDITIES, IDEAS §19.111)

- **153 files:** size/try −3.20% against 7z. decode/no −0.37%, 9.4 s to install the lot.
- **decode/try:** the same bytes as decode/no, in 1.24 h instead of 5 minutes.
- **The rope** led on 106 of them for decoding speed.
- **New oddity:** five small JPEGs were 18 to 225 bytes larger with Try every way. Mixing was
  weighed against LZMA2's estimate rather than the stream as made. It's a packer-only fix.
