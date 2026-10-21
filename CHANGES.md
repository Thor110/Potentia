# Fixes for the second GitHub CI run (21 October 2026)

The last fixes worked: Linux and Windows built and passed every unit test, so the checks got
further and found four more problems. None is in Sieve's packing.

- **macOS** (`Sieve/CMakeLists.txt`): the installer was linked with two options only Linux's
  linker knows. On macOS it now uses Apple's own option for the same thing.
- **Windows** (`Sieve/tools/sieve_cli.cpp`): `sieve world --read … --out FILE` built the file name
  from the start of one copy of the name and the end of another copy. That is a bug on every
  system; Linux's compiler happened to get away with it, and Windows' did not ("string too
  long"). Now it uses one copy. No other line does the same.
- **All three** (`.github/workflows/build.yml`):
  - GitHub stops a step when any command in a pipe fails. One check runs a training that is meant
    to fail ("NOT reproduced"), so it stopped the step. It now allows that failure.
  - Two checks expected the AI line's filters in the wrong order.
  - The model step opens the hallway, which needs a screen. It now uses the same no-screen settings
    as the hallway's own step.
  - When a step fails, the log now says which line it stopped at.
- These steps had never run on GitHub before, because the workflow only ran once it moved to the
  repository root. Here, every Linux step now passes, run the way GitHub runs it. Windows and
  macOS run more of the steps than before, so they may still find something new; if they do, send
  me the run and I'll read its log.
- **Docs:** HANDOFF.

To apply: copy `.github/` and `Sieve/` over the repository's top folder.
