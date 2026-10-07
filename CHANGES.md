# Installers packed: sieve-manifest-v4

Against `f32d3ee` (weigh and full).

## Why compression is built in
A release's files (programs, word lists, filters, models) are not Sieve items, so the weighing's
ways alone would not have changed the release installer at all. I measured that, asked, and you
chose compression built into the installer. So the installer now packs the folder itself, and the
release no longer goes through 7-Zip first.

## The format: sieve-manifest-v4
- **The listing:** v3's, with two `stream` lines (x86 and lzma2: size packed, size unpacked,
  dictionary) and each file's way before its path.
- **The ways:**
  - `x86`: programs (EXE and ELF, by their first bytes), through the x86 branch filter, then
    LZMA2;
  - `raw`: everything else, LZMA2;
  - `lines K`: a text made of some of file K's lines, in order, carried as a mask of K's lines,
    one bit each. Each SCOWL list but the largest is a mask over the largest.
- **After `end`:** the two streams, raw LZMA2, at preset 9 extreme.
- **Safety:** what's packed is unpacked and compared before it's used. Where v4 would be no
  smaller (a tiny folder, files already compressed), the installer is v3, with the files as they
  are.

## One decoder everywhere
The LZMA SDK decoder that `sieve-install` already carried for 7z now sits in the shared install
code. `sieve`, the hallway and `sieve-install` all unpack with it. `sieve-install` stays exactly
the same size: 2,038,584 bytes on Linux.

## Where it applies
- **`sieve locate --installer` and `--program`** make v4. `--v3` makes the old kind.
- **The File Locator** saves and measures v4.
- **`make_release.py`** packs the staged release folder, with no 7z step; 7-Zip is still used for
  the source archive. `--seven-zip` makes the old 7z-carrying installers from the same build, for
  your comparison.
- **`sieve-install` and `sieve install`** install v4, every file checked against its SHA-256.
  Older installers (v3, v2, a carried 7z) install as before.

## Measured (Linux, a release-shaped folder, 32,887,678 bytes)
| Made as | Bytes | Share |
| :--- | :--- | :--- |
| zip, each file on its own | 11,597,734 | 35.26% |
| one LZMA2 stream, preset 9e (7z without its container) | 6,961,919 | 21.17% |
| **v4 installer** | **6,577,790** | **20.00%** |

The x86 filter takes 258 KB off the programs, and the five smaller SCOWL lists shrink to 180 KB of
masks. Running `make_release.py` here gave a `sieve.sieve` of 6,613,564 bytes for the full release
folder, and both installers install back to it byte for byte.

**For your test on Windows:** 7-Zip's Ultra uses BCJ2 on programs, which does a little better
than the x86 filter, so I can't promise the margin over your real 7z. Run
`make_release.py --seven-zip` beside the default run on the same build and weigh the two
installers directly.

## Checked
- **Build:** no warnings (Linux, GCC 13), tools and client.
- **Unit tests:** 47,426 checks, 0 failures.
- **Oracle step:**
  - v3 is still the oracle's to the byte (with `--v3`);
  - a v4 fixture with a program and a derived list is unpacked by the oracle's own Python `lzma`
    unpacker (`sieve_ref.py unpack`, nothing of Sieve's) and by `sieve install`, both identical;
  - a damaged stream is refused, with nothing written.
- **Hallway step, run here in full:** `sieve-install --yes` installs a v4 with a program and a
  derived list, identically.
- **Release-shaped folder:** installed by `sieve install` and by `sieve-install`, both identical to
  the original.

## Found and fixed on the way
A file located without a folder in its name (`--locate big.bin`) gave the packer an empty root, so
the File Locator failed on it. The packer now uses the current folder, and needs no folder at all
when it already has the bytes. The hallway step caught it.

## Docs
- **SPECIFICATIONS §12.2:** v4.
- **SIEVE-INSTALL-USAGE.md:** rewritten around packing, with the measurement; its old "Not yet"
  item, compression inside the installer, is done.
- **README.**
- **`sieve help locate`.**
- **HANDOFF:** an entry, and the file map.
- **IDEAS §12.**
- **`make_release.py`'s docstring, and `.bat`.**

## Still open
- **Ways for Sieve items** (a page or melody as its place on its line, which is what the weighing
  measures). These need the lines in `sieve-install`.
- **"Made from another file" ways beyond lines**, such as a delta.
