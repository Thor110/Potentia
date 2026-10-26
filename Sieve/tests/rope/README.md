# Pictures for the rope's checks

| File | What |
| :--- | :--- |
| `vitruvian-centre-256.gray` | 256 by 256 grey pixels, one byte each, row by row (65,536 bytes): the centre of `data/anchors/public-domain/Vitruvian.jpg` (Leonardo's Vitruvian Man, public domain), read as grey by Pillow 12.3 and cropped at (346, 522) |

It is kept as raw bytes, not made from the JPEG in CI, so that no JPEG decoder's version can change
it. CI step 11 checks that the rope's search finds the triangle on it (rope-wave-v6, P4), and that
Sieve and the oracle give it back from v23 instructions.
