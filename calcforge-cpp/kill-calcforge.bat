@echo off
echo Killing all CalcForge.exe processes...

REM Check if any CalcForge processes are running
tasklist | findstr /i "CalcForge.exe" > nul
if %ERRORLEVEL% EQU 0 (
    echo Found running CalcForge processes:
    tasklist | findstr /i "CalcForge.exe"
    echo.
    echo Killing all CalcForge.exe processes...
    taskkill /f /im CalcForge.exe /t
    echo.
    echo Waiting 2 seconds for processes to terminate...
    timeout /t 2 /nobreak > nul
    echo.
    echo Checking if any CalcForge processes remain...
    tasklist | findstr /i "CalcForge.exe" > nul
    if %ERRORLEVEL% EQU 0 (
        echo WARNING: Some CalcForge processes may still be running:
        tasklist | findstr /i "CalcForge.exe"
    ) else (
        echo All CalcForge processes terminated successfully.
    )
) else (
    echo No CalcForge processes found running.
)

echo.
echo Press any key to continue...
pause > nul