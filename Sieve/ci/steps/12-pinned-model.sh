#!/usr/bin/env bash
# Sieve CI, step 12: The pinned model rebuilds byte for byte from its corpus (Linux).
# Run by .github/workflows/build.yml (on Linux only), or alone or with the
# others by ci/run.sh (ci/README.md), from the Sieve folder with the Release build in build/.
# only: Linux
set -eo pipefail
cd "$(dirname "$0")/../.."
. ci/lib.sh

trap 'echo "::error::the step stopped at its line $LINENO: $BASH_COMMAND" >&2' ERR # names the failing line
python tools/fetch_corpus.py
pinned=$(awk -F'\t' '$1 == "gutenberg-lower27-o5" {print $5}' data/models/models.tsv)
build/sieve train --out rebuilt.model
test "$(sha256sum rebuilt.model | cut -d' ' -f1)" = "$pinned"
# (The oracle runs as a module from the Sieve folder, its corpus and texts named from here, as
# reference/ is where its defaults are reckoned from.)
python -m reference.sieve_ref model-build --corpus data/models/corpus/gutenberg-nltk.tsv --texts corpus/gutenberg --out oracle.model
test "$(sha256sum oracle.model | cut -d' ' -f1)" = "$pinned"
build/sieve measure
