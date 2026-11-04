## Paper...Proof

Take everything that could ever exist as a file, place it on a line and then represent that line as a circle.

If you can find a filter that rotates the location of your file to an exact integer interval then you can provably reduce any file of any size to just four values.

settings
dimension
rotation
item

The scope is borderline infinite: every dimension, every filter design and every combination of them is somewhere to search, for as long as anyone cares to search.

Without filters, the rotation is the length of the file: a file's place on its line is its own bytes read as one number, so the four values together are as long as the file itself.

A filter shortens them only when the reader already has it.

A filter made for one file becomes part of that file's address, and the more filters there are to choose from, the more it costs to say which one was chosen.

That is why a perfect reduction is rare, and why finding one for a real file would mean something.

# Overhead

There is also room for further reductions when the initial overhead instructions are eventually reduced by being packed down into a bitmask or integer list.

# Sample

Included is a sample file that takes 360 files from every single dimension.

degrees.sieve              every dimension at every whole degree 0-359: 3,960 files,
                           13,321,756 bytes -> 7,686 bytes (7z of the folder: 22,961)

Note : These files are essentially just noise and finding a perfect reduction is incredibly unlikely within a reasonable amount of time.

As well as a file that is somewhat representative of the information floor where file sizes increase due to including the overhead.

welcome to the sieve.sieve the starting page (32 bytes) -> 158 bytes

The overhead also includes a file hash for verification.

A test installation of degrees.sieve on my computer installed 3960 files, every one checked against its SHA-256, decoded in 0.356s, checked and written in 1.461s.

# Specifications

The four values above are the idea at its simplest. What a `.sieve` file holds in practice, and every way a file is tried, is set out here:

- [The .sieve format, on one page](../Sieve/docs/SIEVE-FORMAT.md): the head, the listing, the streams and each file's way, as Sieve writes them today.
- [Every way Sieve tries a file](../Sieve/docs/METHODS.md): in the order it happens, and how the smallest is chosen.
- [The full specifications](../Sieve/docs/SPECIFICATIONS.md): the lines, addresses and filters, and every version of the format (§12.2).
- [Filters as data](../Sieve/docs/FILTER-PLUGINS.md): how a filter is written, counted and ranked.
- [Statistics](STATISTICS.md): every figure measured, and what carried each file.
- [The oracle](../Sieve/reference/sieve_ref.py): a second decoder in Python, written from the specifications and sharing no code with Sieve.
