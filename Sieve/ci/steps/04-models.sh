#!/usr/bin/env bash
# Sieve CI, step 04: A model is one object with an address on two lines.
# Run by .github/workflows/build.yml (on Windows, Linux and macOS), or alone or with the
# others by ci/run.sh (ci/README.md), from the Sieve folder with the Release build in build/.
set -eo pipefail
cd "$(dirname "$0")/../.."
. ci/lib.sh
sieve_ci_no_display # SDL with no screen, as the workflow sets it

trap 'echo "::error::the step stopped at its line $LINENO: $BASH_COMMAND" >&2' ERR # names the failing line
q() { grep -c "$@" > /dev/null; }
exe=build/sieve
[ -f build/Release/sieve.exe ] && exe=build/Release/sieve.exe
$exe mesh | tr -d '\r' | q "^space        models/V8/F12/C16/key=sieve/modelspace-v1"
$exe mesh | tr -d '\r' | q "^as text      304 characters"
# A cube in, its address out, and the same cube back.
printf 'v -1 -1 -1\nv 1 -1 -1\nv 1 1 -1\nv -1 1 -1\nv -1 -1 1\nv 1 -1 1\nv 1 1 1\nv -1 1 1\n' > cube.obj
printf 'f 1 2 3\nf 1 3 4\nf 5 7 6\nf 5 8 7\nf 1 5 6\nf 1 6 2\nf 2 6 7\nf 2 7 3\nf 3 7 8\nf 3 8 4\nf 4 8 5\nf 4 5 1\n' >> cube.obj
a=$($exe mesh --warp cube.obj 2>/dev/null | tr -d '\r' | awk '$1 == "positional" {print $2}')
echo "cube: $a"
test "$a" = "000f00ff00f000ff0ffff0ff0504e6b3e128a4dc725baedf8e0"
$exe mesh --read "$a" --out round.obj 2>/dev/null
# Symmetric on the grid, and no two spaces in a row anywhere in the canonical form.
sed -n 1p round.obj | tr -d '\r' | q "^v -0.9375 -0.9375 -0.9375$"
sed -n 2p round.obj | tr -d '\r' | q "^v +0.9375 -0.9375 -0.9375$"
if grep -c "  " round.obj > /dev/null; then exit 1; fi
test "$(wc -c < round.obj | tr -d ' ')" = "304"
# The same file as a unit of an ascii96 text line, and back, byte for byte.
t=$($exe warp --alphabet ascii96 --length 304 --mode positional --file round.obj | tr -d '\r' | awk '$1 == "positional" {print $2}')
$exe read --alphabet ascii96 --length 304 --mode positional "$t" --out viatext.obj > /dev/null
cmp round.obj viatext.obj
b=$($exe mesh --warp viatext.obj 2>/dev/null | tr -d '\r' | awk '$1 == "positional" {print $2}')
test "$a" = "$b"
# Browsing the line, and a shape it cannot have.
$exe mesh --browse 2 --seed 4 > /dev/null
# Worlds: models of the models line placed in a world. Composed, read back, and its .obj
# the oracle's, byte for byte.
$exe world | tr -d '\r' | q "^line         worlds  (worlds/image/mono/10x10/L100+lower27/L32+V8/F12/C16x4/G8/key=sieve/worldspace-v1)"
$exe world --world-models 2 --compose "0:0.0.0.0|1:7.7.7.5" --title "two models" | tr -d '\r' > wc.txt
w=$(awk '$1 == "positional" {print $2}' wc.txt)
$exe world --world-models 2 --read "$w" --out world.obj | tr -d '\r' | q "^slots        0*:0.0.0.0|0*1:7.7.7.5$"
python reference/sieve_ref.py world-obj --world-models 2 --slots "0:0.0.0.0|1:7.7.7.5" > world-ref.obj
cmp world.obj world-ref.obj
ws=$(awk '$1 == "scrambled" {print $2}' wc.txt)
$exe world --world-models 2 --read "$ws" --mode scrambled --out world2.obj > /dev/null
cmp world.obj world2.obj
# A model file taken apart (sieve tensors; IDEAS §15): a small stand-in Llama model made by the
# oracle, its start rebuilt by the tool from the config alone, and its statistics the oracle's.
python reference/sieve_ref.py tensors-fixture --config tests/llama_tiny_config.json --seed 7 --out tiny.safetensors
$exe tensors tiny.safetensors --config tests/llama_tiny_config.json | tr -d '\r' > tens.txt
cat tens.txt
q "^layout       safetensors-layout-v1: the start is rebuilt" tens.txt
q "the start is rebuilt from config.json$" tens.txt
q "^weights      211072 bytes: 105536 values (BF16 20)$" tens.txt
$exe tensors --config tests/llama_tiny_config.json --start-out tiny-start.bin > /dev/null
head -c "$(wc -c < tiny-start.bin)" tiny.safetensors | cmp - tiny-start.bin
$exe tensors tiny.safetensors --tsv | tr -d '\r' > tens-cpp.tsv
python reference/sieve_ref.py tensors tiny.safetensors --tsv | tr -d '\r' | diff tens-cpp.tsv -
$exe tensors tiny.safetensors --stats | tr -d '\r' | q "^coded by frequency  "
# The same model coded under the prior (sieve-weights-v1): the tool's file and the oracle's are
# one file, and each rebuilds the model, byte for byte.
$exe tensors tiny.safetensors --pack tiny.sieve-weights | tr -d '\r' | q "read back: it rebuilds the file"
python reference/sieve_ref.py weights-pack tiny.safetensors --out tiny-ref.sieve-weights
cmp tiny.sieve-weights tiny-ref.sieve-weights
python reference/sieve_ref.py weights-unpack tiny.sieve-weights --out tiny-ref.safetensors
cmp tiny.safetensors tiny-ref.safetensors
$exe tensors --unpack tiny-ref.sieve-weights --out tiny-back.safetensors > /dev/null
cmp tiny.safetensors tiny-back.safetensors
# The same model run by Sieve itself (sieve chat): with the oracle's stand-in tokenizer, its tokens
# are the oracle's on texts of every kind, its logits the oracle's (computed in 64-bit floats)
# to within a thousandth, and a reply comes back.
mkdir -p tinymodel
cp tests/llama_tiny_config.json tinymodel/config.json
cp tests/llama_tiny_tokenizer.json tinymodel/tokenizer.json
cp tiny.safetensors tinymodel/model.safetensors
python reference/sieve_ref.py llm-tokenizer-fixture --out tok-again.json
python -c "import json,sys; sys.exit(json.load(open('tok-again.json',encoding='utf-8')) != json.load(open('tests/llama_tiny_tokenizer.json',encoding='utf-8')))"
python reference/sieve_ref.py llm-corpus --out corpus.jsonl
$exe chat --model tinymodel --encode-lines corpus.jsonl | tr -d '\r' > tok-cpp.txt
python reference/sieve_ref.py llm-encode-lines corpus.jsonl --model tinymodel | tr -d '\r' | diff tok-cpp.txt -
$exe chat --model tinymodel --logits "The library holds every book. It's 2026." --logits-out tiny-logits.f32 > /dev/null
python reference/sieve_ref.py llm-logits "The library holds every book. It's 2026." --model tinymodel --compare tiny-logits.f32
$exe chat --model tinymodel --prompt "hello" --temperature 0 --max-tokens 8 > /dev/null
# The AI line (sieve ai): every model of one shape, its weights its address. The model at a
# bearing is the oracle's, file for file, in both orderings; its logits are the oracle's; its
# file warps back to its address; and saved, sieve chat --raw talks to it.
$exe ai | tr -d '\r' | q "^models       2^32768 "
for m in positional scrambled; do
  $exe ai --bearing 246 --mode $m --out ai246-$m > /dev/null
  python reference/sieve_ref.py ai-read --bearing 246 --mode $m --out ai246-$m-ref.safetensors > /dev/null
  cmp ai246-$m/model.safetensors ai246-$m-ref.safetensors
