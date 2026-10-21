# CI on GitHub: macOS, round seven (21 October 2026)

macOS got further again, to one of the last checks in the oracle step. Linux and Windows passed.

- **The cause** (`.github/workflows/build.yml`): the check copies Sieve's own program into a test
  folder and expects Sieve to put it in its "x86 stream", the part that packs x86 machine code
  better. Sieve does that for Windows and Linux programs, but not for Mac programs, which use a
  different format (and GitHub's Macs are ARM, not x86). So on a Mac the check could never pass.
  Sieve was right; the check assumed an x86 Windows or Linux machine.
- **The fix:** on macOS only, the check's program is the same bytes behind a Linux program's
  4-byte header, so the x86 stream is still tested and read back there. Linux and Windows are
  unchanged. I tested such a stand-in here: Sieve puts it in the x86 stream, and both the
  installer and the oracle give the folder back exactly.
- No other check assumes the machine's processor type.
- **Docs:** HANDOFF.

To apply: copy `.github/` and `Sieve/` over the repository's top folder.
