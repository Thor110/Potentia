@echo off
rem Sieve CI, step 06: A file is one unit of the bytes256 line, and its address is its own bytes.
rem Double-click to run this step alone, as ci\run.bat 06 does (in a fresh copy, with the
rem Release build); the window stays open at the end. ci\run.bat runs every step.
call "%~dp0..\run.bat" 06 %*
