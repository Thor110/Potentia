# Changes: the MSVC error in packMP3 (C3861 'setmode')

One commit on top of 8f13307 (sieve-mp3.zip).

packMP3 switched stdin and stdout to binary mode (`setmode`) every time it opened a stream, for its
own command-line program. MSVC doesn't declare `setmode` by that name, and inside Sieve the call would
change how Sieve's own console output is written on Windows, while Sieve only ever gives packMP3 memory
buffers. So I took the call out instead of adding the header.

The rest of packMP3's built sources were checked for other GNU-only code (variable-length arrays,
statement expressions and the like): none found. That check is clang and GCC's, not MSVC's, so your
build is still the real test.

Checked here: the same 17 MP3s round-trip byte for byte; sieve_mp3_check gives 2,000 damaged forms,
1,997 refused, 3 decoded, 0 failures; CI step 11's MP3 block passes.

Files: Sieve/third_party/packmp3/bitops.cpp, Sieve/third_party/packmp3/SIEVE-CHANGES.md.
