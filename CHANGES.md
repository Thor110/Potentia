# CI step 11: --served-at on Windows (25 October 2026)

Unzip over the repository root. Commit b654226 locally (not pushed), on top of your "sieve ci fixes current".

- **The cause:** when Git for Windows' bash starts a Windows program, it rewrites any argument that looks like a Unix path. `--served-at /Potentia/` reached `sieve.exe` as a folder under Git's own install, so no links from the server's root were made and line 337 found none.
- **Not your machine alone:** GitHub's Windows job failed on the same line, in run 43 ("sieve bzip2 zstd"). The Linux and macOS jobs passed.
- **The fix:** the two `--served-at` commands run with `MSYS_NO_PATHCONV=1`, which keeps the argument as written. On Linux and macOS it does nothing.
- **Checked:** no other step hands a Windows program an argument starting with `/` (step 10 does, but it runs on Linux only). Step 11 passes here (1,015 s).
