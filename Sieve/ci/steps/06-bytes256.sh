#!/usr/bin/env bash
# Sieve CI, step 06: A file is one unit of the bytes256 line, and its address is its own bytes.
# Run by .github/workflows/build.yml (on Windows, Linux and macOS), or alone or with the
# others by ci/run.sh (ci/README.md), from the Sieve folder with the Release build in build/.
set -eo pipefail
cd "$(dirname "$0")/../.."
. ci/lib.sh

trap 'echo "::error::the step stopped at its line $LINENO: $BASH_COMMAND" >&2' ERR # names the failing line
q() { grep -c "$@" > /dev/null; }
exe=build/sieve
[ -f build/Release/sieve.exe ] && exe=build/Release/sieve.exe
# bytes256 holds every byte value in byte order, so canonicalisation is the identity
# (canon-bytes-v1) and a file of N bytes is exactly one unit of a line of length N.
$exe alphabets | tr -d '\r' | q "bytes256"
python -c "open('four.bin','wb').write(bytes([0xde,0xad,0xbe,0xef]))"
out=$($exe warp --alphabet bytes256 --length 4 --file four.bin)
echo "$out"
echo "$out" | q "canon-bytes-v1"
# The positional address IS the file, read as one number.
echo "$out" | q "deadbeef"
$exe read --alphabet bytes256 --length 4 --mode positional deadbeef --out back.bin > /dev/null
cmp four.bin back.bin
# A file-sized address is far too long for a command line, so it can come from a file.
# (Python opens the file itself, so on Windows it needs the name with .exe; bash finds it alone.)
file=$exe; [ -f "$exe.exe" ] && file=$exe.exe
python -c "d=open('$file','rb').read()[:20000]; open('big.bin','wb').write(d); open('big.hex','w').write(d.hex())"
$exe read --alphabet bytes256 --length 20000 --mode positional --address-file big.hex --out back2.bin > /dev/null
cmp big.bin back2.bin
# Nothing is folded away: a file of every byte value comes back unchanged.
python -c "open('all.bin','wb').write(bytes(range(256)))"
python -c "open('all.hex','w').write(bytes(range(256)).hex())"
$exe read --alphabet bytes256 --length 256 --mode positional --address-file all.hex --out allback.bin > /dev/null
cmp all.bin allback.bin
# ...and the address it prints for that file is exactly that hex dump.
$exe warp --alphabet bytes256 --length 256 --file all.bin | tr -d '\r' | q "000102030405"
