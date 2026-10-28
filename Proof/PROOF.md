## Paper...Proof

Take everything that could ever exist as a file, place it on a line and then represent that line as a circle.

If you can find a filter that rotates the location of your file to an exact integer interval then you can provably reduce any file of any size to just four values.

settings
dimension
rotation
item

The amount of time it might take to find that address? Irrelevant.

There is also room for further reductions when the initial overhead instructions are eventually reduced by being packed down into a bitmask or integer list.

Included is a sample file that takes 360 files from every single dimension.

degrees.sieve              every dimension at every whole degree 0-359: 3,960 files,
                           13,321,756 bytes -> 7,887 bytes (7z of the folder: 22,961)

Note : These files are essentially just noise and finding a perfect reduction is incredibly unlikely within a reasonable amount of time.

As well as a file that is somewhat representative of the information floor where file sizes increase due to including the overhead.

welcome to the sieve.sieve the starting page (32 bytes) -> 159 bytes

The overhead also includes a file hash for verification.

A test installation of degrees.sieve on my computer installed 3960 files, every one checked against its SHA-256, decoded in 0.356s, checked and written in 1.461s.