# Version 0.13.1

Every "0.13.0" in the repository is now "0.13.1" (16 places; `sieve version` reports `sieve 0.13.1`):
- **The build:** `Sieve/CMakeLists.txt` (`project(... VERSION ...)`, which the tools and the hallway
  are built with) and `Sieve/vcpkg.json`.
- **The release tool:** `Sieve/tools/make_release.py`, in its examples and help.
- **Folder-name examples:** `Sieve/docs/SIEVE-INSTALL-USAGE.md`, and a comment in
  `Sieve/client/unpack_7z.hpp`.
- **The release checklist** in `Sieve/docs/HANDOFF.md`.
- **The status line** in the root `README.md` ("released (v0.13.1)").

Left alone: digit strings in the test vectors that happen to contain "0.13", a colour value in
`build_mesh_templates.py`, and a 0.13 ms timing in HANDOFF.

`version-only.patch` is this change on its own, except HANDOFF.md, which also holds the earlier
rounds' notes. `full-vs-origin-main-ac9096e.patch` and `files/` are everything not yet on origin/main:
the Variable Length Addressing round, the not-written fix, and this.
