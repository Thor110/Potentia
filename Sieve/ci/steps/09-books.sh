#!/usr/bin/env bash
# Sieve CI, step 09: Books bind, unbind and keep their id in every ordering.
# Run by .github/workflows/build.yml (on Windows, Linux and macOS), or alone or with the
# others by ci/run.sh (ci/README.md), from the Sieve folder with the Release build in build/.
set -eo pipefail
cd "$(dirname "$0")/../.."
. ci/lib.sh

trap 'echo "::error::the step stopped at its line $LINENO: $BASH_COMMAND" >&2' ERR # names the failing line
# q PATTERN: like grep -q, but reads all its input, so the command before it never
# gets SIGPIPE (the steps run with pipefail).
q() { grep -c "$@" > /dev/null; }
exe=build/sieve
[ -f build/Release/sieve.exe ] && exe=build/Release/sieve.exe
T=tests/example_book_source.txt
# The committed example reads back to its text, and rebinding its source gives the same record.
$exe unbind tests/example_book_v1.book --pages unbound.txt > /dev/null
cmp unbound.txt tests/example_book_v1.pages.txt
$exe bind --title "A Tale of Two Cities" --cover tests/example_book_cover.png --pages $T --length 400 --mode guided --out again.book
cmp again.book tests/example_book_v1.book
id=$(awk '$1 == "id" {print $2}' tests/example_book_v1.book)
for m in positional scrambled; do
  $exe bind --title "A Tale of Two Cities" --cover tests/example_book_cover.png --pages $T --length 400 --mode $m --key other --out $m.book
  test "$(awk '$1 == "id" {print $2}' $m.book)" = "$id"
  $exe unbind $m.book --pages $m.txt > /dev/null
  cmp $m.txt tests/example_book_v1.pages.txt
done
# A changed address is refused.
python - <<'PY'
l = open("scrambled.book").read().split("\n")
i = l.index("end") - 1
l[i] = l[i][:5] + ("0" if l[i][5] != "0" else "1") + l[i][6:]
open("tampered.book", "w").write("\n".join(l))
PY
if $exe unbind tampered.book > /dev/null 2>&1; then exit 1; fi
# Book filters: the title as one page, the pages as one continuous text, so the word cut
# by each page break is judged whole and the real book survives.
printf '[books]\nmode = compact\n[books.title]\nfilters = title-v1\n[books.title.title-v1]\ndictionary = scowl-en-60-names-v2\n[books.pages]\nfilters = words-v2\n[books.pages.words-v2]\ndictionary = scowl-en-60-names-v2\n' > books.ini
out=$($exe check --book tests/example_book_v1.book --filters books.ini | tr -d '\r')
echo "$out" | tail -4
echo "$out" | q "^book: passes, survivor number 20734810505151566133085913571230753984933224147639"
echo "$out" | q "scrambled  1639f019cdbc3a6bf6cc0c31928397bb8c80e4f3"
$exe filters --line books --length 400 --book-pages 3 --filters books.ini | tr -d '\r' | q "^survivors    2488547448310960803236478086023355485244578445496671176953713542241"
# Tracks and movies keep records too (sieve-book-v2 where an audio section names a note set
# other than notes104): bind makes one from a melody, read --record makes the same one
# from the item's address, and unbind gives the units back joined, as one MIDI file.
T="--note-set notes2 --voices 2 --length 4 --track-units 3"
printf 'C4q E4q G4q C5q D5q E5q F5q G5q // C3w G3w C3w\n' > tune.txt
out=$($exe bind --line tracks $T --file tune.txt --title "a small tune" --out tune.track 2>/dev/null | tr -d '\r')
echo "$out"
head -1 tune.track | q "^sieve-book-v2$"
q "^notes notes2/C3-C6/seEqQhHw/V2$" tune.track
a=$(echo "$out" | awk '$1 == "address" {print $2}')
$exe read --line tracks $T --mode scrambled "$a" --record back.track > /dev/null 2>&1
cmp tune.track back.track
$exe unbind tune.track --units tune.mid 2> /dev/null | tr -d '\r' | q "^a small tune"
head -c 4 tune.mid | q MThd
# A record of notes104 stays sieve-book-v1, readable by every earlier Sieve.
printf 'C4q E4q G4q C5q D5q E5q\n' > tune1.txt
$exe bind --line tracks --length 4 --track-units 2 --file tune1.txt --out plain.track > /dev/null 2>&1
head -1 plain.track | q "^sieve-book-v1$"
