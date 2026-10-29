#!/usr/bin/env bash
# Sieve CI, step 03: Filters judge, count and rank the same on every platform.
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
# words-v2 with the default list, named (scowl-en-60-names-v2 since 24 October 2026), so a later
# default cannot move these figures unseen; the count, the survivor number and survivor 1,000,000
# are the oracle's too (reference/sieve_ref.py WordsRank).
printf '[text]\nmode = compact\nfilters = words-v2\n[text.words-v2]\ndictionary = scowl-en-60-names-v2\n' > words2.ini
printf '[text]\nmode = compact\nfilters = words-v2, max-run-v1\n[text.words-v2]\ndictionary = scowl-en-60-names-v2\n' > mixed.ini
$exe filters --filters words2.ini | tr -d '\r' | q "^survivors    543295627560114847147360598 "
$exe filters --filters mixed.ini | tr -d '\r' | q "words-v2 can rank, but not together with max-run-v1"
out=$($exe check --length 32 --filters words2.ini "It was the best of times" | tr -d '\r')
echo "$out"
echo "$out" | q "words-v1 *FAIL"
echo "$out" | q "words-v2 *pass"
k=$(echo "$out" | awk '/survivor number/ {print $(NF-2)}')
test "$k" = "283163862792505994991892261"
test "$($exe read --length 32 --mode positional --filters words2.ini --survivor "$k" 2>/dev/null | tr -d '\r' | sed 's/ *$//')" = "it was the best of times"
test "$($exe read --length 32 --mode positional --filters words2.ini --survivor 1000000 2>/dev/null | tr -d '\r' | sed 's/ *$//')" = " a a a a a a a a a a a a sat ya"
# Compact in every ordering: warp gives the compact addresses, read takes them back.
out=$($exe warp --length 32 --filters words2.ini --compact "It was the best of times" 2>/dev/null | tr -d '\r')
echo "$out"
for m in positional scrambled guided; do
  addr=$(echo "$out" | awk -v m=$m '/compact/ {seen = 1; next} seen && $1 == m {print $2; exit}')
  echo "compact $m: $addr"
  test "$($exe read --length 32 --mode $m --filters words2.ini --compact "$addr" 2>/dev/null | tr -d '\r' | sed 's/ *$//')" = "it was the best of times"
done
echo "$out" | q "scrambled   1b2fd28d019b712720f0fd1"
echo "$out" | q "guided      90b25dd0ad7d580  (58 bits"
# The other lines: melodies in a key, black-and-white pictures of low entropy.
printf '[audio]\nmode = compact\nfilters = key-v1\n[audio.key-v1]\ntonic = A\nscale = minor-pentatonic\n[image]\nmode = compact\nfilters = symbol-entropy-v1\n[image.symbol-entropy-v1]\nmax_millibits = 500\n' > lines.ini
$exe filters --line audio --filters lines.ini | tr -d '\r' | q "^survivors    794071845499378503449051136 "
$exe filters --line image --filters lines.ini | tr -d '\r' | q "^survivors    322091425582872 "
printf '[image]\nmode = compact\nfilters = neighbour-agreement-v1\n' > agree.ini
$exe filters --line image --filters agree.ini | tr -d '\r' | q "^survivors    6053792982126620565936264884 "
$exe browse --line image --filters agree.ini --compact --count 2 --seed 4 > /dev/null
# The Encryption Engine, stage 1 (cipher-v1): a recipe there and back, a weak cipher broken, a
# feistel layer left unbroken, and the recipes counted.
enc=$($exe cipher --encrypt --recipe "vigenere lemon; feistel my key" --text "attack at dawn" | tr -d '\r' | awk '$1 == "hex" {print $2}')
$exe cipher --decrypt --recipe "vigenere lemon; feistel my key" --in-hex "$enc" | tr -d '\r' | q '^as text      "attack at dawn"$'
t="It was the best of times, it was the worst of times, it was the age of wisdom, it was the age of foolishness"
weak=$($exe cipher --encrypt --recipe "xor 736563726574" --text "$t" | tr -d '\r' | awk '$1 == "hex" {print $2}')
$exe cipher --break --in-hex "$weak" | tr -d '\r' | q '^guess 1      xor 736563726574 '
strong=$($exe cipher --encrypt --recipe "feistel my key" --text "$t" | tr -d '\r' | awk '$1 == "hex" {print $2}')
$exe cipher --break --in-hex "$strong" | tr -d '\r' | q "^none of the weak ciphers gives English"
$exe cipher --catalogue --depth 2 | tr -d '\r' | q "^layers       1024850422726591137019104700029796025345 (130 bits)$"
# Fitting a picture from a file: canon-image-v2 (the default) keeps a line drawing's ink, which
# v1's plain average (--fit average) turns into plain white paper; a picture already in the
# line's colours, saved at three times the size, comes back as the same unit under both.
v=data/anchors/public-domain/Vitruvian.jpg
$exe warp --line image --file $v | tr -d '\r' | q "^canon        canon-image-v2: 949x1300 source"
$exe warp --line image --fit average --mode positional --file $v | tr -d '\r' | q "^  positional  fffffffffffffffffffffffff$"
vit=$($exe warp --line image --mode positional --file $v | tr -d '\r' | awk '$1 == "positional" {print $2}')
[ "$vit" != "fffffffffffffffffffffffff" ]
$exe read --line image --palette ega16 --mode positional 1234567890123456789 --scale 3 --out ega3.png > /dev/null 2>&1
for f in levels average; do
  back=$($exe warp --line image --palette ega16 --fit $f --mode positional --file ega3.png | tr -d '\r' | awk '$1 == "positional" {print $2}')
  [ "$(echo "$back" | sed 's/^0*//')" = "1234567890123456789" ]
