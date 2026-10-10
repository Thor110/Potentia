# `sieve chat`: the language model run by Sieve itself

Against `92c7a23` (sieve weights). As you chose: no Ollama. Sieve reads the model's own files and
runs it, the engine the AI dimension's room and chat viewer will use.

## What it does
- **`sieve chat --model FOLDER`:** a conversation, a message a line, until an empty line.
  - `--prompt TEXT`: one answer.
  - FOLDER is the model as published: `config.json`, `model.safetensors`, `tokenizer.json`.
- **Sampling:** temperature, top-p, top-k, repeat penalty and a seed. The defaults are 0.2 and 0.9,
  as SmolLM2's makers suggest.
- **The chat format:** ChatML, the format the model was trained on, with its default system line.
- **Looking inside:**
  - `--encode TEXT`: the tokens of a text;
  - `--logits TEXT`: the five likeliest next tokens, and `--logits-out FILE` writes them all.
- **Speed on SmolLM2-360M:** about 9 tokens a second on the 4 cores here; it loads in 1 to 3 s.

## How it is built (SPECIFICATIONS §12.0c)
- **The tokenizer** (`sieve/llm_tokenizer.hpp`): byte-level BPE read from `tokenizer.json`.
  - the added tokens;
  - every digit on its own;
  - the GPT-2 splitting pattern, with Unicode letters and numbers from a table generated for
    Unicode 14.0 (`tools/gen_unicode_classes.py`);
  - merges by rank.
- **The model** (`sieve/llm.hpp`): the Llama forward pass.
  - BF16 weights kept as stored (690 MB in memory), the arithmetic in 32-bit floats;
  - a key and value cache;
  - threads that share out rows, each summed in one fixed order, so the answer is the same
    whatever the number of threads.

## Checked against the real thing
- **The tokenizer:** our tokens equal Hugging Face's own tokenizer's on 5,736 texts: prose, code,
  numbers, every kind of space, contractions, many scripts, emoji, control characters and random
  strings.
- **The forward pass:**
  - on a 42-token chat prompt, every one of the 49,152 logits is within 0.000054 of the model's
    official ONNX export, run by ONNX Runtime;
  - the top 50 tokens come in the same order;
  - the KL divergence is 2e-11.
  - (PyTorch could not be installed here, as its download host is blocked; the ONNX export is from
    the same commit, in float32.)
- **In use:** " Paris" follows "The capital of France is", and it answers follow-up questions in a
  conversation.

## Not yet
- **Floating point:** this is the viewer's engine. Addresses under a model need the pinned
  arithmetic in IDEAS §15.
- **Next:** the AI dimension in the hallway, with its room, its items and the chat viewer.

## The oracle (`reference/sieve_ref.py`)
- **`llm-encode-lines`:** a tokenizer written apart. It also equals Hugging Face's on the 5,736
  texts.
- **`llm-logits`:** the forward pass in 64-bit floats, with `--compare`.
- **`llm-tokenizer-fixture`:** a small 300-token tokenizer learnt from a fixed text. It is saved
  as `tests/llama_tiny_tokenizer.json`.
- **`llm-corpus`:** texts to tokenize.

## Checked here (Linux)
- **Unit tests:** 150,402 checks, 0 failures. The new ones cover:
  - the Unicode classes and the byte characters;
  - the stand-in tokenizer;
  - a small model giving the same logits with one thread and four, and after a reset;
  - seeded and greedy sampling;
  - a streamed chat.
- **CI's new step, run here:**
  - the stand-in tokenizer regenerated and compared;
  - the oracle's tokens on its corpus;
  - the logits within 1e-3 of the oracle's (they differ by 1e-6);
  - a reply.
- **Builds:** the tool, the tests and the hallway, with no warnings.
- **Not checked:** I have not built on Windows.

## Docs
- **SPECIFICATIONS:** §12.0c.
- **README:** a `chat` section.
- **IDEAS:** §15, the engine and what it was checked against; §16.8, your choice.
- **HANDOFF:** a new entry.
