# Sieve Secure on the website (23 October 2026)

## Built
- **`Pages/secure.html` (Sieve Secure)**, in the website's style, with the AI disclosure at the top:
  - what the extension does, and why the link states the hash rather than the file;
  - the setup in four steps: build Sieve, run `install_host.bat`, load the extension, and open the
    live test site (or run `serve_site.bat` for your own copy);
  - what you should see, and what comes next.

  Its buttons go to the live test site, to the source on GitHub, and back to Core Architecture.
- **`index.html`:** a new button under Sieve State Space, "Sieve Secure (Test The Extension)".
- **`Pages/secure/`:** the test site, built and ready for GitHub Pages to serve (600 KB). GitHub
  Pages serves `.sieve` files like any others, and the extension needs nothing else from the server.
  To rebuild it after changing the site, from the repository's top folder:
  `python Sieve\web\test-site\make_site.py out\build\x64-Release\sieve.exe --out Pages\secure --home ../secure.html`
- **The test site** (`Sieve/web/test-site/src`) is now styled as the website: Courier New, the dark
  boxes and the bordered buttons. Its gallery and home page now say plainly when the extension
  isn't running in the browser, instead of showing broken pictures. The extension marks the pages
  it runs in so they can tell.
- **The many-file `.sieve`** is now the pages' own sources (3 KB) rather than all the pictures
  (600 KB). The Vitruvian JPEG is smaller too (116 KB, was 265 KB), to keep the site light.
- The extension's refusal box and viewer bar use the same font.

## Tested here
In Chromium, with the repository's top folder served as GitHub Pages would serve it:
- the new button leads to Sieve Secure, and its Live Test Site button to the test pages;
- six pictures were shown and the tampered one refused;
- About opened with its picture, and the many-file `.sieve` was refused;
- the link back returns to Sieve Secure;
- without the extension, the gallery says it isn't running.

Screenshots are beside this file (not part of the repository).

**Docs:** `Sieve/web/README.md`, IDEAS §19.92, HANDOFF, VOICES.

To apply: copy `index.html`, `Pages/` and `Sieve/` over the repository's top folder (in `Sieve/`,
only `web/` and `docs/` changed).
