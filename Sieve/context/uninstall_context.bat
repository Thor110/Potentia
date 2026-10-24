@echo off
rem Takes Sieve out of the right-click menu of .sieve files (what install_context.bat added).
reg delete "HKCU\Software\Classes\SystemFileAssociations\.sieve\shell\Sieve" /f > nul 2>&1
echo Sieve is no longer in the right-click menu of .sieve files.
