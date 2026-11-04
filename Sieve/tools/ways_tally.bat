@echo off
rem Which ways carried the files (Edward, 4 November 2026): every .sieve in a folder (and the
rem folders inside it) read back, each file's way tallied, and every file with a code of its own
rem (the rope, mixing, runs) set against LZMA2 alone on the same bytes. tools/ways_tally.py does
rem the work; Proof/STATISTICS.md, "What carried each file", says what its tables mean.
rem
rem Double-click it and give the folder when asked (a folder can be dragged into the window), or:
rem
rem   ways_tally.bat                                   asks for the folder
rem   ways_tally.bat D:\bench-results                  that folder's .sieve files
rem   ways_tally.bat D:\one.sieve                      one .sieve
rem   ways_tally.bat C:\Sieve\tools\sieve.exe D:\x     another sieve.exe
rem
rem Each .sieve is installed into a temporary folder (removed after) by the Release build's
rem sieve.exe (out\build\x64-Release) unless the first option names another, so that its files can
rem be packed by LZMA2 to compare; without a sieve.exe the listings alone are tallied. The tables
rem go beside the folder (or the file), as <name>-ways-<date and time>.md and .tsv, and are shown
rem here too. The window stays open at the end.
setlocal
cd /d "%~dp0.."
set "SIEVE=%CD%\out\build\x64-Release\sieve.exe"
if /i "%~x1"==".exe" (
    set "SIEVE=%~f1"
    shift
)
set "TARGET=%~1"
if defined TARGET goto check
echo Which folder's .sieve files are to be tallied? (Type its path, or drag the folder or a .sieve here.)
set /p "TARGET=Folder: "
if not defined TARGET goto notarget
rem (a path dragged in comes with quotes round it: taken off)
set "TARGET=%TARGET:"=%"
:check
rem (a folder ending in a backslash: taken off, so the tables are named after it)
if "%TARGET:~-1%"=="\" set "TARGET=%TARGET:~0,-1%"
if not exist "%TARGET%" goto notarget
set "PY="
py -3 --version >nul 2>&1
if not errorlevel 1 set "PY=py -3"
if not defined PY (
    python -c "import sys" >nul 2>&1
    if not errorlevel 1 set "PY=python"
)
if not defined PY goto nopython
for /f %%t in ('%PY% -c "import time; print(time.strftime('%%Y%%m%%d-%%H%%M%%S'))"') do set "STAMP=%%t"
for %%f in ("%TARGET%") do set "OUT=%%~dpnf-ways-%STAMP%"
set "MEASURE="
if exist "%SIEVE%" (
    set MEASURE=--sieve "%SIEVE%"
    echo Each .sieve installed by %SIEVE% to compare its files with LZMA2: large ones take a while.
) else (
    echo sieve.exe is not at %SIEVE%: the listings alone are tallied, with nothing compared to LZMA2.
)
echo.
%PY% "%~dp0ways_tally.py" "%TARGET%" %MEASURE% --md "%OUT%.md" --tsv "%OUT%.tsv"
echo.
echo Written: %OUT%.md and %OUT%.tsv
goto done
:notarget
echo No such folder or file: %TARGET%
goto done
:nopython
echo Python 3 was not found. Install it from python.org (it adds the py launcher), or turn off
echo the "python.exe" App execution alias in Settings, Apps, Advanced app settings.
:done
echo.
pause
