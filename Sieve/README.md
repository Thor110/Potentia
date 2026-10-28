# Sieve

*Packs, checks and unpacks files exactly; and gives every possible text, picture, melody and file an exact address.*

Sieve is the working part of [Potentia](../README.md). It is one command-line tool (`sieve`), a
small installer (`sieve-install`), a right-click menu, a browser extension, and the **hallway**, a
3D walk through the Gallery of Babel. Everything it does is exact and checked: it either gives you
back precisely what went in, or refuses and says why.

**Concept and architecture by Edward James Gordon.** Built with Claude Opus 5.5, from my specifications.

- [What it does](#what-it-does)
- [Get it](#get-it)
- [Build it](#build-it)
- [How it is checked](#how-it-is-checked)
- [Documentation](#documentation)

## What it does

### Smaller archives that come back exactly

```sh
sieve locate MyFolder --installer MyFolder.sieve --items     # pack
sieve install MyFolder.sieve --to D:\Restored                # unpack, every file checked first
```

A `.sieve` holds a folder as instructions for making it again. Each file is packed whichever way
comes out smallest: LZMA2 (7-Zip's method), *mixing* (several models predicting every bit, mixed,
with an English word list for text), an x86 filter for programs, and archives inside the folder
opened and made again byte for byte. Before a `.sieve` is used it is read back and every file
compared, so a fault in the packer can never hand you something else.

Sieve's own release, 44,647,549 bytes of programs and data, is a 6,319,561-byte `.sieve`
(**7.06:1**). On a folder anyone can check, this repository's own `Sieve` source tree:

| Method | Bytes | Ratio | Of the folder | Make | Unpack |
| :--- | ---: | ---: | ---: | ---: | ---: |
| **Sieve** (`.sieve`) | **5,196,966** | **6.81:1** | **14.68%** | 224.7 s | 19.39 s |
| 7z (LZMA2 9 extreme, solid) | 6,844,677 | 5.17:1 | 19.34% | 52.8 s | 1.04 s |
| xz -9e | 6,852,652 | 5.17:1 | 19.36% | 50.3 s | 0.58 s |
| brotli 11, window 24 | 6,943,118 | 5.10:1 | 19.62% | 70.1 s | 0.14 s |
| zstd 22, long window | 7,082,928 | 5.00:1 | 20.01% | 28.0 s | 0.07 s |
| zstd 19 | 7,144,868 | 4.95:1 | 20.18% | 16.1 s | 0.05 s |
| gzip -9 | 10,857,358 | 3.26:1 | 30.67% | 7.2 s | 0.17 s |
| bzip2 -9 | 11,029,078 | 3.21:1 | 31.16% | 3.9 s | 1.66 s |
| zip (deflate 9) | 11,194,250 | 3.16:1 | 31.62% | 7.1 s | 0.26 s |

743 files, 35,396,955 bytes (the `Sieve` folder at commit `4864f4d`, without `data/anchors/`);
Linux, 4 logical processors, 27 October 2026; every result checked to give the folder back. Sieve
comes out 24% smaller than 7z. Part of that is this folder's own: nine English word lists, most of
them the lines of the largest, which Sieve finds and carries as masks over it.

What it costs: time. Making a `.sieve` takes four times as long as 7z here, since it tries several
ways for each file, and unpacking takes seconds where the others take less than one, since mixing
predicts every bit (a mixed group decodes at about 0.6 MB/s a core). It suits what is packed once
and kept or sent many times. Run `python tools/bench/compare.py FOLDER --sieve PATH` to measure a
folder of your own; 7z there was py7zr's (7-Zip's own program is used where it is installed), and
zstd and brotli the Python packages'.

### Extract almost any archive (Windows)

`context\install_context.bat` adds **Sieve** to the right-click menu of `.sieve` files and of every
archive Sieve reads: `.zip`, `.7z`, `.rar` (4 and 5), `.tar`, `.gz`, `.xz`, `.bz2`, `.zst`, `.lz4`,
`.lz`, `.lzma`, `.br`, `.cab`, `.iso`, `.lzh`, `.cpio`, `.ar` and `.warc`. **Extract here** or
**Extract to a folder of its name**, one layer at a time as 7-Zip does. Every file of a `.sieve`
is checked against its SHA-256 before any is written, and cancelling removes what was written.
[context/README.md](context/README.md)

### Installers that check every file

`sieve locate MyFolder --installer MyFolder.sieve --program MyFolder-setup.exe` makes a single
program that installs the folder anywhere, refusing to write anything until every file has been
checked. The hallway's File Locator does the same with a window.
[docs/SIEVE-INSTALL-USAGE.md](docs/SIEVE-INSTALL-USAGE.md)

### Web pages that prove themselves

`sieve site build` turns a website into `.sieve` files, every link stating the SHA-256 of the file
it names, with one table of every file's hash. Sieve's browser extension shows each file only when
it is exactly that file, and holds a remembered site's pages to its table.
[web/README.md](web/README.md), and the live [Sieve Secure](../Pages/secure.html) test site.

### Training anyone can repeat, to the bit

```sh
sieve ai-train tests/training_run_v1      # trains it again, and says whether it came out the same
```

`sieve ai-train` trains a small language model in pinned integer arithmetic, recording everything
it read and in what order. Training it again, on any machine, with any number of threads, gives
the same model to the last bit. This is the core of Potentia's alignment thesis: a training
trajectory that can be replayed, audited and cited.

### The Gallery of Babel

```sh
sieve warp --length 32 "It was the best of times"    # where this text lives, in each ordering
sieve read --length 32 --mode scrambled 007b30165818bf0311497600aa52996f9395e27
```

Every possible page of text, picture, melody, film, 3D model, language model and file has exactly
one address, worked out in exact integer arithmetic and is the same on every machine. Filters set the
noise aside without visiting it: they count exactly how much of a line survives, and number the
survivors. The **hallway** lays it all out as a corridor of bookcases to walk; its setup menu sets
each line's shape and filters. A *guided* ordering gives likely text short addresses (about 2 bits
a character instead of 4.75) and noise long ones.

Why this does not make everything smaller: an address carries exactly as much information as what
it names, so on its own it is not compression. Only the filters and guided ordering shorten
addresses, by knowing what is likely. [docs/IDEAS.md §0](docs/IDEAS.md) sets out the argument.

## Get it

**Windows:** download `sieve.exe` from the [latest release](https://github.com/Thor110/Potentia/releases/latest)
and run it to install. Start `hallway.exe`; the command-line tools are in `tools\` (`sieve help`).
Each release lists every file's SHA-256 to check the download against.

**Linux and macOS:** build it (below).

## Build it

```sh
cmake --preset linux-release          # or x64-Release on Windows, from a VS Developer prompt
cmake --build --preset linux-release
ctest --preset linux-release
```

The core and the `sieve` tool need nothing but a C++20 compiler and CMake. The hallway needs SDL3:
an installed one is used, otherwise the first configure downloads and builds it. Add
`-DSIEVE_BUILD_CLIENT=OFF` to build without the hallway. More in [the manual](docs/MANUAL.md#build).

## How it is checked

- **An independent oracle.** `reference/sieve_ref.py` is written from the specification, shares no
  code with the C++, and generates the conformance vectors the engine must reproduce bit for bit.
- **Tests:** over 77,000 checks, run again with the CPU's own SHA-256 instructions where it has
  them; CI on Windows, Linux and macOS on every push, checking that every platform gives the same
  addresses.
- **Versioned rules.** A filter, alphabet or format that changes is a new version beside the old,
  so an address or a `.sieve` written today still reads tomorrow. The `.sieve` format is
  `sieve-manifest-v31`, and every older version is still read.

## Documentation

| | |
| :--- | :--- |
| `sieve help`, `sieve help COMMAND`, `hallway --help` | Every command and option, kept with the code that reads it |
| [docs/MANUAL.md](docs/MANUAL.md) | The full manual: folders, build, lines, the hallway, filters, commands, canonicalisation, conformance, results |
| [docs/SIEVE-INSTALL-USAGE.md](docs/SIEVE-INSTALL-USAGE.md) | Making installers, installing, making a release |
| [web/README.md](web/README.md), [context/README.md](context/README.md) | The browser extension and site builder; the right-click menu |
| [docs/SPECIFICATIONS.md](docs/SPECIFICATIONS.md) | The specification |
| [docs/IDEAS.md](docs/IDEAS.md) | What is not built yet, and what was set aside and why |
| [docs/HANDOFF.md](docs/HANDOFF.md) | The briefing for picking the work up cold |
| [docs/SHOWCASE-PLAN.md](docs/SHOWCASE-PLAN.md) | The plan for showing what Sieve is good for: benchmarks, demonstrations, audiences |

## Licence

GNU AGPL v3.0, as Potentia. Third-party licences are in [third_party_licenses/](third_party_licenses/).
