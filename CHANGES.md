# Changes: the listing's head in bytes, `sieve-manifest-v16` (21 October 2026)

Copy `Sieve/` over the repository's `Sieve/`. No new files. Step 3 of the five.

## What changed

The fixed part of the listing is now written as bytes instead of text:

| part | before, as text | now, as bytes |
|---|---|---|
| version line | `sieve-manifest-v15` | 6 bytes |
| which streams there are | their lines | 1 byte of flags |
| stream sizes | decimal text | compact numbers |
| the SHA-256 that checks the files | 64 hex characters | its 32 raw bytes |
| `end` | a line | not needed |

- The per-file lines are still the readable text they were, packed by LZMA2 where that's smaller.
- Reading converts the head back into v15's text, so all the existing reading code is reused.
- `--items` now makes v16. `--v15` keeps the text form.

## Measured here

| | before (text) | now (v16) |
|---|---|---|
| a 1,597-byte script (its listing: 142 → 67 bytes) | 661 | **586** |
| **pi6.txt** | 417,139 | **417,061** |
| the trained model, as one file | 4,257 | **4,185** |
| 64-character pages (CI) | 422 | **345** |

Each one installs byte for byte with `sieve install` and `sieve-install`, and through the oracle.

Your `control.py` came out 10 bytes over 7z; the listing just lost 75 bytes on a similar file, so it
should now come in under 7z. I can't measure it without the file.

## CI

- The checks that read or relabel a listing's text (v7, v12, v13 and v14) now make it with `--v15`.
- New checks: v16 on the degree sample and on the trained model, and a v16 head with an unknown
  flag, refused by Sieve and the oracle.

## Still open

The per-file lines in bytes too, which would help most on folders of many small files. It's noted
as IDEAS §19.90 (your idea of packing into bitmasks), to measure first.

## Checked here

- The full build, with no warnings.
- The hallway CI step, start to end.

I've sent pi6.txt as v16 instructions beside this zip.

**Docs:** SPECIFICATIONS §12.2, IDEAS §19.90, README, HANDOFF, help.
