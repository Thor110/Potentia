# The Encryption Engine's screen: a quick start (13 October 2026)

## What it does
**Encryption Engine** is a new entry on the main menu and the pause menu, after Settings.
- **The rows** (Up/Down choose, type to edit):
  - **INPUT:** text, or bytes in hex.
  - **INPUT IS:** TEXT or HEX (Left/Right).
  - **RECIPE:** layers apart by `;`, as `sieve cipher` takes them: `reverse`, `caesar N`,
    `xor HEX`, `vigenere TEXT`, `feistel TEXT`.
  - **DO:** ENCRYPT, DECRYPT or BREAK (Left/Right).
- **The output:** in hex, and as text when it is printable. **Break** shows the five best
  guesses, ranked, as `sieve cipher --break` does.
- **Keys:**
  - **Enter** runs it again.
  - **Tab** puts the output in as the input and flips encrypt and decrypt, so what was just
    encrypted can be decrypted, or broken, at once.
  - Ctrl+V pastes, Ctrl+C copies the output's hex, Ctrl+Backspace clears a row.
  - Esc goes back.
- **What it opens with:** "attack at dawn" encrypted with "vigenere lemon; feistel my key", so it
  shows something straight away.

## Why after Settings
Every CI step that counts Downs to an existing menu row still lands on the same row.

## Files
- **New:** `Sieve/client/cipher_screen.hpp` and `.cpp`.
- **Wiring:** `main_menu.hpp`/`.cpp`, `pause_menu.cpp`, `hallway.hpp` (`Request::Cipher`),
  `app_main.cpp`, `CMakeLists.txt`, and `tools/cli/args.hpp` (the `--cipher` switch).
- **Strings:** `cipher.*`, `item.cipher` and `pause.cipher` in `data/lang/en.txt`.
- **Scripting:** `hallway --cipher --script "Down,=text,Return" --screenshot F`.
- **CI:**
  - the opening recipe and its round trip (Tab gives back "attack at dawn");
  - an XOR of a Dickens sentence broken, with `xor 736563726574` as the best guess;
  - the main menu's new row.
- **Docs:** README (the main menu, the pause menu and a paragraph on the screen), IDEAS §19.52
  (stage 2 begun, what comes next), and a HANDOFF entry.

## Checked here
- The client builds here (SDL3, offscreen driver).
- The four new CI lines pass.
- Screenshots of the screen, the main menu and the pause menu look right.
- The pause menu's help line was cut off, so it is shorter now.
- The unit suite wasn't run: no core code changed.

## Next for the engine
The item in hand or a file as the input, the output as an item to Go to or save, a layer keyed by a
picture (IDEAS §19.76), and the catalogue on the screen.
