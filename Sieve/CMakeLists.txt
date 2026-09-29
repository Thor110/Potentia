cmake_minimum_required(VERSION 3.21)
project(Sieve VERSION 0.13.0 LANGUAGES CXX)

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
    core/src/booksieve.cpp
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
    core/src/filters/models.cpp
    core/src/filters/pattern.cpp
    core/src/written.cpp
    core/src/filters/builtin.cpp
    core/src/filters/media.cpp
    core/src/filters/statistics.cpp
    core/src/filters/text_m1.cpp
    core/src/guided.cpp
    core/src/model.cpp
    core/src/image.cpp
    core/src/intlog.cpp
    core/src/sha256.cpp
    core/src/sieve.cpp
    core/src/space.cpp
    core/src/utf8.cpp)
target_include_directories(sieve_core PUBLIC core/include)
target_link_libraries(sieve_core PUBLIC Threads::Threads)

if(MSVC)
    target_compile_options(sieve_core PUBLIC /W4 /utf-8 /permissive-)
else()
    target_compile_options(sieve_core PUBLIC -Wall -Wextra -Wpedantic)
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
    tools/cli/lines.cpp
    tools/cli/models.cpp
    tools/cli/plugins.cpp)
target_include_directories(sieve_lines PUBLIC tools)
target_link_libraries(sieve_lines PUBLIC sieve_core sieve_image_io)
# (sieve_locate, defined below, is linked into sieve_lines after it: the lines check the vault.)

# The file locator and the manifest creator (tools/cli/locate.*), and the comparison of a file's
# size against what zip and 7z make of it (tools/cli/compare.*). The comparison needs the two
# compressors themselves: zlib (zip's deflate) and liblzma (7z's LZMA2). An installed copy is used
# if there is one; otherwise each is fetched from its own repository at a pinned release and built
# here as a static library, as SDL3 is below. Only the tools link them, never the core.
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
    set(SIEVE_LZMA liblzma)
    # Built inside this project, xz's liblzma target keeps its header folder to itself (it is
    # only exported when installed), so its users are given it here. A system with its own
    # lzma.h in /usr/include hides the omission, which is how it first got through.
    set(SIEVE_LZMA_INCLUDE ${xz_SOURCE_DIR}/src/liblzma/api)
else()
    set(SIEVE_LZMA LibLZMA::LibLZMA)
endif()

# The locator and the installer's code (no compressors: sieve-install links this alone), and the
# comparison, which is the only part that needs them.
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

add_library(sieve_locate STATIC tools/cli/locate.cpp tools/cli/map.cpp tools/cli/vault.cpp tools/cli/vault_decode.cpp tools/cli/timings.cpp)
target_include_directories(sieve_locate PUBLIC tools)
# The vault checks pictures (PDQ), including files that are pictures, so it reads them (stb).
target_link_libraries(sieve_locate PUBLIC sieve_core PRIVATE sieve_pdq sieve_image_io)
target_link_libraries(sieve_lines PUBLIC sieve_locate) # the lines check units against the vault
add_library(sieve_compare STATIC tools/cli/compare.cpp)
target_include_directories(sieve_compare PUBLIC tools)
target_link_libraries(sieve_compare PUBLIC sieve_core PRIVATE ${SIEVE_ZLIB} ${SIEVE_LZMA})
if(SIEVE_LZMA_INCLUDE)
    target_include_directories(sieve_compare SYSTEM PRIVATE ${SIEVE_LZMA_INCLUDE})
endif()

add_executable(sieve
    tools/sieve_cli.cpp
    tools/cli/help.cpp
    ${SIEVE_ICON_RC})
target_link_libraries(sieve PRIVATE sieve_lines sieve_locate sieve_compare)
if(WIN32)
    # UTF-8 command lines on Windows (the manifest sets the process code page).
    target_sources(sieve PRIVATE tools/utf8.manifest)
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
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/data/dictionaries
            $<TARGET_FILE_DIR:sieve>/dictionaries
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/data/models
            $<TARGET_FILE_DIR:sieve>/models
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/data/lang
            $<TARGET_FILE_DIR:sieve>/lang
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/third_party_licenses
            $<TARGET_FILE_DIR:sieve>/third_party_licenses
    COMMENT "Copying the licences, maps, dictionaries, models and languages next to sieve")
add_dependencies(sieve_data sieve)

# ---------------------------------------------------------------- tests
if(SIEVE_BUILD_TESTS)
    enable_testing()
    add_executable(sieve_tests tests/test_core.cpp)
    target_link_libraries(sieve_tests PRIVATE sieve_core)
    add_test(NAME core COMMAND sieve_tests ${CMAKE_CURRENT_SOURCE_DIR}/tests)
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
    endif()

    add_executable(hallway
        ${SIEVE_ICON_RC}
        client/app_main.cpp
        client/app_settings.cpp
        client/binary_edge.cpp
        client/designer.cpp
        client/designer_model.cpp
        client/item_faces.cpp
        client/door_portal.cpp
        client/file_locator.cpp
        client/node_graph.cpp
        client/item_save.cpp
        client/navigator.cpp
        client/pause_menu.cpp
        client/font.cpp
        client/hallway.cpp
        client/hud.cpp
        client/main_menu.cpp
        client/menu.cpp
        client/media_player.cpp
        client/mesh.cpp
        client/music.cpp
        client/strings.cpp
        client/synth.cpp
        client/world.cpp)
    target_link_libraries(hallway PRIVATE sieve_lines sieve_locate sieve_compare SDL3::SDL3)
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
    # The LZMA SDK's 7z decoder (third_party/lzma, public domain), for sieve-install alone: it
    # unpacks an installer that carries one 7z archive (client/unpack_7z.hpp). C, so its own
    # language; its warnings are the SDK's, at each compiler's usual level.
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
    add_executable(sieve-install WIN32 client/installer_main.cpp client/unpack_7z.cpp ${SIEVE_ICON_RC})
    target_link_libraries(sieve-install PRIVATE sieve_lzma)
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
                target_link_options(sieve-install PRIVATE -Wl,--gc-sections -s)
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
endif()
