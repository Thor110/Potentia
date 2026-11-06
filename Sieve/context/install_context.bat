@echo off
rem Adds Sieve to the right-click menu of every .sieve file, and of every other archive Sieve
rem extracts (a .zip, .7z, .rar, .tar.gz ...: the list sieve-install gives, so a kind added to
rem Sieve is taken up by running this again), for this user only (context\README.md): a Sieve
rem submenu with "Extract here" and "Extract to NAME\", as 7-Zip's are. Each opens sieve-install,
rem which shows what it will install and where, and its progress while it works. With sieve-view
rem beside sieve-install (3 November 2026), a .simage opens in it on a double-click, and the Sieve
rem submenu of .simage and .sieve files has "View pictures" too. Nothing else on the
rem machine is changed; uninstall_context.bat takes it away again.
rem
rem   install_context.bat                              sieve-install from this installation's
rem                                                    tools\, else from the Release build
rem   install_context.bat C:\Sieve\tools\sieve-install.exe   that one (sieve-view beside it)
setlocal
set "INSTALLER=%~1"
if "%INSTALLER%"=="" if exist "%~dp0..\tools\sieve-install.exe" set "INSTALLER=%~dp0..\tools\sieve-install.exe"
if "%INSTALLER%"=="" set "INSTALLER=%~dp0..\out\build\x64-Release\sieve-install.exe"
for %%F in ("%INSTALLER%") do set "INSTALLER=%%~fF"
if not exist "%INSTALLER%" goto missing
for %%F in ("%INSTALLER%") do set "VIEWER=%%~dpFsieve-view.exe"
if not exist "%VIEWER%" set "VIEWER="
call :add .sieve view
call :add .simage view
set /a COUNT=0
for /f "usebackq delims=" %%E in (`"%INSTALLER%" --archive-extensions`) do (
    call :add %%E
    set /a COUNT+=1
)
echo Added Sieve to the right-click menu of .sieve and .simage files and %COUNT% kinds of archive,
echo using
echo   %INSTALLER%
if "%VIEWER%"=="" goto noviewer
rem A .simage (Sieve instructions holding pictures) opens in sieve-view.
set "CLASSES=HKCU\Software\Classes"
reg add "%CLASSES%\Sieve.Picture" /ve /t REG_SZ /d "Sieve picture" /f > nul
reg add "%CLASSES%\Sieve.Picture\DefaultIcon" /ve /t REG_SZ /d "\"%VIEWER%\",0" /f > nul
reg add "%CLASSES%\Sieve.Picture\shell\open\command" /ve /t REG_SZ /d "\"%VIEWER%\" \"%%1\"" /f > nul
reg add "%CLASSES%\.simage" /ve /t REG_SZ /d "Sieve.Picture" /f > nul
reg add "%CLASSES%\.simage\OpenWithProgids" /v Sieve.Picture /t REG_NONE /f > nul
echo .simage files open in
echo   %VIEWER%
rem And their thumbnails in Explorer are their pictures (5 November 2026): sieve-thumbs.dll beside
rem sieve-view, a thumbnail handler (the class below), named in .simage's ShellEx for thumbnails.
for %%F in ("%VIEWER%") do set "THUMBS=%%~dpFsieve-thumbs.dll"
if not exist "%THUMBS%" goto nothumbs
set "CLSID={DEA4E1A4-F2CC-427D-BD89-4A2E0231EB4B}"
reg add "%CLASSES%\CLSID\%CLSID%" /ve /t REG_SZ /d "Sieve picture thumbnails" /f > nul
reg add "%CLASSES%\CLSID\%CLSID%\InprocServer32" /ve /t REG_SZ /d "%THUMBS%" /f > nul
reg add "%CLASSES%\CLSID\%CLSID%\InprocServer32" /v ThreadingModel /t REG_SZ /d "Apartment" /f > nul
reg add "%CLASSES%\.simage\ShellEx\{e357fccd-a995-4576-b01f-234630154e96}" /ve /t REG_SZ /d "%CLSID%" /f > nul
rem (No program icon drawn over the picture, as photos have none (Edward, 5 November 2026): Explorer
rem marks a thumbnail with its program's icon unless the class's TypeOverlay is empty. Taken away
rem with the class by uninstall_context.bat.)
reg add "%CLASSES%\Sieve.Picture" /v TypeOverlay /t REG_SZ /d "" /f > nul
echo and show their pictures as thumbnails, by
echo   %THUMBS%
echo (A file Explorer already showed with the icon may keep it; Disk Cleanup's "Thumbnails"
echo clears those.)
goto done
:nothumbs
echo (No sieve-thumbs.dll beside it, so .simage files show the viewer's icon, not their pictures.)
goto done
:noviewer
echo (No sieve-view.exe beside it, so .simage files are not set to open in the picture viewer.)
:done
rem "Create .sieve" on every file and folder (IDEAS 19.100, as Edward decided on 29 October 2026): the
rem hallway's File Locator alone (hallway --create), NAME.sieve saved beside what was chosen once
rem it is read, then closed; stopped or failed, it stays open saying why. On its own, not in the
rem Sieve submenu, so a .sieve file is not given two menus of one name. The hallway is the one in
rem the folder above tools\ (a release), else beside sieve-install (a build).
for %%F in ("%INSTALLER%") do set "HALLWAY=%%~dpF..\hallway.exe"
if not exist "%HALLWAY%" for %%F in ("%INSTALLER%") do set "HALLWAY=%%~dpFhallway.exe"
for %%F in ("%HALLWAY%") do set "HALLWAY=%%~fF"
if not exist "%HALLWAY%" goto nocreate
rem (Written out for each, not in a FOR: a FOR takes "*" as file names to look for.)
reg add "HKCU\Software\Classes\*\shell\SieveCreate" /v MUIVerb /t REG_SZ /d "Create .sieve" /f > nul
reg add "HKCU\Software\Classes\*\shell\SieveCreate" /v Icon /t REG_SZ /d "\"%HALLWAY%\",0" /f > nul
reg add "HKCU\Software\Classes\*\shell\SieveCreate\command" /ve /t REG_SZ /d "\"%HALLWAY%\" --create \"%%1\"" /f > nul
reg add "HKCU\Software\Classes\Directory\shell\SieveCreate" /v MUIVerb /t REG_SZ /d "Create .sieve" /f > nul
reg add "HKCU\Software\Classes\Directory\shell\SieveCreate" /v Icon /t REG_SZ /d "\"%HALLWAY%\",0" /f > nul
reg add "HKCU\Software\Classes\Directory\shell\SieveCreate\command" /ve /t REG_SZ /d "\"%HALLWAY%\" --create \"%%1\"" /f > nul
echo "Create .sieve" added for every file and folder, by
echo   %HALLWAY%
goto created
:nocreate
echo (No hallway.exe found above or beside it, so "Create .sieve" is not added.)
:created
rem "Compress to" on every file and folder (IDEAS 19.100): a submenu of the formats people already use,
rem each made by sieve compress (sieve help compress) at its strongest usual setting and read back
rem before it is kept, NAME.EXT saved beside what was chosen, in a console window that says what it
rem made (and stays open, saying why, where it fails). Several chosen go into one, as for "Create
rem .sieve" (--gather). sieve.exe is the one beside sieve-install (tools\ in a release; the release's
rem own sieve.exe is the setup program, not this).
for %%F in ("%INSTALLER%") do set "SIEVE=%%~dpFsieve.exe"
if not exist "%SIEVE%" goto nocompress
call :compress "HKCU\Software\Classes\*\shell\SieveCompress"
call :compress "HKCU\Software\Classes\Directory\shell\SieveCompress"
echo "Compress to" added for every file and folder, by
echo   %SIEVE%
goto compressed
:nocompress
echo (No sieve.exe beside sieve-install, so "Compress to" is not added.)
:compressed
rem Explorer told the kinds of file changed (SHChangeNotify, SHCNE_ASSOCCHANGED), as installers
rem tell it, so it takes up the handlers now rather than when it next starts (Edward, 5 November
rem 2026: the thumbnails showed only once Explorer was restarted). By PowerShell, which every
rem Windows 10 and 11 has; where it fails, signing out and in again does the same.
powershell -NoProfile -NonInteractive -Command "Add-Type -Namespace SieveShell -Name Notify -MemberDefinition '[DllImport(\"shell32.dll\")] public static extern void SHChangeNotify(int e, uint f, System.IntPtr a, System.IntPtr b);'; [SieveShell.Notify]::SHChangeNotify(0x08000000, 0, [IntPtr]::Zero, [IntPtr]::Zero)" > nul 2>&1
echo (On Windows 11 the menu is under "Show more options", or Shift+right-click.)
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
if "%~2"=="" exit /b 0
if "%VIEWER%"=="" exit /b 0
reg add "%KEY%\shell\3view" /ve /t REG_SZ /d "View pictures" /f > nul
reg add "%KEY%\shell\3view\command" /ve /t REG_SZ /d "\"%VIEWER%\" \"%%1\"" /f > nul
exit /b 0
:compress
reg add "%~1" /v MUIVerb /t REG_SZ /d "Compress to" /f > nul
reg add "%~1" /v SubCommands /t REG_SZ /d "" /f > nul
reg add "%~1" /v Icon /t REG_SZ /d "\"%SIEVE%\",0" /f > nul
reg add "%~1\shell\01zip" /v MUIVerb /t REG_SZ /d "zip" /f > nul
reg add "%~1\shell\01zip\command" /ve /t REG_SZ /d "\"%SIEVE%\" compress \"%%1\" --format zip --gather --from-menu" /f > nul
reg add "%~1\shell\027z" /v MUIVerb /t REG_SZ /d "7z" /f > nul
reg add "%~1\shell\027z\command" /ve /t REG_SZ /d "\"%SIEVE%\" compress \"%%1\" --format 7z --gather --from-menu" /f > nul
reg add "%~1\shell\03xz" /v MUIVerb /t REG_SZ /d "xz (.tar.xz for folders)" /f > nul
reg add "%~1\shell\03xz\command" /ve /t REG_SZ /d "\"%SIEVE%\" compress \"%%1\" --format xz --gather --from-menu" /f > nul
reg add "%~1\shell\04gz" /v MUIVerb /t REG_SZ /d "gzip (.gz)" /f > nul
reg add "%~1\shell\04gz\command" /ve /t REG_SZ /d "\"%SIEVE%\" compress \"%%1\" --format gz --gather --from-menu" /f > nul
reg add "%~1\shell\05bz2" /v MUIVerb /t REG_SZ /d "bzip2 (.bz2)" /f > nul
reg add "%~1\shell\05bz2\command" /ve /t REG_SZ /d "\"%SIEVE%\" compress \"%%1\" --format bz2 --gather --from-menu" /f > nul
reg add "%~1\shell\06zst" /v MUIVerb /t REG_SZ /d "zstd (.zst)" /f > nul
reg add "%~1\shell\06zst\command" /ve /t REG_SZ /d "\"%SIEVE%\" compress \"%%1\" --format zst --gather --from-menu" /f > nul
reg add "%~1\shell\07lz4" /v MUIVerb /t REG_SZ /d "lz4" /f > nul
reg add "%~1\shell\07lz4\command" /ve /t REG_SZ /d "\"%SIEVE%\" compress \"%%1\" --format lz4 --gather --from-menu" /f > nul
reg add "%~1\shell\08br" /v MUIVerb /t REG_SZ /d "brotli (.br)" /f > nul
reg add "%~1\shell\08br\command" /ve /t REG_SZ /d "\"%SIEVE%\" compress \"%%1\" --format br --gather --from-menu" /f > nul
reg add "%~1\shell\09lzma" /v MUIVerb /t REG_SZ /d "lzma (xz's older format)" /f > nul
reg add "%~1\shell\09lzma\command" /ve /t REG_SZ /d "\"%SIEVE%\" compress \"%%1\" --format lzma --gather --from-menu" /f > nul
reg add "%~1\shell\10tar" /v MUIVerb /t REG_SZ /d "tar (not compressed)" /f > nul
reg add "%~1\shell\10tar\command" /ve /t REG_SZ /d "\"%SIEVE%\" compress \"%%1\" --format tar --gather --from-menu" /f > nul
exit /b 0
:missing
echo sieve-install.exe is not at %INSTALLER%
echo Give its path, e.g. install_context.bat C:\Sieve\tools\sieve-install.exe
exit /b 1
