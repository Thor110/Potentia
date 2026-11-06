# GIFs opened, measured (6 November 2026)

Commits c7a32e4 (the IDEAS note) and 278cd89 (the measurement), both on top of your 73c4868, local and not pushed.

## What was measured
The six demonstration GIFs from your InscribedSquareProblem repository, all written by gifenc 1.0.3. The repository clone was deleted afterwards; only the GIFs were kept, in a scratch folder.

**Made again exactly: yes.**
- A compress-style LZW encoder gives back all 346 frames byte for byte.
- gifenc's sub-blocks are always 254 bytes.

So a gifenc GIF can be opened and made again by rule.

**Size:**

| | Bytes | Of the GIFs |
|---|---:|---:|
| The six GIFs | 9,616,677 | 100% |
| Sieve today (Optimise Search) | 8,377,298 | 87% |
| **Opened (true indices + palettes, xz -9e)** | **4,718,704** | **49%** |

That opened figure uses xz alone, before Sieve's rope, mixing, or using the frame before as context.

## Files
- **`results/gif-opened-2026-11-06.md`:** the table and how it was measured.
- **`docs/IDEAS.md`:** §19.123 now carries the measurement.
