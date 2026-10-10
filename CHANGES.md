# `sieve tensors`: a model file taken apart

Against `248c402` (models dissected notes). The dissection of SmolLM2-360M-Instruct, made part of
Sieve so anyone can repeat it against the pinned hash.

## The tool: `sieve tensors`
- **`sieve tensors FILE.safetensors`:**
  - the file's size and SHA-256;
  - its start (the 8-byte length and the JSON table of tensors);
  - its weights (bytes, values, types);
  - whether the start is the one `safetensors-layout-v1` writes. If it is, the start needs no
    storing, since it is rebuilt from the tensors' names, types and shapes.
- **`--config config.json`:**
  - with a model file, it checks that a Llama config names exactly the file's tensors, so the
    start is rebuilt from the config alone;
  - without one, it builds the start a model of that shape has.
- **`--stats`:** the weights by kind of tensor: bits a value coded by frequency, of the high and
  low bytes, and the whole so coded.
- **`--tsv`:** the same statistics, one tensor a line.
- **`--start-out FILE`:** writes the rebuilt start, to compare with the file's first bytes.
- **Speed:** one pass through the file. 4.4 s for SmolLM2's 724 MB here.

## The core
- **`sieve/json.hpp`:** a small strict JSON reader and compact writer. It keeps members in order
  and escapes as the safetensors library does.
- **`sieve/safetensors.hpp`:**
  - the header reader, which refuses offsets that do not hold their shapes, overlap or leave gaps;
  - the layout (`layout_start`, `is_canonical`);
  - the Llama tensors from a config;
  - the statistics for BF16 and F16.
- **SPECIFICATIONS §12.0a:** the format, the layout rule, the Llama naming and the statistics,
  set out exactly.

## The oracle (`reference/sieve_ref.py`)
- **`tensors FILE [--tsv] [--config] [--start-out]`:** the start rebuilt and the statistics
  table, written apart from the C++.
- **`tensors-fixture --config CFG --out FILE`:** a small stand-in Llama model with seeded values,
  for tests. `tests/llama_tiny_config.json` is its shape.

## Checked here (Linux)
- **On SmolLM2-360M-Instruct:**
  - the hash matches;
  - the start rebuilt from the tensors, and from `config.json` alone, is byte for byte the file's
    first 32,672 bytes;
  - `--tsv` is identical to the oracle's on all 290 tensors;
  - coded by frequency, 475,928,934 bytes (65.8%), as in the notes.
- **Unit tests:** 149,802 checks, 0 failures. The new ones cover JSON, the layout (with the
  SHA-256 of SmolLM2's start, pinned, built from its config alone), bad headers and the
  statistics.
- **CI's new step, run here:** the stand-in model, its start rebuilt from its config, and the
  oracle's table.
- **Builds:** the tool, the tests and the hallway, with no warnings under the project's flags.
- **Not checked:** I have not built on Windows.

## Docs
- **README:** a `tensors` section.
- **IDEAS §15:** the measuring tools are now `sieve tensors`.
- **HANDOFF:** a new entry.

## Not in the repository
- **The model itself:** download it to try it. README names the repository and commit.
