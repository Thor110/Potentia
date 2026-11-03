# The picture viewer in the release (3 November 2026)

Unzip this over the repository root, on top of your commit f4e8200 ("sieve planned post
adjustments"). One local commit, not pushed: 1157ab0. The hallway and sieve-view build here; I
couldn't run make_release.py here (it needs 7-Zip and a Windows Release build), so your next
release is its test.

- **`tools/make_release.py`:** `sieve-view` joins the programs a release carries, in `tools\`
  beside `sieve` and `sieve-install`, so its Sieve button finds the `sieve` beside it. It's
  required like the others: a release stops if the build has no `sieve-view.exe` (it's built with
  the client, as `sieve-install` is).
- **The tools' `README.txt`** says what it is: the picture viewer for PNG, JPEG, BMP, GIF, TGA and
  Sieve instructions holding only pictures (.simage, .isieve), its Sieve button, and
  `sieve-view --help` for its keys.
- **The hallway's "This installation" map** lists it.
- **`docs/SIEVE-INSTALL-USAGE.md`** names it among the release's tools.

Not done: nothing yet opens `.simage` files with the viewer by a double-click. The right-click menu
(`context\install_context.bat`) could register it, if you'd like.
