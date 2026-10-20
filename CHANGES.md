# Changes: the chain check fixed (20 October 2026)

Copy `Sieve/` over the repository's `Sieve/`. Two files: the CI workflow and HANDOFF. No code
changed, so nothing needs rebuilding.

## What the check does

It's in the hallway CI step, the part headed CHAIN (SPECIFICATIONS §12.0h):

1. Opens the chain tab on the AI line and presses keys: **Down** selects the second stage, **N**
   inserts a copy of it after it, and each **Right** steps that copy to the next model in the tab's
   list (wrapping round at the end).
2. Saves the chain (`--chain-save`) and compares the `sieve-chain-v1` file with the exact lines
   expected.
3. Runs it with `/chain` in CHAT and checks that every stage ran, in order.

## Why it failed

The tab's list is the trained models in the training folder, with the model in hand first. When
the check was written (13 October) there were two: `figures-2x32` and `melodies-2x64`. Stepping
the copy of `figures-2x32` forward twice wrapped round to `hand`, which is how the check covered a
model-in-hand stage.

Later that day `type-2x32` was added (the character specs work). That added a stage to the default
chain and an entry to the list, so two steps now stopped on `type-2x32` instead. The tab was right;
the check was out of date. Nobody saw it because the CI has never run on GitHub.

## The fix

The check now presses Right three times, so the copy wraps round to `hand` again. It expects five
stages: the text run, `figures-2x32`, `hand`, `melodies-2x64`, `type-2x32`. The whole hallway step
passes here, start to end.
