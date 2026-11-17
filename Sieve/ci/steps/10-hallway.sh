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
exe=build/sieve
[ -f build/Release/sieve.exe ] && exe=build/Release/sieve.exe
W="it was the best of times"
book() { grep -o 'first book [0-9a-f.]*' | tail -1; }
$H --screenshot hallway.png --size 640x360
# docs/SETTINGS.md is written from the code (tools/settings_doc.py, IDEAS 19.134): it must say what
# --write-settings writes, every setting, its default and what it is.
python tools/settings_doc.py "$H" --check
test -s hallway.png
# The metrics list (hallway --metrics, no window): every line under the reference filters,
# as the setup menu counts them; the binary line's binary-kind-v1 removes 99.9954% (the names padded
# to the longest, Fractals; it has its noise filters, none of them ticked there, so it keeps it all).
$H --metrics metrics.txt --filters data/metrics/reference-filters.ini > /dev/null
q "^Binary *: 99.9954%" metrics.txt
q "^Models *: 99.999999999999...% *( Kept : 10^-21.47 )" metrics.txt
[ "$(grep -c ' : .*Kept : ' metrics.txt)" = 12 ]
# ...and what each ordering saves: nothing in positional or scrambled order, what the
# filters remove off every survivor in compact order, and the guided ordering's bound and
# what it did on the reference text (Dickens, which the model was not trained on).
q "^Positional and scrambled : 0 bits, on every line" metrics.txt
q "^Binary *: compact [0-9.]* bits off every survivor" metrics.txt
q "^Pages *: guided: no minimum" metrics.txt
q "^Pages *: guided on reference-text.txt (SHA-256 [0-9a-f]*\.\.\., [0-9]* pages): [0-9.]* bits a page" metrics.txt
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
# Files already there, asked about one at a time (--answers stands in for the person): Zebra.txt
# replaced (y), a.txt left as it was (n), the rest written; then Replace writes over them all.
rm -rf fx_ask
mkdir fx_ask
echo OLD > fx_ask/a.txt
echo OLD > fx_ask/Zebra.txt
build/sieve-install fx.sieve --to fx_ask --answers yn > fx_ask.txt
grep -q "a.txt was already there and is left as it was" fx_ask.txt
test "$(cat fx_ask/a.txt)" = OLD
cmp fx_ask/Zebra.txt tests/manifest_fixture/Zebra.txt
cmp "fx_ask/sub dir/b.bin" "tests/manifest_fixture/sub dir/b.bin"
build/sieve-install fx.sieve --to fx_ask --yes --replace
diff -r tests/manifest_fixture fx_ask
head -c 100 fx.sieve > broken.sieve
if build/sieve-install broken.sieve --to fx_broken --yes; then exit 1; fi
test ! -e fx_broken
rm -rf fx4 fx4_installed
cp -r tests/manifest_fixture fx4
cp build/sieve fx4/program
seq 1 20000 > fx4/all.txt
awk 'NR % 3 == 0' fx4/all.txt > fx4/third.txt
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
# sieve-view, the picture viewer (2 November 2026): a PNG read (its header, then its pixels); made
# into NAME.simage by the sieve beside it (Prioritise Decoding Speed) and read back from that; a
# .sieve holding texts refused by its listing, before anything is decoded; a PNG whose header says
# 100,000 by 100,000 pixels refused by its header; with the memory it may take held to 4 MB
# (--budget), the Vitruvian JPEG shown at an eighth from its blocks' averages and a PNG refused;
# the window drawn with the .simage open.
cp docs/images/hallway-audio.png view_pic.png
rm -f view_pic.simage
build/sieve-view --check view_pic.png | tr -d '\r' | grep -q "^view_pic.png: 1280 by 720 pixels"
build/sieve-view --make-simage view_pic.png > /dev/null
test -s view_pic.simage
# Its thumbnail as Explorer is given it (sieve-thumbs.dll, 5 November 2026: the same code, shared):
# the first picture, its longer side 64.
build/sieve-view --thumbnail 64 view_thumb.png view_pic.simage | tr -d '\r' | q "^thumbnail: 64 by 36 pixels, of view_pic.png (1280 by 720)"
test -s view_thumb.png
build/sieve-view --check view_pic.simage | tr -d '\r' > view.txt
grep -q "^listing: 1 picture" view.txt
grep -q "^view_pic.png: 1280 by 720 pixels" view.txt
if build/sieve-view --check fx.sieve > view.txt; then exit 1; fi
grep -q "^refused: .*not a picture (by its recipe and its name)" view.txt
python -c "
import struct, zlib
def chunk(t, d): return struct.pack('>I', len(d)) + t + d + struct.pack('>I', zlib.crc32(t + d))
open('view_huge.png', 'wb').write(b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', 100000, 100000, 8, 2, 0, 0, 0)) + chunk(b'IDAT', zlib.compress(bytes(1000))) + chunk(b'IEND', b''))
"
if build/sieve-view --check view_huge.png > view.txt; then exit 1; fi
grep -q "^refused: view_huge.png: 100000 by 100000 pixels: its pixels need about" view.txt
build/sieve-view --check data/anchors/public-domain/Vitruvian.jpg --budget 4000000 | tr -d '\r' | grep -q "^Vitruvian.jpg: 949 by 1300 pixels, .*, shown at an eighth (119 by 163)"
if build/sieve-view --check view_pic.png --budget 4000000 > view.txt; then exit 1; fi
grep -q "(only a JPEG is shown reduced)" view.txt
build/sieve-view view_pic.simage --screenshot view.bmp
test -s view.bmp
# Archives of other kinds, extracted as an archiver extracts them (the right-click menu's Extract on
# a .zip, .rar, .tar.gz ...), one layer at a time; each made from the repository's own files.
python ci/extract_check.py build/sieve-install extract_check
# A release's layout: sieve and sieve-install in tools/ below the data folders. sieve there
# finds the dictionaries and models in the folder above, and still makes installer
# programs from the sieve-install beside it.
rm -rf layout
mkdir -p layout/tools
cp build/sieve build/sieve-install layout/tools/
cp -r build/dictionaries build/models layout/
(cd layout/tools && ./sieve dicts | grep -q "layout/dictionaries/dictionaries.tsv")
(cd layout/tools && ./sieve models | grep -q "layout/models/models.tsv")
layout/tools/sieve locate tests/manifest_fixture --program layout-installer --v4 > /dev/null
rm -rf fx_layout
./layout-installer --to fx_layout --yes
diff -r tests/manifest_fixture fx_layout
# An installer program: sieve-install with the installer attached, run on its own, and
# read by sieve install; exactly the program, the dictionaries folder it carries and their
# 24-byte trailer, the installer and its 24-byte trailer; and made from a program that already
# has one attached, the old one replaced, not added to.
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
# Keep a log: an install writes one too, in logs/ beside the settings, with its times.
rm -rf fx_gui_log ilog
mkdir ilog
$H --no-menu --settings ilog/s.ini --locate-log --install fx.sieve --install-to fx_gui_log --screenshot ilog.png | q "saved ilog.png"
grep -q "decoded in .*, checked and written in" ilog/logs/locate-*-fx.sieve.txt
grep -q "^took .* s in all" ilog/logs/locate-*-fx.sieve.txt
grep -q "sieve-carried-d1" fx-installer
carried=$(python3 -I -c "import sys; f=open(sys.argv[1],'rb').read(); n=int.from_bytes(f[-24:-16],'little'); e=len(f)-24-n; print(int.from_bytes(f[e-24:e-16],'little'))" fx-installer)
test "$(wc -c < fx-installer)" -eq "$(( $(wc -c < build/sieve-install) + carried + 24 + $(wc -c < fx.sieve) + 24 ))"
tail -c "$(( $(wc -c < fx.sieve) + 24 ))" fx-installer | head -c "$(wc -c < fx.sieve)" | cmp - fx.sieve
# (5 November 2026, the release's sieve.exe: files mixed with a word list, the program run on its
# own in a folder with no dictionaries anywhere near: it installs them from the ones it carries,
# and leaves nothing of them behind; without them, it could not.)
rm -rf alone
mkdir -p alone/src alone/run
head -c 8192 docs/MANUAL.md > alone/src/text.md
(cd alone && ../build/sieve locate src --installer words.sieve --program words-setup --items > made.txt 2>&1)
grep -q "with a word list" alone/made.txt
grep -q "^          dictionaries: [0-9]* files of " alone/made.txt
cp alone/words-setup alone/run/
(cd alone/run && TMPDIR="$PWD/tmp" && mkdir tmp && export TMPDIR && ./words-setup --to out --yes)
diff -r alone/src alone/run/out
test -z "$(ls alone/run/tmp)"
# The folder an installer program offers (0.13.78): made with --install-folder, run with no --to,
# it installs there; folders not allowed are refused when it is made. Run on Linux only, where Home
# is $HOME and so this run's own folder: Windows and macOS ask the system for the person's home,
# and it would be written there.
rm -rf offer
mkdir -p offer/home
(cd alone && ../build/sieve locate src --program ../offer/offer-setup --install-folder "Home/Games/My Thing" --v4 > ../offer/made.txt 2>&1)
grep -q "^          offers to install into: Home/Games/My Thing" offer/made.txt
if [ "$(uname -s)" = Linux ]; then
    (cd offer && HOME="$PWD/home" ./offer-setup --yes)
    diff -r alone/src "offer/home/Games/My Thing"
fi
for bad in "C:/Windows" "Documents" "Documents/../x" "Documents/CON/x" "Somewhere/x"; do
    if build/sieve locate alone/src --program offer/bad-setup --install-folder "$bad" --v4 > offer/bad.txt 2>&1; then exit 1; fi
    grep -q "the folder to offer, " offer/bad.txt
done
[ ! -e offer/bad-setup ]
# More formats in the comparison (0.13.78): --more-formats adds gzip, bzip2, zstd, brotli and lz4
# beside zip and 7z; bzip2's figure is bzip2 -9's own file, byte for byte (its stream is the file).
build/sieve locate docs/MANUAL.md --compare --more-formats > more.txt
for f in "gzip -9 (as .tar.gz)" "bzip2 -9 (as .tar.bz2)" "zstd -19 --long=27" "brotli -q 11 -w 24" "lz4 -12"; do grep -qF "  $f " more.txt; done
if command -v bzip2 > /dev/null; then grep -qE "^  bzip2 -9 \(as .tar.bz2\) +$(bzip2 -9 -c docs/MANUAL.md | wc -c | tr -d ' ') bytes" more.txt; fi
build/sieve locate docs/MANUAL.md --compare > fewer.txt
if grep -q "bzip2" fewer.txt; then exit 1; fi
rm -rf again
mkdir again
cp build/sieve again/
cp fx-installer again/sieve-install
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
# Text to Voice (9 November 2026): the reply spoken, as text that can be said (a machine with no
# voice says nothing; the scripted run prints it); and its row in Settings, Controls.
$H --no-menu --line ai --walk "0,0" --take --talk "Hello there" --text-to-voice --screenshot ai-voice.png | tr -d '\r' | q "^voice: "
# CHAT's /say: the words said aloud whatever the setting (11 November 2026).
$H --no-menu --line ai --walk "0,0" --take --chat --talk "/say Hello from the gallery" --training-dir data/training --screenshot chat-say.png | tr -d '\r' | q "^voice: Hello from the gallery$"
$H --main-menu --read-aloud --press "Item:settings,Return,Item:controls,Return,Item:text_to_voice" --screenshot tv-row.png | tr -d '\r' | q "^read aloud: Text to Voice"
# Sieve's own voice (results/voice-units-2026-11-09.md): a text read into its speech sounds by
# CMUdict (a number as words), said, and said the same again, sample for sample.
build/sieve speak Hello, 42 times. --phones --out speak1.wav | tr -d '\r' > speak.txt
q "^SIL HH AH L OW SIL F AO R T IY T UW T AY M Z SIL $" speak.txt
q "from 1022 diphones' 33750 takes" speak.txt
build/sieve speak Hello, 42 times. --out speak2.wav > /dev/null
cmp speak1.wav speak2.wav
[ "$(wc -c < speak1.wav)" -gt 20000 ]
# A word CMUdict lacks, by the letter-to-sound rules learnt from it (data/voice/lj-speech/lts.tsv).
build/sieve speak Marshalsea --phones --out speak3.wav | tr -d '\r' | q "^SIL M AA R SH AH L S IY SIL $"
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
head -c 30 data/anchors/public-domain/universalbibliothek.txt > cut.txt
printf '\303' >> cut.txt
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
# A stage steered by the filters (sieve-chain-v2, 12 November 2026): with row-runs-v1 ticked on the
# image line (10x10 mono, the figures model's own shape), its tries go seed, seed + 1, ... and the
# first that passes is kept; the same every time. F in the CHAIN tab steers a stage, saved as v2.
printf '[image]\nmode = mark\nfilters = row-runs-v1\n' > steer.ini
printf 'sieve-chain-v2\nsteer figures-2x32\ndraw figures-2x32\n' > steer.chain
$H --no-menu --line ai --walk "0,0" --take --chat --chain-file steer.chain --filters steer.ini --talk "/chain hello" --training-dir data/training --screenshot chain-steer.png | tr -d '\r' > chainsteer.txt
q "^chain 1/2, figures-2x32, steered by the filters (steered: try 130 passed the line's filters): fff87c0781e1f00e1e01e3fff$" chainsteer.txt
q "^chain 2/2, figures-2x32: fff87f0f61e1605ece07e9fff$" chainsteer.txt
$H --no-menu --line ai --walk "0,0" --take --chain --chain-press "Down,F" --chain-save chain2.chain --training-dir data/training --screenshot chain-tab2.png | q "saved chain-tab2.png"
printf 'sieve-chain-v2\nrun universalbibliothek\nsteer figures-2x32\ndraw melodies-2x64\ndraw type-2x32\n' | cmp - chain2.chain
$H --no-menu --line ai --walk "0,0" --take --chat --picture-model data/training/figures-2x32 --chat-draw 1 --training-dir data/training --screenshot chat-pm.png | tr -d '\r' | q "^figure 1 by figures-2x32: fff0fc2f0de8f01c3fdbe6fff"
printf '[models]\ntext_model =\npicture_model =\n' > models.ini
downs=$(python -c "print(','.join(['Down']*46))")
# Empty means the first trained run (figures-2x32); Right steps to the next (melodies-2x64), Left back.
$H --menu --settings models.ini --training-dir data/training --press "$downs,Right" --screenshot menu-models.png | q "saved menu-models.png"
q "^picture_model = .*melodies-2x64" models.ini
$H --menu --settings models.ini --training-dir data/training --press "$downs,Left" --screenshot menu-models.png | q "saved menu-models.png"
q "^picture_model = .*figures-2x32" models.ini
# The right-hand panel (IDEAS 19.148): the key left of 1, read by its place, opens it and closes it,
# and on the key's row (where letters are typed) it still opens it.
$H --menu --press "\`" --screenshot side-open.png | tr -d '\r' | q "^right-hand panel: open (CHARACTER)"
$H --menu --press "\`,\`" --screenshot side-closed.png | tr -d '\r' > side.txt
if grep -q "right-hand panel" side.txt; then exit 1; fi
$H --menu --press "Down,Down,\`" --screenshot side-key-row.png | tr -d '\r' | q "^right-hand panel: open (CHARACTER)"
# Its CHARACTER page (sieve-character-v1): the default character (seed 0, four limbs of three joints,
# bilateral, a head), and with six limbs of four joints, each named by its SHA-256.
$H --menu --press "\`" --screenshot char-default.png | tr -d '\r' | q "^character: e5ba98461383a7fae83b0e070885b593653c80e1a5e17d79982abc26a12cfcb7"
$H --menu --press "\`,Down,Right,Right,Down,Right" --screenshot char-6x4.png | tr -d '\r' | q "^character: 455b1fd6a86a035c41f4ec6f1d4d2ce60869595fc67c30e4a913397d19f38972"
# A .character file (its settings line alone is enough): opened on the CHARACTER page and beside
# you in the hallway as the same character; a file missing a setting is refused, and the run stops.
printf 'sieve-character-v1 seed=99 limbs=5 joints=3 branches=1 symmetry=bilateral head=yes\n' > c99.character
$H --menu --character c99.character --press "\`" --screenshot char-file.png | tr -d '\r' | q "^character: cb53aba8a6f47e41e897571882c364c61fb34a14375379026c6bc042f21af0a2"
$H --no-menu --character c99.character --screenshot char-hall.png | tr -d '\r' | q "^your character: cb53aba8a6f47e41e897571882c364c61fb34a14375379026c6bc042f21af0a2"
# A path longer than a string keeps in itself (16 characters and more) read whole (14 November 2026: the
# name was taken from two different copies of it, which a short one hid).
mkdir -p cx
$H --no-menu --character cx/../c99.character --screenshot char-long.png | tr -d '\r' | q "^your character: cb53aba8a6f47e41e897571882c364c61fb34a14375379026c6bc042f21af0a2"
# The hallway fits the character (IDEAS 19.153): this one fits as built; eight radial limbs of twelve
# joints grow it 1.41 times.
$H --no-menu --character c99.character --screenshot char-hall.png | tr -d '\r' | q "^hallway scale: 1.00$"
printf 'sieve-character-v1 seed=5 limbs=8 joints=12 branches=3 symmetry=radial head=yes\n' > big.character
$H --no-menu --character big.character --screenshot char-big.png | tr -d '\r' | q "^hallway scale: 1.41$"
printf 'sieve-character-v1 seed=99 limbs=5\n' > bad.character
if $H --no-menu --character bad.character --screenshot char-bad.png > char-bad.txt 2>&1; then exit 1; fi
q "missing: joints" char-bad.txt
# A character holding models (sieve-character-v2): read from its file, the same on the page and
# in the hallway, and its models drawn in their spheres.
printf 'sieve-character-v2 seed=0 limbs=6 joints=4 branches=0 symmetry=bilateral head=yes\nh 0 230d8358dc8e8890b4c58deeb62912ee2f20357ae92a5cc861b\nh 1 9f2e6d33a3717ee826353a404ba4618d1aeeb6879ad7936bce8\n' > held.character
$H --menu --character held.character --press "\`" --screenshot char-held.png | tr -d '\r' | q "^character: e4ba9371be670f52ca007fd12da9178b396c89e47a0ce317cbb7021ccecc828e"
$H --no-menu --character held.character --screenshot char-held-hall.png | tr -d '\r' | q "^your character: e4ba9371be670f52ca007fd12da9178b396c89e47a0ce317cbb7021ccecc828e"
# Personal Dimensions (IDEAS 19.157): the main menu's screen; YOURS listing a store of three characters
# (beside the settings file); and the same store standing in the hallway.
$H --main-menu --press "Item:personal,Return" --screenshot main-personal.png | q "saved main-personal.png"
mkdir -p pdim/characters
for spec in "seed=1 limbs=4 joints=3 branches=0 symmetry=bilateral head=yes" "seed=2 limbs=6 joints=2 branches=1 symmetry=radial head=no"; do
  printf 'sieve-character-v1 %s\n' "$spec" > pdim/one.character
  id=$($H --menu --character pdim/one.character --press "\`" --screenshot pdim-one.png | tr -d '\r' | sed -n 's/^character: //p')
  test -n "$id"
  mv pdim/one.character "pdim/characters/$id.character"
done
printf 'not a character\n' > pdim/characters/bad.character
touch pdim/s.ini
$H --menu --settings pdim/s.ini --press "\`,PageDown,PageDown" --screenshot pdim-yours.png | tr -d '\r' | q "^right-hand panel: open (YOURS, 2 characters)"
$H --no-menu --character-store pdim/characters --screenshot pdim-hall.png | tr -d '\r' | q "^personal dimension: 2 characters, 1 left out"
# The playground (IDEAS 19.152): P fills it along the seed walk from the default character; K keeps the
# second in a fresh store, by its SHA-256.
mkdir -p play && touch play/s.ini
$H --menu --settings play/s.ini --press "\`,PageDown,PageDown,P,Down,K" --screenshot play.png | tr -d '\r' | q "^right-hand panel: open (YOURS, 1 characters)"
test -f play/characters/768169973d3190fd9cfa2c16eab7161e944d50ec85ba469411fc306ee0fc4d5d.character
# Importing a model (IDEAS 19.159, .obj first): a cube into the first sphere, the same character each run.
printf 'v -1 -1 -1\nv 1 -1 -1\nv 1 1 -1\nv -1 1 -1\nv -1 -1 1\nv 1 -1 1\nv 1 1 1\nv -1 1 1\nf 1 2 3 4\nf 5 8 7 6\nf 1 5 6 2\nf 2 6 7 3\nf 3 7 8 4\nf 5 1 4 8\n' > cube.obj
$H --menu --import-obj cube.obj --press "\`" --screenshot char-cube.png | tr -d '\r' | q "^character: c202ce801122dd88e98ed455be1cc39dd81b7df7720ccddd6b48b4dea78ef196"
# A glTF's skeleton as your character (IDEAS 19.170), its animation held at its last frame; the
# settings file keeps the whole character on one line, so the next start reads the same one back.
rm -f arm.ini
$H --menu --settings arm.ini --import-skeleton tests/arm.gltf --animation-frame 30 --press "\`" --screenshot char-arm.png | tr -d '\r' > arm-menu.txt
q "^character: 22807d23fa4d9b7abfb2895e2e97448ad179cd0f0740b5effa13594dc87b78a7" arm-menu.txt
q "^animation: 2a5cb1d00e4ace0f373757b7d29ef342a157d8d27a19bd7fafccaef5a5b54be8 31 frames, 3 bones" arm-menu.txt
grep -q "^line = sieve-character-v3 spheres=3;s 0 -1 0 0 0 1365 body;" arm.ini
$H --menu --settings arm.ini --press "\`" --screenshot char-arm2.png | tr -d '\r' | q "^character: 22807d23fa4d9b7abfb2895e2e97448ad179cd0f0740b5effa13594dc87b78a7"
# The character creator and actions to animations (IDEAS 19.186): on the main menu after the System
# Viewer, and from the CHARACTER page with C. With the walking arm as the character and the arm's
# wave in the store too, idle is given the wave and walk the walk (Left/Right go through none and the
# animations that fit, in the order of their names); the map is kept (sieve-actions-v1) and named in
# the save, which becomes sieve-save-v2. In the hallway, walking plays the walk; jumping, with none,
# plays nothing.
$H --main-menu --read-aloud --press "Item:creator" --screenshot creator-row.png | tr -d '\r' | q "^read aloud: Character Creator$"
$H --menu --press "\`,c" --screenshot creator-c.png | tr -d '\r' | q "^character creator: open$"
rm -rf crt
mkdir -p crt/animations
$exe import tests/arm.gltf --animation-out crt/animations/2a5cb1d00e4ace0f373757b7d29ef342a157d8d27a19bd7fafccaef5a5b54be8.animation --no-stats > /dev/null
$H --menu --settings crt/c.ini --import-skeleton tests/walk.gltf --creator --animation-frame 15 --press "Tab,Right,Down,Right,Right,Return" --screenshot creator.png | tr -d '\r' > creator.txt
q "^actions: [0-9a-f]\{12\}: idle 2a5cb1d00e4a, walk 45adcf033beb, run none, jump none, take none, read none, wave none$" creator.txt
q "^note: walk: playing 45adcf033beb$" creator.txt
amap=$(sed -n 's/^actions = \([0-9a-f]\{64\}\).*/\1/p' crt/c.ini)
test -n "$amap"
head -1 "crt/actions/$amap.actions" | tr -d '\r' | q "^sieve-actions-v1$"
# The view tilts as well as turns (Edward, 14 November 2026), 15 degrees a Shift+] (up to 85), and Home
# puts it back as it opens: the same picture as one never turned or tilted.
$H --menu --settings crt/c.ini --creator --press "Shift+],Shift+],Shift+]" --screenshot creator-tilt.png | tr -d '\r' | q "^character view: turned 34 degrees, tilted 45 degrees$"
$H --menu --settings crt/c.ini --creator --press "Shift+[,Shift+[,Shift+[,Shift+[,Shift+[,Shift+[,Shift+[" --screenshot creator-tilt2.png | tr -d '\r' | q "^character view: turned 34 degrees, tilted -85 degrees$"
$H --menu --settings crt/c.ini --creator --animation-frame 15 --press "Shift+],],],Home" --screenshot creator-home.png | tr -d '\r' | q "^character view: turned 34 degrees, tilted 0 degrees$"
$H --menu --settings crt/c.ini --creator --animation-frame 15 --screenshot creator-still.png > /dev/null
cmp creator-home.png creator-still.png
$H --settings crt/c.ini --no-menu --action walk --animation-frame 15 --screenshot crt-walk.png | tr -d '\r' | q "^action: walk: 45adcf033beb3fc647acd80ff8053472d5f3bef3561cf2510e6208ae6894acb1$"
$H --settings crt/c.ini --no-menu --action jump --screenshot crt-jump.png | tr -d '\r' | q "^action: jump: none$"
# One drawing of characters for every space (IDEAS 19.214): Settings > Graphics > Character Drawing is
# the Creator's drawing row ([character] drawing), one step Left from shaded being voxels (since the
# skin, its quads and its smooth drawing came after it, three Rights from shaded reach the smooth
# drawing); the Creator, your dimension and the hallway then draw voxels, and say so; with Wireframe
# on (the default), each says
# it draws the wireframe; shaded (the default) says nothing, as before.
rm -rf cdr
mkdir -p cdr
touch cdr/g.ini
$H --main-menu --settings cdr/g.ini --press "Item:settings,Return,Item:graphics,Return,Item:character_drawing,Left" --screenshot cdr-graphics.png > /dev/null
grep -c "^drawing = 2 " cdr/g.ini | q "^1$"
$H --menu --real-graphics --settings cdr/g.ini --creator --screenshot cdr-creator.png | tr -d '\r' | q "^character drawing: voxels$"
$H --menu --settings cdr/g.ini --creator --screenshot cdr-creator-off.png | tr -d '\r' | q "^character drawing: voxels (the wireframe, as Wireframe is on)$"
$H --personal-world --real-graphics --settings cdr/g.ini --pworld-look -10 --screenshot cdr-pworld.png | tr -d '\r' | q "^characters: drawn as voxels, [1-9][0-9]* triangles of cubes$"
$H --personal-world --settings cdr/g.ini --pworld-look -10 --screenshot cdr-pworld-off.png | tr -d '\r' | q "^characters: drawn as wireframe (Wireframe is on; voxels chosen)$"
$H --no-menu --real-graphics --settings cdr/g.ini --character-store pdim/characters --screenshot cdr-hall.png | tr -d '\r' | q "^characters: drawn as voxels, [1-9][0-9]* triangles of cubes$"
$H --menu --real-graphics --creator --screenshot cdr-shaded.png | tr -d '\r' > cdr/shaded.txt
test "$(grep -c "^character drawing:" cdr/shaded.txt || true)" = 0 # (grep -c counts 0 and fails: "|| true" keeps its 0 and the ERR trap quiet)
# A character far wider than it is tall, drawn as voxels: its cells grow to fit its widest side in 256,
# so the hallway draws it (it asked for a grid too large to hold, and the hallway stopped, before).
rm -rf cdr/wide
mkdir -p cdr/wide
printf 'sieve-character-v3 spheres=2\ns 0 -1 0 0 0 40 body\ns 1 0 100000000 0 0 40 joint\n' > cdr/wide.character
cp cdr/wide.character "cdr/wide/$(sha256sum cdr/wide.character | cut -d' ' -f1).character"
$H --no-menu --real-graphics --settings cdr/g.ini --character-store cdr/wide --screenshot cdr-wide.png | tr -d '\r' | q "^characters: drawn as voxels, "
# The Creator's own keys are on the registry (Settings > Controls > Keys): given another key, it is that
# key that turns the view (three presses, 15 degrees each, from the 34 it opens at).
printf '[keys]\ncreator/turn_left = K\n' > cdr/k.ini
$H --menu --real-graphics --settings cdr/k.ini --creator --press "K,K,K" --screenshot cdr-k.png | tr -d '\r' | q "^character view: turned -11 degrees"
# How big a voxel is, and greedy meshing (IDEAS 19.216, 19.215): Settings > Graphics > Character Voxels
# sets [character] voxel_joined and voxel_size. Joined, the Creator draws the same picture, byte for byte,
# with fewer faces, and says how many; on the world lattice its cell is a power of two of the base, said
# with its level, and your dimension says its cubes are on the lattice and how many triangles joining
# saved. Neither chosen, the readout says nothing of either, as before.
rm -rf cvx
mkdir -p cvx
printf '[character]\ndrawing = 2\n' > cvx/a.ini
cp cvx/a.ini cvx/j.ini
$H --main-menu --settings cvx/j.ini --press "Item:settings,Return,Item:graphics,Return,Item:character_voxels,Return,Item:voxel_joined,Return" --screenshot cvx-menu.png > /dev/null
grep -c "^voxel_joined = on " cvx/j.ini | q "^1$"
$H --menu --real-graphics --settings cvx/a.ini --creator --screenshot cvx-apart.png | tr -d '\r' > cvx/a.txt
test "$(grep -c "^voxels:" cvx/a.txt || true)" = 0
$H --menu --real-graphics --settings cvx/j.ini --creator --screenshot cvx-joined.png | tr -d '\r' | q "^voxels: cell [0-9.]* units (its height in 16 cells), [1-9][0-9]* faces seen, drawn as [1-9][0-9]* rectangles$"
cmp cvx-apart.png cvx-joined.png
cp cvx/j.ini cvx/l.ini
$H --main-menu --settings cvx/l.ini --press "Item:settings,Return,Item:graphics,Return,Item:character_voxels,Return,Item:voxel_size,Right" --screenshot cvx-menu2.png > /dev/null
grep -c "^voxel_size = 1 " cvx/l.ini | q "^1$"
$H --menu --real-graphics --settings cvx/l.ini --creator --screenshot cvx-lattice.png | tr -d '\r' | q "^voxels: cell [0-9.]* units (the lattice: [0-9]* mm, level [0-9]*), [1-9][0-9]* faces seen, drawn as [1-9][0-9]* rectangles$"
$H --personal-world --real-graphics --settings cvx/l.ini --pworld-look -10 --screenshot cvx-pworld.png | tr -d '\r' | q "^characters: drawn as voxels, [1-9][0-9]* triangles of cubes ([1-9][0-9]* before joining), cells on the lattice$"
# The skin (IDEAS 19.225): one Right from shaded on Settings > Graphics > Character Drawing is the skin,
# drawing 4; Skin Blend steps by ten, and Skin Detail by one, on Settings > Graphics > Character Voxels.
# The Creator says its cell (the voxels' halved, once by default), blend, grid, triangles and the time it
# took to make; your dimension and the hallway say how many triangles of skin they drew; with Real
# Graphics off each draws the wireframe and says so. Its blend 0 is the spheres' plain union, and the
# same skin made twice is the same picture, byte for byte.
rm -rf skn
mkdir -p skn
touch skn/s.ini
$H --main-menu --settings skn/s.ini --press "Item:settings,Return,Item:graphics,Return,Item:character_drawing,Right" --screenshot skn-graphics.png > /dev/null
grep -c "^drawing = 4 " skn/s.ini | q "^1$"
cp skn/s.ini skn/b.ini
cp skn/s.ini skn/off.ini
$H --main-menu --settings skn/b.ini --press "Item:settings,Return,Item:graphics,Return,Item:character_voxels,Return,Item:skin_blend,Left,Left,Left,Left,Left,Item:skin_detail,Right" --screenshot skn-menu.png > /dev/null
grep -c "^skin_blend = 0 " skn/b.ini | q "^1$"
grep -c "^skin_detail = 2 " skn/b.ini | q "^1$"
$H --menu --real-graphics --settings skn/s.ini --character c99.character --creator --screenshot skn-creator.png | tr -d '\r' > skn/creator.txt
q "^character drawing: skin$" skn/creator.txt
q "^skin: cell 40\.2 units (the voxels' halved 1 times), blend 99\.5 units (50%), grid 79x55x82, 23180 triangles made, [1-9][0-9]* drawn, made in [0-9.]* ms" skn/creator.txt
$H --menu --real-graphics --settings skn/s.ini --character c99.character --creator --screenshot skn-creator2.png > /dev/null
cmp skn-creator.png skn-creator2.png
$H --menu --real-graphics --settings skn/b.ini --character c99.character --creator --screenshot skn-union.png | tr -d '\r' | q "^skin: cell 20\.1 units (the voxels' halved 2 times), blend 0\.0 units (0%), "
$H --menu --settings skn/off.ini --character c99.character --creator --screenshot skn-off.png | tr -d '\r' | q "^character drawing: skin (the wireframe, as Wireframe is on)$"
$H --personal-world --real-graphics --settings skn/s.ini --pworld-look -10 --screenshot skn-pworld.png | tr -d '\r' | q "^characters: drawn as skin, [1-9][0-9]* triangles of skin$"
$H --personal-world --settings skn/off.ini --pworld-look -10 --screenshot skn-pworld-off.png | tr -d '\r' | q "^characters: drawn as wireframe (Wireframe is on; skin chosen)$"
$H --no-menu --real-graphics --settings skn/s.ini --character-store pdim/characters --screenshot skn-hall.png | tr -d '\r' | q "^characters: drawn as skin, [1-9][0-9]* triangles of skin$"
# The skin's quads and its smooth drawing (IDEAS 19.214): two and three Rights from shaded on Settings >
# Graphics > Character Drawing, drawings 5 and 6. The Creator says how many quads the skin is and how many
# it drew, and for the smooth drawing how long its normals took to average across the faces; your
# dimension and the hallway say how many quads, or triangles of smooth skin, they drew; with Wireframe
# on each draws the wireframe and says so; the same drawn twice is the same picture, byte for byte.
rm -rf sqd
mkdir -p sqd
touch sqd/q.ini
touch sqd/m.ini
$H --main-menu --settings sqd/q.ini --press "Item:settings,Return,Item:graphics,Return,Item:character_drawing,Right,Right" --screenshot sqd-graphics.png > /dev/null
grep -c "^drawing = 5 " sqd/q.ini | q "^1$"
$H --main-menu --settings sqd/m.ini --press "Item:settings,Return,Item:graphics,Return,Item:character_drawing,Right,Right,Right" --screenshot sqd-graphics2.png > /dev/null
grep -c "^drawing = 6 " sqd/m.ini | q "^1$"
cp sqd/q.ini sqd/off.ini
$H --menu --real-graphics --settings sqd/q.ini --character c99.character --creator --screenshot sqd-quads.png | tr -d '\r' > sqd/quads.txt
q "^character drawing: quads$" sqd/quads.txt
q "^skin: cell 40\.2 units (the voxels' halved 1 times), blend 99\.5 units (50%), grid 79x55x82, 11590 quads made, [1-9][0-9]* drawn, made in [0-9.]* ms" sqd/quads.txt
$H --menu --real-graphics --settings sqd/q.ini --character c99.character --creator --screenshot sqd-quads2.png > /dev/null
cmp sqd-quads.png sqd-quads2.png
$H --menu --real-graphics --settings sqd/m.ini --character c99.character --creator --screenshot sqd-smooth.png | tr -d '\r' > sqd/smooth.txt
q "^character drawing: smooth$" sqd/smooth.txt
q "^skin: cell 40\.2 units (the voxels' halved 1 times), blend 99\.5 units (50%), grid 79x55x82, 23180 triangles made, [1-9][0-9]* drawn, made in [0-9.]* ms.*, normals averaged in [0-9.]* ms$" sqd/smooth.txt
$H --menu --real-graphics --settings sqd/m.ini --character c99.character --creator --screenshot sqd-smooth2.png > /dev/null
cmp sqd-smooth.png sqd-smooth2.png
$H --menu --settings sqd/off.ini --character c99.character --creator --screenshot sqd-off.png | tr -d '\r' | q "^character drawing: quads (the wireframe, as Wireframe is on)$"
$H --personal-world --real-graphics --settings sqd/q.ini --pworld-look -10 --screenshot sqd-pworld.png | tr -d '\r' | q "^characters: drawn as quads, [1-9][0-9]* quads of skin$"
$H --personal-world --real-graphics --settings sqd/m.ini --pworld-look -10 --screenshot sqd-pworld2.png | tr -d '\r' | q "^characters: drawn as smooth, [1-9][0-9]* triangles of smooth skin$"
$H --personal-world --settings sqd/off.ini --pworld-look -10 --screenshot sqd-pworld-off.png | tr -d '\r' | q "^characters: drawn as wireframe (Wireframe is on; quads chosen)$"
$H --no-menu --real-graphics --settings sqd/q.ini --character-store pdim/characters --screenshot sqd-hall.png | tr -d '\r' | q "^characters: drawn as quads, [1-9][0-9]* quads of skin$"
$H --no-menu --real-graphics --settings sqd/m.ini --character-store pdim/characters --screenshot sqd-hall2.png | tr -d '\r' | q "^characters: drawn as smooth, [1-9][0-9]* triangles of smooth skin$"
# The skin's limbs (IDEAS 19.229, the first stage of the mesh scaling pipeline): Limbs on Settings >
# Graphics > Character Voxels, off by default. Off, the skin is as it was (no limbs said, the same
# triangles, the same picture byte for byte); on, each bone is a rounded cone in the skin's field, the
# Creator says how many limbs it joined (26 for seed 99) and the triangles change; your dimension and the
# hallway say "with limbs"; the same skin with limbs made twice is the same picture.
rm -rf slm
mkdir -p slm
cp skn/s.ini slm/s.ini
cp skn/s.ini slm/l.ini
$H --main-menu --settings slm/l.ini --press "Item:settings,Return,Item:graphics,Return,Item:character_voxels,Return,Item:skin_limbs,Return" --screenshot slm-menu.png > /dev/null
grep -c "^skin_limbs = on " slm/l.ini | q "^1$"
grep -c "^skin_limbs = off " slm/s.ini | q "^1$"
$H --menu --real-graphics --settings slm/s.ini --character c99.character --creator --screenshot slm-off.png | tr -d '\r' > slm/off.txt
q "^skin: cell 40\.2 units (the voxels' halved 1 times), blend 99\.5 units (50%), grid 79x55x82, 23180 triangles made, [1-9][0-9]* drawn, made in [0-9.]* ms" slm/off.txt
test "$(grep -c "limbs joined" slm/off.txt || true)" = 0
cmp slm-off.png skn-creator.png
$H --menu --real-graphics --settings slm/l.ini --character c99.character --creator --screenshot slm-on.png | tr -d '\r' > slm/on.txt
q "^skin: cell 40\.2 units (the voxels' halved 1 times), blend 99\.5 units (50%), grid 79x55x82, 22676 triangles made, [1-9][0-9]* drawn, made in [0-9.]* ms.*, 26 limbs joined$" slm/on.txt
$H --menu --real-graphics --settings slm/l.ini --character c99.character --creator --screenshot slm-on2.png > /dev/null
cmp slm-on.png slm-on2.png
$H --personal-world --real-graphics --settings slm/l.ini --pworld-look -10 --screenshot slm-pworld.png | tr -d '\r' | q "^characters: drawn as skin, [1-9][0-9]* triangles of skin, with limbs$"
$H --no-menu --real-graphics --settings slm/l.ini --character-store pdim/characters --screenshot slm-hall.png | tr -d '\r' | q "^characters: drawn as skin, [1-9][0-9]* triangles of skin, with limbs$"
# Smooth plus, the surface (IDEAS 19.229, the pipeline's second stage): Skin Smoothing on Settings >
# Graphics > Character Voxels, rounds of Taubin's smoothing, 0 (off) by default. Off, the skin is as it
# was (nothing said of smoothing, the same picture byte for byte); with four rounds the Creator says the
# rounds, their time and the volume before and after (kept within half a percent), the triangles the same
# 23180, the picture changed, and the same picture made twice; your dimension and the hallway say the
# rounds; with limbs too, both stages are said.
rm -rf ssm
mkdir -p ssm
cp skn/s.ini ssm/s.ini
cp skn/s.ini ssm/r.ini
$H --main-menu --settings ssm/r.ini --press "Item:settings,Return,Item:graphics,Return,Item:character_voxels,Return,Item:skin_smoothing,Right,Right,Right,Right" --screenshot ssm-menu.png > /dev/null
grep -c "^skin_smoothing = 4 " ssm/r.ini | q "^1$"
grep -c "^skin_smoothing = 0 " ssm/s.ini | q "^1$"
$H --menu --real-graphics --settings ssm/s.ini --character c99.character --creator --screenshot ssm-off.png | tr -d '\r' > ssm/off.txt
test "$(grep -c "smoothed" ssm/off.txt || true)" = 0
cmp ssm-off.png skn-creator.png
$H --menu --real-graphics --settings ssm/r.ini --character c99.character --creator --screenshot ssm-on.png | tr -d '\r' > ssm/on.txt
q "^skin: cell 40\.2 units (the voxels' halved 1 times), blend 99\.5 units (50%), grid 79x55x82, 23180 triangles made, [1-9][0-9]* drawn, made in [0-9.]* ms.*, smoothed 4 rounds in [0-9.]* ms.*, volume [1-9][0-9]* to [1-9][0-9]* cubic units (100\.[0-4][0-9]%)$" ssm/on.txt
if cmp -s ssm-on.png skn-creator.png; then exit 1; fi
$H --menu --real-graphics --settings ssm/r.ini --character c99.character --creator --screenshot ssm-on2.png > /dev/null
cmp ssm-on.png ssm-on2.png
$H --personal-world --real-graphics --settings ssm/r.ini --pworld-look -10 --screenshot ssm-pworld.png | tr -d '\r' | q "^characters: drawn as skin, [1-9][0-9]* triangles of skin, smoothed 4 rounds$"
$H --no-menu --real-graphics --settings ssm/r.ini --character-store pdim/characters --screenshot ssm-hall.png | tr -d '\r' | q "^characters: drawn as skin, [1-9][0-9]* triangles of skin, smoothed 4 rounds$"
sed -e "s/^skin_limbs = off /skin_limbs = on /" ssm/r.ini > ssm/lr.ini
$H --menu --real-graphics --settings ssm/lr.ini --character c99.character --creator --screenshot ssm-limbs.png | tr -d '\r' | q "^skin: .*, 26 limbs joined, smoothed 4 rounds in [0-9.]* ms"
$H --personal-world --real-graphics --settings ssm/lr.ini --pworld-look -10 --screenshot ssm-pworld2.png | tr -d '\r' | q "^characters: drawn as skin, [1-9][0-9]* triangles of skin, with limbs, smoothed 4 rounds$"
# Smooth plus, the outline (IDEAS 19.229, the pipeline's third stage): Skin Subdivision on Settings >
# Graphics > Character Voxels, steps of Catmull and Clark's subdivision for the quads and smooth drawings,
# 0 (off) by default. Off, the quads are as they were (nothing said of subdivision, the same picture byte
# for byte); with two steps the Creator says the steps, their time and the quads before and after (seed
# 99's 11590 four times four, 185440), the picture changed and the same made twice; three asked are cut to
# two and the readout says why; the skin drawing is never subdivided (the same picture); the smooth
# drawing's triangles are twice the quads; your dimension and the hallway say the steps and the bound.
rm -rf ssd
mkdir -p ssd
cp sqd/q.ini ssd/s.ini
cp sqd/q.ini ssd/q.ini
$H --main-menu --settings ssd/q.ini --press "Item:settings,Return,Item:graphics,Return,Item:character_voxels,Return,Item:skin_subdivision,Right,Right" --screenshot ssd-menu.png > /dev/null
grep -c "^skin_subdivision = 2 " ssd/q.ini | q "^1$"
grep -c "^skin_subdivision = 0 " ssd/s.ini | q "^1$"
$H --menu --real-graphics --settings ssd/s.ini --character c99.character --creator --screenshot ssd-off.png | tr -d '\r' > ssd/off.txt
test "$(grep -c "subdivided" ssd/off.txt || true)" = 0
cmp ssd-off.png sqd-quads.png
$H --menu --real-graphics --settings ssd/q.ini --character c99.character --creator --screenshot ssd-on.png | tr -d '\r' > ssd/on.txt
q "^skin: cell 40\.2 units (the voxels' halved 1 times), blend 99\.5 units (50%), grid 79x55x82, 185440 quads made, [1-9][0-9]* drawn, made in [0-9.]* ms.*, subdivided 2 steps in [0-9.]* ms.*, 11590 to 185440 quads$" ssd/on.txt
if cmp -s ssd-on.png sqd-quads.png; then exit 1; fi
$H --menu --real-graphics --settings ssd/q.ini --character c99.character --creator --screenshot ssd-on2.png > /dev/null
cmp ssd-on.png ssd-on2.png
sed -e "s/^skin_subdivision = 2 /skin_subdivision = 3 /" ssd/q.ini > ssd/q3.ini
$H --menu --real-graphics --settings ssd/q3.ini --character c99.character --creator --screenshot ssd-three.png | tr -d '\r' | q "^skin: .*, subdivided 2 steps in [0-9.]* ms.*, 11590 to 185440 quads (3 asked: another step would pass 200000 quads)$"
sed -e "s/^drawing = 5 /drawing = 4 /" ssd/q.ini > ssd/k.ini
$H --menu --real-graphics --settings ssd/k.ini --character c99.character --creator --screenshot ssd-skin.png | tr -d '\r' > ssd/skin.txt
test "$(grep -c "subdivided" ssd/skin.txt || true)" = 0
cmp ssd-skin.png skn-creator.png
sed -e "s/^drawing = 5 /drawing = 6 /" ssd/q.ini > ssd/m.ini
$H --menu --real-graphics --settings ssd/m.ini --character c99.character --creator --screenshot ssd-smooth.png | tr -d '\r' | q "^skin: .*, 370880 triangles made, .*, subdivided 2 steps in [0-9.]* ms.*, 11590 to 185440 quads, normals averaged in [0-9.]* ms$"
$H --personal-world --real-graphics --settings ssd/q.ini --pworld-look -10 --screenshot ssd-pworld.png | tr -d '\r' | q "^characters: drawn as quads, [1-9][0-9]* quads of skin, subdivided up to 2 steps (at most 200000 quads a frame, shared by its characters)$"
$H --no-menu --real-graphics --settings ssd/m.ini --character-store pdim/characters --screenshot ssd-hall.png | tr -d '\r' | q "^characters: drawn as smooth, [1-9][0-9]* triangles of smooth skin, subdivided up to 2 steps (at most 200000 quads a frame, shared by its characters)$"
# Editing a character by its joints (IDEAS 19.203): in the Character Creator . chooses the next sphere,
# J adds a joint after it and N grows a limb from it, on both sides of a mirrored character at once; the
# character is kept as its spheres (sieve-character-v4, its symmetry kept) in the settings file, reads back
# the same, and two Ctrl+Z give the seed's character again. X takes a limb away and all beyond it (and its
# mirror), the body is never taken, and a drag with the mouse moves the chosen sphere and its mirror.
# Taking a joint out of a skeleton with an animation on an action names the action that no longer fits.
# The editing keys are listed on Settings > Controls > Keys after every other, and one changed there is
# used. Nothing of it shows until a sphere is chosen, so the creator untouched reads as before.
rm -rf ced
mkdir -p ced
touch ced/e.ini
touch ced/u.ini
touch ced/k.ini
$H --menu --real-graphics --settings ced/e.ini --character c99.character --creator --press ".,.,J,N" --screenshot ced-keys.png | tr -d '\r' > ced/keys.txt
q "^character: b9ed8e57e12483279bb41ef0f382e697ce45ff3edc81daffb91336b35673324a$" ced/keys.txt
q "^character edit: sieve-character-v4 spheres=31 symmetry=bilateral; sphere 8 chosen, its images: 1; edits to undo: 2; stretch off$" ced/keys.txt
q "^note: a limb grown from it; actions checked against its 31 spheres: all fit$" ced/keys.txt
grep -c "^line = sieve-character-v4 spheres=31 symmetry=bilateral;s 0 -1 0 0 0 569 body;" ced/e.ini | q "^1$"
$H --menu --real-graphics --settings ced/e.ini --creator --screenshot ced-back.png | tr -d '\r' | q "^character: b9ed8e57e12483279bb41ef0f382e697ce45ff3edc81daffb91336b35673324a$"
$H --menu --real-graphics --settings ced/u.ini --character c99.character --creator --press ".,.,J,N,Ctrl+Z,Ctrl+Z" --screenshot ced-undo.png | tr -d '\r' | q "^character: cb53aba8a6f47e41e897571882c364c61fb34a14375379026c6bc042f21af0a2$"
grep -c "^line = sieve-character-v1 seed=99 limbs=5 joints=3 branches=1 symmetry=bilateral head=yes " ced/u.ini | q "^1$"
$H --menu --real-graphics --character c99.character --creator --press ".,.,X" --screenshot ced-cut.png | tr -d '\r' | q "^character edit: sieve-character-v4 spheres=17 symmetry=bilateral; sphere 0 chosen"
$H --menu --real-graphics --character c99.character --creator --press "X" --screenshot ced-body.png | tr -d '\r' | q "^note: not edited: the body stays where it is: choose another sphere$"
$H --menu --real-graphics --character c99.character --creator --press ".,." --creator-drag 60,-80 --screenshot ced-drag.png | tr -d '\r' | q "^character edit: sieve-character-v4 spheres=27 symmetry=bilateral; sphere 2 chosen, its images: 1; edits to undo: 1; stretch off$"
$H --menu --real-graphics --import-skeleton tests/arm.gltf --animation-frame 5 --creator --press "Tab,Right,Tab,.,.,Shift+J" --screenshot ced-unfit.png | tr -d '\r' | q "^note: the joint taken out of its limb; actions checked against its 2 spheres; more bones than spheres now, so not played: idle$"
$H --main-menu --settings ced/k.ini --read-aloud --press "Item:settings,Return,Item:controls,Return,Item:keys,Return,Item:keyrow:39" --screenshot ced-keyrow.png | tr -d '\r' | q "^read aloud: The Character Creator: editing: choose the next sphere, \.$"
$H --main-menu --settings ced/k.ini --press "Item:settings,Return,Item:controls,Return,Item:keys,Return,Item:keyrow:41,Return,M" --screenshot ced-keyset.png > /dev/null
grep -c "^creator.edit/grow = M " ced/k.ini | q "^1$"
$H --menu --real-graphics --settings ced/k.ini --character c99.character --creator --press ".,.,M" --screenshot ced-m.png | tr -d '\r' | q "^note: a limb grown from it; "
$H --menu --real-graphics --character c99.character --creator --screenshot ced-none.png | tr -d '\r' > ced/none.txt
test "$(grep -c "^character edit" ced/none.txt || true)" = 0
# Recording and replay (IDEAS 19.184, sieve-replay-v1): a recording's start written where the
# hallway begins; frames added (W held for 1.5 s, the mouse turned, S held, Q pressed); played through,
# it ends where it ends every time, as plain text, packed as .sieve, and packed and cached; played while
# recording, it is written again the same, byte for byte. One begun elsewhere is refused, saying where.
rm -rf rpl
mkdir -p rpl/replays
$H --no-menu --record rpl/start.replay --screenshot rpl-start.png > /dev/null
head -1 rpl/start.replay | tr -d '\r' | q "^sieve-replay-v1$"
python -c "
t = open('rpl/start.replay').read()
fr = ['f 16667 26'] * 90 + ['f 16667 - mm:600:350:120:0'] + ['f 16667 7'] * 40 + ['f 16667 - kd:20:113:0:0 ku:20:113:0:0']
open('rpl/walk.replay', 'w', newline='\n').write(t + '\n'.join(fr) + '\n')"
$H --no-menu --replay rpl/walk.replay --record rpl/again.replay --screenshot rpl-walk.png | tr -d '\r' > rpl-walk.txt
q "^replay: 132 frames, 2.200 s$" rpl-walk.txt
cmp rpl/walk.replay rpl/again.replay
where=$(grep -A1 "^replay: " rpl-walk.txt | tail -1)
$H --no-menu --replay rpl/walk.replay --record rpl/replays/walk.sieve --screenshot rpl-pack.png > /dev/null
$H --no-menu --replay rpl/replays/walk.sieve --screenshot rpl-packed.png | tr -d '\r' | grep -A1 "^replay: " | tail -1 | cmp - <(printf '%s\n' "$where")
$H --no-menu --replay rpl/replays/walk.sieve --replay-cache --screenshot rpl-cached.png | tr -d '\r' | grep -A1 "^replay: " | tail -1 | cmp - <(printf '%s\n' "$where")
if $H --no-menu --line image --replay rpl/walk.replay --screenshot rpl-elsewhere.png > rpl-elsewhere.txt 2>&1; then exit 1; fi
q "the recording began on the text line" rpl-elsewhere.txt
# Text typed is read back at any length its writer writes: twelve characters at once (36 bytes of
# UTF-8, as an input method may commit them; the reader once refused more than 32), played while
# recording, are written again the same.
python -c "
t = open('rpl/start.replay').read()
open('rpl/long.replay', 'w', newline='\n').write(t + 'f 16667 - tx:e4b880e4ba8ce4b889e59b9be4ba94e585ade4b883e585abe4b99de58d81e799bee58d83\nf 16667 -\n')"
$H --no-menu --replay rpl/long.replay --record rpl/long-again.replay --screenshot rpl-long.png | tr -d '\r' | q "^replay: 2 frames, 0.033 s$"
cmp rpl/long.replay rpl/long-again.replay
# The replay viewer (the main menu's Replays): each recording a row, a packed one with its box to be
# decoded once into the cache, ticked with Space and kept by its SHA-256 in [replays] cached.
$H --main-menu --settings rpl/v.ini --read-aloud --press "Item:replays,Return,Space" --screenshot rpl-viewer.png | tr -d '\r' | q "^read aloud: walk.sieve, [0-9]* bytes  \[x\] cache$"
grep -q "^cached = $(sha256sum rpl/replays/walk.sieve | cut -c1-64)" rpl/v.ini
# Stage 4 of the catalogue: Ctrl+K's import is added to its importer's entry, as `sieve import` adds it.
grep -c " what=import result=whole animations=1 .* joints=3 " "$SIEVE_STATS/model-gltf.stats" > /dev/null
# The animation is kept too: in the store beside the settings file, named by its SHA-256 there.
grep -q "^animation = 2a5cb1d00e4ace0f373757b7d29ef342a157d8d27a19bd7fafccaef5a5b54be8" arm.ini
test -s animations/2a5cb1d00e4ace0f373757b7d29ef342a157d8d27a19bd7fafccaef5a5b54be8.animation
$H --menu --settings arm.ini --press "\`" --screenshot char-arm3.png | tr -d '\r' | q "^animation: 2a5cb1d00e4ace0f373757b7d29ef342a157d8d27a19bd7fafccaef5a5b54be8 31 frames, 3 bones"
# The infinite RAID (IDEAS 19.176): with [raid] drives set, each character saved is also kept there as
# a new version of "your character", the second naming the first.
rm -rf raidc1 raidc2
printf '[raid]\ndrives = raidc1,raidc2\n' > raid.ini
$H --menu --settings raid.ini --save-character raid-me.character --press "\`" --screenshot raid1.png | tr -d '\r' | q "^note: saved as raid-me.character; in the store as .*; in the RAID as "
$H --menu --settings raid.ini --save-character raid-me.character --press "\`,G" --screenshot raid2.png > /dev/null
$exe raid list --drives raidc1,raidc2 | tr -d '\r' | q "  your character  (after "
$exe raid verify --drives raidc1,raidc2 | tr -d '\r' | q "^whole$"
# Between machines from the menus (IDEAS 19.176, 19.150), both opt in only behind red boxes in
# Settings > Infinite RAID. Turning sharing on makes the key beside the settings; then Ctrl+S (as
# --save-character does) also sends your save, sealed, to the machine named, and each version's
# remote id is kept beside the key, so the character comes back byte for byte from that machine.
rm -rf raidn raidn-held raidn.ini
mkdir -p raidn
$H --main-menu --settings raidn/raidn.ini --read-aloud --press "Item:settings,Return,Item:raid,Return,Return" --screenshot raid-menu.png | tr -d '\r' | q "^read aloud: Share my saves, On$"
test -s raidn/raid.key
grep -q "^share = on" raidn/raidn.ini
printf '[raid]\ndrives = raidn/d1,raidn/d2\npeers = 127.0.0.1:47133\n[online]\nenabled = on\n' >> raidn/raidn.ini
$exe raid serve --held raidn-held --listen 127.0.0.1:47133 > raidn-serve.txt 2>&1 &
raidn_serve=$!
trap 'kill $raidn_serve 2>/dev/null || true' EXIT
python -c "import time
for _ in range(100):
    if 'listening' in open('raidn-serve.txt').read(): break
    time.sleep(0.1)
else:
    raise SystemExit('the RAID server did not start (port 47133 taken?)')"
# (the note as the picture is taken: still sending, or, since the share's one-write fix made it quick
# (review 1, 14 November 2026), already shared)
$H --menu --settings raidn/raidn.ini --save-character raidn/me.character --press "\`" --screenshot raid-share.png | tr -d '\r' | q "^note: .*\(sending it, sealed, to the machines named\|shared with 1 of 1 machines\)"
test "$(wc -l < raidn/raid-shared.txt)" -eq 2
$exe raid fetch "$(sed -n 1p raidn/raid-shared.txt | cut -d' ' -f2)" --key raidn/raid.key --peers 127.0.0.1:47133 --out raidn/back.character > /dev/null
cmp raidn/me.character raidn/back.character
kill $raidn_serve
# Keeping others' saves, from the menu: ticked, this machine answers at once on [raid] listen.
# Online off (the default) keeps it from answering, however it is set, and its Answering on row says
# so, and why (since 14 November 2026, as sharing and checking say so); on, it answers.
printf '[raid]\nlisten = 127.0.0.1:47134\nheld = raidn-held2\nserve = on\n' > raidn/offline.ini
$H --main-menu --settings raidn/offline.ini --read-aloud --press "Item:settings,Return,Item:raid,Return,Item:raid_listen" --screenshot raid-offline.png | tr -d '\r' | q "^read aloud: Answering on, 127.0.0.1:47134: not answering, as Online is off (Potentia Settings > Online)$"
test ! -e raidn-held2
printf '[online]\nenabled = on\n[raid]\nlisten = 127.0.0.1:47134\nheld = raidn-held2\n' > raidn/serve.ini
$H --main-menu --settings raidn/serve.ini --read-aloud --press "Item:settings,Return,Item:raid,Return,Item:raid_serve,Return,Item:raid_listen" --screenshot raid-serve.png | tr -d '\r' | q "^read aloud: Answering on, on 127.0.0.1:47134, 0 bytes kept for others$"
test -f raidn-held2/sieve-raid-held
# The catalogue of formats as a ring of doors (IDEAS 19.165, stage 3): standing at "image", its doors
# are what it is part of (data), its other kinds (video, documents) and its file kinds; two doors on,
# Enter walks through to "document"; Home goes back to the root; L takes a Unicode block onto the
# pages line as its alphabet. (document's 15: text and image above it; the manifest, PDF, the
# catalogue's two record formats, the hallway's recordings (sieve-replay-v1), favourites, as one
# and as a dimension's list, the RAID's tally of who keeps its pieces, and the fractal dimension's
# rulesets (v1 to v6) and chart (v1 and v2), beneath: 17 since Y2's v5, 18 since X5's v6.)
$H --menu --catalogue-entry image --screenshot cat-image.png | tr -d '\r' | q "^catalogue: image: 8 doors$"
$H --menu --catalogue-entry image --press "Right,Right,Return" --screenshot cat-walk.png | tr -d '\r' | q "^catalogue: document: 18 doors$"
$H --menu --catalogue-entry image --press "Return,Home" --screenshot cat-home.png | tr -d '\r' | q "^catalogue: information: "
$H --menu --catalogue-entry script-greek --press "l" --screenshot cat-greek.png | tr -d '\r' > cat-greek.txt
q "^catalogue: script-greek: " cat-greek.txt
# LANGUAGE beside CHARACTER (IDEAS 19.183): the languages your character speaks, by family (Arabic,
# Hebrew; English, French...; Japanese; Korean; Chinese), ticked with Space and kept in the settings
# file, read back on the next start as part of the same save (sieve-save-v1).
rm -f speaks.ini
$H --menu --settings speaks.ini --press "\`,PageDown,Down,Down,Down,Space,Down,Down,Down,Down,Down,Down,Down,Down,Space" --screenshot speaks.png | tr -d '\r' > speaks.txt
q "^right-hand panel: open (LANGUAGE)$" speaks.txt
q "^speaks: language-french,language-korean$" speaks.txt
save=$(sed -n 's/^save: //p' speaks.txt)
grep -q "^speaks = language-french,language-korean" speaks.ini
$H --menu --settings speaks.ini --press "\`" --screenshot speaks2.png | tr -d '\r' | q "^save: $save$"
# The System Viewer (IDEAS 19.183): on the main menu after Personal Dimensions, its two systems. It is
# a view of its own, the whole window, not the setup menu's panel (Edward, 12 November 2026): S turns
# from FORMATS to LANGUAGE (the five families, writing and information round "language"); a language
# leads to the scripts it is written in.
$H --main-menu --read-aloud --press "Item:systems,Return,Item:systems_language" --screenshot sys-menu.png | tr -d '\r' | q "^read aloud: Language$"
$H --menu --system-viewer information --screenshot sys-viewer.png | tr -d '\r' > sys-viewer.txt
q "^system viewer: open$" sys-viewer.txt
q "^catalogue: information: " sys-viewer.txt
if grep -q "^settings panel: \|^right-hand panel: " sys-viewer.txt; then exit 1; fi
$H --menu --system-viewer information --press "s" --screenshot sys-language.png | tr -d '\r' | q "^catalogue: language: 7 doors$"
$H --menu --system-viewer language-japanese --press "Right,Return" --screenshot sys-kana.png | tr -d '\r' | q "^catalogue: script-japanese-kana: "
# Only one panel in at a time: the right-hand one sends the settings out, Tab brings them back in
# and sends it out, and Esc on it brings the settings back as they were.
$H --menu --catalogue-entry image --screenshot one-side.png | tr -d '\r' | q "^settings panel: out$"
$H --menu --catalogue-entry image --press "Tab" --screenshot one-tab.png | tr -d '\r' > one-tab.txt
q "^settings panel: in$" one-tab.txt
if grep -q "^right-hand panel: " one-tab.txt; then exit 1; fi
$H --menu --press "\`,Escape" --screenshot one-back.png | tr -d '\r' > one-back.txt
q "^settings panel: in$" one-back.txt
if grep -q "^right-hand panel: " one-back.txt; then exit 1; fi
$H --menu --press "Tab,\`,Escape" --screenshot one-out.png | tr -d '\r' | q "^settings panel: out$"
# The right-hand panel's tabs: PgUp/PgDn, or Ctrl+Left/Right for keyboards with no Page keys (or
# a tab clicked), in the order they stand: CHARACTER, LANGUAGE, YOURS, SYSTEMS, and round again.
$H --menu --press "\`,Ctrl+Right" --screenshot tabs-right.png | tr -d '\r' | q "^right-hand panel: open (LANGUAGE)$"
$H --menu --press "\`,Ctrl+Left" --screenshot tabs-left.png | tr -d '\r' | q "^right-hand panel: open (CATALOGUE)$"
# In the hallway: the arm (you, not drawn) plays it, held at its last frame for the picture.
$exe import tests/arm.gltf --character-out arm.character --animation-out arm.animation > /dev/null
$H --no-menu --character arm.character --animation arm.animation --animation-frame 30 --pose 0,0,0,-5 --screenshot hall-arm.png | tr -d '\r' > hall-arm.txt
q "^your character: 22807d23fa4d9b7abfb2895e2e97448ad179cd0f0740b5effa13594dc87b78a7" hall-arm.txt
q "^your animation: 2a5cb1d00e4ace0f373757b7d29ef342a157d8d27a19bd7fafccaef5a5b54be8" hall-arm.txt
test -s hall-arm.png
# The seed walk (IDEAS 19.148 step 7): Shift+G, ten steps from the default character, lands where the
# unit tests pin it.
$H --menu --press "\`,Shift+G" --screenshot char-walk.png | tr -d '\r' | q "^character: 775f9b13e7bb87599c40b20aa700f7444da6aca7a09c4ff2a2a2101593c08688"
# Drawn shaded (the drawing row, the seventh): the same character, since drawing changes none.
$H --menu --press "\`,Down,Right,Right,Down,Right,Down,Down,Down,Down,Right,Right" --screenshot char-shaded.png | tr -d '\r' | q "^character: 455b1fd6a86a035c41f4ec6f1d4d2ce60869595fc67c30e4a913397d19f38972"
# Melodies in CHAT: a melody model's (the same addresses as --make), a random melody and picture
# (/random), and GO TO IT on a melody holds it on AUDIO.
$H --no-menu --line ai --walk "0,0" --take --chat --picture-model data/training/melodies-2x64 --chat-draw 1 --talk "/random melody" --training-dir data/training --screenshot chat-mel.png | tr -d '\r' > chatmel.txt
q "^melody 1 by melodies-2x64: 8ac6f76023e45b65ccb9d8e98b4" chatmel.txt
q "^a random melody: " chatmel.txt
$H --no-menu --line ai --walk "0,0" --take --chat --talk "/random picture" --training-dir data/training --screenshot chat-rand.png | tr -d '\r' | q "^a random picture: "
# A random place on another line (6 November 2026): a book's card, and GO TO IT on a page's goes
# through the door to it.
$H --no-menu --line ai --walk "0,0" --take --chat --talk "/random book" --training-dir data/training --screenshot chat-rbook.png | tr -d '\r' | q "^a random book, on the BOOKS line: [0-9a-f]*$"
$H --no-menu --line ai --walk "0,0" --take --chat --talk "/random page" --chat-press "Ctrl+G" --training-dir data/training --screenshot chat-rpage.png | tr -d '\r' | q "^PAGES line, .*| went to "
# COST's shortest route searched further (L; IDEAS 16.1 (a)): to 640 places, waited for.
$H --no-menu --line binary --walk "0,0" --take --route-places 640 --screenshot cost-route.png | tr -d '\r' | q "^route: .*, searched to 640)$"
# /unit (IDEAS 19.32): the place at a number, in hex, on a line, as a card; GO TO IT goes there.
$H --no-menu --line ai --walk "0,0" --take --chat --talk "/unit page 1f" --chat-press "Ctrl+G" --training-dir data/training --screenshot chat-unit.png | tr -d '\r' | q "^PAGES line, .*| went to 1f$"
# /docs with its card (IDEAS 19.48): the section as a file, walked to on the binary line (GO TO
# IT) and opened as text on the pages line (J).
$H --no-menu --line ai --walk "0,0" --take --chat --talk "/docs ideas 19.48" --training-dir data/training --screenshot chat-docs.png | tr -d '\r' | q "^as a file: IDEAS-19.48-The-documentation-inside-the-game.md, "
$H --no-menu --line ai --walk "0,0" --take --chat --talk "/docs ideas 19.48" --chat-press "Ctrl+G" --training-dir data/training --screenshot chat-docs-go.png | tr -d '\r' | q "^BINARY line, .*went to a file of "
$H --no-menu --line ai --walk "0,0" --take --chat --talk "/docs ideas 19.48" --chat-press "Ctrl+J" --training-dir data/training --screenshot chat-docs-j.png | tr -d '\r' | q "^PAGES line, .*a TXT file, opened on the PAGES line"
$H --no-menu --line ai --walk "0,0" --take --chat --picture-model data/training/melodies-2x64 --chat-draw 1 --chat-press "Ctrl+G" --training-dir data/training --screenshot chat-mgo.png | tr -d '\r' | q "^AUDIO line, .*on the audio line, in hand"
$H --no-menu --line ai --walk "0,0" --take --chat --chat-draw 1 --chat-press "Ctrl+G" --training-dir data/training --screenshot chat-go.png | tr -d '\r' | q "^IMAGE line, .*the figure, on the image line, in hand"
$H --no-menu --line ai --walk "0,0" --take --chat --chat-attach tests/training_corpus.txt --chat-press "Ctrl+J" --training-dir data/training --screenshot chat-j.png | tr -d '\r' | q "^PAGES line, .*a TXT file, opened on the PAGES line"
downs=$(python -c "print(','.join(['Down']*41))")
$H --menu --press "$downs" --screenshot menu-ai.png | q "saved menu-ai.png"
# The AI Training Harness (main menu): the run shipped with Sieve, trained again, comes out the
# same; a run recorded there from a folder, its second file moved first and read in order, is
# trained and its result written; and GO TO IT stands on the AI line with the model in hand.
$H --harness --training-dir data/training --script "Return" --screenshot harness.png | tr -d '\r' | q "^harness: REPRODUCED EXACTLY"
mkdir -p hruns corp
cp tests/training_corpus.txt corp/a.txt
head -c 150 tests/training_corpus.txt | tr 'a-z' 'b-za' > corp/b.txt
$H --harness --training-dir hruns --script "N,=corp,Return,Tab,Down,Shift+Up,M,Return" --screenshot harness-new.png | tr -d '\r' | q "^harness: TRAINED"
[ -f hruns/corp/result.txt ]
q "^reading = in-order" hruns/corp/training.ini
[ "$(grep '^file = ' hruns/corp/training.ini | head -1)" = "file = corpus/b.txt" ]
# Its settings (which have the keys first, the preset their top row): the width one step up
# (two heads' worth, 16 to 20), the learning rate following it, as suggested (0.008 / 20, rounded
# down: 0.0002).
mkdir -p hruns2
$H --harness --training-dir hruns2 --script "N,=corp,Return,Down,Down,Right,Return" --screenshot harness-set.png | tr -d '\r' | q "^harness: TRAINED"
q "^width = 20" hruns2/corp/training.ini
q "^learning-rate = 0.0002" hruns2/corp/training.ini
# The settings added on 9 November 2026: training-v1 (two steps left from v3), momentum 0.95, shuffle
# seed 2, a checkpoint every 50 steps, on one thread; recorded as set, and trained again exactly.
mkdir -p hruns3
$H --harness --training-dir hruns3 --script "N,=corp,Return,Down,Down,Down,Down,Down,Left,Left,Down,Down,Down,Right,Down,Down,Down,Down,Down,Right,Down,Left,Down,Right,Return" --screenshot harness-set3.png | tr -d '\r' | q "^harness: TRAINED"
q "^version = training-v1" hruns3/corp/training.ini
q "^momentum = 0.95" hruns3/corp/training.ini
q "^seed = 2" hruns3/corp/training.ini
q "^checkpoint-every = 50" hruns3/corp/training.ini
$H --harness --training-dir hruns3 --script "Return" --screenshot harness-set3-again.png | tr -d '\r' | q "^harness: REPRODUCED EXACTLY"
$H --harness --training-dir hruns --script "g" --screenshot harness-go.png | tr -d '\r' | q "the model the run trained, in hand"
# The AI line's filters in the hallway: compact (weight-rows-v1), its survivors closed up; and
# with canonical-model-v1 hiding the rest, GO TO IT stands on the model's canonical form.
printf '[ai]\nmode = compact\nfilters = weight-rows-v1\n' > ai-rows.ini
printf '[ai]\nmode = hide\nfilters = canonical-model-v1\n' > ai-canon.ini
$H --no-menu --line ai --filters ai-rows.ini --walk "0,0" --screenshot ai-compact.png | tr -d '\r' | sed -n 1p | q "^AI line, positional, filters: 1 compact"
$H --harness --training-dir data/training --filters ai-canon.ini --script "g" --screenshot harness-go-canon.png | tr -d '\r' | q "as its class's canonical form"
# A new melody model (6 November 2026): M opens its settings, of the audio line as the setup menu
# has it.
$H --harness --training-dir hruns --script "M" --screenshot harness-mel.png | tr -d '\r' | q "^status: set the melody model"
$H --menu --line ai --filters ai-rows.ini --settle 200 --press "F" --screenshot menu-ai-filters.png | q "saved menu-ai-filters.png"
$H --menu --line ai --press "F,Down,Left" --settle 100 --screenshot menu-ai-judge-only.png | q "saved menu-ai-judge-only.png"
printf '[ai]\nmode = compact\nfilters = near-model-v1\n' > ai-near.ini
$H --no-menu --line ai --filters ai-near.ini --walk "0,0" --screenshot ai-near.png | tr -d '\r' | sed -n 1p | q "within 2 digits of universalbibliothek's model (7548948481 survivors)"
# From the main menu: the same locator; Go to it builds the hallway and walks to the file.
$H --main-menu --locate tests/manifest_fixture/a.txt --press "Return" --screenshot locate-menu.png | q "file locator (main menu): open, Go to it offered"
# Each file it reads is added to its kind's entry (what=read): its size and its .sieve's, no name.
grep -c " what=read result=read bytes=34 .*sieve=" "$SIEVE_STATS/file-txt.stats" > /dev/null
if grep -c "a\.txt" "$SIEVE_STATS"/*.stats > /dev/null; then exit 1; fi
$exe catalogue file-txt | tr -d '\r' | q "^read       in the File Locator "
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
# typed in the navigator (X, Tab) is the same item again. (4,320: 360 a line for all twelve, the fractal
# dimension's being every ruleset to 256 characters, v1's, v2's and v3's, since list X3.)
$H --no-menu --sample-degrees degrees --screenshot deg.png | tee deg.txt | q "^sample: 4320 items"
$H --no-menu --sample-degrees degrees2 --sample-out degrees2.sieve --screenshot deg2.png > /dev/null
diff -r degrees degrees2
build/sieve install degrees.sieve --to deg_inst > /dev/null
diff -r degrees deg_inst
# The fractal line's items are written as their rulesets, the steps that make them, every one a ruleset
# the reader takes (the index as text); no picture of one is stored.
test "$(find degrees/fractals -name '*.fractal' | wc -l)" = 359
find degrees/fractals -name '*.fractal' -print0 | xargs -0 -n 1 build/sieve fractal check > fractal-checks.txt
test "$(grep -c '^a sieve-fractal-v[123] ruleset' fractal-checks.txt)" = 359
test -s "degrees/fractals/Index of the fractal dimension.txt"
test "$(find degrees/fractals -name '*.png' | wc -l)" = 0
# The same as sieve-manifest-v5 (--items): the pages, pictures, films and melodies carried as
# their lines and places, made again on installing, every file the same; smaller than v4;
# and sieve-install, the full installer, makes them again too. (The hallway's own sample is
# the newest instructions since 27 October 2026, smaller than both.)
build/sieve locate degrees --installer degrees-v5.sieve --items --v5 --manifest degrees-v5.manifest | tr -d '\r' | q "^items     1368 file(s) carried as items"
python reference/sieve_ref.py unpack degrees-v5.manifest --items-from degrees --to deg_oracle5 > /dev/null
diff -r degrees deg_oracle5
build/sieve locate degrees --installer degrees-v4.sieve --v4 > /dev/null
[ "$(wc -c < degrees-v5.sieve)" -lt "$(wc -c < degrees-v4.sieve)" ]
[ "$(wc -c < degrees2.sieve)" -lt "$(wc -c < degrees-v5.sieve)" ]
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
# than v6, under a fifth of it (a tenth before list X3: the fractal dimension's 360 rulesets, up to
# 256 characters since then, are carried as places of about their own size, 74,527 bytes of the
# 84,225 measured on 16 November 2026, the rest 9,671); every file back, by sieve install, sieve-install and the oracle. --file-hashes keeps every file's own SHA-256, and installs the same.
build/sieve locate degrees --installer degrees-v7.sieve --items --v15 --manifest degrees-v7.manifest | tr -d '\r' > d7.txt
q "^items     1368 file(s) carried as items of Sieve's lines (.* sieve-manifest-v7" d7.txt
q "^listing   [0-9]* bytes, carried as LZMA2 in " d7.txt
[ "$(wc -c < degrees-v7.sieve)" -lt "$(( $(wc -c < degrees-v6.sieve) / 5 ))" ]
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
# sieve-manifest-v20 (the newest until v32, so --v31): the body in bytes too, its "line" lines (which
# hold tabs) kept as they are; no larger than v16, and every file back, by sieve install, the oracle
# and sieve-install. (--priority speed: no mixing, which would make it v28; since list X2 the fractal
# dimension's index, 75 entries, is a text long enough to be mixed.)
build/sieve locate degrees --v31 --priority speed --installer degrees-v20.sieve --items --manifest degrees-v20.manifest | tr -d '\r' > d20.txt
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
rm -rf ub_run ub_model ub_one ub_inst ub_oracle
mkdir ub_one
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
[ "$(wc -c < ub-v7.sieve)" -eq 4253 ]
[ "$(wc -c < ub-v6.sieve)" -eq 4443 ]
q "^line 0 digit-histogram-v1 ai/L1/H16/A2/F64/B4/V256/key=sieve/aispace-v1 line=ai$" ub-v7.manifest
build/sieve install ub-v7.sieve --to ub_inst7 > /dev/null
cmp ub_one/model.safetensors ub_inst7/model.safetensors
python reference/sieve_ref.py unpack ub-v7.manifest --to ub_oracle7 > /dev/null
cmp ub_one/model.safetensors ub_oracle7/model.safetensors
# One file on its own: no root line (it is the file's name), 4,257 bytes for this name.
rm -rf ub_named ub_named_inst
mkdir ub_named
cp ub_one/model.safetensors ub_named/sieve-ai-16432c2a3953.safetensors
build/sieve locate ub_named/sieve-ai-16432c2a3953.safetensors --installer ub-file.sieve --items --v15 > /dev/null
[ "$(wc -c < ub-file.sieve)" -eq 4257 ]
build/sieve install ub-file.sieve --to ub_named_inst > /dev/null
cmp ub_one/model.safetensors ub_named_inst/sieve-ai-16432c2a3953.safetensors
# v16 (--items now): the same, its head in bytes: 4,185 bytes (the folder's, 4,176).
rm -rf ub_named16 ub_inst16 ub_oracle16
build/sieve locate ub_named/sieve-ai-16432c2a3953.safetensors --installer ub-file16.sieve --items --v19 > /dev/null
[ "$(wc -c < ub-file16.sieve)" -eq 4185 ]
# v20 (the newest until v32, so --v31): its body in bytes, 4,182 (its "line" lines four bytes shorter
# each, its entry too).
build/sieve locate ub_named/sieve-ai-16432c2a3953.safetensors --v31 --installer ub-file20.sieve --items > /dev/null
[ "$(wc -c < ub-file20.sieve)" -eq 4182 ]
rm -rf ub_named20
build/sieve install ub-file20.sieve --to ub_named20 > /dev/null
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
rm -rf pg64 pg64_inst pg64_oracle
mkdir pg64
python -c "import random; random.seed(3); [open(f'pg64/p{i}.txt','w').write(''.join(random.choice('abcdefghijklmnopqrstuvwxyz ') for _ in range(64))) for i in range(5)]"
build/sieve locate pg64 --v31 --installer pg64.sieve --items --page-length 64 --manifest pg64.manifest | tr -d '\r' | q "^items     5 file(s) carried as items of Sieve's lines (320 bytes, as 195 bytes of places"
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
rm -rf tune tune_inst tune_oracle
mkdir tune
cp reference/sieve_ref.py tune/
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
rm -rf rope_mix rope_inst rope_oracle rope_si
mkdir rope_mix
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
q "^wave      L8 W[0-9]* S[1-8] P[0-4] K[01]\( C[12]\)\?\( G1\)\?\( T1\)\? (rope-wave-v[1-46])" rope.txt
q "read back and checked" rope.txt
[ "$(wc -c < picture.rope)" -lt "$(sed -n 's/^LZMA2     \([0-9]*\) bytes.*/\1/p' rope.txt)" ]
params=$(sed -n 's/^wave      \(.*\) (rope-wave-v[1-46])/\1/p' rope.txt)
python reference/sieve_ref.py rope-unpack picture.rope --params "$params" --size 30000 --out picture.back > /dev/null
cmp rope_mix/picture.raw picture.back
# sieve-manifest-v8: the picture and the sound carried by the rope (the sound's wave at nine
# of its cycles, a line of 1,131 values: 10,213 bytes against LZMA2's 10,295), the text by LZMA2,
# each the smaller way. Smaller than v7 (no rope); installed by sieve install, the oracle, and
# sieve-install itself (no items, so the small installer needs nothing it lacks). (--v25: v26's
# mix-v2 carries the sound and the text otherwise.)
build/sieve locate rope_mix --installer rope8.sieve --items --v25 --manifest rope8.manifest | tr -d '\r' > rope8.txt
q "^rope      2 file(s) pulled tight by rope-wave-v1\( and v2\| to v[3-6]\)\? (46000 bytes" rope8.txt
python -c "import sys; sys.argv = ['x']; sys.path.insert(0, 'reference'); import sieve_ref; sys.stdout.buffer.write(sieve_ref.v7_listing(sieve_ref.v16_text(open('rope8.manifest', 'rb').read())))" | q -a "^f	30000	rope L8 W[0-9]* S[1-8] P[0-4] K[01]\( C[12]\)\?\( G1\)\?\( T1\)\?	picture.raw$"
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
q "^  picture.raw  *[0-9]*  *[0-9]*   rope L8 W[0-9]* S[1-8] P[0-4] K[01]\( C[12]\)\?\( G1\)\?\( T1\)\?$" ropeweigh.txt
q "^  shared: rope, [1-3] file(s): shape 96 bits$" ropeweigh.txt
# rope-wave-v2: digits in a table (groups of ten, ten groups a line, Windows line ends, from a
# fixed generator), each column's bytes counted apart: the separators' columns cost their
# tables alone, the digits about log2(10) bits each. Smaller than LZMA2; the oracle decodes it
# its own way; as instructions it is sieve-manifest-v9 (--v9), installed by all three readers.
# rope-wave-v4: the search groups the columns (the digits' share one table): smaller again,
# sieve-manifest-v12 (no items, so no items line), installed by all three readers; --v11 makes
# v11 again, a line longer.
rm -rf digits digits_inst digits_oracle digits_si digits_inst9 digits_oracle9 digits_si9
mkdir digits
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
rm -rf digits_oracle11
python reference/sieve_ref.py unpack digits11.manifest --to digits_oracle11 > /dev/null
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
rm -rf lines lines_inst lines_oracle lines_si lines_inst10 lines_oracle10 lines_si10 lines_bad
mkdir lines
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
rm -rf noisy noisy_inst noisy_oracle noisy_si
mkdir noisy
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
rm -rf mixed mixed_inst mixed_oracle mixed_si mixed_bad
mkdir mixed
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
[ "$z" -ge 15904 ]
[ "$z" -le 24096 ]  # the text's 20,000, within a block either way
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
# sieve-manifest-v14, "Archive and Compressed Formats" (--optimise-archives; the File Locator's box):
# off by default, and then nothing changes. Ticked, an xz file is remade exactly from what it
# holds (liblzma, as the xz program writes it) and a 7z (tests/optimise_fixture.7z: 20 blocks
# of the pi-like table above, 22,222 bytes in 10,513) is carried as its contents, in a folder
# named after it, installing saying so; each only where clearly smaller. Sieve and the oracle
# give the same, and so does sieve-install (the full installer); called v13, refused.
rm -rf arch arch_inst arch_oracle arch_si arch_bad arch_bad16
mkdir -p arch/x
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
build/sieve locate arch --installer arch.sieve --items --optimise-archives --compressed-temporary --compressed-permanent --v19 --manifest arch.manifest | tr -d '\r' > arch.txt
q "^remade    2 archive(s) carried as what they hold, remade exactly on installing (sieve-manifest-v17)$" arch.txt
q "^unpacked  2 archive(s) carried as their contents, not as the archives (Archive and Compressed Formats; sieve-manifest-v17)$" arch.txt
q "^items     none; sieve-manifest-v17$" arch.txt
[ "$(wc -c < arch.sieve)" -lt "$(wc -c < arch_off.sieve)" ]
build/sieve install arch.sieve --to arch_inst | tr -d '\r' > arch_inst.txt
q "^note      digits.7z (7z, 10513 bytes) was carried as its contents (Archive and Compressed Formats): digits holds them, not the archive itself$" arch_inst.txt
cmp arch/x/table.txt.xz arch_inst/x/table.txt.xz
cmp arch/y.txt.gz arch_inst/y.txt.gz
q "^note      w.zip (zip, [0-9]* bytes) was carried as its contents (Archive and Compressed Formats): w holds them, not the archive itself$" arch_inst.txt
test "$(cat arch_inst/w/two.txt)" = "hello"
test ! -e arch_inst/w.zip
cmp arch-digits.txt arch_inst/digits/digits.txt
test ! -e arch_inst/digits.7z
python reference/sieve_ref.py unpack arch.manifest --to arch_oracle | tr -d '\r' | q "^unpacked	7z	10513	digits.7z	digits$"
diff -r arch_inst arch_oracle
build/sieve-install arch.sieve --to arch_si --yes
diff -r arch_inst arch_si
build/sieve locate arch --installer arch15.sieve --items --optimise-archives --compressed-temporary --compressed-permanent --v15 --manifest arch15.manifest > /dev/null
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
pip_add "zstandard==0.25.0"
rm -rf arch19 arch19_inst arch19_oracle arch19_si arch19_bad
mkdir -p arch19
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
build/sieve locate arch19 --installer arch19.sieve --items --optimise-archives --compressed-temporary --compressed-permanent --v19 --manifest arch19.manifest | tr -d '\r' > arch19.txt
q "^remade    2 archive(s) carried as what they hold, remade exactly on installing (sieve-manifest-v19)$" arch19.txt
q "^unpacked  1 archive(s) carried as their contents, not as the archives (Archive and Compressed Formats; sieve-manifest-v19)$" arch19.txt
q "^items     none; sieve-manifest-v19$" arch19.txt
build/sieve install arch19.sieve --to arch19_inst | tr -d '\r' > arch19_inst.txt
cmp arch19/y.txt.bz2 arch19_inst/y.txt.bz2
cmp arch19/y.txt.zst arch19_inst/y.txt.zst
cmp arch19-two.txt arch19_inst/two
q "^note      two.zst (zst, [0-9]* bytes) was carried as its contents (Archive and Compressed Formats): two holds them, not the archive itself$" arch19_inst.txt
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
test ! -e vti
test ! -e vti2
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
rm -rf ordinary
mkdir ordinary
cp listed-piece.bin ordinary/
test "$(build/sieve vault --chunks listed.bin --common ordinary | tail -2 | head -1 | tr ',' '\n' | wc -l)" -lt "$(build/sieve vault --chunks listed.bin | tail -2 | head -1 | tr ',' '\n' | wc -l)"
# Filter plugins that do not load are listed with the reason, never skipped: a malformed
# file, and two different files under one name (neither is used: versions are never edited).
printf 'sieve-filter-v1\nid broken\n' > build/filters/broken.sfilter
sed 's/^describe.*/describe edited/' data/filters/clean-data-v1.sfilter > build/filters/clean-edited.sfilter
build/sieve filters --plugins > plugins.out
grep -A1 "broken.sfilter" plugins.out | q "ends with end"
grep -A1 "clean-edited.sfilter" plugins.out | q "with different contents"
filters8=$(build/sieve filters --length 8)
if grep -q "^\[.\] clean-data-v1 " <<< "$filters8"; then exit 1; fi
rm build/filters/broken.sfilter build/filters/clean-edited.sfilter
build/sieve filters --length 8 | q "^\[.\] clean-data-v1 "
# Filters kept as .sieve (IDEAS 19.185): a filter and the word list it reads packed as one, put in the
# filters folder, decoded at start-up into the cache beside its list and loaded as a loose one is (the
# same SHA-256); one changed bit, and it is refused with the reason, never skipped silently.
rm -rf packed-filter
mkdir packed-filter
sed 's/^id        function-words$/id        packed-words/' data/filters/function-words-v2.sfilter > packed-filter/packed-words-v2.sfilter
cp data/filters/function-words-v2.tsv packed-filter/
build/sieve filters --pack packed-filter --out build/filters/packed-words-v2.sieve | tr -d '\r' | q "^packed    .*: 1 filter, packed-words-v2.sfilter$"
packed_sha=$(python -c "import hashlib; print(hashlib.sha256(open('packed-filter/packed-words-v2.sfilter','rb').read()).hexdigest()[:16])")
build/sieve filters --plugins | tr -d '\r' | q "^loaded   packed-words-v2  $packed_sha  .*packed-words-v2.sieve (packed)$"
build/sieve filters --plugin build/filters/packed-words-v2.sieve --length 4 | tr -d '\r' | q "^sha256 *$packed_sha"
python -c "b = bytearray(open('build/filters/packed-words-v2.sieve', 'rb').read()); b[len(b) // 2] ^= 1; open('build/filters/packed-words-v2.sieve', 'wb').write(b)"
build/sieve filters --plugins | tr -d '\r' | q "^REFUSED  ?  -  .*packed-words-v2.sieve (packed)$"
rm -rf packed-filter build/filters/packed-words-v2.sieve
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
# The catalogue (DO: CATALOGUE, Left from ENCRYPT): the recipes counted as sieve cipher --catalogue
# counts them, and the opening recipe's two layers placed among them.
$H --cipher --script "Down,Down,Down,Left" --screenshot cipher-catalogue.png | tr -d '\r' | q "^cipher: the recipe has 2 layer(s): one of the recipes of its depth, named by 260 bits$"
# The completion estimate (IDEAS §19.49; the setup menu's map since 9 November 2026, the main menu's
# before): a line for each of the twelve dimensions, binary first, each with its size and the time to
# walk it at one a second (the fractal dimension's every ruleset to 256 characters, v1's to v4's, lists X3 and X4).
$H --menu --screenshot main-estimate.png | tr -d '\r' > main-estimate.txt
test "$(grep -c '^estimate: ' main-estimate.txt)" = 12
q "^estimate: FRACTALS  rulesets<=256 chars = ~10^405.1  ->  ~10^397.6 years$" main-estimate.txt
q "^estimate: BINARY  title\*files<=" main-estimate.txt
# A line's details (I, or the dots after its name in the map; 9 November 2026): what its figures
# mean, in words; I again closes it.
$H --menu --press I --screenshot details.png | tr -d '\r' > details.txt
q "^details: [A-Z]*: what its figures mean$" details.txt
q "^details: Walking it: " details.txt
q "^details: On this machine: " details.txt
closed=$($H --menu --press "I,I" --screenshot details-closed.png | tr -d '\r')
if grep -q "^details: " <<< "$closed"; then exit 1; fi
$H --main-menu --press "Item:cipher" --screenshot main-cipher.png | q "saved main-cipher.png"
# Read aloud (--read-aloud, unsaved; IDEAS §19.74): what would be said is printed, since there is
# no voice here. The main menu's item, the setup menu's row with its section and value, a filter
# with its tick said as on or off, and the File Locator's box as pressed.
$H --main-menu --read-aloud --press Down --screenshot ra-main.png | tr -d '\r' | q "^read aloud: File Locator$"
# The main menu by size, Start Sieve to Exit Sieve (IDEAS 19.188): Personal Dimensions sixth, the
# Character Creator and Replays after it, and Potentia Settings (Settings until then, its id still
# settings) the row above Exit. Rows counted here on purpose, so a move of these rows is seen.
$H --main-menu --read-aloud --press "Down,Down,Down,Down,Down" --screenshot ra-main-order.png | tr -d '\r' | q "^read aloud: Personal Dimensions$"
$H --main-menu --read-aloud --press "Down,Down,Down,Down,Down,Down,Down,Down" --screenshot ra-main-order2.png | tr -d '\r' | q "^read aloud: Encryption Engine$"
$H --main-menu --read-aloud --press "Up,Up" --screenshot ra-main-order3.png | tr -d '\r' | q "^read aloud: Potentia Settings$"
$H --main-menu --read-aloud --press "Item:settings" --screenshot ra-main-settings.png | tr -d '\r' | q "^read aloud: Potentia Settings$"
$H --menu --read-aloud --press "Down,Down,Down" --screenshot ra-setup.png | tr -d '\r' | q "^read aloud: GLOBAL, number of items per wall, "
$H --menu --read-aloud --press "F,Down,Down" --screenshot ra-filters.png | tr -d '\r' | q -E "^read aloud: max-run-v1, (On|Off), Filtered: "
$H --main-menu --read-aloud --locate tests/manifest_fixture/a.txt --press O --screenshot ra-locator.png | tr -d '\r' | q "^read aloud: Optimise Search, Off$"
# Sound as FLAC (v34): its button, W, shown and said (once only while the v34 draft was on; v34 frozen
# 12 November 2026); it goes round Temporary (the default), Permanent and Off.
$H --main-menu --read-aloud --locate tests/manifest_fixture/a.txt --press W --screenshot ra-flac.png | tr -d '\r' | q "^read aloud: Sound as FLAC: Permanent$"
$H --main-menu --read-aloud --locate tests/manifest_fixture/a.txt --press "W,W,W" --screenshot ra-flac3.png | tr -d '\r' | q "^read aloud: Sound as FLAC: Temporary$"
$H --main-menu --sound-off --read-aloud --locate tests/manifest_fixture/a.txt --press W --screenshot ra-flac0.png | tr -d '\r' | q "^read aloud: Sound as FLAC: Temporary$"
# Reset to defaults (R, 9 November 2026): Priority pressed to Size (Balanced, Decoding speed, Size),
# reset, then pressed once more: Decoding speed, as from Balanced, its default.
$H --main-menu --read-aloud --locate tests/manifest_fixture/a.txt --press "P,P,R,P" --screenshot ra-defaults.png | tr -d '\r' | q "^read aloud: Priority: Decoding speed$"
# Manual Navigation (IDEAS §19.117): Edward's four bytes turned by hand. A byte typed is one step
# (its two digits); the log (--manual-logs: Keep a log's folder) holds every step, undos and redos
# included, each starting where the one before it landed, and the precision changed between them;
# Save Steps (--manual-steps) just the walk, the turns from the file as loaded to the bytes now.
rm -rf mnav-logs
printf '\000\377\377\000' > mnav.bin
$H --manual-nav mnav.bin --manual-logs mnav-logs --script "Up,Right,Down,],Right,Right,=7f,Ctrl+Z,Ctrl+Y" --manual-steps mnav.csv --screenshot mnav.png | tr -d '\r' > mnav.txt
q "^manual navigation: The walk, 3 step(s) from the file as loaded, saved" mnav.txt
q "^3,turn,3,00,7F," mnav.csv
mlog=$(sed -n 's/^  log: //p' mnav.txt)
q "^# sha256,10db5223d19bd1d58c2b8eb3c723b0ba104cf17564f9434e53e1b9e642fb3b37$" "$mlog"
q "^# precision,2$" "$mlog"
q "^4,undo,3,7F,00," "$mlog"
q "^5,redo,3,00,7F," "$mlog"
python -c "
import csv, sys
rows = [r for r in csv.reader(open(sys.argv[1])) if r and r[0].isdigit()]
assert len(rows) == 5, rows
for a, b in zip(rows, rows[1:]):
    # each starts where the last landed (but across the precision changed, after step 2, where the
    # same place is written to more places and lies a different distance from landing)
    if a[0] != '2': assert a[6] == b[5] and a[8] == b[7], (a, b)
" "$mlog"
# Load Log: the file opened from the path the log names, every step replayed (undos and redos
# too), to the same place; the log carried on, not begun again.
$H --manual-log "$mlog" --manual-logs mnav-logs --script "Up" --screenshot mnav-replay.png | tr -d '\r' > mnav-replay.txt
q "5 step(s) replayed from the log" mnav-replay.txt
q "^6,turn,0,01,02," "$mlog"
[ "$(ls mnav-logs | wc -l)" -eq 1 ]
# Moved: not found where the log says; found by hand (as Ctrl+O would), it is replayed, and the log
# notes where it now is. Another file with other bytes is not taken for it.
mv mnav.bin mnav-moved.bin
if $H --manual-log "$mlog" --screenshot mnav-missing.png > mnav-missing.txt 2>&1; then exit 1; fi
q "The log's file is not at" mnav-missing.txt
$H --manual-log "$mlog" --manual-nav mnav-moved.bin --manual-logs mnav-logs --screenshot mnav-moved.png | tr -d '\r' | q "6 step(s) replayed from the log"
q "^# file,.*mnav-moved.bin$" "$mlog"
mv mnav-moved.bin mnav.bin
printf '\001\377\377\000' > mnav-other.bin
$H --manual-log "$mlog" --manual-nav mnav-other.bin --screenshot mnav-other.png | tr -d '\r' | q "Not the file the log was made on"
# Load File's window: a file given as the window gives it, through the same answer (its owner
# checked; 4 November 2026, Edward: "Opening a file seems to do nothing", every answer dropped).
$H --manual-pick mnav.bin --screenshot mnav-pick.png | tr -d '\r' | q "^manual navigation: mnav.bin, 4 bytes"
# The example it opens on with no file loaded (Edward, 4 November 2026): 2 by 2 random bytes
# (--manual-seed: the same bytes every run, on every system), its log carrying its bytes, replayed
# from them alone. W widens it: with FRESH RANDOM BYTES (the default) to 3 by 3 new bytes; unticked
# (F), the first 4 kept and 5 added.
rm -rf mnav-ex
$H --manual-example --manual-seed 7 --manual-logs mnav-ex --script "Up" --screenshot mnav-ex.png | tr -d '\r' > mnav-ex.txt
q "^manual navigation: An example: 4 random bytes, 2 a row" mnav-ex.txt
exlog=$(sed -n 's/^  log: //p' mnav-ex.txt)
q "^# example,133AC751$" "$exlog"
$H --manual-log "$exlog" --screenshot mnav-ex-replay.png | tr -d '\r' > mnav-ex-replay.txt
q "1 step(s) replayed from the log" mnav-ex-replay.txt
[ "$(sed -n 2p mnav-ex.txt)" = "$(sed -n 2p mnav-ex-replay.txt)" ]
rm -rf mnav-ex
$H --manual-example --manual-seed 7 --manual-logs mnav-ex --script "W,Up" --screenshot mnav-ex-w.png > /dev/null
q "^# example,70FAB974FA4E894380$" mnav-ex/*.csv
rm -rf mnav-ex
$H --manual-example --manual-seed 7 --manual-logs mnav-ex --script "R,W,Up" --screenshot mnav-ex-kept.png > /dev/null
q "^# example,133AC75170FAB974FA$" mnav-ex/*.csv
# The marks follow the walk (4 November 2026, Edward: undoing and redoing turned a green step red):
# three turns leave the start and red, green, red; two undos take the last two away; two redos put
# them back as they were, the green still green.
$H --manual-example --manual-seed 7 --script "Up,Down,Up" --screenshot mnav-marks.png | tr -d '\r' | q "^  marks: SRGR$"
$H --manual-example --manual-seed 7 --script "Up,Down,Up,Ctrl+Z,Ctrl+Z" --screenshot mnav-marks-u.png | tr -d '\r' | q "^  marks: SR$"
$H --manual-example --manual-seed 7 --script "Up,Down,Up,Ctrl+Z,Ctrl+Z,Ctrl+Y,Ctrl+Y" --screenshot mnav-marks-r.png | tr -d '\r' | q "^  marks: SRGR$"
# Steering (Edward, 4 November 2026): his walk, the first byte from C0 with 25 F7 63 after it. One
# turn moves the file 14.0076 gaps between bearings; 54 turns up, at F6, is nearest the centre
# (0.0034 from landing); there, the byte is as near as it can be.
printf '\300\045\367\143' > mnav-c0.bin
$H --manual-nav mnav-c0.bin --screenshot mnav-steer.png | tr -d '\r' | q "^  steering byte 0: one turn moves 14.0076 gaps; nearest the centre 54 turn(s) up, at F6 (about 0.0034 from landing)$"
ups=$(printf 'Up,%.0s' $(seq 54))
$H --manual-nav mnav-c0.bin --script "${ups%,}" --screenshot mnav-steered.png | tr -d '\r' > mnav-steered.txt
q "0.0034 from landing" mnav-steered.txt
q "^  steering byte 0: one turn moves 14.0076 gaps; this value is already its nearest to the centre$" mnav-steered.txt
# The exact check (Edward, 4 November 2026): the rotation as a fraction in its lowest terms. C0 25 F7
# 63 is 360 x 3240556644 / 4311810305 = 233320078368 / 862362061 (as Python's fractions make it),
# whose decimals repeat every 30840 digits; 54 AA lands on 120 degrees exactly. A file over 64 KiB
# waits for X.
$H --manual-nav mnav-c0.bin --screenshot mnav-exact.png | tr -d '\r' > mnav-exact.txt
q "^  exact: 360 x 3240556644 / 4311810305 = 233320078368 / 862362061$" mnav-exact.txt
q "^  recurs: repeats every 30840 digits; as the fraction, 9 bytes; the file's own address 4: no shorter address, 5 bytes over$" mnav-exact.txt
printf '\124\252' > mnav-120.bin
$H --manual-nav mnav-120.bin --screenshot mnav-120.png | tr -d '\r' > mnav-120.txt
q "^  exact: 360 x 21931 / 65793 = 120 / 1$" mnav-120.txt
q "^  a whole number of degrees: written exactly, 2 bytes" mnav-120.txt
head -c 70000 /dev/zero > mnav-long.bin
$H --manual-nav mnav-long.bin --screenshot mnav-long.png | tr -d '\r' | q "^  X (or EXACT): the rotation exactly"
$H --manual-nav mnav-long.bin --script X --screenshot mnav-long-x.png | tr -d '\r' | q "^  exact: 360 x ([0-9]* bits) / ([0-9]* bits) = "
# Undo as deep as the steps go (the default): 15 turns, 15 undos, the file as loaded; at a depth of
# 10 (U), 5 turns are left.
ups=$(printf 'Up,%.0s' $(seq 15)); zs=$(printf 'Ctrl+Z,%.0s' $(seq 15))
$H --manual-nav mnav.bin --script "${ups}${zs%,}" --screenshot mnav-undo.png | tr -d '\r' | q "^  rotation 2.8 degrees"
$H --manual-nav mnav.bin --script "U,${ups}${zs%,}" --screenshot mnav-undo10.png | tr -d '\r' | q "^  rotation 9.8 degrees"
$H --main-menu --press "Item:manual_nav" --screenshot main-mnav.png | q "saved main-mnav.png"
$H --main-menu --read-aloud --press "Item:docs" --screenshot main-docs.png | tr -d '\r' | q "^read aloud: Documentation$"
$H --designer --screenshot designer.png | q "designer: my-words-v1"
$H --designer --busy --screenshot designer-busy.png | q "designer: my-words-v1"
# Opening the designer compiles nothing: only F5 (TEST NOW), Save or the relation check do.
idle=$($H --designer --timings --screenshot designer-idle.png 2>&1) # (its own line: a crash fails the step)
test "$(printf '%s\n' "$idle" | { grep -c '^designer.test ' || true; })" = 0
$H --designer --design tests/plugins/toy-grammar-v1.sfilter --design-out tests/plugins/roundtrip-v1.sfilter --screenshot designer2.png | q "designer: toy-grammar-v1"
build/sieve filters --plugin tests/plugins/toy-grammar-v1.sfilter --length 11 | grep -v "^sha256" > rt.a
build/sieve filters --plugin tests/plugins/roundtrip-v1.sfilter --length 11 | grep -v "^sha256" > rt.b
diff rt.a rt.b
rm tests/plugins/roundtrip-v1.sfilter
rm -f build/filters/designed-words-v*.sfilter build/filters/designed-words-v*.sieve
$H --designer --script "Ctrl+N,Down,Return,Ctrl+Backspace,=designed-words,Return,Ctrl+S" --screenshot designer3.png | q "status: saved to .*designed-words-v1.sieve"
# A melody filter, as the Media Player's M opens it, heard as it is tested (IDEAS 19.19): F5 plays the
# first survivor, P in the test panel the next (or says why not, where the machine has no sound);
# and a filter of sound itself (pcm*) tested and played the same way.
$H --designer --design-melody --script F5,Tab,P --screenshot designer-melody.png > dmel.txt
q "^designer: my-melody-v1" dmel.txt
q "^designer played: 2 (" dmel.txt
$H --designer --design data/filters/pcm-quiet-ends-v1.sfilter --script F5 --screenshot designer-pcm.png > dpcm.txt
q "^designer: pcm-quiet-ends-v1" dpcm.txt
q "^designer played: 1 (" dpcm.txt
build/sieve filters --plugins | tr -d '\r' | q "^loaded   designed-words-v1  .*designed-words-v1.sieve (packed)$"
build/sieve filters --plugin build/filters/designed-words-v1.sieve --length 6 --relations | q "the same rule as words-data-v1  -- a duplicate"
$H --designer --design build/filters/designed-words-v1.sieve --script "Down,Down,Down,Down,Down,Down,Down,Return,=!,Return,Ctrl+S" --screenshot designer4.png | q "save as version 2"
rm build/filters/designed-words-v1.sieve
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
# Personal Dimensions on the pause menu opens the main menu's Personal Dimensions screen over the
# hallway (Esc there, or from your world, back to it), not the setup menu (Edward's testing, 14 November).
$H --no-menu --press "Escape,Item:personal,Return" --screenshot pause-personal.png | tr -d '\r' | q "^goes to: Personal Dimensions$"
# The Media Player (pause menu, sixth row): with --media-player a silent music player is
# made for the picture; its settings file goes where --settings says, not beside the program.
mkdir -p mp
$H --no-menu --media-player --settings mp/hallway.ini --press "Escape,Item:media,Return" --screenshot media.png | q "saved media.png"
# Its rings (IDEAS 19.175): R opens them; a C major chord held (C3, C4, E4, G4, C5) lights those
# five notes of their octaves, one ring an octave, and the lens zooms to the overtones and the waves.
$H --no-menu --media-player --settings mp/hallway.ini --press "Escape,Item:media,Return,r" --screenshot rings-open.png | tr -d '\r' | q "^rings: zoom 0;"
$H --no-menu --rings 48,60,64,67,72 --screenshot rings.png | tr -d '\r' | q "^rings: zoom 0; C3 1.00 C4 1.00 E4 1.00 G4 1.00 C5 1.00$"
$H --no-menu --rings 48,60,64,67,72 --rings-zoom 2 --screenshot rings-waves.png | tr -d '\r' | q "^rings: zoom 2; "
# Item:ID presses (IDEAS 19.188): a row chosen by its name; a name the screen has not is refused.
if $H --main-menu --press "Item:no_such_row" --screenshot noitem.png > /dev/null 2>&1; then exit 1; fi
$H --main-menu --press "Item:settings,Return,Item:game_systems,Return,Item:ai_explorers" --read-aloud --screenshot item.png | tr -d '\r' | q "^read aloud: AI Explorers"
# Your personal dimension's world (IDEAS 19.187): a grassy hill with your character on it. Its
# catalogue is Sieve's shelf items, read from data/objects/shelf-items.sieve (checked, in memory),
# apart from the meshes folder anyone may change; Tab, Return places the first (a page), Down twice
# and Return the third (a record sleeve), and both are kept in the world file (sieve-personal-world-v1)
# and read back when it opens again. Backspace takes the nearest away; a line not as written is said.
rm -rf pw && mkdir -p pw
$H --personal-world --world-file pw/p.world --press "Tab,Return,Down,Down,Return" --pworld-back 2 --screenshot pworld.png | tr -d '\r' > pw/out.txt
q "^personal world: seed 0; .*; 2 objects placed$" pw/out.txt
if grep -q "^catalogue: not read" pw/out.txt; then exit 1; fi
head -2 pw/p.world | tr -d '\r' | tr '\n' '|' | q "^sieve-personal-world-v1|seed 0|$"
grep -c "^place pages " pw/p.world | q "^1$"
grep -c "^place audio " pw/p.world | q "^1$"
$H --personal-world --world-file pw/p.world --screenshot pworld2.png | tr -d '\r' | q "; 2 objects placed$"
$H --personal-world --world-file pw/p.world --press "Backspace" --screenshot pworld3.png | tr -d '\r' | q "; 1 objects placed$"
grep -c "^place " pw/p.world | q "^1$"
# Its features (IDEAS 19.197), Personal Dimensions > Features: the map and the compass, each on until
# turned off there, and kept in the settings file.
$H --personal-world --world-file pw/p.world --screenshot pworld-features.png | tr -d '\r' | q "^features: map on, compass on$"
$H --main-menu --settings pw/f.ini --press "Item:personal,Return,Item:personal_features,Return,Item:personal_map,Return" --screenshot features-menu.png > /dev/null
grep -c "^map = off" pw/f.ini | q "^1$"
$H --personal-world --settings pw/f.ini --world-file pw/p.world --screenshot pworld-nomap.png | tr -d '\r' | q "^features: map off, compass on$"
# Esc in your dimension opens its pause menu (Edward's testing, 14 November): the pause menus' rows
# from one list (pause_rows.hpp), the hallway's tools left out here at first; Personal Dimensions >
# Features puts them in (kept as [personal] hallway_tools). A system's row chosen goes to it.
$H --personal-world --world-file pw/p.world --press "Escape" --screenshot pworld-pause.png | tr -d '\r' | q "^pause menu: resume, locator, media, designer, harness, settings, cipher, personal, systems, leave, exit$"
$H --personal-world --world-file pw/p.world --press "Escape,Item:locator,Return" --screenshot pworld-pause2.png | tr -d '\r' | q "^goes to: locator$"
$H --main-menu --settings pw/t.ini --press "Item:personal,Return,Item:personal_features,Return,Item:personal_hallway_tools,Return" --screenshot features-tools.png > /dev/null
grep -c "^hallway_tools = on" pw/t.ini | q "^1$"
$H --personal-world --settings pw/t.ini --world-file pw/p.world --press "Escape" --screenshot pworld-pause3.png | tr -d '\r' | q "^pause menu: resume, restart, graph, navigator, locator, "
# Favourites (IDEAS 19.190): H keeps the item in hand as a favourite of your dimension (beside the
# settings: favourites/SHA.item and worlds/personal.favourites); your dimension's catalogue lists it
# after the shelf items, placed it stands as its line's item, O on it opens it, and the hallway takes
# it into your hand on its line; the main menu's Favourites lists every one.
rm -rf fv && mkdir fv
$H --no-menu --settings fv/s.ini --warp "it was the best of times" --take --press "h" --screenshot fav-keep.png | tr -d '\r' | q "Kept as a favourite of your personal dimension: it was the best of times"
SHA=$(sed -n 2p fv/worlds/personal.favourites | tr -d '\r')
grep -c "^title it was the best of times$" "fv/favourites/$SHA.item" | q "^1$"
# Its picture as the hallway draws it on the shelf, kept beside it, for your dimension to draw on its
# front (Edward's testing, 14 November: a placed page showed no imagery).
python3 -I -c "import sys; d=open(sys.argv[1],'rb').read(24); print(d[:8]==b'\x89PNG\r\n\x1a\n', int.from_bytes(d[16:20],'big'))" "fv/favourites/$SHA.png" | tr -d '\r' | q "^True 128$"
$H --personal-world --settings fv/s.ini --pworld-look -45 --press "Tab,Down,Down,Down,Down,Down,Down,Down,Down,Down,Down,Down,Down,Return,Tab,o" --screenshot fav-open.png | tr -d '\r' | q "^opened: pages $SHA$"
grep -c "^place fav:$SHA " fv/worlds/personal.world | q "^1$"
# O opens it in place (IDEAS 19.208): the hallway's own viewer over your dimension, its line made in
# the background (a small calculating by the crosshair meanwhile) and kept, Esc back to the dimension;
# made under other settings of its line, or with [personal] open_in_place off, the hallway as before.
$H --personal-world --settings fv/s.ini --pworld-in-place --pworld-look -45 --press "o" --screenshot fav-inplace.png | tr -d '\r' | q "^in place: open, pages $SHA; the hallway's viewer, TEXT; its line made for it$"
$H --personal-world --settings fv/s.ini --pworld-in-place --pworld-look -45 --press "o,Tab" --screenshot fav-inplace-tab.png | tr -d '\r' | q "^in place: open, pages $SHA; the hallway's viewer, PICTURE; "
$H --personal-world --settings fv/s.ini --pworld-in-place --pworld-look -45 --press "o,Escape" --screenshot fav-inplace-shut.png | tr -d '\r' | q "^in place: pages $SHA: closed, back in your dimension$"
$H --personal-world --settings fv/s.ini --pworld-in-place --pworld-look -45 --press "o,Escape,o" --screenshot fav-inplace-kept.png | tr -d '\r' | q "^in place: open, pages $SHA; .*its line kept from before$"
$H --personal-world --settings fv/s.ini --pworld-in-place --pworld-making --pworld-look -45 --press "o" --screenshot fav-inplace-making.png | tr -d '\r' | q "^in place: calculating pages $SHA$"
$H --personal-world --settings fv/s.ini --pworld-in-place --pworld-making --pworld-look -45 --press "o,Escape" --screenshot fav-inplace-left.png | tr -d '\r' | q "^in place: pages $SHA: not opened: Esc while it was made$"
printf '[setup]\nlength = 40\n' > fv/len.ini
$H --personal-world --settings fv/len.ini --pworld-in-place --pworld-look -45 --press "o" --screenshot fav-inplace-fell.png | tr -d '\r' > fv/fell.txt
q "^in place: pages $SHA: the hallway instead: its line is set otherwise now than when it was kept$" fv/fell.txt
q "^opened: pages $SHA$" fv/fell.txt
printf '[personal]\nopen_in_place = off\n' > fv/ip.ini
$H --personal-world --settings fv/ip.ini --pworld-in-place --pworld-look -45 --press "o" --screenshot fav-inplace-off.png | tr -d '\r' | q "^in place: off (\[personal\] open_in_place): the hallway instead, as before$"
$H --main-menu --settings fv/ti.ini --press "Item:personal,Return,Item:personal_features,Return,Item:personal_open_in_place,Return" --screenshot features-inplace.png > /dev/null
grep -c "^open_in_place = off " fv/ti.ini | q "^1$"
# Favourites' covers in the catalogue (IDEAS 19.190): each favourite's picture beside its row and the
# chosen one's larger below the list (on by default); [personal] catalogue_covers off leaves the
# catalogue as it was, and Personal Dimensions > Features turns it off.
$H --personal-world --settings fv/s.ini --press "Tab,Down,Down,Down,Down,Down,Down,Down,Down,Down,Down,Down,Down" --screenshot fav-covers.png | tr -d '\r' | q "^covers: on, 1 of 1 favourites with one; the chosen row's 128x[0-9]*, shown$"
printf '[personal]\ncatalogue_covers = off\n' > fv/nc.ini
$H --personal-world --settings fv/nc.ini --press "Tab,Down,Down,Down,Down,Down,Down,Down,Down,Down,Down,Down,Down" --screenshot fav-covers-off.png | tr -d '\r' | q "^covers: off, 1 of 1 favourites with one; the chosen row's 128x[0-9]*, not shown$"
$H --personal-world --settings fv/s.ini --press "Tab,Down" --screenshot fav-covers-shelf.png | tr -d '\r' | q "^covers: on, 1 of 1 favourites with one$"
$H --personal-world --settings fv/s.ini --screenshot fav-covers-shut.png | tr -d '\r' | { grep -c "^covers:" || true; } | q "^0$"
$H --main-menu --settings fv/tc.ini --press "Item:personal,Return,Item:personal_features,Return,Item:personal_catalogue_covers,Return" --screenshot features-covers.png > /dev/null
grep -c "^catalogue_covers = off " fv/tc.ini | q "^1$"
$H --main-menu --settings fv/s.ini --read-aloud --press "Item:personal,Return,Item:personal_favourites,Return" --screenshot favs.png | tr -d '\r' | q "^read aloud: it was the best of times, pages, in personal$"
$H --no-menu --settings fv/s.ini --line text --open-favourite "$SHA" --screenshot fav-hand.png | tr -d '\r' | q "^favourite: in hand$"
$H --no-menu --settings fv/s.ini --warp "it was the best of times" --take --press "h" --screenshot fav-off.png | tr -d '\r' | q "No longer a favourite"
# Every item opens in the viewer made for it (IDEAS 19.200): a model kept from the compacted line
# (filters mr.ini) is kept by its place, and opens with the filters off on its 3D view. A file has no
# viewer of its own yet, so it is shown as its bytes and its kind is flagged in viewers-needed.txt.
rm -rf fm && mkdir fm
printf "[models]\nmode = compact\nfilters = distinct-vertices-v1, distinct-indices-v1, every-vertex-used-v1\n" > fm/mr.ini
$H --no-menu --settings fm/s.ini --line models --filters fm/mr.ini --walk "0,0" --press "e,h" --screenshot fav-model.png | tr -d '\r' | q "Kept as a favourite of your personal dimension: "
MSHA=$(sed -n 2p fm/worlds/personal.favourites | tr -d '\r')
grep -c "^index [0-9a-f]*$" "fm/favourites/$MSHA.item" | q "^1$"
$H --no-menu --settings fm/s.ini --line models --open-favourite "$MSHA" --screenshot fav-model-open.png | tr -d '\r' | sed -n 1,2p | tr '\n' ' ' | q "^favourite: in hand viewer: 3D $"
$H --no-menu --settings fm/s.ini --line binary --press "e,z" --screenshot noviewer.png 2>&1 | tr -d '\r' | q "^viewer: none for .* files yet$"
grep -c . fm/viewers-needed.txt | q "^1$"
# A file whose kind has a viewer opens in it first, its bytes next: a PNG as its picture (read as
# sieve-view reads one, every limit checked first), a text file as its text; neither is flagged.
$H --no-menu --settings fm/s.ini --line image --walk "0,0" --press "E,J,Z" --screenshot openpng.png 2>&1 | tr -d '\r' | q "viewing its PIXELS, 10 x 10 at "
$H --no-menu --settings fm/s.ini --warp "hello world" --press "J,Z" --screenshot opentxt.png 2>&1 | tr -d '\r' | q "viewing its TEXT, "
grep -c . fm/viewers-needed.txt | q "^1$"
# Sound and models too (list V1): a WAV and an AIFF open as their waveform, a MIDI file as its notes,
# either played with Space; an .obj and a .glb open in three dimensions, a glTF whose buffer is a file
# beside it is said to be unreadable from its bytes and shown as its text; a FLAC whose STREAMINFO says
# more samples than the viewer takes is refused before it is decoded, its kind not flagged; an MP3 has
# no decoder here yet, so it is flagged.
python - <<'EOF'
import json, math, struct, wave
w = wave.open("fm/tone.wav", "wb"); w.setnchannels(2); w.setsampwidth(2); w.setframerate(8000)
w.writeframes(b"".join(struct.pack("<hh", int(9000 * math.sin(i / 3)), int(5000 * math.sin(i / 5))) for i in range(4000))); w.close()
data = b"".join(struct.pack(">h", int(9000 * math.sin(i / 4))) for i in range(4000))
comm = struct.pack(">hIh", 1, 4000, 16) + struct.pack(">HQ", 16383 + 12, int(8000 / 2 ** 12 * 2 ** 63))
body = b"AIFF" + b"COMM" + struct.pack(">I", len(comm)) + comm + b"SSND" + struct.pack(">I", 8 + len(data)) + bytes(8) + data
open("fm/tone.aiff", "wb").write(b"FORM" + struct.pack(">I", len(body)) + body)
def trk(b): return b"MTrk" + struct.pack(">I", len(b)) + b
notes = b"".join(bytes([0, 0x90, p, 100, 0x60, 0x80, p, 0]) for p in (60, 62, 64, 65, 67)) + b"\0\xff\x2f\0"
open("fm/tune.mid", "wb").write(b"MThd" + struct.pack(">IHHH", 6, 1, 2, 96) + trk(b"\0\xff\x51\x03\x0a\x2c\x2a\0\xff\x2f\0") + trk(notes))
v = [(-1,-1,-1),(1,-1,-1),(1,1,-1),(-1,1,-1),(-1,-1,1),(1,-1,1),(1,1,1),(-1,1,1)]
f = [(1,2,3),(1,3,4),(5,7,6),(5,8,7),(1,5,6),(1,6,2),(2,6,7),(2,7,3),(3,7,8),(3,8,4),(4,8,5),(4,5,1)]
open("fm/cube.obj", "w").write("".join("v %d %d %d\n" % p for p in v) + "".join("f %d %d %d\n" % t for t in f))
bin_ = b"".join(struct.pack("<3f", *p) for p in v) + b"".join(struct.pack("<3H", *(i - 1 for i in t)) for t in f)
def gltf(buffer):
    return {"asset": {"version": "2.0"}, "buffers": [buffer],
            "bufferViews": [{"buffer": 0, "byteLength": 96}, {"buffer": 0, "byteOffset": 96, "byteLength": 72}],
            "accessors": [{"bufferView": 0, "componentType": 5126, "count": 8, "type": "VEC3"},
                          {"bufferView": 1, "componentType": 5123, "count": 36, "type": "SCALAR"}],
            "meshes": [{"primitives": [{"attributes": {"POSITION": 0}, "indices": 1}]}], "nodes": [{"mesh": 0}], "scenes": [{"nodes": [0]}]}
open("fm/cube.gltf", "w").write(json.dumps(gltf({"byteLength": 168, "uri": "cube.bin"})))
js = json.dumps(gltf({"byteLength": 168})).encode(); js += b" " * (-len(js) % 4)
open("fm/cube.glb", "wb").write(b"glTF" + struct.pack("<II", 2, 12 + 8 + len(js) + 8 + len(bin_)) + struct.pack("<I", len(js)) + b"JSON" + js +
                                struct.pack("<I", len(bin_)) + b"BIN\0" + bin_)
# STREAMINFO alone: blocks of 4096, 44100 a second, 2 channels, 16 bits, 2^36 - 1 samples a channel
si = struct.pack(">HH", 4096, 4096) + bytes(6) + ((44100 << 44) | (1 << 41) | (15 << 36) | ((1 << 36) - 1)).to_bytes(8, "big") + bytes(16)
open("fm/huge.flac", "wb").write(b"fLaC" + bytes([0x80, 0, 0, 34]) + si)
EOF
cp tests/mp3/tone.mp3 fm/tone.mp3
opened() { $H --no-menu --settings fm/s.ini --line binary --binary-length "$(wc -c < "fm/$1")" --warp "fm/$1" --take --press "$2" --screenshot "$3" 2>&1 | tr -d '\r'; }
opened tone.wav "Z" open-wav.png | q "| viewing its SOUND, "
# (played into SDL's dummy audio device: the runners have no sound card, and every other check here
# plays nothing)
export SDL_AUDIO_DRIVER=dummy
opened tone.wav "Z,Space" open-wav-play.png | q "| playing$"
opened tone.aiff "Z" open-aiff.png | q "| viewing its SOUND, "
opened tune.mid "Z" open-mid.png | q "| viewing its NOTES, "
opened tune.mid "Z,Space" open-mid-play.png | q "| playing$"
unset SDL_AUDIO_DRIVER
opened cube.obj "Z" open-obj.png | q "| viewing its 3D, "
opened cube.obj "Z,Tab" open-obj-text.png | q "| viewing its TEXT, "
opened cube.glb "Z" open-glb.png | q "| viewing its 3D, "
opened cube.gltf "Z" open-gltf.png | q "| This file looks like a model but could not be read as one (glTF: buffer 0 could not be read (cube.bin)), so it is shown as it is.$"
opened huge.flac "Z" open-huge.png | q "| This FLAC file could not be opened as its sound (it holds more samples than the viewer takes), so it is shown as its bytes.$"
grep -c . fm/viewers-needed.txt | q "^1$"
opened tone.mp3 "Z" open-mp3.png | q "^viewer: none for MP3 files yet$"
grep -c "^MP3$" fm/viewers-needed.txt | q "^1$"
# The Documentation (list V2, IDEAS 19.201): the help library's pages, links and keys checked against
# the code; it opens on a page, follows a link (Right, Tab, Enter) and comes back (Backspace), its INDEX
# is a Tab away, a search typed finds the pages holding its words, and a page it has not is said.
python tools/check_library.py
$H --docs welcome --screenshot docs.png | tr -d '\r' > docs.txt
q "^documentation: page welcome of [0-9]* pages in 6 categories; CONTENTS: " docs.txt
test "$(grep -c '^  status: ' docs.txt || true)" = 0
$H --docs welcome --script "Right,Tab,Return" --screenshot docs-link.png | tr -d '\r' | q "^documentation: page sieve-files of .*; 1 pages back$"
$H --docs welcome --script "Right,Tab,Return,Backspace" --screenshot docs-back.png | tr -d '\r' | q "^documentation: page welcome of .*; 0 pages back$"
$H --docs welcome --script "Tab" --screenshot docs-index.png | tr -d '\r' | q "^documentation: page welcome of .*; INDEX: [0-9]* rows; "
$H --docs welcome --script "=midi" --screenshot docs-search.png | tr -d '\r' | q "^  list: The viewer$"
$H --docs nosuchpage --screenshot docs-none.png | tr -d '\r' | q "^  status: No page of the library is called nosuchpage, so it opens on its first.$"
# The fractal dimension (list S, IDEAS 19.191, 19.235), a line of its own after AI: its first book the
# index of every fractal charted, each book after it one fractal, taken in hand as any item is and grown
# a step at a time (N, or =; B, or -), found by its name (warp) or its place (go to). (The status line
# is printed on a step or a key, so the checks with neither walk on the spot, --walk "0,0".)
$H --no-menu --line fractals --walk "0,0" --goto 0 --take --screenshot fractals-index.png | tr -d '\r' | q "^FRACTALS line, positional, filters: none, .*first book 0 \"Index of the fractal dimension\""
$H --no-menu --line fractals --walk "0,0" --warp dragon-curve --take --screenshot fractals-dragon.png | tr -d '\r' | q "| went to dragon-curve$"
$H --no-menu --line fractals --warp dragon-curve --take --press N,N --screenshot fractals-deeper.png | tr -d '\r' | q "| Depth 7 of 16.$"
$H --no-menu --line fractals --warp fibonacci-word --take --press B,B,B,B,B,B --screenshot fractals-first.png | tr -d '\r' | q "| It is at its first step, its axiom.$"
$H --no-menu --line fractals --walk "0,0" --warp nosuch --screenshot fractals-none.png | tr -d '\r' | q "No item on this line is called nosuch"
# The whole space (list X1, fractal-space-v1): every ruleset the reader takes, the shorter first, each
# charted fractal at its address there, named (the known fractals included, the default); excluded, a
# charted one is not on the line, nor the index, and Warp says which setting keeps it off and where it
# is changed; only they, the shelves are the chart's 197 after the index (X4's 155, Y2's automata 32 more, X5's attractors, flows and aggregate 10 more). And sieve fractal gives the
# same: the first ruleset of all, and each charted one back from its address (v2's and v3's among them,
# lists X2 and X3), but Voderberg's tiling, 590 characters, past the default longest (256).
$H --no-menu --line fractals --fractal-known only --walk "0,0" --goto 198 --screenshot fractals-past.png | tr -d '\r' | q "This line holds 198 items"
$H --no-menu --line fractals --walk "0,0" --warp voderberg-spiral-tiling --screenshot fractals-long.png | tr -d '\r' | q "voderberg-spiral-tiling is 590 characters, past the longest ruleset on this line, 256 "
$H --no-menu --line fractals --walk "0,0" --warp hilbert-curve-3d --take --screenshot fractals-3d.png | tr -d '\r' | q "| went to hilbert-curve-3d$"
# Beyond rewriting (list X4): Barnsley's fern as an IFS, the Mandelbrot set as an escape-time formula
# and Newton's z^3 - 1 as a convergence one, taken in hand; and Fractint's own name in 22 maps, 690
# characters, past the default longest.
$H --no-menu --line fractals --walk "0,0" --warp barnsley-fern --take --screenshot fractals-fern.png | tr -d '\r' | q "| went to barnsley-fern$"
$H --no-menu --line fractals --walk "0,0" --warp mandelbrot-set --take --screenshot fractals-mandelbrot.png | tr -d '\r' | q "| went to mandelbrot-set$"
$H --no-menu --line fractals --walk "0,0" --warp newton-z3-minus-1 --take --screenshot fractals-newton.png | tr -d '\r' | q "| went to newton-z3-minus-1$"
$H --no-menu --line fractals --walk "0,0" --warp fractint-ifs-fractint --screenshot fractals-long2.png | tr -d '\r' | q "fractint-ifs-fractint is 690 characters, past the longest ruleset on this line, 256 "
$H --no-menu --line fractals --walk "0,0" --warp orb-web --take --screenshot fractals-web.png | tr -d '\r' | q "| went to orb-web$"
$H --no-menu --line fractals --fractal-known only --walk "0,0" --warp dragon-curve --take --screenshot fractals-only.png | tr -d '\r' | q "| went to dragon-curve$"
$H --no-menu --line fractals --fractal-known exclude --walk "0,0" --warp dragon-curve --screenshot fractals-excluded.png | tr -d '\r' | q "dragon-curve is a known fractal, set aside: \"known fractals\" is set to exclude (Setup > FRACTALS, or the command line: --fractal-known)\.$"
$H --no-menu --line fractals --fractal-known exclude --walk "0,0" --warp index --screenshot fractals-excluded-index.png | tr -d '\r' | q "The index is set aside with the known fractals"
$H --no-menu --line fractals --fractal-known exclude --walk "0,0" --goto 0 --screenshot fractals-excluded-first.png | tr -d '\r' | q "first book 0 \"L-system 1, +\""
# Started on, the line opens at its index (IDEAS 19.243), not halfway along its space; with the known
# fractals excluded there is no index, and it opens halfway along.
$H --no-menu --line fractals --walk "0,0" --screenshot fractals-start.png | tr -d '\r' | q "first book 0 \"Index of the fractal dimension\".*went to index$"
$H --no-menu --line fractals --fractal-known exclude --walk "0,0" --screenshot fractals-start-excluded.png | tr -d '\r' | q "50\.0000000000% along.*went to 50%$"
$H --no-menu --line fractals --fractal-longest 60 --walk "0,0" --warp gosper-curve --screenshot fractals-short.png | tr -d '\r' | q "gosper-curve is 82 characters, past the longest ruleset on this line, 60 (Setup > FRACTALS, or the command line: --fractal-longest)\.$"
# Its noise filters ([fractals], judge-only): a ruleset that draws nothing fails fractal-draws-v1, marked
# where it stands; and sieve filters judges rulesets spread over the space, the same on every machine.
printf '[fractals]\nmode = mark\nfilters = fractal-draws-v1, fractal-grows-v1\n' > fractals-mark.ini
$H --no-menu --filters fractals-mark.ini --line fractals --fractal-known exclude --walk "0,0" --goto 0 --screenshot fractals-marked.png | tr -d '\r' | q "^FRACTALS line, positional, filters: 2 mark, "
printf '[fractals]\nmode = hide\nfilters = fractal-draws-v1, fractal-grows-v1, fractal-no-idle-v1\n' > fractals-three.ini
build/sieve filters --line fractals --filters fractals-three.ini --sample 1000 | tr -d '\r' > fractal-sample.txt
q "^  fractal-draws-v1 passes 897 of 1000 (89.70%)$" fractal-sample.txt
q "^  fractal-dimension-v1 passes 195 of 1000 (19.50%)$" fractal-sample.txt
q "^  the ticked stack passes 681 of 1000 (68.10%)$" fractal-sample.txt
q "^\[x\] fractal-no-idle-v1   (hard, judge-only)$" fractal-sample.txt
# In other lines (list X1c): the image line's pictures that are a charted fractal drawn, at each depth,
# fitted as a picture warped in is (fractal-drawn-v1): 5,279 at 10 x 10 in black and white (782 before
# X4's IFS and escape pictures, 1,057 before Y2's automata, an elementary one's history another picture
# at each depth, 5,244 before X5's attractors, flows and aggregate), counted and
# ranked exactly, so the line compacts to them (include) or closes up without them (exclude).
printf '[image]\nmode = compact\nfilters = fractal-drawn-v1\n[image.fractal-drawn-v1]\nkeep = include\n' > drawn-in.ini
printf '[image]\nmode = compact\nfilters = fractal-drawn-v1\n[image.fractal-drawn-v1]\nkeep = exclude\n' > drawn-out.ini
build/sieve filters --line image --filters drawn-in.ini | tr -d '\r' | q "^survivors    5279 (exact; compact mode available)$"
build/sieve filters --line image --filters drawn-out.ini | tr -d '\r' | q "^excluded     5279 (exact)$"
# Version 2 (IDEAS 19.244), what the FRACTALS row sets: each picture also turned and mirrored (eight ways,
# 31,190) and inverted (62,356); with colours = any, every colouring of each shape (on black and white the
# same 62,356).
printf '[image]\nmode = compact\nfilters = fractal-drawn-v2\n[image.fractal-drawn-v2]\nkeep = include\n' > drawn2-in.ini
printf '[image]\nmode = compact\nfilters = fractal-drawn-v2\n[image.fractal-drawn-v2]\nkeep = include\ncolours = drawn\n' > drawn2-turned.ini
printf '[image]\nmode = compact\nfilters = fractal-drawn-v2\n[image.fractal-drawn-v2]\nkeep = exclude\ncolours = any\n' > drawn2-any-out.ini
build/sieve filters --line image --filters drawn2-in.ini | tr -d '\r' | q "^survivors    62356 (exact; compact mode available)$"
build/sieve filters --line image --filters drawn2-turned.ini | tr -d '\r' | q "^survivors    31190 (exact; compact mode available)$"
build/sieve filters --line image --filters drawn2-any-out.ini | tr -d '\r' | q "^excluded     62356 (exact)$"
$H --no-menu --filters drawn2-in.ini --line image --walk "0,0" --screenshot image-fractals.png | tr -d '\r' | q "^IMAGE line, positional, filters: 1 compact (62356 survivors), "
# And the video line (list Y4): a charted fractal growing, its frames its depths in turn, 1,774 at
# 10 x 10 and 4 frames in black and white, counted exactly both ways round.
printf '[video]\nmode = compact\nfilters = fractal-moving-v1\n[video.fractal-moving-v1]\nkeep = include\n' > moving-in.ini
printf '[video]\nmode = compact\nfilters = fractal-moving-v1\n[video.fractal-moving-v1]\nkeep = exclude\n' > moving-out.ini
build/sieve filters --line video --width 10 --height 10 --frames 4 --palette mono --filters moving-in.ini | tr -d '\r' | q "^survivors    1774 (exact; compact mode available)$"
build/sieve filters --line video --width 10 --height 10 --frames 4 --palette mono --filters moving-out.ini | tr -d '\r' | q "^excluded     1774 (exact)$"
# The video line's own (list Z5): still videos set aside, and frames that change a little kept, counted
# exactly and judged (each frame against the one before).
printf '[video]\nmode = mark\nfilters = video-moves-v1\n' > moves.ini
printf '[video]\nmode = mark\nfilters = video-smooth-v1\n[video.video-smooth-v1]\npercent = 5\n' > smooth.ini
build/sieve filters --line video --width 10 --height 10 --frames 4 --palette mono --filters moves.ini | tr -d '\r' | q "^excluded     1267650600228229401496703205376 (exact)$"
build/sieve filters --line video --width 10 --height 10 --frames 4 --palette mono --filters smooth.ini | tr -d '\r' | q "^survivors    633955717147747338836654899364929801782347986355879936 (exact; judged)$"
# The pages (list Z7): few values, two-symbol stretches, the golden ratio's word; and the fractals' words and
# melodies (list Z4).
printf '[text]\nmode = mark\nfilters = few-values-v1\n' > few.ini
printf '[text]\nmode = mark\nfilters = golden-word-v1\n' > golden.ini
printf '[text]\nmode = compact\nfilters = fractal-written-v1\n' > written.ini
printf '[audio]\nmode = compact\nfilters = fractal-scored-v1\n' > scored.ini
build/sieve filters --line text --filters few.ini | tr -d '\r' | q "^excluded     1507533520221 (exact)$"
build/sieve filters --line text --filters golden.ini | tr -d '\r' | q "^survivors    23166 (exact; judged)$"
build/sieve filters --line text --alphabet ascii96 --filters written.ini | tr -d '\r' | q "^survivors    531 (exact; compact mode available)$"
build/sieve filters --line audio --filters scored.ini | tr -d '\r' | q "^survivors    3548 (exact; compact mode available)$"
# Lists Z8 and Z9: pages said aloud, capitals and brackets where writing puts them, words of a length;
# pictures their own mirror image; films of the Game of Life and of a camera panning.
printf '[text]\nmode = mark\nfilters = pronounceable-v1\n' > spoken.ini
printf '[text]\nmode = mark\nfilters = letter-case-v1\n' > case.ini
printf '[text]\nmode = mark\nfilters = brackets-v1\n' > brackets.ini
printf '[text]\nmode = mark\nfilters = word-length-v2\n' > wordlen.ini
printf '[image]\nmode = mark\nfilters = image-mirror-v1\n' > mirror.ini
printf '[video]\nmode = mark\nfilters = life-causal-v1\n' > life.ini
printf '[video]\nmode = mark\nfilters = video-pan-v1\n' > pan.ini
build/sieve filters --line text --length 1000 --filters spoken.ini | tr -d '\r' | q "keeps 10^-29.07 of the line"
build/sieve filters --line text --alphabet ascii96 --length 1000 --filters case.ini | tr -d '\r' | q "keeps 10^-42.46 of the line"
build/sieve filters --line text --alphabet ascii96 --length 1000 --filters brackets.ini | tr -d '\r' | q "keeps 10^-14.87 of the line"
build/sieve filters --line text --length 1000 --filters wordlen.ini | tr -d '\r' | q "keeps 10^-12.92 of the line"
build/sieve filters --line image --filters mirror.ini | tr -d '\r' | q "^survivors    1125899906842624 (exact; judged)$"
build/sieve filters --line video --width 10 --height 10 --frames 4 --palette mono --filters life.ini | tr -d '\r' | q "^survivors    1267650600228229401496703205376 (exact; judged)$"
build/sieve filters --line video --width 10 --height 10 --frames 4 --palette mono --filters pan.ini | tr -d '\r' | q "^survivors    1361129467683753853853498429727072845824 (exact; judged)$"
# Version 2: each video turned, mirrored and inverted, every frame alike: 23,830.
printf '[video]\nmode = compact\nfilters = fractal-moving-v2\n[video.fractal-moving-v2]\nkeep = include\n' > moving2-in.ini
build/sieve filters --line video --width 10 --height 10 --frames 4 --palette mono --filters moving2-in.ini | tr -d '\r' | q "^survivors    23830 (exact; compact mode available)$"
dragon=$(build/sieve fractal index dragon-curve | tr -d '\r' | sed -n 's/^address   [0-9]* (0x\([0-9a-f]*\)).*/\1/p')
test -n "$dragon"
$H --no-menu --line fractals --walk "0,0" --warp dragon-curve --screenshot fractals-space.png | tr -d '\r' | q "^FRACTALS line, positional, filters: none, tile [0-9.]* (39 digits)"
build/sieve fractal at 0 | tr -d '\r' > fractal-first.txt
printf 'sieve-fractal-v1\nlsystem 1\naxiom +\nrule A +\n' | cmp - fractal-first.txt
build/sieve fractal count | tr -d '\r' | q "^  44 characters: 20010504322671072$"
for f in data/fractals/*.fractal; do
    if [ "$(wc -c < "$f")" -gt 256 ]; then continue; fi
    a=$(build/sieve fractal index "$f" | tr -d '\r' | sed -n 's/^address   \([0-9]*\) .*/\1/p')
    build/sieve fractal at "$a" | tr -d '\r' | cmp - "$f"
done
# A bookcase in the catalogue (the corridor's own, kept in shelf-items.sieve), and things rest on what
# is beneath them: a page dropped over the case lands on its top, one dropped inside on a shelf.
$H --personal-world --world-file pw/bc.world --press "Tab,Down,Down,Down,Down,Down,Down,Down,Down,Down,Down,Down,Return" --screenshot pworld-case.png > /dev/null
grep -c "^place bookcase " pw/bc.world | q "^1$"
# Your characters are objects of the world (Edward, 14 November 2026): yours stands ahead of where you
# begin, and nothing of it is written while it stays there (the world still v1); picked up (E), turned
# (R) and placed (Return), the world says where (v2, "characters placed"); taken away (Backspace), it
# stays away; and the catalogue's "your character" places it again before you.
printf 'sieve-personal-world-v1\nseed 0\n' > pw/ch.world
$H --personal-world --world-file pw/ch.world --character c99.character --character-store pw/nostore --screenshot pworld-ch.png | tr -d '\r' | q "; 1 characters; 0 objects placed$"
head -1 pw/ch.world | tr -d '\r' | q "^sieve-personal-world-v1$"
$H --personal-world --world-file pw/ch.world --character c99.character --character-store pw/nostore --pworld-walk 1.0 --pworld-look -12 --press "e,r,Return" --screenshot pworld-ch2.png | tr -d '\r' | q "; char:you placed at 0.00, 1.05, -53.98$"
tr -d '\r' < pw/ch.world | grep -c "^characters placed$" | q "^1$"
tr -d '\r' < pw/ch.world | q "^place char:you 0 24.4472618 -53.9771004 0.261799395 0 0 fixed$"
$H --personal-world --world-file pw/ch.world --character c99.character --character-store pw/nostore --pworld-walk 1.0 --press "BackSpace" --screenshot pworld-ch3.png | tr -d '\r' | q "; 0 characters; 0 objects placed$"
$H --personal-world --world-file pw/ch.world --character c99.character --character-store pw/nostore --screenshot pworld-ch4.png | tr -d '\r' | q "; 0 characters; 0 objects placed$"
$H --personal-world --world-file pw/ch.world --character c99.character --character-store pw/nostore --press "Tab,Down,Down,Down,Down,Down,Down,Down,Down,Down,Down,Down,Down,Return" --screenshot pworld-ch5.png | tr -d '\r' | q "; 1 characters; 0 objects placed$"
tr -d '\r' < pw/ch.world | q "^place char:you 0 22.8426743 -58.5 3.14159274 0 0 fixed$"
printf 'sieve-personal-world-v2\nseed 0\nplace bookcase 0 22.9 -57 3.14159274 0 0 fixed\nplace pages -0.1 40 -56.82 0 0 0 physics\nplace audio 1.5 25.0 -56.9 3.14159274 0 0 physics\n' > pw/on.world
$H --personal-world --world-file pw/on.world --pworld-wait 6 --screenshot pworld-shelf.png | tr -d '\r' | q "pages dropped at -0.10, 2\.[78][0-9], .*audio dropped at 1.50, 1\.9[0-9], "
# Every key on one registry (IDEAS 19.199): the map's zoom (+ and -), each step in halving what it
# shows; Settings > Controls > Keys lists every key by section, and one changed there (pick up, to F)
# is kept in the settings file under [keys] and used: F then picks up what E did.
$H --personal-world --world-file pw/p.world --press "=,=" --screenshot pworld-zoom.png | tr -d '\r' | q "^map: 160 m across$"
$H --personal-world --world-file pw/p.world --press "=,=,-" --screenshot pworld-zoom2.png | tr -d '\r' | q "^map: 320 m across$"
rm -rf kb && mkdir kb
$H --main-menu --settings kb/s.ini --read-aloud --press "Item:settings,Return,Item:controls,Return,Item:keys,Return" --screenshot keys.png | tr -d '\r' | q "^read aloud: Your personal dimension: moving: walk forward, W / Up$"
$H --main-menu --settings kb/s.ini --press "Item:settings,Return,Item:controls,Return,Item:keys,Return,Down,Down,Down,Down,Down,Down,Down,Down,Down,Down,Down,Down,Down,Down,Return,F" --screenshot keys2.png > /dev/null
grep -c "^personal.holding/pick_up = F " kb/s.ini | q "^1$"
$H --personal-world --settings kb/s.ini --pworld-look -45 --press "Tab,Return,Tab,f" --screenshot pworld-f.png | tr -d '\r' | q "pages held at "
# The hallway's own keys on the registry too (IDEAS 19.199), in sections after the Creator's so no row
# before them moves: walk forward is the first of them on Settings > Controls > Keys; one changed there
# (jump 1000 tiles on, to J) is kept under [keys] and used, PageUp then jumping no more; and the footer
# names the key as set (the readout says it only once a hallway key is changed). A key pressed with a
# modifier more than its chord asks is still that key unless another key asks it (Shift and - zooms out
# eight steps, not one).
rm -rf hk
mkdir hk
touch hk/s.ini
$H --main-menu --settings hk/s.ini --read-aloud --press "Item:settings,Return,Item:controls,Return,Item:keys,Return,Item:keyrow:66" --screenshot hk-row.png | tr -d '\r' | q "^read aloud: The hallway: walking: walk forward, W / Up$"
$H --main-menu --settings hk/s.ini --press "Item:settings,Return,Item:controls,Return,Item:keys,Return,Item:keyrow:83,Return,J" --screenshot hk-set.png > /dev/null
grep -c "^hallway.anywhere/thousand_on = J " hk/s.ini | q "^1$"
$H --no-menu --settings hk/s.ini --press "J" --screenshot hk-j.png | tr -d '\r' > hk/j.txt
q "| jumped 1000 tiles (128000 books)$" hk/j.txt
q "^keys footer: WASD move .* wheel/J/\[ \] jump " hk/j.txt
$H --no-menu --settings hk/s.ini --press "PageUp" --screenshot hk-pgup.png | tr -d '\r' > hk/pgup.txt
test "$(grep -c "jumped 1000 tiles" hk/pgup.txt || true)" = 0
$H --no-menu --press "PageUp" --screenshot hk-default.png | tr -d '\r' > hk/default.txt
q "| jumped 1000 tiles (128000 books)$" hk/default.txt
test "$(grep -c "^keys footer:" hk/default.txt || true)" = 0
$H --warp "$W" --press "M,M,Shift+-" --screenshot hk-far.png | q "PAGES line, guided zoom 50, "
# The other systems' keys on the registry too (IDEAS 19.199): the menus' (one shared section every list
# reads), the setup menu's, the Media Player's and its rings', the main menu's, the Encryption Engine's and
# the CHAIN tab's, in sections after the hallway's so no row before them moves (the menus' row up is the
# first of them). With the defaults no two keys read together clash and nothing more is said; one changed
# is used, its old key no longer, and the footer names it as set.
rm -rf mk
mkdir mk
touch mk/s.ini
$H --main-menu --settings mk/s.ini --read-aloud --press "Item:settings,Return,Item:controls,Return,Item:keys,Return,Item:keyrow:114" --screenshot mk-row.png | tr -d '\r' > mk/row.txt
q "^read aloud: The menus: any list: the row above, Up$" mk/row.txt
test "$(grep -c "^key clash:" mk/row.txt || true)" = 0
$H --main-menu --settings mk/s.ini --press "Item:settings,Return,Item:controls,Return,Item:keys,Return,Item:keyrow:115,Return,J" --screenshot mk-set.png > /dev/null
grep -c "^menus/down = J " mk/s.ini | q "^1$"
$H --menu --settings mk/s.ini --press "J,J,J,Down" --screenshot mk-menu.png | tr -d '\r' > mk/menu.txt
q "^setup row: 3$" mk/menu.txt
q "^setup footer: Up/J choose " mk/menu.txt
$H --menu --settings mk/d.ini --filters mk/d-filters.ini --press "Down,Down" --screenshot mk-default.png | tr -d '\r' > mk/default.txt
test "$(grep -c "^setup footer:" mk/default.txt || true)" = 0
printf '[keys]\nmenus/down = A\n' > mk/clash.ini
$H --main-menu --settings mk/clash.ini --screenshot mk-clash.png | tr -d '\r' | q "^key clash: A for menus/down and setup/alphabets$"
printf '[keys]\nsetup/details = Q\nmedia/rings = J\nmedia/go_to = K\nmenus/next = J\nchain/add = J\nmain/back = J\n' > mk/k.ini
$H --menu --settings mk/k.ini --press "Q" --screenshot mk-details.png | tr -d '\r' | q "^details: "
$H --menu --settings mk/k.ini --press "I" --screenshot mk-details2.png | tr -d '\r' > mk/i.txt
test "$(grep -c "^details:" mk/i.txt || true)" = 0
$H --no-menu --media-player --settings mk/k.ini --press "Escape,Item:media,Return,j" --screenshot mk-rings.png | tr -d '\r' > mk/rings.txt
q "^rings: zoom 0;" mk/rings.txt
q "^media footer: .*   K go to   " mk/rings.txt
$H --no-menu --media-player --settings mk/k.ini --press "Escape,Item:media,Return,r" --screenshot mk-rings2.png | tr -d '\r' > mk/r.txt
test "$(grep -c "^rings:" mk/r.txt || true)" = 0
$H --cipher --settings mk/k.ini --script "J" --screenshot mk-cipher.png | tr -d '\r' | q "^output: 61747461636b206174206461776e$"
$H --main-menu --settings mk/k.ini --read-aloud --press "Item:settings,Return,J" --screenshot mk-main.png | tr -d '\r' | q "^read aloud: Potentia Settings$"
$H --no-menu --settings mk/k.ini --line ai --walk "0,0" --take --chain --chain-press "Down,J,Right" --chain-save mk/j.chain --training-dir data/training --screenshot mk-chain.png > /dev/null
$H --no-menu --line ai --walk "0,0" --take --chain --chain-press "Down,N,Right" --chain-save mk/n.chain --training-dir data/training --screenshot mk-chain2.png > /dev/null
cmp mk/j.chain mk/n.chain
# Holding (IDEAS 19.192): E picks up what the crosshair is on, R and T turn it, Return places it where
# it is held (here a little above the grass, tipped), and the world is written as v2 for it; Q drops one,
# which falls and comes to rest on the grass; both read back. A v2 line this build does not know is
# kept and written back.
$H --personal-world --world-file pw/h.world --pworld-look -45 --press "Tab,Return,Tab,e,r,r,t,Return" --screenshot pworld-held.png | tr -d '\r' | q "; pages placed at [-0-9.]*, 0\.[0-9]*, "
head -1 pw/h.world | tr -d '\r' | q "^sieve-personal-world-v2$"
grep -c " fixed$" pw/h.world | q "^1$"
$H --personal-world --world-file pw/d.world --pworld-look -45 --press "Tab,Return,Tab,e,Q" --pworld-wait 3 --screenshot pworld-drop.png | tr -d '\r' | q "; pages dropped at [-0-9.]*, 0\.00, "
grep -c " physics$" pw/d.world | q "^1$"
$H --personal-world --world-file pw/d.world --screenshot pworld-drop2.png | tr -d '\r' | q "; pages dropped at [-0-9.]*, 0\.00, "
# Turning what you hold (IDEAS 19.221): a ring round it for each turn, coloured by its axis (x red, y
# green, z blue by default), no turn chosen until R, T or G is pressed; then the wheel (WheelUp and
# WheelDown in --press, a notch each) turns it about that turn, 15 degrees a notch. The rings can be
# turned off, the step and the axes' colours set; what is placed is written as before.
$H --personal-world --world-file pw/tr1.world --pworld-look -45 --press "Tab,Return,Tab,e" --screenshot pworld-rings.png | tr -d '\r' | q "^turning: none chosen, wheel 15 degrees a notch; yaw 180\.00, pitch 0\.00, roll 0\.00 degrees; rings 0\.[0-9]* m about "
$H --personal-world --world-file pw/tr2.world --pworld-look -45 --press "Tab,Return,Tab,e,t,WheelUp,WheelUp" --screenshot pworld-wheel.png | tr -d '\r' | q "^turning: pitch (z) chosen, wheel 15 degrees a notch; yaw 180\.00, pitch 45\.00, roll 0\.00 degrees; rings .*, x #FF4040, y #40D040, z #4080FF$"
$H --personal-world --world-file pw/tr3.world --pworld-look -45 --press "Tab,Return,Tab,e,r,WheelDown,g,WheelDown,WheelDown,Return" --screenshot pworld-wheel2.png | tr -d '\r' | q "; pages placed at "
grep -c "^place pages .* 3\.14159274 0 -0\.26179939[0-9]* fixed$" pw/tr3.world | q "^1$"
$H --personal-world --world-file pw/tr4.world --pworld-look -45 --press "Tab,Return,Tab,e,WheelUp" --screenshot pworld-wheel3.png | tr -d '\r' | q "^turning: none chosen, .*yaw 180\.00, pitch 0\.00, roll 0\.00 degrees"
printf '[graphics]\naxis_colours = #112233,#445566,#778899\n[personal]\nturning_rings = off\nwheel_step_degrees = 45\n' > pw/tr.ini
$H --personal-world --settings pw/tr.ini --world-file pw/tr5.world --pworld-look -45 --press "Tab,Return,Tab,e,r,WheelUp" --screenshot pworld-wheel4.png | tr -d '\r' | q "^turning: yaw (y) chosen, wheel 45 degrees a notch; yaw 240\.00, pitch 0\.00, roll 0\.00 degrees; rings off$"
printf '[graphics]\naxis_colours = #112233,#445566,#778899\n' > pw/tr2.ini
$H --personal-world --settings pw/tr2.ini --world-file pw/tr6.world --pworld-look -45 --press "Tab,Return,Tab,e" --screenshot pworld-wheel5.png | tr -d '\r' | q ", x #112233, y #445566, z #778899$"
$H --personal-world --world-file pw/tr1.world --screenshot pworld-wheel6.png | tr -d '\r' | { grep -c "^turning:" || true; } | q "^0$"
$H --main-menu --settings pw/tr.ini --press "Item:settings,Return,Item:graphics,Return,Item:axis_colours,Right" --screenshot axis-colours.png > /dev/null
grep -c "^axis_colours = #FF4040,#40D040,#4080FF " pw/tr.ini | q "^1$"
$H --main-menu --settings pw/tr.ini --press "Item:personal,Return,Item:personal_features,Return,Item:personal_turning_rings,Return,Item:personal_wheel_step,Left" --screenshot features-turning.png > /dev/null
grep -c "^turning_rings = on " pw/tr.ini | q "^1$"
grep -c "^wheel_step_degrees = 30 " pw/tr.ini | q "^1$"
# Day and night (IDEAS 19.222): off by default (no day: line, the sky as before); --pworld-time HOURS
# turns it on with the time of day held there, so midnight is dark with stars and noon is day with the
# sun high in the south. L opens the panel where the day's length is typed: 2^64 is refused (nothing
# written), 2^64 - 1 is kept as the world file's v2 line, read back, and a bad one is left out and said.
$H --personal-world --world-file pw/dn0.world --screenshot pworld-day-off.png | tr -d '\r' | { grep -c "^day:" || true; } | q "^0$"
$H --personal-world --real-graphics --world-file pw/dn0.world --pworld-time 0 --screenshot pworld-night.png | tr -d '\r' | q "^day: on, 1440 seconds a day (the default); time 00:00 (set); sun 53\.1 degrees down; daylight 0\.00; stars 1\.00$"
$H --personal-world --real-graphics --world-file pw/dn0.world --pworld-time 12 --screenshot pworld-noon.png | tr -d '\r' | q "^day: on, .*; time 12:00 (set); sun 53\.1 degrees up; daylight 1\.00; stars 0\.00$"
$H --personal-world --real-graphics --world-file pw/dn0.world --pworld-time 18 --screenshot pworld-dusk.png | tr -d '\r' | q "; time 18:00 (set); sun on the horizon; daylight 0\.[0-9]*; "
$H --personal-world --world-file pw/dn1.world --pworld-time 20 --press "l,1,8,4,4,6,7,4,4,0,7,3,7,0,9,5,5,1,6,1,6,Return" --screenshot pworld-day-big.png | tr -d '\r' | q "^note: 18446744073709551616 is not a day's length here"
test ! -e pw/dn1.world
$H --personal-world --world-file pw/dn1.world --pworld-time 20 --press "l,1,8,4,4,6,7,4,4,0,7,3,7,0,9,5,5,1,6,1,5,Return" --screenshot pworld-day-max.png | tr -d '\r' | q "^day: on, 18446744073709551615 seconds a day; "
head -3 pw/dn1.world | tr -d '\r' | tr '\n' '|' | q "^sieve-personal-world-v2|seed 0|day 18446744073709551615|$"
printf '[personal]\nday_night = on\n' > pw/dn.ini
$H --personal-world --settings pw/dn.ini --world-file pw/dn1.world --screenshot pworld-day-clock.png | tr -d '\r' | q "^day: on, 18446744073709551615 seconds a day; time [0-9][0-9]:[0-9][0-9] (the clock's); "
printf 'sieve-personal-world-v2\nseed 0\nday 0\n' > pw/dn2.world
$H --personal-world --settings pw/dn.ini --world-file pw/dn2.world --screenshot pworld-day-bad.png | tr -d '\r' | q "^note: Lines of the world file not as written, left out: 1\.$"
$H --main-menu --settings pw/tr.ini --press "Item:personal,Return,Item:personal_features,Return,Item:personal_day_night,Return" --screenshot features-day.png > /dev/null
grep -c "^day_night = on " pw/tr.ini | q "^1$"
# The ground shaper (IDEAS 19.209): put away until 1 takes it out (no shaper: line, nothing shaped);
# # or the key in its place (\) puts it away; ] and the wheel widen its brush. Z held raises the ground
# where the crosshair met it, kept as v2 `ground` lines (only the corners moved) and read back; X lowers
# and C flattens; what stood on the ground stays on it; lines not as written are left out and said;
# the radius and strength come from [personal], the keys from [keys].
$H --personal-world --world-file pw/gs0.world --press "1" --screenshot pworld-shaper.png | tr -d '\r' | q "^shaper: out, radius 20 m, strength 4 m a second; aimed at 0\.00, "
$H --personal-world --world-file pw/gs0.world --press "1,#" --screenshot pworld-shaper-away.png | tr -d '\r' | { grep -c "^shaper:" || true; } | q "^0$"
$H --personal-world --world-file pw/gs0.world --press "1,\\" --screenshot pworld-shaper-away2.png | tr -d '\r' | { grep -c "^shaper:" || true; } | q "^0$"
$H --personal-world --world-file pw/gs0.world --press "1,],WheelUp" --screenshot pworld-shaper-size.png | tr -d '\r' | q "^shaper: out, radius 24 m, "
test ! -e pw/gs0.world
$H --personal-world --real-graphics --world-file pw/gs1.world --pworld-look 2 --press "1" --pworld-hold Z:3 --screenshot pworld-raised.png | tr -d '\r' | q "; ground shaped at 10 corners$"
head -3 pw/gs1.world | tr -d '\r' | tr '\n' '|' | q "^sieve-personal-world-v2|seed 0|ground -10 -50 [0-9.]*|$"
grep -c "^ground 0 -40 1[0-9]\.[0-9]*$" pw/gs1.world | q "^1$"
$H --personal-world --world-file pw/gs1.world --pworld-walk 4 --screenshot pworld-raised2.png | tr -d '\r' | q "ground 3[0-9]\.[0-9][0-9] m; .*; ground shaped at 10 corners$"
$H --personal-world --real-graphics --world-file pw/gs2.world --pworld-look 2 --press "1" --pworld-hold X:3 --screenshot pworld-lowered.png | tr -d '\r' | q "; ground shaped at 10 corners$"
grep -c "^ground 0 -40 -1[0-9]\.[0-9]*$" pw/gs2.world | q "^1$"
cp pw/gs1.world pw/gs3.world
$H --personal-world --real-graphics --world-file pw/gs3.world --pworld-look 2 --press "1" --pworld-hold C:8 --screenshot pworld-flattened.png | tr -d '\r' | q "; ground shaped at 1[0-9] corners$"
$H --personal-world --world-file pw/gs4.world --pworld-look -20 --press "Tab,Return,Tab,1" --pworld-hold Z:2 --screenshot pworld-shaped-on.png | tr -d '\r' | q "; 1 objects placed; ground shaped at [0-9]* corners$"
printf 'sieve-personal-world-v2\nseed 0\nground 5 0 1\nground 0 0 1\nground 0 0 2\nground 0 10 x\nground 330 0 1\nground 0 20 0\nground 0 30 1 extra\nground 0 40 600\n' > pw/gs5.world
$H --personal-world --world-file pw/gs5.world --screenshot pworld-ground-bad.png | tr -d '\r' | q "^note: Lines of the world file not as written, left out: 7\.$"
$H --personal-world --world-file pw/gs5.world --screenshot pworld-ground-bad2.png | tr -d '\r' | q "; ground shaped at 1 corners$"
printf '[personal]\nshaper_radius = 30\nshaper_strength = 9\n[keys]\npersonal.tools/shaper = 2\n' > pw/gs.ini
$H --personal-world --settings pw/gs.ini --world-file pw/gs0.world --press "2" --screenshot pworld-shaper-set.png | tr -d '\r' | q "^shaper: out, radius 30 m, strength 9 m a second; "
$H --main-menu --settings pw/gs.ini --press "Item:personal,Return,Item:personal_features,Return,Item:personal_shaper_radius,Right,Item:personal_shaper_strength,Left" --screenshot features-shaper.png > /dev/null
grep -c "^shaper_radius = 32 " pw/gs.ini | q "^1$"
grep -c "^shaper_strength = 8 " pw/gs.ini | q "^1$"
# Place (Tab while holding, a panel sliding in as the setup menu's do): exact numbers typed into its
# rows, then placed there: x 1.5, y 25 m, a 45 degree turn, tipped 10 by two steps of 5.
$H --personal-world --world-file pw/x.world --pworld-look -45 --press "Tab,Return,Tab,e,Tab,1,.,5,Return,Down,2,5,Return,Down,Down,4,5,Return,Down,Right,Right,Down,Down,Return" --screenshot pworld-place.png | tr -d '\r' | q "; pages placed at 1\.50, "
grep -c "^place pages 1.5 25 [-0-9.]* 0.785398185 0.17453292 0 fixed$" pw/x.world | q "^1$"
printf 'sieve-personal-world-v2\nseed 0\nfuture-thing 1 2 3\nplace pages 0 30 -55 0 0 0 fixed\n' > pw/k.world
$H --personal-world --world-file pw/k.world --press "Backspace" --screenshot pworld-kept.png | tr -d '\r' | q "; 0 objects placed$"
grep -c "^future-thing 1 2 3$" pw/k.world | q "^1$"
# An own object (place model:SHA) whose model is not in the store is kept and written back (with its
# model line), not drawn and not taken away unseen; the readout says why it is not drawn.
printf 'sieve-personal-world-v2\nseed 0\nplace model:0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef 0 30 -55 0 0 0 fixed\nplace pages 0 30 -55 0 0 0 fixed\n' > pw/m.world
$H --personal-world --world-file pw/m.world --press "Backspace" --screenshot pworld-model.png | tr -d '\r' > pw/m.txt
q "; 1 objects placed; model:0123456789abcdef" pw/m.txt
q "^models: 1 in the catalogue, 0 drawn; 0123456789ab not drawn: not in the store$" pw/m.txt
grep -c "^place model:0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef 0 30 -55 0 0 0 fixed$" pw/m.world | q "^1$"
grep -c "^model 0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef$" pw/m.world | q "^1$"
# Your own objects (IDEAS 19.187's next step): a model brought in with --pworld-import (the catalogue's
# I), kept as objects/SHA.sieve beside the world's folder, listed after the characters, placed, picked
# up, turned and placed again as model:SHA, and drawn from its own mesh. The same file brought in again
# is the same model; a file with no faces (a glTF of a skeleton alone) is refused.
mkdir -p pw/own/worlds
printf 'mtllib cube.mtl\nv 0 0 0\nv 1 0 0\nv 1 1 0\nv 0 1 0\nv 0 0 1\nv 1 0 1\nv 1 1 1\nv 0 1 1\nusemtl red\nf 1 4 3 2\nf 5 6 7 8\nf 1 2 6 5\nf 2 3 7 6\nf 3 4 8 7\nf 4 1 5 8\n' > pw/cube.obj
printf 'newmtl red\nKd 0.8 0.1 0.1\n' > pw/cube.mtl
$H --personal-world --real-graphics --world-file pw/own/worlds/p.world --pworld-import pw/cube.obj --press "Tab,Return" --screenshot pworld-own.png | tr -d '\r' > pw/own.txt
q "^models: 1 in the catalogue, 1 drawn; [0-9a-f]* cube.obj, 12 triangles$" pw/own.txt
q "^note: Placed cube.obj (your own model, 12 triangles); " pw/own.txt
sha=$(sha256sum pw/cube.obj | cut -c1-64)
test -s "pw/own/objects/$sha.sieve"
grep -c "^model $sha$" pw/own/worlds/p.world | q "^1$"
grep -c "^place model:$sha " pw/own/worlds/p.world | q "^1$"
$H --personal-world --world-file pw/own/worlds/p.world --pworld-look -30 --press "e,r,t,Return" --screenshot pworld-own2.png | tr -d '\r' | q "; model:$sha placed at "
grep -c "^place model:$sha [-0-9.]* [-0-9.]* [-0-9.]* 3.40339208 0.261799395 0 fixed$" pw/own/worlds/p.world | q "^1$"
$H --personal-world --world-file pw/own/worlds/p.world --pworld-import pw/cube.obj --screenshot pworld-own3.png | tr -d '\r' | q "^note: cube.obj is in the catalogue already "
$H --personal-world --world-file pw/own/worlds/p.world --pworld-import tests/arm.gltf --screenshot pworld-own4.png | tr -d '\r' | q "^note: arm.gltf was not brought in: no faces\.$"
$H --personal-world --world-file pw/own/worlds/p.world --pworld-look -30 --press "Backspace" --screenshot pworld-own5.png | tr -d '\r' | q "^note: Took away cube.obj (your own model, 12 triangles)\.$"
grep -c "^model $sha$" pw/own/worlds/p.world | q "^1$"
{ grep -c "^place model:" pw/own/worlds/p.world || true; } | q "^0$"
# Your world on the worlds line (IDEAS 19.187): N opens the panel of its address there, worked out as the
# importers work theirs out. An empty world is the worlds line's world 0 (every slot model 0 in cell 0,
# unturned); nothing is said until N is pressed. Five objects: the first four carried, each as its box in
# the cell its middle is in and turned by the nearest of the 24 (the pitch of 0.3 radians is 17.2 degrees
# off), the fifth left out; the address is a world of the worlds line, its slots the same address again
# through sieve world --compose. Return there and --pworld-follow: the hallway on the worlds line, at it.
printf 'sieve-personal-world-v1\nseed 0\n' > pw/wl0.world
$H --personal-world --world-file pw/wl0.world --no-character --character-store pw/nostore --screenshot pworld-worlds-none.png | tr -d '\r' | { grep -c "^worlds:" || true; } | q "^0$"
$H --personal-world --world-file pw/wl0.world --no-character --character-store pw/nostore --press "n" --screenshot pworld-worlds0.png | tr -d '\r' | q "^worlds: positional 0*; scrambled [0-9a-f]*; carried 0 of 0 objects in 4 slots "
printf 'sieve-personal-world-v2\nseed 0\nplace pages 0 22.66 -58 0 0 0 fixed\nplace audio 2 22.66 -58 0 0.3 0 fixed\nplace books 4 22.66 -58 1.5707963 0 0 fixed\nplace binary 6 22.66 -58 0 0 0 fixed\nplace video 8 22.66 -58 0 0 0 physics\n' > pw/wl.world
$H --personal-world --world-file pw/wl.world --no-character --character-store pw/nostore --press "n" --screenshot pworld-worlds.png | tr -d '\r' > pw/wl.txt
q "; carried 4 of 5 objects in 4 slots (0 favourites, 0 of your own, 0 dropped), each as its box; grid 8, a cell 0\.750 m, " pw/wl.txt
q "; turns at most 17\.2 degrees off; left out: 0 characters, 1 objects past the slots; slot 1 cell 0\.4\.4 turn 22; slot 2 cell 2\.3\.4 turn 22; slot 3 cell 5\.4\.4 turn 0; slot 4 cell 7\.4\.3 turn 22$" pw/wl.txt
wl=$(grep "^worlds: " pw/wl.txt | sed 's/^worlds: positional \([0-9a-f]*\);.*/\1/')
$exe world --read "$wl" | grep -c "^v " | q "^32$"
slots=$($exe world --read "$wl" | tr -d '\r' | grep "^slots " | sed 's/^slots *//')
$exe world --compose "$slots" | tr -d '\r' | q "^  positional $wl$"
$H --personal-world --world-file pw/wl.world --no-character --character-store pw/nostore --press "n,Return" --pworld-follow pworld-worlds-hall.png --screenshot pworld-worlds-go.png | tr -d '\r' > pw/wlgo.txt
q "^note: To the worlds line, in the hallway, at this world\.$" pw/wlgo.txt
q "; go to it chosen$" pw/wlgo.txt
q "^hallway: WORLDS line, positional, .*went to $wl" pw/wlgo.txt
q "^saved pworld-worlds-hall.png$" pw/wlgo.txt
# O on what does not open says why: a character is not an item of a line, and a favourite no longer
# kept (here one never kept) has nothing to open.
printf 'sieve-personal-world-v1\nseed 0\n' > pw/co.world
$H --personal-world --world-file pw/co.world --character c99.character --character-store pw/nostore --pworld-walk 1.0 --pworld-look -12 --press "o" --screenshot pworld-char-o.png | tr -d '\r' | q "^note: your character does not open: a character is not an item of a line\.$"
printf 'sieve-personal-world-v2\nseed 0\nplace fav:fedcba9876543210fedcba9876543210fedcba9876543210fedcba9876543210 0 22.66 -58.5 0 0 0 fixed\n' > pw/fg.world
$H --personal-world --world-file pw/fg.world --no-character --character-store pw/nostore --pworld-look -45 --press "o" --screenshot pworld-fav-gone.png | tr -d '\r' | q "^note: a favourite no longer kept (fedcba987654\.\.\.) has nothing to open: "
printf 'sieve-personal-world-v1\nseed 0\nplace nothing-such 0 0 0\nplace pages 1 2 3\n' > pw/bad.world
$H --personal-world --world-file pw/bad.world --screenshot pworld4.png | tr -d '\r' | q "; 1 objects placed$"
# Music for your dimension (IDEAS 19.220): the Media Player's third space, PERSONAL (Right on its Mode
# row from WORLD), kept in the dimension's own world file. Until it has music of its own it plays WORLD's
# and the file is left as it is; Music here gives it its own (WORLD's settings in the mode of the line
# you are on), written as a `music` line and read back by the world; changed there, written again; given
# again, taken out, and the v1 world is v1 once more, byte for byte.
mkdir -p pw/pm/worlds
printf 'sieve-personal-world-v1\nseed 0\nplace pages 1 2 3\n' > pw/pm/worlds/personal.world
$H --no-menu --media-player --settings pw/pm/hallway.ini --press "Escape,Item:media,Return,Right" --screenshot media-personal.png | tr -d '\r' | q "^media: PERSONAL (worlds/personal.world): none of its own (WORLD's music)$"
head -1 pw/pm/worlds/personal.world | tr -d '\r' | q "^sieve-personal-world-v1$"
$H --no-menu --media-player --settings pw/pm/hallway.ini --press "Escape,Item:media,Return,Right,Down,Right" --screenshot media-personal2.png | tr -d '\r' | q "^media: PERSONAL (worlds/personal.world): its own: on, volume 30%, tempo 72 bpm, soft, mode ionian, [0-9]* filters; written into worlds/personal.world$"
head -3 pw/pm/worlds/personal.world | tr -d '\r' | tr '\n' '|' | q "^sieve-personal-world-v2|seed 0|music on=on volume=30 length=32 source=audio units=4 notes=notes104 tempo=72 voice=soft echo=35 gap=8 character=ionian filters="
$H --personal-world --settings pw/pm/hallway.ini --screenshot pworld-music.png | tr -d '\r' | q "^music: its own: on, volume 30%, tempo 72 bpm, soft, mode ionian, "
$H --no-menu --media-player --settings pw/pm/hallway.ini --press "Escape,Item:media,Return,Right,Down,Down,Down,Down,Down,Down,Down,Down,Down,Down,Down,Down,Right,Right,Down,Down,Down,Down,Right,Right" --screenshot media-personal3.png | tr -d '\r' | q "^media: PERSONAL (worlds/personal.world): its own: on, volume 30%, tempo 76 bpm, soft, mode dorian, "
$H --personal-world --settings pw/pm/hallway.ini --screenshot pworld-music2.png | tr -d '\r' | q "^music: its own: on, volume 30%, tempo 76 bpm, soft, mode dorian, "
$H --no-menu --media-player --settings pw/pm/hallway.ini --press "Escape,Item:media,Return,Right,Down,Right" --screenshot media-personal4.png | tr -d '\r' | q "^media: PERSONAL (worlds/personal.world): none of its own (WORLD's music); taken out of worlds/personal.world: "
tr -d '\r' < pw/pm/worlds/personal.world | tr '\n' '|' | q "^sieve-personal-world-v1|seed 0|place pages 1 2 3|$"
$H --personal-world --settings pw/pm/hallway.ini --screenshot pworld-music3.png | tr -d '\r' | { grep -c "^music:" || true; } | q "^0$"
# A `music` line written by hand: a kept melody played every time, a key this build does not know kept
# and written back; a second line, and a tempo past the Media Player's bounds, left out and said.
printf 'sieve-personal-world-v2\nseed 0\nmusic tempo=90 voice=sine character=dorian melody=notes104:0:5,9,13,17 later=1\nmusic volume=10\nmusic tempo=900\n' > pw/mu.world
$H --personal-world --world-file pw/mu.world --screenshot pworld-music4.png | tr -d '\r' | q "^music: its own: on, volume 30%, tempo 90 bpm, sine, mode dorian, kept melody of 4 notes$"
$H --personal-world --world-file pw/mu.world --screenshot pworld-music5.png | tr -d '\r' | q "^note: Lines of the world file not as written, left out: 2\.$"
$H --personal-world --world-file pw/mu.world --pworld-look -45 --press "Tab,Return" --screenshot pworld-music6.png | tr -d '\r' | q "; 1 objects placed$"
grep -c "^music tempo=90 voice=sine character=dorian melody=notes104:0:5,9,13,17 later=1$" pw/mu.world | q "^1$"
out=$($H --warp "$W" --press "X,Home,Up,Return" --screenshot nav.png)
echo "$out" | tail -2 | sed -n 1p | q "went to 1"
if [ "$(echo "$out" | tail -2 | book)" = "$here" ]; then exit 1; fi
# The navigator's bearing field: Tab, a bearing, ENTER, and you stand at that angle round
# the loop (180 degrees is exactly halfway); a bearing of 360 or more is refused.
$H --warp "$W" --press "X,Tab,1,8,0,Return" --screenshot navangle.png | tail -2 | sed -n 1p | q "50.0000000000% along"
navangle2=$($H --warp "$W" --press "X,Tab,3,6,0,Return" --screenshot navangle2.png | tail -2 | sed -n 1p)
if grep -q "went to" <<< "$navangle2"; then exit 1; fi
start=$($H --mode guided --warp "$W" --zoom 20 --walk "0,0" --screenshot start.png | book)
# The corridor end to end and back: ten doors left from pages reach binary (books,
# audio, tracks, video, movies, models, worlds, AI, fractals), which is where the run finishes,
# and ten right brings you back to the same book on pages.
out=$($H --mode guided --warp "$W" --zoom 20 --pose 0,7,-90,0 --walk "-2.3,0;-4.2,0;-4.2,0;-4.2,0;-4.2,0;-4.2,0;-4.2,0;-4.2,0;-4.2,0;-4.2,0;4.2,0;4.2,0;4.2,0;4.2,0;4.2,0;4.2,0;4.2,0;4.2,0;4.2,0;4.2,0" --screenshot guided.png)
echo "$out"
echo "$out" | q "BOOKS line"
echo "$out" | q "TRACKS line"
echo "$out" | q "VIDEO line"
echo "$out" | q "MOVIES line"
echo "$out" | q "MODELS line"
echo "$out" | q "WORLDS line, positional"
echo "$out" | q "AI line, positional"
echo "$out" | sed -n 9p | q "^FRACTALS line, positional"
echo "$out" | q "BINARY line"
echo "$out" | sed -n 10p | q "^BINARY line"
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
test -s menu.png
test -s menu2.png
test -s menu3.png
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
$H --line image --edge-glow --walk "0,0" --screenshot glow.png 2>&1 | q "graphics: edge glow on, wireframe on"
# Wireframe off (--real-graphics, its older name): a line loads its own item model where there is one (data/meshes/book-audio.obj
# here) and the templates for the rest.
out=$($H --line audio --real-graphics --walk "0,0" --screenshot real.png 2>&1)
echo "$out" | q "models for the audio line: hallway=hallway.obj bookshelf=bookshelf.obj book=book-audio.obj marker=marker.obj"
# Tracks and movies have models of their own (begun as copies of audio's and video's).
for l in tracks movies; do
  $H --line $l --real-graphics --walk "0,0" --screenshot real-$l.png 2>&1 | q "models for the $l line: .* book=book-$l.obj"
done
out=$($H --real-graphics --fps-counter --bench 5 --walk "0,0" --screenshot fps.png 2>&1)
echo "$out" | q "^bench: 5 frames"
echo "$out" | q "graphics: edge glow off, wireframe off, door portals on, fps counter on"
# Door Portals: Wireframe off brings them, and they also stand alone with the wireframe.
$H --line image --door-portals --walk "0,0" --screenshot portal.png 2>&1 | q "door portals on"
$H --line image --edge-glow --walk "0,0" --screenshot noportal.png 2>&1 | q "door portals off"
test -s portal.png
test -s noportal.png
$H --main-menu --press "Item:settings,Return,Return,Return" --screenshot main-res.png | q "saved"
# Settings, Graphics, then VSync off, Geometry Edge Glow on, Wireframe off — which
# turns the glow back off, the glow being drawn on the wireframe only. From a settings file
# that does not exist yet, so the toggles start from their defaults whatever ran before.
# Item:real_graphics, the row's name before (IDEAS 19.223), still reaches the Wireframe row.
rm -f app.ini
$H --main-menu --settings app.ini --press "Item:settings,Return,Return,Item:vsync,Return,Item:edge_glow,Return,Item:real_graphics,Return" --screenshot main-gfx.png > /dev/null
grep -q "^vsync = off" app.ini
grep -q "^edge_glow = off" app.ini
grep -q "^wireframe = off" app.ini
test "$(grep -c '^real_graphics' app.ini || true)" = 0
grep -q "^door_portals = on" app.ini   # Wireframe off brings Door Portals with it
# Wireframe (IDEAS 19.223): on by default; the row by its own name turns it off, and the glow
# turned on after it turns it back on. A settings file with the old real_graphics is read the
# other way round, and --wireframe on|off wins over the file for the run, --real-graphics being
# --wireframe off.
$H --write-settings wf-default.ini | q "wrote"
grep -q "^wireframe = on " wf-default.ini
rm -f wf.ini
$H --main-menu --settings wf.ini --press "Item:settings,Return,Item:graphics,Return,Item:wireframe,Return" --screenshot main-wf.png > /dev/null
grep -q "^wireframe = off " wf.ini
$H --main-menu --settings wf.ini --press "Item:settings,Return,Item:graphics,Return,Item:edge_glow,Return" --screenshot main-wf2.png > /dev/null
grep -q "^wireframe = on " wf.ini
grep -q "^edge_glow = on " wf.ini
printf '[graphics]\nreal_graphics = on\n' > wf-old.ini
$H --settings wf-old.ini --write-settings wf-new.ini | q "wrote"
grep -q "^wireframe = off " wf-new.ini
$H --line audio --settings wf-old.ini --walk "0,0" --screenshot wf-old.png 2>&1 | q "graphics: edge glow off, wireframe off, door portals off"
$H --line audio --settings wf-old.ini --wireframe on --walk "0,0" --screenshot wf-on.png 2>&1 | q "graphics: edge glow off, wireframe on, door portals off"
$H --line audio --wireframe off --walk "0,0" --screenshot wf-off.png 2>&1 | q "graphics: edge glow off, wireframe off, door portals on"
if $H --wireframe maybe --screenshot wf-bad.png > wf-bad.txt 2>&1; then exit 1; fi
q "^error: --wireframe expects on or off$" wf-bad.txt
# View Distance and Picture Distance, the two rows after the FPS counter, save.
rm -f vr.ini
$H --main-menu --settings vr.ini --press "Item:settings,Return,Return,Item:view_rooms,Right,Item:picture_rooms,Right,Right" --screenshot main-view.png > /dev/null
grep -q "^view_rooms = 8" vr.ini
grep -q "^picture_rooms = 3" vr.ini
mkdir -p lt/data/lang
cp data/lang/en.txt lt/data/lang/
printf 'language.name = Test\nitem.start = START TEST\n' > lt/data/lang/tt.txt
(cd lt && ../$H --main-menu --language tt --screenshot ../main-tt.png) | q "main menu: language tt, font sieve8x8"
# The language's font: sieve8x8 draws accented and Greek letters; a missing font is reported.
mkdir -p lt/data/fonts
cp data/fonts/sieve8x8.hex lt/data/fonts/
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
# Every footer of keys wraps upwards through the one footer (IDEAS 19.224, client/footer.hpp): at an
# 800 by 600 window --footers says each footer drawn ("footer NAME: N lines, widest W of window X (room
# R)"), and no line of any of them is wider than its room, nor its room than its window, on every screen
# --press reaches. fits FILE: there is at least one footer, and every one fits.
fits() { tr -d '\r' < "$1" | awk '/^footer / { n++; sub(/\)$/, "", $11); if ($6 + 0 > $11 + 0 || $11 + 0 > $9 + 0) bad++ } END { exit (n == 0 || bad > 0) }'; }
$H --size 800x600 --no-menu --footers --screenshot fw-hallway.png > fw-hallway.txt
fits fw-hallway.txt
q "^footer hallway: 2 lines, widest [0-9]* of window 800 (room 776)$" fw-hallway.txt
$H --size 800x600 --no-menu --footers --press "E" --screenshot fw-hand.png > fw-hand.txt
fits fw-hand.txt
q "^footer hand: 2 lines, " fw-hand.txt
$H --size 800x600 --no-menu --footers --press "E,Z" --screenshot fw-viewer.png > fw-viewer.txt
fits fw-viewer.txt
q "^footer viewer: 2 lines, " fw-viewer.txt
$H --size 800x600 --no-menu --footers --line binary --walk "0,0" --take --route-places 640 --screenshot fw-cost.png > fw-cost.txt
fits fw-cost.txt
q "^footer cost: 2 lines, " fw-cost.txt
$H --size 800x600 --no-menu --footers --line ai --walk "0,0" --take --chat --training-dir data/training --screenshot fw-chat.png > fw-chat.txt
fits fw-chat.txt
q "^footer chat: " fw-chat.txt
$H --size 800x600 --no-menu --footers --line ai --walk "0,0" --take --chain --training-dir data/training --screenshot fw-chain.png > fw-chain.txt
fits fw-chain.txt
q "^footer chain: " fw-chain.txt
$H --size 800x600 --no-menu --footers --press "Escape" --screenshot fw-pause.png > fw-pause.txt
fits fw-pause.txt
q "^footer pause: 1 line, " fw-pause.txt
$H --size 800x600 --no-menu --footers --press "X" --screenshot fw-nav.png > fw-nav.txt
fits fw-nav.txt
q "^footer navigator: [3-9] lines, " fw-nav.txt
$H --size 800x600 --no-menu --footers --map tests/manifest_fixture --graph --screenshot fw-graph.png > fw-graph.txt
fits fw-graph.txt
q "^footer graph: 2 lines, " fw-graph.txt
$H --size 800x600 --main-menu --footers --screenshot fw-menu.png > fw-menu.txt
fits fw-menu.txt
q "^footer menu: 1 line, " fw-menu.txt
$H --size 800x600 --main-menu --footers --press "Item:settings,Return" --screenshot fw-settings.png > fw-settings.txt
fits fw-settings.txt
q "^footer menu: " fw-settings.txt
$H --size 800x600 --menu --footers --screenshot fw-setup.png > fw-setup.txt
fits fw-setup.txt
q "^footer setup: 2 lines, " fw-setup.txt
$H --size 800x600 --menu --footers --press "\`" --screenshot fw-character.png > fw-character.txt
fits fw-character.txt
q "^footer character: 2 lines, " fw-character.txt
$H --size 800x600 --menu --footers --creator --screenshot fw-creator.png > fw-creator.txt
fits fw-creator.txt
q "^footer creator: " fw-creator.txt
$H --size 800x600 --menu --footers --system-viewer information --screenshot fw-sysviewer.png > fw-sysviewer.txt
fits fw-sysviewer.txt
q "^footer system_viewer: " fw-sysviewer.txt
$H --size 800x600 --no-menu --footers --media-player --press "Escape,Item:media,Return" --screenshot fw-media.png > fw-media.txt
fits fw-media.txt
q "^footer media: 2 lines, " fw-media.txt
$H --size 800x600 --no-menu --footers --rings 48,60,64,67,72 --screenshot fw-rings.png > fw-rings.txt
fits fw-rings.txt
q "^footer rings: " fw-rings.txt
$H --size 800x600 --no-menu --footers --locate tests/manifest_fixture --screenshot fw-locator.png > fw-locator.txt
fits fw-locator.txt
q "^footer locator: [2-9] lines, " fw-locator.txt
$H --size 800x600 --footers --harness --training-dir data/training --screenshot fw-harness.png > fw-harness.txt
fits fw-harness.txt
q "^footer harness: " fw-harness.txt
$H --size 800x600 --footers --designer --screenshot fw-designer.png > fw-designer.txt
fits fw-designer.txt
q "^footer designer: " fw-designer.txt
$H --size 800x600 --footers --cipher --screenshot fw-cipher.png > fw-cipher.txt
fits fw-cipher.txt
q "^footer cipher: " fw-cipher.txt
$H --size 800x600 --footers --manual-example --manual-seed 7 --screenshot fw-mnav.png > fw-mnav.txt
fits fw-mnav.txt
q "^footer manual_nav: " fw-mnav.txt
$H --size 800x600 --footers --docs welcome --screenshot fw-docs.png > fw-docs.txt
fits fw-docs.txt
q "^footer docs: " fw-docs.txt
$H --size 800x600 --footers --personal-world --screenshot fw-personal.png > fw-personal.txt
fits fw-personal.txt
q "^footer personal: [2-9] lines, " fw-personal.txt
$H --size 800x600 --footers --personal-world --press "Escape" --screenshot fw-ppause.png > fw-ppause.txt
fits fw-ppause.txt
q "^footer pause.personal: " fw-ppause.txt
# ...and without --footers the readout says none, as it always read.
$H --size 800x600 --personal-world --screenshot fw-quiet.png > fw-quiet.txt
test "$(grep -c '^footer ' fw-quiet.txt || true)" = 0
# At 1280 by 720 your dimension's footer, wider than the window, takes two lines; the main menu's,
# which fits, stays on its one as written.
$H --footers --personal-world --screenshot fw-personal-wide.png | tr -d '\r' | q "^footer personal: 2 lines, widest [0-9]* of window 1280 (room 1240)$"
$H --footers --main-menu --screenshot fw-menu-wide.png | tr -d '\r' | q "^footer menu: 1 line, widest [0-9]* of window 1280 (room 1240)$"
test -s compact.png
test -s mixed.png
test -s mark.png
test -s overlay.png
