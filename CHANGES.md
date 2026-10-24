# The full installer, and the right-click menu (24 October 2026)

This zip includes yesterday's right-click menu change as well, so it's the only one to apply.

## `sieve-install` is now the full installer
It installs anything Sieve makes, the same as `sieve install`. That includes the two kinds of file
it used to refuse:
- files carried as items of Sieve's lines, made again from their places;
- archives remade exactly (with Optimise Archive Formats).

There's still one program, used two ways:
- **self-extracting installers:** `sieve locate --program` and the File Locator's "Make an
  installer program" attach the instructions to a copy of `sieve-install`;
- **the right-click menu:** `context\install_context.bat` adds **Sieve > Extract here** and
  **Extract to a folder of its name** to `.sieve` files. Each opens `sieve-install`'s window on the
  file (`--extract-here`, `--extract-folder`), already set up to install there, showing its
  progress.

So both have the full capability. `sieve-install` needs no data files.

**Size** (measured on Linux, Release): 2,227,000 bytes before, 5,209,160 after. Every installer
program carries that. Windows isn't measured yet; it was about 4.9 MB before.

## Tested here
- **A folder with a picture item and a gzip remade:** installed byte for byte by `sieve-install
  --yes`, and by an installer program made from it.
- **The CI:** its four checks that `sieve-install` refuses such files now check that it installs
  every file instead.
- **Both extract options:** the files landed beside the `.sieve`, and in a folder of its name.
- Unit tests and the full local CI (from step 6) pass. `check_help.py`: 0 missing.
- The menu itself hasn't been tried in Windows Explorer.

## Noted, not built
A "Sieve File/s" entry for making `.sieve` files.

## To apply
1. Copy `Sieve/` and `.github/` over, and rebuild.
2. Run `Sieve\context\install_context.bat` (or with an installation's
   `tools\sieve-install.exe`).
