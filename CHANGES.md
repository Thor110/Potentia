# SmolLM2-360M-Instruct taken apart: IDEAS and HANDOFF

Against `d5d3013` (meshes update). Docs only.

## IDEAS §15: "The first model, taken apart"
- **The pin:** the repository and commit, the licence, and the file's size and SHA-256.
- **The shape:** from `config.json`.
- **The file in three parts:**
  - **The start:** 32,672 bytes, rebuilt byte for byte from `config.json`, so it needs no address of
    its own.
  - **The weights:** by kind of tensor.
  - **The tokenizer:** a file of its own.
- **What the weights are made of:**
  - the high byte carries 2.72 bits and the low byte 7.85 of 8;
  - neighbours tell almost nothing;
  - 6,486 distinct values.
- **Lossless sizes:** coded by frequency 476 MB (66%), xz 512.9 MB (71%); quantised sizes for
  comparison.
- **Where the AI dimension shortens things:** in the text named under the model.
- **Your view:** the gains grow as the system is built out, and where the measurements say they
  can (the exponent bits, and the text).

## IDEAS §16.8(b)
- **A pointer** to the measurements.

## HANDOFF
- **A new entry:**
  - the model, pinned, and the allowed domains it came through;
  - the findings;
  - next: the dissection in `sieve`, the weight prior, then the AI dimension.

## Not in the repository
- **The model files and the measuring programs:** they are in my scratch space. The figures come
  from them, run here.
