# Third-party licences

Everything in Sieve that was not written for it, with its licence. The folders hold the licence texts.

| Component | Version | Licence | Where it is used | Licence file |
| :--- | :--- | :--- | :--- | :--- |
| SDL | 3.2.30 when CMake fetches it (a vcpkg build uses vcpkg's SDL3) | zlib | The hallway and `sieve-install` (window, input, drawing). Fetched and built by CMake, or taken from vcpkg; not stored in this repository | `SDL3/LICENSE.txt` |
| stb_image, stb_image_write | 2.30, 1.16 | MIT or public domain (your choice) | Reading pictures and writing PNGs (`third_party/stb`) | `stb/LICENSE.txt` |
| SCOWL word lists | 2020.12.07 | SCOWL's permissive licence (notice required) | The dictionaries in `data/dictionaries`, and the words of `data/filters/moby-inflections-v1.tsv` and `moby-names-v1.tsv` | `SCOWL/Copyright.txt` |
| Moby Part-of-Speech II (Grady Ward) | The list as mirrored at github.com/Hyneman/moby-project, commit 672f6bd (Project Gutenberg eBook #3203) | Public domain (the author's grant, January 2001) | The grammar filter plugins in `data/filters` (`moby-pos-v1.tsv`, converted by `tools/moby_pos.py`; see `data/filters/moby-pos-v1.md`). The inflections and names lists beside it are drawn from SCOWL as well, whose notice covers them | `Moby/PUBLIC-DOMAIN.txt` |
| font8x8 | - | Public domain | The menu font, `data/fonts/sieve8x8.hex` | `font8x8/LICENSE.txt` |
| zlib | 1.3.2 when CMake fetches it (else the system's) | zlib | The `sieve` tool's comparison page: zip's deflate (`tools/cli/compare.cpp`). Fetched and built by CMake if the system has none; not stored in this repository | `zlib/LICENSE` |
| LZMA SDK (7z decoder) | 26.03 (from 7-Zip 26.03) | Public domain | `sieve-install` only: unpacking an installer that carries one 7z archive (`client/unpack_7z.cpp`); the files are in `third_party/lzma` | `lzma/LICENSE.txt` |
| PDQ (Meta's reference C++) | ThreatExchange commit ec3671b (2026-09-25) | BSD | The vault's perceptual check for pictures (`tools/cli/pdq_hash.cpp`), in every program that checks the vault, the installer included; the files are in `third_party/pdq` | `pdq/LICENSE` |
| liblzma (XZ Utils) | 5.8.1 when CMake fetches it (else the system's) | 0BSD for liblzma, the only part built; the rest of XZ Utils (command line tools, scripts, build system) is not built or shipped | The `sieve` tool's comparison page: 7z's LZMA2 (`tools/cli/compare.cpp`). Fetched and built by CMake if the system has none; not stored in this repository | `xz/COPYING`, `xz/COPYING.0BSD` |
| libbzip2 (bzip2) | 1.0.8, always fetched by CMake from gitlab.com/bzip2/bzip2 (tag bzip2-1.0.8) | BSD-style (bzip2's own licence: keep the notice; altered versions marked as such) | The `sieve` tool and the installers: bzip2 archives read, carried as their contents, and remade exactly (`tools/cli/pack.cpp`). Its library's C files only, built as a static library; the `bzip2` program and its scripts are not built or shipped; not stored in this repository | `bzip2/LICENSE` |
| zstd (Zstandard) | 1.5.7, always fetched by CMake from github.com/facebook/zstd (tag v1.5.7) | BSD (zstd is offered under BSD or GPLv2; Sieve takes the BSD licence) | The `sieve` tool and the installers: zstd archives read, carried as their contents, and remade exactly where this same version made them (`tools/cli/pack.cpp`). The library alone, static and single-threaded; its programs are not built or shipped; not stored in this repository | `zstd/LICENSE` |
| lz4 (the library) | 1.10.0, always fetched by CMake from github.com/lz4/lz4 (tag v1.10.0) | BSD 2-clause (the `lib` folder's own licence; the lz4 programs are GPLv2 and are not built or shipped) | The `sieve` tool and the installers: lz4 files read, carried as their contents, and remade exactly (`tools/cli/pack.cpp`). Its library's four C files, built as a static library; not stored in this repository | `lz4/LICENSE` |
| brotli | 1.2.1, always fetched by CMake from github.com/google/brotli (tag v1.2.1) | MIT | The `sieve` tool and the installers: brotli files read, carried as their contents, and remade exactly (`tools/cli/pack.cpp`). Its C encoder, decoder and common files (with its built-in dictionary), built as a static library; not stored in this repository | `brotli/LICENSE` |
| libarchive | 3.8.10, always fetched by CMake from github.com/libarchive/libarchive (tag v3.8.10) | BSD 2-clause for most files; some files under a 3-clause UC Regents licence, the NetBSD Foundation's 2-clause licence, or in the public domain, and BLAKE2 under CC0 (see `COPYING`, and `NOTICES.txt` for every notice in the files built) | The `sieve` tool and the installers: RAR (its own reader, written for libarchive, not unRAR's), cab, ISO 9660, LHA, cpio, ar and WARC archives read and carried as their contents, never remade (`tools/cli/pack.cpp`). Its library alone, static, with no programs, tests or crypto and none of its optional compression libraries but zlib; not stored in this repository | `libarchive/COPYING`, `libarchive/NOTICES.txt` |

**ffmpeg is not part of Sieve.** Sieve reads picture and video formats beyond stb_image's with an ffmpeg program it finds on the machine (`tools/cli/media_decode.cpp`), run as a separate process: none of ffmpeg is linked, built or stored here, and Sieve works without it.
- **If you ship one beside Sieve,** that copy comes with its own licence, and its terms are the ones you follow:
  - **LGPL 2.1 or later:** the default build, whose decoders read every common format, H.264 included.
  - **GPL:** a build made with `--enable-gpl`, for example with x264.
- **Either way, shipping one means** its licence text, its notices, and its source (or a written offer of it) go with it.
- **Patents:** some codecs (H.264, HEVC) are covered by patent licences in some countries, separately from ffmpeg's copyright licence.

**Parts of SDL with their own notices.** A static build of the hallway compiles in code from SDL's source tree that carries its own licence. The notices it asks to be kept are here:

| Part | Licence | Compiled in when | File |
| :--- | :--- | :--- | :--- |
| HIDAPI (game controllers) | Your choice of GPLv3, BSD-style or the original HIDAPI licence; Sieve uses it under the BSD-style or original licence | Always, for controller support | `SDL3/hidapi-LICENSE.txt`, `SDL3/hidapi-LICENSE-bsd.txt`, `SDL3/hidapi-LICENSE-orig.txt` |
| yuv2rgb (video colour conversion) | BSD 3-clause | Always | `SDL3/yuv2rgb-LICENSE.txt` |
| Sun Microsystems maths library (`src/libm`) | Free to use and redistribute if the notice is kept | Always | `SDL3/libm-NOTICE.txt` |
| X11 keysym tables (`src/events/imKStoUCS.c`) | MIT/X11 | Linux builds with X11 or Wayland | `SDL3/x11-keysym-LICENSE.txt` |
| EDID parser (`src/video/x11/edid-parse.c`, Red Hat) | MIT | Linux builds with X11 | `SDL3/x11-edid-LICENSE.txt` |
| xdg-user-dirs lookup (`src/filesystem/unix/SDL_sysfilesystem.c`, Red Hat) | MIT | Linux and other Unix builds | `SDL3/xdg-user-dirs-LICENSE.txt` |

SDL's controller database comes from Valve under the zlib licence, the same terms as SDL. The Khronos OpenGL, EGL and Vulkan headers are MIT or Apache-2.0 and are used only for declarations. Neither needs a separate file.

**Data that is not shipped.** The model's training corpus (`corpus/`, fetched by `tools/fetch_corpus.py`) is public-domain Project Gutenberg texts as packaged by NLTK, and is not distributed. The pinned model `data/models/gutenberg-lower27-o5.model` holds only character counts derived from those texts. `tests/example_book_source.txt` is an excerpt of *A Tale of Two Cities* (public domain).

**Sieve itself** is part of Potentia, under the GNU Affero General Public License v3 (Potentia's `LICENSE`, copied beside the programs as `potentia-license.txt`).

When you add a library or data set, add its licence here. Keep each licence file as the upstream wrote it.
