# A second code review: fixes where Sieve reads other people's files (27 October 2026)

Unzip over the repository root. Committed locally (not pushed). Rebuild needed: the core, the
`sieve` tool and `sieve-install` (no format, version or address changes; the hallway is untouched
apart from what it links from `tools/cli`).

The full account, with how each fix was tried, is in `Sieve/docs/REVIEW.md` under "The second pass".

## Fixed
- **A RAR's recipe in a `.sieve` was trusted on installing** (`locate.cpp`, `rar_remake.*`). Its
  `rar` switches went to `rar` as written (`-df` deletes what it archives), and its archive, folder
  and member paths were never checked (members had `chmod` and their times set before anything was
  verified). Now refused when the instructions are read: switches must be ones Sieve writes, paths
  must stay inside the folder (the same rule as every file's path).
- **The web host decoded first and checked its 256 MiB limit after** (`web_host.cpp`,
  `locate.*`). A page can trigger a decode with no click, and a few hundred bytes could declare
  gigabytes. New `DecodeLimit`: declared stream and file sizes are checked before anything is made.
  Set only by the web host; everything else is unchanged.
- **The web host skipped the vault** (`web_host.cpp`). The file it returns is now checked, as
  Extract checks one.
- **The web host could send text that is not UTF-8** (`web_host.cpp`), which makes Chrome close the
  connection. Strings in its replies are now made well-formed.
- **A failed or cancelled Extract with Replace ticked deleted files it had written over**
  (`pack.cpp`, `unpack_archive`). It now removes only what it created, as installing does.
- **The page could press the guard notice's buttons** (`web/extension/notice.js`), including "Trust
  the new table". They now act only on a real click (`event.isTrusted`). The buttons themselves stay.
- **A safetensors header with one name twice could pass** (`safetensors.cpp`), when the two were
  not neighbours in the data. Two unit checks added.
- **Warnings under `-Wall -Wextra -Wpedantic`** (GCC 13 and clang 18): 17 missing-initialiser
  warnings (constructors added to `PackedStream` and `UnpackedArchive`, no call site changed), an
  unused `sat()` in `training.cpp`, and an unused copy of `kL` in `weights.cpp`.

## Checked here
- Unit suite: 77,110 checks, 0 failures (clang Release, core and tool; the hallway was not built).
- CI step 11's web-host block and v22 RAR block run alone against the changed build: passed.
- Each fix tried against crafted input, before and after where the old code could be rebuilt.

## Found and left (questions for you, in `REVIEW.md`)
1. The site guard can be bypassed by a changed page's own scripts (they run before the check and
   share `sessionStorage`, where "Show it anyway" is kept). Moving that flag into the extension is
   a small change; whether to is your call.
2. A listing or site table over 1 MB (measured: about 9,000 files of short names) is sent as one
   native message, which Chrome refuses. Fix needs the host and `background.js` changed together.
3. Smaller items: `stb_image_write` globals read without the lock in `encode_png`; Firefox not
   registered by the host scripts; `install_host.sh` path not JSON-escaped; a detached thread in
   the harness.

## Files
- `Sieve/core/src/safetensors.cpp`, `Sieve/core/src/training.cpp`, `Sieve/core/src/weights.cpp`
- `Sieve/tools/cli/locate.cpp`, `Sieve/tools/cli/locate.hpp`, `Sieve/tools/cli/pack.cpp`
- `Sieve/tools/cli/rar_remake.cpp`, `Sieve/tools/cli/rar_remake.hpp`
- `Sieve/tools/cli/web_host.cpp`, `Sieve/tools/cli/web_host.hpp`
- `Sieve/web/extension/notice.js`
- `Sieve/tests/test_core.cpp`
- `Sieve/docs/REVIEW.md`, `Sieve/docs/HANDOFF.md`
