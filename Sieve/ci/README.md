# Sieve's CI, step by step

The checks GitHub runs on every push (`.github/workflows/build.yml`, on Windows, Linux and macOS)
are one script each here, so any of them can be run alone, as a change needs it, and all of them
together when everything is to be checked at once (Edward, 24 October 2026).

| Step | What it checks | Run when this changes |
| :--- | :--- | :--- |
| `01-unit-tests` | the unit and conformance tests (`ctest`) | anything in `core/`, `tools/`, `client/` |
| `02-same-addresses` | the same addresses on every platform | the lines, canonical forms, addresses |
| `03-filters` | filters judge, count and rank the same | `core/src/filters`, filter settings |
| `04-models` | the models line: meshes, items, with the hallway's screens of them and the oracle | `core/src/model*`, meshes |
| `05-biguint-portable` | BigUint without a 128-bit integer (Linux) | `core/src/biguint.cpp` |
| `06-bytes256` | a file as one unit of the bytes256 line | the binary line |
| `07-sound` | sound in and out as the same WAV | `core/src/sound*`, audio |
| `08-ffmpeg` | other picture and video formats through ffmpeg | pictures, video |
| `09-books` | books bind, unbind and keep their id | books |
| `10-hallway` | the hallway, its menus and the File Locator, `sieve locate` and installers, the Filter Designer (Linux) | `client/`, `tools/cli/locate*`, `pack*` |
| `11-oracle` | the oracle reproduces every committed vector, and the plugins, maps, the web host, `sieve site` and the crawler checked against it | `reference/`, `tests/`, `data/filters`, `tools/cli/map*`, `site*`, `web/` |
| `12-pinned-model` | the pinned model rebuilds from its corpus (Linux; downloads it) | the language model |

## Running them

```sh
ci/run.sh                     # every step (the Linux-only ones only on Linux)
ci/run.sh 03 10               # steps by number
ci/run.sh web oracle          # or by a word of their name
ci/run.sh --list              # the steps
ci/run.sh --build out/build/linux-release 01
```

On Windows: `ci\run.bat` (double-click it, or give it the same options), which runs `run.sh` with
the bash that comes with Git for Windows, and the Release build in `out\build\x64-Release`.
Each step that runs on Windows also has its own `.bat` beside it in `steps\` (`01-unit-tests.bat`
and so on): double-click one to run that step alone, as `ci\run.bat 01` does. Double-clicking
a step's `.sh` does not work: a step expects the fresh copy `run.sh` makes, with the build in
`build\`. A new step that runs on Windows gets a `.bat` of the same shape.

The steps write files where they run, so `run.sh` makes a fresh copy of the Sieve folder (less its
build folders) for each run, copies the build's programs and data folders in as `build/`, and runs
the steps asked for there, one after another. Each step's output goes to a log; a step that fails
shows its last lines and where its log is, and the copy is kept to look at (`--keep` keeps it
anyway). Python is `python` where that is Python 3, else the `py` launcher (`ci/lib.sh`).

Each step's commands also go to a `.trace` beside its log, each as it starts and after the time it
started (in bash 5). `python ci/trace_times.py LOGS/11-oracle.trace ci/steps/11-oracle.sh` adds a
step's time up by block (a block runs from one comment after a command to the next), the slowest
first.

Each step can run on its own: none needs a file an earlier one made. GitHub runs them in order
in one checkout, as before; the workflow only calls the scripts.
