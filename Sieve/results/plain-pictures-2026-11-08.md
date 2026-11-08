# Pictures in plain pixels: their shape from their headers (v34 draft, 8 November 2026)

Edward (8 November 2026): "work through all of those planned steps and formats". Step 4 of the plan:
pictures of plain pixels (BMP, TGA, uncompressed TIFF, PNM).

**What was there already.** A file of plain pixels is mixed as a picture where a shape guessed from
its bytes (its width and bytes a pixel) makes its first 256 KiB at least 2% smaller (`picture_shape`);
the rope searches widths from the file's repeats. Neither reads the file's header.

**Measured first.** Six pictures (three 800 by 450 photographs, the 949 by 1300 Vitruvian drawing, two
1920 by 1080 screenshots) as BMP and uncompressed TGA (24-bit), PPM and uncompressed TIFF (RGB), each
alone, `--v34-draft --optimise-search --priority size`. The guess was right for TGA, PPM and TIFF, and
wrong for one BMP: `door.bmp` mixed as 1,618 wide, 205,863 bytes; the same pixels as TGA, 800 wide,
193,538. As a folder, the PNG copies (whose recipe gives mixing their own shape) came to 2,442,839
bytes against the BMPs' 2,258,201: knowing the shape is no gain in itself where the guess is right.

**Built.** `header_shape` (tools/cli/pack.cpp) reads the shape from a BMP's, an uncompressed TGA's, a
binary PNM's or an uncompressed TIFF's header, where its rows follow one another with nothing between
(a BMP whose rows need padding to four bytes is left to the guess), and offers it to mixing beside the
guess, weighed on the file's start as a PNG's own shape is, with a second shape: the rows as pictures of
one byte a pixel (each channel's neighbour beside it). No format change: mix-v4's `I`, which every
reader since v26 reads; under the v34 draft only.

| Picture | Before | After | |
| :--- | ---: | ---: | ---: |
| bmp/12-state-space-hallway | 153,580 | 155,522 | +1.26% |
| bmp/15-state-space-ai-interaction | 106,614 | 106,448 | −0.16% |
| bmp/door | 205,863 | 189,011 | −8.19% |
| bmp/picture-in-hand | 159,645 | 153,569 | −3.81% |
| tga/door | 193,538 | 189,164 | −2.26% |
| tga/picture-in-hand | 159,308 | 153,062 | −3.92% |
| ppm/door | 193,385 | 188,033 | −2.77% |
| ppm/picture-in-hand | 159,530 | 154,591 | −3.10% |
| tif/door | 193,944 | 188,641 | −2.73% |
| tif/picture-in-hand | 160,121 | 155,199 | −3.07% |
| the other 14 | 4,920,167 | 4,920,167 | 0.00% |
| all 24 | 8,405,705 | 8,353,417 | −0.62% |

(Bytes of each `.sieve` made of the picture alone; every one installed byte for byte.)

**The one larger.** The hallway screenshot as BMP: on its start the rows' shape won by 1.1% (6,622
bytes against 6,697), and the whole came out 1.26% larger. Weighing the middle of the file as well
before taking the rows (tried) kept that screenshot as it was and lost the photographs' gains (all 24:
−0.39%), so it was not kept.

**Not built, and why.** Taking a padded BMP's padding out (rows of 949 RGB pixels padded to 2,848
bytes): the padded Vitruvian BMP, mixed as rows of 2,848 one-byte pixels, was already the smallest of
its four formats (1,262,158 against 1,355,228 to 1,359,990). Compressed TGA (RLE) and TIFF (LZW,
deflate, PackBits): carried as their bytes; made again exactly, they would need their encoders'
choices found (as the GIF route found gifenc's), and are in the plan for later formats.
