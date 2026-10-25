@echo off
rem Runs Sieve's CI steps on Windows (ci\README.md), through the bash that comes with Git for
rem Windows. Double-click it for every step, or give steps:
rem
rem   ci\run.bat                     every step (the Linux-only ones are skipped)
rem   ci\run.bat 03 web              steps by number, or by a word of their name
rem   ci\run.bat --list              the steps
rem   ci\run.bat --build C:\path\to\build 10
rem
rem It uses out\build\x64-Release unless --build names another. The window stays open at the end.
setlocal
set "BASH="
for %%B in ("%ProgramFiles%\Git\bin\bash.exe" "%ProgramFiles(x86)%\Git\bin\bash.exe" "%LocalAppData%\Programs\Git\bin\bash.exe") do (
    if not defined BASH if exist %%B set "BASH=%%~B"
)
rem No brackets inside a bracketed block: cmd reads a ")" in an echo there as the block's end,
rem stops at once and closes the window.
if defined BASH goto run
echo Git's bash was not found. Install Git for Windows from git-scm.com, which brings it.
goto done
:run
"%BASH%" "%~dp0run.sh" %*
:done
echo.
pause
