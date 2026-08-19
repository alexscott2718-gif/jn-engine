@echo off
REM Populate assets\exe\ from your own copy of the game (Windows double-click helper).
REM Drag a game folder or a .iso onto this file, or run it and follow the prompt.
setlocal
cd /d "%~dp0\.."
set "SRC=%~1"
if "%SRC%"=="" set /p SRC=Path to your installed game folder, disc drive, or .iso: 
if "%SRC%"=="" goto :nosrc
where python >nul 2>&1 || (echo Python 3 is required and was not found on PATH. & pause & exit /b 2)
python tools\extract_game_exes.py --source "%SRC%" %2 %3 %4
echo.
pause
exit /b 0
:nosrc
echo No source given. See docs\GAME_FILES.md
pause
exit /b 2
