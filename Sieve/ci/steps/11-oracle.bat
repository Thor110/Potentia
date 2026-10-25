@echo off
rem Sieve CI, step 11: Reference oracle reproduces the committed vectors.
rem Double-click to run this step alone, as ci\run.bat 11 does (in a fresh copy, with the
rem Release build); the window stays open at the end. ci\run.bat runs every step.
call "%~dp0..\run.bat" 11 %*
