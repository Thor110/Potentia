#!/usr/bin/env bash
# Sieve CI, step 10: Hallway renders, and its doors keep the corridor position (Linux, no display).
# Run by .github/workflows/build.yml (on Linux only), or alone or with the
# others by ci/run.sh (ci/README.md), from the Sieve folder with the Release build in build/.
# only: Linux
set -eo pipefail
cd "$(dirname "$0")/../.."
. ci/lib.sh
sieve_ci_no_display # SDL with no screen, as the workflow sets it

trap 'echo "::error::the step stopped at its line $LINENO: $BASH_COMMAND" >&2' ERR # names the failing line
# q PATTERN: like grep -q, but reads all its input, so the command before it never
# gets SIGPIPE (the steps run with pipefail).
q() { grep -c "$@" > /dev/null; }
H=build/hallway
W="it was the best of times"
book() { grep -o 'first book [0-9a-f.]*' | tail -1; }
$H --screenshot hallway.png --size 640x360
test -s hallway.png
# The metrics list (hallway --metrics, no window): every line under the reference filters,
# as the setup menu counts them; the binary line's binary-kind-v1 removes 99.9954%.
$H --metrics metrics.txt --filters data/metrics/reference-filters.ini > /dev/null
q "^Binary : 99.9954%" metrics.txt
q "^Models : 99.999999999999...% *( Kept : 10^-21.47 )" metrics.txt
[ "$(grep -c ' : .*Kept : ' metrics.txt)" = 11 ]
# ...and what each ordering saves: nothing in positional or scrambled order, what the
# filters remove off every survivor in compact order, and the guided ordering's bound and
# what it did on the reference text (Dickens, which the model was not trained on).
q "^Positional and scrambled : 0 bits, on every line" metrics.txt
q "^Binary : compact [0-9.]* bits off every survivor" metrics.txt
q "^Pages  : guided: no minimum" metrics.txt
q "^Pages  : guided on reference-text.txt (SHA-256 [0-9a-f]*\.\.\., [0-9]* pages): [0-9.]* bits a page" metrics.txt
# The model Sieve trained, against its own file and the compressors (whose sizes depend on
# the zlib and liblzma linked, so only the address and the file are pinned).
q "^universalbibliothek (ai/L1/H16/A2/F64/B4/V256/key=sieve/aispace-v1" metrics.txt
q "^  trained: address 32,768; file 272,576 " metrics.txt
q "^  random : address 32,768; file 272,576 " metrics.txt
here=$($H --warp "$W" --walk "0,0" --screenshot a.png | book)
next=$($H --warp "$W" --tile 1 --walk "0,0" --screenshot b.png | book)
# Through a door and straight back: exactly where you were.
out=$($H --warp "$W" --pose 0,7,-90,0 --walk "-2.3,0;2.5,0" --screenshot door.png)
echo "$out"
echo "$out" | sed -n 1p | q "through the door into the BOOKS line"
test "$(echo "$out" | tail -2 | book)" = "$here"
# A door keeps your angle: 36.25% along pages is 36.25% along books; one tile along
# there and back through the door is still 36.25% (to the readout's ten places), on pages.
out=$($H --goto "36.25%" --pose 0,7,-90,0 --walk "-2.3,0;-0.8,0;0,8;1.5,0" --screenshot loop.png)
echo "$out"
echo "$out" | sed -n 1p | q "BOOKS line.* 36.2500000000% along"
echo "$out" | tail -2 | sed -n 1p | q "PAGES line.* 36.2500000000% along"
# Guided order: M cycles to it and - zooms out.
$H --warp "$W" --press "M,M,-" --screenshot keys.png | tail -2 | sed -n 1p | q "PAGES line, guided zoom 57"
# The address navigator: X opens the whole address, Home chooses its first digit, Up turns
# it (carrying and wrapping), and ENTER goes there, far from where the walk started.
# Its random buttons: R draws a place on this line, Shift+R a line too; ENTER goes there.
$H --no-menu --press "X,R,Return" --screenshot nav-random.png | tr -d '\r' | tail -2 | sed -n 1p | q "^PAGES line, positional"
$H --no-menu --press "X,Shift+R" --screenshot nav-random-any.png | q "saved nav-random-any.png"
# sieve-install, unattended: the fixture put back from its installer (v4, packed),
# identical; a second time over it is refused (exit 1), as is a damaged installer. Then
# the fixture with a program and a list made from another's lines in it, packed, too.
build/sieve locate tests/manifest_fixture --installer fx.sieve --v4 > /dev/null
rm -rf fx_installed
build/sieve-install fx.sieve --to fx_installed --yes --screenshot installer.bmp
diff -r tests/manifest_fixture fx_installed
test -s installer.bmp
if build/sieve-install fx.sieve --to fx_installed --yes; then exit 1; fi
head -c 100 fx.sieve > broken.sieve
if build/sieve-install broken.sieve --to fx_broken --yes; then exit 1; fi
test ! -e fx_broken
rm -rf fx4 fx4_installed && cp -r tests/manifest_fixture fx4 && cp build/sieve fx4/program
seq 1 20000 > fx4/all.txt && awk 'NR % 3 == 0' fx4/all.txt > fx4/third.txt
build/sieve locate fx4 --installer fx4.sieve --v4 2>&1 | grep -c "1 derived from another's lines" > /dev/null
build/sieve-install fx4.sieve --to fx4_installed --yes
diff -r fx4 fx4_installed
# An installer carrying one 7z: sieve-install unpacks it into a folder named after the file,
# its one top folder left out, so the fixture comes back as it was; the locator's own
# install gives back the archive itself, byte for byte.
build/sieve locate tests/manifest_fixture.7z --installer fx7.sieve --v4 > /dev/null
rm -rf fx7 fx7raw
build/sieve-install fx7.sieve --to fx7 --yes
diff -r tests/manifest_fixture fx7
build/sieve install fx7.sieve --to fx7raw > /dev/null
cmp tests/manifest_fixture.7z fx7raw/manifest_fixture.7z
# Archives of other kinds, extracted as an archiver extracts them (the right-click menu's Extract on
# a .zip, .rar, .tar.gz ...), one layer at a time; each made from the repository's own files.
python ci/extract_check.py build/sieve-install extract_check
# A release's layout: sieve and sieve-install in tools/ below the data folders. sieve there
# finds the dictionaries and models in the folder above, and still makes installer
# programs from the sieve-install beside it.
rm -rf layout && mkdir -p layout/tools
cp build/sieve build/sieve-install layout/tools/ && cp -r build/dictionaries build/models layout/
(cd layout/tools && ./sieve dicts | grep -q "layout/dictionaries/dictionaries.tsv")
(cd layout/tools && ./sieve models | grep -q "layout/models/models.tsv")
layout/tools/sieve locate tests/manifest_fixture --program layout-installer --v4 > /dev/null
rm -rf fx_layout && ./layout-installer --to fx_layout --yes && diff -r tests/manifest_fixture fx_layout
# An installer program: sieve-install with the installer attached, run on its own, and
# read by sieve install; exactly the program, the installer and a 24-byte trailer; and
# made from a program that already has one attached, the old one replaced, not added to.
build/sieve locate tests/manifest_fixture --program fx-installer --v4 > /dev/null
rm -rf fx_prog fx_prog2
./fx-installer --to fx_prog --yes
diff -r tests/manifest_fixture fx_prog
build/sieve install fx-installer --to fx_prog2 > /dev/null
diff -r tests/manifest_fixture fx_prog2
# The File Locator's Install from an installer, in the hallway: the same folder again.
rm -rf fx_gui
$H --no-menu --install fx.sieve --install-to fx_gui --screenshot locinstall.png | q "saved locinstall.png"
diff -r tests/manifest_fixture fx_gui
test "$(wc -c < fx-installer)" -eq "$(( $(wc -c < build/sieve-install) + $(wc -c < fx.sieve) + 24 ))"
rm -rf again && mkdir again && cp build/sieve again/ && cp fx-installer again/sieve-install
again/sieve locate tests/manifest_fixture --program fx-again --v4 > /dev/null
cmp fx-installer fx-again
# The File Locator (pause menu): a folder measured, and a file walked to on the binary line.
$H --no-menu --locate tests/manifest_fixture --screenshot locate.png | q "saved locate.png"
$H --no-menu --binary-length 64 --locate tests/manifest_fixture/a.txt --press "Return" --screenshot locate2.png | tail -2 | sed -n 1p | tee l2.txt | q "^BINARY line"
# Its kind, read from its first bytes.
grep -q " TXT " l2.txt
# Worlds in the hallway: the world at an address the tool composed, saved with F, is the
# tool's .obj of it, byte for byte; the setup menu draws the WORLDS rows and column.
wa=$(build/sieve world --compose "1:1.2.3.4|2:7.0.7.23" --title "a small world" | tr -d '\r' | awk '$1 == "positional" {print $2}')
$H --no-menu --line worlds --goto "$wa" --save-item hw.obj --screenshot hw.png > /dev/null
build/sieve world --read "$wa" --out hw-ref.obj > /dev/null
cmp hw.obj hw-ref.obj
$H --no-menu --line worlds --walk "0,0" --screenshot worlds.png | sed -n 1p | q "^WORLDS line, positional"
# The AI line in the hallway: the model at a bearing, saved with F, is the tool's file of it,
# byte for byte; talked to, it answers; the setup menu draws the AI rows and its broken bar.
build/sieve ai --bearing 137 --mode scrambled --out ai137 > /dev/null
aa=$(build/sieve ai --warp ai137 --address-out ai137.hex --mode scrambled > /dev/null; tr -d '\r\n' < ai137.hex)
$H --no-menu --line ai --mode scrambled --goto "$aa" --save-item hai.safetensors --screenshot hai.png > /dev/null
cmp hai.safetensors ai137/model.safetensors
$H --no-menu --line ai --walk "0,0" --take --talk "Hello there" --screenshot ai-talk.png | tr -d '\r' | tee ai-talk.txt | sed -n 1p | q "^AI line, positional"
q "^you: Hello there" ai-talk.txt
q "^it: " ai-talk.txt
# The chat window (IDEAS §19.13): a file given to the model, two figures drawn with the shipped
# picture model (its pictures 1 and 2, as `sieve diffuse --make` makes them), and something
# said; GO TO IT on a figure holds it on the image line, J on a text file opens it on PAGES.
$H --no-menu --line ai --walk "0,0" --take --chat --chat-attach tests/training_corpus.txt --chat-draw 2 --talk "Hello there" --training-dir data/training --screenshot chat.png | tr -d '\r' > chat.txt
q "^file: training_corpus.txt (418 bytes)" chat.txt
q "^figure 1 by figures-2x32: fff0fc2f0de8f01c3fdbe6fff" chat.txt
q "^figure 2 by figures-2x32: fff29e4e04fbf01b3e05e9fff" chat.txt
q "^you: Hello there" chat.txt
# Cards show what a file is: a picture's thumbnail (the vault looking at it too), a text's first
# lines (one cut mid-character among them), anything else its first bytes.
head -c 30 data/anchors/public-domain/universalbibliothek.txt > cut.txt && printf '\303' >> cut.txt
$H --no-menu --line ai --walk "0,0" --take --chat --chat-attach "data/anchors/public-domain/Vitruvian.jpg;cut.txt;data/anchors/public-domain/universalbibliothek.txt;data/anchors/public-domain/hurrian-hymn-6.mid" --training-dir data/training --screenshot chat-cards.png | tr -d '\r' > cards.txt
q "^file: Vitruvian.jpg (207,500 bytes)" cards.txt
q "^file: cut.txt (31 bytes)" cards.txt
q "^file: hurrian-hymn-6.mid (1,377 bytes)" cards.txt
# The conversation saved as text (/log PATH, or Ctrl+L into chat-logs): the model it was
# with, by its address, then who said what, each card a line.
rm -f chatlog.txt
$H --no-menu --line ai --walk "0,0" --take --chat --chat-attach tests/training_corpus.txt --talk "/log chatlog.txt" --training-dir data/training --screenshot chat-log.png > /dev/null
q "^Sieve .* chat log, " chatlog.txt
q "^model in hand, its address ([0-9]* hex digits): [0-9a-f]" chatlog.txt
q "^  \[file\] training_corpus.txt, TXT, " chatlog.txt
q "^you: /log chatlog.txt" chatlog.txt
# CHAT with a model file (the setup menu's AI rows; here --text-model): the tiny model checked
# against the oracle above answers, given a file with the message; and DRAW with a chosen picture
# model. The setup menu's rows save them in sieve-hallway.ini's [models].
$H --no-menu --line ai --walk "0,0" --take --chat --text-model tinymodel --chat-attach tests/training_corpus.txt --talk "Hello there" --training-dir data/training --screenshot chat-file.png | tr -d '\r' > chatfile.txt
q "^you: Hello there" chatfile.txt
q "^it: " chatfile.txt
# What a model given as a file is told (docs/SYSTEM-PROMPT.md): sieve-system-v1 word for word
# (the tiny model has too few positions to be given it, and is told the short one), then the place.
$H --no-menu --line ai --walk "0,0" --take --chat --text-model tinymodel --print-system-prompt --training-dir data/training --screenshot prompt.png | tr -d '\r' > prompt.txt
sed -n '/^You are inside the Sieve of Potentia/,$p' prompt.txt | head -n "$(wc -l < data/prompts/sieve-system-v1.txt)" | cmp - data/prompts/sieve-system-v1.txt
q "^- Line: the AI line (ai/L1/H16/A2/" prompt.txt
q "^- You: the model in the folder tinymodel, given as a file$" prompt.txt
# CHAIN (SPECIFICATIONS §12.0h): the tab edited by keys and saved as sieve-chain-v1, and chains run
# with /chain. Pictures and melodies are integer arithmetic, so a chain of them makes the same
# units everywhere; with text models (floating point) each stage is checked to have run.
# The training folder's models, in the tab's order: hand, universalbibliothek, figures-2x32,
# melodies-2x64, type-2x32; the default chain is the text run, then every picture and melody
# model. Down, N, then Right three times: a copy of figures-2x32 stepped round to hand.
$H --no-menu --line ai --walk "0,0" --take --chain --chain-press "Down,N,Right,Right,Right" --chain-save chain1.chain --training-dir data/training --screenshot chain-tab.png | q "saved chain-tab.png"
printf 'sieve-chain-v1\nrun universalbibliothek\ndraw figures-2x32\nhand\ndraw melodies-2x64\ndraw type-2x32\n' | cmp - chain1.chain
$H --no-menu --line ai --walk "0,0" --take --chat --chain-file chain1.chain --talk "/chain the library of babel" --training-dir data/training --screenshot chain-run.png | tr -d '\r' > chainrun.txt
for n in "1/5, text run universalbibliothek: " "2/5, figures-2x32: " "3/5, the model in hand: " "4/5, melodies-2x64: " "5/5, type-2x32: "; do q "^chain $n" chainrun.txt; done
printf 'sieve-chain-v1\ndraw figures-2x32\ndraw melodies-2x64\ndraw figures-2x32\n' > pm.chain
$H --no-menu --line ai --walk "0,0" --take --chat --chain-file pm.chain --talk "/chain hello" --training-dir data/training --screenshot chain-pm.png | tr -d '\r' > chainpm.txt
q "^chain 1/3, figures-2x32: fff83c0780e2f83ec70b81fff$" chainpm.txt
q "^chain 2/3, melodies-2x64: 758454efa87a3b3fcbb9dbf5afc$" chainpm.txt
q "^chain 3/3, figures-2x32: fff4bc570790ba3e5f034afff$" chainpm.txt
$H --no-menu --line ai --walk "0,0" --take --chat --picture-model data/training/figures-2x32 --chat-draw 1 --training-dir data/training --screenshot chat-pm.png | tr -d '\r' | q "^figure 1 by figures-2x32: fff0fc2f0de8f01c3fdbe6fff"
printf '[models]\ntext_model =\npicture_model =\n' > models.ini
downs=$(python -c "print(','.join(['Down']*46))")
# Empty means the first trained run (figures-2x32); Right steps to the next (melodies-2x64), Left back.
$H --menu --settings models.ini --training-dir data/training --press "$downs,Right" --screenshot menu-models.png | q "saved menu-models.png"
q "^picture_model = .*melodies-2x64" models.ini
$H --menu --settings models.ini --training-dir data/training --press "$downs,Left" --screenshot menu-models.png | q "saved menu-models.png"
q "^picture_model = .*figures-2x32" models.ini
# Melodies in CHAT: a melody model's (the same addresses as --make), a random melody and picture
# (/random), and GO TO IT on a melody holds it on AUDIO.
$H --no-menu --line ai --walk "0,0" --take --chat --picture-model data/training/melodies-2x64 --chat-draw 1 --talk "/random melody" --training-dir data/training --screenshot chat-mel.png | tr -d '\r' > chatmel.txt
q "^melody 1 by melodies-2x64: 8ac6f76023e45b65ccb9d8e98b4" chatmel.txt
q "^a random melody: " chatmel.txt
$H --no-menu --line ai --walk "0,0" --take --chat --talk "/random picture" --training-dir data/training --screenshot chat-rand.png | tr -d '\r' | q "^a random picture: "
$H --no-menu --line ai --walk "0,0" --take --chat --picture-model data/training/melodies-2x64 --chat-draw 1 --chat-press "Ctrl+G" --training-dir data/training --screenshot chat-mgo.png | tr -d '\r' | q "^AUDIO line, .*on the audio line, in hand"
$H --no-menu --line ai --walk "0,0" --take --chat --chat-draw 1 --chat-press "Ctrl+G" --training-dir data/training --screenshot chat-go.png | tr -d '\r' | q "^IMAGE line, .*the figure, on the image line, in hand"
$H --no-menu --line ai --walk "0,0" --take --chat --chat-attach tests/training_corpus.txt --chat-press "Ctrl+J" --training-dir data/training --screenshot chat-j.png | tr -d '\r' | q "^PAGES line, .*a TXT file, opened on the PAGES line"
downs=$(python -c "print(','.join(['Down']*41))")
$H --menu --press "$downs" --screenshot menu-ai.png | q "saved menu-ai.png"
# The AI Training Harness (main menu): the run shipped with Sieve, trained again, comes out the
# same; a run recorded there from a folder, its second file moved first and read in order, is
# trained and its result written; and GO TO IT stands on the AI line with the model in hand.
$H --harness --training-dir data/training --script "Return" --screenshot harness.png | tr -d '\r' | q "^harness: REPRODUCED EXACTLY"
mkdir -p hruns corp && cp tests/training_corpus.txt corp/a.txt && head -c 150 tests/training_corpus.txt | tr 'a-z' 'b-za' > corp/b.txt
$H --harness --training-dir hruns --script "N,=corp,Return,Down,Shift+Up,M,Return" --screenshot harness-new.png | tr -d '\r' | q "^harness: TRAINED"
[ -f hruns/corp/result.txt ]
q "^reading = in-order" hruns/corp/training.ini
[ "$(grep '^file = ' hruns/corp/training.ini | head -1)" = "file = corpus/b.txt" ]
# Its settings: the width one step up (two heads' worth, 16 to 20), the learning rate
# following it, as suggested (0.008 / 20, rounded down: 0.0002).
mkdir -p hruns2
$H --harness --training-dir hruns2 --script "N,=corp,Return,Tab,Down,Right,Return" --screenshot harness-set.png | tr -d '\r' | q "^harness: TRAINED"
q "^width = 20" hruns2/corp/training.ini
q "^learning-rate = 0.0002" hruns2/corp/training.ini
$H --harness --training-dir hruns --script "g" --screenshot harness-go.png | tr -d '\r' | q "the model the run trained, in hand"
# The AI line's filters in the hallway: compact (weight-rows-v1), its survivors closed up; and
# with canonical-model-v1 hiding the rest, GO TO IT stands on the model's canonical form.
printf '[ai]\nmode = compact\nfilters = weight-rows-v1\n' > ai-rows.ini
printf '[ai]\nmode = hide\nfilters = canonical-model-v1\n' > ai-canon.ini
$H --no-menu --line ai --filters ai-rows.ini --walk "0,0" --screenshot ai-compact.png | tr -d '\r' | sed -n 1p | q "^AI line, positional, filters: 1 compact"
$H --harness --training-dir data/training --filters ai-canon.ini --script "g" --screenshot harness-go-canon.png | tr -d '\r' | q "as its class's canonical form"
$H --menu --line ai --filters ai-rows.ini --settle 200 --press "F" --screenshot menu-ai-filters.png | q "saved menu-ai-filters.png"
$H --menu --line ai --press "F,Down,Left" --settle 100 --screenshot menu-ai-judge-only.png | q "saved menu-ai-judge-only.png"
printf '[ai]\nmode = compact\nfilters = near-model-v1\n' > ai-near.ini
$H --no-menu --line ai --filters ai-near.ini --walk "0,0" --screenshot ai-near.png | tr -d '\r' | sed -n 1p | q "within 2 digits of universalbibliothek's model (7548948481 survivors)"
# From the main menu: the same locator; Go to it builds the hallway and walks to the file.
$H --main-menu --locate tests/manifest_fixture/a.txt --press "Return" --screenshot locate-menu.png | q "file locator (main menu): open, Go to it offered"
# Its search for shorter addresses, given a time for the run (the file's follows), and the
# keys that move them: [ and ] a file, Shift the run.
$H --main-menu --locate tests/manifest_fixture --tailored --search-total 2 --screenshot locate-search.png | q "file locator (main menu)"
$H --main-menu --locate tests/manifest_fixture --press "],Shift+]" --screenshot locate-search-keys.png | q "file locator (main menu)"
# Saved from the File Locator: the instructions made when it was read, written as they
# are; and Stop pressed while it reads, which keeps the best found by then: either way
# they install to the same files.
$H --main-menu --locate tests/manifest_fixture/a.txt --locate-save located.sieve --screenshot locate-save.png | q "wrote Sieve instructions to located.sieve"
rm -rf located_in located_stop
build/sieve install located.sieve --to located_in > /dev/null
cmp located_in/a.txt tests/manifest_fixture/a.txt
$H --main-menu --locate tests/manifest_fixture --stop-after 0.01 --locate-save located-stop.sieve --screenshot locate-stop.png | q "wrote Sieve instructions to located-stop.sieve"
build/sieve install located-stop.sieve --to located_stop > /dev/null
diff -r tests/manifest_fixture located_stop
# Force stop (Stop pressed twice): the pass under way abandoned, and what it ends with
# still installs to the same files.
rm -rf located_force
$H --main-menu --locate tests/manifest_fixture --stop-after 0.005 --force-after 0.01 --locate-save located-force.sieve --screenshot locate-force.png | q "wrote Sieve instructions to located-force.sieve"
build/sieve install located-force.sieve --to located_force > /dev/null
diff -r tests/manifest_fixture located_force
# The demonstration (IDEAS 16.9): every dimension's item at each whole degree, as one
# .sieve, the same on every machine; installing it gives every file back, and a bearing
# typed in the navigator (X, Tab) is the same item again.
$H --no-menu --sample-degrees degrees --screenshot deg.png | tee deg.txt | q "^sample: 3960 items"
$H --no-menu --sample-degrees degrees2 --sample-out degrees2.sieve --screenshot deg2.png > /dev/null
diff -r degrees degrees2
build/sieve install degrees.sieve --to deg_inst > /dev/null
diff -r degrees deg_inst
# The same as sieve-manifest-v5 (--items): the pages, pictures, films and melodies carried as
# their lines and places, made again on installing, every file the same; smaller than v4;
# and sieve-install, the full installer, makes them again too.
build/sieve locate degrees --installer degrees-v5.sieve --items --v5 --manifest degrees-v5.manifest | tr -d '\r' | q "^items     1368 file(s) carried as items"
python reference/sieve_ref.py unpack degrees-v5.manifest --items-from degrees --to deg_oracle5 > /dev/null
diff -r degrees deg_oracle5
[ "$(wc -c < degrees-v5.sieve)" -lt "$(wc -c < degrees2.sieve)" ]
build/sieve install degrees-v5.sieve --to deg_inst5 > /dev/null
diff -r degrees deg_inst5
build/sieve-install degrees-v5.sieve --to deg_inst5b --yes
diff -r degrees deg_inst5b
# sieve-manifest-v6 (--items --v6): each item's place as raw bytes, the places carried raw or as
# LZMA2, whichever is smaller, and a line's items carried as places only where that is smaller
# than its files packed (the sample's 360 near-identical models' places pack to almost nothing
# as LZMA2, so they are carried as places). Smaller than v5; every file back, by sieve install
# and by the oracle.
build/sieve locate degrees --installer degrees-v6.sieve --items --v6 --manifest degrees-v6.manifest | tr -d '\r' | q "^items     1368 file(s) carried as items of Sieve's lines (.* sieve-manifest-v6"
[ "$(wc -c < degrees-v6.sieve)" -lt "$(wc -c < degrees-v5.sieve)" ]
build/sieve install degrees-v6.sieve --to deg_inst6 > /dev/null
diff -r degrees deg_inst6
python reference/sieve_ref.py unpack degrees-v6.manifest --items-from degrees --to deg_oracle6 > /dev/null
diff -r degrees deg_oracle6
build/sieve-install degrees-v6.sieve --to deg_inst6b --yes
diff -r degrees deg_inst6b
# sieve-manifest-v7 (--items now): v6 with what the rest gives left out (every file's own
# SHA-256 among it, the tree's covering them all), items listed "i K path", and the listing
# carried as LZMA2 where that is smaller (the sample's paths repeat themselves). Far smaller
# than v6; every file back, by sieve install, sieve-install and the oracle. --file-hashes keeps every file's own SHA-256, and installs the same.
build/sieve locate degrees --installer degrees-v7.sieve --items --v15 --manifest degrees-v7.manifest | tr -d '\r' > d7.txt
q "^items     1368 file(s) carried as items of Sieve's lines (.* sieve-manifest-v7" d7.txt
q "^listing   [0-9]* bytes, carried as LZMA2 in " d7.txt
[ "$(wc -c < degrees-v7.sieve)" -lt "$(( $(wc -c < degrees-v6.sieve) / 10 ))" ]
build/sieve install degrees-v7.sieve --to deg_inst7 > /dev/null
diff -r degrees deg_inst7
python reference/sieve_ref.py unpack degrees-v7.manifest --items-from degrees --to deg_oracle7 > /dev/null
diff -r degrees deg_oracle7
build/sieve-install degrees-v7.sieve --to deg_inst7b --yes
diff -r degrees deg_inst7b
# sieve-manifest-v16 (--items now): v7's listing with its head in bytes (the streams' sizes,
# the tree's SHA-256 as 32 bytes), the rest of it packed as before: smaller; every file back.
build/sieve locate degrees --installer degrees-v16.sieve --items --v19 --manifest degrees-v16.manifest | tr -d '\r' > d16.txt
q "^items     1368 file(s) carried as items of Sieve's lines (.* sieve-manifest-v16" d16.txt
q "^head      [0-9]* bytes in bytes, the listing [0-9]* as text (sieve-manifest-v16)$" d16.txt
[ "$(wc -c < degrees-v16.sieve)" -lt "$(wc -c < degrees-v7.sieve)" ]
build/sieve install degrees-v16.sieve --to deg_inst16 > /dev/null
diff -r degrees deg_inst16
python reference/sieve_ref.py unpack degrees-v16.manifest --items-from degrees --to deg_oracle16 > /dev/null
diff -r degrees deg_oracle16
# sieve-manifest-v20 (the newest): the body in bytes too, its "line" lines (which hold tabs) kept as
# they are; no larger than v16, and every file back, by sieve install, the oracle and sieve-install.
build/sieve locate degrees --installer degrees-v20.sieve --items --manifest degrees-v20.manifest | tr -d '\r' > d20.txt
q "^head      [0-9]* bytes in bytes, the listing [0-9]* as text (sieve-manifest-v20; its body in bytes too)$" d20.txt
python -c "d = open('degrees-v20.manifest', 'rb').read(); assert d[:6] == b'sieve\x14', d[:6]"
[ "$(wc -c < degrees-v20.sieve)" -le "$(wc -c < degrees-v16.sieve)" ]
build/sieve install degrees-v20.sieve --to deg_inst20 > /dev/null
diff -r degrees deg_inst20
python reference/sieve_ref.py unpack degrees-v20.manifest --items-from degrees --to deg_oracle20 > /dev/null
diff -r degrees deg_oracle20
build/sieve locate degrees --installer degrees-v7h.sieve --items --v15 --file-hashes --manifest degrees-v7h.manifest > /dev/null
[ "$(wc -c < degrees-v7h.sieve)" -gt "$(wc -c < degrees-v7.sieve)" ]
build/sieve install degrees-v7h.sieve --to deg_inst7h > /dev/null
diff -r degrees deg_inst7h
python reference/sieve_ref.py unpack degrees-v7h.manifest --items-from degrees --to deg_oracle7h > /dev/null
diff -r degrees deg_oracle7h
# A model trained by Sieve alone (universalbibliothek): carried as its 4,096 bytes of digits on the
# AI line, not its 34,072-byte file, smaller than the file packed (v4); the oracle makes the
# model's file again from the place itself, as sieve install does.
rm -rf ub_run ub_model ub_one ub_inst ub_oracle && mkdir ub_one
build/sieve install data/training/universalbibliothek.sieve --to ub_run > /dev/null
grep '^final ' ub_run/result.txt | cut -d' ' -f2 > ub_final.hex
build/sieve ai --address-file ub_final.hex --out ub_model > /dev/null
cp ub_model/model.safetensors ub_one/
build/sieve locate ub_one --installer ub-v4.sieve --v4 > /dev/null
build/sieve locate ub_one --installer ub-v6.sieve --items --v6 --manifest ub-v6.manifest | tr -d '\r' | q "^items     1 file(s) carried as items of Sieve's lines (34072 bytes, as 4096 bytes of places"
[ "$(wc -c < ub-v6.sieve)" -lt "$(wc -c < ub-v4.sieve)" ]
build/sieve install ub-v6.sieve --to ub_inst > /dev/null
cmp ub_one/model.safetensors ub_inst/model.safetensors
python reference/sieve_ref.py unpack ub-v6.manifest --to ub_oracle > /dev/null
cmp ub_one/model.safetensors ub_oracle/model.safetensors
# v7: the same model's place coded by its tensors' digit histograms (digit-histogram-v1):
# trained, its digits are not spread as evenly as chance spreads them, so its code, 4,020
# bytes, is shorter than its place, 4,096; and the listing round it shorter (4,253 bytes
# against v6's 4,443; nothing here is compressed, so the sizes are exact). Both v7
# readers (sieve install, the oracle's own decoding) give the model back.
rm -rf ub_inst7 ub_oracle7
build/sieve locate ub_one --installer ub-v7.sieve --items --v15 --manifest ub-v7.manifest | tr -d '\r' | q "^items     1 file(s) carried as items of Sieve's lines (34072 bytes, as 4022 bytes of places (4096 raw; digit-histogram-v1 on 1 line(s))"
[ "$(wc -c < ub-v7.sieve)" -eq 4253 ] && [ "$(wc -c < ub-v6.sieve)" -eq 4443 ]
q "^line 0 digit-histogram-v1 ai/L1/H16/A2/F64/B4/V256/key=sieve/aispace-v1 line=ai$" ub-v7.manifest
build/sieve install ub-v7.sieve --to ub_inst7 > /dev/null
cmp ub_one/model.safetensors ub_inst7/model.safetensors
python reference/sieve_ref.py unpack ub-v7.manifest --to ub_oracle7 > /dev/null
cmp ub_one/model.safetensors ub_oracle7/model.safetensors
# One file on its own: no root line (it is the file's name), 4,257 bytes for this name.
rm -rf ub_named ub_named_inst && mkdir ub_named && cp ub_one/model.safetensors ub_named/sieve-ai-16432c2a3953.safetensors
build/sieve locate ub_named/sieve-ai-16432c2a3953.safetensors --installer ub-file.sieve --items --v15 > /dev/null
[ "$(wc -c < ub-file.sieve)" -eq 4257 ]
build/sieve install ub-file.sieve --to ub_named_inst > /dev/null
cmp ub_one/model.safetensors ub_named_inst/sieve-ai-16432c2a3953.safetensors
# v16 (--items now): the same, its head in bytes: 4,185 bytes (the folder's, 4,176).
rm -rf ub_named16 ub_inst16 ub_oracle16
build/sieve locate ub_named/sieve-ai-16432c2a3953.safetensors --installer ub-file16.sieve --items --v19 > /dev/null
[ "$(wc -c < ub-file16.sieve)" -eq 4185 ]
# v20 (the newest): its body in bytes, 4,182 (its "line" lines four bytes shorter each, its entry too).
build/sieve locate ub_named/sieve-ai-16432c2a3953.safetensors --installer ub-file20.sieve --items > /dev/null
[ "$(wc -c < ub-file20.sieve)" -eq 4182 ]
rm -rf ub_named20 && build/sieve install ub-file20.sieve --to ub_named20 > /dev/null
cmp ub_one/model.safetensors ub_named20/sieve-ai-16432c2a3953.safetensors
build/sieve install ub-file16.sieve --to ub_named16 > /dev/null
cmp ub_one/model.safetensors ub_named16/sieve-ai-16432c2a3953.safetensors
build/sieve locate ub_one --installer ub-v16.sieve --items --v19 --manifest ub-v16.manifest > /dev/null
[ "$(wc -c < ub-v16.sieve)" -eq 4176 ]
build/sieve install ub-v16.sieve --to ub_inst16 > /dev/null
# It says how long decoding took, and checking and writing.
rm -rf ub_inst16t
build/sieve install ub-v16.sieve --to ub_inst16t | tr -d '\r' | q "^time      decoded in [0-9]*\.[0-9]\{3\} s, checked and written in [0-9]*\.[0-9]\{3\} s$"
cmp ub_one/model.safetensors ub_inst16/model.safetensors
python reference/sieve_ref.py unpack ub-v16.manifest --to ub_oracle16 > /dev/null
cmp ub_one/model.safetensors ub_oracle16/model.safetensors
# Pages of 64 characters: the line's options trimmed to what its id does not give (the
# alphabet is the default and goes; the length is not and stays), read back by both.
rm -rf pg64 pg64_inst pg64_oracle && mkdir pg64
python -c "import random; random.seed(3); [open(f'pg64/p{i}.txt','w').write(''.join(random.choice('abcdefghijklmnopqrstuvwxyz ') for _ in range(64))) for i in range(5)]"
build/sieve locate pg64 --installer pg64.sieve --items --page-length 64 --manifest pg64.manifest | tr -d '\r' | q "^items     5 file(s) carried as items of Sieve's lines (320 bytes, as 195 bytes of places"
[ "$(wc -c < pg64.sieve)" -eq 339 ] # LZMA2's settings chosen for its listing (v16 345; v7, its listing as text: 422)
# (v20, its body as text after all, the head's flag 128: five near-alike lines pack smaller as text.)
python -c "d = open('pg64.manifest', 'rb').read(); assert d[:6] == b'sieve\x14' and d[6] & 128, d[:7]"
build/sieve locate pg64 --installer pg64-16.sieve --items --page-length 64 --v16 > /dev/null
[ "$(wc -c < pg64-16.sieve)" -eq 345 ]
python -c "import sys; sys.argv = ['x']; sys.path.insert(0, 'reference'); import sieve_ref; sys.stdout.buffer.write(sieve_ref.v7_listing(sieve_ref.v16_text(open('pg64.manifest', 'rb').read())))" | q -a "^line 0 39 lower27/L64/key=sieve/feistel-sha256-v1 length=64 line=text$"
build/sieve install pg64.sieve --to pg64_inst > /dev/null
diff -r pg64 pg64_inst
python reference/sieve_ref.py unpack pg64.manifest --to pg64_oracle > /dev/null
diff -r pg64 pg64_oracle
# LZMA2's settings chosen per stream (v17's packing; the reader takes them from the stream):
# the oracle's own source as one file is smaller than v16 made it, and both read it back.
rm -rf tune tune_inst tune_oracle && mkdir tune && cp reference/sieve_ref.py tune/
build/sieve locate tune --installer tune.sieve --items --manifest tune.manifest > /dev/null
build/sieve locate tune --installer tune16.sieve --items --v16 > /dev/null
[ "$(wc -c < tune.sieve)" -lt "$(wc -c < tune16.sieve)" ]
build/sieve install tune.sieve --to tune_inst > /dev/null
diff -r tune tune_inst
python reference/sieve_ref.py unpack tune.manifest --to tune_oracle > /dev/null
diff -r tune tune_oracle
# The tightened rope (rope-wave-v1, IDEAS 19.87): a smooth picture in lines and a noisy
# sound, made here from a fixed generator. The search finds each one's wave, the code is
# smaller than LZMA2's, and the oracle's own decoding gives the file back.
rm -rf rope_mix rope_inst rope_oracle rope_si && mkdir rope_mix
python -c "
import math
s = 12345
def r():
    global s
    s = (s * 1103515245 + 12345) % 2147483648
    return s >> 16
