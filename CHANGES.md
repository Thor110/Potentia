# CI on GitHub: macOS, round six (21 October 2026)

macOS got as far as the oracle step this time, the second-to-last. Linux and Windows passed
everything.

- **The cause** (`.github/workflows/build.yml`): a check compared a file's size, from `wc -c`,
  as text. On a Mac, `wc` pads the number with spaces ("    1234"), so it never equals "1234".
  The three checks written that way now compare numbers, which ignores the padding. Sieve itself
  was right; only the check was wrong.
- I also searched the steps that run on macOS for other tools that behave differently there
  (`stat -c`, `sed -i`, `sha256sum`, `timeout` and so on) and found none.
- **Docs:** HANDOFF.

To apply: copy `.github/` and `Sieve/` over the repository's top folder.
