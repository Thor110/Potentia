# X4 fix: fractal formulas drawn the same on every machine

CI step 01 failed on Windows (test_core.cpp:7770): the charted fractals drawn at 10x10 counted other than
1,057 under Microsoft's C library. A v4 formula is followed thousands of times a point with the system's
complex numbers and exp, log, sin, cos, pow and atan2, whose last bit differs between libraries; a chaotic
iteration grows that bit until a point escapes on one machine and not on another.

- core/src/fractal.cpp: the formulas' arithmetic is Sieve's own: complex + - * / (Smith's division), and
  exp, log, sin, cos, sinh, cosh and atan2 built from + - * /, sqrt, floor, ldexp, frexp and fmod only,
  which IEEE 754 makes the same everywhere. Measured against GNU's library over 2,000,000 random
  arguments: within 4.9e-16 (relative) at most. The settled test compares squares (no hypot).
- CMakeLists.txt: fractal.cpp compiled with -ffp-contract=off on GCC and Clang, so no multiply and add
  are fused (MSVC's /fp:precise fuses none).
- docs/SPECIFICATIONS.md: the v4 bullet says so.

Here (GCC, Linux) every figure is as it was: 1,057 drawn; the sample's 894 / 180 / 688; the fern's 1.279;
the core tests pass (12,813,492 checks, 0 failures).
