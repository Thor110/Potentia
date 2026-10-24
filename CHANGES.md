# Step 4, the web's phase 2: the first two parts (24 October 2026)

## 1. Links from the server's root (`/pictures/a.png`)
- **`sieve site build|check --served-at PATH`:** PATH is where the site is served from: `/` at the
  server's root, or `/Potentia/` in a folder, as on GitHub Pages. A link under that path is read
  as the site's own. It's made into a `.sieve` and checked like a relative link, and written from
  the root as before (`/Potentia/pictures/door.sieve`).
- **Without it, nothing changes:** such links are left alone, as before. `check` now says how many
  it couldn't read ("3 from the server's root not read (give --served-at)").
- Links to other servers (`//host/...`, `https://...`) are always left as they are.
- **CI:** one site built both ways, plus the check with and without the option.

## 2. A `.sieve` of many files, shown as a listing
- **`sieve web-host`:** a `decode` of a `.sieve` with many files now answers with its listing
  (paths, sizes, SHA-256s, folders) instead of refusing it. A `decode` that names a `path` gives
  that one file: cut from the checked contents and checked against its own SHA-256. Nothing is
  written to the disk.
- **The viewer:** shows the listing as a tree, and each file opens by its path (screenshots
  included). A page among them takes its pictures, and its links to the other files, from the same
  `.sieve`, checked against the same hash. A picture it names that the `.sieve` doesn't hold is
  refused, saying so ("pages.sieve holds no file pictures/door.jpg").
- **Pictures:** an `<img>` whose `.sieve` holds many files is still refused, since a picture names
  one file.
- **`check_host.py`:** has `--listing` and `--path P FILE`. The CI checks the fixture's listing, a
  file by its path, and a path it doesn't hold. The CI line that expected such a `.sieve` to be
  refused is replaced.
- **Seen in Chromium** (headless, the extension loaded, the test site served):
  - "Every page in one" lists its 5 files;
  - About opens from the listing, and its Home, Gallery and Notes links lead to those files by path;
  - the gallery's deliberately tampered picture is still refused.

## Tested here
- Unit tests: 77,033 checks, 0 failures.
- `check_help.py`: 0 missing.
- The new CI lines, run by hand, including under `bash -eo pipefail`.
- Not run: the full CI.

## Left in step 4: two questions for you
1. **The extension reading `site.sieve`.** As agreed in IDEAS §19.92, its hash is the one root to
   trust, pinned in the extension or published apart from the site. My suggestion:
   - a small options page in the extension, where you paste a site's address and the `site.sieve`
     hash that `sieve site build` prints;
   - on that site's pages, the extension checks the page itself, and any file it links to without
     a hash, against `site.sieve`'s list;
   - a bar then says "checked against site.sieve", or what didn't match.

   Shall I build it like that?
2. **The crawler (`site check` against a server).** Sieve has no HTTP client. The simplest way,
   with nothing new to build into Sieve:
   - a Python script fetches `site.sieve` from the server, then every file it names, into a
     temporary folder;
   - it runs `sieve site check` there.

   The other way is an HTTP library built into `sieve` (HTTPS would need OpenSSL on Linux and
   macOS). I'd recommend the Python script.

## To apply
Copy `Sieve/` and `.github/` over, and rebuild. To test, reload the extension in
`chrome://extensions`.
