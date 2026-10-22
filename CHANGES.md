# 7-Zip's encoder, install times, a web dictionary weighed, maps and instructions (22 October 2026)

## Built
- **7-Zip's own LZMA2 encoder as one more candidate** (`Sieve/tools/cli/pack.cpp`,
  `Sieve/third_party/lzma/`, `Sieve/CMakeLists.txt`). Each stream of the newest version is also
  packed by 7-Zip's encoder (from the same 7-Zip 26.03 as the decoders already in the repository),
  and the smaller is kept. Readers need nothing new: it is the same LZMA2. It sits in a library of
  its own that only the packer uses, so the installers stay as small as before. Measured, one
  file's instructions before → after: the `sieve` program 1,892,158 → 1,889,931; a build log
  19,312 → 19,039; one JPEG 150,465 → 150,454. Everything else in the varied set was unchanged,
  because liblzma's stream was already the smaller, or the rope carries the file. Every file
  installs and unpacks through the oracle byte for byte, and the Linux and Mac-style builds make
  exactly the same bytes. I tried all of 7-Zip's settings first; that was no better than this one
  pass, and four times slower.
- **Install times** (`Sieve/tools/sieve_cli.cpp`, `Sieve/client/file_locator.cpp`,
  `Sieve/tools/cli/locate.*`, `help.cpp`). `sieve install` now ends with
  `time      decoded in 0.032 s, checked and written in 0.021 s`, and the hallway's File Locator
  says the same after its "installed …" message. "Decoded" is reading the instructions and
  rebuilding every file's bytes; "checked and written" is checking each file against its SHA-256
  and saving it. A CI check reads the line.

## Measured, not built
- **The web dictionary** (IDEAS §19.92). A 120 KB dictionary learnt from half of 90 real sites,
  tested on 102 pages from the other half: 10% smaller than LZMA2 alone, and level with brotli
  overall (0.5% smaller, smaller on 66 of 102 pages). On small pages like yours brotli stays ahead
  (by 6.6% on 7 of your pages), so it isn't worth a new format yet.

## For you to decide
- **`.map` folded into `.sieve`** (IDEAS §19.93). The 3D node graph stores nothing in a map: it
  lays the graph out afresh each time. What a map adds is files named but not held, edges, notes on
  nodes, and a seal. Since the binary head (v16), those could live in a `.sieve` behind a flag,
  costing ordinary instructions nothing. My recommendation is one format, with old `.map` files
  still readable and convertible.

**Docs:** SPECIFICATIONS §12.2, IDEAS §19.92 and §19.93, HANDOFF, VOICES (your quotes).

To apply: copy `.github/` and `Sieve/` over the repository's top folder. The eight new files in
`Sieve/third_party/lzma/` are 7-Zip's public-domain encoder.
