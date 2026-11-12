# Changes since 1eedaf3 "sieve ideas" (12 November 2026)

Sixteen commits (84d4054 to f8b4d5a). This zip replaces every zip sent today. One code change (the hallway's shimmer) and one comment on the website; the rest is documentation.

## Code
- **Sieve/client/hallway.cpp:** the item under the crosshair shimmers (IDEAS 5.6): its tint breathes over 1.6 s and a leaning band sweeps across its front. Comments on the binary line's doors corrected (the last line before binary is AI now, not models). CI step 10 passes (916 s).

## The ideas, filtered
- **Sieve/docs/IDEAS-TRIAGE.md (new):** every idea (584) given a kind, a status and a verdict, with its reason.
- **Sieve/docs/ideas/ (new folder):**
  - IMPLEMENTED.md: 40 done entries, moved whole (5.6, the shimmer, among them), and every one checked against the code: all present, none missing; 13 have moved on since and carry a dated note of how.
  - DEPRECATED.md: 19 set-aside entries, all of §17 among them, with their reasons.
  - WORK-DONE.md: a dated log of work, from results/ and today's.
  - VOICES-STASHED.md: VOICES' technical lines, each with where its idea went.
  IDEAS.md keeps a one-line pointer at every moved number.
- **Sieve/docs/ROADMAP.md:** the long plan through the ideas, stages 8 to 18, at the top. Stage 8 (the record) and Stage 10 (hallway polish) are begun.

## New ideas noted (IDEAS §19.140 to §19.147)
- §19.140: the tactile Sieve.
- §19.141: the Game of Life over noise through time, and infinite short routes into the space.
- §19.142: Sieve as a lock in time.
- §19.143: colour at infinite dimensions; the setup menu as a scroll strip; views from other angles. The colour-blindness pass was done (6 November), and the plans that said otherwise are corrected.
- §19.144: an emotion dimension as a circle of doors; highway dimensions; AI and human work preserved; doors through doors.
- §19.145: the cross-dimensional lattice of highways, and ideas checked against what was built.
- §19.146: voices heard once a save; a highway for stories and genres.
- §19.147: characters that speak; a character system from low to high poly; one interface for every engine and device; the scalable graphics seed and the pre-render barrier.

## Documents matched to the code (IDEAS §19.134)
Fifteen places corrected across three files:
- **MANUAL.md:** the door order with worlds and AI; size limits that follow the machine; ITEM MEMORY; META as the fifth tab; the menu walk to Settings; `compress` and `speak`; the tile size; the 729-unit example.
- **SPECIFICATIONS.md:** colour table and door order with worlds and AI; the entropy filters' lines.
- **FILTER-PLUGINS.md:** retired data filters; filter forms.

## VOICES
- 62 technical lines and Gemini's two messages moved to ideas/VOICES-STASHED.md.
- The note at the top updated.
- 38 of Edward's lines added to the candidates today.

## The website
- **Pages/release.html (Build or Buy):** the comment on two constants no longer points at the Orbital page's script, removed on 6 November.

## Other documents
- GAME.md and HANDOFF.md: the colour-blindness pass marked done.
- GAME-OF-LIFE.md: a pointer to §19.141.
