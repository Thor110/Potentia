# Hex and lists of byte values carried decoded (2 November 2026)

Unzip this over the repository root, on top of your commit a6af023 ("sieve base64"). One local
commit, not pushed: a85d6ea. The unit suite passes (77,133 checks, 0 failures). The changed CI
block in step 11 passes here; the full steps 10 and 11 haven't been run on it.

## What changed (v33 draft only)

The base64 way now covers the other ways binary is written as text:
- **Hex written straight through:** `xxd -p` dumps, hashes, blobs. One line or lines of one width;
  small letters or capitals.
- **Lists of byte values:** C arrays (`0x7f, 0x45,` as `xxd -i` writes), escaped strings
  (`\x7f\x45`), spaced hex (`7f 45 4c`, as `od` writes) and decimal lists (`[127, 69, 76]`). Each
  list has a prefix, a separator and a line break every so many values.

**The recipe is renamed** from `b64` to `spn` (spans), since it isn't only base64 now. The draft
isn't frozen, so I renamed rather than add a second recipe; anything made with the last zip's draft
build won't read. The format number (24) stays.

As before, each span is taken only where writing it again gives exactly the text, and a file is
carried so only where it comes out clearly smaller.

## Measured (bytes; v32 → the draft)

The same 101,888-byte program written four ways. The program packed as itself comes to 41,667.

| Written as | Size | v32 | The draft |
| :--- | ---: | ---: | ---: |
| plain hex, lines of 64 (`xxd -p`) | 206,960 | 48,314 | 41,571 |
| a C array (`xxd -i`) | 628,377 | 52,065 | 41,639 |
| spaced hex (`od -An -tx1`) | 312,032 | 46,682 | 41,576 |
| a JSON list in decimal | 421,105 | 48,124 | 41,606 |

In the repository: the test vectors (`tests/vectors_*.tsv`, 1.5 MB) 263,921 → 259,086; the icon's
pixels (`client/icon_pixels.h`) 5,395 → 4,267; the melody training data left as it was. The base64
results stand (pack.cpp as base64 50,323; the JPEG in HTML 170,423), and v32's output is unchanged.

## Checks

- Sieve, sieve-install and the oracle give every file above back byte for byte.
- Fuzzed: 30,000 made-up texts with all three kinds (66,140 spans) and 30,000 malformed skeletons,
  under the address and undefined-behaviour sanitisers: every span given back, every bad skeleton
  refused without a crash.
- The CI block (step 11) now also has a C array and a decimal list: 87,114 bytes at v32, 68,142
  with the draft.

## Next

The rope's search on files it doesn't win, measured now (a separate zip).
