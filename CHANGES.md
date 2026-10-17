# Notes from playing it, and the ideas after (17 October 2026)

Notes only; no code changed.

## IDEAS
- **§19.79, things that get in the way:**
  - ffmpeg warnings always in red, saying where Sieve looked; a Settings row to choose ffmpeg;
  - the Encryption Engine taking the item in hand;
  - controls flowing between windows;
  - the window still not coming back to the front after a file is opened. It was tried in
    `dialogs.hpp` and isn't enough on Windows; the likely cause and the fix to try are noted;
  - CHAT's tabs answering the mouse, so Esc isn't needed before C;
  - saving chat logs and editing the context window;
  - a help tab in CHAT and on every item;
  - bumping the version, still 0.13.1.
- **§19.80, one less if:** a tab with a use belongs on every line. CHAT on every line, about the
  item in hand, with the loaded model or a chosen chain.
- **§19.81:**
  - folding as a second temperature;
  - token and process viewers, mappers and loggers, as 3D node graphs of how models move;
  - models as filters, and the constraining map;
  - pictures boiled down to their titles.
- **§19.82, the Encryption Engine over every file:**
  - `.sieve` recipes with keys split into several. The standard name for that is secret sharing
    (Shamir, 1979).
  - Filtering encrypted files out, honestly: a strong cipher's output is built to look random, so
    a filter can set aside what looks random, but can't tell a ciphertext from noise. What it can
    catch exactly is the weak ciphers and the formats that announce themselves.
  - The baseline analysis of every anchor as a measured catalogue.
- **§19.83:** one model at the centre of every model, starting as one node map per model type,
  with chains drawn across them.
- **§19.84, a website gallery:**
  - the button and a small preview on Build or Buy;
  - every subfolder on the gallery page;
  - a `gallery.json` written by a tool, since a website can't list a folder.
  - Note: `Images/Sieve` has no images at its top level since the move, so the preview needs some
    put there.
- **§19.85:** a skeleton system beside the UV system, with joint limits as a filter on poses.

## VOICES.md
- "If a tab has utility, it should exist in all dimensions." (rule)
- "One less if statement." (rule; a point about everything)
- "It should become a living web of ranges and states and fluctuations…" (quote; a point about
  everything)

## HANDOFF
- An entry for this round.
