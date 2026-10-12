# The missing figure-v1 files

Your main has every later change, but five files from figure-v1.zip didn't make it. The
CMakeLists.txt from diffusion-v1.zip names figure.cpp, hence the two CMake errors.

- `Sieve/core/src/filters/figure.cpp` (new): the filter itself.
- `Sieve/core/src/filters/builtin.cpp`: registers it (two lines).
- `Sieve/tests/vectors_figure_v1.tsv` (new): the oracle's vectors, which `test_figure_vectors`
  reads.
- `Sieve/docs/FILTER-PLUGINS.md` and `Sieve/docs/FILTERS-CONFLICTS.md`: the filter's rows.

Checked against origin/main file by file: with these five, every file matches what was tested here
(152,668 checks, 0 failures).
