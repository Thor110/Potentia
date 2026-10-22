# LZMA SDK: the 7z decoder, and the LZMA2 encoder

The C code for 7z archives and LZMA2 from Igor Pavlov's LZMA SDK, public domain, taken from 7-Zip
26.03 (2026-09-03, github.com/ip7z/7zip, folder `C/`).

The decoders: `sieve-install` uses them to unpack an installer that carries one 7z archive
(`client/unpack_7z.hpp`), and every program that installs Sieve instructions unpacks their LZMA2
and x86 streams with them (`tools/cli/locate.cpp`). They are the archive reader (`7zArcIn.c`,
`7zDec.c`, `7zStream.c`, `7zBuf*.c`, `7zAlloc.c`, `7zCrc*.c`), the decoders (`LzmaDec.c`,
`Lzma2Dec.c`, `Ppmd7*.c`), the filters (`Bcj2.c`, `Bra*.c`, `Delta.c`), `CpuArch.c`, and the
headers those include (library `sieve_lzma`).

The encoder (22 October 2026): `LzmaEnc.c`, `Lzma2Enc.c`, `LzFind.c`, `LzFindOpt.c` and their
headers with `LzHash.h`, built single-threaded (`Z7_ST`, so `MtCoder` and `LzFindMt` are not
needed) as a library of its own, `sieve_lzma_enc`, which only the packer links
(`tools/cli/pack.cpp`: one more candidate for each stream beside liblzma's).

The files are as upstream wrote them; each says "Public domain" in its first lines. The licence
note is `third_party_licenses/lzma/LICENSE.txt`.

To update: copy the same files from a newer 7-Zip's `C/` folder, and the headers they include.
