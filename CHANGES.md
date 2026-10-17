# Changes: the rest of IDEAS §19.79 (17 October 2026)

Copy `files/` over the repository root, on top of the 0.13.25 zip and the docs zip.

- **Your calls, recorded:** controls between windows is mostly the ethos now (IDEAS §19.79), and
  CHAT on every line is deprecated for now (IDEAS §17.2, struck through in §19.80; the rest of
  §19.80 stands).
- **The Encryption Engine takes items and files** (`client/cipher_screen.*`, `app_main.cpp`,
  `hallway.*`):
  - **I** on the item page opens the engine with the item in hand as its input (its bytes, as F
    would save them). It is a key on the item page because Esc puts the item down before the
    pause menu opens.
  - **Ctrl+O** takes a file; Ctrl+I takes the item again. INPUT names it and shows its first
    bytes; typing there goes back to typed input.
  - **Ctrl+S** saves the output. **GO TO IT** (Ctrl+G) puts you on the binary line in front of
    it; from the main menu, a hallway is built as the settings stand.
  - Four buttons do the same with the mouse. BREAK reads at most the first 64 KB.
  - `--cipher-in FILE` and `--cipher-out FILE` for scripted runs; a CI line checks that the
    engine's output on a file matches `sieve cipher`'s byte for byte.
- **The context window in CHAT** (`client/chat.cpp`): `/context` shows what the model is given,
  and **Ctrl+E** edits it.
  - The model in hand: every byte it was fed since it started (non-text bytes as `\xNN`). Enter
    starts it again from exactly what the box says.
  - A model file: its system prompt. Enter starts the conversation again with it.
  - Esc leaves it as it was. Ctrl+C copies the box, Ctrl+V pastes, Ctrl+Backspace clears.
- **HELP** lists I. **README**, **IDEAS** (§19.79, §19.52, §19.82) and **HANDOFF** updated.
- **Checked here:** every changed file compiles with no warnings against SDL's headers, and the
  `\xNN` round trip is exact over every byte value. Not run here: the build, unit suite or CI.
- **Worth trying:**
  - I with a picture, a page and a file in hand; Ctrl+O; Ctrl+S; GO TO IT from the hallway and
    from the main menu.
  - In CHAT: say something, Ctrl+E, change it, Enter, then `/context` to see it took.
  - With a model file, Ctrl+E shows the system prompt.
