# Transformed copies: compression and encryption as filters

This notes a design conversation (Edward with Gemini, 28 September 2026) about extending the
cross-line filters to catch *transformed* copies of other lines' content: compressed, obfuscated
or encrypted. After the notes comes an assessment, and then what could be built, in tiers. Nothing
here is built yet. It belongs beside `not-written-v1` and `binary-kind-v1` (FILTER-PLUGINS.md
§15), which catch content that is only *written out* (hex, base64, bits and so on) and not
transformed.

---

## 1. The idea, as discussed

**Edward's first note.** Use every compression and encryption technique to trim more units from
the lines. There are nearly infinitely many, so the way to do it is to test them all by program.
It would be very compute-heavy. But the filters run before anything is generated, against the
generating algorithm itself, so it should be worth it.

**Gemini's first reading (a misunderstanding).** Gemini took this to mean spotting compressed
files by their signatures, which the file-kind filters already do. It raised *ciphertext
indistinguishability*: arbitrary noise is valid ciphertext, so decrypting noise with enough keys
turns up anything.

**Edward's clarification.** Not signatures. Take the filter that removes each line's content from
the other lines, and also remove every possible compressed or encrypted variant of those same
files, by program. It would "work somewhat like a scale, testing encryption from all angles".

**Gemini: two directions.**
- *Forwards* is a wall: every encryption of every file under every key (2^128 variants per file
  and per key size) cannot be precomputed.
- *Backwards* is how file carving and malware scanners work: take a candidate unit, and see
  whether a streaming decompression or decryption (zlib inflate, common ciphers with sliding keys,
  multi-byte XOR) turns it back into a known file.

**Edward: it is not about precomputing.** If `74 65 73 74` ("test") encrypts to `CF CD 92 23`,
the maths is applied walking forwards through the bytes. So it should be a matter of forbidding
the walk from going that way: we know exactly how encryption works, so if we know which way the
maths goes as we generate, we can exclude the encrypted versions. It does need the whole file,
though, so this may have to be an active, ongoing filter that works as you walk through the state
space.

**Gemini: the dividing line.**
- *Deterministic transforms without a secret* (XOR with a known key, substitution, block
  shuffles, bit permutations, compression streams) can be followed by an automaton and pruned as
  the unit is walked.
- *Modern ciphers* (AES, RSA) have huge key spaces and are built for the avalanche effect.
  Almost every byte string is a valid ciphertext under some key, so they cannot be pruned without
  a target key.

**Edward: keys from the space itself.** Modern ciphers stay an exhaustive problem that can only be
met one way. Could all keys of length X be sourced from the binary line? Every key already exists
there. Gemini called it the space as its own keyring: an Ouroboros, the space encrypting and
revealing itself.

---

## 2. Assessment

Edward's instinct is right about how to do it: follow the transform forwards as the unit is walked,
as an automaton. That is exactly how `not-written-v1` handles hex, base64 and the rest, and it is
why that filter counts exactly. Where it breaks down is the key, and the reason is arithmetic, not
compute.

**With every key, everything is a ciphertext of everything.** XOR with a key as long as the file
(a one-time pad, the simplest cipher of all) turns any `N`-byte string into any other: for a
target file `F` and any unit `C`, the key `F ⊕ C` makes `C` an encryption of `F`. So "no unit that
is an encryption of another line's content, under any key" excludes *every* unit of that length.
Drawing the keys from the binary line is the same as allowing every key. No amount of computing
power changes this. It is a property of the sets, and it is the same wall Gemini called
indistinguishability.

**With a fixed, finite family of keys, it works, and it is small.** Pick the transforms in advance
(say XOR with any one-byte key, the Caesar shifts, bit reversal, the nibble swap). Each transform
maps the excluded set onto a set of the same size, so a family of `k` transforms excludes at most
`k` times as much as the plain filter. That is the same lesson as §15 of FILTER-PLUGINS.md:
exclusion removes a sliver. XOR with every one-byte key multiplies a sliver of 10^-19 by at most
256.

**Encrypted content *is* the noise.** A good cipher's output is designed to look like uniform
random bytes, and uniform random bytes are what almost all of the space is. So the noise the
filters cannot remove is, read one way, every encrypted file there is, under every key. It cannot
be told apart from noise without the key, and that is the point of encryption. This is a fair thing
to say in the documentation, and maybe in the game (GAME.md).

**Compression sits in between.** Decompressing a unit (inflate, LZMA) and asking whether the
result is another line's content is *judging*. It is exact, and cheap per unit. Counting it is
another matter: a deflate stream carries its own code tables and a 32 KB window, so the automaton
of "decompresses to a signed file" is far too large to build, and such a filter judges only (no
compact mode). As with encryption, only the *formats* can be listed, not "every compressor": any
bijection is a compressor for something.

**"Active, ongoing, as you walk" is already how it runs.** The engine never generates and then
discards. A filter is an automaton over the unit's symbols; counting and ranking walk it; the
hallway judges each unit it shows. A transform filter is one more automaton (or, for compression,
one more judge), not a different kind of machinery.

---

## 3. What could be built, in tiers

| Tier | Transforms | Counts exactly? | Note |
| :--- | :--- | :--- | :--- |
| 1 | Keyless, streaming: bit inversion, bit reversal within bytes, nibble swap, byte reversal of the whole unit, ROT-n / Caesar on letters, Atbash | yes | each is a relabelling of the symbols or of the bytes, applied before the existing readings; the machines take it as a first step |
| 2 | Small keyed families: XOR with any 1-byte key, add-mod-256 with any 1-byte key, XOR with any 2-byte repeating key | yes, by symmetry | the key is fixed by the first byte or two of a signature, so, as with the binary reading's two symbols, the key does not need to be carried as separate states: for each signature, the key is what makes its first byte match |
| 3 | Compression formats: deflate (zlib, gzip, raw), LZMA, bzip2 | judges only | decompress, and apply the file-kind and not-another-line tests to the result, with a size cap so a decompression bomb cannot stall a frame |
| 4 | Modern ciphers under all keys | not possible | excludes everything (§2) |
| 4a | Modern ciphers under *named* keys | judges only | e.g. keys published in the anchor registry, or a user's own key; a way to recognise one's own encrypted files, not to prune the space |

Filters would be versioned like every other: `not-transformed-v1` (tiers 1 and 2), with a
`transforms` parameter (`all`, or one family), stacking on `not-written-v1` and the cross-line
filters. The oracle would have its own implementations, as for `not-written-v1`. Tier 3 would be a
separate judge-only filter (`not-compressed-v1`) whose vectors are chosen files, not counts.

Measure before building: `sieve filters` already prints what a stack sets aside as a power of ten.
Tier 1 on 32-letter pages should come out at about ten times `not-written-v1`'s 10^-4.23, since
there are a few relabellings per reading. It is worth confirming that estimate on a prototype
before committing to the tiers.

---

## 4. Open questions

- Which keyless transforms are worth naming? Every permutation of the symbols is one, and there
  are 27! of them on `lower27`. A list is a choice, so it wants to be short and well-known, and
  versioned.
- Should tier 2 key families be one filter with a parameter for the key length, given that the
  excluded share grows with each byte of key (at most 256 times per byte)?
- Tier 4a: where would named keys live, and would recognising content under a user's key belong
  to the vault's side (withholding) rather than to the filters (tidiness)?
