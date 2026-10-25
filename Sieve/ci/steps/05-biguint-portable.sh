#!/usr/bin/env bash
# Sieve CI, step 05: BigUint's arithmetic also builds and passes without a 128-bit integer type.
# Run by .github/workflows/build.yml (on Linux only), or alone or with the
# others by ci/run.sh (ci/README.md), from the Sieve folder with the Release build in build/.
# only: Linux
set -eo pipefail
cd "$(dirname "$0")/../.."
. ci/lib.sh

trap 'echo "::error::the step stopped at its line $LINENO: $BASH_COMMAND" >&2' ERR # names the failing line
# Every limb operation has two implementations: one using the compiler's 128-bit
# integer, and one built from 32-bit halves for compilers that have no such type.
# The second is the one nothing would otherwise exercise, so it is built and run here
# against the same vectors.
cmake -S . -B build-portable -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-DSIEVE_BIGUINT_PORTABLE"
cmake --build build-portable --target sieve_tests --parallel
./build-portable/sieve_tests tests | tail -1 | grep -q ", 0 failures"
