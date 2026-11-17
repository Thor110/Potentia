# Sieve: the help library is copied to help-library (fixes Rebuild)

One commit on top of 406ebe2 (list W).

## Why Rebuild stopped with "remove(help): The directory is not empty"

Ninja has a built-in target called `help`, which prints the list of targets. Ninja counts a file called
`help` in the build folder as that target's output. The build copied the help library into a folder with
that same name and place. A plain build never runs Ninja's `help` target, so the copy went through. A
clean, however, deletes every target's output, and Visual Studio's Rebuild runs a clean first. The clean
tried to delete the `help` folder as if it were Ninja's file, found it full, and stopped.

I reproduced it with a minimal CMake project: copying into `help` makes the clean fail with exactly this
error, and copying into `help-library` makes the clean and the rebuild both succeed.

## The change

- **CMakeLists.txt:** copies `data/help` to `help-library` beside the programs, with a comment saying
  why.
- **tools/make_release.py:** puts `help-library` in the release.
- **The program:** looks for its help pages in this order:
  1. `data/help` (when run from the repository);
  2. `help-library` beside the program;
  3. an old `help` folder beside it, so earlier builds and releases still work;
  4. `../data/help`.
- **Checked here:** run from outside the repository, the Documentation opens from `help-library` (26
  pages); run from the repository, from `data/help`.

## Files to delete by hand

- The old `help` folder in each build folder, once: `out\build\x64-Debug\help` and
  `out\build\x64-Release\help`. After that, Rebuild works.
- Nothing in the repository.
