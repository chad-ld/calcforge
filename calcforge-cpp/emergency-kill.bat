@echo off
echo Emergency CalcForge process termination...
taskkill /f /im CalcForge.exe /t 2>nul
echo Processes killed.
echo Deleting massive log files to free disk space...
if exist ".\build\Release\logs\calcforge_2025-07-29_09-39-14.log" (
    del ".\build\Release\logs\calcforge_2025-07-29_09-39-14.log"
    echo Deleted massive log file.
)
if exist ".\build\Release\logs\calcforge_2025-07-29_09-28-29.log" (
    del ".\build\Release\logs\calcforge_2025-07-29_09-28-29.log"
    echo Deleted another massive log file.
)
echo Done.