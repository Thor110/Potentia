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
if ! command -v ffmpeg > /dev/null; then echo "no ffmpeg on this runner: the rest is checked where there is one"; exit 0; fi
ffmpeg -hide_banner -version | head -1
# A lossless video's frame lands exactly where the same frame as a PNG (read by stb_image) does.
ffmpeg -v error -y -f lavfi -i testsrc=size=16x12:rate=10 -frames:v 1 -pix_fmt rgb24 f.png
ffmpeg -v error -y -i f.png -c:v ffv1 -pix_fmt bgr0 f.mkv
a=$($exe warp --line image --width 16 --height 12 --palette rgb24 --mode positional --file f.png | tr -d '\r' | awk '$1 == "positional" {print $2}')
b=$($exe warp --line image --width 16 --height 12 --palette rgb24 --mode positional --file f.mkv | tr -d '\r' | awk '$1 == "positional" {print $2}')
test -n "$a" && test "$a" = "$b"
# A video in a folder whose name needs quoting: every stored frame, in order, up to --frames.
mkdir -p "it's a folder"
ffmpeg -v error -y -f lavfi -i testsrc=size=64x48:rate=10 -frames:v 12 -c:v ffv1 "it's a folder/clip.mkv"
$exe warp --line video --frames 16 --file "it's a folder/clip.mkv" | tr -d '\r' | q "12 frame(s)"
$exe warp --line video --frames 8 --file "it's a folder/clip.mkv" | tr -d '\r' | q "stopped after 8 frame(s)"
# The same file gives the same address every time.
c=$($exe warp --line video --frames 8 --mode positional --file "it's a folder/clip.mkv" | tr -d '\r' | awk '$1 == "positional" {print $2}')
d=$($exe warp --line video --frames 8 --mode positional --file "it's a folder/clip.mkv" | tr -d '\r' | awk '$1 == "positional" {print $2}')
test -n "$c" && test "$c" = "$d"
# Sound in any format ffmpeg reads: FLAC lands where the same sound as WAV does.
ffmpeg -v error -y -f lavfi -i "sine=frequency=440:sample_rate=16000:duration=0.2" -c:a pcm_s16le s.wav
ffmpeg -v error -y -i s.wav s.flac
e=$($exe warp --line audio --note-set pcm --mode positional --file s.wav | tr -d '\r' | awk '$1 == "positional" {print $2}')
f=$($exe warp --line audio --note-set pcm --mode positional --file s.flac | tr -d '\r' | awk '$1 == "positional" {print $2}')
test -n "$e" && test "$e" = "$f"
# And saved through ffmpeg: a lossless format comes back to the same address.
echo "$e" > e.hex
$exe read --line audio --note-set pcm --mode positional --address-file e.hex --out back.flac > /dev/null
$exe warp --line audio --note-set pcm --mode positional --file back.flac | tr -d '\r' | awk '$1 == "positional" {print $2}' | cmp - e.hex
$exe read --line video --mode positional --out v.gif 00ff00ff00ff0f00ff > /dev/null
$exe warp --line video --mode positional --file v.gif | tr -d '\r' | q "00ff00ff00ff0f00ff"
