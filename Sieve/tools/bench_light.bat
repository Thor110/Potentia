@echo off
rem The light bench (Edward, 2 November 2026): every file of a folder made into Sieve instructions
rem two ways, decode/no (Prioritise Decoding Speed, without Try every way) and size/try (for size,
rem with it), each installed again and checked, with no search limit. The other two of the full
rem bench (bench_files.bat) came to the same bytes as these on Edward's 153 JPEGs and on enwik9,
rem each taking longer: decode/try as decode/no, size/no as size/try.
rem
rem Double-click it and give the folder when asked (a folder can be dragged into the window), or:
rem
rem   bench_light.bat                                  asks for the folder
rem   bench_light.bat D:\bench                         that folder
rem   bench_light.bat D:\bench --keep                  and keeps each .sieve made
rem   bench_light.bat C:\Sieve\tools\sieve.exe D:\bench    another sieve.exe
rem
rem It uses the Release build's sieve.exe (out\build\x64-Release) unless the first option names
rem another. The results go beside the folder, in <folder>-bench-<date and time>, as the full
rem bench's do. The window stays open at the end.
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
echo Every file of %FOLDER%, largest first, two ways each (decode/no and size/try), with %SIEVE%.
echo It can take many hours (enwik9 alone takes several). Close the window to stop it: the
echo results so far are kept.
echo.
py -3 --version >nul 2>&1
if not errorlevel 1 (
    py -3 "%~dp0bench\bench_files.py" "%SIEVE%" "%FOLDER%" --ways decode/no,size/try %REST%
    goto done
)
python -c "import sys" >nul 2>&1
if not errorlevel 1 (
    python "%~dp0bench\bench_files.py" "%SIEVE%" "%FOLDER%" --ways decode/no,size/try %REST%
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
echo Build the Release, or name it: bench_light.bat C:\path\to\sieve.exe D:\bench
:done
echo.
pause
