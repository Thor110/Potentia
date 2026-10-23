# `sieve site` on a real site, and PHP and scripts (23 October 2026)

I tried `sieve site build` on a copy of the Potentia website. It showed problems the test site
didn't, and they answer your question about PHP and JavaScript.

## What went wrong on the Potentia website
- **It converted 90 pictures that no page shows.** A built site holds the `.sieve`, not the
  original, so a script or CSS naming one of those pictures would have named a file that was gone.
- **With `--sieve-pages`, it found the pages' loop only after 3.5 minutes** of making pictures,
  and listed four pages without saying which link made the loop.
- **The failed build left a half-made folder**, which then blocked the next build.
- **Pages with scripts became `.sieve` pages.** The viewer runs no scripts, so the charts on
  Metrics, Specs and Release would have been lost.

## How it works now
- **Only what the extension can check is converted.** That means a picture that an `<img>`,
  `<source>` or `<a>` names. A picture no such tag names is copied as it is.
- **Anything named somewhere else keeps its original**, beside its `.sieve`, so the page still
  works (unchecked) with or without the extension. Somewhere else means:
  - CSS (`url(a.png)`);
  - a script (`fetch("a.png")`, `img.src = "a.png"`);
  - PHP's code;
  - `srcset`, `style`, or any other attribute.

  These are found by the file's whole name. A script that puts a name together from parts
  (`"pic" + n + ".png"`) can't be seen, so `--keep-originals` keeps every original.
- **A page with a script stays plain**, and the build says so.
- **PHP:**
  - A `.php` page is read for its links and stays a `.php`.
  - Its plain HTML is rewritten like any page's. Its code (`<?php ... ?>`) is never touched, and
    the result passes `php -l`.
  - What PHP writes when it runs can't be seen beforehand. For pages built entirely by PHP,
    either save the pages the running site serves and build those, or have PHP write each link's
    `integrity` from `site.sieve`. That would be a small PHP helper, not built yet.
- **Loops are found before anything is made.** The error shows one loop, such as
  `a.html -> b.html -> a.html`, and a `--plain` list that would break every loop.
- **A failed build leaves nothing behind.** It also says which file it's making as it goes.
- **Extension fix: `srcset` and `<picture>`.** A browser shows a `srcset` or a `<picture>`'s
  `<source>` instead of the `src`, so with the extension running it showed the unchecked file.
  The extension now removes those before showing the checked `.sieve`.

## Tested here
- **The Potentia website, with `--sieve-pages` and no `--plain`:** 101 files. Its 3 pages without
  scripts were made into `.sieve` files in 2.9 s, its 5 pages with scripts stayed plain, and its
  90 pictures were copied as they are. The check found nothing wrong.
- **A small mixed site:**
  - a picture CSS names, kept beside its `.sieve`;
  - a picture a script names, kept;
  - a picture PHP's code names, kept, with PHP's code untouched;
  - a `srcset` picture, kept;
  - a picture nothing links to, not converted;
  - a page with a script, kept plain.
- **Chromium with the extension, on that site:** all 5 pictures shown from checked files, with no
  `srcset` or `<picture>` sources left.
- **The test site:** the same files as before, `Pages/secure` byte-for-byte unchanged, and the
  same results in Chromium.
- The CI now builds that mixed site and checks each of these. Unit tests pass, the local CI (from
  step 6) passes, and the libc++ build compiles.

## To apply
1. Copy `Sieve/` over the repository's `Sieve/`, and `.github/` over `.github/`.
2. Press **Reload** on the Sieve extension in `chrome://extensions`, since its scripts changed.
3. `Pages/` is unchanged.
