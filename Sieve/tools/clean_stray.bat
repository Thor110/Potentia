@echo off
rem Removes the stray files from Sieve's folder: leftovers from before the rename from Potentia to
rem Sieve, and the scratch the checks in .github/workflows/build.yml write into the working folder.
rem Every file is named here; nothing is matched by pattern, so nothing else can be touched.
rem
rem Inside a Git repository each file is also taken out of the index (git rm --cached), since a
rem file Git already tracks stays tracked whatever .gitignore says; commit afterwards to record it.
rem
rem It finds the Sieve folder itself, whether it sits in Sieve\tools or in Sieve, and refuses to
rem run anywhere that is not Sieve (a folder whose CMakeLists.txt declares project(Sieve ...)).
setlocal enabledelayedexpansion
set "HERE=%~dp0"
if exist "%HERE%CMakeLists.txt" (
    cd /d "%HERE%"
) else (
    cd /d "%HERE%.."
)
findstr /c:"project(Sieve" CMakeLists.txt >nul 2>&1
if errorlevel 1 (
    echo This is not the Sieve folder: %CD%
    echo Put clean_stray.bat in Sieve or Sieve\tools and run it again. Nothing was changed.
    pause
    exit /b 1
)
echo Cleaning %CD%
echo.

set GIT=0
git rev-parse --is-inside-work-tree >nul 2>&1 && set GIT=1

rem Leftovers from before the rename to Sieve.
for %%F in (
    "tools\potentia_cli.cpp"
    "reference\potentia_ref.py"
    "client\crate_faces.cpp"
    "results\m1_sieve_gcide.csv"
    "results\m1_sieve_gcide.png"
) do call :remove %%F
call :remove_dir "core\include\potentia"

rem Scratch from the checks, in the Sieve folder itself.
for %%F in (
    "again.book" "positional.book" "scrambled.book" "tampered.book"
    "all.bin" "all.hex" "allback.bin" "back.bin" "back2.bin" "big.bin" "big.hex" "four.bin"
    "cube.obj" "round.obj" "viatext.obj" "obj.txt" "objcrlf.txt"
    "oracle.model" "rebuilt.model"
    "positional.txt" "scrambled.txt" "unbound.txt"
    "agree.ini" "app.ini" "books.ini" "lines.ini" "mark.ini" "mixed.ini" "words2.ini"
    "reference\book.out"
) do call :remove %%F

echo.
if %GIT%==1 (
    echo Done. The removals are staged: check "git status", then commit them.
) else (
    echo Done.
)
pause
exit /b 0

:remove
if not exist "%~1" exit /b 0
if %GIT%==1 git rm --cached --quiet --ignore-unmatch -- "%~1" >nul 2>&1
del /q "%~1" && echo   removed %~1
exit /b 0

:remove_dir
if not exist "%~1\" exit /b 0
if %GIT%==1 git rm -r --cached --quiet --ignore-unmatch -- "%~1" >nul 2>&1
rmdir /s /q "%~1" && echo   removed %~1\
exit /b 0
