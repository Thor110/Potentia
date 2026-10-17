# Changes: the build error fixed, and the trained model on the record (17 October 2026)

Copy `Sieve/` over the repository's `Sieve/`.

- **Fixed: `'executable_dir': is not a member of 'sieve::cli'`** (`client/app_main.cpp`, the
  `--metrics-text` lookup): the missing `#include "cli/dictionaries.hpp"`. I'd compile-checked
  the other changed files, but not this one after its last edit. This time the whole hallway was
  built here (Release, SDL3 from source, no warnings), and every CI line added this session was
  run against it. All pass.
- **The models Sieve trained, against their files** (`client/menu.cpp`: `Menu::record_metrics`):
  a new last part of the metrics list, measured live on every build.
  - universalbibliothek: its address on the AI line is 32,768 bits; `model.safetensors` (as F saves
    it) is 272,576; zip's deflate 58,256; 7z's LZMA2 44,608. So the address is **12.0% of the
    file, and 73.5% of the best compressor's result**.
  - To keep it unbiased, a model drawn at random from the same line (fixed seed) is weighed beside
    it: address 32,768, file 272,576, LZMA2 45,056. Every model of the line has the same address
    length. The saving comes from knowing the line's shape (the file stores 4-bit weights as
    32-bit floats), not from the training.
  - The line's id, which a reader must also be given (360 bits), is shown.
  - CI pins the address and file figures. The compressors' sizes depend on the zlib and liblzma
    versions linked, so CI doesn't pin them.
- **Your two statements of aim** are in `docs/VOICES.md`: reduce files to their theoretical limit
  and prove it possible, even through absurdly strict filtering; keep metrics unbiased but
  informative.
- **README** (the new part of the metrics list) and **HANDOFF** (the entry, and a rule: build the
  hallway whole before handing over a change to it).
