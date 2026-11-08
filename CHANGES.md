# Sound as FLAC: temporary / permanent / off (9 November 2026)

As agreed: the sound switch alone decides how sound is taken (the archives' boxes no longer do), with
the v34 draft on.

- Temporary (the default): only what Sieve gives back byte for byte. WAV, AIFF, RF64, Wave64, .au, CAF
  as FLAC plus the rest of the file ("flac"); MPEG-1 MP3 as packMP3's form ("pmp"). Everything else
  (FLAC, WavPack, Monkey's Audio, TTA, ALAC, MPEG-2/2.5 MP3) carried as its bytes, the best way found,
  until Sieve can give it back exactly.
- Permanent: temporary's, plus installed as name.flac where smaller (WAV family; WavPack, Monkey's
  Audio, TTA, ALAC through ffmpeg); a FLAC re-coded. Asked for by name; packing warns.
- Off: sound left to the ways every file has.

Command line: --sound-temporary, --sound-permanent (--sound-to-flac still works), --sound-off. File
Locator: W is now a button going round "Sound as FLAC: Temporary / Permanent / Off" (read aloud the
same). Settings: [locator] sound = temporary|permanent|off (an old sound_to_flac = 1 is read as
permanent).

Archives chained to sound: a zip, 7z or tar made again from its skeleton (archives' temporary) now
carries its WAV and MP3 members as flac / pmp, so the archive still comes back byte for byte; the
tar's weighing counts them so. An archive installed as its contents (archives' permanent) has its
sound files taken as any file (so with sound permanent, pack/b.wav becomes pack/b.flac). Reader,
sieve-install and the oracle accept flac and pmp members (SPECIFICATIONS v34); the oracle now makes
members before their archive.

Also: the File Locator's layout counted rows without the Compare box, so with the draft on the "took"
line overlapped the Tailor button; fixed.

Docs: FORMATS.md has a new section, "Sound: every format, and where it goes" (each format under
temporary, permanent and off, and why); MANUAL.md, SPECIFICATIONS.md, NEXT-VERSION.md, FORMATS-PLAN.md
(the rule: a format joins temporary only when Sieve gives it back exactly), help, settings text.

Checked here: unit suite (4/4); new CI step 11 block (temporary default, off, zip members flac+pmp by
Sieve and sieve-install, a tar's flac member by the oracle, permanent .flac inside an archive); step
11's existing MP3, PCX, FLAC, AIFF, RF64, Sound as FLAC, tags and float blocks; step 08's block; the
hallway's W read aloud through all three.

Please run: build, the full CI (steps 08, 10, 11 touched).
