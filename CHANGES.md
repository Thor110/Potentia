# Windows build fix for pack.cpp (25 October 2026)

Unzip over the repository root. Commit d29afaf (not pushed); it goes on top of sieve-bzip2-zstd.zip.

- **The cause:** `bzlib.h` includes `<windows.h>` on Windows. pack.cpp included it before defining `NOMINMAX`, so `std::min` and `std::max` became Windows' `min`/`max` macros. Every error in the list came from that.
- **The fix:** `WIN32_LEAN_AND_MEAN` and `NOMINMAX` are now defined before `bzlib.h`, and the later definitions are guarded.
- **Checked:** with a stand-in `windows.h`, the old order leaves `max` defined as a macro and the new order doesn't. The Linux build is clean. I have no MSVC here, so your Debug build is the real check.
- **IDEAS §19.88:** your permanent/temporary archive idea, written down (not built).
