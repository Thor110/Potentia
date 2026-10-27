# Directory Listing Testing on the Sieve Secure pages (27 October 2026)

Unzip over the repository root. Committed locally (not pushed). No rebuild needed: only the test
site, its build script, CI step 11 and the README changed. `Pages/secure` was rebuilt whole.

**Delete these first** (the rebuild removed them; a zip cannot): `Pages/secure/site.sieve` and the
folder `Pages/secure/images/`. Both were left from a build before the site had its table
(`index.sieve`); nothing links to them.

## What is new
- **`directory.sieve`**, linked from the test site's main page as **Directory Listing Testing**:
  four of the test site's own pictures in folders within folders, and a small page among them.
      drawings/vitruvian.jpg
      hallway/book.jpg
      hallway/door.jpg
      hallway/menus/filters.jpg
      index.html   (shows the four by their paths in the same .sieve)
  326,909 bytes. The viewer opens it as a listing (5 files in 3 folders, each with its size and
  SHA-256); any file opens from it, and the page shows its four pictures from the same `.sieve`.
- **The main page's `pages.sieve` line corrected.** It was under "Links That Should Be Refused",
  saying a many-file `.sieve` is refused; since 24 October it opens as its listing. It is now
  under "Pages That Should Open", beside the new link.
- **`Pages/secure` resieved whole** with `make_site.py ... --out Pages/secure --home ../secure.html`.
  Its table's SHA-256 is now `3317407f517a1ddb6b0b93a2094d82944c80388874d8bc81be0a7ed1c3af9a95`
  (for the crawler; nothing in `secure.html` quotes the old one). A returning visitor is told the
  table has changed, as on any rebuild.

## CI step 11
- The site's table and the crawler count one more file: 14 files, and 30 hashes checked (were 13
  and 28).
- New: `directory.sieve` listed through the host as 5 files in 3 folders, 339,459 bytes, and
  `hallway/menus/filters.jpg` opened from it by its path, the same bytes as the source picture.
- The web section of step 11 passed in a fresh copy.

## Checked in Chromium (headless, Sieve as the host, the extension unpacked)
Served as the website serves it (`Pages/` at the root, the site under `/secure/`): the main page's
link carries its hash and leads to the viewer; the listing shows `drawings/`, `hallway/`,
`menus/` and the five files; `index.html` opened from it shows all four pictures, each taken from
the `.sieve` (560x767 and three of 800x450).

## Files
- `Sieve/web/test-site/make_site.py`, `Sieve/web/test-site/src/index.html`
- `Sieve/ci/steps/11-oracle.sh`, `Sieve/web/README.md`
- `Pages/secure/` (every file; two new: `directory.sieve`, `index.sieve`)
