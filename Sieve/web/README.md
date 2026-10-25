# Sieve on the web: the extension (a first test)

A Chrome extension that shows the `.sieve` files a page links to, each checked against the hash its
link states, and decoded by Sieve on this computer (IDEAS §19.92). It is a first test, for one
machine: Chrome reaches `sieve.exe` through native messaging, which is registered once.

## What it does

- **A picture whose source is a `.sieve`** (`<img src="a.sieve" integrity="sha256-...">`) is
  shown as the file it holds. If anything is wrong, a red box saying why is shown in its place.
- **A link to a `.sieve`** (`<a href="b.sieve" integrity="sha256-...">`) opens the extension's
  viewer:
  - a page is shown in a frame that runs no scripts, with its own `.sieve` pictures checked and put
    in, and its `.sieve` links leading back to the viewer;
  - a picture, sound, film or text is shown as itself;
  - a `.sieve` of many files is shown as a listing of its folders and files, each with its size and
    SHA-256; a file opens from it by its path, decoded on its own (nothing is written to the disk).
    A page among them takes its pictures, and its links to the others, from the same `.sieve`,
    checked against the same hash; a picture it names that the `.sieve` does not hold is refused,
    saying so (24 October 2026);
  - anything else is offered to save.
- **Refused:**
  - a `.sieve` whose link states no hash;
  - one whose own SHA-256 is not the one stated (checked before a byte of it is decoded);
  - as a picture (`<img>`), one that holds more than one file: a picture names one;
  - one that carries items of Sieve's lines, a remade archive, or a map;
  - one over 256 MiB unpacked.

The hash is the `.sieve` file's own, written as the web already writes one (Subresource Integrity:
`sha256-` and the base64 of its SHA-256). A file's own hashes say only that it was not damaged, not
that it is the one the page meant: anyone can make a `.sieve` with hashes of their own. A hash
stated by the link that names the file is what makes it the one the page meant.

## Setting it up (Windows)

1. **Build Sieve** (Release), as usual.
2. **Register Sieve as the extension's host**, once, for your user only:
   ```bat
   web\host\install_host.bat
   ```
   - On its own, it finds an installation's `tools\sieve.exe` (a release carries `web\extension` and
     `web\host` beside the programs), else `out\build\x64-Release\sieve.exe`. Give another path if
     yours is elsewhere: `web\host\install_host.bat C:\Sieve\tools\sieve.exe`.
   - It writes `%LOCALAPPDATA%\Sieve\com.potentia.sieve.json` and points Chrome and Edge at it
     (registry: `HKCU\Software\Google\Chrome\NativeMessagingHosts\com.potentia.sieve`, and Edge's).
   - `web\host\uninstall_host.bat` undoes all of it.
3. **Load the extension:** `chrome://extensions`, turn on Developer mode, Load unpacked, and choose
   `web\extension`.
   - Its id is `bnigebolckfoapimidlcmbilcbhjbmjg`, fixed by the key in its manifest, and that id
     is the one the host's JSON allows.
4. **Build the test site and serve it:** double-click `web\test-site\serve_site.bat`.
   - It finds Python as `make_release.bat` does: the `py` launcher first, then `python`.
   - It builds the site with `out\build\x64-Release\sieve.exe`, or the `sieve.exe` you give it:
     `serve_site.bat C:\Sieve\tools\sieve.exe`.
   - It opens <http://localhost:8000/> in your browser, and serves the site (to this computer
     only) until you close its window.
   - By hand, the same is
     `python web\test-site\make_site.py out\build\x64-Release\sieve.exe`, then
     `python -m http.server 8000 --directory web\test-site\site`.
   - Serving it is simpler than opening the files, since Chrome will not let an extension read
     `file://` pages without a switch of its own.

On Linux or a Mac, `web/host/install_host.sh [path/to/sieve]` registers it for Chrome, Chromium and
Edge. A browser started with a profile folder of its own (`--user-data-dir`) looks for the host's
JSON in that folder's `NativeMessagingHosts` instead.

## The test site

- **`index.html`**: the links that should open (About, Notes) and those that should be refused: a
  wrong hash (an empty file's), no hash, and every page in one `.sieve`.
- **`gallery.html`**: a slideshow of seven `.sieve` pictures. The fourth is Vitruvian Man's with one
  byte changed after its hash was written into the page, and is refused in the slideshow itself.
- **`about.sieve`**, **`notes.sieve`**: pages held in `.sieve` files. About has a `.sieve` picture
  of its own.

- **`index.sieve`**: the site's table, every file of the site by its SHA-256.

Every link with a hash names a file made before the page that links to it, so the pages link back
up by name only: two pages cannot each state the other's hash, since changing either would change
both, without end. The table holds every file's hash instead, the plain pages (`index.html`,
`gallery.html`) included, and its own hash is the one to trust for all of it.

## A site's table (`index.sieve`)

