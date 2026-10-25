# Extract any archive from the right-click menu; WoW notes in engine/ (25 October 2026)

Unzip over the repository root. Commit 222fbcd locally (not pushed), on top of sieve-v21-windows.zip.

## Sieve > Extract on every archive Sieve reads
- **What it does:** right-click a `.zip`, `.7z`, `.rar`, `.tar`, `.gz`, `.xz`, `.cab`, `.iso` and so on, and the Sieve submenu's Extract here and Extract to a folder of its name extract it with `sieve-install`, as 7-Zip does. It opens one layer at a time: a `.tar.gz` gives its `.tar`. A `.sieve` is unchanged and gives back exactly what it holds, a `.tar` included.
- **Kinds:**
  - 7z, zip, xz, gzip, bzip2, zstd, lz4, lzip;
  - lzma and brotli, recognised by name;
  - through libarchive: RAR 4 and 5, cab, ISO, LHA, cpio, ar (`.deb` too), WARC and tar.
  - The table of all of them is in `docs/SIEVE-INSTALL-USAGE.md`, so it doubles as the list of formats to compare against later.
- **Future kinds:** they all come from one table in the code (`archive_kinds()` in `tools/cli/pack.cpp`). `install_context.bat` now asks `sieve-install --archive-extensions` which file types to add the menu to. So a kind added to the table reaches the menu the next time you run the script. `uninstall_context.bat` removes Sieve from every file type it's on, whatever the list was.
- **How it behaves:**
  - Extract here keeps the archive's folders.
  - Extract to a folder of its name leaves out a single top folder, so you don't get `photos\photos\`.
  - Files already there are refused unless you tick Replace.
  - A damaged or encrypted archive is refused before anything is written.
  - Links inside a tar are left out, and the window says so.
  - Every file passes the vault check.
- **Checked here:** a new check, `ci/extract_check.py`, runs in step 10. It makes each kind from the repository's own files and libarchive's test archives, extracts it unattended and compares the result. Steps 10 and 11 pass. Not tested on Windows: the `.bat` registration and Explorer's menu. lz4 is checked only where Python's `lz4` module is installed.

## WoW
- `engine/README.md` and `engine/wow.md` hold the notes: both layouts, FFUH, and the measurements. They point towards the far-future game engine. Nothing of the game is in the repository, and Sieve does nothing with `.WoW` files.

## RAR made again by rar.exe (your warning): not built yet
It needs the RAR rebuild itself first, which doesn't exist yet. Here's what I propose:
- **Packing:** where `rar.exe` is found, the packer unpacks the RAR, packs it again with the switches its headers show, and keeps the recipe only if the result is byte for byte the original. That recipe needs the rar version, the switches, and each file's times to the nanosecond, which is new data to carry, so a new manifest version (v22). Otherwise the RAR is carried as its contents, as now.
- **Installing:** with the same `rar` version, the RAR is rebuilt and checked. Without `rar.exe`, or with another version, its files are extracted into a folder named after it, with a warning in the hallway, `sieve-install` and `sieve install`: "x.rar was not made again: rar.exe was not found; its files are in x\".
- **Testing:** I can't run rar in this session (it isn't installed and rarlab.com is refused). CI would test the plumbing and the warning with a stand-in `rar` script. Whether real rar.exe gives the same bytes back would be checked on your machine.
