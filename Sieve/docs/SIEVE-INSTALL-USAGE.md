# Using sieve-install

This page covers making an installer for a folder, handing it to someone, and installing from it,
along with the one step that decides how large the installer is: **compress first**.

## What an installer is

A folder is walked and listed as a manifest: every folder and file, with each file's size and
SHA-256. An installer's manifest (`sieve-manifest-v3`) is that listing followed by every file's
raw bytes, one after another. The installer is **the address of that manifest**: one number,
stored as raw bytes in a `.sieve` file. Reading the number back gives the manifest, and the
manifest gives the files.

An **installer program** is a copy of `sieve-install` with that installer attached to its end.
It is a single file: run it, and it installs.

| What | Size |
|---|---|
| `.sieve` installer | the folder's bytes + its listing (usually a few hundred bytes) |
| installer program | `sieve-install` + the `.sieve` installer + 24 bytes |

## Proper procedure: compress first

**Addressing is not compression.** A file's address is the file itself plus `0101...01`, so an
installer is never smaller than what it carries. Whatever size you hand to `sieve locate` is the
size you get back, plus a few hundred bytes. The installer does no compressing of its own.

So the proper procedure is to **compress the folder first**, and to make the installer from the
compressed archive:

1. Put the release together in a folder.
2. Compress it with 7-Zip (7z, LZMA2, level Ultra). 7z usually does far better than zip, because
   it compresses the whole folder as one stream.
3. Put the `.7z` file in a folder of its own, and make the installer from that folder.
4. Whoever installs it gets the `.7z` back, checked against its SHA-256, and extracts it.

Measured on the Sieve source (September 2026, Windows, trimmed `sieve-install`):

| Made from | Archive | Installer program, sieve-install 7.29 MB | Installer program, trimmed (4.94 MB) |
|---|---|---|---|
| the zip as downloaded | 5,062,498 bytes | 12,349,957 bytes | |
| extracted and recompressed as 7z | 3,245,824 bytes | 10,533,282 bytes | |
| the same, a later build | 3,247,142 bytes | | **8,189,640 bytes** |

The same source as a 7z is about a third smaller than as a zip, and the installer shrinks by
exactly that. Trimming `sieve-install` took off another 2.3 MB. Compressing an archive that is already compressed gains almost nothing (the zip
under 7z: 5,002,467 bytes, about 1%). Extract it and compress the files themselves.

## Making an installer

From the command line:

```sh
sieve locate RELEASE --program "Release installer.exe"   # an installer program: one file to hand out
sieve locate RELEASE --installer release.sieve           # the installer on its own
sieve locate RELEASE --installer release.sieve --compare # and the sizes beside zip and 7z
sieve locate Release.7z --installer release.sieve        # a single file works the same way
```

`--program` needs `sieve-install` beside `sieve`, which is where the build puts it. Make
installer programs from a **Release** build: a Debug `sieve-install` is several times larger.

In the hallway, open the **File Locator** from the pause menu and choose the folder, or a single
file: a file is handled exactly as a folder holding just that file, and installs as that file,
under its name. Then:

- **Save Sieve instructions...** gives the `.sieve` on its own: the folder's size plus a few
  hundred bytes. Send this to anyone who already has Sieve.
- **Make an installer program...** gives a single program that anyone can run.

The folder's listing on its own (its manifest: names, sizes and SHA-256s) is part of the
instructions and is for the tools. The command line still writes it (`sieve locate FOLDER
--manifest OUT`), for checking a copy of a folder.

## What to send

| Send | Size | They need |
|---|---|---|
| an installer program | sieve-install + the folder + a few hundred bytes | nothing |
| Sieve instructions (`.sieve`) | the folder + a few hundred bytes | Sieve (`sieve`, `sieve-install` or the hallway) |

## Installing

- **An installer program:** run it. It shows what it installs and where (your Documents folder
  by default; Browse, or type a path). Then choose Install.
- **A `.sieve` file:** put it beside `sieve-install` and run that (it opens the one `.sieve` beside
  it), drop the file on its window, or open the file with it.
- **From the command line:** `sieve install NAME.sieve --to FOLDER`, which also accepts an
  installer program, or an installer's manifest (`sieve-manifest-v3`) itself. Add `--force` to
  replace files that are already there.
- **In the hallway:** the File Locator's **Install from Sieve instructions...** (or I) takes a
  `.sieve` or an installer program, then asks for the folder to put it in.
- **Unattended:** `sieve-install NAME.sieve --to FOLDER --yes` installs and exits, returning 0 on
  success.

Every file is checked against its size and SHA-256 before anything is written, so a damaged
installer leaves the destination as it was. Files already there are refused unless you choose to
replace them. Cancel part way removes whatever was written, including the folders it made.

Installers made before `sieve-manifest-v3` (v2 manifests, with every file's address written in hex)
still install. They are simply twice as large as they need to be.

## Sizes to expect

- `sieve-install` is built on a trimmed SDL of its own (`SIEVE_SMALL_INSTALLER`, on by default):
  about 1.5 MB on Linux and about 4.9 MB on Windows. That is the fixed cost of every installer
  program.
- For a small folder, that fixed cost is most of the program. Hand out the `.sieve` file on its
  own instead, when the other person already has `sieve-install`.
- For a large release, the fixed cost is small beside the files, and compressing first is what
  counts.

## Not yet

- An icon for `sieve-install` and for the hallway, before the first release.
- Code signing. Windows SmartScreen warns about unsigned programs downloaded from the internet.
- Optional compression inside the installer (for example `--pack lzma2`): the files as one LZMA2
  stream, used only when it comes out smaller. Until then, compress first as above.

## In the future: platform installer APIs

`sieve-install` is one SDL program for every system, so it carries SDL (about 4.9 MB on Windows)
in every installer program. We should move to **platform-specific installer front ends**. On
Windows, that means the system's own windowing and dialogs (Win32, the common item dialog for the
folder), which would bring the program down to a few hundred kilobytes. The manifest reading,
checking and writing (`install_tree` in `tools/cli/locate.cpp`) stays shared, so each platform
only adds its window. Linux can keep the SDL front end until it gets one of its own.
