cmake_minimum_required(VERSION 3.21)
project(Potentia VERSION 0.3.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(POTENTIA_BUILD_TESTS  "Build the core test suite"            ON)
option(POTENTIA_BUILD_CLIENT "Build the SDL3 hallway client"          ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

find_package(Threads REQUIRED)

# ---------------------------------------------------------------- core (no dependencies)
add_library(potentia_core STATIC
    core/src/alphabet.cpp
    core/src/audio.cpp
    core/src/biguint.cpp
    core/src/canon.cpp
    core/src/image.cpp
    core/src/sha256.cpp
    core/src/sieve.cpp
    core/src/space.cpp
    core/src/utf8.cpp)
target_include_directories(potentia_core PUBLIC core/include)
target_link_libraries(potentia_core PUBLIC Threads::Threads)

if(MSVC)
    target_compile_options(potentia_core PUBLIC /W4 /utf-8 /permissive-)
else()
    target_compile_options(potentia_core PUBLIC -Wall -Wextra -Wpedantic)
endif()

# ---------------------------------------------------------------- command-line tool
# Image file I/O wraps stb_image (third-party, public domain). It lives in its own library so
# it is compiled without the core's strict warning flags.
add_library(potentia_image_io STATIC tools/cli/image_io.cpp)
target_include_directories(potentia_image_io PRIVATE core/include)
target_include_directories(potentia_image_io SYSTEM PRIVATE third_party/stb)
if(MSVC)
    target_compile_options(potentia_image_io PRIVATE /utf-8)
endif()

# Shared by the command-line tool and the hallway: the four lines, file input/output, dictionaries.
add_library(potentia_lines STATIC
    tools/cli/dictionaries.cpp
    tools/cli/lines.cpp)
target_include_directories(potentia_lines PUBLIC tools)
target_link_libraries(potentia_lines PUBLIC potentia_core potentia_image_io)

add_executable(potentia
    tools/potentia_cli.cpp
    tools/cli/help.cpp)
target_link_libraries(potentia PRIVATE potentia_lines)
target_compile_definitions(potentia PRIVATE POTENTIA_VERSION="${PROJECT_VERSION}")

# Copy the bundled dictionaries next to the executable, so `potentia sieve` works from any
# folder and in every configuration (Debug, Release, ...).
add_custom_command(TARGET potentia POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/data/dictionaries
            $<TARGET_FILE_DIR:potentia>/dictionaries
    COMMENT "Copying dictionaries next to potentia")

# ---------------------------------------------------------------- tests
if(POTENTIA_BUILD_TESTS)
    enable_testing()
    add_executable(potentia_tests tests/test_core.cpp)
    target_link_libraries(potentia_tests PRIVATE potentia_core)
    add_test(NAME core COMMAND potentia_tests ${CMAKE_CURRENT_SOURCE_DIR}/tests)
endif()

# ---------------------------------------------------------------- hallway client (SDL3)
# Uses an installed SDL3 (vcpkg with the "client" feature, or the system) if one is found;
# otherwise downloads and builds SDL 3.2.30 as a static library the first time you configure.
if(POTENTIA_BUILD_CLIENT)
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
    target_link_libraries(hallway PRIVATE potentia_lines SDL3::SDL3)
    add_custom_command(TARGET hallway POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${CMAKE_CURRENT_SOURCE_DIR}/data/dictionaries
                $<TARGET_FILE_DIR:hallway>/dictionaries)
endif()
