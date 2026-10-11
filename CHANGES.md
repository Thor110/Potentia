# Curricula, and training runs that carry only what they need

Against the AI Training Harness (the previous batch, which you have committed, and which
reproduced on Windows).

## As you decided
- **Maps stay maps, and the manifest gains nothing for training.** A file carries only what it
  needs.
  - A training run is a folder of ordinary files, made by the harness (or `sieve ai-train`) and
    packed as ordinary Sieve instructions.
  - Other `.sieve` and `.map` files carry nothing of training's.
  - IDEAS §16.3 now says this; the plan to fold maps into v5 is set aside.
- **Your item-number idea** is noted there for a far later manifest. It would record a run's file
  order as numbers in the instructions' own file list rather than as paths.

## The curriculum, in the harness
New runs now go through a list before they are recorded:
- **N** then a path lists the corpus's files.
- **Up/Down** choose a file; **Shift+Up/Down** (or the buttons) move it.
- **M** chooses how each pass reads the files:
  - in order: file by file, each from its start;
  - file by file, each file's windows shuffled;
  - all the windows shuffled together.
- **Left/Right** set how many passes; **Shift** steps them by 10.
- **Enter** records the run and trains it.

A run's page now shows how it reads and its files in the order read. The learning curve has about
200 points across the run instead of one per checkpoint, so short runs show one too.

## Runs no longer store their order (SPECIFICATIONS §12.0e)
- **`order-v1`** makes the order from a few lines of `training.ini`: the files in reading order,
  how each pass reads, the passes, the window and the seed. The rule is pinned and versioned, and
  the oracle follows it apart.
- **`order.txt`** is written only on request (`--write-order`), for an order the rule can't
  express.
- **`result.txt` v2** lists each checkpoint by a hash of the model. Full addresses are kept only on
  request (`--checkpoint-addresses`), as a path to walk without training again. The final model's
  address and SHA-256 are still there, so G works at once.
- **The first runs' format still trains and is checked.** `tests/training_run_v1` is a fixture of
  it, checked in CI by the tool and the oracle.
- **The shipped run, recorded again,** trains to the very same model (`b3e5b752...`), now in
  14.5 KB instead of 79 KB:
  - the order went from 391 KB of text (20 KB packed) to ten lines;
  - the checkpoints went from 50 KB of addresses to hashes.
  
  Most of what's left is the text itself.
- **`sieve ai-train` options:**
  - `--reading MODE`;
  - `--file-order FILE` (the files to read first, in order);
  - `--checkpoint-addresses`;
  - `--write-order`.

## Checked here (Linux)
- **Unit tests:** 150,992 checks, 0 failures. The new ones cover:
  - the three readings;
  - the recipe's new lines;
  - both result versions, read back.
- **CI's training steps, all run here:**
  - each reading, with the files in a chosen order: the oracle trains it again to the same result;
  - a written-out order with checkpoint addresses:
    - reproduced from its `.sieve`;
    - not reproduced with windows of two steps swapped;
  - the old-format fixture, by the tool and the oracle;
  - the shipped run;
  - in the harness:
    - a run recorded from a folder, its second file moved first and read in order, trained;
    - its model gone to;
  - the earlier hallway steps.
- **Screenshots:**
  - the curriculum list;
  - the run it made;
  - the shipped run reproduced, with its full curve.
- **Builds:** the tool, the tests and the hallway, with no warnings.
- **Not checked:** I have not built on Windows. Training the new shipped run again in the harness
  is the test, as before.

## Docs
- **SPECIFICATIONS:**
  - §12.0e: run format 2, `order-v1` written out in full, result v2, the first runs' format;
  - §12.3: the maps note removed.
- **README:**
  - the harness's curriculum;
  - the new options;
  - the shipped run's size.
- **IDEAS:** §16.3, corrected, with your item-number idea.
- **HANDOFF:** a new entry.

## New files
- `tests/training_run_v1/` (a fixture run in the first format: `training.ini`, `order.txt`,
  `corpus/training_corpus.txt`, `result.txt`)
- `.gitignore` now makes an exception for that fixture's `corpus/` folder. Without it, the rule
  ignoring downloaded corpora would have left the fixture's text out of your commit, and CI would
  fail.