done
$exe ai --warp ai246-positional | tr -d '\r' | q "^positional   aeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee\.\.\."
$exe chat --model ai246-scrambled --logits "Hello, shelf." --logits-out ai-logits.f32 > /dev/null
python reference/sieve_ref.py llm-logits "Hello, shelf." --model ai246-scrambled --compare ai-logits.f32
$exe chat --model ai246-positional --raw --prompt "Hello" --max-tokens 16 > /dev/null
# Training (sieve ai-train, training-v1): runs recorded and trained, the order made from
# training.ini (order-v1) in each of its three readings, the files in a chosen order; the
# oracle, written apart, trains each again and writes the same result.txt. Trained again
# from its .sieve with one thread, a run is reproduced. A run whose order is written out
# (order.txt), with windows of two steps swapped, is not (within a step the order cannot
# matter: its windows' gradients are summed). A run in the first runs' format still trains
# to its result, by the tool and the oracle; and the run shipped with Sieve trains again to
# its own result, as it did where it was made.
mkdir -p corp && cp tests/training_corpus.txt corp/a.txt && head -c 150 tests/training_corpus.txt | tr 'a-z' 'b-za' > corp/b.txt
printf 'b.txt\n' > file-order.txt
for rd in in-order shuffled-within-files shuffled; do
  $exe ai-train --record corp --out run-$rd --reading $rd --file-order file-order.txt --ai-width 8 --ai-heads 2 --ai-bits 3 --window 17 --epochs 2 --checkpoint-every 3 --learning-rate 0.01 --quiet > /dev/null
  python reference/sieve_ref.py train-replay --run run-$rd | tr -d '\r' | cmp - run-$rd/result.txt
