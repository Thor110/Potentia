# Third-party licences

Everything in Sieve that was not written for it, with its licence. The folders hold the licence texts.

| Component | Version | Licence | Where it is used | Licence file |
| :--- | :--- | :--- | :--- | :--- |
| SDL | 3.2.30 when CMake fetches it (a vcpkg build uses vcpkg's SDL3) | zlib | The hallway and `sieve-install` (window, input, drawing). Fetched and built by CMake, or taken from vcpkg; not stored in this repository | `SDL3/LICENSE.txt` |
| stb_image, stb_image_write | 2.30, 1.16 | MIT or public domain (your choice) | Reading pictures and writing PNGs (`third_party/stb`) | `stb/LICENSE.txt` |
| SCOWL word lists | 2020.12.07 | SCOWL's permissive licence (notice required) | The dictionaries in `data/dictionaries` | `SCOWL/Copyright.txt` |
| font8x8 | - | Public domain | The menu font, `data/fonts/sieve8x8.hex` | `font8x8/LICENSE.txt` |
| zlib | 1.3.2 when CMake fetches it (else the system's) | zlib | The `sieve` tool's comparison page: zip's deflate (`tools/cli/compare.cpp`). Fetched and built by CMake if the system has none; not stored in this repository | `zlib/LICENSE` |
| LZMA SDK (7z decoder) | 26.03 (from 7-Zip 26.03) | Public domain | `sieve-install` only: unpacking an installer that carries one 7z archive (`client/unpack_7z.cpp`); the files are in `third_party/lzma` | `lzma/LICENSE.txt` |
| liblzma (XZ Utils) | 5.8.1 when CMake fetches it (else the system's) | 0BSD for liblzma, the only part built; the rest of XZ Utils (command line tools, scripts, build system) is not built or shipped | The `sieve` tool's comparison page: 7z's LZMA2 (`tools/cli/compare.cpp`). Fetched and built by CMake if the system has none; not stored in this repository | `xz/COPYING`, `xz/COPYING.0BSD` |

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
