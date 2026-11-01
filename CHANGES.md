# Silesia at v32 (commit ff3e7c6)

| File | Bytes | 7z (9e) | size, Try every way | size, without | decoding (either) | Against 7z, for size |
| :--- | ---: | ---: | ---: | ---: | ---: | ---: |
| mozilla | 51,220,480 | 13,376,180 | **13,376,239** | 13,376,239 | 13,376,239 | +59 bytes |
| webster | 41,458,703 | 8,368,609 | **6,246,725** | 8,355,800 | 8,355,800 | −25.36% |
| nci | 33,553,445 | 1,449,209 | **1,293,653** | 1,440,114 | 1,440,114 | −10.73% |
| samba | 21,606,400 | 3,739,464 | **3,287,995** | 3,728,015 | 3,728,015 | −12.07% |
| dickens | 10,192,446 | 2,831,149 | **2,210,442** | 2,210,442 | 2,825,117 | −21.92% |
| osdb | 10,085,684 | 2,844,493 | **2,389,894** | 2,389,894 | 2,835,826 | −15.98% |
| mr | 9,970,564 | 2,751,831 | **1,967,650** | 1,967,650 | 2,314,195 | −28.50% |
| x-ray | 8,474,240 | 4,491,203 | **3,622,683** | 3,622,683 | 4,146,903 | −19.34% |
| sao | 7,251,944 | 4,425,604 | **4,170,594** | 4,413,371 | 4,413,371 | −5.76% |
| reymont | 6,627,202 | 1,315,531 | **1,093,050** | 1,093,050 | 1,313,857 | −16.91% |
| ooffice | 6,152,192 | 2,427,163 | **2,079,028** | 2,079,028 | 2,079,028 | −14.34% |
| xml | 5,345,280 | 434,830 | **353,167** | 353,167 | 433,871 | −18.78% |
| **all twelve** | **211,938,580** | **48,455,266** | **42,091,120** | **45,029,453** | **47,262,336** | **−13.13%** |

- Every one of the 48 runs installed back the same.
- **Try every way:** without it, mixing skips files over 16 MiB, so webster (41 MB) is 8,355,800 without it and 6,246,725 with it.
- **mozilla** (a tar of programs) comes out as LZMA2 does, 59 bytes larger: read as one file, its programs get no x86 filter. A tar read as its members could fix that; not measured.
- **Files:** Proof/STATISTICS.md (a new Silesia section and an at-a-glance row), and ROADMAP and SHOWCASE-PLAN.
- **Still to run:** compare.py beside 7-Zip itself, xz, zstd, brotli and paq8px.
