# The six, then the smaller things (6 November 2026)

Fourteen commits on top of becb224. Nothing needs a new manifest version. The unit suite passes
(154,324 checks); every new CI line was run here. The roof (item 4) is still measuring and comes in
the next zip.

## The six

| # | Item | Result |
| ---: | :--- | :--- |
| 3 | The reader fuzzed (`tools/fuzz/fuzz_reader.py`, ASan and UBSan) | 45,283 changed `.sieve` files: nothing in Sieve's own code. One shift by 32 inside preflate (pinned third-party code; the release build refuses that file cleanly) is for you to decide on: results/fuzz-2026-11-06.md |
| 1 | Several files chosen at once make one `.sieve` | a folder of just them, named after the folder they are in |
| 5 | A rising bar for mixing over 16 MiB (1%, and 0.5% more for each 100 MB) | only mozilla changes: 0.43% larger, installs in 1.0 s instead of 49.5 s (results/rising-bar-2026-11-06.md) |
| 2 | "Compress to" in the right-click menu, and `sieve compress` | zip, 7z, xz, gz, bz2, zst, lz4, br, lzma, tar; each read back before it is saved |
| 4 | The roof, 1 MB to 1 GB | running now on enwik9 (fetched for it); next zip |
| 6 | Names written two ways (NFC and NFD) count as one name on macOS | the packer warns, macOS refuses |

## The smaller things

- **Hallway:**
  - the wheel spun fast goes further (up to 1,024 tiles a notch);
  - the navigator shows the place in decimal too;
  - every "along the line" says how far is still to go;
  - a picture file's card in CHAT shows its exchange rate (the file 1,660,000 bits, full size
    29,608,800, fitted to the image line 100, for Vitruvian.jpg).
- **CHAT:**
  - `/random page|book|world|model|ai|track|video|movie|file`;
  - `/docs`, `/docs NAME`, `/docs NAME SECTION`. The documents are found beside the program or in
    a folder above it. A release does not carry them yet; whether it should is your call.
- **Harness:** "+ new melody model..." (or M), as a picture model is made.
- **AI:** `widen_weights = auto` widens a model only if, widened, it fits ITEM MEMORY's share of
  memory.
- **Accessibility:**
  - `tools/colour_check.py`. One pair is lost: tracks and movies under deuteranopia (58.9 normally,
    2.4). The ways out are measured in IDEAS §2.4 for you to choose. No colour has been changed.
  - **Read menus aloud** (Settings, Controls; off by default) speaks the main menu's and pause
    menu's chosen item with Windows' own voice.
- **Build:** `tools\build_release.bat` builds with only Visual Studio, then makes the release where
  Python is installed.
- **Music:** the detailed description is in IDEAS §7.8, and nothing is built yet. It covers the
  tile's melody chosen by the place's hash among the survivors, when it changes, a setting, and
  what changes a place's tune.

## For you to try on Windows (none of this could be run here)

- **The right-click menu:** run `context\install_context.bat` again, then right-click a file, a
  folder, or several. "Compress to" should give `NAME.zip`, `NAME.7z`, `NAME.tar.xz` and so on
  beside them, in a console window that closes by itself. The batch files are CRLF.
- **Read menus aloud:** Settings, Controls. It uses SAPI through COM and has never been compiled on
  Windows: watch for build errors in `client/speech.cpp`.
- **`tools\build_release.bat`:** double-click it.
- **Names:** `sieve compress` with a name that has an accent, to zip and 7z. Both are read back
  here, and 7-Zip reads them.

## Notes

- The fuzzer's ASan build lived in its own folder with its own `_deps`, and the main build was
  checked clean of sanitizer symbols.
- New files:
  - `tools/cli/compress.cpp/.hpp`, `tools/cli/gather.cpp/.hpp`;
  - `client/speech.cpp/.hpp`;
  - `tools/colour_check.py`, `tools/fuzz/`, `tools/build_release.bat`.
