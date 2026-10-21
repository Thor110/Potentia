# CI on GitHub: the Windows and macOS failures (21 October 2026)

On every run since the last fixes, Linux has passed every step. Windows and macOS each fail at
one place, and neither is in Sieve's packing.

- **Windows** (`Sieve/reference/sieve_ref.py`): the oracle builds a test WAV from sine values.
  Python's sine comes from the system's maths library, and Windows' gave one sample a different
  last bit from Linux's. So the WAV it built differed from the committed one, and the oracle step
  stopped there. The oracle now computes that sine exactly, which gives the same bits on every
  system. The committed vectors don't change: Linux's sine was already exact on all forty samples.
- **macOS** (`Sieve/CMakeLists.txt`): the installer failed to link. On a Mac, SDL's graphics
  library needs Apple's system libraries named at link time, and the installer's build named only
  Windows' and Linux's. It now names Apple's too, and drops any the installer doesn't use.
- **Docs:** HANDOFF.

I can't run Windows or macOS here. The Windows oracle step stopped early, so the rest of it hasn't
run there yet, and the macOS change can only be proven by the next run. If either still fails,
tell me and I'll read the logs.

To apply: copy `Sieve/` over the repository's top folder. The workflow file hasn't changed.
