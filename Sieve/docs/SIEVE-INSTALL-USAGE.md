# Using sieve-install

This page covers making an installer for a folder, handing it to someone, and installing from it.
Installers **pack their files themselves**, so there is no separate compressing step. The plain
installer is `sieve-manifest-v4` (`--v4`), described first below; the hallway's File Locator and
`sieve locate --installer` make the newest (`sieve-manifest-v31`), which also mixes, pulls files
tight with the rope, cuts them into pieces, and carries items of Sieve's lines by their places
(*The formats, as they grew*, below; every version in full: SPECIFICATIONS §12.2). Every
option: `sieve help locate`, `sieve help install`, `sieve-install --help`.

## What an installer is

A folder is walked and listed as a manifest: every folder and file, with each file's size and
SHA-256. An installer's manifest (`sieve-manifest-v4`) is that listing, each file's line saying how
it is carried, followed by every file **packed**:

- **programs** (EXE and ELF, by their first bytes) go through the x86 filter, which turns their
  calls' relative addresses into absolute ones that repeat, and then LZMA2;
- a **text made of some of another file's lines**, in its order, is carried as a mask of that file's
  lines, one bit each, where that is smaller (each SCOWL word list but the largest is a mask over
  the largest);
- **everything else** goes into the second stream as it is.

Both streams are LZMA2 at its strongest. The installer is **the address of that manifest**: one
number, stored as raw bytes in a `.sieve` file. Reading the number back gives the manifest, the
manifest gives the files, and every file is checked against its size and SHA-256 before anything is
written. What the packer makes is unpacked and compared before it is used, so a fault in it can
never make an installer that gives back something else. Where packing would not make it smaller (a
few bytes of text, files that are already compressed), the files are carried as they are
(`sieve-manifest-v3`), so an installer is never larger than it need be.

An **installer program** is a copy of `sieve-install` with that installer attached to its end.
It is a single file: run it, and it installs.

| What | Size |
|---|---|
| `.sieve` installer | the folder packed + its listing (usually a few kilobytes) |
| installer program | `sieve-install` + the `.sieve` installer + 24 bytes |

## How small: measured

**Addressing is not compression:** an address is the file it names. What makes an installer small
is the packing, before the address is taken. A folder shaped like a release (the hallway, `sieve`
and `sieve-install`, every data folder; Linux, 7 October 2026), 32,887,678 bytes:

| Made as | Bytes | Share |
|---|---|---|
| zip, each file on its own (deflate, level 9) | 11,597,734 | 35.26% |
| one LZMA2 stream, preset 9e (what 7z does, without its container) | 6,961,919 | 21.17% |
| **`sieve-manifest-v4`, the installer** | **6,577,790** | **20.00%** |

And a Windows release (Edward, 7 October 2026), the installer program from the same build:
carrying a solid 7z of the folder (`make_release.py --seven-zip`), 7,448,947 bytes; packed (v4),
**7,403,749 bytes**, 45,198 bytes (0.61%) smaller.

The x86 filter takes 258 KB off the three programs; the masks take the five smaller SCOWL lists
from what LZMA2 makes of them down to about 180 KB of masks. 7-Zip's own Ultra uses BCJ2 rather
than BCJ on programs, which does a little better on them than the x86 filter does, so set the real
7z of a release beside the v4 installer to weigh them (`make_release.py --seven-zip` makes the old
kind from the same build).

Compressing first is no longer needed, and no longer helps: a folder holding one `.7z` is carried
as it is, since an archive does not pack further. (An installer carrying one 7z archive still
unpacks it; see below.)

## The formats, as they grew

