# CI on GitHub: macOS, round five (21 October 2026)

Linux and Windows passed every step. macOS stopped in the filters step with "an instrument is a
General MIDI program, 0 to 127" on a notes3 line.

- **The cause, a bug on every system** (`Sieve/core/src/notes3.cpp`): when one instrument is
  given for several voices, the code filled the list with copies of its own first entry, in a way
  the C++ standard doesn't allow. GNU's library (Linux, and how Windows behaved) happens to cope;
  Apple's library frees the list before reading that entry, so it read garbage (a number far over
  127). I reproduced it here with Apple's library: the first entry, 0, came back as 1,627,873,494.
  It now copies the entry first. Nothing else in Sieve does the same.
- **Checked as a Mac would run it:** I built all of Sieve here with clang and Apple's standard
  library (libc++). The unit tests pass (0 failures), and so does every workflow step.
- **Docs:** HANDOFF, with how to make that build again if macOS ever fails.

To apply: copy `Sieve/` over the repository's top folder. The workflow file hasn't changed.
