#!/usr/bin/env bash
# Sieve CI, step 01: Unit and conformance tests.
# Run by .github/workflows/build.yml (on Windows, Linux and macOS), or alone or with the
# others by ci/run.sh (ci/README.md), from the Sieve folder with the Release build in build/.
set -eo pipefail
cd "$(dirname "$0")/../.."
. ci/lib.sh

# The core suite runs as its parts (core-spaces, core-fractals and so on, IDEAS 19.236), so a failure
# names its part; SIEVE_CI_PARTS, a ctest pattern, runs only those (SIEVE_CI_PARTS=core-fractals ci/run.sh 01).
# The parts are separate processes that share no files, so they run side by side, one to a core
# (SIEVE_CI_JOBS sets how many at once, should a machine run short of memory).
jobs="${SIEVE_CI_JOBS:-$(nproc 2> /dev/null || getconf _NPROCESSORS_ONLN 2> /dev/null || echo 2)}"
if [ -n "${SIEVE_CI_PARTS:-}" ]; then
    ctest --test-dir build -C Release --output-on-failure -j "$jobs" -R "$SIEVE_CI_PARTS"
else
    ctest --test-dir build -C Release --output-on-failure -j "$jobs"
fi
