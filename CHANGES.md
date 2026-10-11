# The AI Training Harness: models trained on the record

Against the AI line (the previous batch, which you have committed). As you described it: a
training harness that records exactly what a model learnt from and in what order, so that anyone
can train it again from that record and get the same model; an **AI Training Harness** on the
main menu; and the `.sieve` doing a map's work rather than a new map format.

## Training on the record (SPECIFICATIONS §12.0e, `training-v1`)
- **Pinned integer arithmetic:** every number is a 64-bit integer, and every rounding is pinned.
  - The functions it needs (2^x, sine and cosine, the square root) are fixed polynomials and
    integer steps. Their constants are written in the specification.
  - Floating point can't promise the same result from one machine to the next. This does, with
    any number of threads, because integer sums don't depend on their order.
- **Every checkpoint is a model of the AI line:**
  - the forward pass uses the line's own weight values;
  - the master weights, kept to 32 fractional bits, take the gradients.
  So a training run is a path of addresses on the line, and the trained model is on the line too.
- **A run is a folder** (no map needed):
  - `training.ini`: the model's shape, where it starts, how it learns;
  - `order.txt`: every window of bytes it learns from, in order, written out;
  - `corpus/`: the material itself;
  - `result.txt`, once trained: its address at every checkpoint, and the final model's SHA-256.

  Packed, it's an ordinary v4 `.sieve`. Training it again must give `result.txt` byte for byte;
  that's the check.
- **It learns:**
  - A Micro model reads Kurd Laßwitz's *Die Universalbibliothek* (1904, the Library of Babel's
    forerunner, 21 KB) thirty times over in about five seconds.
  - It goes from 11.9 bits a byte (guessing) to 2.9.
  - It writes German-looking words: "eine", "der", "meine Zeichen keiner eine Biblions".
  - A wider model (width 32, 8 bits) writes "der Bibliothek".

## In the hallway: the main menu's AI Training Harness
- **The runs:** it lists the runs in the `training` folder beside the programs, both folders and
  `.sieve` files.
- **Enter trains the selected run.** Training runs on a worker and shows:
  - its progress;
  - a curve of bits a byte;
  - what the model now says;
  - for a run trained before, **REPRODUCED EXACTLY**, or where it differs.
- **N** records a new run from a file or folder you type, then trains it.
- **P** packs a run folder as a `.sieve`.
- **G** goes to the trained model on the AI line, in the run's shape, and puts it in your hand to
  talk to.
- **Sieve ships one run,** `universalbibliothek.sieve` (79 KB). **Training it again on your
  Windows build is the first real test across machines:** if it says REPRODUCED EXACTLY, MSVC's
  arithmetic is the same as GCC's here, bit for bit.

## The tool
- **`sieve ai-train`:**
  - `--record CORPUS --out FOLDER` records a run and trains it;
  - `--sieve FILE` also packs it;
  - `--model-out` writes the trained model's files, for `sieve chat --raw`;
  - naming a run (a folder or a `.sieve`) trains it again and checks the result (exit 1 if it
    isn't reproduced).
- **The name:** `sieve train` already exists (it builds the guided ordering's models), so this is
  `ai-train`.

## The oracle (`reference/sieve_ref.py`)
- **`TrainerRef`:** the training, written from §12.0e alone, in Python's integers.
  - It matched the C++ the first time it ran, on every step.
  - On two recorded runs it matched byte for byte, every checkpoint address and the model's hash
    included.
- **`train-vectors`:** `tests/vectors_training_v1.tsv`, nine steps on three shapes, plus the
  functions at a few points.
- **`train-replay`:** trains a run folder again and writes its `result.txt`.

## On maps (your question)
- **I agree, and have written it up:** IDEAS §16.3, and a note in SPECIFICATIONS §12.3.
- **What a map adds that a manifest lacks:**
  - files named but not held (a size and a hash);
  - edges;
  - metadata;
  - a seal.
- **All four fit in manifest v5:** a fourth way, "named", plus edge, metadata and seal lines.
  `.map` then stops being written but can still be read.
- **Training runs already show it working:** they're folders packed as `.sieve` files.

## Checked here (Linux)
- **Unit tests:** 150,974 checks, 0 failures. The new ones cover:
  - the oracle's vectors;
  - one thread and three giving the same model;
  - the run's files read back.
- **CI's new steps, all run here:**
  - a run recorded, then trained again:
    - by the oracle, giving the same `result.txt`;
    - from its `.sieve` with one thread: reproduced;
    - with windows of two steps swapped: not reproduced.

    (Swapping two windows within one step changes nothing, rightly: a step's gradients are summed.
    That is now in the spec.)
  - the shipped run, reproduced;
  - in the harness:
    - the shipped run, reproduced;
    - a run recorded and trained;
    - its model gone to, in hand on the AI line.
  - the vectors, regenerated.
- **Screenshots:** the main menu, the harness after training, and the model in hand.
- **Builds:** the tool, the tests and the hallway, with no warnings.
- **Not checked:** I have not built on Windows. Please run the shipped run in the harness; that's
  the test.

## Docs
- **SPECIFICATIONS:** §12.0e, and a note in §12.3.
- **README:**
  - the harness;
  - a `sieve ai-train` section;
  - six main-menu choices.
- **IDEAS:**
  - §15, training on the line;
  - §16.3, maps folded into v5;
  - §16.8.
- **HANDOFF:**
  - two rows in the file map;
  - a new entry.

## New files
- `core/include/sieve/training.hpp`, `core/src/training.cpp`
- `tools/cli/train.hpp`, `tools/cli/train.cpp`, `tools/cli/train_cmd.cpp`
- `client/harness.hpp`, `client/harness.cpp`
- `data/training/universalbibliothek.sieve`
- `tests/training_corpus.txt`, `tests/vectors_training_v1.tsv`

Reconfigure CMake before building: there are new source files.