open('rope_mix/picture.raw', 'wb').write(bytes((x * 3 + y * 2 + r() % 5) % 256 for y in range(150) for x in range(200)))
b = bytearray()
for i in range(8000):
    b += (int(9000 * math.sin(i * 0.05)) + r() % 81 - 40).to_bytes(2, 'little', signed=True)
open('rope_mix/sound.raw', 'wb').write(bytes(b))
"
cp data/anchors/public-domain/universalbibliothek.txt rope_mix/
build/sieve rope rope_mix/picture.raw --out picture.rope | tr -d '\r' > rope.txt
q "^wave      L8 W[0-9]* S[1-8] P[0-3] K[01]\( C[12]\)\?\( G1\)\? (rope-wave-v[1-4])" rope.txt
q "read back and checked" rope.txt
[ "$(wc -c < picture.rope)" -lt "$(sed -n 's/^LZMA2     \([0-9]*\) bytes.*/\1/p' rope.txt)" ]
params=$(sed -n 's/^wave      \(.*\) (rope-wave-v[1-4])/\1/p' rope.txt)
python reference/sieve_ref.py rope-unpack picture.rope --params "$params" --size 30000 --out picture.back > /dev/null
cmp rope_mix/picture.raw picture.back
# sieve-manifest-v8: the picture and the sound carried by the rope (the sound's wave at nine
# of its cycles, a line of 1,131 values: 10,213 bytes against LZMA2's 10,295), the text by LZMA2,
# each the smaller way. Smaller than v7 (no rope); installed by sieve install, the oracle, and
# sieve-install itself (no items, so the small installer needs nothing it lacks).
build/sieve locate rope_mix --installer rope8.sieve --items --manifest rope8.manifest | tr -d '\r' > rope8.txt
q "^rope      2 file(s) pulled tight by rope-wave-v1\( and v2\| to v[34]\)\? (46000 bytes" rope8.txt
python -c "import sys; sys.argv = ['x']; sys.path.insert(0, 'reference'); import sieve_ref; sys.stdout.buffer.write(sieve_ref.v7_listing(sieve_ref.v16_text(open('rope8.manifest', 'rb').read())))" | q -a "^f	30000	rope L8 W[0-9]* S[1-8] P[0-3] K[01]\( C[12]\)\?\( G1\)\?	picture.raw$"
build/sieve locate rope_mix --installer rope7.sieve --items --v7 > /dev/null
[ "$(wc -c < rope8.sieve)" -lt "$(wc -c < rope7.sieve)" ]
build/sieve install rope8.sieve --to rope_inst > /dev/null
diff -r rope_mix rope_inst
python reference/sieve_ref.py unpack rope8.manifest --to rope_oracle > /dev/null
diff -r rope_mix rope_oracle
build/sieve-install rope8.sieve --to rope_si --yes > /dev/null
diff -r rope_mix rope_si
# Weighed: the rope is a way to name any file, its code, its parameters and its size paid
# for, and its name once for every file that uses it.
build/sieve locate rope_mix --weigh 2>&1 | tr -d '\r' > ropeweigh.txt
q "^  picture.raw  *[0-9]*  *[0-9]*   rope L8 W[0-9]* S[1-8] P[0-3] K[01]\( C[12]\)\?\( G1\)\?$" ropeweigh.txt
q "^  shared: rope, [1-3] file(s): shape 96 bits$" ropeweigh.txt
# rope-wave-v2: digits in a table (groups of ten, ten groups a line, Windows line ends, from a
# fixed generator), each column's bytes counted apart: the separators' columns cost their
# tables alone, the digits about log2(10) bits each. Smaller than LZMA2; the oracle decodes it
# its own way; as instructions it is sieve-manifest-v9 (--v9), installed by all three readers.
# rope-wave-v4: the search groups the columns (the digits' share one table): smaller again,
# sieve-manifest-v12 (no items, so no items line), installed by all three readers; --v11 makes
# v11 again, a line longer.
rm -rf digits digits_inst digits_oracle digits_si digits_inst9 digits_oracle9 digits_si9 && mkdir digits
python -c "
s = 271828
def r():
    global s
    s = (s * 1103515245 + 12345) % 2147483648
    return (s >> 16) % 10