done
$exe browse --line audio --filters lines.ini --compact --count 2 --seed 4 > /dev/null
# The notes2 family: two voices of a larger set, warped, and a stack of melody plugins
# that judges each voice, counting one voice's survivors squared (218112^2).
$exe warp --line audio --note-set notes2 --voices 2 --length 6 "C4q E4q. G4h Bb5s // C3w G2h" | tr -d '\r' | q "C4q E4q. G4h A#5s Rs Rs // C3w G3h Rs Rs Rs Rs"
printf '[audio]\nmode = compact\nfilters = key-data-v2, melody-leap-v1\n[audio.key-data-v2]\ntonic = D\n[audio.melody-leap-v1]\nleap = 2\n' > n2.ini
$exe filters --line audio --note-set notes2 --voices 1 --length 3 --filters n2.ini | tr -d '\r' | q "^survivors    218112 "
$exe filters --line audio --note-set notes2 --voices 2 --length 3 --filters n2.ini | tr -d '\r' | q "^survivors    47572844544 "
$exe browse --line audio --note-set notes2 --voices 2 --length 3 --filters n2.ini --compact --count 2 --seed 4 | tr -d '\r' | q " // "
# The notes3 family: its own melody plugins, stacked, counted, and compact both ways.
N="--line audio --note-set notes3 --low C3 --high C6 --voices 2 --length 6"
printf '[audio]\nmode = compact\nfilters = key-notes3-v1, melody-leap-notes3-v1, melody-loudness-notes3-v1\n[audio.key-notes3-v1]\ntonic = D\n[audio.melody-loudness-notes3-v1]\nstep = 1\n' > n3.ini
$exe filters $N --filters n3.ini | tr -d '\r' | q "^survivors    3895893800931124287377439195136 "
out=$($exe warp $N --filters n3.ini --compact "D4q E4q F#4h A4q // D3w F#3w" 2>/dev/null | tr -d '\r')
echo "$out" | q "survivor number 1703397087741355665989795315712 of"
for m in positional scrambled; do
  a=$(echo "$out" | awk -v m=$m '/compact/ {seen = 1; next} seen && $1 == m {print $2; exit}')
  $exe read $N --filters n3.ini --compact --mode $m "$a" 2>/dev/null | tr -d '\r' | q "^D4:4!6 E4:4!6 F#4:8!6 A4:4!6 R:1 R:1 // D3:16!6 F#3:16!6"
