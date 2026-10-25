# Windows fixes for v21 (25 October 2026)

Unzip over the repository root. Commit 0802d38 locally (not pushed), on top of sieve-v21.zip.

## "archive_static links to ZLIB::ZLIBSTATIC but the target was not found"
- **What happened:** the first configure worked. Any later one, which happens whenever CMakeLists.txt changes, failed. The cache entries left for libarchive (zlib as the target `ZLIB::ZLIBSTATIC`) were taken by Sieve's own zlib search for a system zlib, so zlib wasn't fetched and the target never existed.
- **The fix:** those entries are now forgotten before Sieve looks for zlib.
- **Reproduced and checked here:** a build with the system's libraries hidden from CMake, as on Windows. Before the fix it failed on the second configure with your error. After it, three configures in a row work and it builds. Clearing your cache would only have got one build through.

## preflate under MSVC (task_pool.h: `result_of` is not a member of `std`)
- **The cause:** preflate's headers use `std::result_of`, which C++20 removed and MSVC enforces. GCC still allows it, which is why it built here.
- **The fix:** preflate is built as C++17. The rest of Sieve reaches it through `tools/cli/preflate_glue.hpp`, two functions in a header that includes nothing of preflate's. Its files are still unchanged.
- **Checked here:** builds, and step 11's v21 block passes. Not checked under MSVC.

## Notes, as you asked (IDEAS §19.88)
- **RAR:** `rar` reads every older RAR, but packing again gives the same bytes only with the same version and settings that made it. RAR is listed as read and carried as its contents; it's rebuilt only by the same `rar` that made it, and nothing more is built for now.
- **Headers kept as they are:** a per-format header model (rebuilt from the files and a few settings, keeping only what differs, as preflate does for deflate) would carry less. Headers are small and LZMA2 already packs their repeats, so it waits.
- **Your view:** optimising archives is a pleasantry, not core. The far aim is to replace them with `.sieve`, reading them meanwhile so what they hold reaches the future.
