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
$S site build rooted --out rooted-site --sieve-pages --served-at /Potentia/ --search-per-file 0.2 > /dev/null 2>&1
grep -c '<img src="/Potentia/p/door.sieve" integrity="sha256-' rooted-site/index.html > /dev/null
grep -c '<a href="/Potentia/sub/n.sieve" integrity="sha256-' rooted-site/index.html > /dev/null
grep -c '<img src="//example.com/x.jpg">' rooted-site/index.html > /dev/null
$S site check rooted-site --served-at /Potentia/
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
