# One picture extension: .simage (3 November 2026)

Unzip this over the repository root, on top of your commit c3ed01d ("sieve context viewer"). One
local commit, not pushed: 86204f8. The viewer builds here; the .bat files keep CRLF.

`.isieve` is gone; `.simage` is Sieve's picture file.
- **The viewer** opens a picture, a `.simage`, or any `.sieve` holding only pictures. A `.sieve`
  holding anything else is refused with the reason, as before (checked here: a `.sieve` of a text
  file gives "refused: GAME.md: not a picture (by its recipe and its name), so the .sieve is not
  opened"; one of a JPEG opens).
- **`context\install_context.bat`** sets `.simage` alone to open in the viewer, and adds the Sieve
  submenu (with View pictures) to `.sieve` and `.simage` files.
- **`context\uninstall_context.bat`** still removes `.isieve`'s keys, in case you ran the previous
  version.
- `MANUAL.md`, `context/README.md`, the release script's notes and the viewer's `--help` say
  `.simage` only. `IDEAS.md` notes that `.isieve` was dropped.
