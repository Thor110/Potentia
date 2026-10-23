@echo off
rem Undoes install_host.bat: the two registry keys and the JSON file it wrote.
reg delete "HKCU\Software\Google\Chrome\NativeMessagingHosts\com.potentia.sieve" /f > nul 2>&1
reg delete "HKCU\Software\Microsoft\Edge\NativeMessagingHosts\com.potentia.sieve" /f > nul 2>&1
del "%LOCALAPPDATA%\Sieve\com.potentia.sieve.json" > nul 2>&1
echo Sieve is no longer registered as the web plugin's host.
