# CI on GitHub: the fourth round (21 October 2026)

All three systems got further than before, and each stopped somewhere new. One was a bug in the
hallway itself; the other two were differences between the systems.

- **Linux: a hallway bug** (`Sieve/client/menu.cpp`). On GitHub's bigger machine, the setup
  menu's "find my limits" grew the AI line past 64 layers, the most its format allows, and the
  hallway then refused its own settings ("layers from 1 to 64"). It would happen on any machine
  with enough memory. Growing now stops at a line's own limit. Tested here with a far larger
  budget: it stops at 64, with no error. The check now also confirms the saved layers are 1 to 64.
- **macOS:** it builds now, so last round's link fix worked. The hallway's checks couldn't open
  a window: on a Mac, SDL's no-screen driver still asks for OpenGL, which GitHub's Macs don't have.
  On macOS that step now uses SDL's other no-screen driver ("dummy"); Linux and Windows are
  unchanged (`.github/workflows/build.yml`).
- **Windows:** past last round's sine fix, it stopped where `sieve map` prints a map to the
  console. Windows turns each line ending into a carriage return plus line feed there, so the
  printed map no longer matched the file. `sieve map` now prints the map's exact bytes on every
  system (`Sieve/tools/sieve_cli.cpp`). No other check compares Sieve's console output byte for byte.
- **Docs:** HANDOFF.

Every step passes here on Linux. I can't run Windows or macOS here, and each now gets further than
it ever has, so there may still be something new. If so, tell me and I'll read the logs.

To apply: copy `.github/` and `Sieve/` over the repository's top folder.
