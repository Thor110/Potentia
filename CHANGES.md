# Models given as files, and two notes

Against your main ("chat cards"). Copy `Sieve/` over yours. There are two new files,
`Sieve/tools/cli/model_folder.hpp` and `.cpp`, which are already in CMakeLists.txt.

## Text and picture models as files, in the setup menu
The AI section of the setup menu has two more rows, under the line's shape.
- **CHAT's model:**
  - It is a model folder: config.json, model.safetensors and tokenizer.json, as `sieve chat
    --model` reads one. That covers a published model such as SmolLM2, or a model of the line
    saved with `sieve ai --out`.
  - Enter (or Right) opens the system's folder dialog; Left goes back to the model in hand.
  - A folder without the three files says "not a model folder".
- **DRAW's picture model:**
  - Left and Right go through the trained picture models in the training folders.
  - Enter chooses any other run folder.
  - With none chosen, the first trained one is used, as before.
- **Saved:** both are kept in `sieve-hallway.ini`, in a new `[models]` section, as full paths.

## In CHAT
- **Talking:** with a model file set, CHAT talks with it, run by Sieve's own engine.
  - A ChatML model (SmolLM2 and most instruction models) is talked to as a chat. Any other model
    continues your line until it writes a newline, as the line's models do.
  - Replies are at most 200 tokens, and drawn as text.
  - It loads the first time you speak to it (LOADING) and answers on THINKING.
  - When the conversation fills its positions, it starts again and says so.
- **Giving it files and figures:** they are read with your next message: a text's first 1,500
  bytes, and of anything else, what it is (name, kind, size). The cards say so.
- **Ctrl+M** turns between the model file and the model in hand. The title line says which, and
  the model file's replies are labelled with its folder's name.
- **If the folder cannot be loaded,** CHAT says why, and the model in hand answers instead.
- **Scripted runs:** `--text-model DIR` and `--picture-model DIR`.

## IDEAS
- **§19.27 Files from known generators and their seeds.**
  - *As a name:* generator, seed and length name a generated file exactly, in a few bytes.
  - *As a filter:* it removes almost nothing. There are 2^64 outputs among 2^(8L) files, so it
    names a tiny set, where the noise gradient reorders everything a little.
  - *Recognising them:* LCGs, xorshift, the Mersenne Twister and splitmix64 can be run backwards
    from their output, so "made by generator G" is a quick check rather than a search.
    Cryptographic generators can't, by design.
  - *The general rule:* any description both ends can run is shorthand for exactly what it makes.
- **§19.28 A universe in a seed.**
  - *Minecraft seeds* are recovered from screenshots because each visible feature is a filter
    over a 2^64 seed line.
  - *A probe's readings* come from nature, so a seed can stand only for what a shared model
    predicts. The message is the model's settings plus the corrections, which are exactly what was
    news.
  - *How it fits:* it joins up with the agreed flags (§19.15) and the predicted tags (§19.16).

## README and HANDOFF
- **README:** the AI line's paragraph describes the model files.
- **HANDOFF:** a new entry.

## Checked here
- **Unit tests:** 76,334 checks, 0 failures (once, as this machine has no hardware SHA-256).
- **New CI lines:**
  - the tiny model (from the oracle's earlier step) answers through `--text-model`;
  - DRAW uses `--picture-model`;
  - the setup menu's picture row is saved in `[models]`.
- **Also tried here:**
  - a model of the line saved with `sieve ai --out` (the non-chat path);
  - a missing folder (the fallback);
  - Left clearing CHAT's model, saved as empty and read back as empty.
- **One existing CI line changed:** the loop that opens each line's filters from its setup row
  now finds BINARY at row 47 instead of 45, since the AI section has two more rows. I ran the
  whole loop here.
- **Run again on this build:** every other chat and harness line.
- **Not tried:** a real published model, such as SmolLM2-135M. There is none on this machine.
  It is the same engine `sieve chat` already runs it with.