rows = [' '.join(''.join(str(r()) for _ in range(10)) for _ in range(10)) for _ in range(2000)]
open('digits/table.txt', 'wb').write(('\\r\\n'.join(rows) + '\\r\\n').encode())
"
build/sieve rope digits/table.txt --out table.rope | tr -d '\r' > table.txt
q "^wave      L8 W111 S1 P[0-3] K[01] C1 G1 (rope-wave-v4)" table.txt
[ "$(wc -c < table.rope)" -lt "$(sed -n 's/^LZMA2     \([0-9]*\) bytes.*/\1/p' table.txt)" ]
[ "$(wc -c < table.rope)" -lt 89048 ] # the digits' floor, 200,000 x log2(10) / 8 = 83,048, and 6,000 for the columns' tables
tparams=$(sed -n 's/^wave      \(.*\) (rope-wave-v4)/\1/p' table.txt)
python reference/sieve_ref.py rope-unpack table.rope --params "$tparams" --size 222000 --out table.back > /dev/null
cmp digits/table.txt table.back
build/sieve rope digits/table.txt --params "${tparams% G1}" --out table2.rope | tr -d '\r' | q "(rope-wave-v2)"
[ "$(wc -c < table.rope)" -lt "$(wc -c < table2.rope)" ]
python reference/sieve_ref.py rope-unpack table2.rope --params "${tparams% G1}" --size 222000 --out table2.back > /dev/null
cmp digits/table.txt table2.back
build/sieve locate digits --installer digits.sieve --items --v19 --manifest digits.manifest | tr -d '\r' | q "^rope      1 file(s) pulled tight by rope-wave-v1 to v4 (222000 bytes, carried in [0-9]*; sieve-manifest-v16)"
build/sieve install digits.sieve --to digits_inst > /dev/null
diff -r digits digits_inst
python reference/sieve_ref.py unpack digits.manifest --to digits_oracle > /dev/null
diff -r digits digits_oracle
build/sieve-install digits.sieve --to digits_si --yes > /dev/null
diff -r digits digits_si
build/sieve locate digits --installer digits11.sieve --items --v11 --manifest digits11.manifest | tr -d '\r' | q "; sieve-manifest-v11)"
[ "$(wc -c < digits.sieve)" -lt "$(wc -c < digits11.sieve)" ]
rm -rf digits_oracle11 && python reference/sieve_ref.py unpack digits11.manifest --to digits_oracle11 > /dev/null
diff -r digits digits_oracle11
build/sieve locate digits --installer digits9.sieve --items --v9 --manifest digits9.manifest | tr -d '\r' | q "^rope      1 file(s) pulled tight by rope-wave-v1 and v2 (222000 bytes, carried in [0-9]*; sieve-manifest-v9)"
[ "$(wc -c < digits.sieve)" -lt "$(wc -c < digits9.sieve)" ]
build/sieve install digits9.sieve --to digits_inst9 > /dev/null
diff -r digits digits_inst9
python reference/sieve_ref.py unpack digits9.manifest --to digits_oracle9 > /dev/null
diff -r digits digits_oracle9
build/sieve-install digits9.sieve --to digits_si9 --yes > /dev/null
diff -r digits digits_si9
# rope-wave-v3: the same kind of table laid out as pi6.txt is (Edward's stress test): lines of
# 55 and 54 bytes, a blank line after every twenty and a form feed in every tenth, which shift
# any fixed width. Counted by the column in each line, it is near the digits' floor (100,000
# digits: 41,525 bytes) and below LZMA2; sieve-manifest-v10 (--v10); all three readers give it
# back. rope-wave-v4: the columns grouped, smaller again; rope-wave-v5: the byte above too,
# smaller again; sieve-manifest-v15, all three readers.
rm -rf lines lines_inst lines_oracle lines_si lines_inst10 lines_oracle10 lines_si10 lines_bad && mkdir lines
python -c "
s = 314159
def r():
    global s
    s = (s * 1103515245 + 12345) % 2147483648
    return (s >> 16) % 10
