# cmix as a yardstick (7 November 2026)

Edward: "Yes, run the cmix yardstick on a few files" (IDEAS 19.126; control.py at his asking). cmix v21
(github.com/byronknoll/cmix, 194af9c, 26 September 2026), built with clang 18 at its own flags
(`make all`: `-Ofast -march=native`), against Sieve with Optimise Search, on this machine (4 cores,
15 GiB). Every file given back the same by both.

**cmix had to be made smaller to run here.** It asks for 32 GB. As published it passed 14 GB and was
killed on the 64 KB picture. Two settings were lowered: its PPMd's memory from 14,000 MB to 1,000 MB
(the same output while PPMd's memory is not filled, which 100 KB cannot fill), and its paq8 model's
level from 11 to 9 (each level doubles its tables). The second can change cmix's output a little, so
the cmix column is close to cmix as published on files this small, not certainly the same.

## Sizes

Sieve's code alone is what its mixing carried the file in; the `.sieve` adds the file's name, size
and SHA-256 and the head (about 70 to 85 bytes). cmix's output holds the file's size. Texts were given
cmix's English dictionary (`-c english.dic`); the rest `-c`.

| File | Bytes | xz -9e | cmix | Sieve, code alone | Sieve, the `.sieve` | Sieve against cmix |
| :--- | ---: | ---: | ---: | ---: | ---: | ---: |
| Dickens (Silesia), first 100 KB | 100,000 | 35,680 | 22,510 | 25,976 | 26,055 | +15.4% |
| enwik8, first 100 KB | 100,000 | 33,016 | 19,743 | 24,772 | 24,859 | +25.5% |
| `core/src/written.cpp` (C++) | 60,678 | 14,080 | 9,089 | 11,345 | 11,427 | +24.8% |
| Vitruvian centre, 256 by 256 grey (raw) | 65,536 | 47,584 | 43,054 | 41,904 | 41,988 | **−2.7%** |
| `/usr/bin/ls` (x86-64) | 142,312 | 54,520 | 35,567 | 48,480 | 48,556 | +36.3% |
| `control.py` (data/anchors) | 1,597 | 748 | 461 (542 without its dictionary) | 520 | 591 | +12.8% (−4.1% against 542) |

## Time and memory

| File | cmix makes | cmix gives back | cmix's memory | Sieve makes | Sieve installs | Sieve's memory |
| :--- | ---: | ---: | ---: | ---: | ---: | ---: |
| Dickens, 100 KB | 259.0 s | 259.7 s | 12.1 GB | 2.6 s | 0.4 s | 371 MB |
| enwik8, 100 KB | 261.4 s | 256.5 s | 12.3 GB | 1.0 s | 0.4 s | 371 MB |
| `written.cpp` | 84.0 s | 81.7 s | 9.8 GB | 0.6 s | 0.3 s | 379 MB |
| Vitruvian | 95.7 s | 97.4 s | 11.1 GB | 2.0 s | 0.1 s | 262 MB |
| `ls` | 194.4 s | 190.7 s | 11.7 GB | 0.4 s | 0.2 s | 290 MB |
| `control.py`, dictionary | 176.4 s | 178.3 s | 11.0 GB | 0.3 s | 0.2 s | 132 MB |
| `control.py`, none | 7.0 s | 5.4 s | 6.3 GB | | | |

cmix's memory is set by its settings, not by the file: 6.3 GB for 1,597 bytes, 11 GB with its
dictionary loaded. Most of its time on small texts is the dictionary (control.py: 7 s without, 176 s
with). It installs as slowly as it makes; Sieve installs these in under half a second.

## What it says

- **Where Sieve is ahead:** the raw picture, by 2.7% (the rope finds it 256 wide and mixing reads it
  so; cmix, given no header, does not). And a short file without cmix's dictionary.
- **Where the gap is widest:**
  - **programs, 36%:** paq8's model of x86 code (instructions and their parts as contexts) against
    Sieve's x86 filter feeding a general mixer;
  - **text, 15% to 26%:** cmix's dictionary, which turns words into short codes before modelling,
    then its word models and many more mixers, SSE stages and an LSTM;
  - **source code, 25%:** the same.
- **For IDEAS 19.126,** in the order they would close the most: a model of x86 instructions (integer,
  paq8's own kind); text by dictionary before mixing (Sieve's word list already primes mixing,
  F80, but does not replace words); more SSE and mixer layers. Each in integers, under a new manifest
  version, measured first.
