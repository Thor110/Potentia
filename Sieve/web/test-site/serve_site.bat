@echo off
rem Builds the web test site (make_site.py) with your sieve.exe and serves it at
rem http://localhost:8000/, opening it in your browser (web\README.md, step 4). Double-click it, or
rem give another sieve.exe: serve_site.bat C:\Sieve\tools\sieve.exe. Close the window, or press
rem Ctrl+C, to stop serving.
rem
rem Python is found as make_release.bat finds it: the py launcher first (python.org's installers put
rem it in place), then "python", which on Windows 10 and 11 can be the Microsoft Store's stand-in.
setlocal
rem A sieve.exe given is taken as a full path before the folder changes, so a relative one works.
set "SIEVE="
if not "%~1"=="" set "SIEVE=%~f1"
cd /d "%~dp0..\.."
if "%SIEVE%"=="" set "SIEVE=%CD%\out\build\x64-Release\sieve.exe"
rem (No bracketed block around the path: a ")" in it, as in "Program Files (x86)", would end one.)
if exist "%SIEVE%" goto have_sieve
echo sieve.exe is not at "%SIEVE%": build Release first, or give its path:
echo   serve_site.bat C:\path\to\sieve.exe
goto done
:have_sieve
set "PY="
py -3 --version >nul 2>&1
if not errorlevel 1 set "PY=py -3"
if not defined PY (
    python -c "import sys" >nul 2>&1
    if not errorlevel 1 set "PY=python"
)
if not defined PY (
    echo Python 3 was not found. Install it from python.org ^(it adds the py launcher^), or turn off
    echo the "python.exe" App execution alias in Settings, Apps, Advanced app settings.
    goto done
)
%PY% "%~dp0make_site.py" "%SIEVE%"
if errorlevel 1 goto done
echo.
echo Serving web\test-site\site at http://localhost:8000/ (close this window to stop).
rem The browser opened a moment later, once the server below is listening.
start "" /b %PY% -c "import time, webbrowser; time.sleep(1.5); webbrowser.open('http://localhost:8000/')"
%PY% -m http.server 8000 --bind 127.0.0.1 --directory "%~dp0site"
:done
echo.
pause
