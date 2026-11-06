# A sieved GIF opens about twice as fast in the viewer (6 November 2026)

One commit on top of 56af7f3 ("orbital simulation update").

Where the time went, opening cube_demonstration's .simage (made by the viewer's Sieve button):
- **The vault check, frame by frame:** installing checks every picture against the vault, which
  decoded the GIF and took a PDQ hash of each of its 54 frames one after another (147 ms).
- **A second decode:** the viewer then decoded the same GIF again to show it.

Now:
- **Hashes on every core:** the vault hashes the frames in parallel, and skips a frame identical to
  the one before (same pixels give the same hash). Every frame is still checked: a GIF with the
  vault's test picture as its fourth frame is withheld.
- **Decoded once:** the vault hands the frames it decoded to the viewer, for those exact bytes only
  (size and SHA-256). It's off unless the viewer asks, holds one picture, and is cleared after each
  load.

| `sieve-view --check` (every frame made) | Before | After |
| :--- | ---: | ---: |
| the GIF | 0.17 s | 0.17 s |
| its .simage (Sieve button) | 0.63 s | 0.29 s |
| its .simage, v34 draft, Prioritise Decoding Speed | 0.62 s | 0.37 s |
| the six GIFs' .sieve (all six made) | 3.82 s | 3.36 s |

**Checks:** frame 30 shows the same pixels from the .simage as from the GIF, and the unit suite
passed with 0 failures.

**Still slow:** a GIF carried mixed (v34 draft without Prioritise Decoding Speed) takes about 9 s,
because mixing decodes about 2 MB a second. The viewer's Sieve button never mixes, so its .simage
files aren't affected.
