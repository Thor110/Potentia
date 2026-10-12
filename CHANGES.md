# Notes: encryption, rules as automata, code, a model map, and the release

Against your main ("sound model"). Copy `Sieve/` over yours. Documentation only: no code, so
nothing new to build or test.

## IDEAS
- **§19.34 The Encryption Engine** (main and pause menus).
  - *Recipes are a line:* a way to encrypt is a list of layers (compress, encode, cipher and
    mode, key from password, container), so every way, layered again and again, has an address.
  - *Your 50/50, made precise:*
    - a strong cipher's output can't be told from noise; that is what strong means;
    - everything around it can be told: headers, containers, sizes;
    - weak and historical ciphers (Caesar, Vigenère, short XOR keys, ECB) can be told and broken
      by trying every key.
  - *Enumerating the ways, as a filter:* shortest recipes first, as with seeds. It finishes where
    the key space is small; a 128-bit key is the counting argument's wall. With the key,
    decrypting is J.
  - *The menus:* each layer's output is an item on the binary line; identify and decrypt going
    the other way.
  - *Two cautions:*
    - Sieve's scrambled orders use a published key, so they hide nothing. For real data, use
      reviewed implementations of standard ciphers.
    - Check the export rules for encryption before selling.
- **§19.35 Every rule as an automaton?**
  - *Strictly, yes:* units have a fixed length, so any rule keeps a finite set, which some
    automaton recognises. What matters is whether it is small.
  - *Small:* bounded-memory rules, counting rules, and two-dimensional ones at small widths.
  - *Not small:* rules about the whole unit at once, and rules built on hashes or model scores.
  - *Undecidable at unbounded length:* what a program does.
  - *The aim:* an automaton where a small one exists; elsewhere, an automaton that keeps a
    superset, with a judge to check each unit.
- **§19.36 Only so many ways to write code.** This is superoptimisation (Massalin 1987, STOKE
  2013).
  - *Its limits:* program equality is undecidable in general, the shortest program can't be
    computed, and "right" is a frontier of trade-offs, not one best.
  - *In Sieve:* a code line, with filters for "well-formed", "passes the tests" and "cost".
- **§19.37 How powerful rules can become.**
  - *The counting argument caps it;* the lever is rules that fit the world, agreed once.
  - *One AI across every line* can be steered by the filters exactly: a symbol is allowed only if
    the line's automaton can still reach a survivor. That is constrained decoding, and it works
    today wherever a stack has an automaton.
- **§19.38 A map of trained models.**
  - *Models are already points:* addresses or SHA-256s.
  - *The edges:* lineage, nearness, paths between models (linear mode connectivity), and merges.
  - *Strung together,* they make a pipeline of Sieve's own models: what ComfyUI's graph does,
    without ComfyUI.
  - *A window of its own,* in the node graph's format.
- **§19.39 One game, two names.** Paid as *State Space: Near Zero*; free to build from GitHub as
  *The Sieve of Potentia*; identical.
  - *The licence:* your repository's AGPL v3 allows selling builds while the source is free. It
    also lets others sell builds, so the names are protected by trademark, not by the licence.
  - *The closest precedent is Mindustry:* GPL, free to build, sold on Steam, the same game.
    Stealth Bastard and Black Mesa are the spirit of it, though their paid versions added content.
  - *"No changes whatsoever" means:*
    - no Steamworks (or only an optional part a self-built copy lacks);
    - answering Steam's AI-content survey "yes, live-generated" (CHAT, DRAW);
    - checking the export rules if the Encryption Engine ships.
  - *Making "how easy it is" true:*
    - the three steps, written out;
    - the first build downloads SDL3 (internet, a few minutes);
    - the release script needs Python, so a `build_release.bat` that needs only Visual Studio
      would make "done" true;
    - the video is the three steps, uncut, with the time on screen.
  - *The front page:* "This game is free if you build it yourself!", the video, the steps, then
    the Steam version for those who want it ready to play. The store page says the same.

## GAME.md
A line at the top points to §19.39.

## HANDOFF
- **Who runs what:** you run the whole suite on Windows for every package (both SHA-256 paths,
  152,668 checks) and try it in the program. In a conversation I run the unit suite once and the
  CI lines a change adds or touches, and leave the whole hallway step to GitHub's CI and to you.
- **A new entry** for these notes.
- **What is next when you choose:** `canon-image-v2` (the Vitruvian finding), and
  `build_release.bat`.
