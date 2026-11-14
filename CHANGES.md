# Review 1 (performance): every proposed fix, then a second look

Every fix keeps each output the same, checked by comparing the new build's output with the build before it.

## Batch A: the hallway program
- **Your dimension.** Each object keeps its size and its faces until it moves or turns, the hill's heights are worked out once, and the crosshair's test skips what is out of reach. Measured: 200 dropped objects over 20 simulated seconds took **20.9 s, now 0.8 s**. The pictures, the positions and the saved world files were byte for byte the same (wireframe and Real Graphics, dropping, holding, characters).
- **My own bug from the last zip.** Looking up a placed character hashed every stored character each time; their SHA-256s are now worked out once.
- **Hashing every frame, now done once:**
  - the Replays screen read and hashed every recording on every frame; now each is hashed once, when it is first shown;
  - the Character Creator and the hallway hashed every animation each frame to find the one playing;
  - the CHARACTER page and the YOURS list hashed their characters each frame.
- **The Keys screen** took its whole listing two or three times a row each frame. Now it takes it once a frame, and only on the screens that show keys.
- **faces.ini** was read from disk for every favourite in your dimension, every frame. Now it is read once.
- **The viewer** kept an opened picture's pixels twice; now they are moved into the view and back.
- The menu screens (CHARACTER, YOURS, the Creator, Keys, Replays, a viewer) gave the same pictures. The only difference was a 4×1-pixel spot that the old build also changes from run to run.

## Batch B: the RAID, a model file and chains
- **The RAID's list** walked every version chain twice in every comparison of its sort. Measured at 301 versions: the list took **0.195 s, now 0.024 s**, and a put **0.372 s, now 0.047 s**. The lists were identical.
- **Sending a piece.** It went as two writes, then waited on the network's acknowledgements. Now it goes as one write, and RAID connections send at once. Measured: an 8 MiB share took **2.12 s, now 0.14 s**, with the same report. This also answers "sharing asks each peer in turn": those round trips were waiting on the same delay.
- **A .glb's data** is moved, not copied a second time.
- **A steered chain stage** makes its tries a batch at a time on the threads, then judges them in order. The first that passes is kept, as before.

## Batch C: making
- **Text extraction from HTML.** Cutting out `<script>` and `<style>` searched the rest of the page once per block. Measured on a 16 MB page: **4.59 s, now 0.14 s**. The text came out the same on that page and on 30 test pages with awkward tags.
- **The trial mixers' history buffers** come as untouched zero pages, so a trial fed only 4 MiB no longer holds a buffer the size of the whole file. A 6.7 MB text file and a 10 MB program made **the same .sieve bytes**. Peak memory at those sizes: 518 MB against 518 MB, and 831 MB against 841 MB. The saving grows with the file's size; it was not measured on a larger file.
- **Under Balanced**, a program's two mixes (with and without E1) run side by side only when memory allows a second mixer. Otherwise they run one after the other, with the same bytes.

## The second look you asked for
- **The eight findings set aside, rechecked by fresh agents against the code as it now is:**
  - two are fixed by these changes;
  - six are left:
    - the Character Creator draws a whole-window picture: about 2 ms a frame at 1080p, measured, on a menu page;
    - a recording keeps every input event: only while you record;
    - edges are walked once per band: a few hundredths of a millisecond, measured;
    - a piece kept for another machine is written while a lock is held: under a millisecond (worked out), and the suggested fix would let two writers of the same piece spoil each other's file;
    - a request is read a byte at a time: about 6.5 ms a check on one machine (estimated from a measured 17 µs a line), 1% or less of a check across a network.
    - sharing asks each peer in turn: answered by the one-write fix above.
- **Each batch of fixes reviewed by its own agent:**
  - one nit in A: the keys' listing was taken on every screen; fixed;
  - two risks in B: if no thread could be started, the program would have aborted; and a failed try later in a batch could hide an earlier try that passed. Both fixed;
  - nothing found in C.

## Optimisations still to make: a new list
- **docs/OPTIMISATIONS.md** (new): the six findings left for now, plus one measurement still to take. Each entry gives what it costs (measured or estimated, and says which), the redesign it needs, how to show a fix changes nothing, and its CI steps, largest cost first. Your words open it: "we should aim for every possible optimisation within reason, any gain is a gain in the potential for the state space and tools itself".
- In REVIEW-ROUND.md, section 3 has a list O for them, so they can join batches with the same CI steps.

## CI
- Steps 01, 02, 04, 10 and 11, run once for all three batches. Steps 01, 02, 04 and 11 passed.
- Step 10 stopped at one check: it expected the hallway's note while the share was still sending ("sending it, sealed, to the machines named"). Sharing is now quick enough that the share has finished by the time the picture is taken ("shared with 1 of 1 machines"). The check now takes either note. Step 10 then passed (928 s).

## REVIEW-ROUND.md
- Section 3: list O, the optimisations left (docs/OPTIMISATIONS.md).
- Section 4: review 1's batches ticked, with the figures above (item 3d).
- Items 1, 2, 3 and 8 unticked: the physics, the catalogue, holding and the Keys screen were touched.

## Files to delete by hand
- None.
