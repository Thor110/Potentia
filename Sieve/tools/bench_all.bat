@echo off
rem Every benchmark over several folders, one after another, for a night's run: bench_files.bat
rem (no box), bench_files_optimise_temporary.bat and bench_files_optimise_permanent.bat on each
rem folder, then bench_power.bat on one. Put it in Sieve's tools folder, beside those scripts.
rem Each script writes its results beside the folder it was given; this one writes bench_all.log
rem beside itself: when each run began and ended, so the morning shows how long each took.
setlocal enabledelayedexpansion
cd /d "%~dp0"
for %%S in (bench_files.bat bench_files_optimise_temporary.bat bench_files_optimise_permanent.bat bench_power.bat) do (
    if not exist "%~dp0%%S" (
        echo %%S is not beside this script ^(%~dp0^): put bench_all.bat in Sieve's tools folder.
        pause
        exit /b 1
    )
)

rem The folders to benchmark (one per line; leave one empty to skip it).
set "F1=C:\Users\Edward\Pictures\my-test-bench"
set "F2=C:\Users\Edward\Pictures\silesia"
set "F3=C:\Users\Edward\Pictures\jpeg-test"
set "F4=C:\Users\Edward\Pictures\png-test"
rem The folder the power benchmark runs on.
set "POWER=%F2%"

set "LOG=%~dp0bench_all.log"
echo bench_all started %DATE% %TIME%>> "%LOG%"
echo ==========================================================
echo Starting the Sieve benchmarks (each run's times go to %LOG%)
echo ==========================================================

for %%F in ("%F1%" "%F2%" "%F3%" "%F4%") do (
    if "%%~F"=="" (
        echo.>nul
    ) else if not exist "%%~F\" (
        echo.
        echo No such folder: %%~F ^(skipped^)
        echo   skipped %%~F: no such folder>> "%LOG%"
    ) else (
        echo.
        echo ==========================================================
        echo Folder: %%~F
        echo ==========================================================
        echo [1/3] bench_files.bat ^(no box^)
        echo   %%~F, no box: began !DATE! !TIME!>> "%LOG%"
        call "%~dp0bench_files.bat" "%%~F" <nul
        echo   %%~F, no box: ended !DATE! !TIME!>> "%LOG%"
        echo [2/3] bench_files_optimise_temporary.bat
        echo   %%~F, temporary: began !DATE! !TIME!>> "%LOG%"
        call "%~dp0bench_files_optimise_temporary.bat" "%%~F" <nul
        echo   %%~F, temporary: ended !DATE! !TIME!>> "%LOG%"
        echo [3/3] bench_files_optimise_permanent.bat
        echo   %%~F, permanent: began !DATE! !TIME!>> "%LOG%"
        call "%~dp0bench_files_optimise_permanent.bat" "%%~F" <nul
        echo   %%~F, permanent: ended !DATE! !TIME!>> "%LOG%"
    )
)

if not "%POWER%"=="" if exist "%POWER%\" (
    echo.
    echo ==========================================================
    echo The power benchmark on %POWER%
    echo ==========================================================
    echo   power, %POWER%: began !DATE! !TIME!>> "%LOG%"
    call "%~dp0bench_power.bat" "%POWER%" <nul
    echo   power, %POWER%: ended !DATE! !TIME!>> "%LOG%"
)

echo bench_all ended %DATE% %TIME%>> "%LOG%"
echo.
echo ==========================================================
echo All done: each folder's results are beside it; the times are in %LOG%
echo ==========================================================
