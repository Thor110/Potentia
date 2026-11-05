# ODDITIES: figures-2x32 once not reproduced (5 November 2026)

This commit sits on top of 1c1f2da (the `result-again.txt` zip, which you haven't applied yet). All are local and not pushed.

Only `docs/ODDITIES.md` changes: a new open entry recording the event and what was ruled out.

## Ruled out

- **The Windows build:** on your machine, `sieve diffuse` reproduced the run exactly, and so did the hallway when you trained again.
- **Your copy of the run:** the `result.txt` the hallway compared with is the committed one, byte for byte.
- **Thread count, and the harness itself:** every run here reproduced, 40 of 40 on a shorter run with 5 to 29 threads.
- **Memory errors:** valgrind found none.
- **Recent changes:** nothing changed training's arithmetic since the result was recorded.
- **Moving the selection mid-training:** the harness ignores every key but Esc while a model trains. My first theory was that Up or Down could move the selection; that was wrong, and the change is not in this commit.

So it is either a race I haven't found, or the machine itself while it was busy recording.

## If it happens again

With 1c1f2da applied, the new result is kept as `result-again.txt`, and `sieve diffuse` names the line where the two results part. That file is what to send.
