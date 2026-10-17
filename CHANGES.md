# Changes: 0.13.25, and most of IDEAS §19.79 (17 October 2026)

Copy `files/` over the repository root. This zip also holds everything from the earlier zip
(`sieve-hallway.ini` with every setting), so it replaces it.

- **Version 0.13.25** (`CMakeLists.txt`, `vcpkg.json`, the release tool's examples).
- **Settings > Programs:** an ffmpeg row showing the version found, or **not found** in red with
  where it looked. Enter chooses it with the system's file window; Left goes back to looking.
  Saved as `[paths] ffmpeg`.
- **Red warnings:** every message that says something stopped (warp, go-to, save, J, a chain or an
  anchor that would not open) is red on a dark plate. An MP4/AVI/WebP with no ffmpeg now says
  ffmpeg is missing and where it looked, instead of "no line holds that kind". The command line's
  "not found" messages say where it looked too.
- **Tabs answer the mouse:** click any tab of the item page, CHAT included (Tab frees the mouse).
- **HELP tab on every item:** the keys for what you hold, chosen by its line. It sits before CHAIN
  and CHAT, so C still reaches them in the same order.
- **Chat logs:** Ctrl+L or `/log [PATH]` saves the conversation as text, with the model's line and
  address at the top; without a path, into `chat-logs/` beside the program.
- **The window raise on Windows:** `client/window_front.cpp` (new, in both programs) calls
  `SetForegroundWindow` from the dialog's own callback, with `AttachThreadInput`. **Not compiled
  here**: there are no Windows headers in this container, so your build is its first compile, and
  only you can tell if the window now comes back to the front.
- **CI:** a check of the chat log, and a picture of HELP.
- **Docs:** README (Programs, HELP, clicks on tabs, red warnings, /log), IDEAS §19.79 (each item
  marked), HANDOFF (a new entry, and the open list).
- **Checked here:** every changed file compiles with no warnings against SDL's headers, except
  the Windows half of `window_front.cpp`. Not run here: the full build, unit suite or CI.
- **Worth trying:** Settings > Programs with and without ffmpeg; J on an MP4 with no ffmpeg; Tab
  then a click on COST, HELP and CHAT; Ctrl+L in CHAT; opening a file with + or Ctrl+O and seeing
  whether the window comes back to the front.
