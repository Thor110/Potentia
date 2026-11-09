@echo off
rem Makes Sieve's release extensions (tools\extend_release.py): what the release leaves out, each as
rem Sieve instructions of its own to install into a Sieve folder (voice.sieve: Sieve's own voice).
rem The Release build in out\build\x64-Release, the files in Sieve\release, beside the release's.
rem Double-click it, or run it with the script's options (extend_release.bat --only voice, say). The
rem window stays open at the end so the result can be read.
rem
rem Python is found through the py launcher first: python.org's installers put it in place, and on
rem Windows 10 and 11 a bare "python" can be the Microsoft Store's stand-in instead of Python.
cd /d "%~dp0.."
py -3 --version >nul 2>&1
if not errorlevel 1 (
    py -3 "%~dp0extend_release.py" %*
    goto done
)
python -c "import sys" >nul 2>&1
if not errorlevel 1 (
    python "%~dp0extend_release.py" %*
    goto done
)
echo Python 3 was not found. Install it from python.org (it adds the py launcher), or turn off
echo the "python.exe" App execution alias in Settings, Apps, Advanced app settings.
:done
echo.
pause
