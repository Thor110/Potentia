@echo off
rem What a file made smaller saves the networks and moves onto the people who install it
rem (tools\bench\bench_power.py; IDEAS 19.111): each file made by Sieve for size and for decoding
rem speed, and by zip, 7z and brotli, its bytes and the processor time to make and to install it
rem measured; the energy of moving bytes and the processors' power are settings, written beside.
rem
rem Double-click it and give a file or a folder when asked (one can be dragged into the window), or:
rem
rem   bench_power.bat                                  asks
rem   bench_power.bat D:\bench\picture.jpg             that file
rem   bench_power.bat D:\bench                         every file of that folder (not its subfolders)
rem   bench_power.bat C:\Sieve\tools\sieve.exe D:\bench    another sieve.exe
rem
rem Settings go after it, as bench_power.py takes them (--net-kwh-per-gb LOW HIGH, --user-watts W,
rem --server-watts W, --fetches N ...). The results go beside the file or folder, in
rem power-bench-<date and time>. The window stays open at the end.
setlocal enabledelayedexpansion
cd /d "%~dp0.."
set "SIEVE=%CD%\out\build\x64-Release\sieve.exe"
if /i "%~x1"==".exe" (
    set "SIEVE=%~f1"
    shift
)
set "TARGET=%~1"
set "REST="
if not "%~1"=="" shift
:collect
if "%~1"=="" goto ask
set "REST=%REST% %1"
shift
goto collect
:ask
if defined TARGET goto check
echo Which file or folder? (Type its path, or drag it here.)
set /p "TARGET=File or folder: "
if not defined TARGET goto notarget
set "TARGET=%TARGET:"=%"
:check
if not exist "%TARGET%" goto notarget
if not exist "%SIEVE%" goto nosieve
set "FILES="
if exist "%TARGET%\" (
    for %%F in ("%TARGET%\*") do set FILES=!FILES! "%%~fF"
) else (
    set FILES="%TARGET%"
)
py -3 --version >nul 2>&1
if not errorlevel 1 (
    py -3 "%~dp0bench\bench_power.py" "%SIEVE%" %FILES% %REST%
    goto done
)
python -c "import sys" >nul 2>&1
if not errorlevel 1 (
    python "%~dp0bench\bench_power.py" "%SIEVE%" %FILES% %REST%
    goto done
)
echo Python 3 was not found. Install it from python.org (it adds the py launcher), or turn off
echo the "python.exe" App execution alias in Settings, Apps, Advanced app settings.
goto done
:notarget
echo No such file or folder: %TARGET%
goto done
:nosieve
echo sieve.exe is not at %SIEVE%
echo Build the Release, or name it: bench_power.bat C:\path\to\sieve.exe D:\bench
:done
echo.
pause
