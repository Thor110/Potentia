cmake_minimum_required(VERSION 3.21)
project(Sieve VERSION 0.4.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(SIEVE_BUILD_TESTS  "Build the core test suite"            ON)
option(SIEVE_BUILD_CLIENT "Build the SDL3 hallway client"          ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

find_package(Threads REQUIRED)

# ---------------------------------------------------------------- core (no dependencies)
add_library(sieve_core STATIC
    core/src/alphabet.cpp
    core/src/audio.cpp
    core/src/biguint.cpp
    core/src/canon.cpp
    core/src/image.cpp
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
    tools/cli/dictionaries.cpp
    tools/cli/lines.cpp)
target_include_directories(sieve_lines PUBLIC tools)
target_link_libraries(sieve_lines PUBLIC sieve_core sieve_image_io)

add_executable(sieve
    tools/sieve_cli.cpp
    tools/cli/help.cpp)
target_link_libraries(sieve PRIVATE sieve_lines)
target_compile_definitions(sieve PRIVATE SIEVE_VERSION="${PROJECT_VERSION}")

# Copy the bundled dictionaries next to the executable, so `sieve sift` works from any
# folder and in every configuration (Debug, Release, ...).
add_custom_command(TARGET sieve POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/data/dictionaries
            $<TARGET_FILE_DIR:sieve>/dictionaries
    COMMENT "Copying dictionaries next to sieve")

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
        client/hallway.cpp
        client/synth.cpp
        client/world.cpp)
    target_link_libraries(hallway PRIVATE sieve_lines SDL3::SDL3)
    add_custom_command(TARGET hallway POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${CMAKE_CURRENT_SOURCE_DIR}/data/dictionaries
                $<TARGET_FILE_DIR:hallway>/dictionaries)
endif()
