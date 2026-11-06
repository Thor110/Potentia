@echo off
rem Builds Sieve in Release with nothing but Visual Studio (IDEAS 19.39: "how easy it is"), then
rem makes the release from it where Python is installed. Double-click it, or run it with
rem make_release.py's options (build_release.bat --seven-zip, say), which are passed on.
rem
rem   1. Visual Studio 2022 (or later) with "Desktop development with C++" is found by vswhere,
rem      which its installer puts in place, and its build tools are loaded (VsDevCmd), which brings
rem      its own CMake and Ninja: no separate CMake, Ninja or Python is needed for this.
rem   2. The x64-Release preset is configured and built into out\build\x64-Release (the first time
rem      it downloads SDL3 and the libraries, so it needs the internet and some minutes).
rem   3. With Python (the py launcher, or python), tools\make_release.py makes the release from that
rem      build (it also needs 7-Zip). Without Python the build is the result: hallway.exe runs from
rem      out\build\x64-Release, everything it needs beside it.
rem The window stays open at the end so the result can be read.
setlocal
cd /d "%~dp0.."
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" goto novs
set "VSDIR="
for /f "usebackq tokens=*" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSDIR=%%i"
if "%VSDIR%"=="" goto novs
if not exist "%VSDIR%\Common7\Tools\VsDevCmd.bat" goto novs
echo Visual Studio: %VSDIR%
call "%VSDIR%\Common7\Tools\VsDevCmd.bat" -arch=amd64 -host_arch=amd64 -no_logo
where cmake >nul 2>&1
if errorlevel 1 goto nocmake
echo.
echo Configuring x64-Release (the first time downloads SDL3 and the libraries: a few minutes)...
cmake --preset x64-Release
if errorlevel 1 goto failed
echo.
echo Building...
cmake --build --preset x64-Release
if errorlevel 1 goto failed
echo.
echo Built: out\build\x64-Release (hallway.exe runs from there).
py -3 --version >nul 2>&1
if errorlevel 1 goto trypython
echo.
echo Making the release (tools\make_release.py)...
py -3 "%~dp0make_release.py" --skip-build %*
goto done
:trypython
python -c "import sys" >nul 2>&1
if errorlevel 1 goto nopython
echo.
echo Making the release (tools\make_release.py)...
python "%~dp0make_release.py" --skip-build %*
goto done
:nopython
echo.
echo Python 3 was not found, so the release package was not made: the build above is the result.
echo (For the package, install Python from python.org and 7-Zip, then run this again.)
goto done
:novs
echo Visual Studio with "Desktop development with C++" was not found (vswhere looked for it).
echo Install Visual Studio 2022 Community with that workload, then run this again.
goto done
:nocmake
echo Visual Studio's CMake was not found: in the Visual Studio Installer, Modify, and tick
echo "C++ CMake tools for Windows" (part of "Desktop development with C++").
goto done
:failed
echo.
echo The build failed: the messages above say why.
:done
echo.
pause