done
q "^file = corpus/b.txt" run-in-order/training.ini
# A corpus is a folder: a single file is refused.
if $exe ai-train --record tests/training_corpus.txt --out not-a-run > /dev/null 2>&1; then exit 1; fi
mkdir -p tiny-corp && cp tests/training_corpus.txt tiny-corp/
$exe ai-train --record tiny-corp --out tiny-run --ai-width 8 --ai-heads 2 --ai-bits 3 --window 17 --epochs 2 --checkpoint-every 3 --learning-rate 0.01 --write-order --checkpoint-addresses --sieve tiny-run.sieve --quiet > /dev/null
python reference/sieve_ref.py train-replay --run tiny-run | tr -d '\r' | cmp - tiny-run/result.txt
$exe ai-train tiny-run.sieve --threads 1 --quiet | tr -d '\r' | q "^reproduced exactly"
python -c "import sys; p='tiny-run/order.txt'; l=open(p).read().split('\n'); l[1],l[5]=l[5],l[1]; open(p,'w').write('\n'.join(l))"
# (It exits with an error, as it should; the step's shell fails a pipe on any error.)
{ $exe ai-train tiny-run --quiet || true; } | tr -d '\r' | q "^NOT reproduced"
$exe ai-train tests/training_run_v1 --quiet | tr -d '\r' | q "^reproduced exactly"
python reference/sieve_ref.py train-replay --run tests/training_run_v1 | tr -d '\r' | cmp - tests/training_run_v1/result.txt
$exe ai-train data/training/universalbibliothek.sieve --quiet | tr -d '\r' | q "^reproduced exactly"
# training-v2: a loss scale of 2^5, the oracle training it again to the same result.
$exe ai-train --record tiny-corp --out run-v2 --ai-width 8 --ai-heads 2 --ai-bits 3 --window 17 --epochs 2 --checkpoint-every 3 --loss-scale-bits 5 --learning-rate 0.0005 --quiet > /dev/null
q "^version = training-v2" run-v2/training.ini
python reference/sieve_ref.py train-replay --run run-v2 | tr -d '\r' | cmp - run-v2/result.txt
# Stopped part-way twice and carried on (--resume), a run ends exactly as one never stopped.
$exe ai-train --record tiny-corp --out run-stop --ai-width 8 --ai-heads 2 --ai-bits 3 --window 17 --epochs 3 --checkpoint-every 4 --stop-after 7 --quiet | tr -d '\r' | q "^stopped at step 7"
[ -f run-stop.resume ]
$exe ai-train run-stop --resume --stop-after 13 --quiet | tr -d '\r' | q "^stopped at step 13"
$exe ai-train run-stop --resume --quiet | tr -d '\r' | q "^result written"
[ ! -f run-stop.resume ]
python reference/sieve_ref.py train-replay --run run-stop | tr -d '\r' | cmp - run-stop/result.txt
# diffusion-v1 (sieve diffuse): a run whose pictures are drawn from the image line's stack, trained,
# checked again by the tool, and trained again by the oracle to the same result.txt; then pictures made.
printf '[image]\nmode = compact\nfilters = figure-v1\n[image.figure-v1]\nconnect = four\nborder = any\n' > figs.ini
$exe diffuse --record diff-run --filters figs.ini --width 4 --height 3 --figures 40 --ai-width 8 --ai-heads 2 --ai-bits 3 --steps 8 --checkpoint-every 3 --levels 5 --batch 3 --learning-rate 0.01 --quiet
$exe diffuse diff-run --quiet | tr -d '\r' | q "^reproduced exactly"
python reference/sieve_ref.py diffuse-replay --run diff-run | tr -d '\r' | cmp - diff-run/result.txt
$exe diffuse --make diff-run --count 2 | tr -d '\r' | q "^picture "
# The same on the audio line: a melody run (notes104, 6 events), its melodies drawn from the audio
# line's stack, trained, trained again, and trained by the oracle to the same result.txt. The
# cells are 104 symbols, so pictures.txt holds numbers apart by spaces.
printf '[audio]\nmode = compact\nfilters = key-data-v2, melody-ending-v2\n' > mels.ini
$exe diffuse --record mel-run --line audio --length 6 --filters mels.ini --figures 40 --ai-width 8 --ai-heads 2 --ai-bits 3 --steps 8 --checkpoint-every 3 --levels 5 --batch 3 --learning-rate 0.01 --quiet
q "^line = audio" mel-run/diffusion.ini
$exe diffuse mel-run --quiet | tr -d '\r' | q "^reproduced exactly"
python reference/sieve_ref.py diffuse-replay --run mel-run | tr -d '\r' | cmp - mel-run/result.txt
$exe diffuse --make mel-run --count 2 | tr -d '\r' | q "^melody "
# --judge counts what a model makes, and the noise it started from, against a stack.
$exe diffuse --make mel-run --count 20 --judge mels.ini | tr -d '\r' | q "^  the stack  *made [0-9]* of 20, noise [0-9]* of 20"
# The shipped picture model (data/training/figures-2x32): its final model makes the six pictures
# the harness shows, address for address (training it again takes minutes: run it by hand).
$exe diffuse --make data/training/figures-2x32 --count 6 | tr -d '\r' | grep "^picture" | cut -c10-34 | tr '\n' ' ' | q "fff0fc2f0de8f01c3fdbe6fff fff29e4e04fbf01b3e05e9fff fff8ffee10bcf8f80f05c0fff fff83e1f15cef83a1f8de7fff fff93e3713a17f7e2fa3c9fff fff878af09e16a7e0f03c3fff"
# The shipped melody model (data/training/melodies-2x64): its first three melodies, by address.
$exe diffuse --make data/training/melodies-2x64 --count 3 | tr -d '\r' | grep "^melody" | cut -c10-36 | tr '\n' ' ' | q "8ac6f76023e45b65ccb9d8e98b4 02dd451f986f45126c630368e9c 5754e9fcddb9cedeba2ddd88f64"
# The shipped characters model (data/training/type-2x32, IDEAS §19.73): its first six pictures, by
# address, and its training pictures, written again from the font, byte for byte.
$exe diffuse --make data/training/type-2x32 --count 6 | tr -d '\r' | grep "^picture" | cut -c10-34 | tr '\n' ' ' | q "fbf7fffdff6edbc77ffffffff fef7d66aacffb2db7e7ffffff fffbf7fe7c3efcffffb5f3fff fbfebf7bf5deffea7dddfffff fdffff3fffb17fff6ceffffff 7adfebeff9352bffcbfffffff"
python tools/build_character_pictures.py | cmp - data/training/type-2x32/pictures.txt
# Its training pictures are no hand-made list: drawn again from the image line's stack (figure-v1
# connect four, row-runs-v1 changes 2, seed 42), they are pictures.txt byte for byte.
printf '[image]\nmode = compact\nfilters = figure-v1, row-runs-v1\n[image.figure-v1]\nconnect = four\n[image.row-runs-v1]\nchanges = 2\n' > figures.ini
$exe diffuse --record figs-again --filters figures.ini --ai-layers 2 --ai-width 32 --steps 1 --quiet
cmp figs-again/pictures.txt data/training/figures-2x32/pictures.txt
# The AI line's filters (SPECIFICATIONS §12.0f): weight-rows-v1 counts and ranks; the model the
# shipped run trains passes it, and so does its canonical form (canonical-model-v1), which the
# oracle works out the same and which says what the model says.
printf '[ai]\nmode = compact\nfilters = weight-rows-v1\n' > ai-rows.ini
$exe filters --line ai --filters ai-rows.ini | tr -d '\r' | q "^survivors    2\^32768.00 (exact; compact mode available)"
# The two that cannot be counted are marked judge-only (their own tab in the filters window).
$exe filters --line ai | tr -d '\r' | q "^\[ \] canonical-model-v1   (hard, judge-only)"
$exe filters --line ai | tr -d '\r' | q "^\[ \] language-v1   (soft, judge-only)"
$exe filters --line ai --filters ai-rows.ini | tr -d '\r' | q "keeps 10\^-145.65 of the line (exact; it judges)"
$exe ai-train data/training/universalbibliothek.sieve --address-out ub.hex --quiet > /dev/null
$exe ai --address-file ub.hex --filters ai-rows.ini --temperature 0 | tr -d '\r' | tee ub-ai.txt | q "weight-rows-v1 \[x\] pass, canonical-model-v1 \[ \] FAIL"
q "^compact      positional " ub-ai.txt
$exe ai --address-file ub.hex --filters ai-rows.ini --temperature 0 --canonical --address-out ubc.hex | tr -d '\r' | tee ubc-ai.txt | q "weight-rows-v1 \[x\] pass, canonical-model-v1 \[ \] pass"
[ "$(grep '^says' ub-ai.txt)" = "$(grep '^says' ubc-ai.txt)" ]
python reference/sieve_ref.py ai-canonical --address-file ub.hex --rows | tr -d '\r' > ubc-ref.txt
head -1 ubc-ref.txt | cmp - ubc.hex
q "^weight-rows-v1 pass" ubc-ref.txt
# Runs named by the AI line's filters (the shipped run, from data/training): near-model-v1 keeps
# the models within two digits of its model, counted exactly and in compact order; language-v1
# scores a model on its text, exactly. Its model passes both, a model from the line's middle
# neither, and the oracle's vectors agree.
printf '[ai]\nmode = compact\nfilters = near-model-v1, language-v1\n' > ai-run.ini
printf '[ai]\nmode = compact\nfilters = near-model-v1\n' > ai-near.ini
$exe filters --line ai --filters ai-near.ini --exact | tr -d '\r' | q "^survivors    2\^32.81 (exact; compact mode available): 7548948481$"
$exe ai --address-file ub.hex --filters ai-run.ini --temperature 0 | tr -d '\r' | tee ub-run.txt | q "language-v1 \[x\] pass, near-model-v1 \[x\] pass"
q "^layer        0 (digits from the model universalbibliothek trained)" ub-run.txt
q "^speaks       universalbibliothek (corpus/universalbibliothek.txt): 275.71 thousandths" ub-run.txt
$exe ai --bearing 180 --filters ai-run.ini --temperature 0 | tr -d '\r' | q "language-v1 \[x\] FAIL, near-model-v1 \[x\] FAIL"
# And the same line in the hallway: a sixth door, the cube in hand, green on clay.
H=build/hallway
[ -f build/Release/hallway.exe ] && H=build/Release/hallway.exe
$H --no-menu --line models --walk "0,0" --screenshot models.png | sed -n 1p | q "^MODELS line, positional"
out=$($H --no-menu --line models --goto "$a" --walk "0,0" --take --screenshot cube.png)
echo "$out"
echo "$out" | q "8/8 vertices used"
$H --no-menu --line movies --pose "-1.7,7.0,0,0" --walk "-1,0" --screenshot door6.png | sed -n 1p | q "^MODELS line"
# The crates render their models onto their faces, and the cache has a budget.
$H --no-menu --line models --model-cache 8 --walk "0,0" --bench 12 --screenshot crates.png 2>&1 | q "^bench: 12 frames"
# The display cache is a world setting, in the setup menu's GLOBAL section (row 8),
# in steps of 8 MB, and saved with the application's settings. From a settings file that
# does not exist yet, so it starts at its default.
rm -f mc.ini
downs=$(python -c "print(','.join(['Down']*8))")
$H --menu --settings mc.ini --press "$downs,Right,Right" --screenshot mc.png > /dev/null
grep -q "^model_cache_mb = 80" mc.ini
# The settings panel slides out with Tab (the map takes the width), and back with a move.
$H --menu --press "Tab" --screenshot panel-out.png | q "saved panel-out.png"
$H --menu --press "Tab,Down" --screenshot panel-back.png | q "saved panel-back.png"
# The filter memory (GLOBAL row 11): what one count's tables may take, in steps of 256 MB
# from 512, saved with the application's settings.
rm -f fm.ini
downs=$(python -c "print(','.join(['Down']*11))")
$H --menu --settings fm.ini --press "$downs,Right" --screenshot fm.png > /dev/null
grep -q "^filter_memory_mb = 768" fm.ini
# The counting memory and the merge cache, GLOBAL rows 12 and 13: percentages, 5 a
# step from 50, saved with the application's settings.
rm -f cm.ini
downs=$(python -c "print(','.join(['Down']*12))")
$H --menu --settings cm.ini --press "$downs,Right,Down,Left,Left" --screenshot cm.png > /dev/null
grep -q "^counting_memory_pct = 55" cm.ini
grep -q "^merge_cache_pct = 40" cm.ini
# The time budget, GLOBAL row 14: 5 ms a step from 50, saved with the application's
# settings; the menu says it is waiting for X.
rm -f tb.ini
downs=$(python -c "print(','.join(['Down']*14))")
$H --menu --settings tb.ini --press "$downs,Right,Right" --screenshot tb.png > /dev/null
grep -q "^unit_time_ms = 60" tb.ini
# The item memory, the last GLOBAL row (15): percent of installed memory, 5 a step from 25,
# saved with the application's settings.
rm -f im.ini
downs=$(python -c "print(','.join(['Down']*15))")
$H --menu --settings im.ini --press "$downs,Right" --screenshot im.png > /dev/null
grep -q "^item_memory_pct = 30" im.ini
# ...and Graphics Memory, which the setup menu's budget keeps it within, is a graphics
# setting: Settings, Graphics, then the seventh row.
rm -f gm.ini
$H --main-menu --settings gm.ini --press "Down,Down,Down,Down,Down,Return,Return,Down,Down,Down,Down,Down,Down,Right" --screenshot gm.png > /dev/null
grep -q "^graphics_memory_gb = 5" gm.ini
# sieve-hallway.ini holds every setting there is, the setup menu's and ffmpeg's among
# them, each with a comment; and written from the defaults, then written again from that
# file, it comes out the same, byte for byte.
rm -f defaults.ini again.ini
$H --write-settings defaults.ini | q "wrote defaults.ini"
$H --settings defaults.ini --write-settings again.ini > /dev/null
cmp defaults.ini again.ini
for k in "^\[setup\]" "^\[paths\]" "^ffmpeg = *;" "^binary_length = 32 " "^ai_width = 16 " "^items_per_wall = 128 " "^title_length = 32 " "^\[locator\]" "^search_seconds = 0 " "^search_for = file "; do grep -q "$k" defaults.ini; done
if grep -v "^;" defaults.ini | grep -v "^\[" | grep -v "^$" | grep -v " ; "; then exit 1; fi
# An older file gains the entries it lacks and keeps its own values; a value a setting
# cannot have leaves it at its default.
printf '[world]\nunit_time_ms = 70\n[setup]\nlength = 64\nalphabet = nonsense\n[paths]\nffmpeg = /opt/ff/ffmpeg\n[locator]\nsearch_seconds = 2.5\nsearch_for = run\n' > older.ini
$H --settings older.ini --write-settings newer.ini > /dev/null
for k in "^unit_time_ms = 70 " "^length = 64 " "^alphabet = lower27 " "^ffmpeg = /opt/ff/ffmpeg " "^vsync = on " "^search_seconds = 2.5 " "^search_for = run "; do grep -q "$k" newer.ini; done
# The setup menu saves its own settings there too: the first row, the line to start on.
rm -f setup.ini
$H --menu --settings setup.ini --press "Right" --screenshot setup.png > /dev/null
grep -q "^line = " setup.ini
if grep -q "^line = text " setup.ini; then exit 1; fi
# A different shape is a different line, and the setup menu draws all six of them.
$exe mesh --vertices 12 --faces 20 --coords 64 | tr -d '\r' | q "^space        models/V12/F20/C64/key=sieve/modelspace-v1"
$H --no-menu --line models --vertices 12 --faces 20 --coords 64 --walk "0,0" --screenshot shape.png | sed -n 1p | q "^MODELS line"
$H --menu --vertices 12 --faces 20 --coords 64 --screenshot map6.png | q "saved map6.png"
# The setup menu's last three rows: find my limits, reset, and the way in. Finding the
# limits changes every shape; resetting puts them all back.
# Up from the first row wraps to the last, so three Ups land on find-my-limits whatever
# else is in the list: the last three rows are always find-my-limits, reset, the way in.
toend="Up,Up,Up"
# Finding the limits changes the shapes; resetting puts them back exactly as they were. The
# settings each run saves are compared, not screenshots: the menu also shows how long a
# unit took to open, measured each time, which a busy machine changes. (The window's size
# is saved too, once the hallway has opened.)
rm -f lim.ini res.ini dflt.ini
$H --write-settings dflt.ini > /dev/null
$H --menu --settings lim.ini --press "$toend,Return" --screenshot lim.png > /dev/null
$H --menu --settings res.ini --press "$toend,Return,Down,Return,Up" --screenshot res.png > /dev/null
if diff <(grep -v '^resolution =' dflt.ini) <(grep -v '^resolution =' lim.ini) > /dev/null; then exit 1; fi
awk '$1 == "ai_layers" { found = 1; if ($3 < 1 || $3 > 64) exit 1 } END { if (!found) exit 1 }' lim.ini # aispace-v1's 1 to 64, on any machine
diff <(grep -v '^resolution =' dflt.ini) <(grep -v '^resolution =' res.ini) > /dev/null
# The setup menu's rows are numbered, and F on a row opens that line's filters, so the
# mapping has to be pinned or a row inserted anywhere above silently opens the wrong
# line's list. Each row here ticks a mode and the saved file says which line it was.
# Closing the overlay is what writes it, which also covers that path.
# The lines' rows follow the doors (image, pages, books, audio, tracks, video, movies,
# models, worlds, AI, binary); the audio rows are six: its notes, its note set, and notes2's
# range, durations and voices. (AI has no filters yet, so F on its rows opens none.)
for pair in "16 image" "19 text" "23 books" "24 audio" "27 audio" "30 tracks" "31 video" "35 movies" "36 models" "39 worlds" "47 binary"; do
  set -- $pair
  rm -f row$1.ini
  downs=$(python -c "print(','.join(['Down']*$1))")
  $H --menu --filters row$1.ini --press "$downs,F,Space,Escape" --screenshot row$1.png > /dev/null
  grep -A1 "^\[$2\]" row$1.ini | q "mode = mark"
