@echo off
echo Checking CalcForge.exe dependencies...
echo.

REM Try to get dependency information using Windows tools
echo === Attempting to run CalcForge with verbose output ===
cd /d "%~dp0build\Release"

REM Check if we can get any error messages
echo Running CalcForge.exe directly in Release folder...
CalcForge.exe 2>&1
echo Exit code: %ERRORLEVEL%

echo.
echo === Checking for common Qt6 DLLs needed ===
set QTPATH=C:\Qt\6.9.1\msvc2022_64\bin

for %%D in (Qt6Core.dll Qt6Gui.dll Qt6Widgets.dll Qt6Network.dll msvcp140.dll vcruntime140.dll) do (
    if exist "%QTPATH%\%%D" (
        echo FOUND: %%D
    ) else (
        if exist "C:\Windows\System32\%%D" (
            echo FOUND in System32: %%D
        ) else (
            echo MISSING: %%D
        )
    )
)

echo.
echo Press any key to continue...
pause > nul