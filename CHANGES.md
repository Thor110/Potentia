# Base64 in a text carried decoded (2 November 2026)

Unzip this over the repository root, on top of your commit 0cfa985 ("sieve hashes"). One local
commit, not pushed: b99d5f5. There's a new source file, `Sieve/tools/cli/text_spans.cpp`, so
CMake needs to configure again (the build does that itself, as CMakeLists.txt changed). The unit
suite passes (77,133 checks, 0 failures). The new CI block in step 11 passes here; the full steps 10
and 11 haven't been run on it.

## What it does (v33 draft only)

A text holding base64 is carried the way v31 carries an archive made again from its skeleton:
- **The skeleton:** the text with each span of base64 taken out, plus a small table saying where
  each span was and how it was written (alphabet, padding, line width, CR LF or LF).
- **The members:** each span decoded, listed after the text as `<text>/<n>` and carried its own way.
  A JPEG among them reaches brunsli; a text reaches mixing's word list.
- **The recipe:** `b64 <members> <skeleton size>`.

Each span is taken only where encoding it again gives exactly the characters written. The whole
file is weighed (LZMA2's estimate, or mixing's where that is less) against the skeleton and members
together, and must come out clearly smaller.

| File | v32 (as written) | The draft | Decoded file packed alone |
| :--- | ---: | ---: | ---: |
| pack.cpp as base64, lines of 76 | 76,813 | 50,323 | 50,358 |
| The Vitruvian JPEG in an HTML page | 203,542 | 170,423 | 170,340 |

Without the draft nothing changes (the same bytes as before, checked).

## Checks

- Sieve, sieve-install and the oracle give both files back byte for byte. The oracle makes a text
  with base64 itself, and leaves one holding a JPEG unmade, as it does every JPEG (it has no
  brunsli).
- Fuzzed: 20,000 made-up texts mixing prose, data: URLs, PEM-like blocks, CR LF, the URL-safe
  alphabet, no padding and odd widths. 32,339 spans found, every one given back exactly. 20,000
  malformed skeletons, every one refused without a crash (run under the address and
  undefined-behaviour sanitisers).
- The repository's own texts: three spans found (two alphabet strings in sources and a key in
  web/extension/manifest.json); the weighing took none of them.
- Speed: about 0.1 s to look through 12.9 MB of text, 1.4 s through 60 MB of solid base64.

## New CI block (step 11)

A page with a JPEG as a data: URL, a mail with a text as base64 (lines of 76, CR LF) and 300 random
bytes in the URL-safe alphabet unpadded, and a plain text. With the draft, the two texts with base64
are carried so and the plain one isn't; at v32, none, and the file is larger (77,509 bytes against
63,937). Given back by Sieve, sieve-install and the oracle.

## Not yet

Hex and decimal, the other two kinds of binary written as text, aren't done.
