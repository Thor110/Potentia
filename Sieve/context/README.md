# Sieve in the right-click menu (Windows)

Right-click any `.sieve` file and choose **Sieve**:

- **Extract here**: its files are put beside it, as 7-Zip's "Extract here" does.
- **Extract to a folder of its name**: into a folder named after it (`photos.sieve` into
  `photos\`).

Either opens `sieve-install`, which shows what it will install and where. Choose Install and it
shows the progress. Every file is checked against its SHA-256 before any is written, files
already there are refused unless you tick Replace, and Cancel removes what was written.

## Setting it up

```bat
context\install_context.bat                                  rem sieve-install from out\build\x64-Release
context\install_context.bat C:\Sieve\tools\sieve-install.exe rem or that one (an installation's)
context\uninstall_context.bat                                rem takes it away again
```

It is for your user only (`HKCU\Software\Classes\SystemFileAssociations\.sieve\shell\Sieve`);
nothing else on the machine is changed. On Windows 11 the entry is under **Show more options**
(or Shift+right-click).

`sieve-install` is the full installer: it installs anything Sieve makes, items of Sieve's lines
and remade archives among them, as `sieve install` does.

## Not yet

- **Noted for later (Edward, 24 October 2026):** a "Sieve File/s" entry on any file or folder, to
  make `.sieve` files from them. Whether it should open the command line or a window of its own
  is not decided, so only the extract options are here for now.
