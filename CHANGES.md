# `sieve site`: a harness for any website (23 October 2026)

`make_site.py` had the test site's files built into it. That work is now a Sieve command,
`sieve site`, which builds any site the same way. The test site is built with it, and its `src/`
is now an ordinary site with plain links.

## The command
```bat
sieve site build my-site --out my-site-built --sieve-pages --plain gallery.html
sieve site check my-site-built
```
- **build:**
  - Copies the site.
  - Makes its pictures into `.sieve` files: jpg, jpeg, png, gif, webp, bmp and svg, or the
    extensions you list with `--sieve`.
  - With `--sieve-pages`, makes the pages into `.sieve` files too, except every `index.html` and the
    pages listed with `--plain`.
  - Points every `<img src>`, `<source src>` and `<a href>` at the new `.sieve` file, and adds its
    `integrity` hash. Nothing else in a page changes.
  - Makes each page after the pages it links to. If two pages link to each other, it refuses and
    names them: keep one of them plain.
  - Last, writes `site.sieve`, a map of every file by its SHA-256, and prints that map's own hash:
    the one hash to trust for the whole site.
- **check:** reads a built site as the extension would. It lists every stated hash, and every file
  in `site.sieve`, that doesn't match, and fails if any don't.
- **`data-sieve-keep`:** a tag with this attribute is left exactly as written and isn't checked.
  The test site's deliberately wrong links use it.
- **Safety:** `--out` is replaced only if it's empty or is a site this command built (it has a
  `site.sieve`). Any other folder is refused, so a mistyped path can't wipe something else.
- **Not read yet:** links in CSS and scripts, `srcset`, and links that start with `/`. The
  extension doesn't read `site.sieve` yet; that's the next step, along with the crawler.

`sieve help site` gives all of this.

## The test site
- `src/` has plain links now: `pictures/door.jpg`, `about.html` and so on.
- `make_site.py` only adds the test's own parts, then calls `sieve site build ... --sieve-pages --plain
  gallery.html` and `sieve site check`. The test's own parts are:
  - the link home;
  - the many-file `pages.sieve`;
  - the tampered picture: Vitruvian Man's `.sieve`, made exactly as the build makes it, with one
    byte changed.
- The built pictures are in `pictures/` now, not `images/`.
- The "wrong hash" link now states an empty file's hash. Before, it stated About's hash, which the
  source can't know until the site is built.
- `serve_site.bat` works as before.

## Tested here
- **Test site:** 13 files, 8 made into `.sieve` files (2 of them pages), 11 links given their
  hashes, built in 14.6 s. `site.sieve` is 853 bytes. Check: 24 checked, none wrong, 3 left as
  written. Two builds gave identical files.
- **Chromium 141, with the extension:**
  - 6 slideshow pictures shown, and the tampered one refused;
  - About opened with its own picture;
  - the wrong-hash, no-hash and many-file links refused, each with its reason.
- **Caught by check:** a changed `style.css`, a changed hash in `index.html`, and an extra file.
- **Refused by build:** two pages that link to each other, with both named. They build once one of
  them is kept plain.
- Unit tests pass. The local CI (from step 6) passes, including the new site checks, and the
  libc++ build compiles.

## To apply
1. Copy `Sieve/` over the repository's `Sieve/`, and `.github/` over `.github/`.
2. **Delete `Pages/secure/` first**, then copy the new `Pages/secure/` in. The old `images/`
   folder would otherwise stay behind. `Pages/secure.html` is unchanged.
3. Rebuild Sieve, so `sieve site` exists before you run `serve_site.bat` or `make_site.py`.
