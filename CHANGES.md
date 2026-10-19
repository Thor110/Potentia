# Changes: the tightened rope, your answers and the first measurement (19 October 2026)

Copy `Sieve/` over the repository's `Sieve/`. No code changed; only `docs/IDEAS.md` §19.87.

- **Both rounds of your answers are recorded**, and the design as it now stands.
- **What landing on a degree costs.** A whole degree is 8.5 bits; the rest of the address is the
  walk from that degree to the file. So 0° is the ideal, as you said, and reaching it means the
  wave leaves almost nothing behind.
- **First measurement of tracking mode.** A wave drawn straight through the start, ⅓, centre, ⅔
  and end, taken away, and what's left coded by its histogram:
  - One wave across a whole file is too coarse. On every file tried (a picture's pixels, a text,
    a mesh, a MIDI, a sound, a model) it comes out a little worse than no wave at all.
  - Propagation is what wins. On the Vitruvian picture as raw grey pixels, two phases (the line
    before and the value before, averaged) give 843,855 bytes, against LZMA2's 865,400: 2.5%
    better.
  - The five kept points didn't help at any scale tried.
  - Text, meshes, MIDI and models aren't smooth, so no wave helps them. LZMA2 wins there.
- **Proposed next:** take the propagating form further on raw pictures and sound, and if it holds,
  make it a way to carry such files in the instructions.

The measuring script isn't in the repository. It can go into `tools/` if you want it kept.
