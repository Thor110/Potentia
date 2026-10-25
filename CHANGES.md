# Windows configure fix for libarchive (25 October 2026)

Unzip over the repository root. Commit 55ddad4 locally (not pushed), on top of sieve-v20.zip.

- **The cause:** on Windows only, libarchive's CMake builds and runs a small program against zlib, to see whether it needs `ZLIB_WINAPI` or `ZLIB_DLL`. With the zlib Sieve fetches, that library is a target not yet built, so configuring failed with "ZLIB::ZLIBSTATIC ... the target was not found". Linux never runs that check, which is why my build passed.
- **The fix:** the answers for the static zlib built here (it needs neither) are cached beforehand, along with the library they are for. libarchive skips a check whose answer it already has.
- **Checked here:** a small CMake project with libarchive's check macro copied in, Windows forced on and a zlib target that does not exist. Without the answers, it fails with your error; with them, it configures and gives ZLIB_WINAPI=0 and ZLIB_DLL=0.
- **Not checked:** the real Windows build. If it still fails, the full CMake output would help.
- **Note:** delete `out\build\x64-Debug` (or at least its CMakeCache.txt) before configuring again, so the failed check's state is not kept.