out = []
for block in range(100):
    for line in range(20):
        groups = [''.join(str(r()) for _ in range(10)) for _ in range(5)]
        out.append(' '.join(groups) + (' ' if line % 2 == 0 else ''))
    out.append('\\x0c' if block % 10 == 9 else '')
open('lines/pi-like.txt', 'wb').write(('\\n'.join(out) + '\\n').encode())
"
build/sieve rope lines/pi-like.txt --out lines.rope | tr -d '\r' > lines.txt
q "^wave      L8 W56 S1 P0 K0 C2 G1 A1 (rope-wave-v5)" lines.txt
[ "$(wc -c < lines.rope)" -lt "$(sed -n 's/^LZMA2     \([0-9]*\) bytes.*/\1/p' lines.txt)" ]
[ "$(wc -c < lines.rope)" -lt 42525 ] # the digits' floor, 41,525, and 1,000 for the groups and the layout (measured: 41,790)
python reference/sieve_ref.py rope-unpack lines.rope --params "L8 W56 S1 P0 K0 C2 G1 A1" --size "$(wc -c < lines/pi-like.txt)" --out lines.back > /dev/null
cmp lines/pi-like.txt lines.back
# rope-wave-v4 (the columns grouped, no byte above): larger, and the oracle decodes it too.
build/sieve rope lines/pi-like.txt --params "L8 W56 S1 P0 K0 C2 G1" --out lines4.rope | tr -d '\r' | q "(rope-wave-v4)"
[ "$(wc -c < lines.rope)" -lt "$(wc -c < lines4.rope)" ]
[ "$(wc -c < lines4.rope)" -lt 43525 ] # the digits' floor, and 2,000 for the groups' tables and the layout
python reference/sieve_ref.py rope-unpack lines4.rope --params "L8 W56 S1 P0 K0 C2 G1" --size "$(wc -c < lines/pi-like.txt)" --out lines4.back > /dev/null
cmp lines/pi-like.txt lines4.back
build/sieve rope lines/pi-like.txt --params "L8 W56 S1 P0 K0 C2" --out lines3.rope | tr -d '\r' | q "(rope-wave-v3)"
[ "$(wc -c < lines.rope)" -lt "$(wc -c < lines3.rope)" ]
[ "$(wc -c < lines3.rope)" -lt 45525 ] # the digits' floor, 41,525, and 4,000 for the columns' tables
python reference/sieve_ref.py rope-unpack lines3.rope --params "L8 W56 S1 P0 K0 C2" --size "$(wc -c < lines/pi-like.txt)" --out lines3.back > /dev/null
cmp lines/pi-like.txt lines3.back
build/sieve locate lines --installer lines.sieve --items --v19 --manifest lines.manifest | tr -d '\r' | q "; sieve-manifest-v16)"
build/sieve install lines.sieve --to lines_inst > /dev/null
diff -r lines lines_inst
python reference/sieve_ref.py unpack lines.manifest --to lines_oracle > /dev/null
diff -r lines lines_oracle
build/sieve-install lines.sieve --to lines_si --yes > /dev/null
diff -r lines lines_si
# --v14 (no rope-wave-v5): v12 here, larger; refused when its v5 way is called v14.
build/sieve locate lines --installer lines14.sieve --items --v14 --manifest lines14.manifest | tr -d '\r' | q "; sieve-manifest-v12)"
[ "$(wc -c < lines.sieve)" -lt "$(wc -c < lines14.sieve)" ]
build/sieve locate lines --installer lines15.sieve --items --v15 --manifest lines15.manifest | tr -d '\r' | q "; sieve-manifest-v15)"
[ "$(wc -c < lines.sieve)" -lt "$(wc -c < lines15.sieve)" ]
sed '1s/v15/v14/' lines15.manifest > lines-as-v14.manifest
if build/sieve install lines-as-v14.manifest --to lines_bad > /dev/null 2>&1; then exit 1; fi
if python reference/sieve_ref.py unpack lines-as-v14.manifest --to lines_bad > /dev/null 2>&1; then exit 1; fi
test ! -e lines_bad
build/sieve locate lines --installer lines10.sieve --items --v10 --manifest lines10.manifest | tr -d '\r' | q "; sieve-manifest-v10)"
[ "$(wc -c < lines.sieve)" -lt "$(wc -c < lines10.sieve)" ]
build/sieve install lines10.sieve --to lines_inst10 > /dev/null
diff -r lines lines_inst10
python reference/sieve_ref.py unpack lines10.manifest --to lines_oracle10 > /dev/null
diff -r lines lines_oracle10
build/sieve-install lines10.sieve --to lines_si10 --yes > /dev/null
diff -r lines lines_si10
# sieve-manifest-v12: a file LZMA2 cannot make smaller (bytes from a fixed generator, as a
# PNG's or a JPEG's are, near enough) carried as it is, its stream "stream lzma2 B B 0"; the
# instructions are the file and 153 bytes (v11: 179, with LZMA2's framing and an items line).
rm -rf noisy noisy_inst noisy_oracle noisy_si && mkdir noisy
python -c "
s = 161803
out = bytearray()
for _ in range(50000):
    s = (s * 6364136223846793005 + 1442695040888963407) % 2**64
    out.append(s >> 56)
open('noisy/noise.bin', 'wb').write(bytes(out))
"
build/sieve locate noisy --installer noisy.sieve --items --v19 --manifest noisy.manifest | tr -d '\r' > noisy.txt
q "^as is     the files packed by LZMA2 carried as they are instead" noisy.txt
q "^items     none; sieve-manifest-v16$" noisy.txt
build/sieve locate noisy --installer noisy15.sieve --items --v15 --manifest noisy15.manifest | tr -d '\r' | q "^items     none; sieve-manifest-v12$"
head -c 200 noisy15.manifest | q -a "^stream lzma2 50000 50000 0$"
[ "$(wc -c < noisy.sieve)" -le 50160 ] # the file, its name, size and the tree's SHA-256
build/sieve locate noisy --installer noisy11.sieve --items --v11 > /dev/null
[ "$(wc -c < noisy.sieve)" -lt "$(wc -c < noisy11.sieve)" ]
build/sieve install noisy.sieve --to noisy_inst > /dev/null
diff -r noisy noisy_inst
python reference/sieve_ref.py unpack noisy.manifest --to noisy_oracle > /dev/null
diff -r noisy noisy_oracle
build/sieve-install noisy.sieve --to noisy_si --yes > /dev/null
diff -r noisy noisy_si
# The same instructions called v11, which has neither: refused by Sieve and by the oracle.
sed '1s/v12/v11/' noisy15.manifest > noisy-as-v11.manifest
if build/sieve install noisy-as-v11.manifest --to noisy_bad > /dev/null 2>&1; then exit 1; fi
if python reference/sieve_ref.py unpack noisy-as-v11.manifest --to noisy_bad > /dev/null 2>&1; then exit 1; fi
test ! -e noisy_bad
# sieve-manifest-v13: a file cut into pieces, each coded the way that suits it: 20,000 bytes of
# text (LZMA2's: they repeat) before 300,000 bytes that do not repeat but are unevenly spread
# (the rope's), as a phone's photo has its metadata before its picture. Smaller than v12 (one
# way for the whole file) and than xz; the cut found within a block of where it is; all
# three readers give it back, and the same instructions called v12 are refused.
rm -rf mixed mixed_inst mixed_oracle mixed_si mixed_bad && mkdir mixed
python -c "
import math
s = 2718281
def u():
    global s
    s = (s * 6364136223846793005 + 1442695040888963407) % 2**64
    return ((s >> 11) + 0.5) / 2**53