done
# On the rows that belong to no line, F opens the filters of the line you start on: pages
# by default, and models once the "line" row (the first) is turned to models.
rm -f start.ini start2.ini
$H --menu --filters start.ini --press "F,Space,Escape" --screenshot start.png > /dev/null
grep -A1 "^\[text\]" start.ini | q "mode = mark"
$H --menu --filters start2.ini --press "Right,Right,Right,Right,Right,Right,F,Space,Escape" --screenshot start2.png > /dev/null
grep -A1 "^\[models\]" start2.ini | q "mode = mark"
# Items per wall: the corridor's tile size is a setting. It changes no address -- the
# same unit, at the same percentage along the same line -- only the tile and slot that
# name where it stands, and at 256 a tile the tile number is half what it was. Untitled
# (title length 0), so the starting page stands well along the line, not at its start.
a=$($H --no-menu --title-length 0 --walk "0,0" --screenshot t128.png | sed -n 1p)
b=$($H --no-menu --title-length 0 --items-per-wall 256 --walk "0,0" --screenshot t256.png | sed -n 1p)
echo "$a"; echo "$b"
echo "$a" | q "85.9326950016% along"
echo "$b" | q "85.9326950016% along"
# ...and it is the same book, with the same address, on both.
test "$(echo "$a" | book)" = "$(echo "$b" | book)"
if [ "$(echo "$a" | sed -n 's/.*tile \([0-9.]*\).*/\1/p')" = "$(echo "$b" | sed -n 's/.*tile \([0-9.]*\).*/\1/p')" ]; then exit 1; fi
# What stands in a slot is scaled to the slot, so twice as many to a tile is half-size
# items rather than items running into each other: an item must be narrower than the
# pitch at every size, or the crosshair cannot tell two of them apart.
python -c "shelf=6.0; base=shelf/16; bad=[n for n in (128,256,512,1024) if not 0.28*min(1.0,(shelf/(n//8))/base) < shelf/(n//8)]; assert not bad, bad; print('slots fit at every tile size')"
$H --no-menu --items-per-wall 256 --take --screenshot take256.png | q "saved take256.png"
# The pictures on the crate faces have a size of their own, and it is thrown away and
# redrawn when it changes rather than being stretched.
$H --no-menu --line models --model-tile 128 --walk "0,0" --bench 8 --screenshot tile128.png 2>&1 | q "^bench: 8 frames"
test -s t128.png && test -s t256.png && test -s tile128.png
# The item page has two tabs: the thing itself, and what it costs to name it. On a text
# line the guided row is shorter than the other two exactly when the unit reads as text.
$H --no-menu --warp "it was the best of times" --press "E,C" --screenshot cost.png > /dev/null
test -s cost.png
# HELP, after ITEM, COST and SORT on every line: the keys for what is in hand.
$H --no-menu --warp "it was the best of times" --press "C,C,C" --screenshot help.png > /dev/null
test -s help.png
# The compass: the degree readout beside the percentage, to the precision set in Graphics.
$H --no-menu --line pages --goto "25%" --walk "0,0" --screenshot comp.png | sed -n 1p | q "25.0000000000% along"
test -s comp.png
# The binary line: one wall of empty shelves, and on the other side the edge, the short
# wall and the rain falling off it. It draws with whatever font is present, so it works
# with Sieve's own small one.
python -c "import ast,sys; ast.parse(open('tools/fetch_unifont.py').read())"
$H --no-menu --line binary --pose "0,1.7,4,0" --bench 8 --screenshot edge.png 2>&1 | q "^bench: 8 frames"
# It is a line like the others now: every file up to its length, one loop (its filters,
# by the files' kinds, are checked with the other filters below).
$H --no-menu --line binary --walk "0,0" --screenshot bin.png | sed -n 1p | q "^BINARY line"
# The lines no longer loop into one another: they start and finish at binary.
# Left out of models is worlds, left out of worlds is AI, and left out of AI is binary, where
# you are turned round to come out of its one door (in its left wall); back out through that
# door is AI again. Right out of image is binary too, and there the door leads back to image.
$H --no-menu --line models --pose "-1.7,7.0,0,0" --walk "-1,0" --screenshot door7m.png | sed -n 1p | q "^WORLDS line"
$H --no-menu --line worlds --pose "-1.7,7.0,0,0" --walk "-1,0" --screenshot door7w.png | sed -n 1p | q "^AI line"
$H --no-menu --line ai --pose "-1.7,7.0,0,0" --walk "-1,0" --screenshot door7.png | sed -n 1p | q "^BINARY line"
# (A walk is in the walker's frame, so after being turned round, 4.2 is back the way you came.)
$H --no-menu --line ai --pose "-1.7,7.0,0,0" --walk "-1,0;4.2,0" --screenshot door8.png | sed -n 2p | q "^AI line"
$H --no-menu --line image --pose "1.7,7.0,0,0" --walk "1,0" --screenshot door9.png | sed -n 1p | q "^BINARY line"
$H --no-menu --line image --pose "1.7,7.0,0,0" --walk "1,0;-1,0" --screenshot door10.png | sed -n 2p | q "^IMAGE line"
# Binary's other side is the drop, always on its right: the short wall stops you at the
# edge of the floor. No door, no crossing, whatever the doorway is doing at that z.
$H --no-menu --line ai --pose "-1.7,7.0,0,0" --walk "-1,0;-9,0" --screenshot walk.png | sed -n 2p | q "^BINARY line.*x 1.650000"
# There is no setting for it: not in Settings > Graphics, not in a settings file that
# Graphics has just written, and not on the command line.
rm -f be.ini
$H --main-menu --settings be.ini --press "Down,Down,Down,Down,Down,Return,Return,Down,Down,Down,Return" --screenshot be.png > /dev/null
grep -q "^edge_glow = on" be.ini
if grep -Eq "binary[_-]edge" be.ini; then exit 1; fi
if $H --help 2>&1 | grep -q "binary-edge"; then exit 1; fi
test -s walk.png
test -s edge.png
test -s models.png && test -s cube.png && test -s crates.png && test -s map6.png
if $exe mesh --coords 12 > /dev/null 2>&1; then exit 1; fi
