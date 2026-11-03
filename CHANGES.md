# .simage and .isieve open in the picture viewer (3 November 2026)

Unzip this over the repository root, on top of your commit 12bc15e ("sieve post details"). One
local commit, not pushed: ca0cd1b. The .bat files keep their CRLF line endings. I couldn't run the
scripts here (no Windows), so running `context\install_context.bat` is their test.

**`context\install_context.bat`** now also:
- sets `.simage` and `.isieve` to open in `sieve-view` on a double-click (a `Sieve.Picture` file
  type for your user, with the viewer's icon);
- gives `.simage` and `.isieve` the same Sieve submenu as `.sieve` (Extract here, Extract to a
  folder of its name);
- adds **View pictures** to the Sieve submenu of `.sieve`, `.simage` and `.isieve` files. The
  viewer refuses a `.sieve` holding anything but pictures, and says why.

It looks for `sieve-view.exe` beside `sieve-install.exe` (a release's `tools\`, or the build
folder). Without it, everything works as before and it says the viewer wasn't found.

**`context\uninstall_context.bat`** takes all of it away, including the `.simage` and `.isieve`
keys whole (they're Sieve's own kinds of file).

Windows may keep the old icon on existing `.simage` files until Explorer is restarted.

`context\README.md` and the release notes draft (`tools\make_release.py`) describe it.
