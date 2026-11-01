@echo off
rem Benchmarks every file of a folder for Proof\STATISTICS.md (tools\bench\bench_files.py): each
rem file, largest first, made into Sieve instructions four ways (for size and for decoding speed,
rem each with Try every way and without), each installed again and checked, with no search limit.
rem As bench_files.bat, but with Compressed formats, temporary, ticked for every run: an archive, compressed file, PNG or
rem JPEG carried as what it holds and made again exactly on installing (--compressed-temporary).
rem
rem Double-click it and give the folder when asked (a folder can be dragged into the window), or:
rem
rem   bench_files_optimise_temporary.bat                                  asks for the folder
rem   bench_files_optimise_temporary.bat D:\bench                         that folder
rem   bench_files_optimise_temporary.bat D:\bench --keep                  and keeps each .sieve made
rem   bench_files_optimise_temporary.bat C:\Sieve\tools\sieve.exe D:\bench    another sieve.exe
rem
rem It uses the Release build's sieve.exe (out\build\x64-Release) unless the first option names
rem another. The results go beside the folder, in <folder>-bench-temporary-<date and time>: results.md,
rem results.tsv and each run's output. The window stays open at the end.
rem Python is found as make_release.bat finds it: the py launcher first, then python.
setlocal
cd /d "%~dp0.."
set "SIEVE=%CD%\out\build\x64-Release\sieve.exe"
if /i "%~x1"==".exe" (
    set "SIEVE=%~f1"
    shift
)
set "FOLDER=%~1"
set "REST="
if not "%~1"=="" shift
:collect
if "%~1"=="" goto ask
set "REST=%REST% %1"
shift
goto collect
:ask
if defined FOLDER goto check
echo Which folder's files are to be benchmarked? (Type its path, or drag the folder here.)
set /p "FOLDER=Folder: "
if not defined FOLDER goto nofolder
rem (a path dragged in comes with quotes round it: taken off)
set "FOLDER=%FOLDER:"=%"
:check
if not defined FOLDER goto nofolder
if not exist "%FOLDER%\" goto nofolder
if not exist "%SIEVE%" goto nosieve
echo.
echo Every file of %FOLDER%, largest first, four ways each, Compressed formats temporary, with %SIEVE%.
echo It can take many hours (enwik9 alone takes several). Close the window to stop it: the
echo results so far are kept.
echo.
py -3 --version >nul 2>&1
if not errorlevel 1 (
    py -3 "%~dp0bench\bench_files.py" "%SIEVE%" "%FOLDER%" --compressed temporary %REST%
    goto done
)
python -c "import sys" >nul 2>&1
if not errorlevel 1 (
    python "%~dp0bench\bench_files.py" "%SIEVE%" "%FOLDER%" --compressed temporary %REST%
    goto done
)
echo Python 3 was not found. Install it from python.org (it adds the py launcher), or turn off
echo the "python.exe" App execution alias in Settings, Apps, Advanced app settings.
goto done
:nofolder
echo No such folder: %FOLDER%
goto done
:nosieve
echo sieve.exe is not at %SIEVE%
echo Build the Release, or name it: bench_files_optimise_temporary.bat C:\path\to\sieve.exe D:\bench
:done
echo.
pause
