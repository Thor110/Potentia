# Programs mixed (2 November 2026)

Each program whole, in this container: Linux's own (x86-64 ELF), the two Windows launchers Python's distlib carries (x86 and x86-64 PE), and five of Silesia's mozilla's Alpha libraries (COFF, which Sieve does not take for programs). Each coded by LZMA2 (raw, preset 6, as the packer's estimate), LZMA2 with the x86 filter (as the x86 stream), mix-v4 (`sieve mix`, B22), mix-v4 with the v33 draft's match ways (`B22 R1 M4 C2`), and those after the x86 filter (`B22 R1 M4 C2 X`, the filter the LZMA SDK's, here run on its own). Bytes; "mixed against LZMA2" is the smaller mixing against the smaller LZMA2. The last two columns are the packer's test on the file's first 256 KiB: mixing with the ways against LZMA2, without and with the filter (a file is mixed whole only where its start is 3% smaller).

| Program | Kind | Bytes | LZMA2 | LZMA2, x86 filter | mix-v4 | with the ways | x86 filter, with the ways | Mixed against LZMA2 | Start | Start, x86 filter |
| :--- | :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| bash | Linux x86-64 ELF | 1,446,024 | 583,676 | 545,378 | 519,247 | 508,442 | 477,770 | -12.4% | -6.2% | -6.1% |
| cmake | Linux x86-64 ELF | 11,796,472 | 3,348,353 | 2,921,639 | 3,098,976 | 2,992,678 | 2,611,053 | -10.6% | +9.0% | +8.5% |
| gdb | Linux x86-64 ELF | 8,920,528 | 3,002,098 | 2,725,747 | 2,773,240 | 2,690,201 | 2,439,091 | -10.5% | -13.3% | -14.8% |
| git | Linux x86-64 ELF | 4,066,232 | 1,682,358 | 1,549,874 | 1,458,128 | 1,430,036 | 1,315,846 | -15.1% | -9.9% | -9.8% |
| libc.so | Linux x86-64 ELF library | 2,129,424 | 770,826 | 744,138 | 727,090 | 706,842 | 683,705 | -8.1% | -5.1% | -4.8% |
| libcrypto.so | Linux x86-64 ELF library | 5,309,400 | 1,518,781 | 1,427,723 | 1,476,736 | 1,427,926 | 1,340,502 | -6.1% | -6.1% | -6.0% |
| libstdcxx.so | Linux x86-64 ELF library (libstdc++) | 2,592,224 | 676,996 | 639,261 | 675,050 | 649,556 | 614,028 | -3.9% | -1.6% | -1.8% |
| perl | Linux x86-64 ELF | 4,023,408 | 1,028,956 | 985,320 | 974,159 | 954,799 | 920,230 | -6.6% | -3.8% | -3.5% |
| python3 | Linux x86-64 ELF | 6,536,120 | 2,074,086 | 1,954,406 | 1,939,513 | 1,890,317 | 1,785,369 | -8.6% | -7.2% | -7.1% |
| t32.exe | Windows x86 PE (distlib's launcher) | 97,792 | 41,425 | 39,438 | 42,737 | 40,156 | 38,248 | -3.0% | -3.1% | -3.0% |
| w64.exe | Windows x86-64 PE (distlib's launcher) | 101,888 | 43,427 | 41,672 | 44,258 | 41,722 | 40,020 | -4.0% | -3.9% | -4.0% |
| libgkcontent.so | Alpha COFF library (mozilla) | 7,036,928 | 1,320,366 | 1,337,844 | 1,343,705 | 1,273,717 | 1,292,191 | -3.5% | -0.1% | +3.4% |
| libgklayout.so | Alpha COFF library (mozilla) | 4,292,608 | 1,043,198 | 1,059,237 | 1,032,562 | 1,001,257 | 1,014,709 | -4.0% | +8.4% | +5.7% |
| libmailnews.so | Alpha COFF library (mozilla) | 1,196,032 | 267,505 | 270,183 | 268,271 | 255,978 | 258,828 | -4.3% | -8.0% | -8.1% |
| libnecko.so | Alpha COFF library (mozilla) | 1,499,136 | 333,675 | 337,532 | 337,992 | 324,013 | 327,991 | -2.9% | -4.2% | -4.8% |
| libxpcom.so | Alpha COFF library (mozilla) | 1,622,016 | 359,752 | 366,893 | 364,547 | 349,261 | 355,820 | -2.9% | -2.1% | -2.5% |

Every program came out smaller mixed: the x86 ones 3.0% to 15.1% under LZMA2 with the filter, through the filter and with the ways (every one of the 11 smaller so than without the filter); the Alpha ones 2.9% to 4.3% under LZMA2, with the ways and no filter (the x86 filter does not fit their instructions). By the start test, 5 of the 16 would not have been mixed (cmake 8.5% larger on its first 256 KiB with the filter, mozilla's libgklayout 8.4% larger), each smaller whole.

What follows (v33 draft, `tools/cli/pack.cpp`): a program the x86 stream would carry (Sieve's ELF and EXE) is weighed mixed whole, through the filter (`mix4 ... X`), against LZMA2 with the filter, with no start test; the programs so mixed are weighed again against the x86 stream they would share. The packer's tables for a small file are smaller than B22 (B18 at 98 KB), where t32.exe came out 39,804 bytes mixed against 39,438 by LZMA2 (38,248 at B22), so it stays in the x86 stream. Decoding mixing runs at about 0.5 to 1 MB a second, against LZMA2's tens; with Prioritise Decoding Speed no program is mixed.
