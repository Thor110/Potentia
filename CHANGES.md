# Picture formats list, and the brotli oddity (6 November 2026)

Commit 1861a7f (local, not pushed), on top of your 5d5d687.

## docs/FORMATS.md: "Pictures: every format, and what each part of Sieve does with it"
One table, the same way as the compressed formats. For each format it says what happens in:
- packing into a `.sieve`;
- the image line;
- the viewer and Explorer thumbnails;
- the vault.

The formats: PNG, JPEG, GIF, BMP, TGA, PNM, PSD/PIC, what ffmpeg reads (WebP, TIFF, AVIF...), and `.simage`.

It is taken from the code:
- stb_image is built with every format but HDR;
- the viewer opens six formats by name;
- mixing tries a picture shape guessed from the bytes, and takes it only where it is at least 2% smaller.

PNM on the image line was checked here.

## docs/ODDITIES.md: resolved
**Your More formats test on `oddity-click.zip`** (5,312 bytes: two text files, already deflated):

| | Bytes |
|---|---:|
| zstd -19 | 4,905 |
| brotli -q 11 | 4,915 |
| Sieve, no box | 4,984 |
| Sieve, Archive and compressed formats: temporary | **3,880** |

- **Why Sieve lost:** on data that is already compressed, every coder ties within a few dozen bytes, and the `.sieve` also carries the file's name, size and SHA-256.
- **With the box:** Sieve opens the zip, packs the text inside, and makes the zip again exactly. That is 21% under brotli, and it installs back byte for byte.