text = open('data/anchors/public-domain/universalbibliothek.txt', 'rb').read()[:20000]
body = bytes(min(255, int(-math.log(u()) * 40)) for _ in range(300000))
open('mixed/photo-like.bin', 'wb').write(text + body)
"
build/sieve locate mixed --installer mixed.sieve --items --manifest mixed.manifest | tr -d '\r' > mixed.txt
q "^pieces    1 file(s) cut into pieces: [0-9]* bytes packed by LZMA2, [0-9]* pulled tight by the rope (sieve-manifest-v13)$" mixed.txt
z=$(sed -n 's/^pieces .* cut into pieces: \([0-9]*\) bytes packed.*/\1/p' mixed.txt)
[ "$z" -ge 15904 ] && [ "$z" -le 24096 ] # the text's 20,000, within a block either way
build/sieve locate mixed --installer mixed12.sieve --items --v12 > /dev/null
[ "$(wc -c < mixed.sieve)" -lt "$(wc -c < mixed12.sieve)" ]
[ "$(wc -c < mixed.sieve)" -lt "$(xz -9e -c mixed/photo-like.bin | wc -c)" ]
build/sieve install mixed.sieve --to mixed_inst > /dev/null
diff -r mixed mixed_inst
# Weighed, the file's best way is its pieces, as the instructions carry it (the weighing
# and the packer choose by the same code).
build/sieve locate mixed --weigh 2>&1 | tr -d '\r' > mixedweigh.txt
q "^  photo-like.bin  *[0-9]*  *[0-9]*   pieces z[0-9]* r[0-9]* rope L" mixedweigh.txt
q "^  shared: pieces, 1 file(s): shape 232 bits$" mixedweigh.txt
python reference/sieve_ref.py unpack mixed.manifest --to mixed_oracle > /dev/null
diff -r mixed mixed_oracle
build/sieve-install mixed.sieve --to mixed_si --yes > /dev/null
diff -r mixed mixed_si
build/sieve locate mixed --installer mixed15.sieve --items --v15 --manifest mixed15.manifest > /dev/null
sed '1s/v13/v12/' mixed15.manifest > mixed-as-v12.manifest
if build/sieve install mixed-as-v12.manifest --to mixed_bad > /dev/null 2>&1; then exit 1; fi
if python reference/sieve_ref.py unpack mixed-as-v12.manifest --to mixed_bad > /dev/null 2>&1; then exit 1; fi
test ! -e mixed_bad
# sieve-manifest-v14, "Optimise Archive Formats" (--optimise-archives; the File Locator's box):
# off by default, and then nothing changes. Ticked, an xz file is remade exactly from what it
# holds (liblzma, as the xz program writes it) and a 7z (tests/optimise_fixture.7z: 20 blocks
# of the pi-like table above, 22,222 bytes in 10,513) is carried as its contents, in a folder
# named after it, installing saying so; each only where clearly smaller. Sieve and the oracle
# give the same, and so does sieve-install (the full installer); called v13, refused.
rm -rf arch arch_inst arch_oracle arch_si arch_bad arch_bad16 && mkdir -p arch/x
cp tests/optimise_fixture.7z arch/digits.7z
python -c "
s = 314159
def r():
    global s
    s = (s * 1103515245 + 12345) % 2147483648
    return (s >> 16) % 10
def table(blocks):
    out = []
    for block in range(blocks):
        for line in range(20):
            groups = [''.join(str(r()) for _ in range(10)) for _ in range(5)]
            out.append(' '.join(groups) + (' ' if line % 2 == 0 else ''))
        out.append('\x0c' if block % 10 == 9 else '')
    return ('\n'.join(out) + '\n').encode()
open('arch-digits.txt', 'wb').write(table(20))
s = 271828
open('arch/x/table.txt', 'wb').write(table(60))
# v17: a gzip file as zlib writes it (Python's gzip), remade exactly; a zip of the same
# kind of table, carried as its contents.
import gzip, io, zipfile
s = 161803
t = table(40)
with open('arch/y.txt.gz', 'wb') as f:
    with gzip.GzipFile(filename='y.txt', mode='wb', fileobj=f, compresslevel=9, mtime=0) as g:
        g.write(t)
open('arch-y.txt', 'wb').write(t)
with zipfile.ZipFile('arch/w.zip', 'w', zipfile.ZIP_DEFLATED) as z:
    z.writestr('w/one.txt', table(30))
    z.writestr('w/two.txt', b'hello\n')
