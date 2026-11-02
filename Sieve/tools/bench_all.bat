@echo off
rem Every benchmark over several folders, one after another, for a night's run: bench_files.py with
rem no box, with temporary and with permanent on each folder, then bench_power.py on one. Put it in
rem Sieve's tools folder. It runs the Python scripts itself (not the other .bat files), and every
rem run's output goes to bench_all.log beside it, as well as when each began and ended, so a run that
rem stops is seen in the morning with why. Each run's results go beside the folder it was given.
setlocal
set "HERE=%~dp0"
cd /d "%HERE%"

rem The folders to benchmark (one per line; leave one empty to skip it), and the power benchmark's.
set "F1=C:\Users\Edward\Pictures\my-test-bench"
set "F2=C:\Users\Edward\Pictures\silesia"
set "F3=C:\Users\Edward\Pictures\jpeg-test"
set "F4=C:\Users\Edward\Pictures\png-test"
set "POWER=%F2%"

set "LOG=%HERE%bench_all.log"
set "SIEVE=%HERE%..\out\build\x64-Release\sieve.exe"
if not exist "%SIEVE%" (
    echo sieve.exe is not at %SIEVE%: build the Release first.
    echo sieve.exe not found at %SIEVE%>> "%LOG%"
    pause
    exit /b 1
)
for %%S in ("%SIEVE%") do set "SIEVE=%%~fS"
set "PY="
py -3 --version >nul 2>&1 && set "PY=py -3"
if not defined PY python -c "import sys; sys.exit(sys.version_info[0] != 3)" >nul 2>&1 && set "PY=python"
if not defined PY (
    echo Python 3 was not found: install it from python.org ^(it adds the py launcher^).
    echo Python 3 not found>> "%LOG%"
    pause
    exit /b 1
)

echo ==========================================================>> "%LOG%"
echo bench_all started %DATE% %TIME%, with %SIEVE% and %PY%>> "%LOG%"
echo Starting the Sieve benchmarks: each run's output and times go to %LOG%
call :folder "%F1%"
call :folder "%F2%"
call :folder "%F3%"
call :folder "%F4%"
if not "%POWER%"=="" call :power "%POWER%"
echo bench_all ended %DATE% %TIME%>> "%LOG%"
echo All done: each folder's results are beside it; the times and output are in %LOG%
exit /b 0

:folder
if "%~1"=="" exit /b 0
if not exist "%~1\" (
    echo No such folder: %~1 ^(skipped^)
    echo skipped %~1: no such folder>> "%LOG%"
    exit /b 0
)
call :run "%~1" "no box" ""
call :run "%~1" "temporary" "--compressed temporary"
call :run "%~1" "permanent" "--compressed permanent"
exit /b 0

:run
echo.
echo %~1, %~2: running ^(see %LOG%^)
echo ---------------------------------------------------------->> "%LOG%"
echo %~1, %~2: began %DATE% %TIME%>> "%LOG%"
%PY% "%HERE%bench\bench_files.py" "%SIEVE%" "%~1" %~3 >> "%LOG%" 2>&1
echo %~1, %~2: ended %DATE% %TIME%, exit %ERRORLEVEL%>> "%LOG%"
exit /b 0

:power
if not exist "%~1\" exit /b 0
echo.
echo the power benchmark on %~1: running ^(see %LOG%^)
echo ---------------------------------------------------------->> "%LOG%"
echo power, %~1: began %DATE% %TIME%>> "%LOG%"
set "FILES="
for %%F in ("%~1\*") do call set FILES=%%FILES%% "%%~fF"
%PY% "%HERE%bench\bench_power.py" "%SIEVE%" %FILES% >> "%LOG%" 2>&1
echo power, %~1: ended %DATE% %TIME%, exit %ERRORLEVEL%>> "%LOG%"
exit /b 0
