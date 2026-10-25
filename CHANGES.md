# CI step 11 on a Windows Ninja build (25 October 2026)

Unzip over the repository root. Commit 947305a locally (not pushed); it goes on top of sieve-ci-bats.zip.

- **The cause:** the same as step 06's. Step 11 gives `web/test-site/make_site.py` the program as `build/sieve`. The script checks the file exists before starting it, and on your build only `build/sieve.exe` is there.
- **The fix:** `make_site.py` now takes the `.exe` beside the name when that is what exists. The fix is in the script, not the CI, because people run it directly too.
- **The other Python scripts:** `check_host.py` only starts the program, and Windows adds the `.exe` itself, so it needs nothing.
- **Checked here:** given `sieve` where only `sieve.exe` existed, it built the whole test site.
- **Not yet run on Windows:** the rest of step 11 after this line. Run it with `ci\steps\11-oracle.bat`.
