@echo off
rem How long a picture takes to open as itself and as its .simage, and to install back (time_open_image.py beside
rem this file). Drop a picture on this file, or run it with the picture's path; with neither it asks for one.
rem It uses the Release build's programs (out\build\x64-Release) and the py launcher, else python. Each viewer
rem run opens a window for a moment, takes a screenshot and closes.
setlocal
set "HERE=%~dp0"
set "BIN=%HERE%..\out\build\x64-Release"
if not exist "%BIN%\sieve-view.exe" (
    echo sieve-view.exe is not in %BIN%: build the Release first.
    pause
    exit /b 1
)
for %%B in ("%BIN%") do set "BIN=%%~fB"
set "PY="
py -3 --version >nul 2>&1 && set "PY=py -3"
if not defined PY python -c "import sys; sys.exit(sys.version_info[0] != 3)" >nul 2>&1 && set "PY=python"
if not defined PY (
    echo Python 3 was not found: install it from python.org ^(it adds the py launcher^).
    pause
    exit /b 1
)
set "PIC=%~1"
if "%PIC%"=="" set /p "PIC=Picture to time (drag it here and press Enter): "
set "PIC=%PIC:"=%"
if not exist "%PIC%" (
    echo No such file: %PIC%
    pause
    exit /b 1
)
%PY% "%HERE%time_open_image.py" "%PIC%" --bin "%BIN%" --runs 10
pause
