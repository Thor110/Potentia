@echo off
rem Every test of a RAR made again by your own rar.exe (tools\rar_check.py says what each does).
rem   rar_check.bat                 RARs made here from the repository's files, at many settings
rem   rar_check.bat MY.rar ...      your own RARs too
rem The table at the end, and out\rar-check\rar-check.log to send.
setlocal
where py > nul 2>&1 && (py -3 "%~dp0rar_check.py" %*) || (python "%~dp0rar_check.py" %*)
set RESULT=%ERRORLEVEL%
echo.
pause
exit /b %RESULT%
