# Decompression bombs guarded (6 November 2026)

One commit on top of the last zip (9de9121). Nothing needs a new manifest version.

- **RAR, cab, ISO, LHA, cpio, ar, WARC and tar** (read through libarchive): every member read so far
  is now held, together, to the memory free, as gzip, xz and the other single streams already were.
  Past it the archive is carried as it is, not opened. These had no limit before.
- **7z:** every block's declared size is checked against the memory available before anything is
  unpacked (the LZMA SDK unpacks a block whole, into one buffer of the size its header says). Where
  all the files are held in memory, their total and the largest block are checked together.
- **The limit is now checked from 16 MiB of a read** (it was 256 MiB). Many medium bombs in one
  folder, or one inside another, now meet the limit together.

Measured here:
- a 7z of 16 GiB of zeros (3.6 MB): Extract refuses it at once ("the 7z's blocks declare 32768 MiB to
  unpack into memory, more than the 15534 MiB available"); the packer carries it as it is (4.2 s,
  323 MiB at most);
- an ordinary 7z is still opened and remade as before;
- CI step 11 has a 114-byte 7z that declares 2^50 bytes.

Not tested against a real RAR, cab or LHA bomb (there is no tool to make one here). The limit there
is the same code as for the other formats. IDEAS §19.125 has the whole picture.

The roof is still measuring (at 256 MiB).
