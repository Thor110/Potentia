#!/usr/bin/env bash
# Sieve CI, step 07: Sound itself is a set of the audio line, and comes back as the same WAV.
# Run by .github/workflows/build.yml (on Windows, Linux and macOS), or alone or with the
# others by ci/run.sh (ci/README.md), from the Sieve folder with the Release build in build/.
set -eo pipefail
cd "$(dirname "$0")/../.."
. ci/lib.sh

trap 'echo "::error::the step stopped at its line $LINENO: $BASH_COMMAND" >&2' ERR # names the failing line
q() { grep -c "$@" > /dev/null; }
exe=build/sieve
[ -f build/Release/sieve.exe ] && exe=build/Release/sieve.exe
# A quarter second of a 440 Hz tone, 16-bit stereo at 22050 a second, written here.
python -c "import math,struct; n=5512; d=b''.join(struct.pack('<hh', int(12000*math.sin(2*math.pi*440*i/22050)), int(6000*math.sin(2*math.pi*660*i/22050))) for i in range(n)); open('tone.wav','wb').write(b'RIFF'+struct.pack('<I',36+len(d))+b'WAVEfmt '+struct.pack('<IHHIIHH',16,1,2,22050,22050*4,4,16)+b'data'+struct.pack('<I',len(d))+d)"
# Mixed to one channel, resampled to 8000 a second and rounded to 8 bits: one unit of a second.
out=$($exe warp --line audio --note-set pcm --mode positional --file tone.wav)
echo "$out" | tr -d '\r' | q "canon-pcm-v1: 5512 sample(s) per channel at 22050 a second, 2 channel(s) -> pcm/8000/8/C1, 1 unit(s)"
echo "$out" | tr -d '\r' | q "mixed to one"
echo "$out" | tr -d '\r' | awk '$1 == "positional" {print $2}' > a.hex
# Read back as a WAV file, which warps to the same address.
$exe read --line audio --note-set pcm --mode positional --address-file a.hex --out back.wav > /dev/null
$exe warp --line audio --note-set pcm --mode positional --file back.wav | tr -d '\r' | awk '$1 == "positional" {print $2}' > b.hex
cmp a.hex b.hex
# Any depth and channels: 12 bits, three channels (the last repeated), at its own rate.
$exe warp --line audio --note-set pcm --rate 22050 --bits 12 --channels 3 --length 6000 --mode positional --file tone.wav | tr -d '\r' | awk '$1 == "positional" {print $2}' > c.hex
$exe read --line audio --note-set pcm --rate 22050 --bits 12 --channels 3 --length 6000 --mode positional --address-file c.hex --out back3.wav > /dev/null
$exe warp --line audio --note-set pcm --rate 22050 --bits 12 --channels 3 --length 6000 --mode positional --file back3.wav | tr -d '\r' | awk '$1 == "positional" {print $2}' | cmp - c.hex
# Notes stay notes: notes104 is still the default, and pcm options are refused there.
$exe info --line audio | tr -d '\r' | q "notes104"
if $exe info --line audio --rate 8000 > /dev/null 2>&1; then exit 1; fi
# Saving as a format that needs ffmpeg says so when there is none.
if out=$($exe read --ffmpeg no-such-ffmpeg --line audio --note-set pcm --mode positional --address-file a.hex --out x.mp3 2>&1); then exit 1; fi
echo "$out" | tr -d '\r' | q "there is no ffmpeg at"
# Its filters judge each channel; the three of them merge and count exactly at 800 samples.
printf '[audio]\nmode = mark\nfilters = sound-peak-v1, sound-step-v1, silence-run-v1\n[audio.silence-run-v1]\nsamples = 800\n' > sound.ini
$exe filters --line audio --note-set pcm --length 800 --filters sound.ini | tr -d '\r' | q "keeps 10^-102.20"
# Open-ended notes (notes3): ticks and levels, and a MIDI file that reads back as the same music.
o="--line audio --note-set notes3 --voices 2 --length 4 --instruments 0,40 --mode positional"
$exe warp $o "C#4:3!5 R:1 D4:16!8 E4:1!1 // C2:16!3 R:2 G9:1!8 C-1:3!2" | tr -d '\r' | q "C#4:3!5 R:1 D4:16!8 E4:1!1 // C2:16!3 R:2 G9:1!8 C-1:3!2"
n3=$($exe warp $o "C#4:3!5 R:1 D4:16!8 E4:1!1 // C2:16!3 R:2 G9:1!8 C-1:3!2" | tr -d '\r' | awk '$1 == "positional" {print $2}')
$exe read $o --out n3.mid "$n3" > /dev/null
test "$($exe warp $o --file n3.mid | tr -d '\r' | awk '$1 == "positional" {print $2}')" = "$n3"
printf '[audio]\nmode = mark\nfilters = sound-peak-v1\n' > peak.ini
$exe filters --line audio --note-set pcm --bits 16 --channels 2 --filters peak.ini | tr -d '\r' | q "keeps 10^-732.05"
