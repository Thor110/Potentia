#!/usr/bin/env bash
# Sieve CI, step 02: Same addresses on every platform.
# Run by .github/workflows/build.yml (on Windows, Linux and macOS), or alone or with the
# others by ci/run.sh (ci/README.md), from the Sieve folder with the Release build in build/.
set -eo pipefail
cd "$(dirname "$0")/../.."
. ci/lib.sh

trap 'echo "::error::the step stopped at its line $LINENO: $BASH_COMMAND" >&2' ERR # names the failing line
q() { grep -c "$@" > /dev/null; }
exe=build/sieve
[ -f build/Release/sieve.exe ] && exe=build/Release/sieve.exe
$exe version
got=$($exe warp --length 32 --mode scrambled "It was the best of times" | tr -d '\r' | awk '/scrambled/ {print $2}')
echo "scrambled address: $got"
test "$got" = "007b30165818bf0311497600aa52996f9395e27"
test "$($exe read --length 32 --mode scrambled "$got" | tr -d '\r' | sed 's/ *$//')" = "it was the best of times"
$exe dicts
if $exe dicts | grep -Eq "MISMATCH|MISSING"; then exit 1; fi
$exe sift --lengths 1-8 --brute-max 4 --pruned-max 5 > /dev/null
$exe models
if $exe models | grep -Eq "MISMATCH|MISSING|UNREADABLE"; then exit 1; fi
# Alphabets: built in, stacked from Unicode blocks, and a line that can hold a file.
$exe alphabets > /dev/null
$exe alphabets --spec greek+cyrillic | tr -d '\r' | q "^symbols   400"
# Stacking is a union, in any order: the same alphabet however it is asked for.
test "$($exe alphabets --spec cyrillic+greek | tr -d '\r' | sed -n 1p)" = "$($exe alphabets --spec greek+cyrillic | tr -d '\r' | sed -n 1p)"
$exe alphabets --spec all-emojis+emoticons | tr -d '\r' | q "^symbols   2048"
$exe alphabets --spec ascii+surrogates | tr -d '\r' | q "have no text form"
$exe alphabets --spec ascii96 | tr -d '\r' | q "^digit 0   U+000A"
# An .obj keeps its line breaks through the ascii96 line, from either line ending.
printf 'v 0.5 0.5\nf 1 2 3\n' > obj.txt
printf 'v 0.5 0.5\r\nf 1 2 3\r\n' > objcrlf.txt
a=$($exe warp --alphabet ascii96 --length 32 --mode positional --file obj.txt | tr -d '\r' | awk '$1 == "positional" {print $2}')
b=$($exe warp --alphabet ascii96 --length 32 --mode positional --file objcrlf.txt | tr -d '\r' | awk '$1 == "positional" {print $2}')
test "$a" = "$b"
$exe read --alphabet ascii96 --length 32 --mode positional "$a" | tr -d '\r' | sed -n 2p | q "^f 1 2 3$"
got=$($exe warp --length 32 --mode guided "It was the best of times" | tr -d '\r' | awk '$1 == "guided" {print $2}')
echo "guided address: $got"
test "$got" = "90922c700685e28"
test "$($exe read --length 32 --mode guided "$got" | tr -d '\r' | sed 's/ *$//')" = "it was the best of times"
# Tracks and movies (composition-v1): a melody warped onto tracks, a cover and a title of
# its own (blank), and read back in both orderings.
out=$($exe warp --line tracks --length 4 "C4q E4q G4h C5w E5q D5q C5h G4w" | tr -d '\r')
echo "$out"
echo "$out" | q "positional  000000000000000000000000000000000000000000000000000000000000000076516a4a50a2870d43b3000000"
echo "$out" | q "scrambled   433166bee32c17ef720059c6681dc521b6da066d6111b72bd12451e56d8079d892befd71917b3804ca8f28a0f9"
for m in positional scrambled; do
  a=$(echo "$out" | awk -v m=$m '$1 == m {print $2}')
  $exe read --line tracks --length 4 --mode $m "$a" | tr -d '\r' | tail -1 | q "C4q E4q G4h C5w E5q D5q C5h G4w Re Re"
done
$exe info --line movies --title-length 0 | tr -d '\r' | q "^line         movies  (movies/image/mono/10x10/L100+-+video/mono/5x5x8/L200x4/key=sieve/composition-v1)"
# read --around (REVIEW, the first pass's item 6): a shelf's neighbours, positional ones wrapping
# round the line's ends (the line's last two before its first), and a guided shelf zoomed out.
around=$($exe read --length 32 --mode positional --around 2 0 | tr -d '\r')
printf '%s\n' "$around" | sed -n 1p | q '^      -2  "zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzy"  11d500bfaf40ac5044981798db5fb39f2c17b7f$'
printf '%s\n' "$around" | sed -n 3p | q '^>     +0  "                                "  000000000000000000000000000000000000000$'
printf '%s\n' "$around" | sed -n 5p | q '^      +2  "                               b"  000000000000000000000000000000000000002$'
$exe read --length 32 --mode positional --around 2 12345 | tr -d '\r' | sed -n 5p | q '^      +2  "                            cugt"  000000000000000000000000000000000012347$'
around=$($exe read --length 32 --mode guided --around 2 --zoom 20 90922 | tr -d '\r')
printf '%s\n' "$around" | q '^>     +0  "it was that they are and god tha"  90922  (52 bits)$'
printf '%s\n' "$around" | q '^      +1  "it was the brave uttered through"  90923  (56 bits)$'
if $exe read --length 32 --mode guided --around 2 --zoom 0 90922 > /dev/null 2>&1; then exit 1; fi # (a zoom of 0: refused)
