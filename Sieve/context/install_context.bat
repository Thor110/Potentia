@echo off
rem Adds Sieve to the right-click menu of every .sieve file, for this user only (context\README.md):
rem a Sieve submenu with "Extract here" and "Extract to NAME\", as 7-Zip's are. Each opens
rem sieve-install, which shows what it will install and where, and its progress while it works.
rem Nothing else on the machine is changed; uninstall_context.bat takes it away again.
rem
rem   install_context.bat                              sieve-install from the Release build
rem   install_context.bat C:\Sieve\tools\sieve-install.exe   that one (an installation's tools\)
setlocal
set "INSTALLER=%~1"
if "%INSTALLER%"=="" set "INSTALLER=%~dp0..\out\build\x64-Release\sieve-install.exe"
for %%F in ("%INSTALLER%") do set "INSTALLER=%%~fF"
if not exist "%INSTALLER%" goto missing
set "KEY=HKCU\Software\Classes\SystemFileAssociations\.sieve\shell\Sieve"
reg add "%KEY%" /v MUIVerb /t REG_SZ /d "Sieve" /f > nul
reg add "%KEY%" /v SubCommands /t REG_SZ /d "" /f > nul
reg add "%KEY%" /v Icon /t REG_SZ /d "\"%INSTALLER%\",0" /f > nul
reg add "%KEY%\shell\1here" /ve /t REG_SZ /d "Extract here" /f > nul
reg add "%KEY%\shell\1here\command" /ve /t REG_SZ /d "\"%INSTALLER%\" --extract-here \"%%1\"" /f > nul
reg add "%KEY%\shell\2folder" /ve /t REG_SZ /d "Extract to a folder of its name" /f > nul
reg add "%KEY%\shell\2folder\command" /ve /t REG_SZ /d "\"%INSTALLER%\" --extract-folder \"%%1\"" /f > nul
echo Added Sieve to the right-click menu of .sieve files, using
echo   %INSTALLER%
echo (On Windows 11 it is under "Show more options", or Shift+right-click.)
exit /b 0
:missing
echo sieve-install.exe is not at %INSTALLER%
echo Give its path, e.g. install_context.bat C:\Sieve\tools\sieve-install.exe
exit /b 1
