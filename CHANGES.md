# The Encryption Engine, stage 1, and the voices on the website (13 October 2026)

## The Encryption Engine, stage 1 (`cipher-v1`)
- **New:** `Sieve/core/include/sieve/cipher.hpp`, `Sieve/core/src/cipher.cpp`. A recipe of layers
  on bytes, undone in reverse:
  - `reverse`, `caesar N`, `xor HEX`, `vigenere TEXT`;
  - `feistel TEXT`: the binary line's own keyed scramble, so n bytes become n bytes
    (format-preserving);
  - `vigenere-hex` and `feistel-hex` for keys that are not plain text.
- **Breaking the weak ones:** every caesar shift and one-byte XOR, plus repeating XOR and Vigenère
  keys up to 16 bytes, fitted column by column. They are ranked by an English score less 1500 for
  each key byte beyond the first. Caesar, XOR and Vigenère come out first; a feistel layer gives
  nothing English.
- **New command, `sieve cipher`** (`Sieve/tools/cli/cipher_cmd.cpp`): `--encrypt`, `--decrypt`,
  `--break` and `--catalogue`. The input is `--in FILE`, `--text TEXT` or `--in-hex HEX`.
  - Registered in `CMakeLists.txt`, `sieve_cli.cpp`, `args.hpp`, `train.hpp` and `help.cpp`.
- **The catalogue:** 130 bits name one layer with keys of up to 16 bytes, counted exactly.
- **Oracle:** `sieve_ref.py cipher-vectors` writes `tests/vectors_cipher_v1.tsv` (55
  encryptions, 15 guesses).
  - `test_core` checks the vectors, round trips and refused recipes: 76,549 checks, 0 failures.
  - CI diffs the oracle's output against the vectors. It also runs the CLI: an encryption and back,
    a broken XOR, a feistel layer left unbroken, and the catalogue.
  - `tools/check_vectors.py` includes the new vectors.
- **Docs:**
  - SPECIFICATIONS §12.0i (`cipher-v1`);
  - README (`cipher`);
  - IDEAS §19.52 "Built (stage 1)", and the new §19.68;
  - a HANDOFF entry.

## Website: a voice from the halls
- **Where:** `Pages/release.html`, at the very bottom, under the buy and download buttons. The
  disclosure stays at the top.
- **What it does:** shows one voice from `Sieve/docs/VOICES.md` at random, with who said it and
  when, and changes it every 12 seconds. "Another Voice" picks a new one.
- **Read aloud:** "Read Aloud" speaks each voice in the browser's own British English voice once
  it is turned on. Browsers do not let a page speak unasked.
- **Where the voices come from:** the page reads the file itself, so a voice added to VOICES.md
  is on the site with no change to the page. It tries `../Sieve/docs/VOICES.md`, then the
  repository's raw copy on GitHub. Opened from disk with neither reachable, the box stays hidden.

## For you to run
- Build Debug and Release, then `tools\run_tests.bat`.
- `sieve cipher --catalogue --depth 2`, and a round trip such as:
  `sieve cipher --encrypt --recipe "vigenere lemon; feistel my key" --text "attack at dawn"`
  followed by `--decrypt` with the printed hex as `--in-hex`.
- Open Buy or Build from the site and turn on "Read Aloud".

## Next
The Encryption Engine screen in the game, on the main and pause menus, with each layer's output an
item.
