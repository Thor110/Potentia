# delta.cpp builds with MSVC (30 October 2026)

Unzip over the repository root, on top of the last zip ("v32 begun"). Committed locally (not
pushed).

**The fault.** `tools/cli/delta.cpp` used `std::to_string` without `#include <string>`. GCC's library
brings that header in through others, and so does MinGW, which uses the same library, so neither
my build nor my Windows check caught it. MSVC's library doesn't.

**The fix.** It now includes `<string>`, and `<cstddef>` for `std::ptrdiff_t`, the one other name it
used without its own header. I checked the files I changed this session the same way; they already
include what they use.

Rebuilt here; a draft pack and install of the 20 MB pair came back identical.

## Files
- `Sieve/tools/cli/delta.cpp`
