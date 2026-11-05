# Thumbnails for .simage in Explorer (5 November 2026)

Commit 35f02ac (local, not pushed), on top of your latest.

## What it does

Explorer shows a `.simage`'s first picture as its thumbnail, instead of the viewer's icon.

## How

- **sieve-thumbs.dll:** a Windows thumbnail handler. Explorer loads it in a separate process (`dllhost`), so a fault can't take Explorer down.
- **It reads the file exactly as sieve-view does.** The viewer's reading code moved into `Sieve/client/picture_source.cpp`, shared by both:
  - the listing is read first;
  - every limit is checked before anything is decoded;
  - each file is checked against its SHA-256;
  - a huge JPEG is shown from its eighth.
- **Memory:** a thumbnail uses at most 1 GiB.
- **Failure:** anything refused or damaged simply keeps its icon.
- **Its data folders:** inside dllhost, "the program's folder" would be Windows' own, so the DLL names its own folder. The vault and dictionaries are then found beside it.

## Building and installing

1. Build as usual. `sieve-thumbs.dll` is a new Windows-only target, built into the same folder as `sieve-view.exe`.
2. Run `context\install_context.bat` again. With the DLL beside `sieve-view`, it registers it for your user (`HKCU\Software\Classes\CLSID\{DEA4E1A4-F2CC-427D-BD89-4A2E0231EB4B}`, named from `.simage\ShellEx`). `uninstall_context.bat` removes it.
3. Open a folder holding a `.simage` file in Explorer, with Medium icons or larger.
   - A folder Explorer already showed may keep its old icons until it is reopened.
   - If they persist, Disk Cleanup's "Thumbnails" option clears the cache.

## Tested here, and what only your machine can test

- **Here (Linux):**
  - the shared reading code;
  - the new `sieve-view --thumbnail SIDE OUT.png FILE`, which makes the same thumbnail as a PNG, on a PNG, on a JPEG carried as its brunsli form, on a text `.sieve` (refused) and under a tiny memory budget (refused);
  - CI step 10, which now checks it, passes whole;
  - unit suite: 77,160 checks, 0 failures.
- **The DLL itself:**
  - It compiles cleanly with the MinGW cross-compiler (-Wall -Wextra), but has never been built with MSVC or run.
  - If MSVC reports anything, or the thumbnails don't appear, send me what you see.

## Files

- **New:** `client/thumbs_main.cpp`, `client/thumbs.def`, `client/picture_source.cpp`/`.hpp`.
- **Changed:**
  - `client/view_main.cpp`: uses the shared code, plus `--thumbnail`;
  - `CMakeLists.txt`;
  - `context/install_context.bat` and `uninstall_context.bat`;
  - `tools/make_release.py`: ships the DLL in `tools\`;
  - `tools/cli/dictionaries.*` and `vault.cpp`: the folder override;
  - CI step 10, and MANUAL, FEATURES and the context README.
