#!/usr/bin/env bash
# Sieve CI, step 08: Other picture and video formats come in through ffmpeg.
# Run by .github/workflows/build.yml (on Windows, Linux and macOS), or alone or with the
# others by ci/run.sh (ci/README.md), from the Sieve folder with the Release build in build/.
set -eo pipefail
cd "$(dirname "$0")/../.."
. ci/lib.sh

trap 'echo "::error::the step stopped at its line $LINENO: $BASH_COMMAND" >&2' ERR # names the failing line
q() { grep -c "$@" > /dev/null; }
exe=build/sieve
[ -f build/Release/sieve.exe ] && exe=build/Release/sieve.exe
# Without an ffmpeg, a format stb_image does not read says where to give one.
printf '\x1aE\xdf\xa3 not really a video' > fake.webm
if out=$($exe warp --ffmpeg no-such-ffmpeg --line video --file fake.webm 2>&1); then exit 1; fi
echo "$out" | tr -d '\r' | q "there is no ffmpeg at"
if [ "$RUNNER_OS" = "Linux" ] && ! command -v ffmpeg > /dev/null; then sudo apt-get update && sudo apt-get install -y ffmpeg || true; fi
if ! command -v ffmpeg > /dev/null; then echo "::warning::step 08: no ffmpeg on this runner, so its videos and other formats were not checked here (only where there is one)"; exit 0; fi
ffmpeg -hide_banner -version | head -1
# A lossless video's frame lands exactly where the same frame as a PNG (read by stb_image) does.
ffmpeg -v error -y -f lavfi -i testsrc=size=16x12:rate=10 -frames:v 1 -pix_fmt rgb24 f.png
ffmpeg -v error -y -i f.png -c:v ffv1 -pix_fmt bgr0 f.mkv
a=$($exe warp --line image --width 16 --height 12 --palette rgb24 --mode positional --file f.png | tr -d '\r' | awk '$1 == "positional" {print $2}')
b=$($exe warp --line image --width 16 --height 12 --palette rgb24 --mode positional --file f.mkv | tr -d '\r' | awk '$1 == "positional" {print $2}')
test -n "$a"
test "$a" = "$b"
# A video in a folder whose name needs quoting: every stored frame, in order, up to --frames.
mkdir -p "it's a folder"
ffmpeg -v error -y -f lavfi -i testsrc=size=64x48:rate=10 -frames:v 12 -c:v ffv1 "it's a folder/clip.mkv"
$exe warp --line video --frames 16 --file "it's a folder/clip.mkv" | tr -d '\r' | q "12 frame(s)"
$exe warp --line video --frames 8 --file "it's a folder/clip.mkv" | tr -d '\r' | q "stopped after 8 frame(s)"
# The same file gives the same address every time.
c=$($exe warp --line video --frames 8 --mode positional --file "it's a folder/clip.mkv" | tr -d '\r' | awk '$1 == "positional" {print $2}')
d=$($exe warp --line video --frames 8 --mode positional --file "it's a folder/clip.mkv" | tr -d '\r' | awk '$1 == "positional" {print $2}')
test -n "$c"
test "$c" = "$d"
# Sound in any format ffmpeg reads: FLAC lands where the same sound as WAV does.
ffmpeg -v error -y -f lavfi -i "sine=frequency=440:sample_rate=16000:duration=0.2" -c:a pcm_s16le s.wav
ffmpeg -v error -y -i s.wav s.flac
e=$($exe warp --line audio --note-set pcm --mode positional --file s.wav | tr -d '\r' | awk '$1 == "positional" {print $2}')
f=$($exe warp --line audio --note-set pcm --mode positional --file s.flac | tr -d '\r' | awk '$1 == "positional" {print $2}')
test -n "$e"
test "$e" = "$f"
# And saved through ffmpeg: a lossless format comes back to the same address.
echo "$e" > e.hex
$exe read --line audio --note-set pcm --mode positional --address-file e.hex --out back.flac > /dev/null
$exe warp --line audio --note-set pcm --mode positional --file back.flac | tr -d '\r' | awk '$1 == "positional" {print $2}' | cmp - e.hex
$exe read --line video --mode positional --out v.gif 00ff00ff00ff0f00ff > /dev/null
$exe warp --line video --mode positional --file v.gif | tr -d '\r' | q "00ff00ff00ff0f00ff"
# Sound as FLAC for WavPack, TTA and ALAC (9 November 2026): each decoded by ffmpeg and installed as
# name.flac of the same samples (their MD5s as ffmpeg decodes them equal), its tags kept, where the
# FLAC is smaller; an 8-bit TTA and a damaged WavPack (a CRC wrong) left as they are; and with no
# ffmpeg, all left as they are, the listing saying why.
rm -rf ll34 ll34_inst ll34n_inst
mkdir ll34
ffmpeg -v error -y -f lavfi -i "sine=frequency=440:sample_rate=44100:duration=3" -f lavfi -i "anoisesrc=d=3:a=0.02:seed=7:r=44100" -filter_complex "[0][1]amix=inputs=2,aformat=channel_layouts=stereo" -c:a pcm_s16le ll.wav
ffmpeg -v error -y -i ll.wav -c:a wavpack ll34/a.wv
ffmpeg -v error -y -i ll.wav -metadata title="Caf$(printf '\xc3\xa9') tune" -c:a alac ll34/b.m4a
ffmpeg -v error -y -i ll.wav -c:a pcm_s24le ll24.wav
ffmpeg -v error -y -i ll24.wav -c:a alac ll34/c.m4a
ffmpeg -v error -y -i ll.wav -c:a pcm_u8 ll8.wav
ffmpeg -v error -y -i ll8.wav -c:a tta ll34/eight.tta
ffmpeg -v error -y -i ll.wav -c:a wavpack ll34/broken.wv
python - ll34/broken.wv <<'PY'
import sys
b = bytearray(open(sys.argv[1], "rb").read())
b[len(b) // 2] ^= 0x55
open(sys.argv[1], "wb").write(b)
PY
cp -r ll34 ll34_was
$exe locate ll34 --installer ll34.sieve --items --v34-draft --sound-to-flac 2>&1 | tr -d '\r' > ll34.txt
q "^as flac   3 file(s) of sound installed as .flac files of the same samples" ll34.txt
$exe install ll34.sieve --to ll34_inst > /dev/null
for f in a b c; do
    src=$(ls ll34_was/$f.*)
    test "$(ffmpeg -v error -i "$src" -map 0:a:0 -c:a pcm_s32le -f md5 -)" = "$(ffmpeg -v error -i ll34_inst/$f.flac -map 0:a:0 -c:a pcm_s32le -f md5 -)"
done
ffmpeg -hide_banner -i ll34_inst/c.flac 2>&1 | q "s32 (24 bit)"
ffmpeg -hide_banner -i ll34_inst/b.flac 2>&1 | q "Caf$(printf '\xc3\xa9') tune"
cmp ll34_was/eight.tta ll34_inst/eight.tta
cmp ll34_was/broken.wv ll34_inst/broken.wv
$exe locate ll34 --installer ll34n.sieve --items --v34-draft --sound-to-flac --ffmpeg no-such-ffmpeg 2>&1 | tr -d '\r' > ll34n.txt
q "^as flac   5 WavPack, Monkey's Audio, TTA or ALAC file(s) left as they are: Sound as FLAC reads them through ffmpeg" ll34n.txt
$exe install ll34n.sieve --to ll34n_inst > /dev/null
diff -r ll34_was ll34n_inst
