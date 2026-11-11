#!/usr/bin/env bash
# Sieve CI, step 11: Reference oracle reproduces the committed vectors.
# Run by .github/workflows/build.yml (on Windows, Linux and macOS), or alone or with the
# others by ci/run.sh (ci/README.md), from the Sieve folder with the Release build in build/.
set -eo pipefail
cd "$(dirname "$0")/../.."
. ci/lib.sh

trap 'echo "::error::the step stopped at its line $LINENO: $BASH_COMMAND" >&2' ERR # names the failing line
cd reference
python sieve_ref.py vectors       | diff - ../tests/vectors_v1.tsv
python sieve_ref.py biguint-vectors | diff - ../tests/vectors_biguint_v1.tsv
python sieve_ref.py biguint-large-vectors | diff - ../tests/vectors_biguint_large_v1.tsv
python sieve_ref.py digit-vectors | diff - ../tests/vectors_digits_v1.tsv
python sieve_ref.py canon-vectors | diff - ../tests/vectors_canon.tsv
python sieve_ref.py bytes-vectors | diff - ../tests/vectors_bytes_v1.tsv
python sieve_ref.py alphabet-vectors | diff - ../tests/vectors_alphabets_v1.tsv
python sieve_ref.py model-vectors | diff - ../tests/vectors_models_v1.tsv
python sieve_ref.py image-vectors | diff - ../tests/vectors_image_v1.tsv
python sieve_ref.py image-v2-vectors | diff - ../tests/vectors_image_v2.tsv
python sieve_ref.py cipher-vectors | diff - ../tests/vectors_cipher_v1.tsv
python sieve_ref.py guided-vectors | diff - ../tests/vectors_guided_v1.tsv
python sieve_ref.py filter-vectors | diff - ../tests/vectors_filters_v1.tsv
python sieve_ref.py compact-vectors | diff - ../tests/vectors_compact_v1.tsv
python sieve_ref.py book-vectors | diff - ../tests/vectors_books_v1.tsv
python sieve_ref.py composition-vectors | diff - ../tests/vectors_compositions_v1.tsv
python sieve_ref.py world-vectors | diff - ../tests/vectors_worlds_v1.tsv
python sieve_ref.py ai-vectors | diff - ../tests/vectors_ai_v1.tsv
python sieve_ref.py ai-filter-vectors | diff - ../tests/vectors_ai_filters_v1.tsv
python sieve_ref.py ai-run-filter-vectors | diff - ../tests/vectors_ai_run_filters_v1.tsv
python sieve_ref.py train-vectors --corpus ../tests/training_corpus.txt | diff - ../tests/vectors_training_v1.tsv
python sieve_ref.py train-vectors --corpus ../tests/training_corpus.txt --v2 | diff - ../tests/vectors_training_v2.tsv
python sieve_ref.py titled-vectors | diff - ../tests/vectors_titled_v1.tsv
python sieve_ref.py binary-vectors | diff - ../tests/vectors_binary_v1.tsv
python sieve_ref.py chunk-vectors | diff - ../tests/vectors_chunks_v1.tsv
python sieve_ref.py notes2-vectors | diff - ../tests/vectors_notes2_v1.tsv
python sieve_ref.py pcm-vectors | diff - ../tests/vectors_pcm_v1.tsv
python sieve_ref.py sound-vectors | diff - ../tests/vectors_sound_v1.tsv
python sieve_ref.py notes3-vectors | diff - ../tests/vectors_notes3_v1.tsv
# One line filtered by another (FILTER-PLUGINS.md section 15): file kinds and the binary
# line's filter by them, and not-written-v1, counted by the oracle's own machines, its
# closed form for the binary reading and the overlap walked pair by pair (about 3 minutes).
python sieve_ref.py kind-vectors | diff - ../tests/vectors_kinds_v1.tsv
python sieve_ref.py written-vectors | diff - ../tests/vectors_written_v1.tsv
# Every line filtered by every other (section 16): not-a-file, the notation and .obj
# machines, the packed closed form and the pages among the binary kinds; and the models
# line's rules with canonical-mesh-v1 (about 45 s).
python sieve_ref.py cross-vectors | diff - ../tests/vectors_cross_v1.tsv
# palette-size-v1 and row-runs-v1 (closed forms, brute force on small lines), and
# utf8-valid-v1 on the binary line (code points by encoded length, Python's decoder).
python sieve_ref.py picture-vectors | diff - ../tests/vectors_picture_v1.tsv
# figure-v1: shapes found by a flood fill on small lines, and at 10 by 10 counted by a
# frontier walk that knows each pixel's row (the engine's automaton does not).
python sieve_ref.py figure-vectors | diff - ../tests/vectors_figure_v1.tsv
# diffusion-v1: training steps that attend everywhere, scored on every clean cell, and a picture made.
python sieve_ref.py diffusion-vectors | diff - ../tests/vectors_diffusion_v1.tsv
python sieve_ref.py utf8-vectors | diff - ../tests/vectors_utf8_v1.tsv
# The generated text plugins are what tools/build_text_plugins.py makes from the pinned lists.
python ../tools/build_text_plugins.py --check
python sieve_ref.py book-filter-vectors | diff - ../tests/vectors_book_filters_v1.tsv
python sieve_ref.py book-read ../tests/example_book_v1.book > book.out
test "$(head -1 book.out)" = "$(awk '$1 == "id" {print $2}' ../tests/example_book_v1.book)"
tail -n +3 book.out | cmp - ../tests/example_book_v1.pages.txt
# The file locator and the manifest creator: the tool's manifest of the fixture folder is
# the oracle's to the byte, and every file's address it writes is binary-v1's.
S=../build/sieve
[ -f ../build/Release/sieve.exe ] && S=../build/Release/sieve.exe
# Filter plugins (docs/FILTER-PLUGINS.md): every reference plugin, at several lengths and
# settings, reported identically by the engine and by the oracle's own parser and engine
# (states declared and minimal, survivors, excluded, and survivors by rank).
for L in 1 2 5 17 40; do
  for spec in "clean-data-v1:" "clean-data-v2:" "max-run-data-v1:max=1" "max-run-data-v1:max=3" "max-run-data-v1:max=5" "key-data-v1:tonic=0" "key-data-v1:tonic=5" "key-data-v1:tonic=11" "tidy-data-v1:" "tidy-data-v1:longest=3" "tidy-data-v2:" "tidy-data-v2:longest=3"; do
    f=${spec%%:*}; p=${spec#*:}
    python sieve_ref.py plugin ../data/filters/$f.sfilter --length $L ${p:+--params $p} > plugin.oracle
    $S filters --plugin ../data/filters/$f.sfilter --length $L ${p:+--params $p} | tr -d '\r' > plugin.engine
    diff plugin.oracle plugin.engine
  done
done
# The token form: the ports of words-v1 and window-v1 (a dictionary each, built another way
# by the oracle: an NFA and the subset construction, then Hopcroft), and a toy grammar
# (word lists, follow, first and last, a word in two sets; whole and cut edges).
for f in words-data-v1 window-data-v1 words-data-v2 window-data-v2 title-data-v1; do
  python sieve_ref.py plugin ../data/filters/$f.sfilter --length 6 --params dictionary=scowl-en-35 > plugin.oracle
  $S filters --plugin ../data/filters/$f.sfilter --length 6 --params dictionary=scowl-en-35 | tr -d '\r' > plugin.engine
  diff plugin.oracle plugin.engine
done
# title-data-v1 with its words held to fewer symbols than the unit has (`within`).
python sieve_ref.py plugin ../data/filters/title-data-v1.sfilter --length 9 --params dictionary=scowl-en-35,max_length=4 > plugin.oracle
$S filters --plugin ../data/filters/title-data-v1.sfilter --length 9 --params dictionary=scowl-en-35,max_length=4 | tr -d '\r' > plugin.engine
diff plugin.oracle plugin.engine
# toy-tags: tagged lists (two files as one, an excluded tag, capitals folded, lines skipped).
# toy-padding: trailing padding and `within` with two separators, cut and whole edges.
for f in toy-grammar-v1 toy-grammar-cut-v1 toy-tags-v1 toy-tags-cut-v1 toy-padding-v1 toy-padding-whole-v1; do
  for L in 1 3 11 25; do
    python sieve_ref.py plugin ../tests/plugins/$f.sfilter --length $L > plugin.oracle
    $S filters --plugin ../tests/plugins/$f.sfilter --length $L | tr -d '\r' > plugin.engine
    diff plugin.oracle plugin.engine
  done
done
# sieve-filter-v2 (comparisons, && || !, min max abs, if/else/fi, choice parameters, the
# line's constants, symbols families): the melody plugins on the audio line at several
# settings, and a toy of the grammar on the text line.
for spec in "key-data-v2:" "key-data-v2:tonic=A,scale=minor" "key-data-v2:tonic=F#,scale=blues" "melody-leap-v1:" "melody-leap-v1:leap=2,rest_resets=yes" "melody-lengths-v1:" "melody-lengths-v1:shortest=q,longest=h,eighths=0" "melody-rests-v1:" "melody-rests-v1:run=1,total=3,leading=yes" "melody-ending-v1:tonic=A,hold=h" "melody-range-v1:low=64,high=72"; do
  f=${spec%%:*}; p=${spec#*:}
  for L in 1 2 5 17; do
    python sieve_ref.py plugin ../data/filters/$f.sfilter --length $L ${p:+--params $p} > plugin.oracle
    $S filters --plugin ../data/filters/$f.sfilter --length $L ${p:+--params $p} | tr -d '\r' > plugin.engine
    diff plugin.oracle plugin.engine
  done
done
for spec in "melody-metre-v1:" "melody-metre-v1:time=3-4,complete=yes" "melody-metre-v1:time=5-4" "melody-ambitus-v1:" "melody-ambitus-v1:span=4" "melody-gapfill-v1:" "melody-gapfill-v1:leap=2,step=5,resolve=yes"; do
  f=${spec%%:*}; p=${spec#*:}
  for L in 1 2 5 17; do
    python sieve_ref.py plugin ../data/filters/$f.sfilter --length $L ${p:+--params $p} > plugin.oracle
    $S filters --plugin ../data/filters/$f.sfilter --length $L ${p:+--params $p} | tr -d '\r' > plugin.engine
    diff plugin.oracle plugin.engine
  done
done
# The generated text plugins (tools/build_text_plugins.py), at several lengths and settings.
for spec in "letter-pairs-v1:" "letter-pairs-v1:min_words=5" "letter-triples-v1:" "letter-triples-v1:min_words=3" "word-cost-v1:" "word-cost-v1:rate=12,slack=8" "sentence-shape-v1:" "sentence-shape-v1:capitals=no,others=yes" "babel-punctuation-v1:"; do
  f=${spec%%:*}; p=${spec#*:}
  for L in 1 2 5 17 40; do
    python sieve_ref.py plugin ../data/filters/$f.sfilter --length $L ${p:+--params $p} > plugin.oracle
    $S filters --plugin ../data/filters/$f.sfilter --length $L ${p:+--params $p} | tr -d '\r' > plugin.engine
    diff plugin.oracle plugin.engine
  done
done
python sieve_ref.py plugin ../data/filters/sentence-shape-v1.sfilter --length 6 --judge ../tests/plugins/sentence-shape-cases.txt > plugin.oracle
$S filters --plugin ../data/filters/sentence-shape-v1.sfilter --length 6 --judge ../tests/plugins/sentence-shape-cases.txt | tr -d '\r' > plugin.engine
diff plugin.oracle plugin.engine
# function-words-v1 reads every word of scowl-en-60-names: the oracle's lazy mode, as for Moby.
python sieve_ref.py plugin ../data/filters/function-words-v1.sfilter --length 6 --lazy --judge ../tests/plugins/moby-sentences.txt | grep -v "^states" > plugin.oracle
$S filters --plugin ../data/filters/function-words-v1.sfilter --length 6 --judge ../tests/plugins/moby-sentences.txt | tr -d '\r' | grep -v "^states" > plugin.engine
diff plugin.oracle plugin.engine
# function-words-v2: the same from scowl-en-60-names-v2 (IDEAS §2.3).
python sieve_ref.py plugin ../data/filters/function-words-v2.sfilter --length 6 --lazy --judge ../tests/plugins/moby-sentences.txt | grep -v "^states" > plugin.oracle
$S filters --plugin ../data/filters/function-words-v2.sfilter --length 6 --judge ../tests/plugins/moby-sentences.txt | tr -d '\r' | grep -v "^states" > plugin.engine
diff plugin.oracle plugin.engine
# Anchor plugins (tools/build_anchor_plugins.py): near one anchor, near several, and fragments.
for spec in "near-example-v1:" "near-example-v1:distance=0" "near-example-v1:distance=7" "near-many-example-v1:" "contains-example-v1:"; do
  f=${spec%%:*}; p=${spec#*:}
  for L in 1 8 20 32; do
    python sieve_ref.py plugin ../tests/plugins/$f.sfilter --length $L ${p:+--params $p} > plugin.oracle
    $S filters --plugin ../tests/plugins/$f.sfilter --length $L ${p:+--params $p} | tr -d '\r' > plugin.engine
    diff plugin.oracle plugin.engine
  done
done
# Within d substitutions of one 32-letter page: sum over i <= d of C(32, i) 26^i.
$S filters --plugin ../tests/plugins/near-example-v1.sfilter --length 32 --params distance=3 | grep -qx "survivors  87513089"
for p in "" "gap=0" "mode=far" "mode=any,gap=5"; do
  for L in 1 2 5 12; do
    python sieve_ref.py plugin ../tests/plugins/toy-v2-v1.sfilter --length $L ${p:+--params $p} > plugin.oracle
    $S filters --plugin ../tests/plugins/toy-v2-v1.sfilter --length $L ${p:+--params $p} | tr -d '\r' > plugin.engine
    diff plugin.oracle plugin.engine
  done
done
# The notes2 family: the note plugins on sets of other ranges and durations, one to three
# voices, each voice judged on its own (counts to the power of the voices, ranks by voice).
for set in "--low C3 --high C6 --durations seEqQhHw" "--low F#3 --high G5 --durations EQH"; do
  for V in 1 2 3; do
    for spec in "key-data-v2:tonic=D,scale=blues" "melody-leap-v1:leap=3" "melody-rests-v1:run=1,total=2,leading=yes" "melody-range-v1:low=62,high=70" "melody-lengths-v2:shortest=e,longest=h,eighths=1" "melody-ending-v2:tonic=G,hold=Q"; do
      f=${spec%%:*}; p=${spec#*:}
      for L in 1 2 5; do
        python sieve_ref.py plugin ../data/filters/$f.sfilter --line audio --note-set notes2 $set --voices $V --length $L --params $p > plugin.oracle
        $S filters --plugin ../data/filters/$f.sfilter --line audio --note-set notes2 $set --voices $V --length $L --params $p | tr -d '\r' > plugin.engine
        diff plugin.oracle plugin.engine
      done
    done
  done
done
# The notes3 family (symbols notes3*, NOTE() and REST()): every notes3 plugin, on two small
# sets of other ranges, ticks and levels, one and two voices.
for set in "--low C4 --high C5 --tpq 2 --longest 4 --levels 2" "--low A3 --high B4 --tpq 3 --longest 5 --levels 3"; do
  for V in 1 2; do
    for spec in "key-notes3-v1:" "key-notes3-v1:tonic=A,scale=minor" "melody-leap-notes3-v1:leap=2,rest_resets=yes" "melody-range-notes3-v1:low=62,high=70" "melody-ambitus-notes3-v1:span=4" "melody-rests-notes3-v1:run=1,total=2,leading=yes" "melody-gapfill-notes3-v1:leap=2,step=3,resolve=yes" "melody-metre-notes3-v1:" "melody-metre-notes3-v1:time=6-8,complete=yes" "melody-ending-notes3-v1:tonic=G,hold=q" "melody-lengths-notes3-v1:shortest=e,longest=h,eighths=1" "melody-loudness-notes3-v1:softest=2,loudest=3,step=1"; do
      f=${spec%%:*}; p=${spec#*:}
      for L in 1 2 4; do
        python sieve_ref.py plugin ../data/filters/$f.sfilter --line audio --note-set notes3 $set --voices $V --length $L ${p:+--params $p} > plugin.oracle
        $S filters --plugin ../data/filters/$f.sfilter --line audio --note-set notes3 $set --voices $V --length $L ${p:+--params $p} | tr -d '\r' > plugin.engine
        diff plugin.oracle plugin.engine
      done
    done
  done
done
# The Moby grammars: too large for the oracle to determinise in full (its NFA keeps each
# token's reading apart, and the subsets run past ten million), so its lazy mode makes the
# subsets only as they are reached: counts, ranks and verdicts on real and shuffled
# sentences must match the engine's; the minimal size is the engine's alone.
for f in moby-grammar-v1 moby-grammar-strict-v1; do
  python sieve_ref.py plugin ../data/filters/$f.sfilter --length 6 --lazy --judge ../tests/plugins/moby-sentences.txt | grep -v "^states" > plugin.oracle
  $S filters --plugin ../data/filters/$f.sfilter --length 6 --judge ../tests/plugins/moby-sentences.txt | tr -d '\r' | grep -v "^states" > plugin.engine
  diff plugin.oracle plugin.engine
done
# Every shipped plugin loads: as many as data/filters holds (not a fixed number, which went
# stale when key-data-v2 was added).
test "$($S filters --plugins | grep -c "^loaded")" = "$(ls ../data/filters/*.sfilter | wc -l | tr -d ' ')"
$S filters --plugin ../data/filters/words-data-v1.sfilter --length 6 --relations | grep -q "stricter than (implies) window-data-v1"
rm -rf addr
$S locate ../tests/manifest_fixture --manifest fixture.manifest --addresses addr --compare > locate.out
python sieve_ref.py manifest ../tests/manifest_fixture | cmp - fixture.manifest
python sieve_ref.py manifest ../tests/manifest_fixture --addresses addr
grep -q "7z (LZMA2" locate.out
# A map is Sieve instructions (sieve-manifest-v18 at the least): its text, the graph part, is the
# oracle's to the byte, read back by both; it is written for other programs as DOT and as
# well-formed GraphML. Nothing writes the older map files (sieve-map-v1, v2) any more.
$S map ../tests/manifest_fixture --out fixture.sieve --dot fixture.dot --graphml fixture.graphml > /dev/null
python sieve_ref.py map ../tests/manifest_fixture > fixture.txt
$S map fixture.sieve | cmp - fixture.txt
python sieve_ref.py graph fixture.sieve | cmp - fixture.txt
grep -q '^digraph "manifest_fixture"' fixture.dot
python -c "import xml.dom.minidom as m; m.parse('fixture.graphml')"
if $S map ../tests/manifest_fixture --out fixture.map > /dev/null 2>&1; then exit 1; fi
# A release map (files found beside it, sealed) and a map holding anchors (their bytes its
# instructions' files) are the oracle's to the byte; the held files are put back by
# installing, and by the oracle; removing an anchor leaves the oracle's map of the rest; a
# sealed map refuses to change.
F=../tests/manifest_fixture
$S map $F/a.txt $F/Zebra.txt --name release --seal --out rel.sieve 2> /dev/null
python sieve_ref.py map $F/a.txt $F/Zebra.txt --name release --seal > rel.txt
$S map rel.sieve | cmp - rel.txt
if $S map rel.sieve --add $F/a.txt --out rel2.sieve 2> /dev/null; then exit 1; fi
$S map --new default --out empty.sieve 2> /dev/null
$S map empty.sieve --add "$F/sub dir/b.bin" --out held.sieve 2> /dev/null
$S map held.sieve --add $F/a.txt --out held.sieve 2> /dev/null
python sieve_ref.py map "$F/sub dir/b.bin" $F/a.txt --held --name default --text > held.txt
$S map held.sieve | cmp - held.txt
python sieve_ref.py graph held.sieve | cmp - held.txt
rm -rf held_in held_oracle
$S install held.sieve --to held_in > /dev/null
python sieve_ref.py unpack held.sieve --to held_oracle > /dev/null
cmp held_in/b.bin "$F/sub dir/b.bin"
cmp held_in/a.txt $F/a.txt
cmp held_oracle/b.bin "$F/sub dir/b.bin"
if $S map held.sieve --add $F/a.txt --out held2.sieve 2> /dev/null; then exit 1; fi
# The rope's search for each held file: 10 seconds unless given; the fixture's files are
# too small for it, so no limit gives the same bytes; a time that is not one is refused.
$S map held.sieve --search-per-file 0 --out held0.sieve 2> /dev/null
cmp held.sieve held0.sieve
if $S map held.sieve --search-per-file soon --out held0.sieve 2> /dev/null; then exit 1; fi
$S map held.sieve --remove 1 --out held1.sieve 2> /dev/null
python sieve_ref.py map $F/a.txt --held --name default --text > held1.txt
$S map held1.sieve | cmp - held1.txt
# Metadata on a node, set and read back as the oracle writes it.
$S map held1.sieve --meta "1:source=the fixture folder" --out meta.sieve 2> /dev/null
$S map meta.sieve --meta "1:found=2026-09-27" --out meta.sieve 2> /dev/null
python sieve_ref.py map $F/a.txt --held --name default --meta "1:source=the fixture folder" --meta "1:found=2026-09-27" --text > meta.txt
$S map meta.sieve | cmp - meta.txt
$S map --new sealed --seal --out sealed.sieve 2> /dev/null
$S map sealed.sieve 2>&1 | grep -c "sealed (sealed)" > /dev/null
if $S map sealed.sieve --add $F/a.txt --out sealed2.sieve 2> /dev/null; then exit 1; fi
# Any instructions read as a map: those without a graph part as the map of their tree.
$S locate $F --installer tree.sieve --v4 > /dev/null
$S map tree.sieve | grep -c "^root manifest_fixture$" > /dev/null
# An older map file (the oracle writes sieve-map-v2, held bytes and all) is still read, and
# becomes Sieve instructions with the same text.
python sieve_ref.py map $F/a.txt --held --name default --meta "1:source=the fixture folder" > old.map
python sieve_ref.py map $F/a.txt --held --name default --meta "1:source=the fixture folder" --text > old.txt
$S map old.map | cmp - old.txt
$S map old.map --out old.sieve 2> /dev/null
$S map old.sieve | cmp - old.txt
rm -rf old_in
$S install old.sieve --to old_in > /dev/null
cmp old_in/a.txt $F/a.txt
# Maps are made as the newest instructions are (27 October 2026): two held texts of 9,000 bytes
# are mixed together in one group (sieve-manifest-v30's groups; v33 at the newest, its match ways weighed), as the report says, and read back by Sieve
# and the oracle. (The same holds for sites, training runs and the hallway's saves: one helper,
# add_packed_newest, makes them all; a site's page is checked below.)
head -c 9000 ../README.md > mt1.md
head -c 9000 ../docs/GAME.md > mt2.md
$S map --new t --out mt.sieve 2> /dev/null
$S map mt.sieve --add mt1.md --out mt.sieve 2> /dev/null
$S map mt.sieve --add mt2.md --out mt.sieve > mt.txt 2>&1
grep -c "^wrote     mt.sieve: Sieve instructions (sieve-manifest-v33), " mt.txt > /dev/null
rm -rf mt_in mt_oracle
$S install mt.sieve --to mt_in > /dev/null
cmp mt_in/mt1.md mt1.md
cmp mt_in/mt2.md mt2.md
python sieve_ref.py unpack mt.sieve --to mt_oracle | grep -c "^mixt " > /dev/null
cmp mt_oracle/mt2.md mt2.md
# A site's page of 9,000 bytes made into a .sieve is mixed (mix-v4, with the word list: at the
# newest, v32's, counted by use), with or without the match model's ways after it (4 November 2026:
# the README's text changed and C2 was weighed in, "mix4 B16 F80 C2").
rm -rf mts mts_out mts_oracle
mkdir mts
printf '<html><body><a href="long.html">long</a></body></html>\n' > mts/index.html
(printf '<html><body><pre>\n'; head -c 9000 ../README.md; printf '</pre></body></html>\n') > mts/long.html
$S site build mts --out mts_out --sieve-pages > /dev/null 2>&1
python sieve_ref.py unpack mts_out/long.sieve --to mts_oracle | grep -c -E "^mix4 B[0-9]+ F80( [A-Z][0-9]+)*	" > /dev/null
cmp mts_oracle/long.html mts/long.html
# The web plugin's host (web/README.md), talked to as Chrome does (native messaging: a
# length, then JSON): a single file's .sieve decoded to the same bytes, its hash stated as
# an integrity attribute; refused with a wrong hash, and a .sieve of many files refused
# with its own; the test site built by sieve site build (its pages after the files they
# state hashes of) and checked by sieve site check; a site changed after it was built is
# caught, and pages naming each other refused; the extension's scripts and manifest well
# formed.
$S locate $F/a.txt --installer web-a.sieve --items > /dev/null
python ../web/host/check_host.py $S web-a.sieve $F/a.txt | grep -c "^decoded: a.txt, text/plain, 34 bytes in 1 part" > /dev/null
python ../web/host/check_host.py $S web-a.sieve --refused | grep -c "not the 0000" > /dev/null
# A .sieve of many files: its listing, and any one of its files by its path (never the disk).
python ../web/host/check_host.py $S tree.sieve --listing | grep -c "^listing: 6 files, 2 folders, 73 bytes" > /dev/null
python ../web/host/check_host.py $S tree.sieve --path "sub dir/deeper/c.txt" "$F/sub dir/deeper/c.txt" | grep -c "^decoded: c.txt" > /dev/null
if python ../web/host/check_host.py $S tree.sieve --path nothing.txt $F/a.txt > nothing.txt 2>&1; then exit 1; fi
grep -c "it holds no file nothing.txt" nothing.txt > /dev/null
# A listing past the 1 MB a message to the browser may be (27 October 2026): 10,000 files, whose
# listing is about 1.18 MB, comes in two parts, and check_host.py refuses any message over 1 MB.
rm -rf web-many && python -c "
import os
os.makedirs('web-many/pages')
for i in range(10000): open('web-many/pages/page-%d.html' % i, 'w', newline='').write('page %d\n' % i)"
$S locate web-many --installer web-many.sieve > /dev/null 2>&1
python ../web/host/check_host.py $S web-many.sieve --listing | grep -c "^listing: 10000 files, 1 folders, 98890 bytes in 2 part(s)" > /dev/null
python ../web/host/check_host.py $S web-many.sieve --path pages/page-9999.html web-many/pages/page-9999.html | grep -c "^decoded: page-9999.html" > /dev/null
python ../web/test-site/make_site.py $S --out web-site | grep -c "^pictures/tampered.sieve" > /dev/null
[ "$(grep -o 'integrity="sha256-' web-site/gallery.html | wc -l)" -eq 8 ] # 7 pictures, and its stylesheet
$S site check web-site 2>&1 | grep -c "0 not as stated, 3 kept as written" > /dev/null
# The site's table (index.sieve), named by every plain page, read by the host as the
# extension reads it; and the crawler (web/crawl_site.py) against the site served here:
# all as kept, then a stylesheet changed on the server, and a table that is not the one kept.
grep -c '<link rel="sieve-table" href="index.sieve">' web-site/index.html > /dev/null
python ../web/host/check_host.py $S web-site/index.sieve --table | grep -c "^table: 14 files" > /dev/null
# Directory Listing Testing: four pictures in folders within folders and a page, one .sieve.
python ../web/host/check_host.py $S web-site/directory.sieve --listing | grep -c "^listing: 5 files, 3 folders, 339459 bytes" > /dev/null
python ../web/host/check_host.py $S web-site/directory.sieve --path hallway/menus/filters.jpg ../web/test-site/src/pictures/filters.jpg | grep -c "^decoded: filters.jpg" > /dev/null
table=$(python -c "import hashlib; print(hashlib.sha256(open('web-site/index.sieve', 'rb').read()).hexdigest())")
python -m http.server 8799 --bind 127.0.0.1 --directory web-site > /dev/null 2>&1 &
server=$!
trap 'kill $server 2>/dev/null || true' EXIT # (stopped however the step ends: a server left behind serves the next run its old site)
python -c "import time, urllib.request
for _ in range(50):
    try:
        urllib.request.urlopen('http://127.0.0.1:8799/index.sieve'); break
    except Exception:
        time.sleep(0.2)
else:
    raise SystemExit('the test server did not answer (port 8799 taken?)')"
python ../web/crawl_site.py http://127.0.0.1:8799/ --table $table --sieve $S | grep -c "table as kept, 14 files fetched; 30 checked, 0 not as stated" > /dev/null
printf x >> web-site/style.css
if python ../web/crawl_site.py http://127.0.0.1:8799/ --table $table --sieve $S > crawl.txt; then exit 1; fi
grep -c "MISMATCH  index.html: the hash it states for style.css is not that file's" crawl.txt > /dev/null
if python ../web/crawl_site.py http://127.0.0.1:8799/ --table 0000000000000000000000000000000000000000000000000000000000000000 --sieve $S > crawl.txt; then exit 1; fi
grep -c "MISMATCH  the site's table" crawl.txt > /dev/null
kill $server
if $S site check web-site > site-check.txt; then exit 1; fi
grep -c "index.sieve: style.css is not the file it names" site-check.txt > /dev/null
grep -c "index.html: the hash it states for style.css is not that file's" site-check.txt > /dev/null
rm -rf cyc
mkdir cyc
echo '<a href="b.html">b</a>' > cyc/a.html
echo '<a href="a.html">a</a>' > cyc/b.html
if $S site build cyc --out cyc-site --sieve-pages 2> cyc.txt; then exit 1; fi
grep -c "these pages link in a loop: a.html -> b.html -> a.html.*--plain a.html" cyc.txt > /dev/null
[ ! -e cyc-site ]
$S site build cyc --out cyc-site --sieve-pages --plain b.html > /dev/null
$S site check cyc-site
# What it cannot follow is kept as it is: a picture CSS names (beside its .sieve), one no
# tag names (not made), a page with a script (plain), and PHP's code (untouched; the HTML
# around it rewritten).
rm -rf mix
mkdir -p mix/p
cp ../web/test-site/src/pictures/door.jpg ../web/test-site/src/pictures/book.jpg ../web/test-site/src/pictures/audio.jpg mix/p/
printf '<link rel="stylesheet" href="s.css"><img src="p/door.jpg"><a href="run.html">run</a><a href="b.html">b</a>\n' > mix/index.html
printf 'body{background:url("p/door.jpg")}\n' > mix/s.css
printf '<img src="p/book.jpg"><script>let n = 1;</script>\n' > mix/run.html
printf '<a href="p/book.jpg">book</a>\n' > mix/b.html
printf '<?php $x = "<img src=\x27p/book.jpg\x27>"; ?>\n<img src="p/book.jpg">\n' > mix/shop.php
$S site build mix --out mix-site --sieve-pages --search-per-file 0.2 2> mix.txt > /dev/null
ls mix-site/p/door.jpg mix-site/p/door.sieve mix-site/p/audio.jpg mix-site/run.html mix-site/b.sieve > /dev/null
[ ! -e mix-site/p/audio.sieve ]
[ ! -e mix-site/run.sieve ]
grep -c "kept plain: run.html (it has a script" mix.txt > /dev/null
grep -c "1 picture no page names" mix.txt > /dev/null
grep -c "<?php \$x = \"<img src='p/book.jpg'>\"; ?>" mix-site/shop.php > /dev/null
grep -c '<img src="p/book.sieve" integrity="sha256-' mix-site/shop.php > /dev/null
$S site check mix-site
# Links from the server's root: left alone unless the site says where it is served from
# (--served-at), then made and checked as relative ones are; another server's never.
rm -rf rooted
mkdir -p rooted/p rooted/sub
cp ../web/test-site/src/pictures/door.jpg rooted/p/
printf '<img src="/Potentia/p/door.jpg"><a href="/Potentia/sub/n.html">n</a><img src="//example.com/x.jpg">\n' > rooted/index.html
printf '<img src="/Potentia/p/door.jpg?v=1">\n' > rooted/sub/n.html
$S site build rooted --out rooted-plain --sieve-pages > /dev/null 2>&1
grep -c '<img src="/Potentia/p/door.jpg">' rooted-plain/index.html > /dev/null
# (MSYS_NO_PATHCONV: Git for Windows' bash turns an argument such as /Potentia/ into a path
# under its own folder when it starts a Windows program; this keeps it as written.)
MSYS_NO_PATHCONV=1 $S site build rooted --out rooted-site --sieve-pages --served-at /Potentia/ --search-per-file 0.2 > /dev/null 2>&1
grep -c '<img src="/Potentia/p/door.sieve" integrity="sha256-' rooted-site/index.html > /dev/null
grep -c '<a href="/Potentia/sub/n.sieve" integrity="sha256-' rooted-site/index.html > /dev/null
grep -c '<img src="//example.com/x.jpg">' rooted-site/index.html > /dev/null
MSYS_NO_PATHCONV=1 $S site check rooted-site --served-at /Potentia/
$S site check rooted-site 2>&1 | grep -c "3 from the server's root not read" > /dev/null
python -c "import json; json.load(open('../web/extension/manifest.json'))"
for js in ../web/extension/*.js; do node --check "$js"; done
# The programs' own help names every option they read (sieve help, hallway --help).
python ../tools/check_help.py
# The packer's threads change nothing: Vitruvian Man cut into pieces (its rope's search
# without a time limit, so the same everywhere), made on one thread and on four, the same.
SIEVE_PACK_THREADS=1 $S locate ../data/anchors/public-domain/Vitruvian.jpg --installer thr1.sieve --search-per-file 0 > thr1.txt
SIEVE_PACK_THREADS=4 $S locate ../data/anchors/public-domain/Vitruvian.jpg --installer thr4.sieve --search-per-file 0 > /dev/null
grep -c "1 file(s) cut into pieces" thr1.txt > /dev/null
cmp thr1.sieve thr4.sieve
# And a folder, whose files are worked on several at once (IDEAS §19.99): the test site's pages
# and pictures, on one thread and on four, the same; and the oracle unpacks it.
SIEVE_PACK_THREADS=1 $S locate ../web/test-site/src --installer thrf1.sieve --search-per-file 0 > /dev/null
SIEVE_PACK_THREADS=4 $S locate ../web/test-site/src --installer thrf4.sieve --search-per-file 0 > /dev/null
cmp thrf1.sieve thrf4.sieve
python sieve_ref.py unpack thrf4.sieve > /dev/null
# A v2 manifest (every file's address listed) is the oracle's to the byte.
$S locate ../tests/manifest_fixture --manifest fixture2.manifest --with-addresses > /dev/null
python sieve_ref.py manifest ../tests/manifest_fixture --with-addresses | cmp - fixture2.manifest
# The older installer, --v3: a v3 manifest (the listing, then every file's bytes; the
# oracle's, to the byte), its address, and the folder put back from that one address,
# identical.
$S locate ../tests/manifest_fixture --manifest fixture3.manifest --installer fixture.sieve --v3 > /dev/null
python sieve_ref.py manifest ../tests/manifest_fixture --with-contents | cmp - fixture3.manifest
# The installer is the manifest's address as raw bytes: exactly the manifest's size here,
# which is the files' bytes and the listing, no more.
test "$(wc -c < fixture.sieve)" -eq "$(wc -c < fixture3.manifest)"
test "$(wc -c < fixture3.manifest)" -eq "$(( $(python sieve_ref.py manifest ../tests/manifest_fixture | wc -c) + $(awk '$1 == "bytes" {print $2}' fixture.manifest) ))"
rm -rf installed
$S install fixture.sieve --to installed > /dev/null
diff -r ../tests/manifest_fixture installed
# A single file's Sieve instructions: as a folder holding just it, the oracle's v3 manifest
# to the byte (seen through its address), installed back as the file under its name.
$S locate ../tests/manifest_fixture/a.txt --installer a.sieve --v3 > /dev/null
python sieve_ref.py manifest ../tests/manifest_fixture/a.txt --with-contents > a3.manifest
python -c "
import sys
o = open('a3.manifest', 'rb').read(); s = open('a.sieve', 'rb').read()
n = int.from_bytes(o, 'big') + int('01' * len(o), 16)
sys.exit(0 if n.to_bytes((n.bit_length() + 7) // 8, 'big') == s else 1)"
rm -rf one_file
$S install a.sieve --to one_file > /dev/null
cmp one_file/a.txt ../tests/manifest_fixture/a.txt
# An installer's manifest installs as it is too (it holds the files); a v1 manifest,
# which only lists them, is refused.
rm -rf installed3
$S install fixture3.manifest --to installed3 > /dev/null
diff -r ../tests/manifest_fixture installed3
if $S install fixture.manifest --to installed_v1 2> /dev/null; then exit 1; fi
# An older installer, of a v2 manifest, still installs.
python -c "
import sys; sys.path.insert(0, '.')
from sieve_ref import binary_index
d = open('fixture2.manifest', 'rb').read(); a = binary_index(d)
open('fixture_v2.sieve', 'wb').write(a.to_bytes((a.bit_length() + 7) // 8, 'big'))"
rm -rf installed2
$S install fixture_v2.sieve --to installed2 > /dev/null
diff -r ../tests/manifest_fixture installed2
test "$($S locate ../tests/manifest_fixture/a.txt | awk '$1 == "sha256" {print $2}')" = "$(python -c "import hashlib;print(hashlib.sha256(open('../tests/manifest_fixture/a.txt','rb').read()).hexdigest())")"
# The installer as made now, v4: the fixture with a program (the x86 stream) and a list
# made from another's lines (a mask), packed; the oracle unpacks it with Python's own
# lzma, nothing of Sieve's, and gives back the folder, as sieve install does.
rm -rf v4fx
cp -r ../tests/manifest_fixture v4fx
cp $S v4fx/program
# A Mac's program is Mach-O, which Sieve does not put in the x86 stream (GitHub's Macs are
# ARM besides); there the program's bytes behind an ELF's first four stand in for one.
if [ "$(uname -s)" = Darwin ]; then { printf '\177ELF'; cat $S; } > v4fx/program; fi
python -c "open('v4fx/all.txt', 'w').write(''.join(f'{i}\n' for i in range(1, 20001)))"
python -c "open('v4fx/third.txt', 'w').write(''.join(f'{i}\n' for i in range(3, 20001, 3)))"
out=$($S locate v4fx --manifest fixture4.manifest --installer fixture4.sieve --v4 | tr -d '\r')
echo "$out" | grep -v "^[df]	"
echo "$out" | grep -c "1 program(s) in the x86 stream" > /dev/null
echo "$out" | grep -c "third.txt: lines of all.txt" > /dev/null
head -1 fixture4.manifest | grep -c "^sieve-manifest-v4" > /dev/null
rm -rf unpacked4 installed4
python sieve_ref.py unpack fixture4.manifest --to unpacked4 > /dev/null
diff -r v4fx unpacked4
$S install fixture4.sieve --to installed4 > /dev/null
diff -r v4fx installed4
# A damaged stream is refused, and nothing is written.
python -c "
d = bytearray(open('fixture4.manifest', 'rb').read()); d[-40] ^= 0xFF
open('broken4.manifest', 'wb').write(bytes(d))"
if $S install broken4.manifest --to broken4 2> /dev/null; then exit 1; fi
test ! -e broken4
# v20 (25 October 2026): lz4, brotli and lzma remade exactly (by the modules the oracle remakes them
# with: liblz4, brotli 1.2.1, liblzma), two gzip members carried as what they hold, and the listing's
# body in bytes; installed, by the oracle and by sieve-install alike.
pip_add "lz4==4.4.5" "brotli==1.2.1"
rm -rf a20 a20_inst a20_oracle a20_si
mkdir -p a20
python -c "
import lz4.frame, brotli, lzma, gzip
t = open('sieve_ref.py', 'rb').read()
open('a20/r.py.lz4', 'wb').write(lz4.frame.compress(t, compression_level=9, block_size=lz4.frame.BLOCKSIZE_MAX4MB))
open('a20/r.py.br', 'wb').write(brotli.compress(t, quality=5, lgwin=19))
open('a20/r.py.lzma', 'wb').write(lzma.compress(t, format=lzma.FORMAT_ALONE, preset=0))
open('a20/two.gz', 'wb').write(gzip.compress(t[:1000]) + gzip.compress(t[1000:]))"
$S locate a20 --installer a20.sieve --items --optimise-archives --compressed-temporary --compressed-permanent --v20 --manifest a20.manifest | tr -d '\r' > a20.txt
grep -c "^remade    3 archive(s) carried as what they hold, remade exactly on installing (sieve-manifest-v20)$" a20.txt > /dev/null
grep -c "^unpacked  1 archive(s) carried as their contents" a20.txt > /dev/null
grep -c "^items     none; sieve-manifest-v20$" a20.txt > /dev/null
python -c "d = open('a20.manifest', 'rb').read(); assert d[:6] == b'sieve\x14', d[:6]"
$S install a20.sieve --to a20_inst > /dev/null
for f in r.py.lz4 r.py.br r.py.lzma; do cmp a20/$f a20_inst/$f; done
cmp sieve_ref.py a20_inst/two
python sieve_ref.py unpack a20.manifest --to a20_oracle > /dev/null
diff -r a20_inst a20_oracle
sieve_ci_no_display # (sieve-install opens SDL; with no screen, its offscreen driver)
SI=../build/sieve-install
[ -f ../build/Release/sieve-install.exe ] && SI=../build/Release/sieve-install.exe
$SI a20.sieve --to a20_si --yes > /dev/null
diff -r a20_inst a20_si
# --v19 makes v19 at the newest (its body as text; here v16, there being no gzip, bzip2 or zstd file
# in it), never v20; v20 read as v19 is refused (its body is not text).
$S locate a20 --installer a20-v19.sieve --items --v19 --manifest a20-v19.manifest > /dev/null
python -c "d = open('a20-v19.manifest', 'rb').read(); assert d[:5] == b'sieve' and 16 <= d[5] <= 19, d[:6]"
python -c "d = bytearray(open('a20.manifest', 'rb').read()); d[5] = 19; open('a20-as-v19.manifest', 'wb').write(d)"
if $S install a20-as-v19.manifest --to a20_bad > /dev/null 2>&1; then exit 1; fi
# libarchive's: a cab (MSZIP, zlib's deflate; and LZX), RAR 4 and 5 and LHA, from libarchive's own tests
# (tests/archives), each carried as its contents (SIEVE_ARCHIVES_ALWAYS: they are too small to be
# worth it otherwise), the files they hold checked by their SHA-256 as bsdtar extracts them.
for pair in "test_read_format_cab_1.cab cab dir1/file1=d0c504f06bbd64d183524eb35e5482ee5d966d456b905a24147165b2904d301b,dir2/file2=60f47caf717b06cf21b3bbb7775e49269a1b5cd6b94bba62da29a2ecb048ccf2,empty=e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855" \
            "test_read_format_cab_2.cab cab zero=8ec8d677b0280efeddd04d0b25d8b80fa50a9825382fa93a17d61c4ca002b1ad,dir1/file1=d0c504f06bbd64d183524eb35e5482ee5d966d456b905a24147165b2904d301b" \
            "test_read_format_rar_noeof.rar rar test.txt=5a5f16e01faf8adf92eb4499a2d3e93010c4b41dbb7f698f4a8466d9f58e6dd2" \
            "test_read_format_rar5_compressed.rar rar test.bin=588870a2dade35c2650fbb7898c9a9c7f21fce7c281198604e8d0c9737f2c375" \
            "test_read_format_lha_header3.lzh lha file1=d0c504f06bbd64d183524eb35e5482ee5d966d456b905a24147165b2904d301b,file2=60f47caf717b06cf21b3bbb7775e49269a1b5cd6b94bba62da29a2ecb048ccf2"; do
  set -- $pair
  rm -rf la_inst la_oracle
  SIEVE_ARCHIVES_ALWAYS=1 $S locate ../tests/archives/$1 --installer la.sieve --items --optimise-archives --compressed-temporary --compressed-permanent | tr -d '\r' > la.txt
  grep -c "^unpacked  1 archive(s) carried as their contents" la.txt > /dev/null
  $S install la.sieve --to la_inst | tr -d '\r' | grep -c "^note      $1 ($2, [0-9]* bytes) was carried as its contents" > /dev/null
  python -c "
import hashlib, sys
for kv in sys.argv[1].split(','):
    p, h = kv.split('=')
    assert hashlib.sha256(open('la_inst/' + p, 'rb').read()).hexdigest() == h, p" "$3"
  python sieve_ref.py unpack la.sieve --to la_oracle > /dev/null
  diff -r la_inst la_oracle
done
# v21 (25 October 2026): a gzip and a zip made again exactly, whoever made them, from a skeleton and
# what they hold (their deflate by preflate): a gzip of zlib's Huffman-only deflate (which no level
# of zlib gives back, so v17's way cannot), a zip of zlib's deflate and one of stored entries; and
# 7-Zip's own LZMA2, LZMA and deflate (tests/archives), by the LZMA SDK's encoder and preflate.
rm -rf a21 a21_inst a21_oracle
mkdir -p a21
python -c "
import zlib, zipfile, struct
t = open('sieve_ref.py', 'rb').read()
c = zlib.compressobj(9, zlib.DEFLATED, -15, 9, zlib.Z_HUFFMAN_ONLY)
d = c.compress(t) + c.flush()
open('a21/huff.gz', 'wb').write(b'\x1f\x8b\x08\x00\x00\x00\x00\x00\x02\xff' + d + struct.pack('<II', zlib.crc32(t), len(t)))
with zipfile.ZipFile('a21/py.zip', 'w', zipfile.ZIP_DEFLATED) as z:
    z.writestr(zipfile.ZipInfo('sieve_ref.py', (2026, 10, 25, 12, 0, 0)), t, compress_type=zipfile.ZIP_DEFLATED)
with zipfile.ZipFile('a21/stored.zip', 'w', zipfile.ZIP_STORED) as z:
    z.writestr(zipfile.ZipInfo('sieve_ref.py', (2026, 10, 25, 12, 0, 0)), t)"
cp ../tests/archives/sieve-7zip-lzma2.7z ../tests/archives/sieve-7zip-lzma.7z ../tests/archives/sieve-7zip-deflate.zip a21/
# (Prioritise Decoding Speed: what the archives hold, 1.4 MB, is otherwise mixed as one group, which
# the oracle took 726 s to decode in Python; groups are checked on their own below, v29 and v30.)
SIEVE_ARCHIVES_ALWAYS=1 $S locate a21 --installer a21.sieve --items --optimise-archives --compressed-temporary --compressed-permanent --prioritise-decoding --manifest a21.manifest | tr -d '\r' > a21.txt
grep -c "^remade    6 archive(s) carried as what they hold, remade exactly on installing (sieve-manifest-v\(2[1-9]\|3[0-3]\))$" a21.txt > /dev/null
python -c "d = open('a21.manifest', 'rb').read(); assert d[:5] == b'sieve' and 21 <= d[5] <= 32, d[:6]"
$S install a21.sieve --to a21_inst > /dev/null
for f in huff.gz py.zip stored.zip sieve-7zip-lzma2.7z sieve-7zip-lzma.7z sieve-7zip-deflate.zip; do cmp a21/$f a21_inst/$f; done
# The oracle reads every skeleton whole and makes the stored zip again; the others, made by preflate
# or the LZMA SDK (which it has not), it names as not made again, and so leaves the tree unchecked.
python sieve_ref.py unpack a21.manifest --to a21_oracle | tr -d '\r' > a21_oracle.txt
cmp a21/stored.zip a21_oracle/stored.zip
[ "$(grep -c "^not made again by the oracle (preflate, the LZMA SDK, stb_image_write, brunsli or packMP3): " a21_oracle.txt)" -eq 5 ]
$SI a21.sieve --to a21_si --yes > /dev/null
diff -r a21_inst a21_si
# v22 (25 October 2026): a RAR made again by the machine's own rar, where it gives the archive back
# byte for byte; else its contents, with a warning. No rar can be had here (it is not free), so a
# stand-in (ci/stand_in_rar.py: a RAR 5 writer of stored files, as RARLAB describes the format)
# plays it; whether the real rar gives a RAR back is for a machine that has it. Not on Windows,
# where a Python script cannot be run as a program.
if [ "$(uname -s)" = Linux ] || [ "$(uname -s)" = Darwin ]; then
  rm -rf r22
  mkdir -p r22/src r22/folder
  cp ../ci/stand_in_rar.py r22/rar
  chmod +x r22/rar
  RAR="$PWD/r22/rar"
  cp -r ../tests/manifest_fixture r22/src/pack
  seq 1 30000 > r22/src/pack/numbers.txt
  (cd r22/src && "$RAR" a -cfg- -idq -y -m0 -qo- -r -- ../made.rar pack)
  # Alone: made again from its contents, then given back as the archive itself.
  SIEVE_ARCHIVES_ALWAYS=1 SIEVE_RAR="$RAR" $S locate r22/made.rar --installer r22.sieve --items --optimise-archives --compressed-temporary --compressed-permanent | tr -d '\r' > r22.txt
  grep -c "^rar       made.rar: made again by RAR 0.01 sieve-stand-in (top, -m0 -qo-)" r22.txt > /dev/null
  test -s r22.sieve
  # (0.13.78) Making it says so, as a warning (the File Locator in a box): only where the installing
  # machine has that rar does it come back byte for byte. Under Optimise Search no RAR is made again
  # (its box is greyed then), so there is nothing to warn of.
  SIEVE_ARCHIVES_ALWAYS=1 SIEVE_RAR="$RAR" $S locate r22/made.rar --installer r22w.sieve --optimise-archives --compressed-temporary --compressed-permanent > /dev/null 2> r22w.txt
  grep -q "^warning: A RAR here is carried as its files and made again on installing by rar: made.rar (RAR 0.01 sieve-stand-in)" r22w.txt
  SIEVE_ARCHIVES_ALWAYS=1 SIEVE_RAR="$RAR" $S locate r22/made.rar --installer r22o.sieve --optimise-archives --compressed-temporary --compressed-permanent --optimise-search > /dev/null 2> r22o.txt
  if grep -q "made again on installing by rar" r22o.txt; then exit 1; fi
  rm -rf r22_inst r22_none r22_oracle r22_si
  SIEVE_RAR="$RAR" $S install r22.sieve --to r22_inst | tr -d '\r' | grep -c "^note      made.rar made again by RAR 0.01 sieve-stand-in, checked against its SHA-256" > /dev/null
  cmp r22/made.rar r22_inst/made.rar
  [ "$(ls -A r22_inst)" = made.rar ]
  # With no rar: its contents, and a warning saying why.
  SIEVE_RAR=/nowhere/rar $S install r22.sieve --to r22_none | tr -d '\r' | grep -c "^warning   made.rar was not made again as a RAR: rar was not found" > /dev/null
  diff -r r22/src r22_none
  python sieve_ref.py unpack r22.sieve --to r22_oracle | tr -d '\r' | grep -c "^rar	made.rar: made again by RAR 0.01 sieve-stand-in where it is found" > /dev/null
  diff -r r22_none r22_oracle
  SIEVE_RAR="$RAR" $SI r22.sieve --to r22_si --yes > /dev/null
  cmp r22/made.rar r22_si/made.rar
  # In a folder: the archive given back in its place, the folder's other files as they were.
  cp r22/made.rar r22/folder/
  cp ../tests/manifest_fixture/a.txt r22/folder/
  SIEVE_ARCHIVES_ALWAYS=1 SIEVE_RAR="$RAR" $S locate r22/folder --installer r22f.sieve --items --optimise-archives --compressed-temporary --compressed-permanent 2>&1 | tr -d '\r' | grep -c "^unpacked .*sieve-manifest-v\(2[2-9]\|3[0-3]\)" > /dev/null
  rm -rf r22f_inst
  SIEVE_RAR="$RAR" $S install r22f.sieve --to r22f_inst > /dev/null
  diff -r r22/folder r22f_inst
  # --v21: as before, its contents alone.
  SIEVE_ARCHIVES_ALWAYS=1 SIEVE_RAR="$RAR" $S locate r22/made.rar --installer r21.sieve --items --optimise-archives --compressed-temporary --compressed-permanent --v21 | tr -d '\r' | grep -c "sieve-manifest-v21\|sieve-manifest-v20" > /dev/null
  rm -rf r21_inst
  $S install r21.sieve --to r21_inst | tr -d '\r' | grep -c "^note      made.rar (rar, [0-9]* bytes) was carried as its contents" > /dev/null
fi
# v23 (26 October 2026): rope-wave-v6, Edward's triangle (P4: a value predicted from the one
# before, the one above and the corner between, the median edge detector) and its contexts (T1:
# what is left coded by how busy the triangle is). On the centre of the Vitruvian Man (tests/rope,
# 256 by 256 grey bytes) the search finds the triangle, the instructions are v23, and Sieve and the
# oracle both give the picture back; --v22 keeps to rope-wave-v5. (--v25: v26's mix-v2 takes the
# picture otherwise.)
cp ../tests/rope/vitruvian-centre-256.gray tri.raw
$S rope tri.raw | tr -d '\r' | grep -c "^wave      L8 W256 S1 P4 K[01]\( T1\)\? (rope-wave-v6)" > /dev/null
$S locate tri.raw --installer tri.sieve --items --v25 | tr -d '\r' | grep -c "^rope .*rope-wave-v1 to v6.*sieve-manifest-v23" > /dev/null
test -s tri.sieve
rm -rf tri_inst tri_oracle
$S install tri.sieve --to tri_inst > /dev/null
cmp tri.raw tri_inst/tri.raw
python sieve_ref.py unpack tri.sieve --to tri_oracle > /dev/null
cmp tri.raw tri_oracle/tri.raw
$S locate tri.raw --installer tri22.sieve --items --v22 | tr -d '\r' > tri22.txt
grep -c "^rope .*rope-wave-v1\( to v[2-5]\)\? (" tri22.txt > /dev/null
if grep -q "sieve-manifest-v23\|not an option" tri22.txt; then exit 1; fi
# The oracle's own rope-wave-v6 against Sieve's codes: every value length, steps, keep or not.
for pr in "L8 W256 S1 P4 K0" "L8 W256 S1 P4 K1 T1" "L8 W256 S2 P3 K0 T1" "L16 W128 S1 P4 K0 T1" "L32 W64 S1 P4 K1 T1"; do
  $S rope tri.raw --params "$pr" --out tri.rope > /dev/null
  python sieve_ref.py rope-unpack tri.rope --params "$pr" --size 65536 --out tri.back > /dev/null
  cmp tri.raw tri.back
done
# v24 (26 October 2026): mixing (mix-v1): every bit predicted by the bytes before it (0 to 6) and
# the word so far, the predictions mixed by weights that learn, coded exactly in integers. The
# start of Don Quixote (40,000 bytes): sieve mix gives it back; Sieve instructions carry it so (v24),
# smaller than v23's; Sieve and the oracle (its own mix-v1) both give it back; --v23 does not mix.
# (--v1 and --v25: mix-v1, which v26's mix-v2 replaces.)
head -c 40000 ../data/anchors/public-domain/pg996.txt > dq.txt
$S mix dq.txt --v1 --out dq.mix | tr -d '\r' > dqmix.txt
grep -c "^mix       B17 (mix-v1)" dqmix.txt > /dev/null
grep -c "read back and checked" dqmix.txt > /dev/null
python sieve_ref.py mix-unpack dq.mix --params B17 --size 40000 --out dq.back > /dev/null
cmp dq.txt dq.back
$S locate dq.txt --installer dq.sieve --items --v25 | tr -d '\r' | grep -c "^mix       1 file(s) carried by mixing, mix-v1 (40000 bytes, carried in [0-9]*; sieve-manifest-v24)" > /dev/null
$S locate dq.txt --installer dq23.sieve --items --v23 | tr -d '\r' > dq23.txt
if grep -q "^mix \|sieve-manifest-v24\|not an option" dq23.txt; then exit 1; fi
[ "$(wc -c < dq.sieve)" -lt "$(wc -c < dq23.sieve)" ]
rm -rf dq_inst dq_oracle
$S install dq.sieve --to dq_inst > /dev/null
cmp dq.txt dq_inst/dq.txt
python sieve_ref.py unpack dq.sieve --to dq_oracle > /dev/null
cmp dq.txt dq_oracle/dq.txt
# v25 (26 October 2026): PNGs made again exactly (Optimise Compressed Formats). A hallway screenshot
# (docs/images: stb_image_write's own, at its default level 8) as its pixels, made again by
# stb_image_write; the extension's icon (zlib's deflate) from its skeleton, its IDATs one run of
# chunks, their deflate by preflate and their CRCs worked out again. Installed exactly; the oracle
# reads both and names them (it has neither stb_image_write nor preflate); --v24 makes neither.
# (Prioritising decoding keeps their pixels under LZMA2: mixed, as they are since v32, the oracle's
# own mixing took about 600 s of the step's 2,860 here, 2 November 2026, and 0.3 s without; mixing
# pictures has blocks of its own below.)
rm -rf png25
mkdir png25
cp ../docs/images/hallway-audio.png ../web/extension/icon128.png png25/
$S locate png25 --installer png25.sieve --items --optimise-archives --compressed-temporary --compressed-permanent --prioritise-decoding 2>&1 | tr -d '\r' | grep -c "^remade    2 archive(s) carried as what they hold, remade exactly on installing (sieve-manifest-v\(2[5-9]\|3[0-3]\))$" > /dev/null
[ "$(wc -c < png25.sieve)" -lt 50000 ]
rm -rf png25_inst png25_oracle
$S install png25.sieve --to png25_inst > /dev/null
diff -r png25 png25_inst
python sieve_ref.py unpack png25.sieve --to png25_oracle | tr -d '\r' | grep -c "^not made again by the oracle (preflate, the LZMA SDK, stb_image_write, brunsli or packMP3): " | grep -q "^2$"
$S locate png25 --installer png24.sieve --items --optimise-archives --compressed-temporary --compressed-permanent --v24 2>&1 | tr -d '\r' > png24.txt
if grep -q "sieve-manifest-v25\|not an option" png24.txt; then exit 1; fi
# v26 (27 October 2026): mix-v2: mix-v1 grown (a match model, the column and field, sparse
# contexts; for a picture, Edward's triangle and its neighbours; two mixers and two SSE stages).
# The same 40,000 bytes of Don Quixote: smaller than mix-v1's code, and the instructions smaller than
# v25's; the top 64 rows of the Vitruvian centre (16,384 grey bytes) carried as a picture, its shape
# guessed from its bytes (I256x1), smaller than v25's instructions. Sieve and the oracle (its own
# mix-v2) give both back, from the codes and from the instructions; --v25 makes no mix-v2. (--v2 and
# --v26: v27's mix-v3 and v28's mix-v4 take them otherwise.)
$S mix dq.txt --v2 --out dq2.mix | tr -d '\r' > dqmix2.txt
grep -c "^mix       B17 (mix-v2)" dqmix2.txt > /dev/null
[ "$(wc -c < dq2.mix)" -lt "$(wc -c < dq.mix)" ]
python sieve_ref.py mix-unpack dq2.mix --v2 --params B17 --size 40000 --out dq2.back > /dev/null
cmp dq.txt dq2.back
$S locate dq.txt --installer dq26.sieve --items --v26 | tr -d '\r' | grep -c "^mix       1 file(s) carried by mixing, mix-v2 (40000 bytes, carried in [0-9]*; sieve-manifest-v26)" > /dev/null
[ "$(wc -c < dq26.sieve)" -lt "$(wc -c < dq.sieve)" ]
rm -rf dq26_inst dq26_oracle
$S install dq26.sieve --to dq26_inst > /dev/null
cmp dq.txt dq26_inst/dq.txt
python sieve_ref.py unpack dq26.sieve --to dq26_oracle > /dev/null
cmp dq.txt dq26_oracle/dq.txt
head -c 16384 tri.raw > top.raw
$S mix top.raw --v2 --picture 256x1 --out top.mix | tr -d '\r' | grep -c "^mix       B16 I256x1 (mix-v2)" > /dev/null
python sieve_ref.py mix-unpack top.mix --v2 --params "B16 I256x1" --size 16384 --out top.back > /dev/null
cmp top.raw top.back
$S locate top.raw --installer top26.sieve --items --v26 | tr -d '\r' > top26.txt
grep -c "^mix       1 file(s) carried by mixing, mix-v2, 1 as picture(s) (16384 bytes, carried in [0-9]*; sieve-manifest-v26)" top26.txt > /dev/null
$S locate top.raw --installer top25.sieve --items --v25 | tr -d '\r' > top25.txt
if grep -q "mix-v2\|sieve-manifest-v26\|not an option" top25.txt; then exit 1; fi
[ "$(wc -c < top26.sieve)" -lt "$(wc -c < top25.sieve)" ]
rm -rf top26_inst top26_oracle
$S install top26.sieve --to top26_inst > /dev/null
cmp top.raw top26_inst/top.raw
python sieve_ref.py unpack top26.sieve --to top26_oracle | tr -d '\r' | grep -c "^mix2 B16 I256x1	16384	top.raw" > /dev/null
cmp top.raw top26_oracle/top.raw
# v27 (27 October 2026): mix-v3, mix-v2 for text with a word list's model (the registered
# scowl-en-60, its SHA-256 checked). The same 40,000 bytes: smaller than mix-v2's code, the
# instructions v27 and smaller than v26's; Sieve, the oracle (its own mix-v3 and trie) and
# sieve-install give it back; --v26 makes no mix-v3. (--v2 and --v27: v28's mix-v4 takes it otherwise.)
$S mix dq.txt --v2 --words 60 --out dq3.mix | tr -d '\r' > dqmix3.txt
grep -c "^mix       B17 D60 (mix-v3)" dqmix3.txt > /dev/null
[ "$(wc -c < dq3.mix)" -lt "$(wc -c < dq2.mix)" ]
python sieve_ref.py mix-unpack dq3.mix --words 60 --params "B17 D60" --size 40000 --out dq3.back > /dev/null
cmp dq.txt dq3.back
$S locate dq.txt --installer dq27.sieve --items --v27 | tr -d '\r' | grep -c "^mix       1 file(s) carried by mixing, mix-v3 (with a word list) (40000 bytes, carried in [0-9]*; sieve-manifest-v27)" > /dev/null
[ "$(wc -c < dq27.sieve)" -lt "$(wc -c < dq26.sieve)" ]
$S locate dq.txt --installer dq26b.sieve --items --v26 | tr -d '\r' > dq26b.txt
if grep -q "mix-v3\|sieve-manifest-v27\|not an option" dq26b.txt; then exit 1; fi
rm -rf dq27_inst dq27_oracle dq27_si
$S install dq27.sieve --to dq27_inst > /dev/null
cmp dq.txt dq27_inst/dq.txt
python sieve_ref.py unpack dq27.sieve --to dq27_oracle > /dev/null
cmp dq.txt dq27_oracle/dq.txt
$SI dq27.sieve --to dq27_si --yes > /dev/null
cmp dq.txt dq27_si/dq.txt
# v28 (27 October 2026): mix-v4, mix-v2 and mix-v3 without their two SSE stages (sse-v1, which
# Edward retired: 9% to 29% of the time for 0.07% to 2.3% of the size). The same 40,000 bytes
# with the word list, and the 64 rows of the Vitruvian centre as a picture: v28, read back by Sieve,
# the oracle (its own mix-v4) and sieve-install, from the codes and from the instructions; --v27
# makes no mix-v4. (A little larger than v27, by design: quicker to decode.) (--v28: v29 mixes the
# text with the stream the files carried as they are share otherwise.)
$S mix dq.txt --words 60 --out dq4.mix | tr -d '\r' | grep -c "^mix       B17 D60 (mix-v4)" > /dev/null
python sieve_ref.py mix-unpack dq4.mix --v4 --params "B17 D60" --size 40000 --out dq4.back > /dev/null
cmp dq.txt dq4.back
$S mix top.raw --picture 256x1 --out top4.mix | tr -d '\r' | grep -c "^mix       B16 I256x1 (mix-v4)" > /dev/null
python sieve_ref.py mix-unpack top4.mix --v4 --params "B16 I256x1" --size 16384 --out top4.back > /dev/null
cmp top.raw top4.back
rm -rf m28
mkdir m28
cp dq.txt top.raw m28/
$S locate m28 --installer m28.sieve --items --v28 2>&1 | tr -d '\r' > m28.txt
grep -c "^mix       2 file(s) carried by mixing, mix-v4, 1 as picture(s), 1 with a word list ([0-9]* bytes, carried in [0-9]*; sieve-manifest-v28)" m28.txt > /dev/null
$S locate m28 --installer m27.sieve --items --v27 2>&1 | tr -d '\r' > m27.txt
if grep -q "mix-v4\|sieve-manifest-v28\|not an option" m27.txt; then exit 1; fi
rm -rf m28_inst m28_oracle m28_si
$S install m28.sieve --to m28_inst > /dev/null
diff -r m28 m28_inst
python sieve_ref.py unpack m28.sieve --to m28_oracle > /dev/null
diff -r m28 m28_oracle
$SI m28.sieve --to m28_si --yes > /dev/null
diff -r m28 m28_si
# v29 (27 October 2026): the files carried as they are, and those that would be mixed one by one
# (not pictures), mixed together as one stream (mix-v4, with the word list for text): each file
# starts from what the ones before taught. Five small files of this repository: v29, smaller than
# v28's, and given back by Sieve, the oracle (its own mix-v4 over the stream) and sieve-install;
# --v28 and Prioritise Decoding Speed make no v29.
rm -rf t29
mkdir t29
cp ../tools/check_help.py ../tools/rar_check.py ../ci/steps/03-filters.sh ../ci/steps/07-sound.sh t29/
head -c 3000 ../README.md > t29/small.md
$S locate t29 --installer t29.sieve --items --v29 2>&1 | tr -d '\r' > t29.txt
grep -c "^stream    the files carried together mixed by mix-v4 (B[0-9]* D60): [0-9]* bytes, LZMA2's [0-9]*; [0-9]* of them mixed together rather than one by one ([0-9]* bytes apart) (sieve-manifest-v29)$" t29.txt > /dev/null
$S locate t29 --installer t28.sieve --items --v28 2>&1 | tr -d '\r' > t28.txt
if grep -q "sieve-manifest-v29\|not an option" t28.txt; then exit 1; fi
[ "$(wc -c < t29.sieve)" -lt "$(wc -c < t28.sieve)" ]
$S locate t29 --installer t29fast.sieve --items --prioritise-decoding 2>&1 | tr -d '\r' > t29fast.txt
if grep -q "sieve-manifest-v29\|^stream    " t29fast.txt; then exit 1; fi
rm -rf t29_inst t29_oracle t29_si
$S install t29.sieve --to t29_inst > /dev/null
diff -r t29 t29_inst
python sieve_ref.py unpack t29.sieve --to t29_oracle > /dev/null
diff -r t29 t29_oracle
$SI t29.sieve --to t29_si --yes > /dev/null
diff -r t29 t29_si

# v30 (27 October 2026): files mixed together in groups of up to 16 MiB, each group its own mixed
# code in the rope stream, the groups decoded at once; files that LZMA2 can't shrink by 5% (already
# compressed) are left out. The same five files: v30, smaller than v28, read back by Sieve, the
# oracle and sieve-install. --v29 and Prioritise Decoding Speed make no v30.
$S locate t29 --installer t30.sieve --items 2>&1 | tr -d '\r' > t30.txt
grep -c "^groups    [0-9]* file(s) mixed together in 1 group(s) by mix-v4 ([0-9]* bytes, carried in [0-9]*; apart [0-9]*; sieve-manifest-v30)$" t30.txt > /dev/null
if grep -q "sieve-manifest-v30" t29.txt; then exit 1; fi
[ "$(wc -c < t30.sieve)" -lt "$(wc -c < t28.sieve)" ]
$S locate t29 --installer t30fast.sieve --items --prioritise-decoding 2>&1 | tr -d '\r' > t30fast.txt
if grep -q "sieve-manifest-v30\|^groups    " t30fast.txt; then exit 1; fi
rm -rf t30_inst t30_oracle t30_si
$S install t30.sieve --to t30_inst > /dev/null
diff -r t29 t30_inst
python sieve_ref.py unpack t30.sieve --to t30_oracle > /dev/null
diff -r t29 t30_oracle
$SI t30.sieve --to t30_si --yes > /dev/null
diff -r t29 t30_si

# v31 (27 October 2026): an archive made again from its skeleton with what it holds carried as its
# own files ("skm", the files "<archive>/<n>" in its order), each carried its own way. A zip of three
# stored files (forced: on files this small unpacking would not pay): with --compressed-temporary,
# v31, the zip given back by Sieve, by the oracle (which makes a stored zip again itself, so the
# tree is checked) and by sieve-install, and nothing else installed; --v30 makes no v31; with
# --compressed-permanent, its files installed in its place.
rm -rf z31
mkdir z31
python -c "import zipfile; z = zipfile.ZipFile('z31/three.zip', 'w', zipfile.ZIP_STORED); [z.write('../' + f, f) for f in ('tools/check_help.py', 'tools/rar_check.py', 'ci/steps/03-filters.sh')]; z.close()"
SIEVE_ARCHIVES_ALWAYS=1 $S locate z31 --installer z31.sieve --items --compressed-temporary 2>&1 | tr -d '\r' > z31.txt
grep -c "^members   1 of them made again from their skeleton and their 3 file(s), each carried its own way (sieve-manifest-v31)$" z31.txt > /dev/null
rm -rf z31_inst z31_oracle z31_si z31p_inst
$S install z31.sieve --to z31_inst > /dev/null
cmp z31/three.zip z31_inst/three.zip
[ "$(ls z31_inst | wc -l)" -eq 1 ]
python sieve_ref.py unpack z31.sieve --to z31_oracle | grep -c "^skm 3 " > /dev/null
cmp z31/three.zip z31_oracle/three.zip
[ "$(ls z31_oracle | wc -l)" -eq 1 ]
$SI z31.sieve --to z31_si --yes > /dev/null
cmp z31/three.zip z31_si/three.zip
SIEVE_ARCHIVES_ALWAYS=1 $S locate z31 --installer z31v30.sieve --items --compressed-temporary --v30 2>&1 | tr -d '\r' > z31v30.txt
if grep -q "sieve-manifest-v31\|^members " z31v30.txt; then exit 1; fi
SIEVE_ARCHIVES_ALWAYS=1 $S locate z31 --installer z31p.sieve --items --compressed-permanent 2>&1 | tr -d '\r' > z31p.txt
if grep -q "^members \|^remade " z31p.txt; then exit 1; fi
$S install z31p.sieve --to z31p_inst > /dev/null
cmp z31p_inst/three/tools/check_help.py ../tools/check_help.py
# A tar (Archive and compressed formats, 1 November 2026): with temporary, made again exactly from
# its skeleton (its headers, padding and end) with its files carried as its members, each its own way
# (v31's skm), and given back byte for byte by Sieve, sieve-install and the oracle (which makes a
# skeleton of bytes and stored data itself); with permanent, installed as its files. Both taken so
# here whether or not smaller (SIEVE_ARCHIVES_ALWAYS): a tar of three texts packs smaller whole, and
# the packer takes a tar apart only where its programs' x86 filter is estimated to pay.
rm -rf tar32
mkdir tar32
python -c "
import tarfile, io
with tarfile.open('tar32/bundle.tar', 'w', format=tarfile.USTAR_FORMAT) as t:
    for name in ('../README.md', '../docs/GAME.md', '../ci/stand_in_rar.py'):
        data = open(name, 'rb').read()
        info = tarfile.TarInfo('bundle/' + name.split('/')[-1]); info.size = len(data); info.mtime = 0
        t.addfile(info, io.BytesIO(data))
"
SIEVE_ARCHIVES_ALWAYS=1 $S locate tar32 --installer tar32t.sieve --items --compressed-temporary 2>&1 | tr -d '\r' > tar32t.txt
grep -q "^members   1 of them made again from their skeleton and their 3 file(s), each carried its own way" tar32t.txt
rm -rf tar32t_inst tar32t_si tar32t_oracle
$S install tar32t.sieve --to tar32t_inst > /dev/null
diff -r tar32 tar32t_inst
$SI tar32t.sieve --to tar32t_si --yes > /dev/null
diff -r tar32 tar32t_si
python sieve_ref.py unpack tar32t.sieve --to tar32t_oracle > /dev/null
diff -r tar32 tar32t_oracle
SIEVE_ARCHIVES_ALWAYS=1 $S locate tar32 --installer tar32p.sieve --items --compressed-permanent 2>&1 | tr -d '\r' | grep -q "^unpacked  1 archive(s) carried as their contents"
rm -rf tar32p_inst
$S install tar32p.sieve --to tar32p_inst > /dev/null
cmp tar32p_inst/bundle.contents/GAME.md ../docs/GAME.md || cmp tar32p_inst/bundle/GAME.md ../docs/GAME.md
# The same tar with its numbers as older GNU tars write them, octal after leading spaces
# ("     120000 ", as Silesia's mozilla has them): read as the same sizes, so still taken apart.
rm -rf tar32o
mkdir tar32o
python -c "
b = bytearray(open('tar32/bundle.tar', 'rb').read()); at = 0
while at + 512 <= len(b) and any(b[at:at + 512]):
    size = int(b[at + 124:at + 135].decode(), 8)
    b[at + 124:at + 136] = ('%11o ' % size).encode()
    b[at + 148:at + 156] = b' ' * 8
    b[at + 148:at + 156] = ('%06o' % sum(b[at:at + 512])).encode() + b'\\0 '
    at += 512 + (size + 511) // 512 * 512
open('tar32o/bundle.tar', 'wb').write(b)
"
SIEVE_ARCHIVES_ALWAYS=1 $S locate tar32o --installer tar32o.sieve --items --compressed-temporary 2>&1 | tr -d '\r' | grep -q "^members   1 of them made again from their skeleton and their 3 file(s), each carried its own way"
rm -rf tar32o_inst tar32o_oracle
$S install tar32o.sieve --to tar32o_inst > /dev/null
diff -r tar32o tar32o_inst
python sieve_ref.py unpack tar32o.sieve --to tar32o_oracle > /dev/null
diff -r tar32o tar32o_oracle
# Files mixed weighed again against the stream they would share (1 November 2026, no new format):
# two texts holding much of the same, too large together for one group (groups made small here by
# SIEVE_MIX_GROUP_BYTES, for testing), each mixed alone, then carried as they are instead, as LZMA2
# in one stream finds what they share; given back by Sieve, sieve-install and the oracle.
rm -rf ws32
mkdir ws32
python -c "
import random
r = random.Random(32)
words = [''.join(r.choice('abcdefghijklmnopqrstuvwxyz') for _ in range(r.randint(2, 9))) for _ in range(3000)]
shared = ' '.join(r.choice(words) for _ in range(10000)).encode()[:60000]
for k in range(2):
    own = ' '.join(r.choice(words) for _ in range(800)).encode()
    open('ws32/part%d.txt' % k, 'wb').write(own + shared)
"
SIEVE_MIX_GROUP_BYTES=80000 $S locate ws32 --installer ws32.sieve --search-per-file 1 2>&1 | tr -d '\r' | grep -q "^weighed   [0-9]* file(s) mixed carried as they are instead"
rm -rf ws32_inst ws32_si ws32_oracle
$S install ws32.sieve --to ws32_inst > /dev/null
diff -r ws32 ws32_inst
$SI ws32.sieve --to ws32_si --yes > /dev/null
diff -r ws32 ws32_si
python sieve_ref.py unpack ws32.sieve --to ws32_oracle > /dev/null
diff -r ws32 ws32_oracle
# v33 (2 November 2026; the draft's first piece, frozen 3 November 2026): mix-v4's match model with its
# ways (R1: a match goes on past one wrong byte; M4: taken after 4 bytes; C2: two places a context
# was seen). Mixing's code with them made by Sieve and read by the oracle; then a slice of the Sieve
# just built (a program, where they pay) made with Try every way: v33 where it is smaller, never
# larger than v32, and given back by Sieve, sieve-install and the oracle; with --v32, v32. (The
# blocks below were the v33 draft's too: each now makes v33 by default and v32 with --v32.)
head -c 24576 ../data/anchors/public-domain/pg996.txt > m33.txt
$S mix m33.txt --params "B16 R1 M4 C2" --out m33.mix | tr -d '\r' | grep -q "read back and checked"
python sieve_ref.py mix-unpack m33.mix --v4 --params "B16 R1 M4 C2" --size 24576 --out m33.back > /dev/null
cmp m33.txt m33.back
if python sieve_ref.py mix-unpack m33.mix --params "B16 R1 M4 C2" --size 24576 --out m33.back > /dev/null 2>&1; then exit 1; fi
rm -rf m33
mkdir m33
head -c 1114112 "$S" | tail -c 65536 > m33/code.bin
$S locate m33 --installer m33-32.sieve --search-per-file 1 --search-every-wave --v32 2>&1 | tr -d '\r' | grep -q "^manifest  sieve-manifest-v32"
$S locate m33 --installer m33-33.sieve --search-per-file 1 --search-every-wave 2>&1 | tr -d '\r' > m33.txt
if grep -q "^manifest  sieve-manifest-v33" m33.txt; then
    python ../tools/sieve_read.py m33-33.sieve | grep -q "mix4 B[0-9]* .*\(R1\|M4\|C2\)"
fi
[ "$(wc -c < m33-33.sieve)" -le "$(wc -c < m33-32.sieve)" ]
rm -rf m33_inst m33_si m33_oracle
$S install m33-33.sieve --to m33_inst > /dev/null
diff -r m33 m33_inst
$SI m33-33.sieve --to m33_si --yes > /dev/null
diff -r m33 m33_si
python sieve_ref.py unpack m33-33.sieve --to m33_oracle > /dev/null
diff -r m33 m33_oracle
# The v33 draft's second piece (2 November 2026): a program mixed through the x86 filter (mix-v4's
# X). A made-up ELF of 48,000 bytes, a third of its instructions calls to 40 places: with the draft,
# mixed whole through the filter (its own mix4 ... X), smaller than v32 makes it (in the x86 stream),
# and given back by Sieve, sieve-install and the oracle (its own filter, the LZMA SDK's); without the
# switch, v32. (9,627 bytes against 13,948 here; the oracle reads it in about 18 s.)
rm -rf x33
mkdir x33
python - <<'PY'
import random
r = random.Random(33)
b = bytearray(b"\x7fELF\x02\x01\x01" + bytes(57))
places = [r.randrange(1 << 20) for _ in range(40)]
while len(b) < 48000:
    k = r.randrange(6)
    if k < 3:
        rel = (0x10000 + places[r.randrange(40)] % 0x8000 - (len(b) + 5)) & 0xFFFFFFFF
        b += bytes([0xE8]) + rel.to_bytes(4, "little")
    elif k == 3:
        b += bytes([0x48, 0x89, 0xC7 + r.randrange(8)])
    elif k == 4:
        b += bytes([0x8B, 0x45, r.randrange(256) & 0xF8])
    else:
        b += bytes([0xC3, 0x90, 0x55])
open("x33/prog", "wb").write(bytes(b))
PY
$S locate x33 --installer x33-32.sieve --items --v32 2>&1 | tr -d '\r' | grep -q "^manifest  sieve-manifest-v32"
$S locate x33 --installer x33-33.sieve --items 2>&1 | tr -d '\r' > x33.txt
grep -q "^manifest  sieve-manifest-v33" x33.txt
[ "$(wc -c < x33-33.sieve)" -lt "$(wc -c < x33-32.sieve)" ]
rm -rf x33_inst x33_si x33_oracle
$S install x33-33.sieve --to x33_inst > /dev/null
diff -r x33 x33_inst
$SI x33-33.sieve --to x33_si --yes > /dev/null
diff -r x33 x33_si
python sieve_ref.py unpack x33-33.sieve --to x33_oracle | grep -q "^mix4 B[0-9]* R1 M4 C2 X	48000	prog$"
diff -r x33 x33_oracle
# The v34 draft's x86 instruction model (E1, 7 November 2026): the same program mixed with contexts
# from where each byte falls in its instruction, smaller than v33 made it; given back by Sieve,
# sieve-install and the oracle (its own X86Reader); without the draft, v33 as before.
$S locate x33 --installer x34.sieve --items --v34-draft 2>&1 | tr -d '\r' > x34.txt
grep -q "^manifest  sieve-manifest-v34" x34.txt
[ "$(wc -c < x34.sieve)" -lt "$(wc -c < x33-33.sieve)" ]
rm -rf x34_inst x34_si x34_oracle
$S install x34.sieve --to x34_inst > /dev/null
diff -r x33 x34_inst
$SI x34.sieve --to x34_si --yes > /dev/null
diff -r x33 x34_si
python sieve_ref.py unpack x34.sieve --to x34_oracle | grep -q "^mix4 B[0-9]* R1 M4 C2 E1 X	48000	prog$"
diff -r x33 x34_oracle
# v30's groups with a file of another way between their files (SPECIFICATIONS v30, as decided 31
# October 2026): two texts mixed as one group, random bytes carried as they are between them, read
# back the same by Sieve and the oracle.
rm -rf g30
mkdir g30
head -c 6000 ../docs/GAME.md > g30/a.txt
python -c "import random; random.seed(30); open('g30/b.bin', 'wb').write(bytes(random.getrandbits(8) for _ in range(3000)))"
head -c 6000 ../docs/SHOWCASE-PLAN.md > g30/c.txt
$S locate g30 --installer g30.sieve 2>&1 | tr -d '\r' > g30.txt
grep -q "^groups    2 file(s) mixed together in 1 group(s)" g30.txt
grep -q "	3000	raw	b.bin$" g30.txt
grep -q "	6000	mixn	c.txt$" g30.txt
rm -rf g30_inst g30_oracle
$S install g30.sieve --to g30_inst > /dev/null
diff -r g30 g30_inst
python sieve_ref.py unpack g30.sieve --to g30_oracle > /dev/null
diff -r g30 g30_oracle
# v32 (30 October 2026, frozen 31 October; --v31 for the version before: docs/NEXT-VERSION.md): a
# file carried as its differences from another of the folder ("delta K L", delta-v1). Two files of
# about 330 KB, the second the first with stretches changed and bytes put in (so all after them is
# shifted; SIEVE_DELTA_FROM takes files this small, which the packer leaves to its groups): at v32,
# one is a delta, given back by Sieve, by the oracle (which reads delta-v1
# itself, from the specification) and by sieve-install; with --v31, nothing is v32.
# (Prioritising decoding keeps the first file under LZMA2: mixed, the oracle's own mixing took
# about 195 s here, 2 November 2026, and 0.3 s without.)
rm -rf d32
mkdir d32
python - <<'PY'
a = b"".join(open(f, "rb").read() for f in ("../docs/MANUAL.md", "../docs/SPECIFICATIONS.md"))[:330000]
b = bytearray(a)
for k in range(8):  # (the same changes every run)
    at = 20000 + k * 37000
    b[at:at + 900] = bytes((k * 31 + j * 7) % 256 for j in range(900))
    b[at + 5000:at + 5000] = b"put in here, %d\n" % k
open("d32/one.txt", "wb").write(a)
open("d32/two.txt", "wb").write(bytes(b))
PY
SIEVE_DELTA_FROM=65536 $S locate d32 --installer d32.sieve --items --prioritise-decoding 2>&1 | tr -d '\r' > d32.txt
grep -c "^deltas    1 file(s) carried as their differences from another" d32.txt > /dev/null
grep -c "^manifest  sieve-manifest-v32" d32.txt > /dev/null
rm -rf d32_inst d32_oracle d32_si
$S install d32.sieve --to d32_inst > /dev/null
diff -r d32 d32_inst
python sieve_ref.py unpack d32.sieve --to d32_oracle | grep -c "^delta [0-9]* [0-9]*	" > /dev/null
diff -r d32 d32_oracle
$SI d32.sieve --to d32_si --yes > /dev/null
diff -r d32 d32_si
$S locate d32 --v31 --installer d32v31.sieve --items 2>&1 | tr -d '\r' > d32v31.txt
if grep -q "sieve-manifest-v32\|^deltas " d32v31.txt; then exit 1; fi
# A repeating byte or pattern (2 November 2026, Edward: "squashed down to length x 00 using a
# repeating character counter"; "the check likely needs to come before anything else"): 4 MiB of
# zeros then 4 MiB of FF bytes, two segments, DEADBEEF four million times, one, and a 16-bit
# counter (a ramp: each value the one before plus 1, wrapping), one. The packer sees them before
# anything else and makes no search (it says so); at v32 LZMA2 takes them (1,351, 2,573 and, for 8
# MiB of the counter, 44,470 bytes here); with the v33 draft, the segments themselves, each its
# count, pattern length and pattern, or for a ramp its width, first value and step (81, 80 and 83),
# given back by Sieve, sieve-install and the oracle.
rm -rf runs && mkdir runs && python -c "
open('runs/two.bin', 'wb').write(bytes(4 << 20) + b'\xff' * (4 << 20))
open('runs/beef.bin', 'wb').write(b'\xde\xad\xbe\xef' * (4 << 20))
open('runs/count16.bin', 'wb').write(b''.join((k & 0xFFFF).to_bytes(2, 'little') for k in range(1 << 20)))
"
$S locate runs --installer runs32.sieve --progress --v32 2>&1 | tr -d '\r' > runs32.txt
grep -q "2 segment(s) of a repeating byte or pattern, LZMA2 about [0-9]*, rope [0-9]* (L8 W1 S1 P1 K0, not searched)" runs32.txt
grep -q "1 segment(s) of a repeating byte or pattern, LZMA2 about" runs32.txt
$S locate runs --installer runs33.sieve 2>&1 | tr -d '\r' | grep -q "^manifest  sieve-manifest-v33"
[ "$(wc -c < runs33.sieve)" -lt 200 ]
[ "$(wc -c < runs33.sieve)" -lt "$(wc -c < runs32.sieve)" ]
rm -rf runs_inst runs_si runs_oracle
$S install runs33.sieve --to runs_inst > /dev/null
diff -r runs runs_inst
$SI runs33.sieve --to runs_si --yes > /dev/null
diff -r runs runs_si
python sieve_ref.py unpack runs33.sieve --to runs_oracle | grep -c "^runs	" | grep -q "^3$"
diff -r runs runs_oracle
# The rope not searched on a file it cannot win (any version, 2 November 2026): a file over 512 KiB
# that is not text has the rope's search made on its first 256 KiB first, and where that comes to
# 1.1 times LZMA2's estimate of the same or more, no whole search. 64 random blocks of 16 KiB
# repeated in a shuffled order (LZMA2 finds the repeats; the rope cannot) is not searched; a smooth
# 16-bit sound, which the rope wins, is. Both given back.
rm -rf probe
mkdir -p probe/lose probe/win
python -c "import random; random.seed(56); b = [bytes(random.getrandbits(8) for _ in range(16384)) for _ in range(16)]; open('probe/lose/blocks.bin', 'wb').write(b''.join(random.choice(b) for _ in range(64)))"
python -c "import math, random; random.seed(57); open('probe/win/sound.raw', 'wb').write(b''.join(int(9000 * math.sin(i / 7.0) + 3000 * math.sin(i / 2.3) + random.gauss(0, 40)).to_bytes(2, 'little', signed=True) for i in range(400000)))"
$S locate probe/lose --installer probe-lose.sieve --progress 2>&1 | tr -d '\r' > probe-lose.txt
grep -q "rope not searched: its first 256 KiB the rope's search makes [0-9.]* times LZMA2's" probe-lose.txt
$S locate probe/win --installer probe-win.sieve --progress 2>&1 | tr -d '\r' > probe-win.txt
if grep -q "not searched" probe-win.txt; then exit 1; fi
rm -rf probe_lose_inst probe_win_inst
$S install probe-lose.sieve --to probe_lose_inst > /dev/null
diff -r probe/lose probe_lose_inst
$S install probe-win.sieve --to probe_win_inst > /dev/null
diff -r probe/win probe_win_inst
# A picture of few colours mixed as its palette (the v33 draft, mix-v4's P): 320 by 240 pixels of 3
# bytes in six colours, each pixel most often its left or upper neighbour's, as raw pixels (the
# rope reads it as 3 wide, so the shape guessed from its bytes is weighed too). With the draft,
# mixed as a palette of six (its indices, one byte a pixel, then the palette) and smaller than v32;
# given back by Sieve, sieve-install and the oracle.
rm -rf pal
mkdir pal
python -c "
import random
random.seed(6)
cols = [(250, 250, 245), (30, 30, 40), (200, 60, 50), (40, 120, 200), (90, 170, 90), (240, 200, 60)]
w, h = 320, 240
px = [[0] * w for _ in range(h)]
for y in range(h):
    for x in range(w):
        r = random.random()
        px[y][x] = px[y][x - 1] if r < 0.7 and x else px[y - 1][x] if r < 0.93 and y else random.randrange(6)
open('pal/picture.rgb', 'wb').write(bytes(c for row in px for i in row for c in cols[i]))
"
$S locate pal --installer pal-33.sieve --progress 2>&1 | tr -d '\r' > pal-33.txt
grep -q "as a palette of 6 colours" pal-33.txt
grep -q "mix4 B[0-9]* I[0-9]*x3 P6" pal-33.txt
$S locate pal --installer pal-32.sieve --v32 2>&1 | tr -d '\r' > pal-32.txt
[ "$(wc -c < pal-33.sieve)" -lt "$(wc -c < pal-32.sieve)" ]
rm -rf pal_inst pal_si pal_oracle
$S install pal-33.sieve --to pal_inst > /dev/null
diff -r pal pal_inst
$SI pal-33.sieve --to pal_si --yes > /dev/null
diff -r pal pal_si
python sieve_ref.py unpack pal-33.sieve --to pal_oracle > /dev/null
cmp pal/picture.rgb pal_oracle/picture.rgb
# Web text primed (v33, reopened 3 November 2026: mix-v4's W1): mixing's model reads MDN's examples
# (data/dictionaries/web-mdn-2026-11-03-64k.bin, its SHA-256 checked) before the file. The test
# site's about page alone (1,117 bytes, mixed though under 4 KiB, as a web text is), and with the
# extension's viewer page beside it, mixed together as a group: both primed, smaller than v32, given
# back by Sieve, sieve-install and the oracle. With --v32, no primer.
rm -rf w33a w33b
mkdir w33a w33b
cp ../web/test-site/src/about.html w33a/
cp ../web/test-site/src/about.html ../web/extension/viewer.html w33b/
for d in w33a w33b; do
    $S locate $d --installer $d.sieve 2>&1 | tr -d '\r' > $d.txt
    $S locate $d --v32 --installer ${d}v32.sieve 2>&1 | tr -d '\r' > ${d}v32.txt
    if grep -q " W1" ${d}v32.txt; then exit 1; fi
    [ "$(wc -c < $d.sieve)" -lt "$(wc -c < ${d}v32.sieve)" ]
    rm -rf ${d}_inst ${d}_si ${d}_oracle
    $S install $d.sieve --to ${d}_inst > /dev/null
    diff -r $d ${d}_inst
    $SI $d.sieve --to ${d}_si --yes > /dev/null
    diff -r $d ${d}_si
    python sieve_ref.py unpack $d.sieve --to ${d}_oracle > /dev/null
    diff -r $d ${d}_oracle
done
grep -q "	mix4 B16 F80 W1	about.html" w33a.txt
# (The group's total, T, is the two pages' bytes, 1,536 with LF line endings: a Windows checkout
# gives them CRLF, so any total is taken; 4 November 2026, Windows CI.)
grep -q -E "	mixt B16 F80 W1 T[0-9]+	about.html" w33b.txt
# Copies (the v33 draft, 2 November 2026, Edward: "a folder with 100 of the same images in"): a text,
# a JPEG and 40,000 random bytes, each with a copy in a folder of its own. With the draft each copy
# is carried as "copy K", decided before anything else (the JPEG not recoded twice), and given back
# by Sieve, sieve-install and the oracle (which cannot make the JPEG, so neither its copy); at v32,
# no copies.
rm -rf cp33 && mkdir -p cp33/sub
head -c 3000 ../docs/GAME.md > cp33/t.txt
cp ../web/test-site/src/pictures/door.jpg cp33/door.jpg
python -c "import random; random.seed(33); open('cp33/noise.bin', 'wb').write(bytes(random.getrandbits(8) for _ in range(40000)))"
cp cp33/t.txt cp33/door.jpg cp33/noise.bin cp33/sub/
$S locate cp33 --installer cp33-33.sieve --compressed-temporary 2>&1 | tr -d '\r' > cp33.txt
grep -q "^copies    3 file(s) carried as copies of another with the same bytes" cp33.txt
grep -q "^manifest  sieve-manifest-v33" cp33.txt
$S locate cp33 --installer cp33-32.sieve --compressed-temporary --v32 2>&1 | tr -d '\r' > cp32.txt
if grep -q "^copies " cp32.txt; then exit 1; fi
rm -rf cp33_inst cp33_si cp33_oracle
$S install cp33-33.sieve --to cp33_inst > /dev/null
diff -r cp33 cp33_inst
$SI cp33-33.sieve --to cp33_si --yes > /dev/null
diff -r cp33 cp33_si
# (The listing's order puts sub/t.txt before t.txt, so t.txt is the copy; the JPEG's copy is not
# listed by the oracle, which leaves both unmade.)
python sieve_ref.py unpack cp33-33.sieve --to cp33_oracle | tr -d '\r' | grep -c "^copy [0-9]*	" | grep -q "^2$"
cmp cp33/t.txt cp33_oracle/t.txt
cmp cp33/noise.bin cp33_oracle/sub/noise.bin
# Binary written as text carried decoded (the v33 draft, spans-v1, IDEAS §19.114): a page holding a
# JPEG as a base64 data: URL; a mail holding a text as base64 in lines of 76 ending CR LF and 300
# random bytes in the URL-safe alphabet unpadded; a C array of a text's bytes ("0x47, 0x41," twelve
# a line, as xxd -i writes); a JSON list of them in decimal; each carried as a skeleton ("spn
# <members> <skeleton size>") and its spans decoded as members (the JPEG reaching brunsli); a text
# with none left as it is. Given back by Sieve, sieve-install and the oracle (which makes all but
# the page: brunsli); at v32, none, and larger.
rm -rf b64
mkdir b64
python -c "import base64; j = base64.b64encode(open('../web/test-site/src/pictures/door.jpg', 'rb').read()).decode(); open('b64/page.html', 'w', newline='').write('<html><body><p>A door.</p><img src=\"data:image/jpeg;base64,' + j + '\"></body></html>\n')"
python -c "import base64, random; random.seed(64); t = base64.b64encode(open('../docs/GAME.md', 'rb').read()[:30000]).decode(); body = '\r\n'.join(t[k:k + 76] for k in range(0, len(t), 76)); u = base64.urlsafe_b64encode(bytes(random.getrandbits(8) for _ in range(300))).decode().rstrip('='); open('b64/mail.txt', 'w', newline='').write('From: someone\r\nContent-Transfer-Encoding: base64\r\n\r\n' + body + '\r\n\r\ntoken \"' + u + '\" ends.\r\n')"
python -c "b = open('../docs/METHODS.md', 'rb').read()[:12000]; rows = [', '.join('0x%02x' % x for x in b[i:i + 12]) for i in range(0, len(b), 12)]; open('b64/methods.h', 'w', newline='').write('unsigned char methods[] = {\n  ' + ',\n  '.join(rows) + '\n};\nunsigned int methods_len = %d;\n' % len(b))"
python -c "import json; open('b64/list.json', 'w', newline='').write(json.dumps({'bytes': list(open('../docs/MANUAL.md', 'rb').read()[:8000])}) + '\n')"
head -c 6000 ../docs/METHODS.md > b64/plain.txt
$S locate b64 --installer b64-33.sieve --compressed-temporary 2>&1 | tr -d '\r' > b64-33.txt
grep -q "^spans     4 text(s) carried with the binary they write out decoded" b64-33.txt
grep -q "^manifest  sieve-manifest-v33" b64-33.txt
$S locate b64 --installer b64-32.sieve --compressed-temporary --v32 2>&1 | tr -d '\r' > b64-32.txt
if grep -q "^spans " b64-32.txt; then exit 1; fi
[ "$(wc -c < b64-33.sieve)" -lt "$(wc -c < b64-32.sieve)" ]
rm -rf b64_inst b64_si b64_oracle
$S install b64-33.sieve --to b64_inst > /dev/null
diff -r b64 b64_inst
$SI b64-33.sieve --to b64_si --yes > /dev/null
diff -r b64 b64_si
python sieve_ref.py unpack b64-33.sieve --to b64_oracle | tr -d '\r' > b64-oracle.txt
grep -c "^spn [0-9]* " b64-oracle.txt | grep -q "^3$"
cmp b64/mail.txt b64_oracle/mail.txt
cmp b64/methods.h b64_oracle/methods.h
cmp b64/list.json b64_oracle/list.json
cmp b64/plain.txt b64_oracle/plain.txt
# The comparison (--compare, as tools/make_release.py asks for it) over the same folder: its texts'
# spans are listed as members after packing, and are not files on the disk to be read (4 November
# 2026: "cannot read .../sieve-install.exe/1" stopped the release); the original counted as the
# folder's five files.
$S locate b64 --installer b64-cmp.sieve --compressed-temporary --compare 2>&1 | tr -d '\r' > b64-cmp.txt
grep -q "^  original  *$(cat b64/* | wc -c) bytes" b64-cmp.txt
# A span that is the same bytes as a file of the folder is weighed as the copy it will be (4 November
# 2026: with Try every way, a page holding a picture in base64 beside the same picture as a file was
# mixed whole, larger than the same folder prioritised for decoding; here, 127,034 bytes before and
# 63,976 after): the page decoded, and the picture carried once.
rm -rf b64dup b64dup_inst
mkdir b64dup
cp b64/page.html ../web/test-site/src/pictures/door.jpg b64dup/
$S locate b64dup --installer b64dup.sieve --search-per-file 0 --search-every-wave 2>&1 | tr -d '\r' > b64dup.txt
grep -q "^spans     1 text(s) carried with the binary they write out decoded" b64dup.txt
grep -q "door.jpg: lines of page.html/1" b64dup.txt
$S install b64dup.sieve --to b64dup_inst > /dev/null
diff -r b64dup b64dup_inst
# A reader given instructions that claim far more than they hold refuses before making it (4
# November 2026 review): an LZMA2 stream declaring 6 GB in 30 KB of junk (its chunks' headers are
# read first, and do not make that size), and 145 bytes of runs declaring an 8 EB file (more than
# twice the memory available; where that cannot be known, the making fails as it begins). Nothing is
# written either way.
python -c "
import hashlib
def tree(fs):
    h = hashlib.sha256()
    for p, b in fs: h.update((p + '\n' + hashlib.sha256(b).hexdigest() + '\n').encode())
    return h.hexdigest()
def v33(stream, listing, streams):
    return ('sieve-manifest-v33\n' + stream + '\ntree ' + tree([('f.bin', b'x')]) + '\n' + listing + '\nend\n').encode() + streams
def vi(n):
    o = b''
    while True:
        b, n = n & 127, n >> 7
        if not n: return o + bytes([b])
        o += bytes([b | 128])
junk = bytes((i * 7919) % 251 for i in range(30000))
# (60 MB declared: past what 30 KB of chunks can make, and under half the memory free on any
# machine that runs this, so the chunks are what refuse it; 6 GB was refused before them where
# less than 12 GB was free, Edward's run of 5 November 2026)
open('forged.sieve', 'wb').write(v33('stream lzma2 30000 60000000 1048576', 'f\t60000000\traw\tf.bin', junk))
code = vi(8 << 60) + vi(1) + b'A'
rope = vi(len(code)) + code
open('claims.sieve', 'wb').write(v33('stream rope %d %d 0' % (len(rope), len(rope)), 'f\t%d\truns\tf.bin' % (8 << 60), rope))
"
rm -rf forged_in claims_in
if $S install forged.sieve --to forged_in > forged.txt 2>&1; then exit 1; fi
grep -c "chunks do not make the size it declares" forged.txt > /dev/null
[ ! -e forged_in/f.bin ]
if $S install claims.sieve --to claims_in > claims.txt 2>&1; then exit 1; fi
[ ! -e claims_in/f.bin ]
# Two names that are one where case is ignored (A.txt and a.txt, made on Linux:
# tests/case_pair.sieve, and a zip of the same made here): where this folder ignores case (Windows,
# macOS), Sieve and sieve-install refuse them before writing anything, naming the pair; where it heeds
# case, both are written. Packing such a folder warns.
rm -rf cs cp_s cp_si cp_z
mkdir cs
echo x > cs/Probe
python -c "import zipfile; z = zipfile.ZipFile('case_pair.zip', 'w'); z.writestr('A.txt', 'upper\n'); z.writestr('a.txt', 'lower\n'); z.close()"
if [ -e cs/probe ]; then
    if $S install ../tests/case_pair.sieve --to cp_s > cp_s.txt 2>&1; then exit 1; fi
    grep -q "A.txt and a.txt are one name here, where case is ignored" cp_s.txt
    if $SI ../tests/case_pair.sieve --to cp_si --yes > cp_si.txt 2>&1; then exit 1; fi
    grep -q "are one name here, where case is ignored" cp_si.txt
    if $SI case_pair.zip --to cp_z --yes > cp_z.txt 2>&1; then exit 1; fi
    grep -q "are one name here, where case is ignored" cp_z.txt
    [ ! -e cp_s/A.txt ]
    [ ! -e cp_si/A.txt ]
    [ ! -e cp_z/A.txt ]
else
    $S install ../tests/case_pair.sieve --to cp_s > /dev/null
    $SI ../tests/case_pair.sieve --to cp_si --yes > /dev/null
    $SI case_pair.zip --to cp_z --yes > /dev/null
    for d in cp_s cp_si cp_z; do [ "$(cat $d/A.txt)" = upper ] && [ "$(cat $d/a.txt)" = lower ]; done
    $S locate cp_s --installer cp_again.sieve 2>&1 | tr -d '\r' | grep -q "^warning: A.txt and a.txt are one name where case is ignored"
    # (6 November 2026, IDEAS §19.120: letters beyond A to Z folded too, as Windows folds them: É and
    # é one name, Σ and σ one; ß and SS two, as Windows has them.)
    rm -rf cp_u cp_ss
    mkdir cp_u cp_ss
    printf one > "cp_u/$(printf '\303\211').txt"
    printf two > "cp_u/$(printf '\303\251').txt"
    $S locate cp_u --installer cp_u.sieve 2>&1 | tr -d '\r' | grep -q "are one name where case is ignored"
    printf one > "cp_ss/stra$(printf '\303\237')e.txt"
    printf two > cp_ss/STRASSE.txt
    said=$($S locate cp_ss --installer cp_ss.sieve 2>&1)
    if grep -q "one name where case is ignored" <<< "$said"; then exit 1; fi
    # (and the Kelvin sign and k two, as Windows has them: it compares uppercase, one character to
    # one, and the Kelvin sign's is itself; the review of 6 November 2026)
    rm -rf cp_k
    mkdir cp_k
    printf one > cp_k/k.txt
    printf two > "cp_k/$(printf '\342\204\252').txt"
    said=$($S locate cp_k --installer cp_k.sieve 2>&1)
    if grep -q "one name where case is ignored" <<< "$said"; then exit 1; fi
    # (and é written as one character and as e and a combining accent: one name on macOS alone,
    # which the packer says; installed here, both)
    rm -rf cp_nf cp_nf_s
    mkdir cp_nf
    printf one > "cp_nf/$(printf '\303\251').txt"
    printf two > "cp_nf/e$(printf '\314\201').txt"
    $S locate cp_nf --installer cp_nf.sieve 2>&1 | tr -d '\r' | grep -q "are one name on macOS, the same letters written in two ways"
    $S install cp_nf.sieve --to cp_nf_s > /dev/null
    [ "$(ls cp_nf_s | wc -l)" -eq 2 ]
fi
# A name Windows cannot hold (aux.txt, the device AUX, made on Linux: tests/windows_names.sieve,
# 0.13.78): on Windows, Sieve and sieve-install refuse it before writing anything, naming it; on
# other systems it is a name like any other, written, and packing it warns.
rm -rf wn_s wn_si
case "$(uname -s)" in
MINGW* | MSYS* | CYGWIN*)
    if $S install ../tests/windows_names.sieve --to wn_s > wn_s.txt 2>&1; then exit 1; fi
    grep -q "aux.txt is a name Windows cannot hold (a device's name (AUX))" wn_s.txt
    if $SI ../tests/windows_names.sieve --to wn_si --yes > wn_si.txt 2>&1; then exit 1; fi
    grep -q "is a name Windows cannot hold" wn_si.txt
    [ ! -e wn_si ] || [ -z "$(ls wn_si)" ]
    ;;
*)
    $S install ../tests/windows_names.sieve --to wn_s > /dev/null
    [ "$(cat wn_s/aux.txt)" = device ]
    $S locate wn_s --installer wn_again.sieve > wn_again.txt 2>&1
    tr -d '\r' < wn_again.txt | grep -q "^warning: aux.txt is a name Windows cannot hold (a device's name (AUX))"
    ;;
esac
# v32, too: a JPEG carried as its brunsli form ("jbr <form size>"), made again byte
# for byte, under Compressed Formats' boxes. The repository's seven JPEGs (all public domain or
# Sieve's own) and a text: with temporary, each JPEG carried so and the folder given
# back by Sieve and by sieve-install; the oracle reads the recipes, names the JPEGs as files it has
# not made (it has no brunsli) and makes the text; with no box, no JPEG recoded (the
# text is v32's still, by its word list: below); with --v31, nothing is v32.
rm -rf j32
mkdir j32
# The anchor is copied under its own name: the test site has a different vitruvian.jpg, and on a
# folder that ignores case (Windows, macOS) the two names are one.
cp ../data/anchors/public-domain/Vitruvian.jpg j32/vitruvian-man.jpg
cp ../web/test-site/src/pictures/*.jpg ../docs/VAULT.md j32/
$S locate j32 --installer j32none.sieve --items 2>&1 | tr -d '\r' > j32none.txt
if grep -q "^jpegs \|	jbr " j32none.txt; then exit 1; fi
$S locate j32 --installer j32.sieve --items --compressed-temporary 2>&1 | tr -d '\r' > j32.txt
grep -c "^jpegs     7 JPEG(s) carried as their brunsli form, made again byte for byte" j32.txt > /dev/null
rm -rf j32_inst j32_oracle j32_si
$S install j32.sieve --to j32_inst > /dev/null
diff -r j32 j32_inst
$SI j32.sieve --to j32_si --yes > /dev/null
diff -r j32 j32_si
python sieve_ref.py unpack j32.sieve --to j32_oracle > j32o.txt
[ "$(grep -c "^not made again by the oracle (.*brunsli or packMP3): .*\.jpg; " j32o.txt)" -eq 7 ]
cmp j32/VAULT.md j32_oracle/VAULT.md
$S locate j32 --v31 --installer j32v31.sieve --items 2>&1 | tr -d '\r' > j32v31.txt
if grep -q "sieve-manifest-v32\|^jpegs " j32v31.txt; then exit 1; fi
# Permanent alone: the same coefficients with tables made for them where that is smaller (of the
# seven, vitruvian-man.jpg: its pixels the same, its file smaller); the rest made again byte for byte.
# Given back the same by Sieve and by sieve-install; the oracle reads the recipes as above.
$S locate j32 --installer j32p.sieve --items --compressed-permanent 2>&1 | tr -d '\r' > j32p.txt
grep -c "^optimised 1 of those JPEG(s) installed with tables made for their values, the same pictures" j32p.txt > /dev/null
rm -rf j32p_inst j32p_si j32p_oracle
$S install j32p.sieve --to j32p_inst > /dev/null
$SI j32p.sieve --to j32p_si --yes > /dev/null
diff -r j32p_inst j32p_si
[ "$(wc -c < j32p_inst/vitruvian-man.jpg)" -lt "$(wc -c < j32/vitruvian-man.jpg)" ]
for f in audio book door filters picture-in-hand vitruvian; do cmp j32/$f.jpg j32p_inst/$f.jpg; done
python sieve_ref.py unpack j32p.sieve --to j32p_oracle > j32po.txt
[ "$(grep -c "^not made again by the oracle (.*brunsli or packMP3): .*\.jpg; " j32po.txt)" -eq 7 ]
# v32, too: with permanent, a PNG installed as the same picture re-saved at its
# smallest ("pngr <filter> <strategy> <split> <head size> <inner size>"), never as its rows alone.
# Four of the repository's small PNGs (the oracle filters each row in Python, slowly): each
# re-saved smaller under its own name, given back the same by Sieve, sieve-install and the oracle
# (which makes them by zlib), each the same pixels as before (its rows read back by zlib alone);
# with --v31, nothing is v32.
rm -rf p32
mkdir p32
cp ../tests/example_book_cover.png ../web/extension/icon16.png ../web/extension/icon32.png ../web/extension/icon128.png p32/
$S locate p32 --installer p32.sieve --items --compressed-permanent 2>&1 | tr -d '\r' > p32.txt
grep -c "^pngs      4 PNG(s) installed re-saved at their smallest, the same pictures" p32.txt > /dev/null
rm -rf p32_inst p32_si p32_oracle
$S install p32.sieve --to p32_inst > /dev/null
$SI p32.sieve --to p32_si --yes > /dev/null
diff -r p32_inst p32_si
python sieve_ref.py unpack p32.sieve --to p32_oracle > /dev/null
diff -r p32_inst p32_oracle
python - p32 p32_inst <<'PY'
import os, struct, sys, zlib
def rows(path):
    b = open(path, "rb").read()
    at, data, ihdr = 8, b"", None
    while at < len(b):
        n = struct.unpack(">I", b[at:at + 4])[0]
        kind = b[at + 4:at + 8]
        if kind == b"IHDR": ihdr = b[at + 8:at + 8 + n]
        if kind == b"IDAT": data += b[at + 8:at + 8 + n]
        at += 12 + n
    w, h, depth, ctype = struct.unpack(">IIBB", ihdr[:10])
    bits = {0: 1, 2: 3, 3: 1, 4: 2, 6: 4}[ctype] * depth
    n, bpp, raw = (w * bits + 7) // 8, max(1, bits // 8), zlib.decompress(data)
    out, prior = [], bytearray(n)
    for y in range(h):
        f, row = raw[y * (n + 1)], bytearray(raw[y * (n + 1) + 1:(y + 1) * (n + 1)])
        for x in range(n):
            a = row[x - bpp] if x >= bpp else 0
            c = prior[x - bpp] if x >= bpp else 0
            p = a + prior[x] - c
            pa, pb, pc = abs(p - a), abs(p - prior[x]), abs(p - c)
            pred = [0, a, prior[x], (a + prior[x]) // 2, a if pa <= pb and pa <= pc else prior[x] if pb <= pc else c][f]
            row[x] = (row[x] + pred) & 255
        out.append(bytes(row))
        prior = row
    return ihdr, out
for name in sorted(os.listdir(sys.argv[1])):
    a, b = os.path.join(sys.argv[1], name), os.path.join(sys.argv[2], name)
    if rows(a) != rows(b) or os.path.getsize(b) >= os.path.getsize(a):
        sys.exit(f"{name}: not the same picture re-saved smaller")
PY
$S locate p32 --v31 --installer p32v31.sieve --items --compressed-permanent 2>&1 | tr -d '\r' > p32v31.txt
if grep -q "sieve-manifest-v32\|^pngs " p32v31.txt; then exit 1; fi
# v32, too: with temporary, a PNG made again exactly with its rows carried unfiltered
# ("skr <skeleton size> <row bytes> <pixel bytes> <rows> <inner size>"; the rows filtered again by
# their filter bytes on installing, then made as "sk" makes them). The extension's four icons (no
# stb PNGs: those are carried as their pixels already): each carried so, given back byte for byte by
# Sieve and by sieve-install; the oracle reads each recipe, filters the rows again and names each as
# a file it has not made (its deflate is preflate's); with --v31, each is "sk" or as it is.
rm -rf r32
mkdir r32
cp ../web/extension/icon16.png ../web/extension/icon32.png ../web/extension/icon48.png ../web/extension/icon128.png r32/
$S locate r32 --installer r32.sieve --items --compressed-temporary 2>&1 | tr -d '\r' > r32.txt
[ "$(grep -c "	skr [0-9]* [0-9]* 4 [0-9]* [0-9]* " r32.txt)" -eq 4 ]
rm -rf r32_inst r32_si r32_oracle
$S install r32.sieve --to r32_inst > /dev/null
diff -r r32 r32_inst
$SI r32.sieve --to r32_si --yes > /dev/null
diff -r r32 r32_si
python sieve_ref.py unpack r32.sieve --to r32_oracle > r32o.txt
[ "$(grep -c "^not made again by the oracle (.*): icon[0-9]*.png; " r32o.txt)" -eq 4 ]
$S locate r32 --v31 --installer r32v31.sieve --items --compressed-temporary 2>&1 | tr -d '\r' > r32v31.txt
if grep -q "sieve-manifest-v32\|	skr " r32v31.txt; then exit 1; fi
# And at v32, mixing weighs a PNG's pixels as a picture of the shape its recipe gives: a
# hallway screenshot (stb's, 1280 by 720, three bytes a pixel), whose pixels the rope reads as three
# bytes wide, mixed as 1280 by 3 (v26's picture parameters, read by every reader since); without
# v32, carried as it was. (Not given to the oracle: its mixing would take hours on 2.7 MB.)
rm -rf s32
mkdir s32
cp ../docs/images/hallway-compact.png s32/
# (From v33, its palette with the match model's ways: 9,674 bytes, against v32's 12,761.)
$S locate s32 --installer s32.sieve --items --compressed-temporary 2>&1 | tr -d '\r' | grep -q "	stb 8 -1 1280 720 3 2764800 mix4 B22 I1280x3 P4 R1 M4 C2	hallway-compact.png"
$S locate s32 --v32 --installer s32v32.sieve --items --compressed-temporary 2>&1 | tr -d '\r' | grep -q "	stb 8 -1 1280 720 3 2764800 mix4 B22 I1280x3	hallway-compact.png"
rm -rf s32_inst
$S install s32.sieve --to s32_inst > /dev/null
diff -r s32 s32_inst
# Without v32, not the shape its recipe gives: since the rope's probe (2 November 2026), whose
# search is not run on these pixels (its start 1.74 times LZMA2's), mixed at a shape guessed from
# its bytes, 3840 by 1 (v26's parameters), where it was carried as it was; installed the same.
$S locate s32 --v31 --installer s32v31.sieve --items --compressed-temporary 2>&1 | tr -d '\r' | grep -q "	stb 8 -1 1280 720 3 2764800 mix4 B22 I3840x1	hallway-compact.png"
rm -rf s32v31_inst
$S install s32v31.sieve --to s32v31_inst > /dev/null
diff -r s32 s32v31_inst
# v32, too: its head read by its version byte (SPECIFICATIONS v32), made for every .sieve in bytes
# at the newest: each stream's dictionary left out where its size gives it, a stream
# of one LZMA2 chunk carried as the chunk's data alone, the last part's size left out, a recipe's
# numbers as numbers. A small file: v32, 9 bytes smaller than v20's, given back the same by Sieve,
# sieve-install and the oracle.
rm -rf h32
mkdir h32
cp ../tools/fetch_corpus.py h32/
$S locate h32 --installer h32.sieve 2>&1 | tr -d '\r' | grep -q "^manifest  sieve-manifest-v32, "
$S locate h32 --v31 --installer h32v20.sieve 2>&1 | tr -d '\r' | grep -q "^manifest  sieve-manifest-v20, "
[ "$(( $(wc -c < h32v20.sieve) - $(wc -c < h32.sieve) ))" -eq 9 ]
rm -rf h32_inst h32_si h32_oracle
$S install h32.sieve --to h32_inst > /dev/null
diff -r h32 h32_inst
$SI h32.sieve --to h32_si --yes > /dev/null
diff -r h32 h32_si
python sieve_ref.py unpack h32.sieve --to h32_oracle > /dev/null
diff -r h32 h32_oracle
# v32, too: mixing's word list scowl-en-80 counted by use ("F80": 4,096 a word of
# scowl-en-35, 64 one of scowl-en-60 only, 1 the rest), in scowl-en-60's place at v32. The
# code alone, smaller than D60's and read back by the oracle (its own trie and weights); a text, and
# two texts mixed together as a group: v32, given back by Sieve, sieve-install and
# the oracle; with --v31, scowl-en-60 as before.
$S mix dq.txt --words 80 --by-use --out dqf.mix | tr -d '\r' | grep -c "^mix       B17 F80 (mix-v4)" > /dev/null
python sieve_ref.py mix-unpack dqf.mix --v4 --params "B17 F80" --size 40000 --out dqf.back > /dev/null
cmp dq.txt dqf.back
[ "$(wc -c < dqf.mix)" -lt "$(wc -c < dq4.mix)" ]
rm -rf f32 f32g f33g
mkdir f32 f32g
# (Two texts kept in tests/texts, the start of METHODS.md and FORMATS.md as they were on 8 November
# 2026: the ways chosen below depend on the bytes, so any edit to the documents could change them.
# They did on 9 November, when a docs fix made the pair 20 bytes smaller without the match model.)
cp ../tests/texts/methods-9000.md f32/methods.md
cp ../tests/texts/methods-9000.md f32g/methods.md
cp ../tests/texts/formats-7000.md f32g/formats.md
$S locate f32 --v32 --installer f32.sieve --items 2>&1 | tr -d '\r' > f32.txt
grep -c "^mix       1 file(s) carried by mixing, mix-v4, 1 with a word list (1 counted by use) ([0-9]* bytes, carried in [0-9]*; sieve-manifest-v32)" f32.txt > /dev/null
$S locate f32g --v32 --installer f32g.sieve --items 2>&1 | tr -d '\r' > f32g.txt
grep -c "	mixt B[0-9]* F80 T" f32g.txt > /dev/null
# (From v33, the group with the match model's ways too: "mixt B16 F80 C2 T...", 3 November 2026.)
mkdir -p f33g
cp f32g/* f33g/
$S locate f33g --installer f33g.sieve --items 2>&1 | tr -d '\r' > f33g.txt
grep -c "	mixt B[0-9]* F80 C2 T" f33g.txt > /dev/null
for d in f32 f32g f33g; do
    rm -rf ${d}_inst ${d}_si ${d}_oracle
    $S install $d.sieve --to ${d}_inst > /dev/null
    diff -r $d ${d}_inst
    $SI $d.sieve --to ${d}_si --yes > /dev/null
    diff -r $d ${d}_si
    python sieve_ref.py unpack $d.sieve --to ${d}_oracle > /dev/null
    diff -r $d ${d}_oracle
done
$S locate f32 --v31 --installer f32v31.sieve --items 2>&1 | tr -d '\r' > f32v31.txt
if grep -q "sieve-manifest-v32\|F80" f32v31.txt; then exit 1; fi
# Prioritise Decoding Speed (27 October 2026; --prioritise-decoding, the File Locator's box): no
# file mixed, so the instructions install quickly: Don Quixote's start carried by the rope or
# LZMA2 instead, larger than with mixing, the report saying so; installed by Sieve and the oracle.
$S locate dq.txt --installer dqfast.sieve --items --prioritise-decoding | tr -d '\r' > dqfast.txt
grep -c "^decoding  prioritised: mixing not tried on 1 file(s) (Prioritise Decoding Speed)$" dqfast.txt > /dev/null
if grep -q "^mix " dqfast.txt; then exit 1; fi
[ "$(wc -c < dqfast.sieve)" -gt "$(wc -c < dq26.sieve)" ]
rm -rf dqfast_inst dqfast_oracle
$S install dqfast.sieve --to dqfast_inst > /dev/null
cmp dq.txt dqfast_inst/dq.txt
python sieve_ref.py unpack dqfast.sieve --to dqfast_oracle > /dev/null
cmp dq.txt dqfast_oracle/dq.txt
# The v34 draft's first piece (6 November 2026, Edward: "build the gif route for gifenc first"): a
# GIF carried as its skeleton and its frames' colour indices, its LZW coded again on installing as
# compress codes it ("gif <sub-block size> <skeleton size> <inner size>"). Two GIFs of ten frames,
# 160 by 120 pixels of 16 colours, written here with the oracle's own LZW coder: one cut into
# sub-blocks of 254 bytes as gifenc cuts them, carried so and given back byte for byte by Sieve,
# sieve-install and the oracle; the other cut 100 bytes first and 254 after, which the rule does
# not give back, so left as it is. Without the draft, neither opened.
rm -rf g34
mkdir g34
python - g34 <<'PY'
import os, sys
sys.path.insert(0, ".")
from sieve_ref import gif_lzw
def gif(cuts):
    o = bytearray(b"GIF89a" + (160).to_bytes(2, "little") + (120).to_bytes(2, "little") + bytes([0xF3, 0, 0]))
    o += bytes(c for i in range(16) for c in (i * 16, 255 - i * 16, (i * 40) & 255))
    o += b"\x21\xFF\x0BNETSCAPE2.0\x03\x01\x00\x00\x00"
    for k in range(10):
        o += b"\x21\xF9\x04\x04" + (8).to_bytes(2, "little") + b"\x00\x00"
        o += b"\x2C\x00\x00\x00\x00" + (160).to_bytes(2, "little") + (120).to_bytes(2, "little") + b"\x00\x04"
        px = bytes(((x + 3 * k) // 9 + (y * y) // 300 + (x * y) % 7 // 6) % 16 for y in range(120) for x in range(160))
        lzw, at = gif_lzw(4, px), 0
        for n in cuts(len(lzw)):
            o += bytes([n]) + lzw[at:at + n]
            at += n
        o += b"\x00"
    return bytes(o + b"\x3B")
def even(total):
    return [min(254, total - a) for a in range(0, total, 254)]
def odd(total):
    return [min(100, total)] + [min(254, total - a) for a in range(100, total, 254)]
open(os.path.join(sys.argv[1], "even.gif"), "wb").write(gif(even))
open(os.path.join(sys.argv[1], "odd.gif"), "wb").write(gif(odd))
PY
$S locate g34 --installer g34.sieve --items --compressed-temporary --v34-draft 2>&1 | tr -d '\r' > g34.txt
grep -q "^gifs      1 GIF(s) carried as their frames' colour indices, 10 frames" g34.txt
grep -q "^manifest  sieve-manifest-v34" g34.txt
grep -q "	gif 254 [0-9]* 192[0-9]* .*even.gif$" g34.txt
# (Mixed, where mixing carries them, as a picture as wide as the frames, from the recipe: I160x1.)
if grep "	gif .*even.gif$" g34.txt | grep -q "	gif [0-9 ]* mix"; then grep -q "	gif [0-9 ]* mix4 .*I160x1.*even.gif$" g34.txt; fi
if grep -q "	gif .*odd.gif$" g34.txt; then exit 1; fi
rm -rf g34_inst g34_si g34_oracle
$S install g34.sieve --to g34_inst > /dev/null
diff -r g34 g34_inst
$SI g34.sieve --to g34_si --yes > /dev/null
diff -r g34 g34_si
python sieve_ref.py unpack g34.sieve --to g34_oracle > /dev/null
diff -r g34 g34_oracle
$S locate g34 --installer g34v33.sieve --items --compressed-temporary 2>&1 | tr -d '\r' > g34v33.txt
if grep -q "sieve-manifest-v34\|^gifs \|	gif " g34v33.txt; then exit 1; fi
# The v34 draft's PDF (8 November 2026, Edward: "make a start on listing out and implementing all
# formats"): a PDF as a skeleton and members, as v31 carries a tar ("skm"): its deflate streams made
# again by preflate from what they hold, its JPEGs carried as they are, each a member carried its own
# way. A PDF written here: eight pages whose content streams are deflated by zlib, and the repository's
# door.jpg as a /DCTDecode picture. Given back byte for byte by Sieve and sieve-install, its listing
# read by the oracle; without the draft, not opened.
rm -rf p34
mkdir p34
python - p34/pages.pdf ../web/test-site/src/pictures/door.jpg <<'PY'
import sys, zlib
jpeg = open(sys.argv[2], "rb").read()
objs = []
def obj(body): objs.append(body); return len(objs)
pages = []
img = obj(b"<< /Type /XObject /Subtype /Image /Width 800 /Height 450 /ColorSpace /DeviceRGB /BitsPerComponent 8 /Filter /DCTDecode /Length " + str(len(jpeg)).encode() + b" >>\nstream\n" + jpeg + b"\nendstream")
for k in range(8):
    text = b"".join(b"BT /F1 11 Tf 72 %d Td (Line %d of page %d: the rope, the wave and the sieve.) Tj ET\n" % (720 - 14 * j, j, k) for j in range(48))
    z = zlib.compress(text + b"q 200 0 0 112 72 40 cm /Im0 Do Q\n")
    c = obj(b"<< /Length " + str(len(z)).encode() + b" /Filter /FlateDecode >>\nstream\n" + z + b"\nendstream")
    pages.append(c)
font = obj(b"<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica >>")
kids = []
first = len(objs) + 1
root_pages = first + 8
for c in pages:
    kids.append(obj(b"<< /Type /Page /Parent %d 0 R /MediaBox [0 0 612 792] /Contents %d 0 R /Resources << /Font << /F1 %d 0 R >> /XObject << /Im0 %d 0 R >> >> >>" % (root_pages, c, font, img)))
assert obj(b"<< /Type /Pages /Kids [" + b" ".join(b"%d 0 R" % k for k in kids) + b"] /Count 8 >>") == root_pages
cat = obj(b"<< /Type /Catalog /Pages %d 0 R >>" % root_pages)
out, offs = bytearray(b"%PDF-1.4\n%\xe2\xe3\xcf\xd3\n"), []
for i, body in enumerate(objs, 1):
    offs.append(len(out))
    out += b"%d 0 obj\n" % i + body + b"\nendobj\n"
x = len(out)
out += b"xref\n0 %d\n0000000000 65535 f \n" % (len(objs) + 1) + b"".join(b"%010d 00000 n \n" % o for o in offs)
out += b"trailer\n<< /Size %d /Root %d 0 R >>\nstartxref\n%d\n%%%%EOF\n" % (len(objs) + 1, cat, x)
open(sys.argv[1], "wb").write(out)
PY
$S locate p34 --installer p34.sieve --items --compressed-temporary --v34-draft 2>&1 | tr -d '\r' > p34.txt
grep -q "^pdfs      1 PDF(s) carried as their skeletons" p34.txt
grep -q "	skm 9 [0-9]* .*pages.pdf$" p34.txt
rm -rf p34_inst p34_si p34_oracle
$S install p34.sieve --to p34_inst > /dev/null
diff -r p34 p34_inst
$SI p34.sieve --to p34_si --yes > /dev/null
diff -r p34 p34_si
# (the oracle has no preflate: it reads the listing and every member, and says the PDF is not made
# again by it, as it says of a zip's deflate)
python sieve_ref.py unpack p34.sieve --to p34_oracle 2>&1 | tr -d '\r' | grep -q "^not made again by the oracle (preflate.*pages.pdf"
$S locate p34 --installer p34v33.sieve --items --compressed-temporary 2>&1 | tr -d '\r' > p34v33.txt
if grep -q "^pdfs " p34v33.txt; then exit 1; fi
# Compressed TIFFs (v34 draft, 8 November 2026, Edward: "work through all of the things on that
# list"): written here by Pillow, through the libtiff it carries: LZW with and without the horizontal
# predictor, deflate with it, PackBits, 16-bit grey (LZW, predictor), and three pages in one file; all
# six opened ("tiff <skeleton size> <inner size>") and given back byte for byte by Sieve and
# sieve-install. The oracle makes the LZW and PackBits ones again itself (its own coders, from the
# specification), and says the deflate one is not made again by it (no preflate). Opened whatever
# they come to (SIEVE_ARCHIVES_ALWAYS: pictures this small, 96 by 64, so that the oracle's mixing is
# quick, may not pay); an uncompressed TIFF, and one whose strips overlap, left to the other ways;
# without the draft, none.
pip_add "pillow==12.0.0"
rm -rf t34 t34_inst t34_si t34_oracle
mkdir t34
python - t34 <<'PY'
import os, random, struct, sys
from PIL import Image
out = sys.argv[1]
rng = random.Random(5)
w, h = 96, 64
px = bytes(min(255, max(0, (x * 255 // w + y // 2 + rng.randint(-6, 6)) % 256)) for y in range(h) for x in range(w) for c in range(3))
im = Image.frombytes("RGB", (w, h), px)
im.save(os.path.join(out, "lzw.tif"), compression="tiff_lzw")
im.save(os.path.join(out, "lzw-pred.tif"), compression="tiff_lzw", tiffinfo={317: 2})
im.save(os.path.join(out, "deflate-pred.tif"), compression="tiff_adobe_deflate", tiffinfo={317: 2})
im.save(os.path.join(out, "packbits.tif"), compression="packbits")
im.save(os.path.join(out, "plain.tif"), compression="raw")
g = Image.frombytes("I;16", (w, h), b"".join(struct.pack("<H", (x * 200 + y * 50 + rng.randint(0, 40)) & 0xFFFF) for y in range(h) for x in range(w)))
g.save(os.path.join(out, "grey16-lzw-pred.tif"), compression="tiff_lzw", tiffinfo={317: 2})
im.save(os.path.join(out, "pages.tif"), compression="tiff_lzw", save_all=True, append_images=[im.rotate(90, expand=True), im.convert("L")])
# strips that overlap: the LZW TIFF's second strip said to start where its first does
b = bytearray(open(os.path.join(out, "lzw.tif"), "rb").read())
ifd = struct.unpack_from("<I", b, 4)[0]
for k in range(struct.unpack_from("<H", b, ifd)[0]):
    tag, typ, cnt, val = struct.unpack_from("<HHII", b, ifd + 2 + 12 * k)
    if tag == 273:
        struct.pack_into("<I", b, val + 4, struct.unpack_from("<I", b, val)[0])
open(os.path.join(out, "overlap.tif"), "wb").write(bytes(b))
PY
SIEVE_ARCHIVES_ALWAYS=1 $S locate t34 --installer t34.sieve --items --compressed-temporary --v34-draft 2>&1 | tr -d '\r' > t34.txt
grep -q "^tiffs     6 TIFF(s) carried as their skeletons" t34.txt
for f in lzw.tif lzw-pred.tif deflate-pred.tif packbits.tif grey16-lzw-pred.tif pages.tif; do grep -q "	tiff [0-9]* [0-9]* .*	$f$" t34.txt; done
for f in plain.tif overlap.tif; do if grep -q "	tiff .*$f$" t34.txt; then exit 1; fi; done
$S install t34.sieve --to t34_inst > /dev/null
diff -r t34 t34_inst
$SI t34.sieve --to t34_si --yes > /dev/null
diff -r t34 t34_si
python sieve_ref.py unpack t34.sieve --to t34_oracle 2>&1 | tr -d '\r' > t34_oracle.txt
grep -q "^not made again by the oracle (preflate.*deflate-pred.tif" t34_oracle.txt
for f in lzw.tif lzw-pred.tif packbits.tif grey16-lzw-pred.tif pages.tif plain.tif overlap.tif; do cmp t34/$f t34_oracle/$f; done
SIEVE_ARCHIVES_ALWAYS=1 $S locate t34 --installer t34v33.sieve --items --compressed-temporary 2>&1 | tr -d '\r' > t34v33.txt
if grep -q "^tiffs \|	tiff " t34v33.txt; then exit 1; fi
# zlib streams inside a file of any other kind (v34 draft, 8 November 2026): a file of records, each
# a little binary head and a zlib stream (levels 1, 6 and 9, as a git pack's objects or a WOFF's
# tables are), carried with its streams opened by "sk" (v21) and given back byte for byte by Sieve and
# sieve-install; the oracle reads it and says it is not made again by it (no preflate). A file whose
# zlib header starts a stream cut short is left as it is; without the draft, neither is opened.
rm -rf z34 z34_inst z34_si z34_oracle
mkdir z34
python - z34 <<'PY'
import os, random, struct, sys, zlib
out = sys.argv[1]
rng = random.Random(9)
words = "sieve keeps every byte of what it carries and makes it again exactly as it was".split()
recs = []
for k in range(60):
    text = " ".join(rng.choice(words) for _ in range(rng.randint(40, 400))).encode()
    recs.append(struct.pack(">HI", k, len(text)) + zlib.compress(text, (1, 6, 9)[k % 3]))
open(os.path.join(out, "records.bin"), "wb").write(b"RECS" + b"".join(recs))
bad = zlib.compress(b" ".join(w.encode() for w in words) * 40, 9)
open(os.path.join(out, "cut.bin"), "wb").write(b"HEAD" + bytes(rng.randrange(256) for _ in range(2000)) + bad[:len(bad) // 2])
PY
$S locate z34 --installer z34.sieve --items --compressed-temporary --v34-draft 2>&1 | tr -d '\r' > z34.txt
grep -q "^zlib      1 file(s) carried with the zlib streams inside them opened" z34.txt
grep -q "	sk [0-9]* [0-9]* .*	records.bin$" z34.txt
if grep -q "	sk .*	cut.bin$" z34.txt; then exit 1; fi
$S install z34.sieve --to z34_inst > /dev/null
diff -r z34 z34_inst
$SI z34.sieve --to z34_si --yes > /dev/null
diff -r z34 z34_si
python sieve_ref.py unpack z34.sieve --to z34_oracle 2>&1 | tr -d '\r' | grep -q "^not made again by the oracle (preflate.*records.bin"
$S locate z34 --installer z34v33.sieve --items --compressed-temporary 2>&1 | tr -d '\r' > z34v33.txt
if grep -q "^zlib " z34v33.txt; then exit 1; fi
# MP3 (v34 draft, 8 November 2026, Edward: "lets focus on making the decoder safe for MP3"):
# tests/mp3/tone.mp3 (two sine tones, LAME) carried as packMP3 codes it ("pmp <inner size>") and given
# back byte for byte by Sieve and sieve-install; the oracle reads the listing and says it is not made
# again by it (no packMP3); without the draft, not opened. Then packMP3's decoder given 2,000 damaged
# forms of the MP3 (sieve_mp3_check, built with the tests): each refused or decoded, none ending the
# program (it was made safe for damaged input: results/mp3-pmp-2026-11-08.md).
rm -rf m34 m34_inst m34_si m34_oracle
mkdir m34
cp ../tests/mp3/tone.mp3 m34/
$S locate m34 --installer m34.sieve --items --compressed-temporary --v34-draft 2>&1 | tr -d '\r' > m34.txt
grep -q "^mp3       1 MP3(s) carried as packMP3 codes them" m34.txt
grep -q "	pmp [0-9]* .*	tone.mp3$" m34.txt
$S install m34.sieve --to m34_inst > /dev/null
diff -r m34 m34_inst
$SI m34.sieve --to m34_si --yes > /dev/null
diff -r m34 m34_si
python sieve_ref.py unpack m34.sieve --to m34_oracle 2>&1 | tr -d '\r' | grep -q "^not made again by the oracle (.*packMP3): tone.mp3"
$S locate m34 --installer m34v33.sieve --items --compressed-temporary 2>&1 | tr -d '\r' > m34v33.txt
if grep -q "^mp3 \|	pmp " m34v33.txt; then exit 1; fi
# (packMP3's decoder given 2,000 damaged forms of the MP3, where the build has the test program)
if [ -x ../build/sieve_mp3_check ] || [ -x ../build/sieve_mp3_check.exe ]; then
  ../build/sieve_mp3_check ../tests | tr -d '\r' | grep -q ", 0 failures$"
fi
# PCX pictures (v34 draft, 8 November 2026): written here by Pillow (RGB, grey with its palette after
# the pixels, colour-mapped, and an odd width whose plane lines are padded to an even length), carried
# as their pixels, planes interleaved ("pcx <skeleton size> <inner size>"), opened whatever they come
# to (SIEVE_ARCHIVES_ALWAYS: pictures this small may not pay); given back byte for byte by Sieve,
# sieve-install and the oracle, which codes them itself. One whose runs cross from one plane line to
# the next (as no writer measured does) is left as it is; without the draft, none is opened.
rm -rf x34 x34_inst x34_si x34_oracle
mkdir x34
python - x34 <<'PY'
import os, random, sys
from PIL import Image
out = sys.argv[1]
rng = random.Random(3)
def picture(w, h):
    return Image.frombytes("RGB", (w, h), bytes(min(255, (x * 7 + y * 3) % 256 // 32 * 32 + rng.choice((0, 0, 0, 9))) for y in range(h) for x in range(w) for c in range(3)))
im = picture(96, 64)
im.save(os.path.join(out, "rgb.pcx"))
im.convert("L").save(os.path.join(out, "grey.pcx"))
im.convert("P").save(os.path.join(out, "mapped.pcx"))
picture(95, 40).save(os.path.join(out, "odd.pcx"))
# a run across two plane lines: one grey line's last byte and the next line's first, coded as one run
w, h = 8, 4
head = bytearray(128)
head[0:4] = bytes([0x0A, 5, 1, 8])
head[8:10], head[10:12] = (w - 1).to_bytes(2, "little"), (h - 1).to_bytes(2, "little")
head[65], head[66:68] = 1, w.to_bytes(2, "little")
body = bytes([0xC0 | 12, 7]) + bytes([0xC0 | 20, 9])
open(os.path.join(out, "across.pcx"), "wb").write(bytes(head) + body)
PY
SIEVE_ARCHIVES_ALWAYS=1 $S locate x34 --installer x34.sieve --items --compressed-temporary --v34-draft 2>&1 | tr -d '\r' > x34.txt
grep -q "^pcx       4 PCX picture(s) carried as their pixels" x34.txt
for f in rgb.pcx grey.pcx mapped.pcx odd.pcx; do grep -q "	pcx [0-9]* [0-9]* .*	$f$" x34.txt; done
if grep -q "	pcx .*	across.pcx$" x34.txt; then exit 1; fi
$S install x34.sieve --to x34_inst > /dev/null
diff -r x34 x34_inst
$SI x34.sieve --to x34_si --yes > /dev/null
diff -r x34 x34_si
python sieve_ref.py unpack x34.sieve --to x34_oracle > /dev/null
diff -r x34 x34_oracle
SIEVE_ARCHIVES_ALWAYS=1 $S locate x34 --installer x34v33.sieve --items --compressed-temporary 2>&1 | tr -d '\r' > x34v33.txt
if grep -q "^pcx \|	pcx " x34v33.txt; then exit 1; fi
# The v34 draft's FLAC way (8 November 2026, Edward: "make a start on that, use the three levels as
# the size/balanced/speed paths"): a WAV carried as its samples coded by libFLAC and the rest of the
# file as it was ("flac <skeleton size> <inner size>"). Six WAVs written here: 8-bit mono of an odd
# length (a pad byte) with a LIST chunk after its data, 16-bit stereo with a JUNK chunk before its
# format, 24-bit WAVE_FORMAT_EXTENSIBLE, 32-bit mono, all four opened; a floating-point WAV and one
# whose data chunk says more than the file holds, both left to the other ways. Given back byte for
# byte by Sieve, sieve-install and the oracle (which decodes FLAC itself); at Decoding speed too;
# without the draft, none opened.
rm -rf w34
mkdir w34
python - w34 <<'PY'
import math, os, random, struct, sys
out = sys.argv[1]
def chunk(cid, body): return cid + struct.pack("<I", len(body)) + body + (b"\0" if len(body) & 1 else b"")
def fmt(ch, rate, bits, tag=1, ext=False):
    align = ch * bits // 8
    b = struct.pack("<HHIIHH", 0xFFFE if ext else tag, ch, rate, rate * align, align, bits)
    if ext:
        b += struct.pack("<HHI", 22, bits, 3) + bytes([1, 0, 0, 0, 0, 0, 0x10, 0, 0x80, 0, 0, 0xAA, 0, 0x38, 0x9B, 0x71])
    return chunk(b"fmt ", b)
def riff(*chunks):
    body = b"WAVE" + b"".join(chunks)
    return b"RIFF" + struct.pack("<I", len(body)) + body
rng = random.Random(7)
def sound(n, ch, amp, rate):
    # a gliding tone with a wandering second voice and some noise: smooth, but never repeating exactly
    out, ph, ph2 = [], 0.0, 0.0
    for k in range(n):
        ph += 2 * math.pi * (300 + 200 * math.sin(k / rate * 1.3)) / rate
        ph2 += 2 * math.pi * (1100 + 300 * math.sin(k / rate * 0.7)) / rate
        v = 0.6 * math.sin(ph) + 0.3 * math.sin(ph2)
        out.append([int(amp * v * (1 - 0.2 * c) + rng.gauss(0, amp / 300)) for c in range(ch)])
    return out
def le(v, n): return int.to_bytes(v, n, "little", signed=True)
s = sound(11025 * 2 + 1, 1, 90, 11025)  # 8-bit mono, odd: a pad byte; a LIST chunk after the data
open(os.path.join(out, "eight-bit-mono.wav"), "wb").write(riff(fmt(1, 11025, 8), chunk(b"data", bytes(v[0] + 128 for v in s)),
                                                                chunk(b"LIST", b"INFOISFT\x05\0\0\0Test\0\0")))
s = sound(44100, 2, 20000, 44100)  # 16-bit stereo, a JUNK chunk before the format
open(os.path.join(out, "stereo-16.wav"), "wb").write(riff(chunk(b"JUNK", b"\0" * 28), fmt(2, 44100, 16), chunk(b"data", b"".join(le(x, 2) for v in s for x in v))))
s = sound(24000, 2, 4000000, 48000)  # 24-bit, WAVE_FORMAT_EXTENSIBLE
open(os.path.join(out, "ext-24.wav"), "wb").write(riff(fmt(2, 48000, 24, ext=True), chunk(b"data", b"".join(le(x, 3) for v in s for x in v))))
s = sound(22050, 1, 1000000000, 22050)  # 32-bit integer mono
open(os.path.join(out, "int-32.wav"), "wb").write(riff(fmt(1, 22050, 32), chunk(b"data", b"".join(le(v[0], 4) for v in s))))
s = sound(22050, 1, 1, 22050)  # floating point (format 3): left to the other ways
open(os.path.join(out, "float.wav"), "wb").write(riff(fmt(1, 22050, 32, tag=3), chunk(b"data", b"".join(struct.pack("<f", 0.5 * math.sin(k / 9)) for k in range(22050)))))
s = sound(22050, 2, 9000, 22050)  # a data chunk saying more than the file holds: left as it is
body = b"".join(le(x, 2) for v in s for x in v)
open(os.path.join(out, "short-data.wav"), "wb").write(riff(fmt(2, 22050, 16), b"data" + struct.pack("<I", len(body) + 4000) + body))
# AIFF: chunks with big-endian sizes, samples big-endian and signed (AIFF-C "sowt": little-endian)
def achunk(cid, body): return cid + struct.pack(">I", len(body)) + body + (b"\0" if len(body) & 1 else b"")
def ext80(rate):
    e = rate.bit_length() - 1
    return struct.pack(">HQ", 16383 + e, rate << (63 - e))
def comm(ch, frames, bits, rate, kind=None):
    b = struct.pack(">HIH", ch, frames, bits) + ext80(rate)
    if kind: b += kind + b"\x0bnot named\0"
    return achunk(b"COMM", b)
def form(kind, *chunks):
    body = kind + b"".join(chunks)
    return b"FORM" + struct.pack(">I", len(body)) + body
def ssnd(data): return achunk(b"SSND", b"\0" * 8 + data)
def be(v, n, order="big"): return int.to_bytes(v, n, order, signed=True)
s = sound(44100, 2, 20000, 44100)  # 16-bit stereo, a NAME chunk before COMM
open(os.path.join(out, "stereo-16.aiff"), "wb").write(form(b"AIFF", achunk(b"NAME", b"a tone"), comm(2, len(s), 16, 44100), ssnd(b"".join(be(x, 2) for v in s for x in v))))
s = sound(11025 * 2 + 1, 1, 90, 11025)  # 8-bit mono, odd: a pad byte; an ANNO chunk after the samples
open(os.path.join(out, "eight-bit-mono.aiff"), "wb").write(form(b"AIFF", comm(1, len(s), 8, 11025), ssnd(bytes(v[0] & 255 for v in s)), achunk(b"ANNO", b"made here")))
s = sound(24000, 1, 4000000, 48000)  # 24-bit mono
open(os.path.join(out, "mono-24.aif"), "wb").write(form(b"AIFF", comm(1, len(s), 24, 48000), ssnd(b"".join(be(v[0], 3) for v in s))))
s = sound(22050, 2, 15000, 22050)  # AIFF-C "sowt": little-endian samples
open(os.path.join(out, "sowt.aifc"), "wb").write(form(b"AIFC", achunk(b"FVER", struct.pack(">I", 0xA2805140)), comm(2, len(s), 16, 22050, b"sowt"), ssnd(b"".join(be(x, 2, "little") for v in s for x in v))))
s = sound(22050, 1, 1, 22050)  # AIFF-C of floating-point samples: left to the other ways
open(os.path.join(out, "float.aifc"), "wb").write(form(b"AIFC", comm(1, 22050, 32, 22050, b"fl32"), ssnd(b"".join(struct.pack(">f", 0.5 * math.sin(k / 9)) for k in range(22050)))))
s = sound(22050, 1, 12000, 22050)  # its COMM after its SSND: left as it is
open(os.path.join(out, "comm-last.aiff"), "wb").write(form(b"AIFF", ssnd(b"".join(be(v[0], 2) for v in s)), comm(1, len(s), 16, 22050)))
# RF64 (its sizes in ds64), Wave64 (GUIDs, 64-bit sizes), Sun's .au (a known size, and one to the
# end), CAF (big-endian with its size, and little-endian to the end)
s = sound(22050, 2, 15000, 22050)
body = b"".join(le(x, 2) for v in s for x in v)
ds64 = chunk(b"ds64", struct.pack("<QQQI", 0, len(body), len(s), 0))
open(os.path.join(out, "rf64.wav"), "wb").write(b"RF64" + b"\xff" * 4 + b"WAVE" + ds64 + fmt(2, 22050, 16) + b"data" + b"\xff" * 4 + body)
guid = lambda h: bytes.fromhex(h)
def w64chunk(g, b):
    c = g + struct.pack("<Q", 24 + len(b)) + b
    return c + b"\0" * (-len(c) % 8)
fmtbody = struct.pack("<HHIIHH", 1, 1, 48000, 48000 * 3, 3, 24)
s = sound(12001, 1, 4000000, 48000)
chunks = w64chunk(guid("666d7420f3acd3118cd100c04f8edb8a"), fmtbody) + w64chunk(guid("64617461f3acd3118cd100c04f8edb8a"), b"".join(le(v[0], 3) for v in s))
open(os.path.join(out, "mono-24.w64"), "wb").write(guid("726966662e91cf11a5d628db04c10000") + struct.pack("<Q", 40 + len(chunks)) + guid("77617665f3acd3118cd100c04f8edb8a") + chunks)
s = sound(16000, 1, 9000, 8000)
data = b"".join(be(v[0], 2) for v in s)
open(os.path.join(out, "known.au"), "wb").write(b".snd" + struct.pack(">IIIII", 32, len(data), 3, 8000, 1) + b"a note\0\0" + data + b"tail")
s = sound(16000, 2, 9000, 16000)
open(os.path.join(out, "to-end.au"), "wb").write(b".snd" + struct.pack(">IIIII", 24, 0xFFFFFFFF, 3, 16000, 2) + b"".join(be(x, 2) for v in s for x in v))
def caf(chunks): return b"caff\0\x01\0\0" + b"".join(c + struct.pack(">q", len(b)) + b for c, b in chunks)
def desc(rate, ch, bits, little): return struct.pack(">d4sIIIII", rate, b"lpcm", 2 if little else 0, ch * bits // 8, 1, ch, bits)
s = sound(22050, 2, 3000000, 44100)
open(os.path.join(out, "known-be.caf"), "wb").write(caf([(b"desc", desc(44100, 2, 24, False)), (b"data", b"\0" * 4 + b"".join(be(x, 3) for v in s for x in v)), (b"free", b"\0" * 10)]))
s = sound(22050, 2, 15000, 44100)
open(os.path.join(out, "to-end-le.caf"), "wb").write(caf([(b"desc", desc(44100, 2, 16, True))]) + b"data" + struct.pack(">q", -1) + b"\0" * 4 + b"".join(be(x, 2, "little") for v in s for x in v))
PY
$S locate w34 --installer w34.sieve --items --compressed-temporary --v34-draft 2>&1 | tr -d '\r' > w34.txt
grep -q "^sound     14 file(s) of sound (WAV, AIFF, RF64, Wave64, .au, CAF), " w34.txt
grep -q "^manifest  sieve-manifest-v34" w34.txt
for f in eight-bit-mono.wav ext-24.wav int-32.wav stereo-16.wav stereo-16.aiff eight-bit-mono.aiff mono-24.aif sowt.aifc rf64.wav mono-24.w64 known.au to-end.au known-be.caf to-end-le.caf; do grep -q "	flac [0-9]* [0-9]* .*$f$" w34.txt; done
for f in float.wav short-data.wav float.aifc comm-last.aiff; do if grep -q "	flac .*$f$" w34.txt; then exit 1; fi; done
rm -rf w34_inst w34_si w34_oracle w34s_inst
$S install w34.sieve --to w34_inst > /dev/null
diff -r w34 w34_inst
$SI w34.sieve --to w34_si --yes > /dev/null
diff -r w34 w34_si
python sieve_ref.py unpack w34.sieve --to w34_oracle > /dev/null
diff -r w34 w34_oracle
$S locate w34 --installer w34s.sieve --items --compressed-temporary --v34-draft --priority speed 2>&1 | tr -d '\r' > w34s.txt
grep -q "^sound     14 file(s) of sound " w34s.txt
$S install w34s.sieve --to w34s_inst > /dev/null
diff -r w34 w34s_inst
$S locate w34 --installer w34v33.sieve --items --compressed-temporary 2>&1 | tr -d '\r' > w34v33.txt
if grep -q "sieve-manifest-v34\|^sound \|	flac " w34v33.txt; then exit 1; fi
# Sound as FLAC (--sound-to-flac, 8 November 2026): the same folder, each sound file of 1 or 2
# channels, up to 24 bits, at a rate FLAC's subset holds, installed as name.flac (where no namesake
# took that name first); each .flac decoded by the oracle's own FLAC reader to its file's samples. Then
# one of those FLACs given a tag and 8 KiB of padding and packed again: re-coded, the tag kept, the
# padding gone, the same samples.
rm -rf w34f_inst f34 f34_inst
$S locate w34 --installer w34f.sieve --items --v34-draft --sound-to-flac 2>&1 | tr -d '\r' > w34f.txt
grep -q "^as flac   10 file(s) of sound installed as .flac files of the same samples, 0 FLAC file(s) re-coded" w34f.txt
grep -q "^warning: 10 files of sound here are installed as .flac" w34f.txt
$S install w34f.sieve --to w34f_inst > /dev/null
python - w34 w34f_inst <<'PY'
import os, sys
sys.path.insert(0, ".")
from sieve_ref import wav_walk, flac_decode
src, out = sys.argv[1], sys.argv[2]
flacs = sorted(f for f in os.listdir(out) if f.endswith(".flac"))
assert len(flacs) == 10, flacs
for f in flacs:
    stem = f[:-5]
    orig = [g for g in sorted(os.listdir(src)) if g.rsplit(".", 1)[0] == stem][0]
    b = open(os.path.join(src, orig), "rb").read()
    ch, bits, at, size, big, unsigned8, _ = wav_walk(b, orig)
    nb = bits // 8
    size = len(b) - at if size is None else size
    raw = b[at:at + size]
    want = [(raw[k] - 128 if unsigned8 else raw[k] - 256 * (raw[k] > 127)) if nb == 1 else int.from_bytes(raw[k:k + nb], "big" if big else "little", signed=True) for k in range(0, len(raw), nb)]
    fch, fbits, total, got = flac_decode(open(os.path.join(out, f), "rb").read(), f)
    assert (fch, fbits) == (ch, bits) and got == want, f
PY
mkdir f34
python - w34f_inst/stereo-16.flac f34/tagged.flac <<'PY'
import struct, sys
f = open(sys.argv[1], "rb").read()
assert f[:4] == b"fLaC" and f[4] & 0x7F == 0
at, heads = 4, []
while True:  # its metadata blocks, to the last
    h, ln = f[at], int.from_bytes(f[at + 1:at + 4], "big")
    heads.append(f[at:at + 4 + ln])
    at += 4 + ln
    if h & 0x80:
        break
vendor = b"made here"
comment = struct.pack("<I", len(vendor)) + vendor + struct.pack("<II", 1, 11) + b"TITLE=Tones"
# STREAMINFO, a comment, 8 KiB of padding (the last block), the frames
out = b"fLaC" + bytes([0]) + heads[0][1:] + bytes([4]) + len(comment).to_bytes(3, "big") + comment + bytes([0x81]) + (8192).to_bytes(3, "big") + bytes(8192) + f[at:]
open(sys.argv[2], "wb").write(out)
PY
$S locate f34 --installer f34.sieve --items --v34-draft --sound-to-flac 2>&1 | tr -d '\r' > f34.txt
grep -q "^as flac   0 file(s) of sound installed as .flac files of the same samples, 1 FLAC file(s) re-coded" f34.txt
$S install f34.sieve --to f34_inst > /dev/null
python - f34/tagged.flac f34_inst/tagged.flac <<'PY'
import sys
sys.path.insert(0, ".")
from sieve_ref import flac_decode
a, b = open(sys.argv[1], "rb").read(), open(sys.argv[2], "rb").read()
assert len(b) < len(a) - 8000 and b"TITLE=Tones" in b
assert flac_decode(a, "a")[3] == flac_decode(b, "b")[3]
PY
# Sound as FLAC keeps tags (8 November 2026): a WAV's LIST INFO (its title in Latin-1, its artist)
# installed as a .flac whose Vorbis comments say them, in UTF-8; a WAV of six channels in FLAC's own
# order (its channel mask 0x3F) installed as a .flac of six; one of four in another order (0x0F) left
# as it is.
rm -rf tg34 tg34_inst
mkdir tg34
python - tg34 <<'PY'
import math, os, struct, sys
out = sys.argv[1]
def chunk(cid, body): return cid + struct.pack("<I", len(body)) + body + (b"\0" if len(body) & 1 else b"")
def wav(ch, mask, data, extra=b""):
    f = struct.pack("<HHIIHHHHI", 0xFFFE, ch, 22050, 22050 * ch * 2, ch * 2, 16, 22, 16, mask) + bytes([1, 0, 0, 0, 0, 0, 0x10, 0, 0x80, 0, 0, 0xAA, 0, 0x38, 0x9B, 0x71])
    body = b"WAVE" + chunk(b"fmt ", f) + chunk(b"data", data) + extra
    return b"RIFF" + struct.pack("<I", len(body)) + body
def tone(n, ch): return b"".join(struct.pack("<h", int(8000 * math.sin(k / (7 + c)))) for k in range(n) for c in range(ch))
info = chunk(b"LIST", b"INFO" + chunk(b"INAM", "Caf\u00e9 tune\0".encode("latin-1")) + chunk(b"IART", b"Sieve CI\0"))
open(os.path.join(out, "tagged.wav"), "wb").write(wav(2, 3, tone(6000, 2), info))
open(os.path.join(out, "six.wav"), "wb").write(wav(6, 0x3F, tone(3000, 6)))
open(os.path.join(out, "four.wav"), "wb").write(wav(4, 0x0F, tone(3000, 4)))
PY
$S locate tg34 --installer tg34.sieve --items --v34-draft --sound-to-flac 2>&1 | tr -d '\r' > tg34.txt
$S install tg34.sieve --to tg34_inst > /dev/null
test -f tg34_inst/tagged.flac
test -f tg34_inst/six.flac
cmp tg34/four.wav tg34_inst/four.wav
python - tg34_inst <<'PY'
import os, sys
sys.path.insert(0, ".")
from sieve_ref import flac_decode
d = sys.argv[1]
t = open(os.path.join(d, "tagged.flac"), "rb").read()
assert "TITLE=Caf\u00e9 tune".encode() in t and b"ARTIST=Sieve CI" in t
assert flac_decode(open(os.path.join(d, "six.flac"), "rb").read(), "six")[0] == 6
PY
# Floating-point sound (8 November 2026, Edward: "work through all of the things on that list"): a
# file of 32- or 64-bit floats whose every value is a whole number over 2^23 (as a float made from
# 24-bit or 16-bit integers is) carried as those numbers through FLAC at 24 bits; written here as a
# WAV of format 3, a WAVE_FORMAT_EXTENSIBLE float WAV, a WAV of doubles, an AIFF-C "fl32", an .au of
# encoding 6 and a little-endian CAF, all six opened; a float WAV with one -0 in it, and one with a
# value half a step off, both left to the other ways. Given back byte for byte by Sieve, sieve-install
# and the oracle; and as Sound as FLAC, each .flac's samples the floats' numbers.
rm -rf fl34 fl34_inst fl34_si fl34_oracle fl34f_inst
mkdir fl34
python - fl34 <<'PY'
import math, os, random, struct, sys
out = sys.argv[1]
rng = random.Random(11)
def tone(n, ch):
    v, ph = [], 0.0
    for k in range(n):
        ph += 2 * math.pi * (220 + 90 * math.sin(k / 9000)) / 22050
        v.extend(max(-8388608, min(8388607, int(3000000 * math.sin(ph + c) + rng.gauss(0, 3000)))) for c in range(ch))
    return v
def chunk(cid, body, o="<"): return cid + struct.pack(o + "I", len(body)) + body + (b"\0" if len(body) & 1 else b"")
def wav(ch, bits, data, ext=False, tag=3):
    if ext:
        f = struct.pack("<HHIIHHHHI", 0xFFFE, ch, 22050, 22050 * ch * bits // 8, ch * bits // 8, bits, 22, bits, 3) + bytes([3, 0, 0, 0, 0, 0, 0x10, 0, 0x80, 0, 0, 0xAA, 0, 0x38, 0x9B, 0x71])
    else:
        f = struct.pack("<HHIIHHH", tag, ch, 22050, 22050 * ch * bits // 8, ch * bits // 8, bits, 0)
    body = b"WAVE" + chunk(b"fmt ", f) + chunk(b"fact", struct.pack("<I", len(data) // (ch * bits // 8))) + chunk(b"data", data)
    return b"RIFF" + struct.pack("<I", len(body)) + body
def f32(v, o="<"): return b"".join(struct.pack(o + "f", x / 8388608) for x in v)
v = tone(8000, 2)
open(os.path.join(out, "float-24.wav"), "wb").write(wav(2, 32, f32(v)))
open(os.path.join(out, "float-ext.wav"), "wb").write(wav(2, 32, f32(v), ext=True))
w = [x >> 8 << 8 for x in tone(4000, 1)]  # 16-bit steps, as doubles
open(os.path.join(out, "double-16.wav"), "wb").write(wav(1, 64, b"".join(struct.pack("<d", x / 8388608) for x in w)))
e, m = 14 + 16383, 22050 << (63 - 14)
comm = struct.pack(">hIh", 2, len(v) // 2, 32) + struct.pack(">HQ", e, m) + b"fl32" + b"\x0532-bit float\0"
body = b"AIFC" + chunk(b"FVER", struct.pack(">I", 0xA2805140), ">") + chunk(b"COMM", comm, ">") + chunk(b"SSND", bytes(8) + f32(v, ">"), ">")
open(os.path.join(out, "float.aifc"), "wb").write(b"FORM" + struct.pack(">I", len(body)) + body)
open(os.path.join(out, "float-be.au"), "wb").write(b".snd" + struct.pack(">IIIII", 24, 0xFFFFFFFF, 6, 22050, 2) + f32(v, ">"))
desc = struct.pack(">d4sIIIII", 22050.0, b"lpcm", 3, 8, 1, 2, 32)
open(os.path.join(out, "float-le.caf"), "wb").write(b"caff\0\x01\0\0" + b"desc" + struct.pack(">q", len(desc)) + desc + b"data" + struct.pack(">q", 4 + 4 * len(v)) + bytes(4) + f32(v))
z = f32(v[:2000])
open(os.path.join(out, "minus-zero.wav"), "wb").write(wav(2, 32, z[:400] + struct.pack("<f", -0.0) + z[404:]))
open(os.path.join(out, "half-step.wav"), "wb").write(wav(2, 32, z[:400] + struct.pack("<f", 0.5 / 8388608) + z[404:]))
PY
$S locate fl34 --installer fl34.sieve --items --compressed-temporary --v34-draft 2>&1 | tr -d '\r' > fl34.txt
grep -q "^sound     6 file(s) of sound (WAV, AIFF, RF64, Wave64, .au, CAF), " fl34.txt
for f in float-24.wav float-ext.wav double-16.wav float.aifc float-be.au float-le.caf; do grep -q "	flac [0-9]* [0-9]* .*$f$" fl34.txt; done
for f in minus-zero.wav half-step.wav; do if grep -q "	flac .*$f$" fl34.txt; then exit 1; fi; done
$S install fl34.sieve --to fl34_inst > /dev/null
diff -r fl34 fl34_inst
$SI fl34.sieve --to fl34_si --yes > /dev/null
diff -r fl34 fl34_si
python sieve_ref.py unpack fl34.sieve --to fl34_oracle > /dev/null
diff -r fl34 fl34_oracle
$S locate fl34 --installer fl34f.sieve --items --v34-draft --sound-to-flac 2>&1 | tr -d '\r' > fl34f.txt
grep -q "^as flac   6 file(s) of sound installed as .flac files of the same samples" fl34f.txt
$S install fl34f.sieve --to fl34f_inst > /dev/null
python - fl34 fl34f_inst <<'PY'
import os, struct, sys
sys.path.insert(0, ".")
from sieve_ref import wav_walk, flac_decode
src, out = sys.argv[1], sys.argv[2]
flacs = sorted(f for f in os.listdir(out) if f.endswith(".flac"))
assert len(flacs) == 6, flacs
for f in flacs:
    orig = [g for g in sorted(os.listdir(src)) if g.rsplit(".", 1)[0] == f[:-5]][0]
    b = open(os.path.join(src, orig), "rb").read()
    ch, bits, at, size, big, _, flt = wav_walk(b, orig)
    assert flt, orig
    size = len(b) - at if size is None else size
    code = ("<>"[big]) + ("f" if bits == 32 else "d")
    want = [round(x * 8388608) for x in struct.unpack(code[0] + str(size // (bits // 8)) + code[1], b[at:at + size])]
    fch, fbits, total, got = flac_decode(open(os.path.join(out, f), "rb").read(), f)
    assert (fch, fbits) == (ch, 24) and got == want, f
PY
# Sound as FLAC's three ways (9 November 2026): the switch alone decides sound, not the archives'
# boxes. Temporary, the default: a WAV and an MP3 carried as `flac` and `pmp` with no box ticked,
# given back byte for byte; off: neither. With the archives' temporary, a zip holding a WAV and an MP3
# made again from its skeleton, its WAV member carried as `flac` and its MP3 as `pmp`, the zip given
# back byte for byte by Sieve and sieve-install; and a tar of a WAV and a text, its WAV member made
# again first, then the tar from it, by the oracle too (which has no preflate for a zip). With the
# archives' permanent and the sound's, a zip's WAV installed as a .flac in its folder.
rm -rf sm34 sm34z sm34_inst sm34o_inst sm34z_inst sm34z_si sm34w sm34w_oracle sm34p_inst
mkdir sm34 sm34z sm34w
python - sm34 <<'PY'
import math, os, random, struct, sys, zipfile
out = sys.argv[1]
rng = random.Random(34)
def wav(n):
    data = b"".join(struct.pack("<hh", int(6000 * math.sin(k / 9.0) + rng.randint(-300, 300)), int(5000 * math.sin(k / 13.0) + rng.randint(-300, 300))) for k in range(n))
    f = struct.pack("<HHIIHH", 1, 2, 22050, 22050 * 4, 4, 16)
    body = b"WAVE" + b"fmt " + struct.pack("<I", len(f)) + f + b"data" + struct.pack("<I", len(data)) + data
    return b"RIFF" + struct.pack("<I", len(body)) + body
open(os.path.join(out, "tune.wav"), "wb").write(wav(60000))
open(os.path.join(out, "other.wav"), "wb").write(wav(50000))
PY
cp ../tests/mp3/tone.mp3 sm34/
$S locate sm34 --installer sm34.sieve --items --v34-draft 2>&1 | tr -d '\r' > sm34.txt
grep -q "^sound     2 file(s) of sound" sm34.txt
grep -q "^mp3       1 MP3(s) carried as packMP3 codes them" sm34.txt
$S install sm34.sieve --to sm34_inst > /dev/null
diff -r sm34 sm34_inst
$S locate sm34 --installer sm34o.sieve --items --v34-draft --sound-off 2>&1 | tr -d '\r' > sm34o.txt
if grep -q "^sound \|^mp3 \|	flac \|	pmp " sm34o.txt; then exit 1; fi
$S install sm34o.sieve --to sm34o_inst > /dev/null
diff -r sm34 sm34o_inst
if $S locate sm34 --installer x.sieve --items --v34-draft --sound-off --sound-permanent 2> /dev/null; then exit 1; fi
python - sm34 sm34z/pack.zip sm34w/waves.tar <<'PY'
import os, sys, zipfile
src = sys.argv[1]
with zipfile.ZipFile(sys.argv[2], "w", zipfile.ZIP_DEFLATED) as z:
    for n in ("tune.wav", "tone.mp3"):
        z.write(os.path.join(src, n), n)
import io, tarfile
with tarfile.open(sys.argv[3], "w", format=tarfile.USTAR_FORMAT) as t:
    t.add(os.path.join(src, "other.wav"), "other.wav")
    notes = "".join("line %d of the notes\n" % k for k in range(300)).encode()
    info = tarfile.TarInfo("notes.txt")
    info.size, info.mtime = len(notes), 1700000000
    t.addfile(info, io.BytesIO(notes))
PY
$S locate sm34z --installer sm34z.sieve --items --v34-draft --compressed-temporary 2>&1 | tr -d '\r' > sm34z.txt
grep -q "	flac [0-9]* [0-9]* .*	pack.zip/1$" sm34z.txt
grep -q "	pmp [0-9]* .*	pack.zip/2$" sm34z.txt
$S install sm34z.sieve --to sm34z_inst > /dev/null
diff -r sm34z sm34z_inst
$SI sm34z.sieve --to sm34z_si --yes > /dev/null
diff -r sm34z sm34z_si
$S locate sm34w --installer sm34w.sieve --items --v34-draft --compressed-temporary 2>&1 | tr -d '\r' > sm34w.txt
grep -q "	flac [0-9]* [0-9]* .*	waves.tar/1$" sm34w.txt
python sieve_ref.py unpack sm34w.sieve --to sm34w_oracle > /dev/null
diff -r sm34w sm34w_oracle
$S locate sm34z --installer sm34p.sieve --items --v34-draft --compressed-permanent --sound-permanent 2>&1 | tr -d '\r' > sm34p.txt
$S install sm34p.sieve --to sm34p_inst > /dev/null
test -f sm34p_inst/pack/tune.flac
cmp sm34/tone.mp3 sm34p_inst/pack/tone.mp3
# sieve compress (6 November 2026; the right-click menu's "Compress to"): a folder with an accented
# name inside, an empty file and an empty folder, in every format, each read back by Python's own
# modules (and 7z by 7-Zip where the machine has it); one file alone in a stream format as it is.
pip_add "zstandard==0.25.0"
rm -rf cz cz_out
mkdir -p cz/src/sub/empty cz_out
cp sieve_ref.py cz/src/
printf 'caf\xc3\xa9' > "cz/src/sub/caf$(printf '\xc3\xa9').txt"
: > cz/src/sub/empty.txt
for f in zip 7z xz gz bz2 zst lz4 br lzma tar; do
    $S compress cz/src --format $f --out cz_out/src.$f > /dev/null
done
$S compress cz/src/sieve_ref.py --format xz > /dev/null
test -f cz/src/sieve_ref.py.xz
$S compress cz/src/sieve_ref.py --format zip > /dev/null
test -f cz/src/sieve_ref.zip
if $S compress cz/src --format zip --out cz_out/src.zip 2> /dev/null; then exit 1; fi
if $S compress cz/src --format rar 2> /dev/null; then exit 1; fi
rm cz/src/sieve_ref.py.xz cz/src/sieve_ref.zip
python -c "
import io, os, sys, tarfile, zipfile, gzip, lzma, bz2, zstandard, lz4.frame, brotli
want = {}
for root, dirs, files in os.walk('cz/src'):
    for d in dirs: want[os.path.relpath(os.path.join(root, d), 'cz').replace(os.sep, '/') + '/'] = b''
    for f in files: want[os.path.relpath(os.path.join(root, f), 'cz').replace(os.sep, '/')] = open(os.path.join(root, f), 'rb').read()
want['src/'] = b''
def from_tar(b):
    got = {}
    with tarfile.open(fileobj=io.BytesIO(b)) as t:
        for m in t.getmembers():
            got[m.name + ('/' if m.isdir() else '')] = b'' if m.isdir() else t.extractfile(m).read()
    return got
undo = {'gz': gzip.decompress, 'xz': lzma.decompress, 'lzma': lzma.decompress, 'bz2': bz2.decompress,
        'zst': lambda b: zstandard.ZstdDecompressor().stream_reader(io.BytesIO(b)).read(),
        'lz4': lz4.frame.decompress, 'br': brotli.decompress, 'tar': lambda b: b}
for f, u in undo.items():
    got = from_tar(u(open('cz_out/src.' + f, 'rb').read()))
    assert got == want, (f, sorted(set(got) ^ set(want)))
z = zipfile.ZipFile('cz_out/src.zip')
got = {i.filename: z.read(i) for i in z.infolist()}
assert got == want, ('zip', sorted(set(got) ^ set(want)))
assert open('cz_out/src.7z', 'rb').read(6) == b'7z\xbc\xaf\x27\x1c'
"
if command -v 7z > /dev/null; then
    rm -rf cz_7z
    7z x -bd -ocz_7z cz_out/src.7z > /dev/null
    diff -r cz/src cz_7z/src
fi
# Decompression bombs (6 November 2026): a 7z of 114 bytes whose header says its block makes 2^50
# bytes. Extracting it is refused before anything is unpacked (the blocks' declared sizes against
# the memory available), and the packer carries it as it is rather than opening it.
rm -rf bomb7z bomb7z_x bomb7z_i
mkdir -p bomb7z
python -c "
import lzma, struct, zlib
def num(v):
    first, mask = 0, 0x80
    for i in range(8):
        if v < (1 << (7 * (i + 1))):
            first |= v >> (8 * i)
            break
        first |= mask
        mask >>= 1
    else:
        i = 8
    return bytes([first]) + v.to_bytes(8, 'little')[:i]
data = bytes(1000)
packed = lzma.compress(data, format=lzma.FORMAT_RAW, filters=[{'id': lzma.FILTER_LZMA2, 'dict_size': 1 << 20}])
h = bytes([1, 4, 6]) + num(0) + num(1) + bytes([9]) + num(len(packed)) + bytes([0])
h += bytes([7, 0x0B]) + num(1) + bytes([0]) + num(1) + bytes([0x21, 0x21]) + num(1) + bytes([16])
h += bytes([0x0C]) + num(1 << 50) + bytes([0])
h += bytes([8, 0x0A, 1]) + struct.pack('<I', zlib.crc32(data)) + bytes([0, 0])
name = 'zeros.bin'.encode('utf-16-le') + bytes(2)
h += bytes([5]) + num(1) + bytes([0x11]) + num(len(name) + 1) + bytes([0]) + name + bytes([0, 0])
start = struct.pack('<QQI', len(packed), len(h), zlib.crc32(h))
open('bomb7z/claim.7z', 'wb').write(b'7z\xbc\xaf\x27\x1c\x00\x04' + struct.pack('<I', zlib.crc32(start)) + start + packed + h)"
if $SI bomb7z/claim.7z --to bomb7z_x --yes 2> bomb7z.txt; then exit 1; fi
grep -q "blocks declare .* MiB to unpack into memory" bomb7z.txt
test ! -e bomb7z_x/zeros.bin
$S locate bomb7z --installer bomb7z.sieve --optimise-archives --compressed-temporary --compressed-permanent | tr -d '\r' > bomb7z_l.txt
if grep -q "^remade" bomb7z_l.txt; then exit 1; fi
$S install bomb7z.sieve --to bomb7z_i > /dev/null
cmp bomb7z/claim.7z bomb7z_i/claim.7z
# An archive whose members every system could not write as they are is left as it is (7 November
# 2026, from encode.su: a cab in an installer held "turnonnotificationinactrobat.", was carried as its
# contents under Archive and Compressed Formats: permanent, and the install was refused on Windows).
# A zip holding a name with a dot at its end, and one holding two names one where case is ignored:
# each said, left as it is, and installed byte for byte.
rm -rf dotzip dotzip_i
mkdir -p dotzip
python -c "
import zipfile
t = open('../docs/METHODS.md', 'rb').read()[:60000]
with zipfile.ZipFile('dotzip/Data1.zip', 'w', zipfile.ZIP_DEFLATED) as z:
    z.writestr('notes.', t)
    z.writestr('ok.txt', t[::-1])
with zipfile.ZipFile('dotzip/Pair.zip', 'w', zipfile.ZIP_DEFLATED) as z:
    z.writestr('A.txt', t)
    z.writestr('a.txt', t[::-1])
"
$S locate dotzip --installer dotzip.sieve --compressed-permanent --progress 2> dotzip_p.txt | tr -d '\r' > dotzip_l.txt
grep -q "Data1.zip: not opened, left as it is: Data1/notes. is a name Windows cannot hold" dotzip_p.txt
grep -q "Pair.zip: not opened, left as it is: Pair/A.txt and Pair/a.txt are one name where case is ignored" dotzip_p.txt
if grep -q "^unpacked" dotzip_l.txt; then exit 1; fi
$S install dotzip.sieve --to dotzip_i > /dev/null
diff -r dotzip dotzip_i
# Permanent asked for by name on the command line (7 November 2026, Edward: "command line should
# ask explicitly"): --optimise-archives alone only makes again exactly, so a cab is left as it is;
# with --compressed-permanent it is carried as its contents, and packing warns, naming it.
rm -rf cabt cabt_i
mkdir -p cabt
cp ../tests/archives/test_read_format_cab_1.cab cabt/
SIEVE_ARCHIVES_ALWAYS=1 $S locate cabt --installer cabt.sieve --optimise-archives 2>&1 | tr -d '\r' > cabt_l.txt
if grep -q "^unpacked" cabt_l.txt; then exit 1; fi
if grep -q "carried as what it holds, and installing gives that in its place" cabt_l.txt; then exit 1; fi
$S install cabt.sieve --to cabt_i > /dev/null
diff -r cabt cabt_i
SIEVE_ARCHIVES_ALWAYS=1 $S locate cabt --installer cabtp.sieve --optimise-archives --compressed-permanent 2>&1 | tr -d '\r' > cabtp_l.txt
grep -q "^unpacked" cabtp_l.txt
grep -q "^warning: An archive or compressed file here is carried as what it holds, and installing gives that in its place, not the file itself: test_read_format_cab_1.cab" cabtp_l.txt
# Entries out of order (SPECIFICATIONS §12.1: sorted by their paths' bytes) are refused by the C++
# reader as by the oracle (review, 9 November 2026: C++ installed them); in order, both read on.
printf 'sieve-manifest-v12\nroot r\nstream lzma2 2 2 0\ntree b56042fb32681582a3e46f38d4d156e0ec908caa89e8fd579d100eea2bc940a8\nf\t1\traw\ty.txt\nf\t1\traw\tx.txt\nend\nab' > order12.manifest
rm -rf order12_out order12_ref
if $S install order12.manifest --to order12_out > order12.txt 2>&1; then exit 1; fi
grep -q "entries out of order" order12.txt
if python sieve_ref.py unpack order12.manifest --to order12_ref > order12_ref.txt 2>&1; then exit 1; fi
grep -q "entries out of order" order12_ref.txt
# The x86 filter restarted at each program (v34 draft, 11 November 2026): a program and a copy of it
# with ten bytes changed, much smaller than as v33, and installed the same by Sieve and the oracle.
mkdir -p x86pair
head -c 200000 $S > x86pair/a
python -c "import random; b=bytearray(open('x86pair/a','rb').read()); r=random.Random(7); [b.__setitem__(i, (b[i]+1+r.randrange(255))%256) for i in r.sample(range(len(b)),10)]; open('x86pair/b','wb').write(b)"
$S locate x86pair --installer x86pair33.sieve > /dev/null
$S locate x86pair --installer x86pair34.sieve --v34-draft | tr -d '\r' > x86pair34.txt
grep -q "sieve-manifest-v34" x86pair34.txt
[ "$(stat -c%s x86pair34.sieve 2>/dev/null || wc -c < x86pair34.sieve)" -lt "$(( $(stat -c%s x86pair33.sieve 2>/dev/null || wc -c < x86pair33.sieve) * 9 / 10 ))" ]
rm -rf x86pair_i x86pair_o
$S install x86pair34.sieve --to x86pair_i > /dev/null
diff -r x86pair x86pair_i
python sieve_ref.py unpack x86pair34.sieve --to x86pair_o > /dev/null
diff -r x86pair x86pair_o