done
$exe warp $N --filters n3.ini --compact "D4q E4q F#4h A4q // D3w A3w" 2>/dev/null | tr -d '\r' | q "not a survivor: fails melody-leap-notes3-v1"
# Tracks and movies: the units joined into one are judged as one (the joined stack), and
# counted exactly when the units have no filters of their own.
V="--width 2 --height 2 --frames 2 --movie-units 2 --title-length 0 --image-width 2 --image-height 2"
printf '[movies]\nmode = compact\n\n[movies.joined]\nfilters = neighbour-agreement-v1\n' > mj.ini
$exe filters --line movies $V --filters mj.ini | tr -d '\r' | q "^survivors    173984 movies"
printf '[movies]\nmode = compact\n\n[movies.units]\nfilters = neighbour-agreement-v1\n\n[movies.joined]\nfilters = neighbour-agreement-v1\n' > mb.ini
$exe filters --line movies $V --filters mb.ini | tr -d '\r' | q "^compact      unavailable: units and joined"
T="--length 4 --track-units 2 --title-length 0 --image-width 1 --image-height 1"
printf '[tracks]\nmode = compact\n\n[tracks.joined]\nfilters = key-v1\n\n[tracks.joined.key-v1]\ntonic = A\nscale = minor-pentatonic\n' > tj.ini
out=$($exe warp --line tracks $T --filters tj.ini --compact "A4q C5q D5q E5q G5q A4q C5q E5q" 2>/dev/null | tr -d '\r')
echo "$out"
echo "$out" | q "survivor number 12641768583633 of 56358560858112"
for m in positional scrambled; do
  a=$(echo "$out" | awk -v m=$m '/compact/ {seen = 1; next} seen && $1 == m {print $2; exit}')
  $exe read --line tracks $T --filters tj.ini --compact --mode $m "$a" 2>/dev/null | tr -d '\r' | tail -1 | q "A4q C5q D5q E5q G5q A4q C5q E5q"
done
$exe warp --line tracks $T --filters tj.ini --compact "A4q C5q D5q E5q G5q A4q C5q F5q" 2>/dev/null | tr -d '\r' | q "not a survivor: fails joined: key-v1"
# Tailoring: the filters and settings that keep one melody and write its number among the
# survivors with the cleanest digits (its shortest route), saved as a settings file, and
# the melody read back from its compact address under them.
M="A4q C5q D5q E5q G5q A4q C5q E5q"
printf '' > none.ini
out=$($exe tailor --line audio --length 8 --filters none.ini --out ta.ini "$M" | tr -d '\r')
echo "$out"
echo "$out" | q "^\[x\] key-data-v2 .* scale=major-pentatonic"
echo "$out" | q "^survivor     46327 of 204994, route b4f7"
out=$($exe warp --line audio --length 8 --filters ta.ini --compact "$M" 2>/dev/null | tr -d '\r')
a=$(echo "$out" | awk '/compact/ {seen = 1; next} seen && $1 == "positional" {print $2; exit}')
test "$a" = "0b4f7"
$exe read --line audio --length 8 --filters ta.ini --compact --mode positional "$a" 2>/dev/null | tr -d '\r' | tail -1 | q "^A4q C5q D5q E5q G5q A4q C5q E5q"
# Weighing a folder: two melodies and a picture that are exactly Sieve's own files (MIDI, and
# a PNG at one pixel a pixel), and a file that is not. Each is weighed against its own
# address; the audio line's filters are tailored to the melodies, their description paid.
mkdir -p weigh
n=0
for m in "$M" "C4q E4q G4q C5q G4q E4q C4h"; do
  n=$((n+1))
  a=$($exe warp --line audio --length 8 "$m" 2>/dev/null | tr -d '\r' | awk '$1 == "positional" {print $2; exit}')
  $exe read --line audio --length 8 --mode positional "$a" --out weigh/tune$n.mid > /dev/null 2>&1
done
$exe read --line image --width 8 --height 8 --palette mono --mode positional 3 --scale 1 --out weigh/pic.png > /dev/null 2>&1
printf 'Not an item of any line.\n' > weigh/note.txt
out=$($exe locate weigh --weigh --tailored --filters none.ini --length 8 --width 8 --height 8 --palette mono 2>&1 | tr -d '\r')
echo "$out"
echo "$out" | q "^  note.txt  *200  *200   its own address"
echo "$out" | q "^  pic.png  *624  *4   image 3"
echo "$out" | q "^  tune1.mid  *864  *36   audio, tailored 1e5230293"
echo "$out" | q "total: own addresses 2488 bits; best ways 272 + shared 258 = 530 bits"
# The search's time: a generous one finds the same; a tiny one stops, and says so (each
# gives the other, the files counted: 0.001 s a file is 0.004 s for the four).
out=$($exe locate weigh --weigh --tailored --filters none.ini --length 8 --width 8 --height 8 --palette mono --search-total 600 2>&1 | tr -d '\r')
echo "$out" | q "^  search: 150.0 s a file, 600.0 s for the run; the weighing's search took "
echo "$out" | q "total: own addresses 2488 bits; best ways 272 + shared 258 = 530 bits"
out=$($exe locate weigh --weigh --tailored --filters none.ini --length 8 --width 8 --height 8 --palette mono --search-per-file 0.001 2>&1 | tr -d '\r')
echo "$out" | q "^  search: 0.001 s a file, 0.004 s for the run; the weighing's search took "
echo "$out" | q "^  tailored audio: none, "
