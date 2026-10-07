# Using sieve-install

This page covers making an installer for a folder, handing it to someone, and installing from it.
Installers **pack their files themselves** now (`sieve-manifest-v4`), so there is no longer a
separate compressing step.

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

The x86 filter takes 258 KB off the three programs; the masks take the five smaller SCOWL lists
from what LZMA2 makes of them down to about 180 KB of masks. 7-Zip's own Ultra uses BCJ2 rather
than BCJ on programs, which does a little better on them than the x86 filter does, so set the real
7z of a release beside the v4 installer to weigh them (`make_release.py --seven-zip` makes the old
kind from the same build).

Compressing first is no longer needed, and no longer helps: a folder holding one `.7z` is carried
as it is, since an archive does not pack further. (An installer carrying one 7z archive still
unpacks it; see below.)

## Making an installer

From the command line:

```sh
sieve locate RELEASE --program "Release installer.exe"   # an installer program: one file to hand out
sieve locate RELEASE --installer release.sieve           # the installer on its own
sieve locate RELEASE --installer release.sieve --compare # and the sizes beside zip and 7z
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
  installer program, or an installer's manifest (`sieve-manifest-v4` or `-v3`) itself. Add `--force` to
  replace files that are already there.
- **In the hallway:** the File Locator's **Install from Sieve instructions...** (or I) takes a
  `.sieve` or an installer program, then asks for the folder to put it in.
- **Unattended:** `sieve-install NAME.sieve --to FOLDER --yes` installs and exits, returning 0 on
  success.

Every file is checked against its size and SHA-256 before anything is written, so a damaged
installer leaves the destination as it was. Files already there are refused unless you choose to
replace them. Cancel part way removes whatever was written, including the folders it made.

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

- `sieve-install` is built on a trimmed SDL of its own (`SIEVE_SMALL_INSTALLER`, on by default):
  about 1.5 MB on Linux and about 4.9 MB on Windows. That is the fixed cost of every installer
  program.
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
python tools/make_release.py --version 0.13.1                  # the same, with the version given
python tools/make_release.py --version 0.13.1 --skip-build     # the Release build is already made
python tools/make_release.py --version 0.13.1 --seven-zip      # carry a 7z of the folder, as before v4
```

It stages only what the program uses (the programs, their data folders, an empty `maps`
folder, the licences; no map is shipped inside the release, since the published map names the
release's own files) as the folder `Sieve-<version>`, with `hallway` the one program at the top and
`sieve` and `sieve-install` in `tools\` (with a short `README.txt`, so nobody starts the wrong
program first; `sieve` there finds the data folders above it, checked). From the folder it makes
`sieve.sieve` and `sieve.exe`, each the folder packed (`sieve-manifest-v4`), (`sieve-setup` on Linux and macOS, where a program has no
extension), then a sealed `sieve.map` naming both. It then installs each into a scratch folder and compares
the result byte for byte, and writes `SHA256SUMS.txt` and a `RELEASE-NOTES.md` draft. Publish the
three files. GitHub adds the tagged commit's source zip by itself.

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
- More ways for v4 to carry a file: a Sieve item as its place on its line (what `sieve locate
  --weigh` measures), which needs the lines in `sieve-install`; and other "made from another file"
  ways than lines (a file that is another with a few bytes changed: a delta).

## In the future: platform installer APIs

`sieve-install` is one SDL program for every system, so it carries SDL (about 4.9 MB on Windows)
in every installer program. We should move to **platform-specific installer front ends**. On
Windows, that means the system's own windowing and dialogs (Win32, the common item dialog for the
folder), which would bring the program down to a few hundred kilobytes. The manifest reading,
checking and writing (`install_tree` in `tools/cli/locate.cpp`) stays shared, so each platform
only adds its window. Linux can keep the SDL front end until it gets one of its own.
