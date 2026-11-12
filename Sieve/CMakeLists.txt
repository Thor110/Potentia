cmake_minimum_required(VERSION 3.21)
project(Sieve VERSION 0.13.78 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(SIEVE_BUILD_TESTS  "Build the core test suite"            ON)
option(SIEVE_BUILD_CLIENT "Build the SDL3 hallway client"          ON)
option(SIEVE_SMALL_INSTALLER "Build sieve-install on a trimmed SDL of its own, for size" ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

find_package(Threads REQUIRED)

# ---------------------------------------------------------------- core (no dependencies)
add_library(sieve_core STATIC
    core/src/alphabet.cpp
    core/src/unicode_blocks.cpp
    core/src/audio.cpp
    core/src/biguint.cpp
    core/src/jpeg_dc.cpp
    core/src/booksieve.cpp
    core/src/composition.cpp
    core/src/bookspace.cpp
    core/src/titledspace.cpp
    core/src/binaryspace.cpp
    core/src/canon.cpp
    core/src/chunks.cpp
    core/src/compact.cpp
    core/src/dfa.cpp
    core/src/plugin.cpp
    core/src/corridor.cpp
    core/src/modelspace.cpp
    core/src/filter.cpp
    core/src/filters/audio.cpp
    core/src/filters/binary.cpp
    core/src/filters/crossline.cpp
    core/src/filters/written.cpp
    core/src/filekind.cpp
    core/src/modelsieve.cpp
    core/src/worldspace.cpp
    core/src/character.cpp
    core/src/importers.cpp
    core/src/dimensions.cpp
    core/src/json.cpp
    core/src/safetensors.cpp
    core/src/weights.cpp
    core/src/weights_rans.cpp
    core/src/unicode_classes.cpp
    core/src/llm_tokenizer.cpp
    core/src/llm.cpp
    core/src/aispace.cpp
    core/src/digit_histogram.cpp
    core/src/rope_wave.cpp
    core/src/mix.cpp
    core/src/stop.cpp
    core/src/training.cpp
    core/src/diffusion.cpp
    core/src/filters/models.cpp
    core/src/filters/pattern.cpp
    core/src/written.cpp
    core/src/filters/builtin.cpp
    core/src/filters/media.cpp
    core/src/filters/picture.cpp
    core/src/filters/figure.cpp
    core/src/filters/sound.cpp
    core/src/filters/weights.cpp
    core/src/filters/statistics.cpp
    core/src/filters/text_m1.cpp
    core/src/guided.cpp
    core/src/model.cpp
    core/src/image.cpp
    core/src/intlog.cpp
    core/src/sha256.cpp
    core/src/notes3.cpp
    core/src/sieve.cpp
    core/src/sound.cpp
    core/src/space.cpp
    core/src/cipher.cpp
    core/src/utf8.cpp)
target_include_directories(sieve_core PUBLIC core/include)
target_link_libraries(sieve_core PUBLIC Threads::Threads)

if(MSVC)
    target_compile_options(sieve_core PUBLIC /W4 /utf-8 /permissive-)
else()
    target_compile_options(sieve_core PUBLIC -Wall -Wextra -Wpedantic)
endif()
# Warnings in Sieve's own code as errors (review, 9 November 2026: nothing failed a build on a new
# warning). Off unless asked; the Linux CI asks (GCC, where the build is clean of them).
option(SIEVE_WERROR "Warnings in Sieve's own code are errors" OFF)
if(SIEVE_WERROR)
    target_compile_options(sieve_core PUBLIC $<IF:$<CXX_COMPILER_ID:MSVC>,/WX,-Werror>)
endif()

# ---------------------------------------------------------------- command-line tool
# Image file I/O wraps stb_image (third-party, public domain). It lives in its own library so
# it is compiled without the core's strict warning flags.
add_library(sieve_image_io STATIC tools/cli/image_io.cpp)
target_include_directories(sieve_image_io PRIVATE core/include)
target_include_directories(sieve_image_io SYSTEM PRIVATE third_party/stb)
if(MSVC)
    target_compile_options(sieve_image_io PRIVATE /utf-8)
endif()

# Shared by the command-line tool and the hallway: the four lines, file input/output, dictionaries.
add_library(sieve_lines STATIC
    tools/cli/book.cpp
    tools/cli/dictionaries.cpp
    tools/cli/filter_config.cpp
    tools/cli/items.cpp
    tools/cli/lines.cpp
    tools/cli/map_sieve.cpp
    tools/cli/web_host.cpp
    tools/cli/media_decode.cpp
    tools/cli/models.cpp
    tools/cli/plugins.cpp
    tools/cli/tailor.cpp
    tools/cli/weigh.cpp)
target_include_directories(sieve_lines PUBLIC tools)
target_link_libraries(sieve_lines PUBLIC sieve_core sieve_image_io)
# (sieve_locate, defined below, is linked into sieve_lines after it: the lines check the vault.)

# The file locator and the manifest creator (tools/cli/locate.*), and the comparison of a file's
# size against what zip and 7z make of it (tools/cli/compare.*). The comparison needs the two
# compressors themselves: zlib (zip's deflate) and liblzma (7z's LZMA2). An installed copy is used
# if there is one; otherwise each is fetched from its own repository at a pinned release and built
# here as a static library, as SDL3 is below. Only the tools link them, never the core.
# (The cache entries set further down, for libarchive to find the zlib fetched here, are not a
# system's zlib: forgotten before looking, so a second configure fetches it again rather than
# taking them for one.)
if(ZLIB_LIBRARY STREQUAL "ZLIB::ZLIBSTATIC")
    unset(ZLIB_LIBRARY CACHE)
    unset(ZLIB_INCLUDE_DIR CACHE)
    unset(ZLIB_LIBRARY_RELEASE CACHE)
    unset(ZLIB_LIBRARY_DEBUG CACHE)
endif()
# A fetched dependency checked to be the commit it was pinned to (review, 9 November 2026: a tag can
# be moved, and the same tag then gives other source). Where its folder is a Git checkout and Git is
# here; a source folder given another way (FETCHCONTENT_SOURCE_DIR_<NAME>) is the giver's to vouch for.
function(sieve_pinned name dir commit)
    find_package(Git QUIET)
    if(GIT_FOUND AND EXISTS "${dir}/.git")
        execute_process(COMMAND "${GIT_EXECUTABLE}" -C "${dir}" rev-parse HEAD
                        OUTPUT_VARIABLE head OUTPUT_STRIP_TRAILING_WHITESPACE RESULT_VARIABLE failed ERROR_QUIET)
        if(NOT failed AND NOT head STREQUAL commit)
            message(FATAL_ERROR "${name}: fetched commit ${head}, but Sieve is pinned to ${commit}. Its tag may have moved: "
                                "check the source, then update the pin in CMakeLists.txt.")
        endif()
    endif()
endfunction()

find_package(ZLIB QUIET)
find_package(LibLZMA QUIET)
if(NOT ZLIB_FOUND OR SIEVE_FETCH_COMPRESSION)
    include(FetchContent)
    set(ZLIB_BUILD_TESTING OFF CACHE BOOL "" FORCE)
    set(ZLIB_BUILD_SHARED OFF CACHE BOOL "" FORCE)
    set(ZLIB_BUILD_STATIC ON CACHE BOOL "" FORCE)
    set(ZLIB_INSTALL OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(zlib
        GIT_REPOSITORY https://github.com/madler/zlib.git
        GIT_TAG v1.3.2
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(zlib)
    sieve_pinned(zlib "${zlib_SOURCE_DIR}" da607da739fa6047df13e66a2af6b8bec7c2a498) # v1.3.2
    set(SIEVE_ZLIB ZLIB::ZLIBSTATIC)
else()
    set(SIEVE_ZLIB ZLIB::ZLIB)
endif()
if(NOT LIBLZMA_FOUND OR SIEVE_FETCH_COMPRESSION)
    include(FetchContent)
    set(XZ_TOOL_XZ OFF CACHE BOOL "" FORCE)
    set(XZ_TOOL_XZDEC OFF CACHE BOOL "" FORCE)
    set(XZ_TOOL_LZMADEC OFF CACHE BOOL "" FORCE)
    set(XZ_TOOL_LZMAINFO OFF CACHE BOOL "" FORCE)
    set(XZ_TOOL_SCRIPTS OFF CACHE BOOL "" FORCE)
    set(XZ_TOOL_SYMLINKS OFF CACHE BOOL "" FORCE)
    set(XZ_TOOL_SYMLINKS_LZMA OFF CACHE BOOL "" FORCE)
    set(XZ_DOC OFF CACHE BOOL "" FORCE)
    set(XZ_NLS OFF CACHE BOOL "" FORCE)
    set(XZ_DOXYGEN OFF CACHE BOOL "" FORCE)
    set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(xz
        GIT_REPOSITORY https://github.com/tukaani-project/xz.git
        GIT_TAG v5.8.1
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(xz)
    sieve_pinned(xz "${xz_SOURCE_DIR}" a522a226545730551f7e7c2685fab27cf567746c) # v5.8.1
    set(SIEVE_LZMA liblzma)
    # Built inside this project, xz's liblzma target keeps its header folder to itself (it is
    # only exported when installed), so its users are given it here. A system with its own
    # lzma.h in /usr/include hides the omission, which is how it first got through.
    set(SIEVE_LZMA_INCLUDE ${xz_SOURCE_DIR}/src/liblzma/api)
else()
    set(SIEVE_LZMA LibLZMA::LibLZMA)
endif()

# bzip2 and zstd, for archives of those kinds (pack.cpp: read, carried as their contents, and remade
# exactly where the same library makes the same bytes; IDEAS §19.88). Always fetched at the version
# pinned here, never a system copy: a zstd file is remade only by the version that made it, so every
# build of Sieve must have the same one. Static, and only the tools link them, never the core.
include(FetchContent)
# bzip2 1.0.8 (BSD-style licence, third_party_licenses/bzip2): its library's seven C files, built
# here (its release has no CMake build).
FetchContent_Declare(bzip2
    GIT_REPOSITORY https://gitlab.com/bzip2/bzip2.git
    GIT_TAG bzip2-1.0.8
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(bzip2)
sieve_pinned(bzip2 "${bzip2_SOURCE_DIR}" 6a8690fc8d26c815e798c588f796eabe9d684cf0) # bzip2-1.0.8
add_library(sieve_bzip2 STATIC
    ${bzip2_SOURCE_DIR}/blocksort.c ${bzip2_SOURCE_DIR}/huffman.c ${bzip2_SOURCE_DIR}/crctable.c ${bzip2_SOURCE_DIR}/randtable.c
    ${bzip2_SOURCE_DIR}/compress.c ${bzip2_SOURCE_DIR}/decompress.c ${bzip2_SOURCE_DIR}/bzlib.c)
target_include_directories(sieve_bzip2 SYSTEM PUBLIC ${bzip2_SOURCE_DIR})
if(MSVC)
    target_compile_options(sieve_bzip2 PRIVATE /w)
else()
    target_compile_options(sieve_bzip2 PRIVATE -w)
endif()
# zstd 1.5.7 (BSD licence, third_party_licenses/zstd; also offered under GPLv2, not taken): its
# library alone, static, single-threaded.
set(ZSTD_BUILD_PROGRAMS OFF CACHE BOOL "" FORCE)
set(ZSTD_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(ZSTD_BUILD_STATIC ON CACHE BOOL "" FORCE)
set(ZSTD_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ZSTD_BUILD_CONTRIB OFF CACHE BOOL "" FORCE)
set(ZSTD_LEGACY_SUPPORT OFF CACHE BOOL "" FORCE)
set(ZSTD_MULTITHREAD_SUPPORT OFF CACHE BOOL "" FORCE)
FetchContent_Declare(zstd
    GIT_REPOSITORY https://github.com/facebook/zstd.git
    GIT_TAG v1.5.7
    GIT_SHALLOW TRUE
    SOURCE_SUBDIR build/cmake)
FetchContent_MakeAvailable(zstd)
sieve_pinned(zstd "${zstd_SOURCE_DIR}" f8745da6ff1ad1e7bab384bd1f9d742439278e99) # v1.5.7
set(SIEVE_ZSTD libzstd_static)
set(SIEVE_ZSTD_INCLUDE ${zstd_SOURCE_DIR}/lib)
# lz4 1.10.0 (BSD 2-clause, third_party_licenses/lz4; the library's own licence, its lib folder's:
# the lz4 programs are GPLv2 and are not built): its library's C files, built here. Its output was
# the same as 1.9.4's on every file and level measured, but it stays pinned as the others are.
FetchContent_Declare(lz4
    GIT_REPOSITORY https://github.com/lz4/lz4.git
    GIT_TAG v1.10.0
    GIT_SHALLOW TRUE
    SOURCE_SUBDIR lib) # (no CMakeLists.txt there: fetched, not built by its own CMake)
FetchContent_MakeAvailable(lz4)
sieve_pinned(lz4 "${lz4_SOURCE_DIR}" ebb370ca83af193212df4dcbadcc5d87bc0de2f0) # v1.10.0
add_library(sieve_lz4 STATIC ${lz4_SOURCE_DIR}/lib/lz4.c ${lz4_SOURCE_DIR}/lib/lz4hc.c ${lz4_SOURCE_DIR}/lib/lz4frame.c
    ${lz4_SOURCE_DIR}/lib/xxhash.c)
target_include_directories(sieve_lz4 SYSTEM PUBLIC ${lz4_SOURCE_DIR}/lib)
# brotli 1.2.1 (MIT, third_party_licenses/brotli): its encoder's, decoder's and common C files,
# built here.
FetchContent_Declare(brotli
    GIT_REPOSITORY https://github.com/google/brotli.git
    GIT_TAG v1.2.1
    GIT_SHALLOW TRUE
    SOURCE_SUBDIR c/include) # (as lz4: fetched, not built by its own CMake)
FetchContent_MakeAvailable(brotli)
sieve_pinned(brotli "${brotli_SOURCE_DIR}" db24a98aed38a16de78cc05ca15cdd8dc2a0d92d) # v1.2.1
file(GLOB SIEVE_BROTLI_SOURCES ${brotli_SOURCE_DIR}/c/common/*.c ${brotli_SOURCE_DIR}/c/dec/*.c ${brotli_SOURCE_DIR}/c/enc/*.c)
add_library(sieve_brotli STATIC ${SIEVE_BROTLI_SOURCES})
target_include_directories(sieve_brotli SYSTEM PUBLIC ${brotli_SOURCE_DIR}/c/include)
# libarchive 3.8.10 (BSD 2-clause, third_party_licenses/libarchive): the archives Sieve has no reader
# of its own for (RAR, cab, ISO 9660, LHA, cpio, ar, WARC), carried as their contents and never
# remade. Its library alone, static, with none of its programs, tests, crypto or the compression
# libraries Sieve already reads itself; zlib, for cab's MSZIP, is Sieve's own.
set(SIEVE_SAVED_BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS})
set(BUILD_SHARED_LIBS OFF)
foreach(o ENABLE_MBEDTLS ENABLE_NETTLE ENABLE_OPENSSL ENABLE_MD ENABLE_LIBB2 ENABLE_LZ4 ENABLE_LZO ENABLE_LZMA ENABLE_ZSTD ENABLE_BZip2
          ENABLE_LIBXML2 ENABLE_BSDXML ENABLE_EXPAT ENABLE_WIN32_XMLLITE ENABLE_PCREPOSIX ENABLE_PCRE2POSIX ENABLE_LIBGCC ENABLE_CNG
          ENABLE_TAR ENABLE_CPIO ENABLE_CAT ENABLE_UNZIP ENABLE_XATTR ENABLE_ACL ENABLE_ICONV ENABLE_TEST ENABLE_INSTALL)
    set(${o} OFF CACHE BOOL "" FORCE)
endforeach()
set(ENABLE_ZLIB ON CACHE BOOL "" FORCE)
# No POSIX regex either (only libarchive's programs and its pattern options use it, none here): on
# Windows, where there is none, libarchive would look for PCRE and stop at libgcc otherwise.
set(POSIX_REGEX_LIB NONE CACHE STRING "" FORCE)
if(TARGET ZLIB::ZLIBSTATIC)
    # The zlib fetched above: libarchive finds it by its header (copied with the zconf.h its build
    # wrote, as FindZLIB reads the version from it) and links the target.
    set(SIEVE_ZLIB_HEADERS ${CMAKE_BINARY_DIR}/sieve-zlib-include)
    file(MAKE_DIRECTORY ${SIEVE_ZLIB_HEADERS})
    file(COPY ${zlib_SOURCE_DIR}/zlib.h DESTINATION ${SIEVE_ZLIB_HEADERS})
    if(EXISTS ${zlib_BINARY_DIR}/zconf.h)
        file(COPY ${zlib_BINARY_DIR}/zconf.h DESTINATION ${SIEVE_ZLIB_HEADERS})
    else()
        file(COPY ${zlib_SOURCE_DIR}/zconf.h DESTINATION ${SIEVE_ZLIB_HEADERS})
    endif()
    set(ZLIB_INCLUDE_DIR ${SIEVE_ZLIB_HEADERS} CACHE PATH "" FORCE)
    set(ZLIB_LIBRARY ZLIB::ZLIBSTATIC CACHE STRING "" FORCE)
    # (And as plain variables: where the system has a zlib of its own, the search above left these
    # pointing at it, hiding the cache's.)
    set(ZLIB_INCLUDE_DIR ${SIEVE_ZLIB_HEADERS})
    set(ZLIB_LIBRARY ZLIB::ZLIBSTATIC)
    set(ZLIB_LIBRARIES ZLIB::ZLIBSTATIC)
    # On Windows, libarchive tests whether zlib needs ZLIB_WINAPI or ZLIB_DLL by building and running
    # a program against it, which cannot be done before zlib is built. The answers for the static
    # zlib built here (neither) are given beforehand, with the library they are for, so the tests are
    # skipped (they run only where the answer is not already known for that library).
    foreach(v ZLIB_WINAPI ZLIB_DLL)
        set(${v} 0 CACHE INTERNAL "" FORCE)
        set(${v}_WITH_LIB ZLIB::ZLIBSTATIC CACHE INTERNAL "" FORCE)
    endforeach()
    set(WITHOUT_ZLIB_DLL 1 CACHE INTERNAL "" FORCE)
    set(WITHOUT_ZLIB_DLL_WITH_LIB ZLIB::ZLIBSTATIC CACHE INTERNAL "" FORCE)
endif()
FetchContent_Declare(libarchive
    GIT_REPOSITORY https://github.com/libarchive/libarchive.git
    GIT_TAG v3.8.10
    GIT_SHALLOW TRUE)
# (libarchive refuses an empty build type, which a multi-config generator such as Visual Studio's
# leaves: it is given one for its own configuring, then put back.)
set(SIEVE_SAVED_BUILD_TYPE "${CMAKE_BUILD_TYPE}")
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
FetchContent_MakeAvailable(libarchive)
sieve_pinned(libarchive "${libarchive_SOURCE_DIR}" 0c48412be2d94afe1859a807ea7896a18d8900bd) # v3.8.10
set(CMAKE_BUILD_TYPE "${SIEVE_SAVED_BUILD_TYPE}")
set(BUILD_SHARED_LIBS ${SIEVE_SAVED_BUILD_SHARED_LIBS})
set(SIEVE_ARCHIVE archive_static)
set(SIEVE_ARCHIVE_INCLUDE ${libarchive_SOURCE_DIR}/libarchive)
# preflate (Apache 2.0, third_party_licenses/preflate), Dirk Steinke's: any deflate stream (a zip's,
# a gzip's, whoever made it) split into what it holds and a small difference from its own deflate,
# and made again exactly from the two. Pinned to its last commit (it has no releases), its library's
# files built here unchanged but for two guards (below); two headers its files assume are given them
# on the command line, as newer compilers no longer bring them in.
FetchContent_Declare(preflate
    GIT_REPOSITORY https://github.com/deus-libri/preflate.git
    GIT_TAG 609eefaa96ac6c51d7b1a3fb29e0ed94d0f3623e
    SOURCE_SUBDIR support) # (no CMakeLists.txt there: fetched, not built by its own CMake)
FetchContent_MakeAvailable(preflate)
# Two changes to preflate. The first (Edward, 7 November 2026; results/fuzz-2026-11-06.md):
# ArithmeticDecoder::decodeBits shifts by a count read from the instructions, so damaged ones can ask
# for 32 bits, a shift C++ leaves undefined, and a count of 31 or 32 can leave the step 0, which then
# divides. Valid streams never ask for more than about 16 bits (deflate's lengths and distances).
# Patched, such a count gives 0 and the install goes on to refuse the damaged stream as before.
# Applied once to the fetched copy, and checked: configuring stops if the text is not there.
set(SIEVE_PREFLATE_CODER ${preflate_SOURCE_DIR}/support/arithmetic_coder.h)
file(READ ${SIEVE_PREFLATE_CODER} SIEVE_PREFLATE_TEXT)
string(REPLACE "\r\n" "\n" SIEVE_PREFLATE_TEXT "${SIEVE_PREFLATE_TEXT}") # (a Windows checkout's line ends)
string(FIND "${SIEVE_PREFLATE_TEXT}" "SIEVE_PATCHED" SIEVE_PREFLATE_DONE)
if(SIEVE_PREFLATE_DONE EQUAL -1)
    set(SIEVE_PREFLATE_OLD "  unsigned decodeBits(const uint32_t bits) {\n    uint32_t step = ((_high - _low) + 1) >> bits;\n")
    set(SIEVE_PREFLATE_NEW "  unsigned decodeBits(const uint32_t bits) {\n    if (bits >= 32) return 0; // SIEVE_PATCHED: damaged input only\n    uint32_t step = ((_high - _low) + 1) >> bits;\n    if (step == 0) return 0; // SIEVE_PATCHED\n")
    string(FIND "${SIEVE_PREFLATE_TEXT}" "${SIEVE_PREFLATE_OLD}" SIEVE_PREFLATE_AT)
    if(SIEVE_PREFLATE_AT EQUAL -1)
        message(FATAL_ERROR "preflate's decodeBits is not as Sieve's patch expects (${SIEVE_PREFLATE_CODER})")
    endif()
    string(REPLACE "${SIEVE_PREFLATE_OLD}" "${SIEVE_PREFLATE_NEW}" SIEVE_PREFLATE_TEXT "${SIEVE_PREFLATE_TEXT}")
    file(WRITE ${SIEVE_PREFLATE_CODER} "${SIEVE_PREFLATE_TEXT}")
endif()
# A second guard (8 November 2026, the reader fuzzed with a file whose zlib streams preflate opened,
# results/zlib-pcx-2026-11-08.md): preflate's token predictor trusts the instructions to stay within
# what the stream holds: a stored block's length, a block's count of tokens and a corrected reference's
# length and distance are read from them and never weighed against the bytes left (nor a corrected
# length against deflate's 3 to 258: one of 0 had its matching compare 4 GiB past the buffer), so
# damaged ones moved its cursor past the end and its matching read outside the buffer (a crash, under
# AddressSanitizer, in prefixCompare). Patched, each is checked and a damaged stream fails, as one
# whose prediction fails does: valid streams never ask for more than they hold.
set(SIEVE_PREFLATE_TOKENS ${preflate_SOURCE_DIR}/preflate_token_predictor.cpp)
file(READ ${SIEVE_PREFLATE_TOKENS} SIEVE_PREFLATE_TEXT)
string(REPLACE "\r\n" "\n" SIEVE_PREFLATE_TEXT "${SIEVE_PREFLATE_TEXT}")
string(FIND "${SIEVE_PREFLATE_TEXT}" "SIEVE_PATCHED" SIEVE_PREFLATE_DONE)
if(SIEVE_PREFLATE_DONE EQUAL -1)
    # (in its decoding alone, decodeBlock to the end of the file: each text as preflate has it there,
    # once, and what it becomes; configuring stops if one is not there)
    string(FIND "${SIEVE_PREFLATE_TEXT}" "PreflateTokenBlock PreflateTokenPredictor::decodeBlock(" SIEVE_PREFLATE_FROM)
    if(SIEVE_PREFLATE_FROM EQUAL -1)
        message(FATAL_ERROR "preflate's token predictor is not as Sieve's patch expects (${SIEVE_PREFLATE_TOKENS})")
    endif()
    string(SUBSTRING "${SIEVE_PREFLATE_TEXT}" 0 ${SIEVE_PREFLATE_FROM} SIEVE_PREFLATE_HEAD)
    string(SUBSTRING "${SIEVE_PREFLATE_TEXT}" ${SIEVE_PREFLATE_FROM} -1 SIEVE_PREFLATE_TEXT)
    macro(sieve_preflate_guard old new)
        string(FIND "${SIEVE_PREFLATE_TEXT}" "${old}" SIEVE_PREFLATE_AT)
        string(FIND "${SIEVE_PREFLATE_TEXT}" "${old}" SIEVE_PREFLATE_AT_LAST REVERSE)
        if(SIEVE_PREFLATE_AT EQUAL -1 OR NOT SIEVE_PREFLATE_AT EQUAL SIEVE_PREFLATE_AT_LAST)
            message(FATAL_ERROR "preflate's token predictor is not as Sieve's patch expects (${SIEVE_PREFLATE_TOKENS})")
        endif()
        string(REPLACE "${old}" "${new}" SIEVE_PREFLATE_TEXT "${SIEVE_PREFLATE_TEXT}")
    endmacro()
    sieve_preflate_guard([=[
    hash.updateHash(block.uncompressedLen);
]=] [=[
    if (block.uncompressedLen > state.availableInputSize()) { predictionFailure = true; return PreflateTokenBlock(); } // SIEVE_PATCHED
    hash.updateHash(block.uncompressedLen);
]=])
    sieve_preflate_guard([=[
    PreflateToken predictedToken = predictToken();
]=] [=[
    if (state.availableInputSize() == 0) { predictionFailure = true; return PreflateTokenBlock(); } // SIEVE_PATCHED
    PreflateToken predictedToken = predictToken();
]=])
    sieve_preflate_guard([=[
      predictedToken.len = newLen;
]=] [=[
      if (newLen < 3 || newLen > 258) { predictionFailure = true; return PreflateTokenBlock(); } // SIEVE_PATCHED
      predictedToken.len = newLen;
]=])
    sieve_preflate_guard([=[
    if (predictedToken.len == 258) {
]=] [=[
    if (predictedToken.len > state.availableInputSize() || predictedToken.dist > state.currentInputPos()) { predictionFailure = true; return PreflateTokenBlock(); } // SIEVE_PATCHED
    if (predictedToken.len == 258) {
]=])
    file(WRITE ${SIEVE_PREFLATE_TOKENS} "${SIEVE_PREFLATE_HEAD}${SIEVE_PREFLATE_TEXT}")
endif()
set(SIEVE_PREFLATE_SOURCES)
foreach(f block_decoder block_reencoder block_trees complevel_estimator constants decoder hash_chain info parameter_estimator
          parser_config predictor_state reencoder seq_chain statistical_codec statistical_model token token_predictor tree_predictor)
    list(APPEND SIEVE_PREFLATE_SOURCES ${preflate_SOURCE_DIR}/preflate_${f}.cpp)
endforeach()
foreach(f arithmetic_coder array_helper bit_helper bitstream const_division huffman_decoder huffman_encoder huffman_helper memstream
          outputcachestream task_pool)
    list(APPEND SIEVE_PREFLATE_SOURCES ${preflate_SOURCE_DIR}/support/${f}.cpp)
endforeach()
# (C++17, and seen by the rest of Sieve only through tools/cli/preflate_glue.hpp: its headers use
# std::result_of, which C++20 removed.)
add_library(sieve_preflate STATIC ${SIEVE_PREFLATE_SOURCES} tools/cli/preflate_glue.cpp)
set_target_properties(sieve_preflate PROPERTIES CXX_STANDARD 17)
target_include_directories(sieve_preflate SYSTEM PRIVATE ${preflate_SOURCE_DIR})
target_include_directories(sieve_preflate PRIVATE tools)
find_package(Threads REQUIRED)
target_link_libraries(sieve_preflate PUBLIC Threads::Threads)
if(MSVC)
    target_compile_options(sieve_preflate PRIVATE /w /FIcstdint /FIcstring /FIcstdio)
else()
    target_compile_options(sieve_preflate PRIVATE -w "SHELL:-include cstdint" "SHELL:-include cstring" "SHELL:-include cstdio")
endif()
# brunsli (MIT, third_party_licenses/brunsli), Google's: a JPEG's own coded values recoded by a model
# made for them and given back byte for byte (v32 in the making, `jbr`; docs/NEXT-VERSION.md). Pinned
# to the commit measured (30 October 2026: 15.25% to 21.91% off ten original JPEGs, each exact); its
# library's files built here, against Sieve's own brotli, not by its own CMake (which fetches
# brotli again). Seen by the rest of Sieve only through tools/cli/jpeg_glue.hpp.
FetchContent_Declare(brunsli
    GIT_REPOSITORY https://github.com/google/brunsli.git
    GIT_TAG c18b9625ef045b0b89ebb4439ae2339e2160df71
    SOURCE_SUBDIR c/include) # (no CMakeLists.txt there: fetched, not built by its own CMake)
FetchContent_MakeAvailable(brunsli)
set(SIEVE_BRUNSLI_SOURCES)
foreach(f ans_params constants context lehmer_code platform predict quant_matrix)
    list(APPEND SIEVE_BRUNSLI_SOURCES ${brunsli_SOURCE_DIR}/c/common/${f}.cc)
endforeach()
foreach(f ans_decode bit_reader brunsli_decode context_map_decode histogram_decode huffman_decode huffman_table jpeg_data_writer state)
    list(APPEND SIEVE_BRUNSLI_SOURCES ${brunsli_SOURCE_DIR}/c/dec/${f}.cc)
endforeach()
foreach(f ans_encode brunsli_encode context_map_encode histogram_encode huffman_encode huffman_tree jpeg_data_reader jpeg_huffman_decode write_bits)
    list(APPEND SIEVE_BRUNSLI_SOURCES ${brunsli_SOURCE_DIR}/c/enc/${f}.cc)
endforeach()
# (Its export header, which its own CMake would generate: a static library exports nothing.)
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/brunsli_export/brunsli/brunsli_export.h "#pragma once\n#define BRUNSLI_EXPORT\n")
add_library(sieve_brunsli STATIC ${SIEVE_BRUNSLI_SOURCES} tools/cli/jpeg_glue.cpp)
target_include_directories(sieve_brunsli SYSTEM PRIVATE ${brunsli_SOURCE_DIR}/c/include ${brunsli_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}/brunsli_export)
target_include_directories(sieve_brunsli PRIVATE tools)
target_link_libraries(sieve_brunsli PRIVATE sieve_brotli)
if(MSVC)
    target_compile_options(sieve_brunsli PRIVATE /w)
else()
    target_compile_options(sieve_brunsli PRIVATE -w)
endif()
# packMP3 (Matthias Stirner, LGPL v3, third_party_licenses/packmp3): an MP3's Huffman-coded data
# recoded by a model made for it and given back byte for byte (v34 draft, `pmp`). Its source is kept in
# third_party/packmp3 (its last commit, e61c119, 27 April 2020), changed by Sieve where its decoder
# trusted what it read (each change marked "Sieve"; results/mp3-pmp-2026-11-08.md), built as a library
# (BUILD_LIB). Seen by the rest of Sieve only through tools/cli/mp3_glue.hpp.
add_library(sieve_packmp3 STATIC third_party/packmp3/packmp3.cpp third_party/packmp3/huffmp3.cpp third_party/packmp3/aricoder.cpp
            third_party/packmp3/bitops.cpp tools/cli/mp3_glue.cpp)
target_compile_definitions(sieve_packmp3 PRIVATE BUILD_LIB _CRT_SECURE_NO_WARNINGS)
target_include_directories(sieve_packmp3 PRIVATE third_party/packmp3 tools)
if(MSVC)
    target_compile_options(sieve_packmp3 PRIVATE /w)
else()
    target_compile_options(sieve_packmp3 PRIVATE -w)
endif()
# libFLAC 1.4.3 (BSD 3-clause, third_party_licenses/flac): v34 draft's "flac", a WAV's samples carried
# as FLAC codes them and made again byte for byte. Its library alone (no programs, C++ library,
# Ogg, examples, tests or documents), static, by its own CMake; its other targets are left out of
# the build. (8 November 2026: on WoWRevived's seven WAVs, 81.2% of the rope's code.)
set(SIEVE_SAVED_BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS})
set(BUILD_SHARED_LIBS OFF)
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW) # (its option()s take the values set here)
foreach(v BUILD_CXXLIBS BUILD_PROGRAMS BUILD_EXAMPLES BUILD_TESTING BUILD_DOCS INSTALL_MANPAGES WITH_OGG
          INSTALL_PKGCONFIG_MODULES INSTALL_CMAKE_CONFIG_MODULE)
    set(${v} OFF CACHE BOOL "" FORCE)
endforeach()
FetchContent_Declare(flac
    GIT_REPOSITORY https://github.com/xiph/flac.git
    GIT_TAG 1.4.3
    GIT_SHALLOW TRUE)
FetchContent_GetProperties(flac)
if(NOT flac_POPULATED)
    FetchContent_Populate(flac)
    sieve_pinned(flac "${flac_SOURCE_DIR}" 28e4f0528c76b296c561e922ba67d43751990599) # 1.4.3
    # (libarchive's ENABLE_WERROR, ON in a Debug build, would reach libFLAC's CMake too, which then adds
    # GCC's -Werror, an option MSVC refuses: "D8021 invalid numeric argument '/Werror'", Edward's
    # x64-Debug, 8 November 2026. Off for libFLAC alone; libarchive is configured already.)
    set(SIEVE_SAVED_ENABLE_WERROR ${ENABLE_WERROR})
    set(ENABLE_WERROR OFF)
    add_subdirectory(${flac_SOURCE_DIR} ${flac_BINARY_DIR} EXCLUDE_FROM_ALL)
    set(ENABLE_WERROR ${SIEVE_SAVED_ENABLE_WERROR})
endif()
set(BUILD_SHARED_LIBS ${SIEVE_SAVED_BUILD_SHARED_LIBS})
foreach(t sieve_lz4 sieve_brotli)
    if(MSVC)
        target_compile_options(${t} PRIVATE /w)
    else()
        target_compile_options(${t} PRIVATE -w)
    endif()
endforeach()

# The locator and the installer's code (decoders only, the LZMA SDK's: sieve-install links this
# alone), and the comparison and the packer, which alone need the compressors.
# Sieve's icon as a Windows resource (data/icons, made by tools/make_icon.py), for every program.
if(WIN32)
    set(SIEVE_ICON_FILE ${CMAKE_CURRENT_SOURCE_DIR}/data/icons/sieve.ico)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/data/icons/sieve.rc.in ${CMAKE_BINARY_DIR}/sieve_icon.rc @ONLY)
    set(SIEVE_ICON_RC ${CMAKE_BINARY_DIR}/sieve_icon.rc)
else()
    set(SIEVE_ICON_RC "")
endif()

# PDQ, the perceptual hash the vault matches pictures with: Meta's reference code (third_party/pdq,
# BSD licence) and Sieve's wrapper over it (tools/cli/pdq_hash.*), compiled together without the
# core's strict warning flags, as stb and the LZMA SDK are; the wrapper's header shows none of it.
add_library(sieve_pdq STATIC
    tools/cli/pdq_hash.cpp
    third_party/pdq/pdq/cpp/common/pdqhashtypes.cpp
    third_party/pdq/pdq/cpp/downscaling/downscaling.cpp
    third_party/pdq/pdq/cpp/hashing/pdqhashing.cpp
    third_party/pdq/pdq/cpp/hashing/torben.cpp)
target_include_directories(sieve_pdq SYSTEM PRIVATE third_party/pdq)
target_compile_features(sieve_pdq PRIVATE cxx_std_20)
if(MSVC)
    # PDQ's own code, as Meta wrote it, narrows doubles to floats and uses sscanf: MSVC's notes on
    # those (C4244, C4267, C4305, C4996) are about upstream code, which is kept unedited.
    target_compile_options(sieve_pdq PRIVATE /W3 /utf-8 /wd4244 /wd4267 /wd4305 /wd4996)
    target_compile_definitions(sieve_pdq PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

# The LZMA SDK's decoders (third_party/lzma, public domain): LZMA2 and the x86 branch filter, with
# which the locator's install code unpacks a packed installer (sieve-manifest-v4, tools/cli/
# locate.*), and the 7z decoder sieve-install unpacks an installer that carries one 7z archive with
# (client/unpack_7z.hpp). One decoder for every program that installs, the smallest included. C,
# so its own language; its warnings are the SDK's, at each compiler's usual level.
enable_language(C)
add_library(sieve_lzma STATIC
    third_party/lzma/7zAlloc.c third_party/lzma/7zArcIn.c third_party/lzma/7zBuf.c third_party/lzma/7zBuf2.c
    third_party/lzma/7zCrc.c third_party/lzma/7zCrcOpt.c third_party/lzma/7zDec.c third_party/lzma/7zStream.c
    third_party/lzma/Bcj2.c third_party/lzma/Bra.c third_party/lzma/Bra86.c third_party/lzma/BraIA64.c
    third_party/lzma/CpuArch.c third_party/lzma/Delta.c third_party/lzma/Lzma2Dec.c third_party/lzma/LzmaDec.c
    third_party/lzma/Ppmd7.c third_party/lzma/Ppmd7Dec.c)
target_include_directories(sieve_lzma PUBLIC third_party/lzma)
if(MSVC)
    target_compile_options(sieve_lzma PRIVATE /W3)
endif()

# The LZMA SDK's LZMA2 encoder (third_party/lzma, public domain), single-threaded (Z7_ST): one more
# way the packer tries for each stream beside liblzma's, keeping the smaller (tools/cli/pack.cpp).
# A library of its own, so only the packer links it; the installers do not.
add_library(sieve_lzma_enc STATIC
    third_party/lzma/LzmaEnc.c third_party/lzma/Lzma2Enc.c third_party/lzma/LzFind.c third_party/lzma/LzFindOpt.c)
target_include_directories(sieve_lzma_enc PUBLIC third_party/lzma)
target_compile_definitions(sieve_lzma_enc PUBLIC Z7_ST)
target_link_libraries(sieve_lzma_enc PUBLIC sieve_lzma)
if(MSVC)
    target_compile_options(sieve_lzma_enc PRIVATE /W3)
endif()

add_library(sieve_locate STATIC tools/cli/locate.cpp tools/cli/case_names.cpp tools/cli/gather.cpp tools/cli/delta.cpp tools/cli/text_spans.cpp tools/cli/map.cpp tools/cli/vault.cpp tools/cli/vault_decode.cpp tools/cli/timings.cpp
    tools/cli/rar_remake.cpp)
target_include_directories(sieve_locate PUBLIC tools)
# The vault checks pictures (PDQ), including files that are pictures, so it reads them (stb).
target_link_libraries(sieve_locate PUBLIC sieve_core PRIVATE sieve_pdq sieve_image_io sieve_lzma)
target_link_libraries(sieve_lines PUBLIC sieve_locate) # the lines check units against the vault
# (client/unpack_7z.cpp too: "Optimise Archive Formats" reads a 7z's contents with the installer's
# own decoder.)
add_library(sieve_compare STATIC tools/cli/compare.cpp tools/cli/compress.cpp tools/cli/doc_text.cpp tools/cli/pack.cpp tools/cli/png_resave.cpp tools/cli/gif_lzw.cpp tools/cli/flac_wav.cpp tools/cli/voice.cpp tools/cli/lossless_sound.cpp tools/cli/tiff_codec.cpp tools/cli/pcx_rle.cpp tools/cli/train.cpp tools/cli/diffusion_run.cpp tools/cli/model_folder.cpp
    client/unpack_7z.cpp)
target_include_directories(sieve_compare PUBLIC tools PRIVATE client)
target_link_libraries(sieve_compare PUBLIC sieve_core sieve_locate PRIVATE sieve_image_io ${SIEVE_ZLIB} ${SIEVE_LZMA} sieve_lzma sieve_lzma_enc sieve_bzip2 ${SIEVE_ZSTD} sieve_lz4 sieve_brotli ${SIEVE_ARCHIVE} sieve_preflate sieve_brunsli sieve_packmp3 FLAC sieve_lines)
target_include_directories(sieve_compare SYSTEM PRIVATE ${SIEVE_ZSTD_INCLUDE} ${SIEVE_ARCHIVE_INCLUDE})
target_compile_definitions(sieve_compare PRIVATE LIBARCHIVE_STATIC)
if(SIEVE_LZMA_INCLUDE)
    target_include_directories(sieve_compare SYSTEM PRIVATE ${SIEVE_LZMA_INCLUDE})
endif()

add_executable(sieve
    tools/sieve_cli.cpp
    tools/cli/help.cpp
    tools/cli/tensors.cpp
    tools/cli/rope.cpp
    tools/cli/chat.cpp
    tools/cli/ai.cpp
    tools/cli/train_cmd.cpp
    tools/cli/diffuse.cpp
    tools/cli/cipher_cmd.cpp
    tools/cli/site.cpp
    ${SIEVE_ICON_RC})
target_link_libraries(sieve PRIVATE sieve_lines sieve_locate sieve_compare)
if(WIN32)
    # UTF-8 command lines on Windows (the manifest sets the process code page; before Windows 10
    # 1903, which ignores it, main reads the command line again in UTF-16: shell32's
    # CommandLineToArgvW).
    target_sources(sieve PRIVATE tools/utf8.manifest)
    target_link_libraries(sieve PRIVATE shell32)
endif()
target_compile_definitions(sieve PRIVATE SIEVE_VERSION="${PROJECT_VERSION}")

# Copy the bundled dictionaries and models next to the executable, so they work from any
# folder and in every configuration (Debug, Release, ...).
# Runs on every build (not only when sieve relinks), so a changed registry or word list always
# reaches the executable's folder.
# Potentia's licence, which covers Sieve, is kept one folder up (the Potentia repository's root):
# copied beside the programs as potentia-license.txt, so every build and every release carries it.
set(SIEVE_POTENTIA_LICENSE ${CMAKE_CURRENT_SOURCE_DIR}/../LICENSE)
set(SIEVE_LICENSE_COPY "")
if(EXISTS ${SIEVE_POTENTIA_LICENSE})
    set(SIEVE_LICENSE_COPY COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SIEVE_POTENTIA_LICENSE}
                           $<TARGET_FILE_DIR:sieve>/potentia-license.txt)
else()
    message(WARNING "No LICENSE one folder up (${SIEVE_POTENTIA_LICENSE}): the build carries no potentia-license.txt, and a release must not be made from it")
endif()
add_custom_target(sieve_data ALL
    ${SIEVE_LICENSE_COPY}
    # The maps folder, where the node graph (O) lists maps and keeps the ones made in the hallway.
    # No map is shipped in it: the release's map names the release's files, so it cannot be inside
    # them, and "This installation" is made fresh. Maps already in it are left as they are.
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:sieve>/maps
    # The vault's files (docs/VAULT.md): hashes only; the built-in entries are compiled in as well.
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/data/vault
            $<TARGET_FILE_DIR:sieve>/vault
    # Filter plugins (docs/FILTER-PLUGINS.md): the reference plugins; add your own beside them.
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/data/filters
            $<TARGET_FILE_DIR:sieve>/filters
    # The AI Training Harness's runs (SPECIFICATIONS §12.0e): a recorded run to train again and
    # check, bit for bit; runs recorded in the hallway are kept beside it.
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/data/training
            $<TARGET_FILE_DIR:sieve>/training
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/data/dictionaries
            $<TARGET_FILE_DIR:sieve>/dictionaries
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/data/models
            $<TARGET_FILE_DIR:sieve>/models
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/data/lang
            $<TARGET_FILE_DIR:sieve>/lang
    # Sieve's voice (tools/cli/voice.hpp): its pronunciations and alignment; its clips are found in
    # the anchors (data/anchors/public-domain/voice), above a build inside the repository.
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/data/voice
            $<TARGET_FILE_DIR:sieve>/voice
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/third_party_licenses
            $<TARGET_FILE_DIR:sieve>/third_party_licenses
    COMMENT "Copying the licences, maps, dictionaries, models, training runs and languages next to sieve")
add_dependencies(sieve_data sieve)

# ---------------------------------------------------------------- tests
if(SIEVE_BUILD_TESTS)
    enable_testing()
    add_executable(sieve_tests tests/test_core.cpp)
    target_link_libraries(sieve_tests PRIVATE sieve_core)
    add_test(NAME core COMMAND sieve_tests ${CMAKE_CURRENT_SOURCE_DIR}/tests)
    # packMP3's decoder given damaged forms of an MP3 (tests/test_mp3.cpp): none may end the program.
    add_executable(sieve_mp3_check tests/test_mp3.cpp)
    target_include_directories(sieve_mp3_check PRIVATE tools)
    target_link_libraries(sieve_mp3_check PRIVATE sieve_packmp3)
    add_test(NAME mp3 COMMAND sieve_mp3_check ${CMAKE_CURRENT_SOURCE_DIR}/tests)
    # The corridor at the two ends of its tile size (2 and 4096): a program of its own, since the
    # size is latched by the first line built and sieve_tests builds them at 128.
    add_executable(sieve_tile_tests tests/test_tiles.cpp)
    target_link_libraries(sieve_tile_tests PRIVATE sieve_core)
    add_test(NAME tiles_2 COMMAND sieve_tile_tests 2)
    add_test(NAME tiles_4096 COMMAND sieve_tile_tests 4096)
endif()

# ---------------------------------------------------------------- hallway client (SDL3)
# Uses an installed SDL3 (vcpkg with the "client" feature, or the system) if one is found;
# otherwise downloads and builds SDL 3.2.30 as a static library the first time you configure.
if(SIEVE_BUILD_CLIENT)
    find_package(SDL3 CONFIG QUIET)
    if(NOT SDL3_FOUND)
        message(STATUS "SDL3 not found: fetching SDL 3.2.30 (first configure only, takes a few minutes)")
        include(FetchContent)
        set(SDL_SHARED OFF CACHE BOOL "" FORCE)
        set(SDL_STATIC ON CACHE BOOL "" FORCE)
        set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
        set(SDL_TESTS OFF CACHE BOOL "" FORCE)
        set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
        FetchContent_Declare(SDL3
            GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
            GIT_TAG release-3.2.30
            GIT_SHALLOW TRUE)
        FetchContent_MakeAvailable(SDL3)
        sieve_pinned(SDL3 "${sdl3_SOURCE_DIR}" f5e5f6588921eed3d7d048ce43d9eb1ff0da0ffc) # release-3.2.30
    endif()

    # WIN32: a Windows program, so no console window opens behind it (app_main.cpp attaches to
    # the console of a command prompt it was started from).
    add_executable(hallway WIN32
        ${SIEVE_ICON_RC}
        client/app_main.cpp
        client/app_settings.cpp
        client/binary_edge.cpp
        client/designer.cpp
        client/cipher_screen.cpp
        client/manual_nav.cpp
        client/harness.cpp
        client/designer_model.cpp
        client/item_faces.cpp
        client/door_portal.cpp
        client/file_locator.cpp
        client/node_graph.cpp
        client/item_save.cpp
        client/tailoring.cpp
        client/navigator.cpp
        client/pause_menu.cpp
        client/speech.cpp
        client/font.cpp
        client/gpu_memory.cpp
        client/hallway.cpp
        client/hud.cpp
        client/main_menu.cpp
        client/menu.cpp
        client/media_player.cpp
        client/chain.cpp
        client/chat.cpp
        client/viewer.cpp
        client/mesh.cpp
        client/music.cpp
        client/strings.cpp
        client/synth.cpp
        client/win_console.cpp
        client/window_front.cpp
        client/world.cpp)
    target_link_libraries(hallway PRIVATE sieve_lines sieve_locate sieve_compare SDL3::SDL3)
    if(WIN32) # (Explorer's folder picker, window_front.cpp: COM; MSVC also asks by #pragma)
        target_link_libraries(hallway PRIVATE ole32 shell32 uuid)
    endif()
    if(WIN32)
        target_sources(hallway PRIVATE tools/utf8.manifest)
    endif()
    add_custom_command(TARGET hallway POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${CMAKE_CURRENT_SOURCE_DIR}/data/dictionaries
                $<TARGET_FILE_DIR:hallway>/dictionaries
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${CMAKE_CURRENT_SOURCE_DIR}/data/models
                $<TARGET_FILE_DIR:hallway>/models
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${CMAKE_CURRENT_SOURCE_DIR}/data/lang
                $<TARGET_FILE_DIR:hallway>/lang
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${CMAKE_CURRENT_SOURCE_DIR}/data/prompts
                $<TARGET_FILE_DIR:hallway>/prompts
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${CMAKE_CURRENT_SOURCE_DIR}/data/fonts
                $<TARGET_FILE_DIR:hallway>/fonts
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${CMAKE_CURRENT_SOURCE_DIR}/data/meshes
                $<TARGET_FILE_DIR:hallway>/meshes
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${CMAKE_CURRENT_SOURCE_DIR}/third_party_licenses
                $<TARGET_FILE_DIR:hallway>/third_party_licenses)
    target_compile_definitions(hallway PRIVATE SIEVE_VERSION="${PROJECT_VERSION}")

    # sieve-install: the installer, a window of its own over the locator's install code. It needs
    # no data files (SDL's own font), so it is one executable, and an installer program is it with
    # the installer attached. SDL is linked statically where a static SDL is to be had, so that
    # program is one file; with only a shared SDL it also needs SDL3.dll beside it (sieve locate
    # --program says so). On Windows it is a windowed program, without a console.
    add_executable(sieve-install WIN32 client/installer_main.cpp client/unpack_7z.cpp client/window_front.cpp client/win_console.cpp ${SIEVE_ICON_RC})
    # The full installer: it makes again the files carried as items of Sieve's lines and remakes
    # archives exactly, as sieve install does (cli/items.cpp, use_item_maker), so an installer
    # program, or a .sieve opened from the right-click menu, installs anything Sieve makes.
    target_link_libraries(sieve-install PRIVATE sieve_lzma sieve_lines sieve_compare)
    if(WIN32) # (Explorer's folder picker, as the hallway's)
        target_link_libraries(sieve-install PRIVATE ole32 shell32 uuid)
    endif()
    if(SIEVE_SMALL_INSTALLER)
        # Its own SDL, built once from the same source (or fetched, when SDL came from vcpkg or
        # the system), static, optimised for size, and without what the installer never uses:
        # sound, controllers, haptics, sensors, the camera, power, the GPU API, Vulkan and
        # OpenGL. It keeps a window, the 2D renderer and the folder dialog. Every copy of the
        # installer program carries this, so it is worth the minute it takes to build: on Linux
        # 1.5 MB against 3.7 MB. A Debug build gets a Debug SDL, so the runtimes match.
        include(ExternalProject)
        set(SIEVE_SDL_SMALL ${CMAKE_BINARY_DIR}/sdl3-small)
        if(CMAKE_BUILD_TYPE STREQUAL "Debug")
            set(SIEVE_SDL_SMALL_TYPE Debug)
        else()
            set(SIEVE_SDL_SMALL_TYPE MinSizeRel)
        endif()
        if(MSVC)
            set(SIEVE_SDL_SMALL_LIB ${SIEVE_SDL_SMALL}/lib/SDL3-static.lib)
            set(SIEVE_SDL_SMALL_CFLAGS "")
        else()
            set(SIEVE_SDL_SMALL_LIB ${SIEVE_SDL_SMALL}/lib/libSDL3.a)
            set(SIEVE_SDL_SMALL_CFLAGS "-ffunction-sections -fdata-sections")
        endif()
        if(DEFINED SDL3_SOURCE_DIR AND EXISTS "${SDL3_SOURCE_DIR}/CMakeLists.txt")
            set(SIEVE_SDL_SMALL_FROM SOURCE_DIR ${SDL3_SOURCE_DIR})
        else()
            set(SIEVE_SDL_SMALL_FROM GIT_REPOSITORY https://github.com/libsdl-org/SDL.git GIT_TAG release-3.2.30 GIT_SHALLOW TRUE)
        endif()
        ExternalProject_Add(sdl3_small
            ${SIEVE_SDL_SMALL_FROM}
            BINARY_DIR ${CMAKE_BINARY_DIR}/sdl3-small-build
            INSTALL_DIR ${SIEVE_SDL_SMALL}
            CMAKE_ARGS
                -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
                -DCMAKE_BUILD_TYPE=${SIEVE_SDL_SMALL_TYPE}
                -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
                -DCMAKE_C_FLAGS=${SIEVE_SDL_SMALL_CFLAGS}
                -DCMAKE_POSITION_INDEPENDENT_CODE=ON
                -DSDL_SHARED=OFF -DSDL_STATIC=ON -DSDL_TEST_LIBRARY=OFF -DSDL_TESTS=OFF -DSDL_EXAMPLES=OFF
                -DSDL_AUDIO=OFF -DSDL_GPU=OFF -DSDL_CAMERA=OFF -DSDL_JOYSTICK=OFF -DSDL_HAPTIC=OFF
                -DSDL_HIDAPI=OFF -DSDL_POWER=OFF -DSDL_SENSOR=OFF
                -DSDL_VULKAN=OFF -DSDL_RENDER_VULKAN=OFF -DSDL_RENDER_GPU=OFF -DSDL_OPENGL=OFF -DSDL_OPENGLES=OFF
                -DSDL_RENDER_D3D12=OFF -DSDL_OPENVR=OFF -DSDL_KMSDRM=OFF
                -DSDL_DUMMYCAMERA=OFF -DSDL_DISKAUDIO=OFF -DSDL_DUMMYAUDIO=OFF
            BUILD_BYPRODUCTS ${SIEVE_SDL_SMALL_LIB})
        file(MAKE_DIRECTORY ${SIEVE_SDL_SMALL}/include)
        add_library(sieve_sdl_small STATIC IMPORTED)
        set_target_properties(sieve_sdl_small PROPERTIES
            IMPORTED_LOCATION ${SIEVE_SDL_SMALL_LIB}
            INTERFACE_INCLUDE_DIRECTORIES ${SIEVE_SDL_SMALL}/include)
        if(WIN32)
            set_property(TARGET sieve_sdl_small PROPERTY INTERFACE_LINK_LIBRARIES
                kernel32 user32 gdi32 winmm imm32 ole32 oleaut32 version uuid advapi32 setupapi shell32)
        elseif(APPLE)
            # SDL's static library on a Mac is Objective-C on Apple's frameworks, and a static
            # library cannot name them itself (SDL's own CMake target does; this one is made by
            # hand). Each is part of every Mac; the ones nothing calls are dropped at link time
            # (-dead_strip_dylibs, with the installer's other link options below).
            set_property(TARGET sieve_sdl_small PROPERTY INTERFACE_LINK_LIBRARIES
                objc iconv m Threads::Threads
                "-framework Cocoa" "-framework Foundation" "-framework AppKit" "-framework CoreFoundation"
                "-framework CoreGraphics" "-framework CoreServices" "-framework CoreVideo" "-framework IOKit"
                "-framework Carbon" "-framework Metal" "-framework QuartzCore" "-framework UniformTypeIdentifiers"
                "-framework ForceFeedback" "-framework GameController" "-framework CoreHaptics"
                "-framework AudioToolbox" "-framework CoreAudio" "-framework AVFoundation" "-framework CoreMedia")
        else()
            set_property(TARGET sieve_sdl_small PROPERTY INTERFACE_LINK_LIBRARIES m ${CMAKE_DL_LIBS} Threads::Threads)
        endif()
        add_dependencies(sieve-install sdl3_small)
        target_link_libraries(sieve-install PRIVATE sieve_locate sieve_sdl_small)
        # The installer's own code small too, and what nothing calls left out.
        if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
            if(MSVC)
                # (Not /O1: it overrides Release's /O2 with a warning. SDL is where the size is.)
                target_link_options(sieve-install PRIVATE /OPT:REF /OPT:ICF)
            else()
                target_compile_options(sieve-install PRIVATE -Os)
                if(APPLE)
                    # Apple's linker has its own name for this, and strips with strip(1), not -s.
                    target_link_options(sieve-install PRIVATE -Wl,-dead_strip -Wl,-dead_strip_dylibs)
                else()
                    target_link_options(sieve-install PRIVATE -Wl,--gc-sections -s)
                endif()
            endif()
        endif()
    elseif(TARGET SDL3::SDL3-static)
        target_link_libraries(sieve-install PRIVATE sieve_locate SDL3::SDL3-static)
    else()
        target_link_libraries(sieve-install PRIVATE sieve_locate SDL3::SDL3)
    endif()
    if(WIN32)
        target_sources(sieve-install PRIVATE tools/utf8.manifest)
    endif()

    # sieve-view: the picture viewer (2 November 2026), a window of its own over the installer's
    # code (a .sieve's pictures installed into a folder of its own and shown) and stb_image's
    # decoders. Like sieve-install it needs no data files; it is linked with the hallway's SDL. Its
    # "Sieve" button runs the sieve program beside it.
    add_executable(sieve-view WIN32 client/view_main.cpp client/picture_source.cpp client/thumbs_check.cpp client/window_front.cpp client/win_console.cpp ${SIEVE_ICON_RC})
    target_link_libraries(sieve-view PRIVATE sieve_lzma sieve_lines sieve_compare sieve_locate sieve_image_io SDL3::SDL3)
    if(WIN32)
        target_link_libraries(sieve-view PRIVATE ole32 shell32 shlwapi uuid)
        target_sources(sieve-view PRIVATE tools/utf8.manifest)
        # sieve-thumbs.dll (5 November 2026): a .simage's picture as its thumbnail in Explorer, read
        # as sieve-view reads it (client/picture_source.cpp). No SDL, no window: Explorer loads it in
        # a process of its own (dllhost) and asks it for a bitmap. Registered for the person by
        # context/install_context.bat, beside sieve-view.
        add_library(sieve-thumbs SHARED client/thumbs_main.cpp client/picture_source.cpp client/thumbs.def)
        target_link_libraries(sieve-thumbs PRIVATE sieve_lzma sieve_lines sieve_compare sieve_locate sieve_image_io ole32 shlwapi uuid)
        set_target_properties(sieve-thumbs PROPERTIES PREFIX "")
    endif()
endif()
