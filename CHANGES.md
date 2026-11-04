# STATISTICS: 0 bytes on disk (4 November 2026)

Commit 9cb9db1 (local, not pushed), on top of your bdd4063.

- Proof/STATISTICS.md, A small script: the v32 control.py.sieve (591 bytes) now shows 0 bytes on disk on Windows, as you saw. NTFS keeps a file that small inside its Master File Table. The other three still take 4,096 bytes.
