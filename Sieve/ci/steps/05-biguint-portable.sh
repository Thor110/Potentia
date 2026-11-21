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
# Only sieve_tests is built, so not the hallway (whose SDL would otherwise be fetched again), and each
# dependency's source is taken from the main build's fetched copy where there is one (its folder is
# read from build/CMakeCache.txt, so this works under the workflow and under ci/run.sh alike). Only
# the sources are shared: the main build's base folder is not given, as the two trees would then
# build their dependencies into the same folders.
fetched=()
base="$(sed -n 's/^FETCHCONTENT_BASE_DIR:[A-Z]*=//p' build/CMakeCache.txt 2> /dev/null | tr -d '\r' || true)"
if [ -n "$base" ] && [ -d "$base" ]; then
    for src in "$base"/*-src; do
        [ -d "$src" ] || continue
        name="$(basename "$src" -src | tr '[:lower:]' '[:upper:]')"
        [ "$name" = SDL3 ] && continue # (the hallway's alone: given, CMake would warn it went unused)
        fetched+=("-DFETCHCONTENT_SOURCE_DIR_$name=$src")
    done
fi
cmake -S . -B build-portable -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-DSIEVE_BIGUINT_PORTABLE" -DSIEVE_BUILD_CLIENT=OFF "${fetched[@]}"
cmake --build build-portable --target sieve_tests --parallel
./build-portable/sieve_tests tests | tail -1 | grep -q ", 0 failures"
