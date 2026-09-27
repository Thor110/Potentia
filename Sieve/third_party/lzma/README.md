# LZMA SDK: the 7z decoder

The C decoder for 7z archives from Igor Pavlov's LZMA SDK, public domain, taken from 7-Zip 26.03
(2026-09-03, github.com/ip7z/7zip, folder `C/`). `sieve-install` alone uses it, to unpack an
installer that carries one 7z archive (`client/unpack_7z.hpp`); nothing else in Sieve links it.

Only what the decoder needs is here: the archive reader (`7zArcIn.c`, `7zDec.c`, `7zStream.c`,
`7zBuf*.c`, `7zAlloc.c`, `7zCrc*.c`), the decoders (`LzmaDec.c`, `Lzma2Dec.c`, `Ppmd7*.c`), the
filters (`Bcj2.c`, `Bra*.c`, `Delta.c`), `CpuArch.c`, and the headers those include. The files are
as upstream wrote them; each says "Public domain" in its first lines. The licence note is
`third_party_licenses/lzma/LICENSE.txt`.

To update: copy the same files from a newer 7-Zip's `C/` folder, and the headers they include.
