# IDEAS §19.121: seeds from the binary line (5 November 2026)

This zip's commit sits on top of 56a4426 (the temperature zip), and so includes b22a620 and 56a4426 too. All three are local and not pushed.

**One paragraph added under the temperature bullet: why binary is the right state space for the seeds.**

- **The seeds look random.** A seed as long as it needs is a coder's output, and a good coder's output is as close to random as it can make. If it weren't, it could be coded smaller.
- **No ordering helps on average.** For bytes with no pattern, no line puts them earlier on average than any other line does. Making some seeds shorter makes as many others longer.
- **The binary line adds nothing to undo.** Its address for a string is the string itself.
- **The model is a jump.** It goes from a point on the binary line (the seed) to a point on the line of what it makes (§19.2's J). The short seeds, early on the line, land on what the model expects most.
