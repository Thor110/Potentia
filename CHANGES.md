# CHAIN: models one after another, and the notes that wrap it all up

Against your main ("prompt fullsize"). Copy `Sieve/` over yours. There is one new source file,
`Sieve/client/chain.cpp`, already in CMakeLists.txt.

## CHAIN, a tab of its own beside CHAT
- **Where it is:** the tab just before CHAT, so C reaches it on the way: ITEM, COST, SORT, CHAIN,
  CHAT.
- **What it shows:** the chain as a row of nodes with arrows: YOU (in CHAT), then each model in
  turn.
- **What it starts as:** the three trained models in one chain: universalbibliothek's text model,
  then figures-2x32, then melodies-2x64.
- **Running it:** type `/chain` and some words in CHAT. Every stage's result appears there as a
  reply or a card, with GO TO IT, PLAY and SAVE as usual (screenshot sent with the zip).
- **How each stage is fed:**
  - *A text model* (the model in hand, a trained text run, or the model file) continues what it is
    given.
  - *A picture or melody model* takes no input, so what it is given chooses its noise (a generator
    seeded by its SHA-256). The same words make the same picture.
  - *Each stage after* is given what the one before made: text as it is, a picture as its rows, a
    melody as its notation.
- **Adjustable:**
  - Up and Down choose a stage;
  - Left and Right change its model, through every model there is, of any kind and shape;
  - N adds a stage after it; Delete takes it away;
  - Ctrl+Up and Ctrl+Down move it;
  - Enter goes to CHAT with `/chain ` already typed.
- **Savable:** Ctrl+S and Ctrl+O save and open a `.chain` file. The format, `sieve-chain-v1`, is in
  SPECIFICATIONS §12.0h: one stage a line, such as `run universalbibliothek` or
  `draw figures-2x32`.
- **Mappable, for now as a row:** branching chains kept in the node graph's maps are noted next
  (IDEAS §19.46).
- **Deterministic:** a chain of picture and melody models makes the same units on every machine,
  and CI checks three addresses. With text models it is the same on the same machine; their
  arithmetic is floating point.
- **Found and fixed here:** my first version picked a picture's noise as "the k-th noise" for a
  large k. That meant drawing every noise before it, and a run took minutes. Now the noise is drawn
  straight from a seeded generator, and a whole chain runs in under a second.
- **Scripted runs:** `--chain`, `--chain-file PATH`, `--chain-press KEYS` and `--chain-save PATH`.

## The notes (IDEAS §19.45–19.53, GAME.md)
- **§19.45 Stories in the noise, and one equation.**
  - *Pareidolia is a filter with a false-positive rate,* so every coincidence can carry its exact
    odds: a contained lens on the madness of Babel.
  - *The one equation already exists:* a unit read as a number, one bijection per shape.
  - *Everything crosses over freely* through the binary line.
- **§19.46 CHAIN:** what was built, and what is next (branches in the node graph, steering, larger
  models).
- **§19.47 An engine that could make every game.** A game is a starting state and a rule (Life is
  enough to compute anything). The order to build it in:
  1. content (which exists);
  2. rules as units (a Life-like line);
  3. bodies with exact physics;
  4. voxels to triangles;
  5. an optional learned upscaler of our own, since DLSS is NVIDIA's and closed.

  The honest limit: almost nothing on the line is a game, so the work is in the filters that find
  the playable ones.
- **§19.48 The documentation inside the game:** `/docs`, with the documents on the lines and every
  catalogued thing linked by its SHA-256.
- **§19.49 The completion line on the main menu:**
  - in colour, with notation, one line per dimension, folding into "and N more" so it always
    fits;
  - one specification for the numbers, with CI keeping the website and the game in sync.
- **§19.50 Metrics for every build:** written by run_tests.bat after the tests pass, then charted
  over time on the website and on a wall in the world. A change of measure starts a new series.
- **§19.51 A decompilation engine:** compiling as a filter on the code line, with analysis first
  (as Ghidra does) and search for small pieces. Mind the law on others' software.
- **§19.52 The Encryption Engine: anything as anything.**
  - *The catalogue of combinations:* payload, key, recipe, depth and output shape, each a line,
    all countable.
  - *"As anything" has a standard name:* format-preserving encryption. Sieve's scrambled ordering
    is already a Feistel network, as FF1 and FF3-1 are, so with your own key every line is a cipher
    of its own shape (for play and study, not secrets).
  - *What flattening shows:* every output is reachable, so the key is what makes one path the
    message.
- **§19.53 The plan, in order of importance:**
  1. the Encryption Engine, stage 1;
  2. metrics for every build, charted;
  3. models steered by the filters;
  4. the documentation in the game;
  5. the completion line on the main menu, in sync with the website;
  6. chains as graphs;
  7. rules as units;
  8. research (decompilation, voice, code).
- **GAME.md:** stories in the noise, and the engine.

## Also
- **SPECIFICATIONS §12.0h:** `sieve-chain-v1`, exactly, seeds and all.
- **README:** CHAIN and `/chain`.
- **HANDOFF:** a new entry.

## Checked here (kept short, as you asked)
- **Unit tests:** 76,407 checks, 0 failures.
- **New CI lines,** run on a clean copy, passing:
  - the tab edited by keys and saved, byte for byte;
  - a four-stage chain run;
  - a picture-and-melody chain, address for address.
- **Run again:** the CHAT, model-file and prompt lines, since `talk_file` was split into
  `load_file_model` and `file_reply`.