Edward, 25 October 2026: "index.sieve would be the full table ... a crawler can check all hashes
against the table regularly, meaning that if any page or the table mismatches, we would know
instantly". What `sieve site build` writes, and what checks it:
- **The table**, `index.sieve` at the site's top: every file of the site by its SHA-256
  (sieve-manifest-v18, a map). `sieve site build` prints its own SHA-256: keep it.
- **Every plain HTML page** names the table, `<link rel="sieve-table" href="index.sieve">` (the
  path up to the site's top from where the page is).
- **Scripts and stylesheets** the site loads (`<script src>`, `<link rel="stylesheet|modulepreload|preload">`)
  are given their files' `integrity`, which browsers enforce themselves: a script changed on the
  server is not run, by the page that loads it, as well as being caught by the table (Edward: "unless
  the pages that load them also point to their hashes").
- **The extension** remembers a site's table the first time one of its pages names it, and from
  then on holds every page of that site hidden from its start until its bytes are the ones the
  table names (`guard.js`, which Chrome runs only on the sites remembered; `chrome.scripting`).
  A small mark in the corner says it was checked. Otherwise the page is stopped and a notice put
  in its place:
  - **refused:** the page is not the one the table names (its hash and the table's are shown);
  - **the table has changed:** the table is not the one remembered. If the site's owner updated it,
    **Trust the new table**; if not, leave;
  - **not in the table:** a page the table does not name, which can be shown anyway, unchecked, in
    that tab.
  Its settings page (right-click the icon, Options) lists the sites remembered, forgets one, or
  saves a site's table hash by hand before the first visit, so not even that is taken on trust.
- **The crawler**, `web/crawl_site.py` (or `crawl_site.bat`):
  `python web/crawl_site.py https://example.com/ --table SHA256 [--every MINUTES]`. It fetches the
  table and compares it with the hash you kept, then fetches every file it names and runs
  `sieve site check` on them: any page, script or the table changed on the server is reported
  (exit 1). Run it on a schedule (Task Scheduler, cron), or with `--every`.
- **What it cannot cover yet:** pages a server writes as it serves them (PHP and the like) cannot be
  named by a fixed hash, so they show as not matching. A page's own scripts may start before the
  extension's check ends (the page is hidden, not paused); a page that fails is stopped.
- **Updating a site:** after a rebuild, a returning visitor is told the table has changed and
  chooses **Trust the new table** (Edward, 25 October 2026: "trust the new table isn't a terrible
  option at all"). No signing: "I don't trust myself or any human being alive to keep a key safe".
  A server replaced whole is out of the extension's hands; the crawler, holding the hash the build
  printed, is what notices it.
- **Seen in Chromium** (25 October 2026, the test site served): the first visit remembered the
  table; later visits checked (the folder's address as its `index.html`); `gallery.html` changed
  on the server, refused; a page not in the table, offered and shown anyway; the site rebuilt with a
  new table, asked about, trusted, then checked; the settings page listing it. The crawler: all as
  kept (exit 0), a page changed (exit 1), a table not the one kept (exit 1); the CI runs it.

## Any site: `sieve site`

The test site is built by `sieve site build`, which builds any site the same way (`sieve help
site`, `tools/cli/site.*`). Write the site with plain links, as for any server:
`<img src="pictures/door.jpg">`, `<a href="notes.html">`. Then

```bat
sieve site build my-site --out my-site-built --sieve-pages --plain gallery.html
sieve site check my-site-built
```

- **build** copies the site.
  - A picture (jpg, jpeg, png, gif, webp, bmp and svg; `--sieve` names others) that an `<img
    src>`, `<source src>` or `<a href>` names is made into a `.sieve`, and that tag is changed to
    it and given its `integrity`. A picture no such tag names is copied as it is. A link to a
    `.sieve` already in the site is given its hash too. Nothing else in a page is changed.
  - With `--sieve-pages`, a page an `<a>` names is made into a `.sieve` too, but not every
    `index.html`, those given to `--plain`, or a page with a script (the viewer runs none).
  - A page is made after the pages it names. Pages that link in a loop are refused before
    anything is made, with the loop and a `--plain` list that would break every loop.
  - A local script or stylesheet is given its file's `integrity`, and every plain HTML page names
    the site's table.
  - Last it writes the table, `index.sieve`, and prints its SHA-256 (above).
  - A tag with `data-sieve-keep` is left as written (the test site's links meant to be wrong).
  - A build that fails leaves nothing behind, and `--out` is replaced only if it is empty or was
    built before.
- **check** reads a built site as the extension would, and lists every stated hash, and every file
  of the table, that does not match; it ends in an error if any does not.

### PHP, scripts and CSS

Only the tags above can be checked: the extension sees them and swaps in the checked file. Every
other way of naming a file goes past it, so `build` keeps such a file as it is, beside its
`.sieve`, and the page that names it still works (unchecked) with or without the extension:
- **PHP:** a `.php` page is read for its tags and kept a `.php`, since its server must run it. Its
  plain HTML is rewritten as any page's; its code (`<?php ... ?>`) is never touched, and a file it
  names is kept as it is too. What PHP writes when it runs cannot be seen beforehand: build the
  pages PHP serves (saved from the running site) instead, or have PHP write each link's
  `integrity` from the table.
- **Scripts:** a file a script names (`fetch("a.png")`, `img.src = "a.png"`) is kept as it is
  too. It is found by its name, so a name a script puts together from parts (`"pic" + n +
  ".png"`) is not: `--keep-originals` keeps every picture as it is beside its `.sieve`.
- **CSS, `srcset`, `style` and other attributes** (`url(a.png)`, `<link>`, `<video>`,
  `<meta content>`): the same, kept as they are. A picture with a `srcset` or in a `<picture>`
  would show those files instead of its `src`, so the extension takes them away when it shows the
  checked `.sieve`.
- **Links from the server's root** (`/a.png`): left as they are, unless the site says where it is
  served from, `--served-at /` at the server's root or `--served-at /Potentia/` in a folder (give
  `check` the same). Then a link under that path is the site's own, made and checked as a relative
  one is, and kept from the root as written (`/pictures/a.sieve`). `check` without it counts those
  it could not read ("3 from the server's root not read"). Links to other servers (`//host/...`,
  `https://...`) are always left as they are.

The Potentia website itself (its top `index.html`, `Pages`, `Images` and `Art`), built with
`--sieve-pages` on 23 October 2026: 101 files, its 3 pages with no script made into `.sieve` files
in 2.9 s, its 5 pages with scripts kept plain, and its 90 pictures copied as they are, since no
page of it shows one yet; checked, none wrong.

`make_site.py` adds only what is the test's own before calling it: the link home, the many-file
`pages.sieve`, and the tampered picture with its hash.

The pictures are the hallway's screenshots from `docs/images` and the public-domain Vitruvian Man
from `data/anchors/public-domain`, as JPEGs (`test-site/src/pictures`). The pages are styled as the
Potentia website is, and say so when the extension is not running, rather than show broken
pictures.

**On the website:** `Pages/secure.html` (Sieve Secure) explains the setup in four steps and links to
a built copy of the test site in `Pages/secure`, which GitHub Pages serves as it serves any file.
Build it again after changing the site, from the repository's top folder:

```bat
python Sieve\web\test-site\make_site.py out\build\x64-Release\sieve.exe --out Pages\secure --home ../secure.html
```

(`--home` adds a link back to the Sieve Secure page; the `.sieve` files are read the same by any
build of Sieve, so the copy need not be made by yours.)

## How the extension and Sieve talk

`sieve web-host` (`tools/cli/web_host.*`; Chrome starts `sieve` with the extension's origin as its
argument, and that is the same) reads and writes native messaging's messages: each is a 4-byte
length, in this machine's byte order, then that many bytes of JSON.

| Request | Answer |
|---|---|
| `{"id": N, "op": "hello"}` | `{"id": N, "ok": true, "sieve": "0.13.75"}` |
| `{"id": N, "op": "decode", "sha256": "sha256-...", "name": "...", "data": "<the .sieve, base64>"}`, and `"path"` for one file of many | `{"id": N, "ok": true, "name", "type", "size", "sha256", "seconds", "parts": K}`, then K messages `{"id": N, "part": i, "data": "<base64>"}` |
| a decode of a `.sieve` of many files, no `"path"` | `{"id": N, "ok": true, "listing": true, "name", "files": [{"path", "size", "sha256"}, ...], "folders": ["a/", ...], "size", "seconds", "parts": 0}` |
| either, refused | `{"id": N, "ok": false, "error": "<why>"}` |

A message to the browser is at most 1 MB, so a file comes back in parts of 699,999 bytes (whole
groups of three, so the parts' base64 join as one). `seconds` is how long checking and decoding it took in Sieve;
the viewer's bar gives it, with how long fetching the `.sieve` and checking its hash took, the
whole hand-over to Sieve and back, and (for a page) its own `.sieve` pictures. A picture's tooltip
gives its times too.

A page can tell whether the extension is running: `mark.js` marks every page as it starts
(`<html data-sieve-extension="on">`), before the page's own scripts look. The test site's pages
look once they have loaded, so a slow connection is not taken for a missing extension. `web/host/check_host.py SIEVE FILE.sieve ...`
talks to it as Chrome does, without a browser; the CI runs it.

## Next

- **Pages PHP writes:** `integrity` from the table at run time (a small PHP helper), or the
  served pages saved and built.
- **Scripts that load other files** (and PHP's output): what they fetch, checked as well.
- **WebAssembly:** the decoder in the extension itself, so nothing is installed and every browser
  can use it.
