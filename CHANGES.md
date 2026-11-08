# MP3 check: MPEG-2 and 2.5 named (9 November 2026)

Sieve/tools/cli/mp3_glue.cpp: a file starting with an MPEG-2 or MPEG-2.5 Layer III frame (FF F2/F3,
FF E2/E3) now reads "not coded: file is MPEG-2 LAYER III, not supported" (or 2.5) instead of
"not an MP3's start", so the log counts them. Nothing else changes: Sieve already carried these as
their own bytes.

Checked here: an MPEG-2 and an MPEG-2.5 file made with LAME (the 2.5 one starts FF E3 20 C4, as
M.A.S.H..mp3 does) are named; an MPEG-1 file is still "same".
