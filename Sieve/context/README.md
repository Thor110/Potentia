# Sieve in the right-click menu (Windows)

Right-click any `.sieve` file, or any other archive Sieve reads (a `.zip`, `.7z`, `.rar`, `.tar`,
`.gz`, `.xz`, `.cab`, `.iso` and the rest: `docs/SIEVE-INSTALL-USAGE.md` lists them), and choose
**Sieve**:

- **Extract here**: its files are put beside it, as 7-Zip's "Extract here" does.
- **Extract to a folder of its name**: into a folder named after it (`photos.sieve` into
  `photos\`).

An archive is extracted one layer at a time, as 7-Zip's: a `.tar.gz` gives its `.tar`. Extract
here keeps its folders as they are; Extract to a folder of its name leaves out the one top folder
it may have, so `photos.zip` holding `photos\` does not give `photos\photos\`.

Either starts at once, as an archiver's extract does: `sieve-install`'s window shows the
progress and closes when it is done. Every file of a `.sieve` is checked against its SHA-256 before any is
written. If it cannot (files already there, say), the window stays open and says why: **Back**
returns to its first screen, where you can tick Replace or choose another folder and Install
again; Cancel part way removes what was written.

## Setting it up

```bat
context\install_context.bat                                  rem sieve-install from tools\, else out\build\x64-Release
context\install_context.bat C:\Sieve\tools\sieve-install.exe rem or that one
context\uninstall_context.bat                                rem takes it away again
```

It asks `sieve-install --archive-extensions` which names to add the menu to, so a kind of archive
added to Sieve later is taken up by running it again; `uninstall_context.bat` takes Sieve out of
the menu of every kind of file it is on, whatever the list was.

A release carries this folder beside the programs. There, `install_context.bat` finds
`tools\sieve-install.exe` in the installation by itself; in the repository, the Release build's.

It is for your user only (`HKCU\Software\Classes\SystemFileAssociations\<.ext>\shell\Sieve`, one
for `.sieve` and one for each kind of archive);
nothing else on the machine is changed. On Windows 11 the entry is under **Show more options**
(or Shift+right-click).

`sieve-install` is the full installer: it installs anything Sieve makes, items of Sieve's lines
and remade archives among them, as `sieve install` does.

## Not yet

- **Noted for later (Edward, 24 October 2026):** a "Sieve File/s" entry on any file or folder, to
  make `.sieve` files from them. Whether it should open the command line or a window of its own
  is not decided, so only the extract options are here for now.
