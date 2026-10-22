# Force stop, and the graph made readable (22 October 2026)

## Force stop
- **Pressing Stop again makes it Force stop (the last finished result).** The pass under way is
  abandoned instead of finished:
  - LZMA2 is now fed a MiB at a time (the same bytes come out), so it can be cut off within a MiB;
  - 7-Zip's encoder is broken off through its progress callback;
  - the file being pulled tight by the rope, or cut into pieces, is left as it is.
- **If no pass of a stream had finished yet**, the main stream is carried as it is, unpacked. The
  stream of programs (which can't be carried that way) is packed at LZMA2's quickest setting,
  which takes seconds.
- **The longest wait was the estimate.** It works out its estimate by timing the same work on 2 MiB
  of the file, and that ignored Stop. On a tar, whose kinds of bytes change often, it took over a
  minute. Now Stop ends it too, and you just get no estimate.
- **Measured:** Stop at 20 s, Force at 25 s.
  - The 40 MB tar was done at 27 s (it was 143 s with Stop alone), and saved as it is (40,000,074
    bytes).
  - The source's 7z (47.8 MB) was done at 27 s.
  - Both installed byte for byte.

## The graph
- **A step with nothing smaller showed as a sliver.** Your file is already compressed, so its line
  ran flat along the very top, under the frame's edge and the title. Lines now sit a few pixels
  inside the frame, and all the words are outside it.
- **Stop has a row of its own** while it works, under the other buttons.

## Also
- Scripted: `--shot-after S` (a screenshot while it works) and `--force-after S`. A CI check
  force-stops a run and installs what it saved.

**Checked:** unit tests, every workflow step (6 to 16) locally, and a libc++ build.

**Docs:** IDEAS §19.97, README (File Locator), HANDOFF, VOICES.

To apply: copy `.github/` and `Sieve/` over the repository's top folder.
