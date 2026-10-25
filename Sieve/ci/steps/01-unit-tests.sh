#!/usr/bin/env bash
# Sieve CI, step 01: Unit and conformance tests.
# Run by .github/workflows/build.yml (on Windows, Linux and macOS), or alone or with the
# others by ci/run.sh (ci/README.md), from the Sieve folder with the Release build in build/.
set -eo pipefail
cd "$(dirname "$0")/../.."
. ci/lib.sh

ctest --test-dir build -C Release --output-on-failure
