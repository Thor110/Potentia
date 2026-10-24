@echo off
rem Times making Sieve instructions, by stage and kind of file (tools\bench\bench_make.py), with
rem the Release build's sieve.exe (out\build\x64-Release) unless the first option names another.
rem Double-click it for the quick set, or give the script's options:
rem
rem   bench_make.bat                          the quick set, 1 s of search a file
rem   bench_make.bat --set full               and the 52 MB recording (about 6 minutes more)
rem   bench_make.bat --search 1,10            several search times
rem   bench_make.bat C:\Sieve\tools\sieve.exe --set full    another sieve.exe
rem
rem The tables go to Sieve\results (bench-make-*.md and .tsv). The window stays open at the end.
rem Python is found as make_release.bat finds it: the py launcher first, then python.
setlocal
cd /d "%~dp0..\.."
set "SIEVE=%CD%\out\build\x64-Release\sieve.exe"
if /i "%~x1"==".exe" (
    set "SIEVE=%~f1"
    shift
)
set "REST="
:collect
if "%~1"=="" goto run
set "REST=%REST% %1"
shift
goto collect
:run
if not exist "%SIEVE%" goto nosieve
py -3 --version >nul 2>&1
if not errorlevel 1 (
    py -3 "%~dp0bench_make.py" "%SIEVE%" %REST%
    goto done
)
python -c "import sys" >nul 2>&1
if not errorlevel 1 (
    python "%~dp0bench_make.py" "%SIEVE%" %REST%
    goto done
)
echo Python 3 was not found. Install it from python.org (it adds the py launcher), or turn off
echo the "python.exe" App execution alias in Settings, Apps, Advanced app settings.
goto done
:nosieve
echo sieve.exe is not at %SIEVE%
echo Build the Release, or name it: bench_make.bat C:\path\to\sieve.exe
:done
echo.
pause
