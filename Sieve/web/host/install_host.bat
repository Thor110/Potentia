@echo off
rem Registers Sieve as the web plugin's host (IDEAS 19.92, web\README.md), for this user only:
rem writes com.potentia.sieve.json, naming sieve.exe and the extension allowed to start it, and
rem points Chrome and Edge at it. Nothing else on the machine is changed.
rem
rem   install_host.bat                      sieve.exe from this installation's tools\, else
rem                                         from the Release build (out\build\x64-Release)
rem   install_host.bat C:\path\to\sieve.exe  that sieve.exe
setlocal
set "SIEVE=%~1"
if "%SIEVE%"=="" if exist "%~dp0..\..\tools\sieve.exe" set "SIEVE=%~dp0..\..\tools\sieve.exe"
if "%SIEVE%"=="" set "SIEVE=%~dp0..\..\out\build\x64-Release\sieve.exe"
for %%F in ("%SIEVE%") do set "SIEVE=%%~fF"
if not exist "%SIEVE%" (
  echo sieve.exe is not at %SIEVE%: give its path, e.g. install_host.bat C:\Sieve\tools\sieve.exe
  exit /b 1
)
set "DIR=%LOCALAPPDATA%\Sieve"
if not exist "%DIR%" mkdir "%DIR%"
set "JSON=%DIR%\com.potentia.sieve.json"
set "ESCAPED=%SIEVE:\=\\%"
rem (One line at a time: a ")" in the path, as in "Program Files (x86)", would end a bracketed block.)
> "%JSON%" echo {
>> "%JSON%" echo   "name": "com.potentia.sieve",
>> "%JSON%" echo   "description": "Sieve: decodes the .sieve files a page links to, for the Sieve extension",
>> "%JSON%" echo   "path": "%ESCAPED%",
>> "%JSON%" echo   "type": "stdio",
>> "%JSON%" echo   "allowed_origins": ["chrome-extension://bnigebolckfoapimidlcmbilcbhjbmjg/"]
>> "%JSON%" echo }
reg add "HKCU\Software\Google\Chrome\NativeMessagingHosts\com.potentia.sieve" /ve /t REG_SZ /d "%JSON%" /f > nul
reg add "HKCU\Software\Microsoft\Edge\NativeMessagingHosts\com.potentia.sieve" /ve /t REG_SZ /d "%JSON%" /f > nul
echo Registered %SIEVE%
echo   as com.potentia.sieve (%JSON%), for Chrome and Edge.
echo Now load web\extension in chrome://extensions (Developer mode, Load unpacked).
