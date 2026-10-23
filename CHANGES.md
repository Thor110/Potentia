# The "not running" note, again (23 October 2026)

What you saw (pictures decoding, yet the note showing) means the extension's main script was
running but the page was never marked as having the extension. The likely cause:
- My last change moved the marking into `mark.js`, which runs the moment a page starts.
- Chrome documents that stage as running "before any other DOM is constructed", so on your Chrome
  the page's `<html>` element probably didn't exist yet, and the mark was never made.
- My headless Chromium had the element, which is why my test passed. This isn't confirmed on your
  Chrome yet.

## Fixed
- `mark.js` waits for the `<html>` element if it isn't there yet.
- `content.js` marks the page again as well, so the mark arrives even if `mark.js` fails. The
  page's watcher then takes the note away.
- Tested with a `mark.js` that marks nothing, and every request held back a second: the note stays
  hidden on the home page and the gallery.

**After applying:** press **Reload** on the Sieve extension in `chrome://extensions` (Chrome keeps
running the old scripts until you do), then reload the page. Nothing else changed: the pages in
`Pages/secure` are the same.

If it still shows, open DevTools on the page (F12) and type
`document.documentElement.getAttribute("data-sieve-extension")` in the Console. "on" means the
extension marked it; null means it didn't. Tell me which, and I'll go from there.

To apply: copy `Sieve/` over the repository's `Sieve/` folder (only `web/extension/` and
`docs/HANDOFF.md` changed).
