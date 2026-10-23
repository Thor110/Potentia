# The web plugin: no false "not running", and the decode times (23 October 2026)

## Fixed
- **"The Sieve extension is not running" while it was.** I couldn't reproduce it, but the likely
  cause is timing. The extension marked the page only once the page had settled, and the page gave
  up waiting after 1.5 seconds, which a real connection to GitHub Pages can take longer than.
  - A tiny script (`web/extension/mark.js`) now marks every page the moment it starts loading.
  - The test pages wait until they've fully loaded before checking, and take the note away again
    if the mark arrives later anyway.
  - Tested with every request held back 2 seconds: the note stays hidden.

## Added
- **The decode times, in the viewer's bar after decoding:** how long fetching the `.sieve` and
  checking its hash took, how long Sieve itself took to decode it, the hand-over to Sieve and back,
  and the total. For a page, it also gives the time for the page's own `.sieve` pictures. On About
  here: "fetched and checked in 0.004 s, decoded by Sieve in 0.000 s (0.001 s with the hand-over),
  0.004 s in all ... and its 1 .sieve picture in 0.018 s" (screenshot beside this file).
- A picture's tooltip gives its times too.
- `sieve web-host` reports how long its checking and decoding took (`seconds` in its reply).
- The waiting text reads "fetching it, checking its hash, and having Sieve decode it...".

If About still takes a moment on your machine, the bar will now show where the time goes: Sieve's
own decoding, or Chrome starting `sieve.exe` for the first request.

**After applying:** rebuild Sieve (the host changed), and press Reload on the Sieve extension in
`chrome://extensions` (it has a new file). `Pages/secure` is rebuilt with the new pages.

**Checked:** the oracle step of the CI (the web host and the site), the host as Chrome talks to it,
and the whole site in Chromium.

To apply: copy `Pages/` and `Sieve/` over the repository's top folder.
