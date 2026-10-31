# Proof: the two samples' figures at v32 (commit 7fb58df)

From your logs of the samples made again:

| Sample | v20 | v32 | |
| :--- | ---: | ---: | :--- |
| `degrees.sieve` (3,960 files, 13,321,756 bytes) | 7,887 | **7,865** | 1,694:1, 65.75% under 7z's 22,961 |
| `welcome to the sieve.sieve` (32 bytes) | 159 | **158** | |

- `degrees.sieve` took 149.3 s to make with Try every way, and installed in 1.787 s.
- The File Locator's own 7z of the folder (LZMA2 9e, one stream) is 9,226.

**Files:**
- `Proof/PROOF.md`: both figures.
- `Proof/STATISTICS.md`: the at-a-glance row and the section on Sieve's own lines.
