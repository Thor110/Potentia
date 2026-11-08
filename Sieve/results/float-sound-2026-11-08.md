# Floating-point sound through FLAC (v34 draft, 8 November 2026)

Edward (8 November 2026): "work through all of the things on that list next". Second on
docs/FORMATS-PLAN.md.

**Measured first** (the plan's step 2): FLAC on floats made from 24-bit integers came 21% to 24% under
Sieve's own ways; mu-law and A-law did not pay (Sieve's mixing on the 8-bit codes beat FLAC on the
widened samples), so they are not built.

**Built.** A float WAV (format 3, or WAVE_FORMAT_EXTENSIBLE with the float GUID), an AIFF-C `fl32` or
`fl64`, an .au of encoding 6 or 7 and a CAF flagged as floating point, of 32 or 64 bits, where every
sample is exactly n / 2^23 for a whole n (as floats made from 16- or 24-bit integers are): the numbers
n coded by FLAC at 24 bits, each written back as its float by integer arithmetic alone, so the file is
given back bit for bit on any machine. By the same recipe (`flac`), its skeleton saying which kind. A
float of any other value (-0, a fraction of a step, 1 or more, not a number) leaves the whole file to
the other ways. The rule, checked here for all 16,777,216 values at 32 bits and every 97th at 64,
matches IEEE 754's.

**The files**: two recordings already used for the FLAC way (`jazz.wav`, 16-bit; `orchestra-24bit.wav`,
24-bit), each written as floats in six
layouts. Each alone, `--priority balanced --optimise-search`, v34 draft against without it (before);
every one installed byte for byte.

| File | Bytes | Before | Made in | After | Made in | |
| :--- | ---: | ---: | ---: | ---: | ---: | ---: |
| jazz, WAV, float | 16,000,058 | 6,220,817 | 116.8 s | 4,871,451 | 7.2 s | −21.7% |
| jazz, WAV, extensible float | 16,000,080 | 6,813,275 | 90.8 s | 4,871,479 | 6.5 s | −28.5% |
| jazz, WAV, double | 32,000,058 | 7,130,936 | 211.7 s | 4,871,451 | 7.4 s | −31.7% |
| jazz, AIFF-C fl32 | 16,000,084 | 6,179,328 | 109.1 s | 4,871,482 | 6.8 s | −21.2% |
| jazz, .au, float | 16,000,028 | 6,179,274 | 114.4 s | 4,871,418 | 6.6 s | −21.2% |
| jazz, CAF, float, little-endian | 16,000,068 | 6,220,828 | 113.8 s | 4,871,466 | 7.8 s | −21.7% |
| orchestra, WAV, float | 10,666,722 | 5,767,052 | 81.3 s | 4,406,047 | 5.9 s | −23.6% |
| orchestra, WAV, extensible float | 10,666,744 | 5,767,073 | 72.8 s | 4,406,071 | 5.5 s | −23.6% |
| orchestra, WAV, double | 21,333,386 | 5,870,967 | 148.4 s | 4,406,047 | 5.8 s | −25.0% |
| orchestra, AIFF-C fl32 | 10,666,748 | 6,032,417 | 67.6 s | 4,406,074 | 4.6 s | −27.0% |
| orchestra, .au, float | 10,666,692 | 6,029,515 | 69.6 s | 4,406,014 | 5.3 s | −26.9% |
| orchestra, CAF, float, little-endian | 10,666,732 | 5,767,048 | 78.2 s | 4,406,058 | 5.5 s | −23.6% |

Each recording comes to the same size in every layout (its FLAC stream is the same; only the skeleton
differs), and making is 13 to 29 times quicker, as mixing is no longer weighed on the floats' bytes.

**Checked** by CI step 11 (six layouts opened, a float with a -0 and one half a step off left alone;
Sieve, sieve-install and the oracle, which writes the floats by its own rule; Sound as FLAC).
