@echo off
rem Takes Sieve out of the right-click menu (what install_context.bat added): from .sieve files and
rem from every other kind of file it was added to, whatever the list was then; and .simage no longer
rem opens in sieve-view (Sieve's own kind of file, so its key goes whole; and .isieve's, which the
rem first version of install_context.bat set, 3 November 2026).
setlocal
set "BASE=HKCU\Software\Classes\SystemFileAssociations"
for /f "delims=" %%K in ('reg query "%BASE%" 2^>nul') do reg delete "%%K\shell\Sieve" /f > nul 2>&1
reg delete "HKCU\Software\Classes\Sieve.Picture" /f > nul 2>&1
reg delete "HKCU\Software\Classes\.simage" /f > nul 2>&1
reg delete "HKCU\Software\Classes\.isieve" /f > nul 2>&1
rem The thumbnail handler (sieve-thumbs.dll) as install_context.bat registered it.
reg delete "HKCU\Software\Classes\CLSID\{DEA4E1A4-F2CC-427D-BD89-4A2E0231EB4B}" /f > nul 2>&1
rem Explorer told the kinds of file changed (SHChangeNotify, SHCNE_ASSOCCHANGED), as installers
rem tell it, so it takes up the handlers now rather than when it next starts (Edward, 5 November
rem 2026: the thumbnails showed only once Explorer was restarted). By PowerShell, which every
rem Windows 10 and 11 has; where it fails, signing out and in again does the same.
powershell -NoProfile -NonInteractive -Command "Add-Type -Namespace SieveShell -Name Notify -MemberDefinition '[DllImport(\"shell32.dll\")] public static extern void SHChangeNotify(int e, uint f, System.IntPtr a, System.IntPtr b);'; [SieveShell.Notify]::SHChangeNotify(0x08000000, 0, [IntPtr]::Zero, [IntPtr]::Zero)" > nul 2>&1
echo Sieve is no longer in the right-click menu, and .simage files no longer open in sieve-view or show their pictures.
