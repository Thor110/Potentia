@echo off
rem Runs Sieve's tests on Windows, as far as they go without Linux: the unit tests of every build
rem there is (out\build\x64-Debug, x64-RelWithDebInfo and x64-Release), then, if Python is there,
rem every conformance vector reproduced by the oracle (tools\check_vectors.py, as CI checks them).
rem Double-click it, or run it from a command prompt; "run_tests.bat quick" skips the oracle, which
rem takes about a quarter of an hour. The window stays open at the end, with a line for each part.
rem When everything that ran passed, it logs the build's metrics (tools\log_metrics.py, IDEAS
rem §19.50): a record in metrics\builds and a line in metrics\index.tsv, which the website and the
rem game read. Commit them with the build, so the record stays in step with the code.
setlocal
cd /d "%~dp0.."
set "SUMMARY=%TEMP%\sieve-test-summary.txt"
if exist "%SUMMARY%" del "%SUMMARY%"
set FAILED=0
set "UNITS="

set "PY="
py -3 --version >nul 2>&1
if not errorlevel 1 set "PY=py -3"
if defined PY goto found_python
python -c "import sys" >nul 2>&1
if not errorlevel 1 set "PY=python"
:found_python

for %%P in (x64-Debug x64-RelWithDebInfo x64-Release) do call :unit %%P
if /i "%~1"=="quick" goto metrics
if defined PY goto oracle
>>"%SUMMARY%" echo   skipped  the oracle's vectors: Python 3 was not found
goto metrics

:oracle
echo.
echo == the oracle's vectors
%PY% tools\check_vectors.py
if errorlevel 1 (
    set FAILED=1
    >>"%SUMMARY%" echo   FAILED   the oracle's vectors
) else (
    >>"%SUMMARY%" echo   ok       the oracle's vectors
)

:metrics
if %FAILED%==1 goto report
if not defined PY goto report
if not defined UNITS goto report
set "HALLWAY="
for %%P in (x64-Debug x64-RelWithDebInfo x64-Release) do if exist "out\build\%%P\hallway.exe" set "HALLWAY=--hallway out\build\%%P\hallway.exe"
echo.
echo == the build's metrics
%PY% tools\log_metrics.py %UNITS% %HALLWAY%
if errorlevel 1 (
    >>"%SUMMARY%" echo   not logged  the build's metrics
) else (
    >>"%SUMMARY%" echo   logged   the build's metrics, in metrics\builds and metrics\index.tsv
)

:report
echo.
echo == summary
if exist "%SUMMARY%" (type "%SUMMARY%") else (echo   no builds found in out\build: build Debug or Release in Visual Studio first)
echo.
if %FAILED%==1 (echo Something failed: see above.) else (echo Everything that ran passed.)
pause
exit /b %FAILED%

:unit
if not exist "out\build\%1\sieve_tests.exe" exit /b 0
echo.
echo == unit tests, %1
rem The output is kept, for the metrics, and then shown.
"out\build\%1\sieve_tests.exe" tests > "%TEMP%\sieve-unit-%1.txt" 2>&1
set UNIT_FAILED=%errorlevel%
type "%TEMP%\sieve-unit-%1.txt"
set "UNITS=%UNITS% --unit "%1=%TEMP%\sieve-unit-%1.txt""
if not %UNIT_FAILED%==0 (
    set FAILED=1
    >>"%SUMMARY%" echo   FAILED   unit tests, %1
) else (
    >>"%SUMMARY%" echo   ok       unit tests, %1
)
exit /b 0
