# Fixes for the first GitHub CI runs (21 October 2026)

All three runs failed for a different reason each. None was a fault in Sieve's packing; each was
something that differs on that system.

- **Linux** (`.github/workflows/build.yml`): GitHub's Linux machine lacked the X11 and Wayland
  files SDL needs to build the hallway, so configuring stopped. The workflow now installs them
  first.
- **macOS** (`Sieve/core/src/biguint.cpp`): one line compared two whole-number types that are the
  same on Windows and Linux but different on macOS, so it did not compile. A check of all 124
  core, client and tool files found no other such line.
- **Windows** (`Sieve/.gitattributes`): Git on GitHub's Windows machine changes line endings when
  it checks files out. `tests/training_corpus.txt` was not protected from that, so the training
  checks read a different corpus from the one their figures came from (18 failures). Reproduced
  here: the same 18 checks fail at the same lines when the corpus has Windows line endings. All
  of `tests/` is now protected.
- Also run here: the whole unit suite under GCC's undefined-behaviour checker; nothing found.
- **Docs:** HANDOFF; IDEAS §19.91 answers the idea of a file's opposite found in noise; VOICES has
  the quote.

To apply: copy `.github/` and `Sieve/` over the repository's top folder (beside `Sieve/`). If
`Sieve/.github/workflows/build.yml` is still there from before, delete it by hand. After your next
push, GitHub runs the checks again on its own; open the Actions tab to see them.
