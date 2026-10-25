# The site's table, checked by the extension and by a crawler (25 October 2026)

As you described it: `index.sieve` is the site's full table, the extension checks pages against it,
and a crawler checks the whole site against it regularly. This also includes the notes from
yesterday's questions (VOICES and HANDOFF: the crawler, and your words on the table).

## `sieve site build`
- **The table is now `index.sieve`,** at the site's top: every file by its SHA-256. It was
  `site.sieve`, which `check` still reads.
- **It prints the table's hash.** Keep it, for the crawler (and for the extension's settings, if
  you like):
  `index.sieve sha256 ...   (the site's table: keep this, for the crawler and to check it)`.
- **Every plain HTML page names the table:** `<link rel="sieve-table" href="index.sieve">`.
- **Scripts and stylesheets** (`<script src>`, `<link rel="stylesheet">` and the like, for the
  site's own files) are given their file's `integrity`. Browsers enforce it themselves, so a
  script changed on the server isn't run, which covers your point that the pages that load scripts
  should point to their hashes too. `check` checks these hashes as well.

## The extension
- **First visit:** a page that names a table makes the extension remember that table's hash for
  the site, and check the page against it.
- **Later visits:** every page of a remembered site is held hidden from its start until its bytes
  match the table. A small green mark then says it was checked. Otherwise the page is stopped and a
  notice put in its place (screenshots included):
  - **refused:** the page isn't the one the table names (both hashes are shown);
  - **the table has changed:** **Trust the new table** (after your own update) or **Leave**;
  - **not in the table:** **Show it anyway** (unchecked, for that tab only) or **Leave**.
- **A settings page** (right-click the extension's icon, Options) lists the remembered sites. It
  can forget one, or save a site's table hash by hand before the first visit.
- **Permissions:** the extension now asks for `storage` and `scripting`. The guard script runs only
  on the sites it remembers. Every other site is untouched.
- **A fix:** the extension never received a reply from Sieve that came in no parts. That affected
  the new table request, and also `hello`, though nothing used `hello` until now.

## The crawler: `web/crawl_site.py` (and `crawl_site.bat`)
`crawl_site.bat https://example.com/ --table SHA256 [--every 60]`
1. Fetches `index.sieve` and compares it with the hash you kept.
2. Fetches every file it names.
3. Runs `sieve site check` on that copy.

Any page, script or table changed on the server is printed as a `MISMATCH`, with exit 1. Run it on
a schedule (Task Scheduler), or with `--every MINUTES`. The `.bat` finds Python as
`bench_make.bat` does, and the release carries both files.

## Seen and tested here
- **Chromium,** with the extension loaded and the test site served:
  - the first visit remembered the table;
  - the second visit, and the folder's own address, were checked;
  - `gallery.html` changed on the server: refused;
  - a page not in the table: offered, then shown anyway;
  - the site rebuilt with a new table: the change was flagged, then trusted, then checked;
  - the settings page listed the site.
- **The crawler:** a site as built, exit 0; a page changed on the server, exit 1; a table that isn't
  the one kept, exit 1.
- **CI:** the table, the host reading it, and the crawler against the test site served locally.
  That block passes when run here.
- **Unit tests:** 0 failures. `check_help.py`: 0 missing.

## What it doesn't cover yet
- **Pages a server writes as it serves them** (PHP and the like) can't have a fixed hash. They show
  as not matching, and they're for when we look at the other web languages.
- **A page's own scripts may start** before the check ends: the page is hidden, not paused. A page
  that fails is stopped and replaced.
- **Signing:** so that an update needs no new trust, later.

## To apply
1. Copy `Sieve/` and `.github/` over, and rebuild.
2. Reload the extension in `chrome://extensions`. It asks for its two new permissions.
3. The website's `Pages/secure` copy of the test site still has `site.sieve`. Build it again with
   the command in `web/README.md` to get `index.sieve` and the table check.
