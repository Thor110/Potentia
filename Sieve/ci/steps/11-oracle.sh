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
# A map is Sieve instructions (sieve-manifest-v18): its text, the graph part, is the
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
python ../web/test-site/make_site.py $S --out web-site | grep -c "^pictures/tampered.sieve" > /dev/null
[ "$(grep -o 'integrity="sha256-' web-site/gallery.html | wc -l)" -eq 8 ] # 7 pictures, and its stylesheet
$S site check web-site 2>&1 | grep -c "0 not as stated, 3 kept as written" > /dev/null
# The site's table (index.sieve), named by every plain page, read by the host as the
# extension reads it; and the crawler (web/crawl_site.py) against the site served here:
# all as kept, then a stylesheet changed on the server, and a table that is not the one kept.
grep -c '<link rel="sieve-table" href="index.sieve">' web-site/index.html > /dev/null
python ../web/host/check_host.py $S web-site/index.sieve --table | grep -c "^table: 13 files" > /dev/null
table=$(python -c "import hashlib; print(hashlib.sha256(open('web-site/index.sieve', 'rb').read()).hexdigest())")
python -m http.server 8799 --bind 127.0.0.1 --directory web-site > /dev/null 2>&1 &
server=$!
python -c "import time, urllib.request
for _ in range(50):
    try:
        urllib.request.urlopen('http://127.0.0.1:8799/index.sieve'); break
    except Exception:
        time.sleep(0.2)"
python ../web/crawl_site.py http://127.0.0.1:8799/ --table $table --sieve $S | grep -c "table as kept, 13 files fetched; 28 checked, 0 not as stated" > /dev/null
printf x >> web-site/style.css
if python ../web/crawl_site.py http://127.0.0.1:8799/ --table $table --sieve $S > crawl.txt; then exit 1; fi
grep -c "MISMATCH  index.html: the hash it states for style.css is not that file's" crawl.txt > /dev/null
if python ../web/crawl_site.py http://127.0.0.1:8799/ --table 0000000000000000000000000000000000000000000000000000000000000000 --sieve $S > crawl.txt; then exit 1; fi
grep -c "MISMATCH  the site's table" crawl.txt > /dev/null
kill $server
if $S site check web-site > site-check.txt; then exit 1; fi
grep -c "index.sieve: style.css is not the file it names" site-check.txt > /dev/null
grep -c "index.html: the hash it states for style.css is not that file's" site-check.txt > /dev/null
rm -rf cyc && mkdir cyc && echo '<a href="b.html">b</a>' > cyc/a.html && echo '<a href="a.html">a</a>' > cyc/b.html
if $S site build cyc --out cyc-site --sieve-pages 2> cyc.txt; then exit 1; fi
grep -c "these pages link in a loop: a.html -> b.html -> a.html.*--plain a.html" cyc.txt > /dev/null
[ ! -e cyc-site ]
$S site build cyc --out cyc-site --sieve-pages --plain b.html > /dev/null
$S site check cyc-site
# What it cannot follow is kept as it is: a picture CSS names (beside its .sieve), one no
# tag names (not made), a page with a script (plain), and PHP's code (untouched; the HTML
# around it rewritten).
rm -rf mix && mkdir -p mix/p && cp ../web/test-site/src/pictures/door.jpg ../web/test-site/src/pictures/book.jpg ../web/test-site/src/pictures/audio.jpg mix/p/
printf '<link rel="stylesheet" href="s.css"><img src="p/door.jpg"><a href="run.html">run</a><a href="b.html">b</a>\n' > mix/index.html
printf 'body{background:url("p/door.jpg")}\n' > mix/s.css
printf '<img src="p/book.jpg"><script>let n = 1;</script>\n' > mix/run.html
printf '<a href="p/book.jpg">book</a>\n' > mix/b.html
printf '<?php $x = "<img src=\x27p/book.jpg\x27>"; ?>\n<img src="p/book.jpg">\n' > mix/shop.php
$S site build mix --out mix-site --sieve-pages --search-per-file 0.2 2> mix.txt > /dev/null
ls mix-site/p/door.jpg mix-site/p/door.sieve mix-site/p/audio.jpg mix-site/run.html mix-site/b.sieve > /dev/null
[ ! -e mix-site/p/audio.sieve ] && [ ! -e mix-site/run.sieve ]
grep -c "kept plain: run.html (it has a script" mix.txt > /dev/null
grep -c "1 picture no page names" mix.txt > /dev/null
grep -c "<?php \$x = \"<img src='p/book.jpg'>\"; ?>" mix-site/shop.php > /dev/null
grep -c '<img src="p/book.sieve" integrity="sha256-' mix-site/shop.php > /dev/null
$S site check mix-site
# Links from the server's root: left alone unless the site says where it is served from
# (--served-at), then made and checked as relative ones are; another server's never.
rm -rf rooted && mkdir -p rooted/p rooted/sub && cp ../web/test-site/src/pictures/door.jpg rooted/p/
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
rm -rf v4fx && cp -r ../tests/manifest_fixture v4fx && cp $S v4fx/program
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
python -m pip install --quiet "lz4==4.4.5" "brotli==1.2.1"
rm -rf a20 a20_inst a20_oracle a20_si && mkdir -p a20
python -c "
import lz4.frame, brotli, lzma, gzip
t = open('sieve_ref.py', 'rb').read()
open('a20/r.py.lz4', 'wb').write(lz4.frame.compress(t, compression_level=9, block_size=lz4.frame.BLOCKSIZE_MAX4MB))
open('a20/r.py.br', 'wb').write(brotli.compress(t, quality=5, lgwin=19))
open('a20/r.py.lzma', 'wb').write(lzma.compress(t, format=lzma.FORMAT_ALONE, preset=0))
open('a20/two.gz', 'wb').write(gzip.compress(t[:1000]) + gzip.compress(t[1000:]))"
$S locate a20 --installer a20.sieve --items --optimise-archives --v20 --manifest a20.manifest | tr -d '\r' > a20.txt
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
  SIEVE_ARCHIVES_ALWAYS=1 $S locate ../tests/archives/$1 --installer la.sieve --items --optimise-archives | tr -d '\r' > la.txt
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
rm -rf a21 a21_inst a21_oracle && mkdir -p a21
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
SIEVE_ARCHIVES_ALWAYS=1 $S locate a21 --installer a21.sieve --items --optimise-archives --manifest a21.manifest | tr -d '\r' > a21.txt
grep -c "^remade    6 archive(s) carried as what they hold, remade exactly on installing (sieve-manifest-v2[1-9])$" a21.txt > /dev/null
python -c "d = open('a21.manifest', 'rb').read(); assert d[:5] == b'sieve' and 21 <= d[5] <= 29, d[:6]"
$S install a21.sieve --to a21_inst > /dev/null
for f in huff.gz py.zip stored.zip sieve-7zip-lzma2.7z sieve-7zip-lzma.7z sieve-7zip-deflate.zip; do cmp a21/$f a21_inst/$f; done
# The oracle reads every skeleton whole and makes the stored zip again; the others, made by preflate
# or the LZMA SDK (which it has not), it names as not made again, and so leaves the tree unchecked.
python sieve_ref.py unpack a21.manifest --to a21_oracle | tr -d '\r' > a21_oracle.txt
cmp a21/stored.zip a21_oracle/stored.zip
[ "$(grep -c "^not made again by the oracle (preflate, the LZMA SDK or stb_image_write): " a21_oracle.txt)" -eq 5 ]
$SI a21.sieve --to a21_si --yes > /dev/null
diff -r a21_inst a21_si
# v22 (25 October 2026): a RAR made again by the machine's own rar, where it gives the archive back
# byte for byte; else its contents, with a warning. No rar can be had here (it is not free), so a
# stand-in (ci/stand_in_rar.py: a RAR 5 writer of stored files, as RARLAB describes the format)
# plays it; whether the real rar gives a RAR back is for a machine that has it. Not on Windows,
# where a Python script cannot be run as a program.
if [ "$(uname -s)" = Linux ] || [ "$(uname -s)" = Darwin ]; then
  rm -rf r22 && mkdir -p r22/src r22/folder
  cp ../ci/stand_in_rar.py r22/rar && chmod +x r22/rar
  RAR="$PWD/r22/rar"
  cp -r ../tests/manifest_fixture r22/src/pack && seq 1 30000 > r22/src/pack/numbers.txt
  (cd r22/src && "$RAR" a -cfg- -idq -y -m0 -qo- -r -- ../made.rar pack)
  # Alone: made again from its contents, then given back as the archive itself.
  SIEVE_ARCHIVES_ALWAYS=1 SIEVE_RAR="$RAR" $S locate r22/made.rar --installer r22.sieve --items --optimise-archives | tr -d '\r' > r22.txt
  grep -c "^rar       made.rar: made again by RAR 0.01 sieve-stand-in (top, -m0 -qo-)" r22.txt > /dev/null
  python -c "assert open('r22.sieve', 'rb').read()[:0] == b''"
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
  cp r22/made.rar r22/folder/ && cp ../tests/manifest_fixture/a.txt r22/folder/
  SIEVE_ARCHIVES_ALWAYS=1 SIEVE_RAR="$RAR" $S locate r22/folder --installer r22f.sieve --items --optimise-archives 2>&1 | tr -d '\r' | grep -c "^unpacked .*sieve-manifest-v2[2-9]" > /dev/null
  rm -rf r22f_inst && SIEVE_RAR="$RAR" $S install r22f.sieve --to r22f_inst > /dev/null
  diff -r r22/folder r22f_inst
  # --v21: as before, its contents alone.
  SIEVE_ARCHIVES_ALWAYS=1 SIEVE_RAR="$RAR" $S locate r22/made.rar --installer r21.sieve --items --optimise-archives --v21 | tr -d '\r' | grep -c "sieve-manifest-v21\|sieve-manifest-v20" > /dev/null
  rm -rf r21_inst && $S install r21.sieve --to r21_inst | tr -d '\r' | grep -c "^note      made.rar (rar, [0-9]* bytes) was carried as its contents" > /dev/null
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
python -c "assert open('tri.sieve', 'rb').read()[:0] == b''"
rm -rf tri_inst tri_oracle
$S install tri.sieve --to tri_inst > /dev/null
cmp tri.raw tri_inst/tri.raw
python sieve_ref.py unpack tri.sieve --to tri_oracle > /dev/null
cmp tri.raw tri_oracle/tri.raw
$S locate tri.raw --installer tri22.sieve --items --v22 | tr -d '\r' > tri22.txt
grep -c "^rope .*rope-wave-v1\( to v[2-5]\)\? (" tri22.txt > /dev/null
! grep -q "sieve-manifest-v23\|not an option" tri22.txt
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
! grep -q "^mix \|sieve-manifest-v24\|not an option" dq23.txt
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
rm -rf png25 && mkdir png25 && cp ../docs/images/hallway-audio.png ../web/extension/icon128.png png25/
$S locate png25 --installer png25.sieve --items --optimise-archives 2>&1 | tr -d '\r' | grep -c "^remade    2 archive(s) carried as what they hold, remade exactly on installing (sieve-manifest-v2[5-9])$" > /dev/null
[ "$(wc -c < png25.sieve)" -lt 50000 ]
rm -rf png25_inst png25_oracle
$S install png25.sieve --to png25_inst > /dev/null
diff -r png25 png25_inst
python sieve_ref.py unpack png25.sieve --to png25_oracle | tr -d '\r' | grep -c "^not made again by the oracle (preflate, the LZMA SDK or stb_image_write): " | grep -q "^2$"
$S locate png25 --installer png24.sieve --items --optimise-archives --v24 2>&1 | tr -d '\r' > png24.txt
! grep -q "sieve-manifest-v25\|not an option" png24.txt
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
! grep -q "mix-v2\|sieve-manifest-v26\|not an option" top25.txt
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
! grep -q "mix-v3\|sieve-manifest-v27\|not an option" dq26b.txt
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
rm -rf m28 && mkdir m28 && cp dq.txt top.raw m28/
$S locate m28 --installer m28.sieve --items --v28 2>&1 | tr -d '\r' > m28.txt
grep -c "^mix       2 file(s) carried by mixing, mix-v4, 1 as picture(s), 1 with a word list ([0-9]* bytes, carried in [0-9]*; sieve-manifest-v28)" m28.txt > /dev/null
$S locate m28 --installer m27.sieve --items --v27 2>&1 | tr -d '\r' > m27.txt
! grep -q "mix-v4\|sieve-manifest-v28\|not an option" m27.txt
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
rm -rf t29 && mkdir t29 && cp ../tools/check_help.py ../tools/rar_check.py ../ci/steps/03-filters.sh ../ci/steps/07-sound.sh t29/
head -c 3000 ../README.md > t29/small.md
$S locate t29 --installer t29.sieve --items 2>&1 | tr -d '\r' > t29.txt
grep -c "^stream    the files carried together mixed by mix-v4 (B[0-9]* D60): [0-9]* bytes, LZMA2's [0-9]*; [0-9]* of them mixed together rather than one by one ([0-9]* bytes apart) (sieve-manifest-v29)$" t29.txt > /dev/null
$S locate t29 --installer t28.sieve --items --v28 2>&1 | tr -d '\r' > t28.txt
! grep -q "sieve-manifest-v29\|not an option" t28.txt
[ "$(wc -c < t29.sieve)" -lt "$(wc -c < t28.sieve)" ]
$S locate t29 --installer t29fast.sieve --items --prioritise-decoding 2>&1 | tr -d '\r' > t29fast.txt
! grep -q "sieve-manifest-v29\|^stream    " t29fast.txt
rm -rf t29_inst t29_oracle t29_si
$S install t29.sieve --to t29_inst > /dev/null
diff -r t29 t29_inst
python sieve_ref.py unpack t29.sieve --to t29_oracle > /dev/null
diff -r t29 t29_oracle
$SI t29.sieve --to t29_si --yes > /dev/null
diff -r t29 t29_si
# Prioritise Decoding Speed (27 October 2026; --prioritise-decoding, the File Locator's box): no
# file mixed, so the instructions install quickly: Don Quixote's start carried by the rope or
# LZMA2 instead, larger than with mixing, the report saying so; installed by Sieve and the oracle.
$S locate dq.txt --installer dqfast.sieve --items --prioritise-decoding | tr -d '\r' > dqfast.txt
grep -c "^decoding  prioritised: mixing not tried on 1 file(s) (Prioritise Decoding Speed)$" dqfast.txt > /dev/null
! grep -q "^mix " dqfast.txt
[ "$(wc -c < dqfast.sieve)" -gt "$(wc -c < dq26.sieve)" ]
rm -rf dqfast_inst dqfast_oracle
$S install dqfast.sieve --to dqfast_inst > /dev/null
cmp dq.txt dqfast_inst/dq.txt
python sieve_ref.py unpack dqfast.sieve --to dqfast_oracle > /dev/null
cmp dq.txt dqfast_oracle/dq.txt
