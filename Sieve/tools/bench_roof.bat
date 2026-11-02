@echo off
rem The roof: how Sieve's time and memory grow with a file's size (tools\bench\bench_roof.py;
rem docs/ODDITIES.md has the floor beside it). Its first 1, 4, 16, 64, 256 and 1,024 MiB (as far as
rem the file goes), each made into Sieve instructions for size and for decoding speed and installed
rem again, with the time and the most memory each took. enwik9 is the file to give it.
rem
rem Double-click it and give the file when asked (it can be dragged into the window), or:
rem
rem   bench_roof.bat D:\bench\enwik9                         that file
rem   bench_roof.bat D:\bench\enwik9 --sizes 1,16,256          other sizes (MiB)
rem   bench_roof.bat C:\Sieve\tools\sieve.exe D:\bench\enwik9   another sieve.exe
rem
rem The results go beside the file, in <file>-roof-<date and time>. Close what you can while it
rem runs: it measures time and memory. The window stays open at the end.
setlocal
cd /d "%~dp0.."
set "SIEVE=%CD%\out\build\x64-Release\sieve.exe"
if /i "%~x1"==".exe" (
    set "SIEVE=%~f1"
    shift
)
set "BIG=%~1"
set "REST="
if not "%~1"=="" shift
:collect
if "%~1"=="" goto ask
set "REST=%REST% %1"
shift
goto collect
:ask
if defined BIG goto check
echo Which file? (Type its path, or drag it here. enwik9 is the one to give it.)
set /p "BIG=File: "
if not defined BIG goto nofile
set "BIG=%BIG:"=%"
:check
if not exist "%BIG%" goto nofile
if exist "%BIG%\" goto nofile
if not exist "%SIEVE%" goto nosieve
py -3 --version >nul 2>&1
if not errorlevel 1 (
    py -3 "%~dp0bench\bench_roof.py" "%SIEVE%" "%BIG%" %REST%
    goto done
)
python -c "import sys" >nul 2>&1
if not errorlevel 1 (
    python "%~dp0bench\bench_roof.py" "%SIEVE%" "%BIG%" %REST%
    goto done
)
echo Python 3 was not found. Install it from python.org (it adds the py launcher), or turn off
echo the "python.exe" App execution alias in Settings, Apps, Advanced app settings.
goto done
:nofile
echo No such file: %BIG%
goto done
:nosieve
echo sieve.exe is not at %SIEVE%
echo Build the Release, or name it: bench_roof.bat C:\path\to\sieve.exe D:\bench\enwik9
:done
echo.
pause
