# `sieve-weights-v1`: the weights coded under a prior

Against `477fd0b` (sieve tensors). The noise-gradient prior, first version: a model file coded
losslessly so that likely weights cost few bits, and rebuilt byte for byte.

## What it is (SPECIFICATIONS §12.0b)
- **One table for each kind of tensor:** how often each 16-bit value occurs. A kind is the
  tensors whose names differ only in their layer number, of one type.
- **The coder:** each tensor's values are coded under its kind's table with an exact integer
  coder (rANS), so the same file comes back on every machine.
- **The stream:** the start as it was, then the tables, then each tensor's coded stream (any
  tensor that is not BF16 or F16 is kept as it is), then the file's SHA-256. Unpacking refuses
  anything that does not rebuild that hash.

## Why tables by kind (measured on SmolLM2)
- **A table for each tensor** costs 4.6 MB to store and saves only 0.7 MB.
- **The exponent given its row's or column's typical scale** saves only 0.3 MB, so v1 leaves rows
  and columns out.

## SmolLM2-360M-Instruct
- **Packed:** 476,729,540 bytes for 723,674,912, 65.9%. xz -6 gives 512,876,944.
  - the start: 32,675 bytes;
  - the 11 tables: 117,878 bytes;
  - the coded tensors: 476,578,938 bytes.
- **Rebuilt:** byte for byte, SHA-256 `e6bffe74…f86e`.
- **Time:** pack (with its read-back check) 39 s, unpack 12 s.

## The tool
- **`sieve tensors FILE --pack OUT.sieve-weights`:** writes the packed file, reads it back and
  checks that it rebuilds the file before it reports.
- **`sieve tensors --unpack FILE.sieve-weights --out FILE.safetensors`:** rebuilds the model file.

## The oracle (`reference/sieve_ref.py`)
- **`weights-pack` and `weights-unpack`:** written apart from the C++.
- **Agreement:**
  - its packed file is the tool's, byte for byte (on the stand-in model);
  - its unpack rebuilds SmolLM2 from the tool's pack (3 min 42 s in Python).

## Checked here (Linux)
- **Unit tests:** 149,840 checks, 0 failures. The new ones cover:
  - the table rule;
  - coding and back (skewed values, one value alone, none, a value its table lacks);
  - a whole small model with an F32 tensor kept as it is, packed and rebuilt;
  - damaged, short and over-long streams refused.
- **CI's step, run here:** the stand-in model packed by the tool and the oracle, the two files
  compared, and each unpacked by the other.
- **Builds:** the tool, the tests and the hallway, with no warnings.
- **Not checked:** I have not built on Windows.

## Docs
- **SPECIFICATIONS:** §12.0b, the format set out exactly.
- **README:** `--pack` and `--unpack`.
- **IDEAS §16.8b:** v1 marked built, with the measurements.
- **HANDOFF:** a new entry.
