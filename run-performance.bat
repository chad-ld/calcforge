@echo off
echo Running PERFORMANCE-OPTIMIZED CalcForge...

REM Add Qt6 to PATH
set "PATH=C:\Qt\6.9.1\msvc2022_64\bin;%PATH%"

REM Show file info
echo.
echo Performance build info:
for %%F in (build\Release\CalcForge.exe) do (
    echo File size: %%~zF bytes
    echo Location: %%~fF
)

echo.
echo Starting performance-optimized CalcForge...
echo (This should start as fast as Windows Calculator!)
echo.

REM Run the performance build
build\Release\CalcForge.exe

echo.
echo Performance test complete.
pause
