# The `.sieve` format on one page (28 October 2026)

Unzip over the repository root. Committed locally (not pushed). Documentation only.

- `Sieve/docs/SIEVE-FORMAT.md` (new): what a `.sieve` file is, layer by layer, as Sieve writes it at
  v31, for anyone writing a decoder of their own (the plan's step 4; SHOWCASE-PLAN §1). The outer
  layer (the manifest's address as raw bytes), the head in bytes, the body's records, the four
  streams and every way a file can be carried, the tree's check, and what a decoder needs for each
  way. Each part points to its rules in SPECIFICATIONS §12.2 and its function in the oracle.
  Tested by a decoder written from the page alone (the coders, mix-v4 and the rope, borrowed from
  the oracle): it read back three files Sieve made here, byte for byte with the tree's SHA-256
  matching: a folder at v30 (a program in the x86 stream, five files mixed together in a group)
  and two single files at v20. Writing it found one thing the page needed: where to start looking
  for the manifest's length (the file's length less one).
- `Sieve/README.md`: the page in the documentation table.
- `Sieve/docs/SHOWCASE-PLAN.md`, `Sieve/docs/HANDOFF.md`: step 4 marked written.

## Files
- `Sieve/README.md`
- `Sieve/docs/HANDOFF.md`
- `Sieve/docs/SHOWCASE-PLAN.md`
- `Sieve/docs/SIEVE-FORMAT.md` (new)
