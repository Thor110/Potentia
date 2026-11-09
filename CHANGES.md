# Changes: release extensions (9 November 2026)

Edward: "the release should carry as little as possible realistically ... We could perhaps package
stuff like this separately as another sieve file to be extracted to the Sieve directory ... Create
that as an extend_release.bat script rather than adding it to the make_release.bat script".

- **`tools/extend_release.bat`** (and `tools/extend_release.py`): makes what the release leaves out,
  each as Sieve instructions of its own, into `Sieve\release\` beside the release's files. So far
  one: **`voice.sieve`**, Sieve's own voice, which installs `voice\lj-speech` (the hour of LJ Speech
  as FLAC, CMUdict with its licence, the alignment, the recordings' README and transcripts).
  `make_release` is unchanged, and the release does not carry the voice.
- **Install** it into a Sieve folder (a release's Sieve-<version>, say) with sieve-install, choosing
  the Sieve folder, or `tools\sieve install voice.sieve --to <the Sieve folder>`. The hallway and
  sieve look for `voice\lj-speech` in their own folder and the ones above, so it is found there.
- **Checked as it is made:** installed into a scratch folder and compared byte for byte, and the
  installed voice made to say a sentence. Here: 553 files, `voice.sieve` 89,674,275 bytes, made in
  7.5 minutes (the packer's search) and installed in 15 s, every file byte for byte; it spoke.
- Options: `--only voice`, `--build-dir`, `--out`; a `.sha256` file beside each.
- Documents: SIEVE-INSTALL-USAGE (Making a Sieve release: extensions), MANUAL (Text to Voice).
