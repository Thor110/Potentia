# Using sieve-install

This page covers making an installer for a folder, handing it to someone, and installing from it.
Installers **pack their files themselves**, so there is no separate compressing step. The plain
installer is `sieve-manifest-v4` (`--v4`), described first below; the hallway's File Locator and
`sieve locate --installer` make the newest (`sieve-manifest-v16`, or v17 where a gzip is remade), which
also pull files tight with the rope, cut them into pieces, and carry items of Sieve's lines by their
places (*The formats, as they grew*, below; every version in full: SPECIFICATIONS §12.2). Every
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

An installer's manifest (`sieve-manifest-v4`) is the folder structure followed by every file **packed**: programs through the x86 filter, a text made of some of another file's lines as a mask of them (each SCOWL list but the largest), all of it LZMA2 at its strongest, so a release-shaped folder comes to 20% of its size, below one LZMA2 stream of it (21%); its own address is the installer. Unpacking gives every file back exactly, checked against its SHA-256. (`--v3` makes the older installer, every file's raw bytes; it is chosen anyway where packing would not be smaller.) (`--with-addresses` writes `sieve-manifest-v2` instead, a listing with every file's address in hex; installers made from v2 before still install.) `install` reads it back, checks every file against its size and SHA-256 before writing any, and will not replace files without `--force`. With `--compare`, `locate` also shows the installer's manifest compressed. **`--items`** makes `sieve-manifest-v5`: a file that is exactly an item of the pages, image, video or audio line (a page as its text, a picture or film as its PNG, a melody as its MIDI file) is carried as its line and its place there instead of its bytes, made again on installing, with one SHA-256 over the whole tree checking every file. `sieve install` and the hallway install it; the small `sieve-install`, which cannot make items, says to use them instead. **`--items --v6`** makes `sieve-manifest-v6`: each item's place written as raw bytes, the fewest its line's places need, all of them one stream carried raw or as LZMA2, whichever is smaller; a line's items carried as places only where that is smaller than its files packed, and otherwise as files; and a model of the AI line (one `sieve ai --out` writes) carried as its digits. **`--items` now makes `sieve-manifest-v7`**: v6 with nothing written that the rest already gives (the file and byte counts, empty streams, a single file's root, a line's options its id already fixes, and every file's own SHA-256, which the tree's covers: `--file-hashes` keeps them; an item listed as `i K path`), the whole listing carried as LZMA2 where that is smaller, and the AI line's places coded by **digit-histogram-v1** where that is smaller: each tensor's histogram of digits, then the model's rank among the models with those histograms, so a trained model, whose digits are less evenly spread than chance spreads them, has a shorter number. Measured on the degree sample (every dimension at every degree: the README's hallway section): v4 482,496 bytes, v5 443,973, v6 391,033, **v7 9,618** (its listing packs from 203,395 bytes to 6,110). For comparison: `tar` with `xz -9e`, 28,152 bytes; 7z's LZMA2 stream of the files' bytes alone, with no names, 9,226. A model trained by Sieve alone (universalbibliothek, 34,072 bytes as `sieve-ai-16432c2a3953.safetensors`): v4 5,818 bytes (the same as v5, which has no AI line), v6 4,486, **v7 4,257**, its 4,096 bytes of digits coded in 4,020; 7z's LZMA2 of the file, 5,576. The File Locator makes v7 too, with the AI line's shape from the setup menu's settings. **Since 19 October 2026 `--items` makes `sieve-manifest-v8`** where it helps: a file that **the tightened rope** (`rope-wave-v1`, Edward's filter: a wave through the file's values, each predicted from those before it, and what the wave leaves coded by how often each byte occurs) codes smaller than LZMA2 packs it is carried so, its wave searched for `--search-per-file` seconds (the File Locator's time for a file). It wins on smooth data: the Vitruvian picture as raw grey pixels, 832,146 bytes against LZMA2's 865,400, and in colour 468,078 against 623,717; not on prose. Its version 2 (`rope-wave-v2`, `C1`) counts each column of a fixed-width line apart, so a table's separators cost almost nothing: a million digits of pi laid out in lines comes to 420,025 bytes against LZMA2's 486,077, the digits' own floor being 415,241 (instructions with it are `sieve-manifest-v9`). Version 3 (`rope-wave-v3`, `C2`) counts each byte's column from the start of its line, for text whose lines vary or break: Edward's `pi6.txt` (pi to a million digits, 1,111,103 bytes, with blank lines and page breaks) comes to 421,491 bytes as Sieve instructions, against 7z's 512,745 (`sieve-manifest-v10`). Version 4 (`rope-wave-v4`, `G1`) lets columns that hold alike share one table and one stream: `pi6.txt` comes to 419,115 bytes as Sieve instructions (`sieve-manifest-v11`). Version 5 (`rope-wave-v5`, `A1`) counts each byte by the byte above it too, so a column whose bytes take turns line by line costs almost nothing: `pi6.txt` comes to 417,139 bytes (`sieve-manifest-v15`), against the digits' floor of 415,241. `sieve-manifest-v16` writes the listing's head in bytes rather than text (the streams' sizes as varints, the tree's SHA-256 as its 32 bytes): a small file's listing goes from 142 bytes to 67, `pi6.txt` to 417,061 and the trained model to 4,185. `sieve-manifest-v12` writes less around a file's bytes where it can: no line for an empty stream, and a file LZMA2 cannot make smaller (a PNG, a JPEG) carried as it is, so a compressed file's instructions are its bytes and about 145 more; `pi6.txt` is 419,096. `sieve-manifest-v13` cuts a file into pieces where that is smaller, LZMA2 taking what repeats and the rope what is unevenly spread: Edward's phone photos (4,139,682 and 4,587,473 bytes) come to 4,111,592 and 4,558,132, against 7z's 4,127,434 and 4,575,201. **Optimise Archive Formats** (`--optimise-archives`, a box in the File Locator, off unless ticked; `sieve-manifest-v14`): an `.xz` is carried as what it holds and remade exactly on installing, and a `.7z` is carried as its contents, in a folder named after it (said so on installing), each only where clearly smaller: `pi6.txt` as an xz (507,092 bytes) comes to 419,117, as a 7z (507,182) to 419,128. Since 21 October a `.gz` is remade exactly where zlib made it (else carried as what it holds) and a `.zip` is carried as its contents (`sieve-manifest-v17`): `pi6.txt` as a gzip (546,741 bytes) comes to 417,128, as a zip (546,827) to 417,107. v8 needs no line, so `sieve-install` installs it unless it carries items. `sieve rope FILE` shows the wave a file pulls tight to and its code beside LZMA2 (`sieve help rope`), and the weighing (`--weigh`, the File Locator's table) weighs every file's rope code as a way to name it: its code, its wave's parameters and its size, the rope's name paid once.

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
  installer program, or an installer's manifest itself (any version, v2 to v18). Add `--force` to
  replace files that are already there.
- **In the hallway:** the File Locator's **Install from Sieve instructions...** (or I) takes a
  `.sieve` or an installer program, then asks for the folder to put it in.
- **From the right-click menu (Windows):** `context\install_context.bat` adds **Sieve > Extract
  here** and **Extract to a folder of its name** to every `.sieve` file, and to every other archive
  Sieve reads (below), each opening `sieve-install` on it (`--extract-here`, `--extract-folder`);
  `context\README.md`.
- **Unattended:** `sieve-install NAME.sieve --to FOLDER --yes` installs and exits, returning 0 on
  success.

Every file is checked against its size and SHA-256 before anything is written, so a damaged
installer leaves the destination as it was. Files already there are refused unless you choose to
replace them. Cancel part way removes whatever was written, including the folders it made.

**Other archives are extracted, as an archiver extracts them (25 October 2026).** Open a `.zip`,
`.rar`, `.tar.gz` or any kind below with `sieve-install` (or right-click it: Sieve > Extract) and it
is unpacked, **one layer at a time**, as 7-Zip does: a `.tar.gz` gives its `.tar`, a `.zip` its
files. A `.sieve` is never changed by this: it gives back exactly what it holds, a `.tar` in it
included. **Extract here** keeps the archive's folders as they are; **Extract to a folder of its
name** (and opening it in the window) puts its contents into a folder named after it, the one top
folder it may have left out (`photos.zip` holding `photos\...` gives `photos\...`, not
`photos\photos\...`). A file already there is refused unless Replace is ticked, a damaged or
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
added there reaches the menu when `install_context.bat` is run again. (Optimise Archive Formats,
which carries archives inside `.sieve` files, is another thing: what it does with each kind is in
`sieve help locate` and SPECIFICATIONS §12.2.)

**A RAR made again by your own `rar` (sieve-manifest-v22, 25 October 2026).** With Optimise
Archive Formats, a RAR 5 that the packing machine's own `rar` (`rar.exe`, WinRAR's) makes again byte
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
refused unless you tick Replace, and a cancel or a failure removes what was written. It is a 7z by
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
  5,209,160 after; Windows not measured yet (about 4.9 MB before). That is the fixed cost of every
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
python tools/make_release.py --version 0.13.75                  # the same, with the version given
python tools/make_release.py --version 0.13.75 --skip-build     # the Release build is already made
python tools/make_release.py --version 0.13.75 --seven-zip      # carry a 7z of the folder, as before v4
```

It stages only what the program uses (the programs, their data folders, an empty `maps`
folder, the licences; no map is shipped inside the release, since the published map names the
release's own files) as the folder `Sieve-<version>`, with `hallway` the one program at the top and
`sieve` and `sieve-install` in `tools\` (with a short `README.txt`, so nobody starts the wrong
program first; `sieve` there finds the data folders above it, checked). From the folder it makes
`sieve.sieve` and `sieve.exe`, each the folder packed (`sieve-manifest-v4`), (`sieve-setup` on Linux and macOS, where a program has no
extension), then a sealed `sieve.map` naming both. It then installs each into a scratch folder and compares
the result byte for byte. It writes `metrics.txt`: how much of every line Sieve's reference filters
(`data/metrics/reference-filters.ini`, the hard filters that count together) remove, counted by the
build's hallway as its setup menu counts them (`hallway --metrics`), so one release can be compared
with the next. Last, `SHA256SUMS.txt` and a `RELEASE-NOTES.md` draft, which carries the list too.
Publish the four files. GitHub adds the tagged commit's source zip by itself.

**The source, held in the map.** The script also makes `sieve-source.7z`: the Sieve folder's source
as the last commit has it (`git archive` of the Sieve folder alone, run from the repository's top
folder), 7-zipped. So it holds exactly what is committed, the release script included, and none
of the build's or the checks' scratch. The script warns if there are changes not committed, since
they would not be in it, so commit first. The published `sieve.map` names `sieve.exe` and
`sieve.sieve` (found beside it) and **holds** `sieve-source.7z` (its bytes inside the map,
`sieve-map-v2`), so the map is the size of the compressed source. Publish three files: the
installer, the instructions and the map. The whole Potentia repository, Sieve included, is the
release's own "Source code" download on GitHub. (`--map-with FILE` can make the map again to name
another file as well.)

The installers install the runnable folder directly, and are smaller than the 7z route made them
(measured above). `--seven-zip` makes them the old way, carrying a solid 7z of the folder, so the
two can be weighed against each other on the same build.

It needs Python 3.8+, CMake, 7-Zip (for the source archive), and Potentia's `LICENSE` one folder up. The build copies that
licence beside the programs as `potentia-license.txt`, and the script refuses to make a release
without it.

## Not yet

- An icon for `sieve-install` and for the hallway, before the first release.
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
