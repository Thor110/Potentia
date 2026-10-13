# Metrics Over Time: what each chart means, and a line between them (13 October 2026)

## `Pages/metrics.html`
- **Every chart now says what its measure means,** under its title, in plain words:
  - **Unit test checks:** each check compares something Sieve works out with the answer it must
    give. The suite runs once with Sieve's own SHA-256 and again with the processor's, where it
    has the instructions, and both runs count. A build is logged only when every check passes.
  - **Seconds for the unit suite:** timed by make_release. Builds from run_tests.bat aren't timed,
    so this line has only releases.
  - **Checks a second:** the two above divided. Compare it only between builds of the same version
    on the same machine.
  - **Conformance vector files:** written by the Python oracle straight from the specification, and
    reproduced byte for byte by the C++ engine.
  - **Specification sections, commands, source files, lines of source, ideas, voices:** what each
    counts, and where it is counted from.
  - **What the reference filters keep, each line:** a power of ten. 0 means almost everything is
    kept; −4 means about 1 in 10,000.
- **A dividing line** sits between each chart and the next.

## Checked here
- In Chromium at phone width and desktop width: 21 charts with 20 dividers, the hover works, and
  there are no errors or sideways scroll.
