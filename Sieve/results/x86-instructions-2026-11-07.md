# The x86 instruction model, mix-v4's E1 (v34 draft, 7 November 2026)

Edward: "Yes, start with the x86 instruction model", after the cmix yardstick
(results/cmix-yardstick-2026-11-07.md) showed programs as the widest gap: `/usr/bin/ls` 36% larger
than cmix makes it. mix-v4 filtered a program (X, the x86 filter) and then mixed its bytes as any
others; paq8's model of executables, which cmix carries, also predicts from where each byte falls in
its instruction. E1 does that, in integers, written anew: SPECIFICATIONS §12.2 v34.

## What it is

A small decoder of x86-64's encodings follows the bytes as they are mixed (prefixes and REX, the
three opcode maps, ModRM, SIB, displacement, immediate). Where E1 is named:

- the three contexts mix-v4 makes for text's lines and fields are three from the instruction: what
  this byte is with its opcode; with its ModRM and SIB and the byte before; with the instruction
  before's opcode;
- two of the three sparse contexts are the last three instructions' opcodes, and, in an immediate,
  the byte an absolute address near here would have there (a call's target, through the filter);
- each mixer's weights are chosen by the byte's part of its instruction too;
- one stage after the mixers (as sse-v1's), its context the opcode and the part, half of what is
  coded.

It only chooses contexts: where it reads the bytes wrongly (data among the code, 32-bit code), the
code is a little larger and the file comes back the same.

## How it was chosen

On six programs (`ls`, `bash`, `git`, libc, distlib's two Windows launchers), mix-v4 with the ways
and the filter (`R1 M4 C2 X`), 2,604,959 bytes together:

| Step | Bytes together | Against before |
| :--- | ---: | ---: |
| The instruction's three contexts in place of the text's | 2,464,566 | −5.39% |
| Two sparse contexts made the last three opcodes and the address's byte | 2,455,626 | −5.73% |
| The second mixer's weights chosen by the part | 2,438,390 | −6.39% |
| The first mixer's too | 2,430,925 | −6.68% |
| One stage after the mixers, half of what is coded | **2,409,911** | **−7.49%** |
| (A second stage, keyed by the byte before: not kept, 0.05% more) | 2,408,776 | −7.53% |

Three other ways of the sparse contexts were tried and came out larger than the one kept.

## Measured

Every one given back the same by `sieve mix --decode`; `ls` and the CI's program by sieve, sieve-install
and the oracle (its own X86Reader, in Python: t32.exe's code read back byte for byte, 35 s).

| Program | Kind | Bytes | v33 (`R1 M4 C2 X`) | v34 (`R1 M4 C2 E1 X`) | Change |
| :--- | :--- | ---: | ---: | ---: | ---: |
| ls | Linux x86-64 ELF | 142,312 | 48,480 | 45,201 | −6.8% |
| t32.exe | Windows x86 PE (distlib) | 97,792 | 38,701 | 36,413 | −5.9% |
| w64.exe | Windows x86-64 PE (distlib) | 101,888 | 40,457 | 38,591 | −4.6% |
| bash | Linux x86-64 ELF | 1,446,024 | 477,770 | 443,583 | −7.2% |
| libc.so | Linux x86-64 ELF library | 2,129,424 | 683,705 | 634,633 | −7.2% |
| git | Linux x86-64 ELF | 4,066,232 | 1,315,846 | 1,211,490 | −7.9% |
| perl | Linux x86-64 ELF | 4,023,408 | 920,230 | 863,733 | −6.1% |
| libcrypto.so | Linux x86-64 ELF library | 5,309,400 | 1,340,502 | 1,287,194 | −4.0% |
| python3 | Linux x86-64 ELF | 6,536,120 | 1,785,369 | 1,711,525 | −4.1% |
| gdb | Linux x86-64 ELF | 8,920,528 | 2,439,091 | 2,278,475 | −6.6% |
| cmake | Linux x86-64 ELF | 11,796,472 | 2,611,053 | 2,414,180 | −7.5% |
| Together | | 44,569,600 | 11,701,204 | 10,965,018 | −6.3% |

(bash, git and the rest at B22, `ls` and the launchers at B20, as the packer picks.)

| Time and memory (one run each) | v33 codes | v33 decodes | v34 codes | v34 decodes |
| :--- | ---: | ---: | ---: | ---: |
| ls | 0.5 s | 0.2 s | 0.7 s | 0.3 s |
| bash | 4.5 s | 1.8 s | 5.2 s | 2.5 s |
| libc.so | 6.1 s | 2.6 s | 8.1 s | 4.0 s |
| git | 12.8 s | 5.2 s | 15.5 s | 7.0 s |
| Memory (git) | 306 MB | 302 MB | 310 MB | 307 MB |

Decoding about 35% to 55% slower; about 4 MB more (the stage, and the first mixer's weights eight
times over).

**Against cmix:** `ls` 45,201 now, against cmix's 35,567 (27% larger, from 36%).

## In the packer

With `--v34-draft` (the File Locator's v34 DRAFT), a program carried by mixing (an ELF or EXE the x86
stream would take) is mixed with E1; the `.sieve` is v34. `ls` with Optimise Search: 48,552 bytes at
v33, 45,276 with the draft. CI step 11 checks the made-up program of v33's piece: 9,608 bytes at
v33, 9,345 at v34, given back by Sieve, sieve-install and the oracle.

## Next

The gap to cmix on programs is still 27%: paq8's model also follows jumps' targets and register use;
cmix adds an LSTM and many more mixers. Each would be measured as these were.
