# Version 0.13.76, and the macOS emails explained (28 October 2026)

Unzip over the repository root. Committed locally (not pushed). Reconfigure (CMake reads the
version) and rebuild; no code changed.

- The version is **0.13.76**, to keep the next build apart from the published release:
  `Sieve/CMakeLists.txt`, `Sieve/vcpkg.json`, the extension's `manifest.json`, the web host's `hello`
  example in `Sieve/web/README.md`, the examples in `Sieve/tools/make_release.py` and
  `Sieve/docs/SIEVE-INSTALL-USAGE.md`, and the status line in `VISION.md`. The build records in
  `metrics/` and HANDOFF's history keep 0.13.75, as they were.
- `Sieve/docs/HANDOFF.md`: the macOS emails. Runs 68 and 80 "failed" because their macOS job never
  got a runner and was cancelled after about 15 minutes; no step ran, Windows and Linux passed, and
  macOS passed on the runs either side. Not a fault in the code.

## Files
- `Sieve/CMakeLists.txt`
- `Sieve/docs/HANDOFF.md`
- `Sieve/docs/SIEVE-INSTALL-USAGE.md`
- `Sieve/tools/make_release.py`
- `Sieve/vcpkg.json`
- `Sieve/web/README.md`
- `Sieve/web/extension/manifest.json`
- `VISION.md`
