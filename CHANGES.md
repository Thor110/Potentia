# X4 fix, second step: name the fractals whose pictures differ

CI step 01 still fails under MSVC at test_core.cpp:7770 (the charted pictures at 10x10, 1,057 here)
after the formulas' own arithmetic (95e4054). Nothing left on the drawing path reads as machine-dependent,
so the test now measures it: where the count is not 1,057 (or SIEVE_FRACTAL_DIGEST is set) it prints
the count and, for each charted fractal, one hash of its pictures at every depth, drawn and fitted as
fractal-drawn-v1 draws them. Compared with this machine's list, it names the fractals that differ.

- tests/test_core.cpp: the digest (and <cstdlib> for std::getenv).
