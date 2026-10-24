# Step 2: the release carries the web extension and the right-click menu (24 October 2026)

## What changed
- **`tools/make_release.py`:** the release folder now holds, beside the programs:
  - `context\`: `install_context.bat`, `uninstall_context.bat` and their README;
  - `web\extension\`: the extension, to load unpacked;
  - `web\host\`: `install_host.bat`, `uninstall_host.bat` and `install_host.sh`;
  - `web\README.txt`: a short guide written for an installation (the repository's
    `web/README.md` is about the test site and the build).

  The test site stays in the repository: it's built from the repository's pictures, with Python.
  The release notes' "Getting started" now names both, with how to set each up.
- **The scripts find the installed programs by themselves:**
  - `install_context.bat` tries `..\tools\sieve-install.exe` first;
  - `install_host.bat` and `install_host.sh` try `..\..\tools\sieve(.exe)` first;
  - then each falls back to the Release build, as before. A path given still wins.
- **The node graph's "This installation"** now maps `context` and `web`. It also maps `prompts`,
  which releases carried but the list had left out.
- **Docs:** Sieve's README (the installed folder's layout, which now lists every folder a release
  has), `context/README.md`, `web/README.md` and HANDOFF.

## Measured here (Linux, `make_release.py --skip-build`)
- Every check of the release passed: both installers installed back to the release folder, byte
  for byte, and the map verified.
- The two folders add 44,895 bytes to `sieve.sieve` (8,164,254 → 8,209,149), for 64,098 bytes of
  files.
- `web/host/install_host.sh`, run from inside the release folder, registered the release's own
  `tools/sieve`. `check_host.py` then decoded a `.sieve` through it.

## Not tested
- The `.bat` scripts (Windows only). The change is one line in each: try `tools\` first.
- The full CI. Only the build and the release run above.

## To apply
1. Copy `Sieve/` over and rebuild (`client/node_graph.cpp` changed).
2. `tools\make_release.bat` makes a release with them, whenever you want one.
