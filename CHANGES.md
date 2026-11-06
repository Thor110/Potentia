# 0.13.78: the quick wins (6 November 2026)

Ten commits on top of your 51fd284 (b55487f to fe96326), all local, not pushed.

## The version
**0.13.78**, in the same places as the last bump:
- CMakeLists.txt and vcpkg.json;
- the extension's manifest and web/README;
- make_release.py's examples;
- SIEVE-INSTALL-USAGE and VISION.

HANDOFF notes it.

## The seven

1. **No program icon on `.simage` thumbnails.**
   - `install_context.bat` sets `TypeOverlay` to empty on `Sieve.Picture`, and uninstall removes the class.
   - Untested here (no Windows): run `install_context.bat` again and look.
2. **Version info in the Windows programs.**
   - Properties → Details now shows Sieve 0.13.78, for every program and every installer program.
   - Built with MinGW's resource compiler here; please confirm MSVC builds it.
3. **One-off command-line options are no longer saved.**
   - Options such as `--search-every-wave`, `--filter-memory` and `--prioritise-decoding` used to be written into the settings file by the next setting saved.
   - Measured on a new settings file:
     - before: 1, 100 and 1 written;
     - now: the file's own 0, 512 and 0.
4. **`sieve.exe` and accented paths on older Windows.**
   - Windows 10 before 1903, and Windows 7, ignore the UTF-8 request in the manifest. There, `sieve.exe` now reads its command line in UTF-16 itself, as SDL already does for the windowed programs.
   - Compiled for Windows only; not run on 7.
5. **Names Windows cannot hold:**
   - device names (`aux.txt`, `CON`, `COM1`…);
   - names ending in a dot or a space;
   - `< > : " \ | ? *`: a colon silently writes a hidden stream on NTFS.

   On Windows the installers refuse them before writing anything, saying why. Packing warns about them on every system. `tests/windows_names.sieve` (`aux.txt`, made on Linux) is checked in CI step 11.
6. **An installer program can offer its own folder.**
   - **How to set it:** `sieve locate FOLDER --program OUT --install-folder Documents/Games/WoW`, or in the File Locator. After "Make an installer program..." picks the name, a prompt asks for the folder:
     - it starts as `Documents/<name>`, and Enter keeps it;
     - empty offers none;
     - Esc doesn't make the program.
   - **The places:** Documents, Desktop, Downloads or Home, the person's own wherever it runs.
   - **Checks:** `..`, absolute paths and names Windows can't hold are refused when the program is made, and checked again when it is read.
   - **Where it is kept:** in a small block beside the instructions, so `.sieve` files are unchanged. SPECIFICATIONS §12.2 describes it.
   - **Tested here:** installed exactly into the offered folder, and 8 bad folders were refused, each with a reason.
   - **Please try:** the File Locator prompt. It follows a system dialog, so it can't be scripted.
7. **More formats in the comparison.**
   - The File Locator's **More formats** box (key C, kept in the settings), or `sieve locate PATH --compare --more-formats`.
   - Adds gzip -9, bzip2 -9, zstd -19 --long=27, brotli -q 11 -w 24 and lz4 -12, each the whole as one stream, without its container.
   - Unticked by default, as brotli at 11 is slow.
   - **Checked against the real tools:**
     - bzip2: 46,227 bytes, the same as `bzip2 -9`;
     - gzip: 55,967 bytes, which is `gzip -9`'s 55,995 minus its 18-byte header and trailer and the 10-byte stored name.

## Also
- `docs/ODDITIES.md` covers your click during training and the two other models, which both reproduce.
- `hallway --help` names `--more-formats`, so `check_help.py` is clean (620 options named, 0 missing).

## Tests here
- **Unit suite:** 77,160 checks, 0 failures.
- **CI step 10:** passes in full (1,090 s). It checks the offered folder, the dictionaries, the formats and the help.
- **CI step 11:** only the new reserved-names block was run (its Linux half).
