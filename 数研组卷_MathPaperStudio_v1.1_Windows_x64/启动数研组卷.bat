@echo off
setlocal
cd /d "%~dp0"
if not exist "MathPaperStudio.exe" (
    echo MathPaperStudio.exe was not found in this folder.
    pause
    exit /b 1
)
start "" "MathPaperStudio.exe"
endlocal
