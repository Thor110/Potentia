# Your characters are objects in your dimension

## As you asked
- Your characters still start where they stood (yours ahead of where you begin, your store's round the hilltop), but they are now objects like everything else there:
  - **E** picks one up; **R/T/G** turn it; **Return** places it; **Q** drops it, and it falls and can be knocked like the rest; **Backspace** takes it away.
  - The **catalogue** (Tab) ends with "your character", then your store's characters, so you can place them again (more than once, if you like).
  - Things in front of them hide them as before, and they are aimed at by their spheres.
- "Your character" means whichever character is yours at the time: change it on the CHARACTER page and the one in your world changes with it. A store character is named by its SHA-256.
- **The world file:** while every character stays where it starts, nothing about them is written, so an untouched world is still a v1 file. Once you move one, the file gets a `characters placed` line and a `place char:you ...` (or `char:SHA`) line for each, so a character you take away stays away. A character the file names that isn't here is kept in the file and not drawn (SPECIFICATIONS §12.0r).
- The map shows the characters where they are; the readout counts characters apart from objects.

## CI (step 10)
- New checks: an untouched world stays v1; picking up, turning and placing your character writes where it is; taking it away keeps it away; the catalogue places it again.
- The bookcase and favourite tests now press Down to their row (the catalogue's last rows are now the characters).

## Ideas recorded
- IDEAS 19.221: coloured rings round what you hold, one for each way it turns (x red, y green, z blue by default, the common convention of 3D tools; one global setting for every place axes are drawn, as nothing in Sieve colours axes yet), each a little larger than the object's farthest point; once R, T or G is pressed, the mouse wheel turns it back and forth about that axis.
- IDEAS 19.222: day and night in your dimension, on or off, the day's length any whole number of seconds up to what an unsigned 64-bit value holds (a larger one refused, never wrapped), kept in the dimension's file.

## REVIEW-ROUND.md
- Item 25 added: your characters as objects in your dimension.
- Tasks P7 (the rings and the wheel) and P8 (day and night) added to list P.

## Files to delete by hand
- None.
