# Only the latest push is tested; a game as a test bed planned (28 October 2026)

Unzip over the repository root. Committed locally (not pushed). No code changed.

- `.github/workflows/build.yml`: a `concurrency` group, so a new push to a branch (or a pull
  request) cancels its run still in progress, on Windows, Linux and macOS alike. Only the latest
  commit is tested, and macOS jobs no longer queue behind old ones.
- `Sieve/docs/IDEAS.md`: §19.104, a game's files sieved and streamed into a cache as it runs, with
  Edward's *Alien Trilogy* engine as the test bed: why it suits Sieve, what Sieve has towards it,
  what it lacks (a file decoded without the rest), and what to record.
- `Sieve/docs/HANDOFF.md`: the plan's step 7 (the game), and the workflow change noted.

## Files
- `.github/workflows/build.yml`
- `Sieve/docs/HANDOFF.md`
- `Sieve/docs/IDEAS.md`
