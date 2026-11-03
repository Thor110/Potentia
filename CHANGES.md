# An idea noted: files routed to other programs (3 November 2026)

Unzip this over the repository root, on top of your commit abc1bd3 ("sieve simage"). One local
commit, not pushed: 0cde159. Documentation only.

**IDEAS 19.115**: opening a .sieve decodes it as now (checked against its SHA-256) and hands each
file to the program named for its kind in an .ini, by extension or by data type (for example
`.pdf = reader.exe`, `text = notepad.exe`, `picture = sieve-view`). The note lists the
questions for when it's taken up:
- where the decoded files live while the other program has them;
- what happens with a .sieve of many files;
- what the data type is when the extension is missing or wrong;
- the same refusals the viewer makes before decoding, since any program it hands a file to sees
  what the file claims to be.
