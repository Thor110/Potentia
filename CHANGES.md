# CI step 11: the anchor JPEG copied under its own name

Step 11 failed on your machine at its line 820. The JPEG block copied
`data/anchors/public-domain/Vitruvian.jpg` and `web/test-site/src/pictures/*.jpg` into one folder.
The test site has its own `vitruvian.jpg`, which is a different file, and on a folder that ignores
case (Windows, macOS) the two names are one, so `cp` refused. On Linux they are two names, which is
why it passed here.

- `Sieve/ci/steps/11-oracle.sh`: the anchor is copied as `j32/vitruvian-man.jpg`, and the two lines
  that named it (the permanent check and its comment) follow. A comment says why.
- Checked: every other `cp` in the CI steps that gathers files into one folder, and every tracked
  path in the repository, for two names equal but for case. None.
- The JPEG block run alone through `ci/run.sh` here: passed (33 s). The rest of step 11 is
  unchanged and passed in the full run before.

No program code changed.
