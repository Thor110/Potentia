# An idea transcribed: random maze generators as distortion filters

Against the folder-dialog and model-settings batch (which you have committed).

## IDEAS §18.1 (new section 18, your ideas of 11 October 2026)
- **Your idea, in your words:** the Binary and Byte settings, the far-end argument, the worry
  about data loss with other filters, and recording the seed so the maze can be undone.
- **Evaluated:**
  - Everything is still there, provided the maze is specified as a permutation (or a mask).
  - The far-end file does gain, but only by displacing other files, or by the cost of its seed.
  - Each kind (moving bits, moving bytes, flipping bits) has a limit, which follows from what it
    preserves.
  - With other filters nothing is lost, as long as the maze's version and seed are part of the
    stack's id.
  - It helps when agreed in advance under a guided ordering, or as a drawable keyed ordering.

No code changes.
