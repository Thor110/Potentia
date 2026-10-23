# The test site in one double-click (23 October 2026)

- **`Sieve/web/test-site/serve_site.bat`**, step 4 of `web/README.md` as one script. It:
  - finds Python as `make_release.bat` does: the `py` launcher first, then `python`;
  - builds the site with `out\build\x64-Release\sieve.exe` (or the one you give it:
    `serve_site.bat C:\path\to\sieve.exe`);
  - opens http://localhost:8000/ in your browser, and serves the site to this computer only until
    you close the window.
- `web/README.md` and `make_site.py` mention it.

To apply: copy `Sieve/` over the repository's `Sieve/` folder (only `Sieve/web/` changed).
