@echo off
rem Sieve CI, step 03: Filters judge, count and rank the same on every platform.
rem Double-click to run this step alone, as ci\run.bat 03 does (in a fresh copy, with the
rem Release build); the window stays open at the end. ci\run.bat runs every step.
call "%~dp0..\run.bat" 03 %*
