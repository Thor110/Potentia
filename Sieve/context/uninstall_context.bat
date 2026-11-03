@echo off
rem Takes Sieve out of the right-click menu (what install_context.bat added): from .sieve files and
rem from every other kind of file it was added to, whatever the list was then; and .simage and
rem .isieve no longer open in sieve-view (Sieve's own kinds of file, so their keys go whole).
setlocal
set "BASE=HKCU\Software\Classes\SystemFileAssociations"
for /f "delims=" %%K in ('reg query "%BASE%" 2^>nul') do reg delete "%%K\shell\Sieve" /f > nul 2>&1
reg delete "HKCU\Software\Classes\Sieve.Picture" /f > nul 2>&1
reg delete "HKCU\Software\Classes\.simage" /f > nul 2>&1
reg delete "HKCU\Software\Classes\.isieve" /f > nul 2>&1
echo Sieve is no longer in the right-click menu, and .simage files no longer open in sieve-view.
