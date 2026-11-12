# Fix: CI step 01 (unit tests) failed under ctest (12 November 2026)

On top of a27fb97 ("sieve catalogue plans"). Unzip at the repository root; one file.

The catalogue test (tests/test_core.cpp, test_catalogue) read data/catalogue and data/languages
relative to where it ran. ctest runs the tests in the build folder, where there is no data/, so it
found neither entry folder and failed ("1 - core (Failed)"). It now finds data/ beside the tests
folder it is given, as the other tests find their files, and fails plainly if the folders are not
there. Checked: CI step 01 (ctest) passes here, 309 s.
