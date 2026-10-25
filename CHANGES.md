# pack.cpp on Windows (25 October 2026)

Unzip over the repository root. Commit 9042409 locally (not pushed). It includes the CMakeLists.txt from sieve-libarchive-windows2.zip, so this zip alone brings you up to date.

- **The errors (pack.cpp lines 790 to 794):** libarchive's entry type was held as `mode_t`, a POSIX type MSVC does not have. It is `auto` now, libarchive's own type.
- **Also:** `locate.cpp` includes `<iterator>` for `std::size`, rather than relying on another header bringing it in, as GCC's do and MSVC's may not.
- **Checked here:** the build is clean, and a cab still reads.
- **Not checked:** MSVC. I read through my other new code for other POSIX-only names and found none, but your build is the check.
