@echo off
rem Tests Sieve's MP3 way (packMP3) on every .mp3 under a folder, at any depth: each one coded by
rem packMP3, decoded again and compared byte for byte (tests\test_mp3.cpp: sieve_mp3_check --folder).
rem
rem   tools\mp3_check.bat FOLDER [DAMAGED] [LOG]
rem
rem DAMAGED  also decode each file's packMP3 form changed this many times, as a hostile .sieve might
rem          change it: each must be refused or decoded, never end the program (default 0; 5 is
rem          thorough and takes about twice as long)
rem LOG      the table of results, a line a file (default %USERPROFILE%\sieve-mp3-check.tsv, kept
rem          outside the repository; written over each run)
rem
rem The program is built with the tests: out\build\x64-Release, else x64-Debug, else build, under the
rem Sieve folder; or set SIEVE_MP3_CHECK to it. What to look for: "not given back" or "failed" (a fault
rem to report); "not coded" and "not an MP3's start" are files packMP3 leaves as they are (MPEG-2,
rem files with other data first), which Sieve then carries its other ways.
setlocal EnableExtensions DisableDelayedExpansion
chcp 65001 >nul
if "%~1"=="" (
    echo usage: %~nx0 FOLDER [DAMAGED] [LOG]
    exit /b 2
)
if not exist "%~1\" (
    echo no such folder: %~1
    exit /b 2
)
set "DAMAGED=%~2"
if "%DAMAGED%"=="" set "DAMAGED=0"
set "LOG=%~3"
if "%LOG%"=="" set "LOG=%USERPROFILE%\sieve-mp3-check.tsv"
set "EXE=%SIEVE_MP3_CHECK%"
if not "%EXE%"=="" goto found
if exist "%~dp0..\out\build\x64-Release\sieve_mp3_check.exe" (
    set "EXE=%~dp0..\out\build\x64-Release\sieve_mp3_check.exe"
    goto found
)
if exist "%~dp0..\out\build\x64-Debug\sieve_mp3_check.exe" (
    set "EXE=%~dp0..\out\build\x64-Debug\sieve_mp3_check.exe"
    goto found
)
if exist "%~dp0..\build\sieve_mp3_check.exe" (
    set "EXE=%~dp0..\build\sieve_mp3_check.exe"
    goto found
)
echo sieve_mp3_check.exe not found: build Sieve with its tests, or set SIEVE_MP3_CHECK to it
exit /b 2
:found
echo Testing every .mp3 under %~1
echo with %EXE%
echo (damaged forms a file: %DAMAGED%; the table: %LOG%)
echo.
"%EXE%" --folder "%~1" --damage %DAMAGED% --log "%LOG%"
set "RC=%ERRORLEVEL%"
echo.
echo The table: %LOG%
if "%RC%"=="0" (
    echo Every MP3 packMP3 coded was given back exactly.
) else (
    echo Some MP3s were not given back exactly, or failed: see the lines starting "not given back" or "failed".
)
exit /b %RC%