"
xz -9 arch/x/table.txt
build/sieve locate arch --installer arch_off.sieve --items --v19 --manifest arch_off.manifest | tr -d '\r' > arch_off.txt
q "^items     none; sieve-manifest-v1[2-6]$" arch_off.txt
if grep -q "sieve-manifest-v14\|^remade\|^unpacked" arch_off.txt; then exit 1; fi
build/sieve locate arch --installer arch.sieve --items --optimise-archives --v19 --manifest arch.manifest | tr -d '\r' > arch.txt
q "^remade    2 archive(s) carried as what they hold, remade exactly on installing (sieve-manifest-v17)$" arch.txt
q "^unpacked  2 archive(s) carried as their contents, not as the archives (Optimise Archive Formats; sieve-manifest-v17)$" arch.txt
q "^items     none; sieve-manifest-v17$" arch.txt
[ "$(wc -c < arch.sieve)" -lt "$(wc -c < arch_off.sieve)" ]
build/sieve install arch.sieve --to arch_inst | tr -d '\r' > arch_inst.txt
q "^note      digits.7z (7z, 10513 bytes) was carried as its contents (Optimise Archive Formats): digits holds them, not the archive itself$" arch_inst.txt
cmp arch/x/table.txt.xz arch_inst/x/table.txt.xz
cmp arch/y.txt.gz arch_inst/y.txt.gz
q "^note      w.zip (zip, [0-9]* bytes) was carried as its contents (Optimise Archive Formats): w holds them, not the archive itself$" arch_inst.txt
test "$(cat arch_inst/w/two.txt)" = "hello"
test ! -e arch_inst/w.zip
cmp arch-digits.txt arch_inst/digits/digits.txt
test ! -e arch_inst/digits.7z
python reference/sieve_ref.py unpack arch.manifest --to arch_oracle | tr -d '\r' | q "^unpacked	7z	10513	digits.7z	digits$"
diff -r arch_inst arch_oracle
build/sieve-install arch.sieve --to arch_si --yes
diff -r arch_inst arch_si
build/sieve locate arch --installer arch15.sieve --items --optimise-archives --v15 --manifest arch15.manifest > /dev/null
sed '1s/v1[45]$/v13/' arch15.manifest > arch-as-v13.manifest
if build/sieve install arch-as-v13.manifest --to arch_bad > /dev/null 2>&1; then exit 1; fi
if python reference/sieve_ref.py unpack arch-as-v13.manifest --to arch_bad > /dev/null 2>&1; then exit 1; fi
test ! -e arch_bad
# v17 called v16 (its head's byte 17 made 16): the gzip way refused, by Sieve and the oracle.
python -c "d = bytearray(open('arch.manifest', 'rb').read()); assert d[:6] == b'sieve\x11'; d[5] = 16; open('arch-as-v16.manifest', 'wb').write(d)"
if build/sieve install arch-as-v16.manifest --to arch_bad16 > /dev/null 2>&1; then exit 1; fi
if python reference/sieve_ref.py unpack arch-as-v16.manifest --to arch_bad16 > /dev/null 2>&1; then exit 1; fi
test ! -e arch_bad16
# v19 (25 October 2026): bzip2 and zstd. A bzip2 file (Python's bz2: libbzip2) and a zstd file
# written by libzstd 1.5.7 (the zstandard module's, the version Sieve is built with) are remade
# exactly; a zstd file of two frames is carried as its contents. Sieve, the oracle and
# sieve-install give the same; called v18, refused.
python -m pip install --quiet "zstandard==0.25.0"
rm -rf arch19 arch19_inst arch19_oracle arch19_si arch19_bad && mkdir -p arch19
python -c "
import bz2, zstandard
t = open('arch-y.txt', 'rb').read()
assert '.'.join(map(str, zstandard.ZSTD_VERSION)) == '1.5.7'
open('arch19/y.txt.bz2', 'wb').write(bz2.compress(t, 9))
open('arch19/y.txt.zst', 'wb').write(zstandard.ZstdCompressor(level=7, write_checksum=True).compress(t))
d = open('arch-digits.txt', 'rb').read()
c = zstandard.ZstdCompressor(level=3)
open('arch19/two.zst', 'wb').write(c.compress(d[:len(d) // 2]) + c.compress(d[len(d) // 2:]))
open('arch19-two.txt', 'wb').write(d)
"
build/sieve locate arch19 --installer arch19.sieve --items --optimise-archives --v19 --manifest arch19.manifest | tr -d '\r' > arch19.txt
q "^remade    2 archive(s) carried as what they hold, remade exactly on installing (sieve-manifest-v19)$" arch19.txt
q "^unpacked  1 archive(s) carried as their contents, not as the archives (Optimise Archive Formats; sieve-manifest-v19)$" arch19.txt
q "^items     none; sieve-manifest-v19$" arch19.txt
build/sieve install arch19.sieve --to arch19_inst | tr -d '\r' > arch19_inst.txt
cmp arch19/y.txt.bz2 arch19_inst/y.txt.bz2
cmp arch19/y.txt.zst arch19_inst/y.txt.zst
cmp arch19-two.txt arch19_inst/two
q "^note      two.zst (zst, [0-9]* bytes) was carried as its contents (Optimise Archive Formats): two holds them, not the archive itself$" arch19_inst.txt
python reference/sieve_ref.py unpack arch19.manifest --to arch19_oracle > /dev/null
diff -r arch19_inst arch19_oracle
build/sieve-install arch19.sieve --to arch19_si --yes
diff -r arch19_inst arch19_si
python -c "d = bytearray(open('arch19.manifest', 'rb').read()); assert d[:6] == b'sieve\x13'; d[5] = 18; open('arch19-as-v18.manifest', 'wb').write(d)"
if build/sieve install arch19-as-v18.manifest --to arch19_bad > /dev/null 2>&1; then exit 1; fi
if python reference/sieve_ref.py unpack arch19-as-v18.manifest --to arch19_bad > /dev/null 2>&1; then exit 1; fi
test ! -e arch19_bad
# sieve-manifest-v16's head: a flag this Sieve does not know is refused, by Sieve and the oracle.
python -c "d = bytearray(open('noisy.manifest', 'rb').read()); assert d[:6] == b'sieve\x10'; d[6] |= 0x40; open('noisy-flag.manifest', 'wb').write(d)"
if build/sieve install noisy-flag.manifest --to noisy_flag > /dev/null 2>&1; then exit 1; fi
if python reference/sieve_ref.py unpack noisy-flag.manifest --to noisy_flag > /dev/null 2>&1; then exit 1; fi
test ! -e noisy_flag
# (each file is named by its item's title; the report says which file each degree gave)
$H --no-menu --press "X,Tab,9,0,Return" --take --save-item p90.txt --screenshot degp.png > /dev/null
cmp p90.txt "degrees/$(sed -n 's/^pages 090 //p' deg.txt)"
$H --no-menu --line models --press "X,Tab,1,3,Return" --take --save-item m13.obj --screenshot degm.png > /dev/null
cmp m13.obj "degrees/$(sed -n 's/^models 013 //p' deg.txt)"
# The excluded mode: read back from the settings, and the excluded count is exactly the
# line less the survivors (27^32 less clean-v2's survivors at length 32).
printf "[text]\nmode = excluded\nfilters = clean-v2\n" > excluded.ini
build/sieve filters --line text --length 32 --filters excluded.ini | q "^mode *excluded"
build/sieve filters --line text --length 32 --filters excluded.ini | q "^excluded *247982505688110457657730525203966127924868733 "
$H --no-menu --filters excluded.ini --goto 0% --press Escape --screenshot excluded.png | q "saved excluded.png"
# The vault (docs/VAULT.md): its built-in test entry, the 16 bytes "sieve vault test", is
# withheld as a file everywhere (located, mapped, installed by either installer) and
# written out as text (as it is, or in any vault-decoders-v1 form, such as hex, base64 or
# the a-p nibbles a lower27 line can hold), while text that is not a known file, however
# close its words, is not. A vault file that cannot be read fails it closed.
printf 'sieve vault test' > vt.txt
build/sieve vault | q "3 built in"
build/sieve vault | q "vault-decoders-v1"
# Pictures, by PDQ: this cover's hash as Meta's own pdqhash (Pillow decoding) gives it, so
# Sieve's reading of the reference code agrees bit for bit; then the test picture withheld
# as it is, scaled, as an image unit, and base64'd, while an ordinary picture is not.
build/sieve vault --pdq tests/example_book_cover.png | q "^cc661c39738cc4c68c713399cc63cc66739c1939cc66e3ce39391c71e3c6e38e  quality 56 "
build/sieve vault --test-picture vtp.png > /dev/null
build/sieve vault --test-picture vtp4.png --scale 4 > /dev/null
if build/sieve vault vtp.png > /dev/null; then exit 1; fi
if build/sieve vault vtp4.png > /dev/null; then exit 1; fi
if build/sieve locate vtp4.png > /dev/null 2>&1; then exit 1; fi
if build/sieve warp --line image --palette rgb24 --width 32 --height 32 --file vtp.png > /dev/null 2>&1; then exit 1; fi
base64 vtp.png > vtp.b64
if build/sieve vault --written vtp.b64 > /dev/null; then exit 1; fi
build/sieve vault tests/example_book_cover.png | q "^clear"
build/sieve warp --line image --palette rgb24 --width 32 --height 32 --file tests/example_book_cover.png | q "positional"
if build/sieve vault vt.txt > /dev/null; then exit 1; fi
if build/sieve locate vt.txt > /dev/null 2>&1; then exit 1; fi
if build/sieve warp --line text --length 32 "sieve vault test" > /dev/null 2>&1; then exit 1; fi
if build/sieve warp --line text --length 32 hdgjgfhggfcahggbhfgmhecahegfhdhe > /dev/null 2>&1; then exit 1; fi
printf '73 69 65 76 65 20 76 61 75 6c 74 20 74 65 73 74\n' > vhex.txt
printf 'c2lldmUgdmF1bHQgdGVzdA==' > vb64.txt
if build/sieve vault --written vhex.txt > /dev/null; then exit 1; fi
if build/sieve vault --written vb64.txt > /dev/null; then exit 1; fi
build/sieve warp --line text --length 32 "sieve vault tests" | q "positional"
build/sieve map --new vt --out vt-map.sieve 2> /dev/null
if build/sieve map vt-map.sieve --add vt.txt --out vt-map.sieve > /dev/null 2>&1; then exit 1; fi
python reference/sieve_ref.py manifest vt.txt --with-contents > vt.manifest
python -c "b=open('vt.manifest','rb').read(); n=int.from_bytes(b,'big')+(256**len(b)-1)//255; open('vt.sieve','wb').write(n.to_bytes((n.bit_length()+7)//8,'big'))"
rm -rf vti vti2
if build/sieve install vt.sieve --to vti > /dev/null 2>&1; then exit 1; fi
if build/sieve-install vt.sieve --to vti2 --yes; then exit 1; fi
test ! -e vti && test ! -e vti2
printf 'nonsense\n' > data/vault/broken.vault
printf 'harmless' > vok.txt
build/sieve vault | q "FAILED CLOSED"
if build/sieve locate vok.txt > /dev/null 2>&1; then exit 1; fi
rm data/vault/broken.vault
build/sieve locate vok.txt | q "^sha256"
# Pieces, by chunks (cdc-v1, chunk-match-v1): the built-in test file is listed by its chunks
# only, so a 2.5 KB piece of it between other bytes is withheld, as a file, located, and
# base64'd; a vault file written by --chunks lists another file the same way; and
# --common leaves out chunks that ordinary files also hold.
build/sieve vault | q "1 chunks: 13 chunks in all"
build/sieve vault --test-file vtf.bin > /dev/null
python -c "import base64; b=open('vtf.bin','rb').read(); open('vtf-piece.bin','wb').write(b'junk'+b[1000:3500]+b'junk'); open('vtf-piece.b64','w').write(base64.b64encode(b[500:3000]).decode())"
if build/sieve vault vtf-piece.bin > /dev/null; then exit 1; fi
if build/sieve locate vtf-piece.bin > /dev/null 2>&1; then exit 1; fi
if build/sieve vault --written vtf-piece.b64 > /dev/null; then exit 1; fi
python -c "import hashlib; open('listed.bin','wb').write(b''.join(hashlib.sha256(b'listed %d' % i).digest() for i in range(300)))"
python -c "b=open('listed.bin','rb').read(); open('listed-piece.bin','wb').write(b[3000:6000])"
build/sieve vault listed-piece.bin | q "^clear"
build/sieve vault --chunks listed.bin > data/vault/listed.vault
build/sieve vault --parse data/vault/listed.vault | q "1 chunks), well formed"
if build/sieve vault listed-piece.bin > /dev/null; then exit 1; fi
rm data/vault/listed.vault
rm -rf ordinary && mkdir ordinary && cp listed-piece.bin ordinary/
test "$(build/sieve vault --chunks listed.bin --common ordinary | tail -2 | head -1 | tr ',' '\n' | wc -l)" -lt "$(build/sieve vault --chunks listed.bin | tail -2 | head -1 | tr ',' '\n' | wc -l)"
# Filter plugins that do not load are listed with the reason, never skipped: a malformed
# file, and two different files under one name (neither is used: versions are never edited).
printf 'sieve-filter-v1\nid broken\n' > build/filters/broken.sfilter
sed 's/^describe.*/describe edited/' data/filters/clean-data-v1.sfilter > build/filters/clean-edited.sfilter
build/sieve filters --plugins > plugins.out
grep -A1 "broken.sfilter" plugins.out | q "ends with end"
grep -A1 "clean-edited.sfilter" plugins.out | q "with different contents"
if build/sieve filters --length 8 | grep -q "^\[.\] clean-data-v1 "; then exit 1; fi
rm build/filters/broken.sfilter build/filters/clean-edited.sfilter
build/sieve filters --length 8 | q "^\[.\] clean-data-v1 "
# `requires`: ticking tidy-data-v2 alone brings clean-data-v1 and the built-in max-run-v1
# (at the max_run it pins) into the stack, which still counts, exactly as tidy-data-v1 and
# max-run-data-v1 did (both retired for them); a different setting is flagged; and a
# copy of a plugin under another name is found to be the same rule.
printf "[text]\nmode = compact\nfilters = tidy-data-v2\n" > tidy.ini
build/sieve filters --length 12 --filters tidy.ini | q "max-run-v1{max_run=3}; clean-data-v1"
build/sieve filters --length 12 --filters tidy.ini | q "^survivors    147852209843303498 (exact; compact mode available)"
printf "[text]\nmode = compact\nfilters = tidy-data-v1\n" > tidy1.ini
build/sieve filters --length 12 --filters tidy1.ini | q "^survivors    147852209843303498 (exact; compact mode available)"
build/sieve filters --length 12 | q "tidy-data-v1   (custom)   (retired: replaced by tidy-data-v2)"
printf "[text]\nfilters = tidy-data-v2, max-run-v1\n[text.max-run-v1]\nmax_run = 5\n" > tidy5.ini
build/sieve filters --length 12 --filters tidy5.ini | q "requires max-run-v1 at max_run=3; it is set to max_run=5"
sed 's/^id        clean-data/id        clean-copy/' data/filters/clean-data-v1.sfilter > clean-copy.sfilter
build/sieve filters --plugin clean-copy.sfilter --relations | q "the same rule as clean-data-v1  -- a duplicate"
# The filter designer (main menu): it opens a plugin and writes it back as it would save
# it, the same rule (a round trip); a word filter made and saved by script is registered
# at once and named a duplicate of words-data-v1; saving other contents under that
# version offers the next one instead (versions are never edited).
# The Encryption Engine's screen (IDEAS §19.52): its opening recipe there and back (Tab puts
# the output in as the input, and turns encrypt into decrypt), and a weak cipher broken.
$H --cipher --screenshot cipher.png | tr -d '\r' | q "^output: c0dfd54c54f353b432f1ba19925f$"
$H --cipher --script "Tab" --screenshot cipher-back.png | tr -d '\r' | q "^output: 61747461636b206174206461776e$"
# A file's bytes as the input (as the item in hand or Ctrl+O gives them), and the output
# saved: the same bytes as sieve cipher makes of the same file with the same recipe.
rm -f engine.enc cli.enc
$H --cipher --cipher-in tests/manifest_fixture/a.txt --cipher-out engine.enc --screenshot cipher-file.png | tr -d '\r' | q "^cipher: the output saved: "
build/sieve cipher --encrypt --in tests/manifest_fixture/a.txt --recipe "vigenere lemon; feistel my key" --out cli.enc > /dev/null
cmp engine.enc cli.enc
$H --cipher --script "Ctrl+Backspace,=It was the best of times it was the worst of times it was the age of wisdom it was the age of foolishness,Down,Down,Ctrl+Backspace,=xor 736563726574,Return,Tab,Down,Right" --screenshot cipher-break.png | tr -d '\r' | q "^cipher: the best guess: xor 736563726574$"
$H --main-menu --press "Down,Down,Down,Down,Down,Down" --screenshot main-cipher.png | q "saved main-cipher.png"
$H --designer --screenshot designer.png | q "designer: my-words-v1"
$H --designer --busy --screenshot designer-busy.png | q "designer: my-words-v1"
# Opening the designer compiles nothing: only F5 (TEST NOW), Save or the relation check do.
test "$($H --designer --timings --screenshot designer-idle.png 2>&1 | grep -c '^designer.test ')" = 0
$H --designer --design tests/plugins/toy-grammar-v1.sfilter --design-out tests/plugins/roundtrip-v1.sfilter --screenshot designer2.png | q "designer: toy-grammar-v1"
build/sieve filters --plugin tests/plugins/toy-grammar-v1.sfilter --length 11 | grep -v "^sha256" > rt.a
build/sieve filters --plugin tests/plugins/roundtrip-v1.sfilter --length 11 | grep -v "^sha256" > rt.b
diff rt.a rt.b
rm tests/plugins/roundtrip-v1.sfilter
rm -f build/filters/designed-words-v*.sfilter
$H --designer --script "Ctrl+N,Down,Return,Ctrl+Backspace,=designed-words,Return,Ctrl+S" --screenshot designer3.png | q "status: saved to .*designed-words-v1.sfilter"
build/sieve filters --plugins | q "^loaded   designed-words-v1"
build/sieve filters --plugin build/filters/designed-words-v1.sfilter --length 6 --relations | q "the same rule as words-data-v1  -- a duplicate"
$H --designer --design build/filters/designed-words-v1.sfilter --script "Down,Down,Down,Down,Down,Down,Down,Return,=!,Return,Ctrl+S" --screenshot designer4.png | q "save as version 2"
rm build/filters/designed-words-v1.sfilter
# --timings: the table at exit, on the command line and from the hallway (to its file too).
build/sieve warp --line text --length 32 --timings "timed" 2>&1 | q "^sieve warp "
$H --no-menu --timings --settings timed.ini --screenshot timed.png 2>&1 | q "^hallway.item "
grep -q "^hallway.build " sieve-timings.txt
# F on the item page saves the item in hand (here without its dialog): a page of the text
# line as text, and a file of the binary line as exactly its bytes.
rm -f saved_page.txt saved_file.bin
$H --no-menu --pose 0.8,3.12,-90,-5 --take --save-item saved_page --screenshot savepage.png | q "saved savepage.png"
test -s saved_page.txt
$H --no-menu --line binary --binary-length 64 --pose 0.8,3.12,-90,-5 --take --save-item saved_file.bin --screenshot savefile.png | q "saved savefile.png"
test "$(wc -c < saved_file.bin)" -gt 0
# Past the budget: a 100 KB file, twice the BINARY length, is still reached by pressing
# Go to it twice; the line is made long enough, positional, and only the room is kept.
head -c 100000 /dev/urandom > big.bin
$H --no-menu --binary-length 1000 --locate big.bin --press "Return,Return" --screenshot big.png | tail -2 | sed -n 1p | q "^BINARY line, positional"
# The node graph (O, and the pause menu's third row): the fixture's map, laid out; Tab to
# its third node (a.txt) and Enter walk to it on the binary line; and the item page's
# SORT tab knows a.txt for a verified anchor of that map, and anything else for a lone point.
$H --no-menu --map tests/manifest_fixture --graph --screenshot graph.png | q "saved graph.png"
$H --no-menu --binary-length 64 --map tests/manifest_fixture --graph --press "Tab,Tab,Tab,Return" --screenshot graph2.png | tail -2 | sed -n 1p | q "^BINARY line.* TXT 34 bytes"
$H --no-menu --binary-length 64 --map tests/manifest_fixture --locate tests/manifest_fixture/a.txt --press "Return,C,C" --screenshot sort.png | tail -2 | sed -n 1p | q "a verified anchor: node 2, a.txt, in manifest_fixture"
$H --no-menu --line binary --map tests/manifest_fixture --walk "0,0" --press "E,C,C" --screenshot sort2.png | tail -2 | sed -n 1p > sort2.txt
if grep -q "verified anchor" sort2.txt; then exit 1; fi
# (--warp starts with the item in your hands: the first Escape puts it down, the second pauses.)
$H --warp "$W" --press "Escape,Escape,Down,Down,Return" --screenshot pausegraph.png | q "saved pausegraph.png"
$H --no-menu --graph --screenshot installation.png | q "saved installation.png"
# Verified anchors: New map... makes an empty map; V adds the file in hand to it, held
# (its bytes kept in the map), and V again takes it out; with metadata, the item page
# gains a META tab; a held anchor can be walked to from the viewer, whose Delete removes
# it; a sealed map will not take one.
rm -f build/maps/finds.sieve
$H --no-menu --binary-length 64 --new-map build/maps/finds.sieve --locate tests/manifest_fixture/a.txt --press "Return,V" --screenshot anchor.png | q "added a.txt to finds as a verified anchor (node 1)"
build/sieve map build/maps/finds.sieve 2>&1 | q "1 held"
$H --no-menu --binary-length 64 --map build/maps/finds.sieve --locate tests/manifest_fixture/a.txt --press "Return,V" --screenshot anchor2.png | q "removed a.txt from finds"
build/sieve map build/maps/finds.sieve 2>&1 | q "0 held"
build/sieve map build/maps/finds.sieve --add tests/manifest_fixture/a.txt --out build/maps/finds.sieve 2> /dev/null
build/sieve map build/maps/finds.sieve --meta "1:source=the fixture folder" --out build/maps/finds.sieve 2> /dev/null
$H --no-menu --binary-length 64 --map build/maps/finds.sieve --locate tests/manifest_fixture/a.txt --press "Return,C,C,C" --screenshot meta.png | q "saved meta.png"
$H --no-menu --binary-length 64 --map build/maps/finds.sieve --graph --press "Tab,Tab,Return" --screenshot anchorgo.png | tail -2 | sed -n 1p | q "^BINARY line.* TXT 34 bytes"
$H --no-menu --map build/maps/finds.sieve --graph --press "Tab,Tab,Delete" --screenshot anchordel.png | q "saved anchordel.png"
build/sieve map build/maps/finds.sieve 2>&1 | q "0 held"
rm -f build/maps/finds.sieve
build/sieve map --new sealed --seal --out build/maps/sealed.sieve 2> /dev/null
$H --no-menu --binary-length 64 --map build/maps/sealed.sieve --locate tests/manifest_fixture/a.txt --press "Return,V" --screenshot sealed.png | q "sealed is read-only"
rm -f build/maps/sealed.sieve
# The pause menu (Esc): its fourth row is the Navigation System, which opens the navigator.
$H --warp "$W" --press "Escape,Escape,Down,Down,Down,Return,Escape" --screenshot pause.png | q "saved pause.png"
# The Media Player (pause menu, sixth row): with --media-player a silent music player is
# made for the picture; its settings file goes where --settings says, not beside the program.
mkdir -p mp
$H --no-menu --media-player --settings mp/hallway.ini --press "Escape,Down,Down,Down,Down,Down,Return" --screenshot media.png | q "saved media.png"
out=$($H --warp "$W" --press "X,Home,Up,Return" --screenshot nav.png)
echo "$out" | tail -2 | sed -n 1p | q "went to 1"
if [ "$(echo "$out" | tail -2 | book)" = "$here" ]; then exit 1; fi
# The navigator's bearing field: Tab, a bearing, ENTER, and you stand at that angle round
# the loop (180 degrees is exactly halfway); a bearing of 360 or more is refused.
$H --warp "$W" --press "X,Tab,1,8,0,Return" --screenshot navangle.png | tail -2 | sed -n 1p | q "50.0000000000% along"
if $H --warp "$W" --press "X,Tab,3,6,0,Return" --screenshot navangle2.png | tail -2 | sed -n 1p | grep -q "went to"; then exit 1; fi
start=$($H --mode guided --warp "$W" --zoom 20 --walk "0,0" --screenshot start.png | book)
# The corridor end to end and back: nine doors left from pages reach binary (books,
# audio, tracks, video, movies, models, worlds, AI), which is where the run finishes, and
# nine right brings you back to the same book on pages.
out=$($H --mode guided --warp "$W" --zoom 20 --pose 0,7,-90,0 --walk "-2.3,0;-4.2,0;-4.2,0;-4.2,0;-4.2,0;-4.2,0;-4.2,0;-4.2,0;-4.2,0;4.2,0;4.2,0;4.2,0;4.2,0;4.2,0;4.2,0;4.2,0;4.2,0;4.2,0" --screenshot guided.png)
echo "$out"
echo "$out" | q "BOOKS line"
echo "$out" | q "TRACKS line"
echo "$out" | q "VIDEO line"
echo "$out" | q "MOVIES line"
echo "$out" | q "MODELS line"
echo "$out" | q "WORLDS line, positional"
echo "$out" | q "AI line, positional"
echo "$out" | q "BINARY line"
echo "$out" | sed -n 9p | q "^BINARY line"
echo "$out" | tail -2 | sed -n 1p | q "^PAGES line"
test "$(echo "$out" | tail -2 | book)" = "$start"
# Loops: a 2-character line is 729 units = 6 tiles, so tile 6 starts the second copy,
# and the CLI reads the same corridor places the hallway shows. The CLI's lines are bare,
# and a title length of 0 makes the hallway's the same.
$H --length 2 --title-length 0 --goto @6 --walk "0,0" --screenshot loop6.png | q "first book 000 "
test "$(build/sieve read --length 2 --mode positional --at 6:0 2>/dev/null)" = "  "
test "$(build/sieve read --length 2 --mode positional --at -1:127 2>/dev/null)" = "(empty: padding at the end of a loop)"
build/sieve info --length 2 | q "39 empty slots"
build/sieve info --line image | q "fills whole tiles exactly"
# The setup menu renders at ordinary and enormous sizes, and scripted keys reach it.
$H --menu --screenshot menu.png | q "saved menu.png"
$H --menu --press "Down,Down,Down,Right,PageUp" --screenshot menu2.png | q "saved menu2.png"
$H --menu --length 4000000000 --video-width 100000 --video-frames 100000 --screenshot menu3.png | q "saved menu3.png"
test -s menu.png && test -s menu2.png && test -s menu3.png
# Books: the fifth line. Its books are a cover, a title and pages; the committed example
# book opens on its shelf when the lines have its shape, and page N turns.
$H --line books --walk "0,0" --screenshot books.png | sed -n 1p | q "BOOKS line, positional, filters: none"
out=$($H --line books --length 400 --book-pages 3 --warp tests/example_book_v1.book --walk "0,0" --press N --screenshot book-open.png)
echo "$out"
echo "$out" | sed -n 1p | q '"a tale of two cities'
echo "$out" | q "opened the book"
$H --line books --warp tests/example_book_v1.book --walk "0,0" --screenshot book-wrong.png | sed -n 1p | q "in the menu to the book's shape"
$H --line books --mode scrambled --walk "0,0" --take --screenshot book-hand.png | sed -n 1p | q "BOOKS line, scrambled"
printf '[books]\nmode = compact\n[books.title]\nfilters = title-v1\n[books.title.title-v1]\ndictionary = scowl-en-60-names-v2\n[books.pages]\nfilters = words-v2\n[books.pages.words-v2]\ndictionary = scowl-en-60-names-v2\n' > books.ini
$H --filters books.ini --line books --length 400 --book-pages 3 --warp tests/example_book_v1.book --walk "0,0" --screenshot book-compact.png | sed -n 1p | q "surviving books"
$H --filters books.ini --line books --length 400 --book-pages 3 --mode scrambled --walk "0,0" --take --screenshot book-compact2.png | sed -n 1p | q "BOOKS line, scrambled, filters: 0+1+1 compact"
# Tracks and movies (composition-v1): each line comes up in both orderings; a warp onto
# tracks splits into its units; an item saves as one unit of its line N units long; and
# a units filter that can rank makes the line compact, in either ordering.
$H --no-menu --line tracks --walk "0,0" --screenshot tracks.png | sed -n 1p | q "^TRACKS line, positional, filters: none"
$H --no-menu --line movies --mode scrambled --walk "0,0" --screenshot movies.png | sed -n 1p | q "^MOVIES line, scrambled, filters: none"
$H --no-menu --line tracks --warp "C4q E4q G4h Re B4e C5w" --walk "0,0" --screenshot trackwarp.png | sed -n 1p | q "warped: canon-notes-v1: 6 event(s), 1 unit(s)"
$H --no-menu --line tracks --settle 30 --walk "0,0" --take --save-item track.mid --screenshot tracksave.png > /dev/null
head -c 4 track.mid | q MThd
# A track or movie saved as a record (F, .track or .movie) and opened again with J from the
# binary line comes back the same item: saved again, the same record.
$H --no-menu --line tracks --settle 30 --walk "0,0" --take --save-item h.track --screenshot rec1.png > /dev/null
head -1 h.track | q "^sieve-book-v1$"
$H --no-menu --binary-length 1024 --locate h.track --press "Return,J" --save-item h2.track --screenshot rec2.png | q "opened on the TRACKS line"
cmp h.track h2.track
$H --no-menu --line movies --settle 30 --walk "0,0" --take --save-item m.movie --screenshot rec3.png > /dev/null
$H --no-menu --binary-length 4096 --locate m.movie --press "Return,J" --save-item m2.movie --screenshot rec4.png | q "opened on the MOVIES line"
cmp m.movie m2.movie
$H --no-menu --line movies --settle 30 --walk "0,0" --take --save-item movie.png --screenshot moviesave.png > /dev/null
test -s movie.png
printf '[movies]\nmode = compact\n\n[movies.units]\nfilters = neighbour-agreement-v1\n' > mv.ini
for m in positional scrambled; do
  $H --no-menu --line movies --video-width 2 --video-height 2 --video-frames 2 --filters mv.ini --mode $m --walk "0,0" --screenshot mc-$m.png | sed -n 1p | q "^MOVIES line, $m, filters: 0+0+1+0 compact"
done
# The same tracks in the sieve tool and the hallway: an address from one is the same item
# in the other, plainly and compact through the joined stack (the MIDI files identical).
S=build/sieve
out=$($S warp --line tracks --length 4 "C4q E4q G4h C5w E5q D5q C5h G4w")
for m in positional scrambled; do
  a=$(echo "$out" | awk -v m=$m '$1 == m {print $2}')
  $S read --line tracks --length 4 --mode $m "$a" --out cli-$m.mid > /dev/null 2>&1
  $H --no-menu --line tracks --notes 4 --mode $m --goto "$a" --walk "0,0" --take --save-item hall-$m.mid --screenshot tx-$m.png > /dev/null
  cmp cli-$m.mid hall-$m.mid
done
T="--track-units 2 --title-length 0 --image-width 1 --image-height 1"
printf '[tracks]\nmode = compact\n\n[tracks.joined]\nfilters = key-v1\n\n[tracks.joined.key-v1]\ntonic = A\nscale = minor-pentatonic\n' > tj.ini
out=$($S warp --line tracks --length 4 $T --filters tj.ini --compact "A4q C5q D5q E5q G5q A4q C5q E5q" 2>/dev/null)
for m in positional scrambled; do
  a=$(echo "$out" | awk -v m=$m '/compact/ {seen = 1; next} seen && $1 == m {print $2; exit}')
  $S read --line tracks --length 4 $T --filters tj.ini --compact --mode $m "$a" --out clic-$m.mid > /dev/null 2>&1
  $H --no-menu --line tracks --notes 4 $T --filters tj.ini --mode $m --goto "$a" --walk "0,0" --take --save-item hallc-$m.mid --screenshot tj-$m.png | sed -n 1p | q "^TRACKS line, $m, filters: 0+0+0+1 compact"
  cmp clic-$m.mid hallc-$m.mid
done
$H --menu --screenshot menu5.png | q "saved menu5.png"
# The main menu, its settings screens and a language file with gaps (English fills them).
$H --main-menu --screenshot main.png | q "saved main.png"
$H --line image --edge-glow --walk "0,0" --screenshot glow.png 2>&1 | q "graphics: edge glow on, real graphics off"
# Real Graphics: a line loads its own item model where there is one (data/meshes/book-audio.obj
# here) and the templates for the rest.
out=$($H --line audio --real-graphics --walk "0,0" --screenshot real.png 2>&1)
echo "$out" | q "real graphics for the audio line: hallway=hallway.obj bookshelf=bookshelf.obj book=book-audio.obj marker=marker.obj"
# Tracks and movies have models of their own (begun as copies of audio's and video's).
for l in tracks movies; do
  $H --line $l --real-graphics --walk "0,0" --screenshot real-$l.png 2>&1 | q "real graphics for the $l line: .* book=book-$l.obj"
done
out=$($H --real-graphics --fps-counter --bench 5 --walk "0,0" --screenshot fps.png 2>&1)
echo "$out" | q "^bench: 5 frames"
echo "$out" | q "graphics: edge glow off, real graphics on, door portals on, fps counter on"
# Door Portals: Real Graphics brings them, and they also stand alone with the wireframe.
$H --line image --door-portals --walk "0,0" --screenshot portal.png 2>&1 | q "door portals on"
$H --line image --edge-glow --walk "0,0" --screenshot noportal.png 2>&1 | q "door portals off"
test -s portal.png && test -s noportal.png
$H --main-menu --press "Down,Down,Down,Down,Down,Return,Return,Return" --screenshot main-res.png | q "saved"
# Settings, Graphics, then VSync off, Geometry Edge Glow on, Real Graphics on — which
# turns the glow back off, the two being mutually exclusive. From a settings file that
# does not exist yet, so the toggles start from their defaults whatever ran before.
rm -f app.ini
$H --main-menu --settings app.ini --press "Down,Down,Down,Down,Down,Return,Return,Down,Down,Return,Down,Return,Down,Return" --screenshot main-gfx.png > /dev/null
grep -q "^vsync = off" app.ini
grep -q "^edge_glow = off" app.ini
grep -q "^real_graphics = on" app.ini
grep -q "^door_portals = on" app.ini   # Real Graphics brings Door Portals with it
# View Distance and Picture Distance, the two rows after the FPS counter, save.
rm -f vr.ini
$H --main-menu --settings vr.ini --press "Down,Down,Down,Down,Down,Return,Return,Down,Down,Down,Down,Down,Down,Down,Down,Right,Down,Right,Right" --screenshot main-view.png > /dev/null
grep -q "^view_rooms = 8" vr.ini
grep -q "^picture_rooms = 3" vr.ini
mkdir -p lt/data/lang && cp data/lang/en.txt lt/data/lang/ && printf 'language.name = Test\nitem.start = START TEST\n' > lt/data/lang/tt.txt
(cd lt && ../$H --main-menu --language tt --screenshot ../main-tt.png) | q "main menu: language tt, font sieve8x8"
# The language's font: sieve8x8 draws accented and Greek letters; a missing font is reported.
mkdir -p lt/data/fonts && cp data/fonts/sieve8x8.hex lt/data/fonts/
printf 'font = sieve8x8\nlanguage.name = Fran\xc3\xa7ais\nitem.start = D\xc3\xa9marrer Sieve\n' > lt/data/lang/fr.txt
out=$(cd lt && ../$H --main-menu --language fr --screenshot ../main-fr.png 2>&1)
echo "$out" | q "main menu: language fr, font sieve8x8"
if echo "$out" | grep -c "not found" > /dev/null; then exit 1; fi
printf 'font = nosuchfont\nitem.start = X\n' > lt/data/lang/xx.txt
out=$(cd lt && ../$H --main-menu --language xx --screenshot ../main-xx.png 2>&1)
echo "$out" | q "font 'nosuchfont' not found"
echo "$out" | q "main menu: language xx, font sieve8x8"
# Filter modes: compact shelves only survivors (warped text lands on a shelf, not in hand),
# an unrankable stack falls back to hide, and the filter overlay renders.
printf '[text]\nmode = compact\nfilters = words-v2\n[text.words-v2]\ndictionary = scowl-en-60-names-v2\n' > words2.ini
printf '[text]\nmode = compact\nfilters = words-v2, max-run-v1\n[text.words-v2]\ndictionary = scowl-en-60-names-v2\n' > mixed.ini
printf '[text]\nmode = mark\nfilters = words-v1\n[text.words-v1]\ndictionary = scowl-en-60-names-v2\n' > mark.ini
printf '[audio]\nmode = compact\nfilters = key-v1\n[audio.key-v1]\ntonic = A\nscale = minor-pentatonic\n[image]\nmode = compact\nfilters = symbol-entropy-v1\n[image.symbol-entropy-v1]\nmax_millibits = 500\n' > lines.ini
out=$($H --filters words2.ini --warp "$W" --walk "0,0" --screenshot compact.png)
echo "$out"
echo "$out" | q "filters: 1 compact (543295...0598 (27 digits) survivors), tile 221221...4158 (25 digits)"
$H --filters mixed.ini --walk "0,0" --screenshot mixed.png | q "filters: 2 hide (compact: words-v2 can rank, but not together with max-run-v1)"
$H --filters mark.ini --length 3 --walk "0,0" --screenshot mark.png | q "filters: 1 mark"
$H --menu --filters words2.ini --press "F" --screenshot overlay.png | q "saved overlay.png"
# Compact in the scrambled and guided orderings, and on the image and audio lines.
$H --filters words2.ini --mode scrambled --warp "$W" --walk "0,0" --screenshot cs.png | sed -n 1p | q "PAGES line, scrambled, filters: 1 compact"
$H --filters words2.ini --mode guided --warp "$W" --walk "0,0" --screenshot cg.png | sed -n 1p | q "PAGES line, guided zoom 58, filters: 1 compact"
$H --filters lines.ini --line image --mode scrambled --walk "0,0" --screenshot ci.png | sed -n 1p | q "IMAGE line, scrambled, filters: 1 compact (322091425582872 survivors)"
$H --filters lines.ini --line audio --walk "0,0" --screenshot ca.png | sed -n 1p | q "AUDIO line, positional, filters: 1 compact"
# A notes2 audio line of two voices, its plugins judging each voice, in compact order.
printf '[audio]\nmode = compact\nfilters = key-data-v2, melody-leap-v1\n[audio.key-data-v2]\ntonic = D\n[audio.melody-leap-v1]\nleap = 2\n' > n2h.ini
$H --filters n2h.ini --line audio --note-set notes2 --voices 2 --notes 3 --walk "0,0" --screenshot n2a.png | sed -n 1p | q "AUDIO line, positional, filters: 2 compact (47572844544 survivors)"
# One line filtered by another. The CLI's counts are the oracle's (the vector files'
# rows): not-written-v1 on 32-letter pages, alone and with max-run-data-v1; binary-kind-v1
# on a 33-byte binary line. The kinds a file says it is, from locate and check.
printf "[text]\nmode = compact\nfilters = not-written-v1\n" > nw.ini
want=$(awk -F'\t' '$1 == "count" && $2 == "lower27" && $3 == "all" && $4 == "32" && NF == 5 {print $5}' tests/vectors_written_v1.tsv)
build/sieve filters --length 32 --filters nw.ini | q "^survivors    $want (exact; compact mode available)"
build/sieve filters --length 32 --filters nw.ini | q "^share        the stack keeps about all of the line and sets aside 10\^-4.23"
printf "[text]\nmode = compact\nfilters = not-written-v1, max-run-data-v1\n[text.max-run-data-v1]\nmax = 2\n" > nw2.ini
want=$(awk -F'\t' '$1 == "count" && $6 == "max-run-data-v1" && $7 == "max=2" {print $5}' tests/vectors_written_v1.tsv)
build/sieve filters --length 16 --filters nw2.ini | q "^survivors    $want (exact; compact mode available)"
build/sieve check --length 32 --filters nw.ini "enfk" | q "stack: fails at not-written-v1"
build/sieve check --length 32 --filters nw.ini "hello world" | q "stack: passes, survivor number"
printf "[binary]\nmode = compact\nfilters = binary-kind-v1\n[binary.binary-kind-v1]\nkinds = png\n" > bk.ini
want=$(awk -F'\t' '$1 == "count" && $2 == "33" && $3 == "png" && $4 == "keep" {print $5}' tests/vectors_kinds_v1.tsv)
build/sieve filters --line binary --length 33 --filters bk.ini | q "^survivors    $want (exact; compact mode available)"
build/sieve filters --line binary --length 32 | q "^  TXT  *10\^-0.87"
printf 'MThd\0\0\0\6' > k.mid
build/sieve locate k.mid | q "^kind      MID (file-kinds-v1, from its first bytes)"
build/sieve check --line binary --file k.mid --filters bk.ini | q "stack: fails at binary-kind-v1"
# In the hallway: the binary line compacted to PNGs, every file on the shelves a PNG; J
# from a page to its file on the binary line and back; and from a picture, a PNG.
$H --no-menu --line binary --filters bk.ini --walk "0,0" --screenshot binpng.png | sed -n 1p | q "^BINARY line, positional, filters: 1 compact .* PNG 32 bytes: 89 50 4e 47"
$H --no-menu --warp "hello world" --press "J" --screenshot j1.png | q "^BINARY line.*its file: TXT, 32 bytes, on the binary line"
$H --no-menu --warp "hello world" --press "J,J" --screenshot j2.png | q "^PAGES line.*a TXT file, opened on the PAGES line"
$H --no-menu --line image --walk "0,0" --press "E,J,J" --screenshot j3.png | q "^IMAGE line.*a PNG file, opened on the IMAGE line"
$H --no-menu --line audio --warp "C4q E4q G4h Re B4e C5w" --press "J,J" --screenshot j4.png | q "^AUDIO line.*a MID file, opened on the AUDIO line"
# The item viewer: a click on the page in hand opens all of it, laid out as the picture on
# the item lays it out (52 columns of 77 rows, in cells 8 x 10); Z opens it too, Shift+=
# doubles the zoom, and Esc closes it with the page still in hand.
$H --no-menu --length 4000 --pose 0.9,3,94,-30 --press "E,Click" --screenshot view.png | q "viewing its TEXT, 416 x 770 at one to one"
$H --no-menu --length 4000 --pose 0.9,3,94,-30 --press "E,Z,Shift+=,End" --screenshot view2.png | tail -2 | sed -n 1p | q "viewing at 400%"
$H --no-menu --length 4000 --pose 0.9,3,94,-30 --press "E,Z,Escape" --screenshot view3.png | tail -2 | sed -n 1p > view3.txt
if grep -q "viewing at" view3.txt; then echo "Esc left the viewer open"; exit 1; fi
# Its views (Tab): the picture on the item, drawn again and saved as a PNG (F, here
# --save-view); a book's cover and its title page, saved as a PNG and as text.
$H --no-menu --length 4000 --pose 0.9,3,94,-30 --press "E,Z,Tab" --save-view vpic.png --screenshot vpic-shot.png | q "viewing its PICTURE"
test "$(head -c 4 vpic.png | tail -c 3)" = "PNG"
$H --no-menu --line books --mode scrambled --pose 0.9,3,94,-30 --press "E,Z,Tab,Tab" --save-view vcover.png --screenshot vcover-shot.png | q "viewing its COVER, 10 x 10"
test "$(head -c 4 vcover.png | tail -c 3)" = "PNG"
$H --no-menu --line books --mode scrambled --pose 0.9,3,94,-30 --press "E,Z,Tab,Tab,Tab" --save-view vtitle.txt --screenshot vtitle-shot.png | q "viewing its TITLE"
test -s vtitle.txt
# Every line filtered by every other (FILTER-PLUGINS.md section 16): the CLI's counts are
# the oracle's cross-vectors; a page's file fails not-an-item-v1 on the binary line, and
# the hallway says why the binary line cannot compact with every item form asked for.
printf "[text]\nmode = compact\nfilters = not-a-file-v1\n" > naf.ini
want=$(awk -F'\t' '$1 == "not-a-file" && $2 == "27" && $3 == "32" {print $4}' tests/vectors_cross_v1.tsv)
build/sieve filters --length 32 --filters naf.ini | q "^survivors    $want (exact; compact mode available)"
printf "[image]\nmode = compact\nfilters = not-packed-v1\n" > np.ini
want=$(awk -F'\t' '$1 == "packed" && $2 == "2" && $3 == "100" {print $4}' tests/vectors_cross_v1.tsv)
build/sieve filters --line image --filters np.ini | q "^survivors    $want (exact; compact mode available)"
printf "[binary]\nmode = compact\nfilters = binary-kind-v1, not-an-item-v1\n[binary.binary-kind-v1]\nkinds = signed\nkeep = exclude\n" > ni.ini
want=$(awk -F'\t' '$1 == "item-pages" && $2 == "40" {print $7}' tests/vectors_cross_v1.tsv)
build/sieve filters --line binary --length 40 --alphabet ascii95 --page-length 20 --filters ni.ini | q "^survivors    $want (exact; compact mode available)"
printf 'hello world, 20 char' > page.txt
build/sieve check --line binary --length 40 --alphabet ascii95 --page-length 20 --file page.txt --filters ni.ini | q "stack: fails at not-an-item-v1"
# The models line's own stack: not-a-file-v1 on a model's number, counted as the oracle
# counts it, a cube's verdict and compact address, and the line compacted in the hallway.
# The cube (as 04-models writes it): each step makes what it needs, so it can run alone.
printf 'v -1 -1 -1\nv 1 -1 -1\nv 1 1 -1\nv -1 1 -1\nv -1 -1 1\nv 1 -1 1\nv 1 1 1\nv -1 1 1\n' > cube.obj
printf 'f 1 2 3\nf 1 3 4\nf 5 7 6\nf 5 8 7\nf 1 5 6\nf 1 6 2\nf 2 6 7\nf 2 7 3\nf 3 7 8\nf 3 8 4\nf 4 8 5\nf 4 5 1\n' >> cube.obj
printf "[models]\nmode = compact\nfilters = not-a-file-v1\n" > mf.ini
want=$(awk -F'\t' '$1 == "models" && $2 == "8" && $3 == "12" && $4 == "16" {print $5}' tests/vectors_cross_v1.tsv)
build/sieve filters --line models --filters mf.ini | q "^survivors    $want (exact; compact mode available)"
build/sieve mesh --warp cube.obj --filters mf.ini | q "^stack: passes, survivor number"
$H --no-menu --line models --filters mf.ini --walk "0,0" --screenshot mcompact.png | q "^MODELS line, positional, filters: 1 compact"
# And the line's own filters: counted as the oracle's face walk counts them, a cube passes
# all three, and the hallway's compact line holds only well-formed models.
printf "[models]\nmode = compact\nfilters = distinct-vertices-v1, distinct-indices-v1, every-vertex-used-v1\n" > mr.ini
want=$(awk -F'\t' '$1 == "mesh" && $2 == "8" && $3 == "12" && $4 == "16" && $5 == "viu" {print $6}' tests/vectors_cross_v1.tsv)
build/sieve filters --line models --filters mr.ini | q "^survivors    $want (exact; compact mode available)"
build/sieve mesh --warp cube.obj --filters mr.ini | q "^stack: passes, survivor number"
$H --no-menu --line models --filters mr.ini --walk "0,0" --take --screenshot mrules.png | q "8/8 vertices used, 0 degenerate faces"
# not-a-pattern-v1: counted as the oracle counts it (by inclusion and exclusion over the
# block lengths), and a ramp through the alphabet fails while text passes.
printf "[text]\nmode = compact\nfilters = not-a-pattern-v1\n" > pat.ini
want=$(awk -F'\t' '$1 == "pattern" && $2 == "27" && $3 == "32" && $4 == "1" {print $8}' tests/vectors_cross_v1.tsv)
build/sieve filters --length 32 --filters pat.ini | q "^survivors    $want (exact; compact mode available)"
build/sieve check --length 26 --filters pat.ini --model none "abcdefghijklmnopqrstuvwxyz" | q "stack: fails at not-a-pattern-v1"
build/sieve check --length 26 --filters pat.ini --model none "it was the best of times it" | q "stack: passes"
printf "[binary]\nmode = compact\nfilters = not-an-item-v1\n[binary.not-an-item-v1]\nitems = all\n" > nia.ini
$H --no-menu --line binary --filters nia.ini --walk "0,0" --screenshot nia.png | q "^BINARY line, positional, filters: 1 hide (compact: not-an-item-v1 judges models file by file"
test -s compact.png && test -s mixed.png && test -s mark.png && test -s overlay.png
