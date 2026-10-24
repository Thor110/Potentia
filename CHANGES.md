# Right-click extract works like an archiver's (24 October 2026)

- **Extract here** and **Extract to a folder of its name** now start at once: no Install button.
  The window shows the progress and closes when it's done, with no Finish button.
- **If it can't extract** (files already there, say), the window stays open and says why. The new
  **Back** button returns to the first screen, where you can tick Replace or choose another folder,
  then Install again.
- Self-extracting installer programs, and opening `sieve-install` directly, work as before.

Tested here: the first extract closed by itself with every file written; a second into the same
folder stayed open. The full CI wasn't rerun, since only the right-click options changed.

To apply: copy `Sieve/` over and rebuild. The right-click entries don't need registering again.
