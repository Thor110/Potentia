@echo off
rem Adds Sieve to the right-click menu of every .sieve file, and of every other archive Sieve
rem extracts (a .zip, .7z, .rar, .tar.gz ...: the list sieve-install gives, so a kind added to
rem Sieve is taken up by running this again), for this user only (context\README.md): a Sieve
rem submenu with "Extract here" and "Extract to NAME\", as 7-Zip's are. Each opens sieve-install,
rem which shows what it will install and where, and its progress while it works. Nothing else on
rem the machine is changed; uninstall_context.bat takes it away again.
rem
rem   install_context.bat                              sieve-install from this installation's
rem                                                    tools\, else from the Release build
rem   install_context.bat C:\Sieve\tools\sieve-install.exe   that one
setlocal
set "INSTALLER=%~1"
if "%INSTALLER%"=="" if exist "%~dp0..\tools\sieve-install.exe" set "INSTALLER=%~dp0..\tools\sieve-install.exe"
if "%INSTALLER%"=="" set "INSTALLER=%~dp0..\out\build\x64-Release\sieve-install.exe"
for %%F in ("%INSTALLER%") do set "INSTALLER=%%~fF"
if not exist "%INSTALLER%" goto missing
call :add .sieve
set /a COUNT=0
for /f "usebackq delims=" %%E in (`"%INSTALLER%" --archive-extensions`) do (
    call :add %%E
    set /a COUNT+=1
)
echo Added Sieve to the right-click menu of .sieve files and %COUNT% kinds of archive, using
echo   %INSTALLER%
echo (On Windows 11 it is under "Show more options", or Shift+right-click.)
exit /b 0
:add
set "KEY=HKCU\Software\Classes\SystemFileAssociations\%~1\shell\Sieve"
reg add "%KEY%" /v MUIVerb /t REG_SZ /d "Sieve" /f > nul
reg add "%KEY%" /v SubCommands /t REG_SZ /d "" /f > nul
reg add "%KEY%" /v Icon /t REG_SZ /d "\"%INSTALLER%\",0" /f > nul
reg add "%KEY%\shell\1here" /ve /t REG_SZ /d "Extract here" /f > nul
reg add "%KEY%\shell\1here\command" /ve /t REG_SZ /d "\"%INSTALLER%\" --extract-here \"%%1\"" /f > nul
reg add "%KEY%\shell\2folder" /ve /t REG_SZ /d "Extract to a folder of its name" /f > nul
reg add "%KEY%\shell\2folder\command" /ve /t REG_SZ /d "\"%INSTALLER%\" --extract-folder \"%%1\"" /f > nul
exit /b 0
:missing
echo sieve-install.exe is not at %INSTALLER%
echo Give its path, e.g. install_context.bat C:\Sieve\tools\sieve-install.exe
exit /b 1
