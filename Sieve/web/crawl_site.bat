@echo off
rem Checks a site on its server against its table (web\crawl_site.py; web\README.md):
rem
rem   crawl_site.bat https://example.com/ --table SHA256                 once
rem   crawl_site.bat https://example.com/ --table SHA256 --every 60      every hour, until closed
rem
rem SHA256 is the table's, as sieve site build printed it ("index.sieve sha256 ..."). It uses this
rem installation's tools\sieve.exe, else the Release build's (out\build\x64-Release); give
rem --sieve PATH for another. For Windows' Task Scheduler, run it once at the times you choose.
rem Python is found as make_release.bat finds it: the py launcher first, then python.
setlocal
set "SIEVE="
if exist "%~dp0..\tools\sieve.exe" set "SIEVE=%~dp0..\tools\sieve.exe"
if "%SIEVE%"=="" set "SIEVE=%~dp0..\out\build\x64-Release\sieve.exe"
py -3 --version >nul 2>&1
if not errorlevel 1 (
    py -3 "%~dp0crawl_site.py" --sieve "%SIEVE%" %*
    exit /b %errorlevel%
)
python -c "import sys" >nul 2>&1
if not errorlevel 1 (
    python "%~dp0crawl_site.py" --sieve "%SIEVE%" %*
    exit /b %errorlevel%
)
echo Python 3 was not found. Install it from python.org (it adds the py launcher), or turn off
echo the "python.exe" App execution alias in Settings, Apps, Advanced app settings.
exit /b 2
