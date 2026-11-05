# CI step 11: the forged-stream check (5 November 2026)

This commit sits on top of c6a7315 (the IDEAS zip). All are local and not pushed.

## Why it failed on your machine

The check builds a 30 KB file whose LZMA2 stream claims to make 6 GB, then expects the installer to refuse it with "a stream's chunks do not make the size it declares".

The installer did refuse it, but an earlier check got there first. Installing refuses any total past half the memory available, before reading a single chunk, and your machine had less than 12 GB free during the run. This machine had 15 GB available, so the chunk check was reached here. Both refusals are right; the test assumed a large machine.

## The fix

The forged file now claims 60 MB. That is still far more than its 30 KB of chunks can make, and under half the free memory on any machine that runs CI.

Checked here: refused with the chunks' message, and nothing written.

## Also in your log

`cmp: tar32p_inst/bundle.contents/GAME.md: No such file` is expected. That line tries one folder name, then the other.

## Please

Run step 11 again, since the lines after 1202 didn't run on your machine.
