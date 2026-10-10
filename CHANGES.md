# The AI line: every language model of one shape

Against `2f2c21d` (sieve chat). As you chose: an item is a **model**, every model of one shape
numbered by its weights, and the default shape is **Micro**.

## What it is (SPECIFICATIONS §12.0d, `aispace-v1`)
- **The shape:** a Llama model with these settings:
  - layers: 1;
  - width: 16;
  - heads: 2;
  - bits a weight: 4.
  Its feed-forward is four times the width, its vocabulary is the 256 bytes, its output is tied to
  its embedding, and its norms are all 1.
- **Micro:** 8,192 weights of 4 bits, so 2^32768 models, each with an address of 8,192 hex digits.
- **The address is the weights:** their digits are written out, the first most significant, the
  tensors in name order. Scrambled order is shuffle-sha256-v1 over the whole line. Walking the line
  downloads nothing.
- **Each model runs on Sieve's own engine** (the one `sieve chat` uses), so it can be talked to.
  Almost every one babbles, and positional neighbours say nearly the same thing.

## In the hallway
- **The door:** AI stands after WORLDS and before BINARY, in deep violet with lilac edges.
- **On the shelf:** a crate showing what the model says, started from a newline. Bytes that are not
  printable are drawn as `·`.
- **In hand:** the shape, what it says, and the conversation. Enter opens `SAY > `:
  - it continues your line until it writes a newline of its own, at most 80 bytes;
  - when its 512 positions are full, it starts again.
- **F, J and the viewer:**
  - F saves `sieve-ai-<the address's first 12 hex digits>.safetensors`;
  - J finds that file on the binary line;
  - the viewer has a WEIGHTS tab: the tensors, then the digits.
- **T:** refuses with a message pointing to `sieve ai --warp`.
- **The setup menu:**
  - AI rows for layers, width (in steps of twice the heads), heads (only those that keep the head
    size even) and bits;
  - FIND MY LIMITS grows the layers.
- **The map:** a bar more than 8 times the median no longer sets the scale; it is drawn broken.
  Without this, the AI line's 32,768 bits flattened every other bar.
- **`--talk TEXT`** (after `--take`) prints a conversation, for scripts.
- **The degrees sample** now has eleven dimensions: 3,960 items and 13,321,756 bytes (mostly the
  models, 131,072 bytes of weights each). Its `.sieve` is 482,495 bytes.
- **No filters yet.** What a model says is floating point, so it cannot decide which models
  survive until the arithmetic is pinned (IDEAS §15).

## The tool
- **`sieve ai`:**
  - with no address, the line's size;
  - `--read ADDR`, `--bearing DEG` or `--browse N` run each model and print what it says after
    `--prompt`;
  - `--out FOLDER` writes `config.json`, `model.safetensors` (F32) and `tokenizer.json`;
  - `--warp PATH` takes a model's file back to its address, if it has the line's shape and every
    weight is exactly one of the line's values.
- **`sieve chat --raw`:** continues a text with no chat format, for the line's models. The reply's
  statistics now read "read N tokens in X s; wrote M in Y s, Z a second".
- **Core:**
  - `sieve/aispace.hpp`;
  - in `sieve/llm.hpp`: a `Model` built from tensors in memory, `continue_text` and
    `Chat::last_reply_tokens()`.

## The oracle (`reference/sieve_ref.py`)
- **`ai-vectors`:** `tests/vectors_ai_v1.tsv`, 30 rows. They cover three shapes, both orders, and
  the bearings 0, 13, 137, 246 and 359: yours, Gemini's and mine.
- **`ai-read`:** a model's files, which are the tool's byte for byte.
- **`llm-logits`:** now reads F32 too. Its logits agree with the tool's to 1.5e-6.

## Checked here (Linux)
- **Unit tests:** 150,848 checks, 0 failures. The new ones cover:
  - the vectors;
  - round trips;
  - addresses off the line and bad shapes, refused;
  - the values;
  - a model talking.
- **CI's new steps, all run here:**
  - `sieve ai` at 246 degrees in both orders, against the oracle's files;
  - warp;
  - the logits against the oracle;
  - `chat --raw`;
  - the vectors regenerated and compared;
  - in the hallway:
    - the model at 137 degrees, saved with F, is the tool's file;
    - a conversation;
    - the setup menu;
    - the doors WORLDS → AI → BINARY;
    - the sample;
    - the corridor walk of nine doors and back.
- **Screenshots:**
  - the shelves;
  - a model in hand;
  - the WEIGHTS viewer;
  - the AI rows in the setup menu;
  - the broken bar.
- **Builds:** the tool, the tests and the hallway, with no warnings.
- **Not checked:** I have not built on Windows.

## Docs
- **SPECIFICATIONS:**
  - §12.0d;
  - §3 now counts eleven lines.
- **README:**
  - the AI line's paragraph;
  - a `sieve ai` section;
  - `--raw` in `chat`;
  - the eleven lines;
  - the sample's numbers.
- **IDEAS:**
  - §15 "Another reading", marked built, with what comes next;
  - §16.8.
- **HANDOFF:**
  - the line overview (eleven lines);
  - a new entry.

## Next on this line
- Filters on the weights, and a guided order from a prior over weights (§16.8b).
- Warping a model in from the hallway.
- Larger shapes, as the engine gets faster.
