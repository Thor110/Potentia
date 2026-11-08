# Changes (8 November 2026)

Replace Sieve/CMakeLists.txt.

Fixes MSVC error D8021 "invalid numeric argument /Werror" in a Debug build: libarchive turns its
ENABLE_WERROR on for Debug, and libFLAC's CMake read the same setting and added GCC's -Werror, which
MSVC refuses. It is now off for libFLAC alone (libarchive keeps its own). Delete the build folder's
CMakeCache.txt (or out/build/x64-Debug) and configure again.
