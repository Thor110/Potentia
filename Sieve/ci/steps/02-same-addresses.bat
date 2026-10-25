@echo off
rem Sieve CI, step 02: Same addresses on every platform.
rem Double-click to run this step alone, as ci\run.bat 02 does (in a fresh copy, with the
rem Release build); the window stays open at the end. ci\run.bat runs every step.
call "%~dp0..\run.bat" 02 %*
