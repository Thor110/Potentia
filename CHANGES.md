# The site guard's "Show it anyway", listings in parts, and the guard's cache (27 October 2026)

Unzip over the repository root. Committed locally (not pushed). Rebuild needed: the `sieve` tool
(the web host). Reload the extension. No format, version or address changes.

## The two you asked for
- **"Show it anyway" is kept by the extension, not the page** (`background.js`, `notice.js`,
  `guard.js`). It was in the page's own `sessionStorage`, which the page's scripts could set, and
  then the check was skipped entirely. Now the background keeps it per tab
  (`chrome.storage.session`, forgotten when the tab closes), only the notice's button sets it, and
  it applies only to a page the table does not name: a page that does not match is refused anyway.
- **Listings and site tables come in parts** (`web_host.*`, `background.js`, `check_host.py`), as a
  file's bytes already did. Before: a `.sieve` of 10,000 files gave one message of 1,178,012 bytes,
  which Chrome refuses. After: two parts, and the viewer lists all 10,000. New CI lines in step 11.

## A third, found while trying them in Chromium
- **The guard could pass a page changed on the server** (`background.js`). Its fetch of the page
  came from the extension's own HTTP cache, kept apart from the tab's. With a server sending
  `Cache-Control: max-age=600` (GitHub Pages does) a changed page showed "checked" while the tab
  showed the changed bytes. The fetch now revalidates; the same case is refused.

## Checked here
- CI step 11's web section in a fresh copy, with the new lines: passed. GCC and clang: no warnings.
- Chromium (headless, the test site served, Sieve as the native host, the extension unpacked):
  first visit remembered; a page not in the table whose own script sets the old mark and clicks the
  buttons is still stopped; "Show it anyway" by a real click works in that tab only; a changed page
  refused (plain server and `max-age=600` server); the 10,000-file listing in the viewer; a changed
  table not trusted by the page's clicks, trusted by a real click.
- The unit suite was not run again: nothing it covers changed.

## Files
- `Sieve/tools/cli/web_host.cpp`, `Sieve/tools/cli/web_host.hpp`
- `Sieve/web/extension/background.js`, `Sieve/web/extension/notice.js`, `Sieve/web/extension/guard.js`
- `Sieve/web/host/check_host.py`, `Sieve/ci/steps/11-oracle.sh`
- `Sieve/web/README.md`, `Sieve/docs/REVIEW.md`, `Sieve/docs/HANDOFF.md`