Every version is still read, and `sieve locate --installer` and the File Locator make the newest,
**`sieve-manifest-v31`**, where the format now rests (HANDOFF's standing rules). Each version is in
full in SPECIFICATIONS §12.2; the figures below were measured when each was built.

**The older installers, by flag.** `--v4` makes the plain installer above. `--v3` makes every
file's raw bytes (chosen anyway where packing would not be smaller). `--with-addresses` makes
`sieve-manifest-v2`, a listing with every file's address in hex, twice as large as it need be.
`install` reads them all back, checks every file against its size and SHA-256 before writing any,
and will not replace files without `--force`. With `--compare`, `locate` also shows the
installer's manifest compressed.

**`--items` and after.** `--items` makes the newest. The versions it went through:

| Version | What it added | Measured then |
| :--- | :--- | :--- |
| v5 | A file that is exactly an item of the pages, image, video or audio line (a page as its text, a picture or film as its PNG, a melody as its MIDI) carried as its line and place, made again on installing; one SHA-256 over the whole tree | The degree sample 443,973 bytes (v4 482,496) |
| v6 (`--items --v6`) | Places as raw bytes in one stream; a line's items as places only where smaller than its files; an AI-line model as its digits | The degree sample 391,033 |
| v7 | Nothing written that the rest already gives (counts, empty streams, each file's own SHA-256: `--file-hashes` keeps them); the listing as LZMA2 where smaller; the AI line by `digit-histogram-v1` | The degree sample **9,618** (7z of the files' bytes alone 9,226); a trained model of 34,072 bytes 4,257 (7z 5,576) |
| v8 | The tightened rope (`rope-wave-v1`, Edward's filter): each value predicted from those before it, what is left coded by how often each byte occurs; searched for `--search-per-file` seconds | The Vitruvian picture as grey pixels 832,146 (LZMA2 865,400) |
| v9, v10, v11 | `rope-wave-v2` (fixed-width columns), `v3` (columns from each line's start), `v4` (columns that hold alike share a table) | `pi6.txt` (1,111,103 bytes) 421,491, then 419,115 (7z 512,745) |
| v12 | Less around a file: no line for an empty stream, a file LZMA2 cannot shrink (a PNG, a JPEG) carried as it is | `pi6.txt` 419,096 |
| v13 | A file cut into pieces: LZMA2 for what repeats, the rope for what is unevenly spread | Edward's phone photos 4,111,592 and 4,558,132 (7z 4,127,434 and 4,575,201) |
| v14 | Optimise Compressed Formats (`--optimise-compressed`): an `.xz` remade exactly, a `.7z` carried as its contents | `pi6.txt` as an xz 419,117, as a 7z 419,128 |
| v15 | `rope-wave-v5`: each byte counted by the byte above it too | `pi6.txt` 417,139 (floor 415,241) |
| v16 | The listing's head in bytes (varints, the tree's SHA-256 as 32 bytes) | A small file's listing 142 bytes to 67; `pi6.txt` 417,061 |
| v17 | A `.gz` remade exactly where zlib made it; a `.zip` as its contents | `pi6.txt` as a gzip 417,128, as a zip 417,107 |
| v18 | Maps folded in: a graph part (named-only files, edges, metadata, a seal) behind a head flag | |
| v19, v20 | bzip2 and zstd remade; then lz4, brotli, `.lzma`, lzip, and libarchive's formats (RAR, cab, ISO and more) read; the listing's entries in bytes | A bzip2 word list of 766,439 bytes 469,671 |
| v21 | 7z, zip and gzip made again exactly (preflate for deflate, the LZMA SDK's encoder) | A 7z at `-mx1` 329,861 → 280,971 |
| v22 | A RAR 5 made again by the machine's own `rar` (below) | |
| v23 | `rope-wave-v6`: the triangle (JPEG-LS's median edge detector) | The Vitruvian picture 762,766 |
| v24 | Mixing for text (`mix-v1`) | Don Quixote (2,391,678) 565,664 (683,195 before) |
| v25 | PNGs made again exactly, as their pixels | Ten of this repository's PNGs, 657,221 bytes, 247,695 |
| v26, v27 | `mix-v2` (a match model, columns, the triangle inside mixing); `mix-v3` (a word list's prediction) | Don Quixote 532,501, then 527,837; the ten PNGs 210,046 |
| v28 | `mix-v4`: the SSE stages retired for decoding speed | |
| v29, v30 | Files mixed together, then in groups of up to 8 MiB decoded at once | Sieve's data folder 3,160,426 (v29) → 3,100,643 (v30) |
| v31 | Compressed formats temporary (made again on installing) and permanent (kept as their contents): `--compressed-temporary`, `--compressed-permanent` | The source's 7z 8,005,603 → 6,189,674 |

v8 and later need no line, so `sieve-install` installs them unless they carry items. Prioritise
Decoding Speed (`--prioritise-decoding`) leaves mixing out where installing time matters more than
size. `sieve rope FILE` and `sieve mix FILE` show a file's code beside LZMA2 (`sieve help rope`,
`sieve help mix`), and the weighing (`--weigh`, the File Locator's table) sets every way of naming
a file side by side.

## Making an installer

From the command line:

```sh
sieve locate RELEASE --program "Release installer.exe"   # an installer program: one file to hand out
sieve locate RELEASE --installer release.sieve           # the installer on its own
sieve locate RELEASE --installer release.sieve --compare # and the sizes beside zip and 7z
sieve locate RELEASE --installer release.sieve --v4      # the plain installer: every file packed by LZMA2
sieve locate RELEASE --installer release.sieve --v3      # the older installer: every file as it is
sieve locate notes.txt --installer notes.sieve           # a single file works the same way
```

`--program` needs `sieve-install` beside `sieve`, which is where the build puts it. Make
installer programs from a **Release** build: a Debug `sieve-install` is several times larger.

In the hallway, open the **File Locator** from the pause menu and choose the folder, or a single
file: a file is handled exactly as a folder holding just that file, and installs as that file,
under its name. Then:

- **Save Sieve instructions...** gives the `.sieve` on its own: the folder packed, plus its
  listing. Send this to anyone who already has Sieve.
- **Make an installer program...** gives a single program that anyone can run.

The folder's listing on its own (its manifest: names, sizes and SHA-256s) is part of the
instructions and is for the tools. The command line still writes it (`sieve locate FOLDER
--manifest OUT`), for checking a copy of a folder.

## What to send

| Send | Size | They need |
|---|---|---|
| an installer program | sieve-install + the folder packed + its listing | nothing |
| Sieve instructions (`.sieve`) | the folder packed + its listing | Sieve (`sieve`, `sieve-install` or the hallway) |

## Installing

- **An installer program:** run it. It shows what it installs and where (your Documents folder
  by default; Browse, or type a path). Then choose Install.
- **A `.sieve` file:** put it beside `sieve-install` and run that (it opens the one `.sieve` beside
  it), drop the file on its window, or open the file with it.
- **From the command line:** `sieve install NAME.sieve --to FOLDER`, which also accepts an
  installer program, or an installer's manifest itself (any version, v2 to v31). Add `--force` to
  replace files that are already there.
- **In the hallway:** the File Locator's **Install from Sieve instructions...** (or I) takes a
  `.sieve` or an installer program, then asks for the folder to put it in.
- **From the right-click menu (Windows):** `context\install_context.bat` adds **Sieve > Extract
  here** and **Extract to a folder of its name** to every `.sieve` file, and to every other archive
  Sieve reads (below), each opening `sieve-install` on it (`--extract-here`, `--extract-folder`);
  `context\README.md`.
- **Unattended:** `sieve-install NAME.sieve --to FOLDER --yes` installs and exits, returning 0 on
  success. Files already there are refused, unless `--replace` is added.

Every file is checked against its size and SHA-256 before anything is written, so a damaged
installer leaves the destination as it was. Cancel part way removes whatever was written, including
the folders it made; a file it wrote over is not put back.

**Files already there (29 October 2026).** With Replace not ticked, the installer asks about each
file already in the destination before writing anything, as other extractors do: which file, its
size there now and the size the installer gives, then **Yes**, **No**, **Yes to all**, **No to all**
or **Cancel** (keys Y, N, A, Shift+N and Escape; Enter answers nothing). A file answered No is
left as it was and the rest are written; Cancel writes nothing (from the right-click menu, the
window then closes; otherwise it is back to choose again). Ticking Replace is Yes to all before
it is asked. A RAR made again is asked about by its own name, not its pieces. For testing without
a person, `--answers LETTERS` answers in turn (y, n, a for Yes to all, x for No to all, c for
Cancel) and exits as `--yes` does; each file left as it was is printed.

**Other archives are extracted, as an archiver extracts them (25 October 2026).** Open a `.zip`,
`.rar`, `.tar.gz` or any kind below with `sieve-install` (or right-click it: Sieve > Extract) and it
is unpacked, **one layer at a time**, as 7-Zip does: a `.tar.gz` gives its `.tar`, a `.zip` its
files. A `.sieve` is never changed by this: it gives back exactly what it holds, a `.tar` in it
included. **Extract here** keeps the archive's folders as they are; **Extract to a folder of its
name** (and opening it in the window) puts its contents into a folder named after it, the one top
folder it may have left out (`photos.zip` holding `photos\...` gives `photos\...`, not
`photos\photos\...`). A file already there is asked about (above) unless Replace is ticked, a damaged or
encrypted archive is refused before anything is written, and a link in a tar is left out (said so).
Each file is checked by the vault, as everything an installer writes is. The whole archive is read
into memory first.

| Kind | Names | Read by |
| :--- | :--- | :--- |
| 7z | `.7z` | the LZMA SDK (LZMA, LZMA2, PPMd, BCJ, BCJ2, ARM, Delta) |
| zip | `.zip`, `.zipx` | Sieve (stored, deflate), else libarchive |
| xz, gzip, bzip2, zstd, lz4, lzip | `.xz` `.txz`, `.gz` `.tgz`, `.bz2` `.tbz2` `.tbz`, `.zst` `.tzst`, `.lz4` `.tlz4`, `.lz` `.tlz` | liblzma, zlib, libbzip2, libzstd, lz4, liblzma |
| lzma, brotli (by name: no mark of their own) | `.lzma`, `.br` | liblzma, brotli |
| RAR 4 and 5 | `.rar` | libarchive (read only: no free RAR compressor exists; inside a `.sieve`, a RAR 5 may be made again by your own `rar`, below) |
| cab, ISO 9660, LHA, cpio, ar, WARC, tar | `.cab`, `.iso`, `.lzh` `.lha`, `.cpio`, `.ar` `.deb`, `.warc`, `.tar` | libarchive |

The list is one table in the code (`archive_kinds`, `tools/cli/pack.cpp`); `sieve-install
--archive-extensions` prints its names, which is what the right-click menu is made from, so a kind
added there reaches the menu when `install_context.bat` is run again. (The File Locator's Compressed
formats boxes, temporary and permanent, which carry archives inside `.sieve` files, are another thing: what it does with each kind is in
`sieve help locate` and SPECIFICATIONS §12.2.)

**A RAR made again by your own `rar` (sieve-manifest-v22, 25 October 2026).** With Optimise
Compressed Formats, a RAR 5 that the packing machine's own `rar` (`rar.exe`, WinRAR's) makes again byte
for byte from its files is carried as its contents with that recipe. Installing it (`sieve-install`,
`sieve install`, the hallway) gives the RAR back where the same version of `rar` is found, checked
against its SHA-256. Where none is found, or another version, the files are given in a folder named
after it instead, and a **warning** says so: `sieve-install` and the hallway pop it up in a box of
its own, `sieve install` prints it (`warning   x.rar was not made again as a RAR: rar was not found
...`). Sieve looks for `rar` in `SIEVE_RAR`, beside its own programs, on the `PATH`, and in WinRAR's
folder in Program Files. Every program that installs shows the same notes, from the same code.

**7z archives are unpacked, by the installer only.** When what an installer carries is one 7z
archive (as releases did before v4: `sieve.7z`), `sieve-install` unpacks it automatically into a folder
named after the file, instead of writing the archive itself. Choose `C:\TEST` and `sieve.7z`
unpacks into `C:\TEST\sieve\`, with its files straight inside it. If everything in the archive
sits in one top folder (a release's `Sieve-0.13.1\`), that folder is left out, so you get
`C:\TEST\sieve\hallway.exe` rather than `C:\TEST\sieve\Sieve-0.13.1\hallway.exe`. The window says
"Unpacks" instead of "Installs" and shows what the archive holds. The archive is checked against its
SHA-256 before it is opened, and 7z checks each file's CRC as it unpacks; files already there are
asked about (above) unless you tick Replace, and a cancel or a failure removes what was written. It is a 7z by
its first bytes, not its name. The decoder is the LZMA SDK's (public domain), which reads what
7-Zip writes by default (LZMA, LZMA2, PPMd, and the BCJ, BCJ2, ARM and Delta filters); an
encrypted archive is refused. Only the installer does this: `sieve install` and the hallway's
File Locator give back exactly the file that was located, the archive itself.

Older installers still install: v3 (every file's bytes as they are) and v2 (every file's address
written in hex, twice as large as it need be). v4 is unpacked by the LZMA SDK's decoder (public
domain), the same one `sieve-install` already carried for 7z, so `sieve-install` is no larger for
it; the oracle (`reference/sieve_ref.py unpack`) unpacks v4 with Python's own lzma, independently.

## Sizes to expect

- `sieve-install` is built on a trimmed SDL of its own (`SIEVE_SMALL_INSTALLER`, on by default).
  Since 24 October 2026 it is the full installer, carrying the lines' code and the compressors so
  that it installs anything Sieve makes: measured on Linux (Release), 2,227,000 bytes before and
  5,209,160 after; on Windows 4,497,408 (Edward's WoWRevived installer, 28 October 2026). That is the fixed cost of every
  installer program.
- For a small folder, that fixed cost is most of the program. Hand out the `.sieve` file on its
  own instead, when the other person already has `sieve-install`.
- For a large release, the fixed cost is small beside the files, and the packing is what counts.

## Making a Sieve release

`tools/make_release.py` makes a release in the one order that works, since every file is named by
its hash and every hash depends on the step before it:

On Windows, double-click `tools\make_release.bat`: it runs everything with the defaults (the
version from `CMakeLists.txt`, the Release build in `out\build\x64-Release`) and keeps its window
open. The files go to `Sieve\release\`, which `.gitignore` keeps out of the repository, so they do
not show in GitHub Desktop.

```sh
python tools/make_release.py                                   # build, stage, instructions, program, map, checks
python tools/make_release.py --version 0.13.77                  # the same, with the version given
python tools/make_release.py --version 0.13.77 --skip-build     # the Release build is already made
python tools/make_release.py --version 0.13.77 --seven-zip      # carry a 7z of the folder, as before v4
```

It stages only what the program uses (the programs, their data folders, an empty `maps`
folder, the licences; no map is shipped inside the release, since the published map names the
release's own files) as the folder `Sieve-<version>`, with `hallway` the one program at the top and
`sieve` and `sieve-install` in `tools\` (with a short `README.txt`, so nobody starts the wrong
program first; `sieve` there finds the data folders above it, checked). From the folder it makes
`sieve.sieve` and `sieve.exe` (`sieve-setup` on Linux and macOS, where a program has no
extension), each the folder packed at the newest version (`--items`; until 22 October 2026 they
were v4). It then installs each into a scratch folder and compares the result byte for byte. It writes `metrics.txt`: how much of every line Sieve's reference filters
(`data/metrics/reference-filters.ini`, the hard filters that count together) remove, counted by the
build's hallway as its setup menu counts them (`hallway --metrics`), so one release can be compared
with the next. Last, `SHA256SUMS.txt` and a `RELEASE-NOTES.md` draft, which carries the list too.
GitHub adds the tagged commit's source zip by itself.

**The source, held in the map.** The script also makes `sieve-source.7z`: the Sieve folder's source
as the last commit has it (`git archive` of the Sieve folder alone, run from the repository's top
folder), 7-zipped. So it holds exactly what is committed, the release script included, and none
of the build's or the checks' scratch. The script warns if there are changes not committed, since
they would not be in it, so commit first. The same tree is made into `source.sieve`, Sieve
instructions of the newest version (`sieve locate --items`), which install as the Sieve folder and
are checked to give it back file for file; the 7z is published beside it, the baseline it is
compared with. (Until 27 October 2026 the published map, `sieve.map` and then `source.sieve`, held
the 7z itself, so it was no smaller than the 7z.) Publish five files: the installer, the
instructions, `source.sieve`, `sieve-source.7z` and `metrics.txt`. The whole Potentia repository,
Sieve included, is the release's own "Source code" download on GitHub.

The installers install the runnable folder directly, and are smaller than the 7z route made them
(measured above, at v4; the newest is smaller again: Proof/STATISTICS.md). `--seven-zip` makes them the old way, carrying a solid 7z of the folder, so the
two can be weighed against each other on the same build.

It needs Python 3.8+, CMake, 7-Zip (for the source archive), and Potentia's `LICENSE` one folder up. The build copies that
licence beside the programs as `potentia-license.txt`, and the script refuses to make a release
without it.

## Not yet

- Code signing. Windows SmartScreen warns about unsigned programs downloaded from the internet.
- Other "made from another file" ways than lines (a file that is another with a few bytes changed:
  a delta).

## In the future: platform installer APIs

`sieve-install` is one SDL program for every system, so it carries SDL (about 4.9 MB on Windows)
in every installer program. We should move to **platform-specific installer front ends**. On
Windows, that means the system's own windowing and dialogs (Win32, the common item dialog for the
folder), which would bring the program down to a few hundred kilobytes. The manifest reading,
checking and writing (`install_tree` in `tools/cli/locate.cpp`) stays shared, so each platform
only adds its window. Linux can keep the SDL front end until it gets one of its own.
