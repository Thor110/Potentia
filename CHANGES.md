# A RAID folder's id: every source hashed together

## As you asked
- A new RAID folder's id is now the first 32 hex digits of the SHA-256 of all of these:
  - the system's random bytes (as a key is made);
  - `std::random_device`;
  - this machine's id, where the system keeps one (`/etc/machine-id` on Linux), and its host name;
  - the drive the folder is on and the folder's own number on it (the volume serial and file index on Windows, the device and inode elsewhere);
  - its path;
  - the time, from both clocks.
- No one of them failing can make two folders alike. Some MinGW builds' `std::random_device` gives a fixed sequence, which on its own would have given every new folder the same id, and the RAID would have refused the second as "a folder given twice".
- It is hashed, so nothing of the machine can be read from the id, which travels with the folder.
- It is still 32 lower-case hex digits, so folders made before keep their ids, and the build before this one reads the new folders. Checked: three folders made together got three different ids, and the old build listed them as the new one did.
- The case it guards against (a fixed random sequence) could not be reproduced here. That two folders on one drive differ follows from the folder number, by reasoning, not by test.
- SPECIFICATIONS §12.0l (the infinite RAID's folders) says how the id is made.

## CI
- Steps 01, 04 and 10: all passed.

## Files to delete by hand
- None.
