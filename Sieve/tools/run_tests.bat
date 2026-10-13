@echo off
rem Runs Sieve's tests on Windows, as far as they go without Linux: the unit tests of every build
rem there is (out\build\x64-Debug, x64-RelWithDebInfo and x64-Release), then, if Python is there,
rem every conformance vector reproduced by the oracle (tools\check_vectors.py, as CI checks them).
rem Double-click it, or run it from a command prompt; "run_tests.bat quick" skips the oracle, which
rem takes about a quarter of an hour. The window stays open at the end, with a line for each part.
setlocal
cd /d "%~dp0.."
set "SUMMARY=%TEMP%\sieve-test-summary.txt"
if exist "%SUMMARY%" del "%SUMMARY%"
set FAILED=0

for %%P in (x64-Debug x64-RelWithDebInfo x64-Release) do call :unit %%P
if /i "%~1"=="quick" goto report

set "PY="
py -3 --version >nul 2>&1
if not errorlevel 1 set "PY=py -3"
if defined PY goto oracle
python -c "import sys" >nul 2>&1
if not errorlevel 1 set "PY=python"
if defined PY goto oracle
>>"%SUMMARY%" echo   skipped  the oracle's vectors: Python 3 was not found
goto report

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
"out\build\%1\sieve_tests.exe" tests
if errorlevel 1 (
    set FAILED=1
    >>"%SUMMARY%" echo   FAILED   unit tests, %1
) else (
    >>"%SUMMARY%" echo   ok       unit tests, %1
)
exit /b 0
