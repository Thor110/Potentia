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
  - anything else is offered to save.
- **Refused:**
  - a `.sieve` whose link states no hash;
  - one whose own SHA-256 is not the one stated (checked before a byte of it is decoded);
  - one that holds more than one file;
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
   - On its own, it finds `out\build\x64-Release\sieve.exe`. Give another path if yours is
     elsewhere: `web\host\install_host.bat C:\Sieve\tools\sieve.exe`.
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
  wrong hash, no hash, and every picture in one `.sieve`.
- **`gallery.html`**: a slideshow of seven `.sieve` pictures. The fourth had one byte changed after
  its hash was written into the page, and is refused in the slideshow itself.
- **`about.sieve`**, **`notes.sieve`**: pages held in `.sieve` files. About has a `.sieve` picture
  of its own.

Every link with a hash names a file made before the page that links to it, so the pages link back
up by name only: two pages cannot each state the other's hash, since changing either would change
both, without end. Pages that link to one another are checked against one list of every page's
hash instead, which is the next step.

The pictures are the hallway's screenshots from `docs/images` and the public-domain Vitruvian Man
from `data/anchors/public-domain`, as JPEGs (`test-site/src/pictures`).

## How the extension and Sieve talk

`sieve web-host` (`tools/cli/web_host.*`; Chrome starts `sieve` with the extension's origin as its
argument, and that is the same) reads and writes native messaging's messages: each is a 4-byte
length, in this machine's byte order, then that many bytes of JSON.

| Request | Answer |
|---|---|
| `{"id": N, "op": "hello"}` | `{"id": N, "ok": true, "sieve": "0.13.75"}` |
| `{"id": N, "op": "decode", "sha256": "sha256-...", "name": "...", "data": "<the .sieve, base64>"}` | `{"id": N, "ok": true, "name", "type", "size", "sha256", "parts": K}`, then K messages `{"id": N, "part": i, "data": "<base64>"}` |
| either, refused | `{"id": N, "ok": false, "error": "<why>"}` |

A message to the browser is at most 1 MB, so a file comes back in parts of 699,999 bytes (whole
groups of three, so the parts' base64 join as one). `web/host/check_host.py SIEVE FILE.sieve ...`
talks to it as Chrome does, without a browser; the CI runs it.

## Next

- **Pages that link to one another:** one list of every page's hash, a map (`sieve-manifest-v18`),
  whose own hash is the one to trust; a harness that writes a site's link hashes and its map
  together; a crawler that checks a site's links against them.
- **A `.sieve` of many files:** shown as a listing of its folders and files.
- **WebAssembly:** the decoder in the extension itself, so nothing is installed and every browser
  can use it.
